/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

This file is part of the Angie Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "RenderLocal.h"
#include "SSAORenderer.h"

#include <Core/Public/Random.h>

ARuntimeVariable r_HBAODeinterleaved( _CTS( "r_HBAODeinterleaved" ), _CTS( "1" ) );
ARuntimeVariable r_HBAOBlur( _CTS( "r_HBAOBlur" ), _CTS( "1" ) );
ARuntimeVariable r_HBAORadius( _CTS( "r_HBAORadius" ), _CTS( "2" ) );
ARuntimeVariable r_HBAOBias( _CTS( "r_HBAOBias" ), _CTS( "0.1" ) );
ARuntimeVariable r_HBAOPowExponent( _CTS( "r_HBAOPowExponent" ), _CTS( "1.5" ) );

using namespace RenderCore;

ASSAORenderer::ASSAORenderer()
{
    SPipelineResourceLayout resourceLayout;

    SSamplerInfo nearestSampler;

    nearestSampler.Filter = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    SSamplerInfo pipeSamplers[3];

    // Linear depth sampler
    pipeSamplers[0] = nearestSampler;

    // Normal texture sampler
    pipeSamplers[1] = nearestSampler;

    // Random map sampler
    pipeSamplers[2].Filter = FILTER_NEAREST;
    pipeSamplers[2].AddressU = SAMPLER_ADDRESS_WRAP;
    pipeSamplers[2].AddressV = SAMPLER_ADDRESS_WRAP;
    pipeSamplers[2].AddressW = SAMPLER_ADDRESS_WRAP;

    SBufferInfo bufferInfo[2];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    resourceLayout.NumBuffers = 2;
    resourceLayout.Buffers = bufferInfo;

    resourceLayout.NumSamplers = AN_ARRAY_SIZE( pipeSamplers );
    resourceLayout.Samplers = pipeSamplers;

    CreateFullscreenQuadPipeline( &Pipe, "postprocess/ssao/ssao.vert", "postprocess/ssao/simple.frag", &resourceLayout );
    CreateFullscreenQuadPipeline( &Pipe_ORTHO, "postprocess/ssao/ssao.vert", "postprocess/ssao/simple_ortho.frag", &resourceLayout );

    SSamplerInfo cacheAwareSamplers[2];

    // Deinterleave depth array sampler
    cacheAwareSamplers[0] = nearestSampler;
    // Normal texture sampler
    cacheAwareSamplers[1] = nearestSampler;

    resourceLayout.NumSamplers = AN_ARRAY_SIZE( cacheAwareSamplers );
    resourceLayout.Samplers = cacheAwareSamplers;

    CreateFullscreenQuadPipelineGS( &CacheAwarePipe, "postprocess/ssao/ssao.vert", "postprocess/ssao/deinterleaved.frag", "postprocess/ssao/deinterleaved.geom", &resourceLayout );
    CreateFullscreenQuadPipelineGS( &CacheAwarePipe_ORTHO, "postprocess/ssao/ssao.vert", "postprocess/ssao/deinterleaved_ortho.frag", "postprocess/ssao/deinterleaved.geom", &resourceLayout );

    SSamplerInfo blurSamplers[2];

    // SSAO texture sampler
    blurSamplers[0].Filter = FILTER_LINEAR;
    blurSamplers[0].AddressU = SAMPLER_ADDRESS_CLAMP;
    blurSamplers[0].AddressV = SAMPLER_ADDRESS_CLAMP;
    blurSamplers[0].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Linear depth sampler
    blurSamplers[1] = nearestSampler;

    resourceLayout.NumSamplers = AN_ARRAY_SIZE( blurSamplers );
    resourceLayout.Samplers = blurSamplers;

    CreateFullscreenQuadPipeline( &BlurPipe, "postprocess/ssao/blur.vert", "postprocess/ssao/blur.frag", &resourceLayout );

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &nearestSampler;

    CreateFullscreenQuadPipeline( &DeinterleavePipe, "postprocess/ssao/deinterleave.vert", "postprocess/ssao/deinterleave.frag", &resourceLayout );

    resourceLayout.NumBuffers = 0;
    CreateFullscreenQuadPipeline( &ReinterleavePipe, "postprocess/ssao/reinterleave.vert", "postprocess/ssao/reinterleave.frag", &resourceLayout );
   
    AMersenneTwisterRand rng( 0u );

    const float NUM_DIRECTIONS = 8;

    Float3 hbaoRandom[HBAO_RANDOM_ELEMENTS];

    for ( int i = 0 ; i < HBAO_RANDOM_ELEMENTS ; i++ ) {
        const float r1 = rng.GetFloat();
        const float r2 = rng.GetFloat();

        // Random rotation angles in [0,2PI/NUM_DIRECTIONS)
        const float angle = Math::_2PI * r1 / NUM_DIRECTIONS;

        float s, c;

        Math::SinCos( angle, s, c );

        hbaoRandom[i].X = c;
        hbaoRandom[i].Y = s;
        hbaoRandom[i].Z = r2;

        StdSwap( hbaoRandom[i].X, hbaoRandom[i].Z ); // Swap to BGR
    }

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    GLogger.Printf( "vec2( %f, %f ),\n", float( i % 4 ) + 0.5f, float( i / 4 ) + 0.5f );
    //}

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    GLogger.Printf( "vec4( %f, %f, %f, 0.0 ),\n", hbaoRandom[i].X, hbaoRandom[i].Y, hbaoRandom[i].Z );
    //}

    GDevice->CreateTexture( RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_RGB16F, STextureResolution2D( HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE ) ), &RandomMap );
    RandomMap->Write( 0, RenderCore::FORMAT_FLOAT3, sizeof( hbaoRandom ), 1, hbaoRandom );
}

void ASSAORenderer::ResizeAO( int Width, int Height )
{
    if ( AOWidth != Width
         || AOHeight != Height ) {

        AOWidth = Width;
        AOHeight = Height;

        AOQuarterWidth = ((AOWidth+3)/4);
        AOQuarterHeight = ((AOHeight+3)/4);

        GDevice->CreateTexture(
            RenderCore::MakeTexture( TEXTURE_FORMAT_R32F,
                                     RenderCore::STextureResolution2DArray( AOQuarterWidth, AOQuarterHeight, HBAO_RANDOM_ELEMENTS )
            ), &SSAODeinterleaveDepthArray
        );

        for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
            RenderCore::STextureViewCreateInfo textureCI = {};
            textureCI.Type = RenderCore::TEXTURE_2D;
            textureCI.Format = RenderCore::TEXTURE_FORMAT_R32F;
            textureCI.pOriginalTexture = SSAODeinterleaveDepthArray;
            textureCI.MinLod = 0;
            textureCI.NumLods = 1;
            textureCI.MinLayer = i;
            textureCI.NumLayers = 1;
            GDevice->CreateTextureView( textureCI, &SSAODeinterleaveDepthView[i] );
        }
    }
}

void ASSAORenderer::AddDeinterleaveDepthPass( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth, AFrameGraphTexture ** ppDeinterleaveDepthArray )
{
    AFrameGraphTexture * SSAODeinterleaveDepthView_R[16];
    for ( int i = 0 ; i < 16 ; i++ ) {
        SSAODeinterleaveDepthView_R[i] = FrameGraph.AddExternalResource< RenderCore::STextureCreateInfo, RenderCore::ITexture >(
            Core::Fmt( "Deinterleave Depth View %d", i ),
            RenderCore::STextureCreateInfo(),
            SSAODeinterleaveDepthView[i] );
    }

    ARenderPass & deinterleavePass = FrameGraph.AddTask< ARenderPass >( "Deinterleave Depth Pass" );
    deinterleavePass.SetRenderArea( AOQuarterWidth, AOQuarterHeight );
    deinterleavePass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    deinterleavePass.SetColorAttachments(
    {
        { SSAODeinterleaveDepthView_R[0], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[1], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[2], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[3], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[4], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[5], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[6], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[7], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
    } );
    deinterleavePass.AddSubpass( { 0,1,2,3,4,5,6,7 }, // color attachment refs
                                 [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        struct SDrawCall
        {
            Float2 UVOffset;
            Float2 InvFullResolution;
        };

        SDrawCall * drawCall = MapDrawCallConstants< SDrawCall >();
        drawCall->UVOffset.X = 0.5f;
        drawCall->UVOffset.Y = 0.5f;
        drawCall->InvFullResolution.X = 1.0f / AOWidth;
        drawCall->InvFullResolution.Y = 1.0f / AOHeight;

        rtbl->BindTexture( 0, LinearDepth->Actual() );

        DrawSAQ( DeinterleavePipe );
    } );

    ARenderPass & deinterleavePass2 = FrameGraph.AddTask< ARenderPass >( "Deinterleave Depth Pass 2" );
    deinterleavePass2.SetRenderArea( AOQuarterWidth, AOQuarterHeight );
    deinterleavePass2.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    deinterleavePass2.SetColorAttachments(
    {
        { SSAODeinterleaveDepthView_R[8], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[9], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[10], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[11], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[12], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[13], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[14], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[15], RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE ) },
    } );
    deinterleavePass2.AddSubpass( { 0,1,2,3,4,5,6,7 }, // color attachment refs
                                  [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        struct SDrawCall
        {
            Float2 UVOffset;
            Float2 InvFullResolution;
        };

        SDrawCall * drawCall = MapDrawCallConstants< SDrawCall >();
        drawCall->UVOffset.X = float( 8 % 4 ) + 0.5f;
        drawCall->UVOffset.Y = float( 8 / 4 ) + 0.5f;
        drawCall->InvFullResolution.X = 1.0f / AOWidth;
        drawCall->InvFullResolution.Y = 1.0f / AOHeight;

        rtbl->BindTexture( 0, LinearDepth->Actual() );

        DrawSAQ( DeinterleavePipe );
    } );

    AFrameGraphTexture * DeinterleaveDepthArray_R = FrameGraph.AddExternalResource< RenderCore::STextureCreateInfo, RenderCore::ITexture >(
        "Deinterleave Depth Array",
        RenderCore::STextureCreateInfo(),
        SSAODeinterleaveDepthArray );

    *ppDeinterleaveDepthArray = DeinterleaveDepthArray_R;
}

void ASSAORenderer::AddCacheAwareAOPass( AFrameGraph & FrameGraph, AFrameGraphTexture * DeinterleaveDepthArray, AFrameGraphTexture * NormalTexture, AFrameGraphTexture ** ppSSAOTextureArray )
{
    ARenderPass & cacheAwareAO = FrameGraph.AddTask< ARenderPass >( "Cache Aware AO Pass" );
    cacheAwareAO.SetRenderArea( AOQuarterWidth, AOQuarterHeight );
    cacheAwareAO.AddResource( DeinterleaveDepthArray, RESOURCE_ACCESS_READ );
    cacheAwareAO.AddResource( NormalTexture, RESOURCE_ACCESS_READ );
    cacheAwareAO.SetColorAttachments(
    {
        {
            "SSAO Texture Array",
            RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_R8, RenderCore::STextureResolution2DArray( AOQuarterWidth, AOQuarterHeight, HBAO_RANDOM_ELEMENTS ) ),
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    cacheAwareAO.AddSubpass( { 0 }, // color attachment refs
                             [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        struct SDrawCall
        {
            float Bias;
            float FallofFactor;
            float RadiusToScreen;
            float PowExponent;
            float Multiplier;
            float Pad;
            Float2 InvFullResolution;
            Float2 InvQuarterResolution;
        };

        SDrawCall * drawCall = MapDrawCallConstants< SDrawCall >();

        float projScale;

        if ( GRenderView->bPerspective ) {
            projScale = (float)AOHeight / std::tan( GRenderView->ViewFovY * 0.5f ) * 0.5f;
        }
        else {
            projScale = (float)AOHeight * GRenderView->ProjectionMatrix[1][1] * 0.5f;
        }

        drawCall->Bias = r_HBAOBias.GetFloat();
        drawCall->FallofFactor = -1.0f / (r_HBAORadius.GetFloat() * r_HBAORadius.GetFloat());
        drawCall->RadiusToScreen = r_HBAORadius.GetFloat() * 0.5f * projScale;
        drawCall->PowExponent = r_HBAOPowExponent.GetFloat();
        drawCall->Multiplier = 1.0f / (1.0f - r_HBAOBias.GetFloat());
        drawCall->InvFullResolution.X = 1.0f / AOWidth;
        drawCall->InvFullResolution.Y = 1.0f / AOHeight;
        drawCall->InvQuarterResolution.X = 1.0f / AOQuarterWidth;
        drawCall->InvQuarterResolution.Y = 1.0f / AOQuarterHeight;

        rtbl->BindTexture( 0, DeinterleaveDepthArray->Actual() );
        rtbl->BindTexture( 1, NormalTexture->Actual() );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( CacheAwarePipe );
        }
        else {
            DrawSAQ( CacheAwarePipe_ORTHO );
        }
    } );

    *ppSSAOTextureArray = cacheAwareAO.GetColorAttachments()[0].Resource;
}

void ASSAORenderer::AddReinterleavePass( AFrameGraph & FrameGraph, AFrameGraphTexture * SSAOTextureArray, AFrameGraphTexture ** ppSSAOTexture )
{
    ARenderPass & reinterleavePass = FrameGraph.AddTask< ARenderPass >( "Reinterleave Pass" );
    reinterleavePass.SetRenderArea( AOWidth, AOHeight );
    reinterleavePass.AddResource( SSAOTextureArray, RESOURCE_ACCESS_READ );
    reinterleavePass.SetColorAttachments(
    {
        {
            "SSAO Texture",
            RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_R8, RenderCore::STextureResolution2D( AOWidth, AOHeight ) ),
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    reinterleavePass.AddSubpass( { 0 }, // color attachment refs
                                 [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        rtbl->BindTexture( 0, SSAOTextureArray->Actual() );

        DrawSAQ( ReinterleavePipe );
    } );

    *ppSSAOTexture = reinterleavePass.GetColorAttachments()[0].Resource;
}

void ASSAORenderer::AddSimpleAOPass( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth, AFrameGraphTexture * NormalTexture, AFrameGraphTexture ** ppSSAOTexture )
{
    AFrameGraphTexture * RandomMapTexture_R = FrameGraph.AddExternalResource< RenderCore::STextureCreateInfo, RenderCore::ITexture >(
        "SSAO Random Map", RenderCore::STextureCreateInfo(), RandomMap );

    ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Simple AO Pass" );
    pass.SetRenderArea( GRenderView->Width, GRenderView->Height );
    pass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    pass.AddResource( NormalTexture, RESOURCE_ACCESS_READ );
    pass.AddResource( RandomMapTexture_R, RESOURCE_ACCESS_READ );
    pass.SetColorAttachments(
    {
        {
            "SSAO Texture (Interleaved)",
            RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_R8, RenderCore::STextureResolution2D( AOWidth, AOHeight ) ),
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    pass.AddSubpass( { 0 }, // color attachment refs
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        struct SDrawCall
        {
            float Bias;
            float FallofFactor;
            float RadiusToScreen;
            float PowExponent;
            float Multiplier;
            float Pad;
            Float2 InvFullResolution;
            Float2 InvQuarterResolution;
        };

        SDrawCall * drawCall = MapDrawCallConstants< SDrawCall >();

        float projScale;

        if ( GRenderView->bPerspective ) {
            projScale = (float)/*AOHeight*/GRenderView->Height / std::tan( GRenderView->ViewFovY * 0.5f ) * 0.5f;
        }
        else {
            projScale = (float)/*AOHeight*/GRenderView->Height * GRenderView->ProjectionMatrix[1][1] * 0.5f;
        }

        drawCall->Bias = r_HBAOBias.GetFloat();
        drawCall->FallofFactor = -1.0f / (r_HBAORadius.GetFloat() * r_HBAORadius.GetFloat());
        drawCall->RadiusToScreen = r_HBAORadius.GetFloat() * 0.5f * projScale;
        drawCall->PowExponent = r_HBAOPowExponent.GetFloat();
        drawCall->Multiplier = 1.0f / (1.0f - r_HBAOBias.GetFloat());
        drawCall->InvFullResolution.X = 1.0f / GRenderView->Width;//AOWidth;
        drawCall->InvFullResolution.Y = 1.0f / GRenderView->Height;//AOHeight;
        drawCall->InvQuarterResolution.X = 0; // don't care
        drawCall->InvQuarterResolution.Y = 0; // don't care

        rtbl->BindTexture( 0, LinearDepth->Actual() );
        rtbl->BindTexture( 1, NormalTexture->Actual() );
        rtbl->BindTexture( 2, RandomMapTexture_R->Actual() );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( Pipe );
        }
        else {
            DrawSAQ( Pipe_ORTHO );
        }
    } );

    *ppSSAOTexture = pass.GetColorAttachments()[0].Resource;
}

void ASSAORenderer::AddAOBlurPass( AFrameGraph & FrameGraph, AFrameGraphTexture * SSAOTexture, AFrameGraphTexture * LinearDepth, AFrameGraphTexture ** ppBluredSSAO )
{
    ARenderPass & aoBlurXPass = FrameGraph.AddTask< ARenderPass >( "AO Blur X Pass" );
    aoBlurXPass.SetRenderArea( GRenderView->Width, GRenderView->Height );
    aoBlurXPass.SetColorAttachments(
    {
        {
            "Temp SSAO Texture (Blur X)",
            RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_R8, RenderCore::STextureResolution2D( AOWidth, AOHeight ) ),
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    aoBlurXPass.AddResource( SSAOTexture, RESOURCE_ACCESS_READ );
    aoBlurXPass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    aoBlurXPass.AddSubpass( { 0 }, // color attachment refs
                            [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        struct SDrawCall
        {
            Float2 InvSize;
        };

        SDrawCall * drawCall = MapDrawCallConstants< SDrawCall >();
        drawCall->InvSize.X = 1.0f / RenderPass.GetRenderArea().Width;
        drawCall->InvSize.Y = 0;

        // SSAO blur X
        rtbl->BindTexture( 0, SSAOTexture->Actual() );
        rtbl->BindTexture( 1, LinearDepth->Actual() );

        DrawSAQ( BlurPipe );
    } );

    AFrameGraphTexture * TempSSAOTextureBlurX = aoBlurXPass.GetColorAttachments()[0].Resource;

    ARenderPass & aoBlurYPass = FrameGraph.AddTask< ARenderPass >( "AO Blur Y Pass" );
    aoBlurYPass.SetRenderArea( GRenderView->Width, GRenderView->Height );
    aoBlurYPass.SetColorAttachments(
    {
        {
            "Blured SSAO Texture",
            RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_R8, RenderCore::STextureResolution2D( AOWidth, AOHeight ) ),
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    aoBlurYPass.AddResource( TempSSAOTextureBlurX, RESOURCE_ACCESS_READ );
    aoBlurYPass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    aoBlurYPass.AddSubpass( { 0 }, // color attachment refs
                            [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        struct SDrawCall
        {
            Float2 InvSize;
        };

        SDrawCall * drawCall = MapDrawCallConstants< SDrawCall >();
        drawCall->InvSize.X = 0;
        drawCall->InvSize.Y = 1.0f / RenderPass.GetRenderArea().Height;

        // SSAO blur Y
        rtbl->BindTexture( 0, TempSSAOTextureBlurX->Actual() );
        rtbl->BindTexture( 1, LinearDepth->Actual() );

        DrawSAQ( BlurPipe );
    } );

    *ppBluredSSAO = aoBlurYPass.GetColorAttachments()[0].Resource;
}

void ASSAORenderer::AddPasses( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth, AFrameGraphTexture * NormalTexture, AFrameGraphTexture ** ppSSAOTexture )
{
    ResizeAO( GFrameData->RenderTargetMaxWidth, GFrameData->RenderTargetMaxHeight );

    if ( r_HBAODeinterleaved && GRenderView->Width == GFrameData->RenderTargetMaxWidth && GRenderView->Height == GFrameData->RenderTargetMaxHeight ) {
        AFrameGraphTexture * DeinterleaveDepthArray, * SSAOTextureArray;
        
        AddDeinterleaveDepthPass( FrameGraph, LinearDepth, &DeinterleaveDepthArray );
        AddCacheAwareAOPass( FrameGraph, DeinterleaveDepthArray, NormalTexture, &SSAOTextureArray );
        AddReinterleavePass( FrameGraph, SSAOTextureArray, ppSSAOTexture );
    }
    else {
        AddSimpleAOPass( FrameGraph, LinearDepth, NormalTexture, ppSSAOTexture );
    }

    if ( r_HBAOBlur ) {
        AddAOBlurPass( FrameGraph, *ppSSAOTexture, LinearDepth, ppSSAOTexture );
    }
}
