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

#include "OpenGL45SSAORenderer.h"

#include <Core/Public/Random.h>

ARuntimeVariable RVSSAODeinterleaved( _CTS( "SSAODeinterleaved" ), _CTS( "1" ) );
ARuntimeVariable RVSSAOBlur( _CTS( "SSAOBlur" ), _CTS( "1" ) );
ARuntimeVariable RVCheckNearest( _CTS( "CheckNearest" ), _CTS( "1" ) );

using namespace GHI;

namespace OpenGL45 {

ASSAORenderer::ASSAORenderer()
{
    CreateFullscreenQuadPipeline( Pipe, "postprocess/ssao.vert", "postprocess/ssao_simple.frag" );
    CreateFullscreenQuadPipeline( Pipe_ORTHO, "postprocess/ssao.vert", "postprocess/ssao_simple_ortho.frag" );
    CreateFullscreenQuadPipelineGS( CacheAwarePipe, "postprocess/ssao.vert", "postprocess/ssao_deinterleaved.frag", "postprocess/ssao_deinterleaved.geom", GHI::BLENDING_NO_BLEND );
    CreateFullscreenQuadPipelineGS( CacheAwarePipe_ORTHO, "postprocess/ssao.vert", "postprocess/ssao_deinterleaved_ortho.frag", "postprocess/ssao_deinterleaved.geom", GHI::BLENDING_NO_BLEND );
    CreateFullscreenQuadPipeline( BlurPipe, "postprocess/ssao_blur.vert", "postprocess/ssao_blur.frag", GHI::BLENDING_NO_BLEND, NULL, &BlendFragmentShader );
    CreateFullscreenQuadPipeline( DeinterleavePipe, "postprocess/hbao_deinterleave.vert", "postprocess/hbao_deinterleave.frag", GHI::BLENDING_NO_BLEND, NULL, &DeinterleaveFragmentShader );
    CreateFullscreenQuadPipeline( ReinterleavePipe, "postprocess/hbao_reinterleave.vert", "postprocess/hbao_reinterleave.frag" );
   
    CreateSamplers();

    AMersenneTwisterRand rng( 0u );

    float NUM_DIRECTIONS = 8;

    for ( int i = 0 ; i < HBAO_RANDOM_ELEMENTS ; i++ ) {
        float r1 = rng.GetFloat();
        float r2 = rng.GetFloat();

        // Random rotation angles in [0,2PI/NUM_DIRECTIONS)
        float angle = Math::_2PI * r1 / NUM_DIRECTIONS;
        hbaoRandom[i].X = cosf( angle );
        hbaoRandom[i].Y = sinf( angle );
        hbaoRandom[i].Z = r2;
    }

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    GLogger.Printf( "vec2( %f, %f ),\n", float( i % 4 ) + 0.5f, float( i / 4 ) + 0.5f );
    //}

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    GLogger.Printf( "vec4( %f, %f, %f, 0.0 ),\n", hbaoRandom[i].X, hbaoRandom[i].Y, hbaoRandom[i].Z );
    //}

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_2D;
    textureCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RGB16F;
    textureCI.Resolution.Tex2D.Width = HBAO_RANDOM_SIZE;
    textureCI.Resolution.Tex2D.Height = HBAO_RANDOM_SIZE;
    textureCI.NumLods = 1;

    RandomMap.InitializeStorage( textureCI );
    RandomMap.Write( 0, GHI::PIXEL_FORMAT_FLOAT_RGB, sizeof( hbaoRandom ), 1, hbaoRandom );
}

void ASSAORenderer::CreateSamplers()
{
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    DepthSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    LinearDepthSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    NormalSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    BlurSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    NearestSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressV = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressW = SAMPLER_ADDRESS_WRAP;
    RandomMapSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void ASSAORenderer::ResizeAO( int Width, int Height ) {
    if ( AOWidth != Width
         || AOHeight != Height ) {

        AOWidth = Width;
        AOHeight = Height;

        AOQuarterWidth = ((AOWidth+3)/4);
        AOQuarterHeight = ((AOHeight+3)/4);

        {
            GHI::TextureStorageCreateInfo textureCI = {};
            textureCI.Type = GHI::TEXTURE_2D_ARRAY;
            textureCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_R32F;
            textureCI.Resolution.Tex2DArray.Width = AOQuarterWidth;
            textureCI.Resolution.Tex2DArray.Height = AOQuarterHeight;
            textureCI.Resolution.Tex2DArray.NumLayers = HBAO_RANDOM_ELEMENTS;
            textureCI.NumLods = 1;

            SSAODeinterleaveDepthArray.InitializeStorage( textureCI );

            // Sampler: Clamp, Nearest
        }

        for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
            GHI::TextureViewCreateInfo textureCI = {};
            textureCI.Type = GHI::TEXTURE_2D;
            textureCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_R32F;
            textureCI.pOriginalTexture = &SSAODeinterleaveDepthArray;
            textureCI.MinLod = 0;
            textureCI.NumLods = 1;
            textureCI.MinLayer = i;
            textureCI.NumLayers = 1;

            SSAODeinterleaveDepthView[i].InitializeView( textureCI );

            // Sampler: Clamp, Nearest
        }
    }
}

AFrameGraphTextureStorage * ASSAORenderer::AddDeinterleaveDepthPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * LinearDepth )
{
    AFrameGraphTextureStorage * SSAODeinterleaveDepthView_R[16];
    for ( int i = 0 ; i < 16 ; i++ ) {
        SSAODeinterleaveDepthView_R[i] = FrameGraph.AddExternalResource< GHI::TextureStorageCreateInfo, GHI::Texture >(
            Core::Fmt( "Deinterleave Depth View %d", i ),
            GHI::TextureStorageCreateInfo(),
            &SSAODeinterleaveDepthView[i] );
    }

    ARenderPass & deinterleavePass = FrameGraph.AddTask< ARenderPass >( "Deinterleave Depth Pass" );
    deinterleavePass.SetRenderArea( AOQuarterWidth, AOQuarterHeight );
    deinterleavePass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    deinterleavePass.SetColorAttachments(
    {
        { SSAODeinterleaveDepthView_R[0], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[1], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[2], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[3], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[4], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[5], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[6], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[7], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
    } );
    deinterleavePass.AddSubpass( { 0,1,2,3,4,5,6,7 }, // color attachment refs
                                 [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        GFrameResources.TextureBindings[0].pTexture = LinearDepth->Actual();
        GFrameResources.SamplerBindings[0].pSampler = NearestSampler;

        DeinterleaveFragmentShader.SetUniform2f( 0, 0.5f, 0.5f );

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &DeinterleavePipe );
    } );

    ARenderPass & deinterleavePass2 = FrameGraph.AddTask< ARenderPass >( "Deinterleave Depth Pass 2" );
    deinterleavePass2.SetRenderArea( AOQuarterWidth, AOQuarterHeight );
    deinterleavePass2.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    deinterleavePass2.SetColorAttachments(
    {
        { SSAODeinterleaveDepthView_R[8], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[9], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[10], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[11], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[12], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[13], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[14], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
        { SSAODeinterleaveDepthView_R[15], GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE ) },
    } );
    deinterleavePass2.AddSubpass( { 0,1,2,3,4,5,6,7 }, // color attachment refs
                                  [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        GFrameResources.TextureBindings[0].pTexture = LinearDepth->Actual();
        GFrameResources.SamplerBindings[0].pSampler = NearestSampler;

        DeinterleaveFragmentShader.SetUniform2f( 0, float( 8 % 4 ) + 0.5f, float( 8 / 4 ) + 0.5f );

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &DeinterleavePipe );
    } );

    AFrameGraphTextureStorage * DeinterleaveDepthArray_R = FrameGraph.AddExternalResource< GHI::TextureStorageCreateInfo, GHI::Texture >(
        "Deinterleave Depth Array",
        GHI::TextureStorageCreateInfo(),
        &SSAODeinterleaveDepthArray );

    return DeinterleaveDepthArray_R;
}

AFrameGraphTextureStorage * ASSAORenderer::AddCacheAwareAOPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * DeinterleaveDepthArray, AFrameGraphTextureStorage * NormalTexture )
{
    ARenderPass & cacheAwareAO = FrameGraph.AddTask< ARenderPass >( "Cache Aware AO Pass" );
    cacheAwareAO.SetRenderArea( AOQuarterWidth, AOQuarterHeight );
    cacheAwareAO.AddResource( DeinterleaveDepthArray, RESOURCE_ACCESS_READ );
    cacheAwareAO.AddResource( NormalTexture, RESOURCE_ACCESS_READ );
    cacheAwareAO.SetColorAttachments(
    {
        {
            "SSAO Texture Array",
            GHI::MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_R8, GHI::TextureResolution2DArray( AOQuarterWidth, AOQuarterHeight, HBAO_RANDOM_ELEMENTS ) ),
            GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    cacheAwareAO.AddSubpass( { 0 }, // color attachment refs
                             [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        GFrameResources.TextureBindings[0].pTexture = DeinterleaveDepthArray->Actual();
        GFrameResources.SamplerBindings[0].pSampler = RVCheckNearest ? NearestSampler : LinearDepthSampler;

        GFrameResources.TextureBindings[1].pTexture = NormalTexture->Actual();
        GFrameResources.SamplerBindings[1].pSampler = NearestSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( &CacheAwarePipe );
        } else {
            DrawSAQ( &CacheAwarePipe_ORTHO );
        }
    } );

    return cacheAwareAO.GetColorAttachments()[0].Resource;
}

AFrameGraphTextureStorage * ASSAORenderer::AddReinterleavePass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * SSAOTextureArray )
{
    ARenderPass & reinterleavePass = FrameGraph.AddTask< ARenderPass >( "Reinterleave Pass" );
    reinterleavePass.SetRenderArea( AOWidth, AOHeight );
    reinterleavePass.AddResource( SSAOTextureArray, RESOURCE_ACCESS_READ );
    reinterleavePass.SetColorAttachments(
    {
        {
            "SSAO Texture",
            GHI::MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_R8, GHI::TextureResolution2D( AOWidth, AOHeight ) ),
            GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    reinterleavePass.AddSubpass( { 0 }, // color attachment refs
                                 [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        GFrameResources.TextureBindings[0].pTexture = SSAOTextureArray->Actual();
        GFrameResources.SamplerBindings[0].pSampler = NearestSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &ReinterleavePipe );
    } );

    return reinterleavePass.GetColorAttachments()[0].Resource;
}

AFrameGraphTextureStorage * ASSAORenderer::AddAOBlurPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * SSAOTexture, AFrameGraphTextureStorage * LinearDepth )
{
    ARenderPass & aoBlurXPass = FrameGraph.AddTask< ARenderPass >( "AO Blur X Pass" );
    aoBlurXPass.SetRenderArea( AOWidth, AOHeight );
    aoBlurXPass.SetColorAttachments(
    {
        {
            "Temp SSAO Texture (Blur X)",
            GHI::MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_R8, GHI::TextureResolution2D( AOWidth, AOHeight ) ),
            GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    aoBlurXPass.AddResource( SSAOTexture, RESOURCE_ACCESS_READ );
    aoBlurXPass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    aoBlurXPass.AddSubpass( { 0 }, // color attachment refs
                            [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        // SSAO blur X
        GFrameResources.TextureBindings[0].pTexture = SSAOTexture->Actual();
        GFrameResources.SamplerBindings[0].pSampler = BlurSampler;

        GFrameResources.TextureBindings[1].pTexture = LinearDepth->Actual();
        GFrameResources.SamplerBindings[1].pSampler = NearestSampler;

        BlendFragmentShader.SetUniform2f( 0, 1.0f / RenderPass.GetRenderArea().Width, 0.0f );

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &BlurPipe );
    } );

    AFrameGraphTextureStorage * TempSSAOTextureBlurX = aoBlurXPass.GetColorAttachments()[0].Resource;

    ARenderPass & aoBlurYPass = FrameGraph.AddTask< ARenderPass >( "AO Blur Y Pass" );
    aoBlurYPass.SetRenderArea( AOWidth, AOHeight );
    aoBlurYPass.SetColorAttachments(
    {
        {
            "Blured SSAO Texture",
            GHI::MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_R8, GHI::TextureResolution2D( AOWidth, AOHeight ) ),
            GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    aoBlurYPass.AddResource( TempSSAOTextureBlurX, RESOURCE_ACCESS_READ );
    aoBlurYPass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    aoBlurYPass.AddSubpass( { 0 }, // color attachment refs
                            [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        // SSAO blur Y
        GFrameResources.TextureBindings[0].pTexture = TempSSAOTextureBlurX->Actual();
        GFrameResources.SamplerBindings[0].pSampler = BlurSampler;

        GFrameResources.TextureBindings[1].pTexture = LinearDepth->Actual();
        GFrameResources.SamplerBindings[1].pSampler = NearestSampler;

        BlendFragmentShader.SetUniform2f( 0, 0.0f, 1.0f / RenderPass.GetRenderArea().Height );

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &BlurPipe );
    } );

    return aoBlurYPass.GetColorAttachments()[0].Resource;
}

AFrameGraphTextureStorage * ASSAORenderer::AddPasses( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * LinearDepth, AFrameGraphTextureStorage * NormalTexture )
{
    ResizeAO( GFrameData->AllocSurfaceWidth, GFrameData->AllocSurfaceHeight );

    AFrameGraphTextureStorage * DeinterleaveDepthArray = AddDeinterleaveDepthPass( FrameGraph, LinearDepth );

    AFrameGraphTextureStorage * SSAOTextureArray = AddCacheAwareAOPass( FrameGraph, DeinterleaveDepthArray, NormalTexture );

    AFrameGraphTextureStorage * SSAOTexture = AddReinterleavePass( FrameGraph, SSAOTextureArray );

    if ( RVSSAOBlur ) {
        SSAOTexture = AddAOBlurPass( FrameGraph, SSAOTexture, LinearDepth );
    }

    return SSAOTexture;
}

#if 0
void ASSAORenderer::RenderSSAOSimple() {
    RenderPassBegin renderPassBegin = {};

    int w = GRenderTarget.AOWidth;
    int h = GRenderTarget.AOHeight;

    renderPassBegin.pRenderPass = &Pass;
    renderPassBegin.pFramebuffer = &GRenderTarget.GetSSAOFramebuffer();
    renderPassBegin.RenderArea.X = 0;
    renderPassBegin.RenderArea.Y = 0;
    renderPassBegin.RenderArea.Width = w;
    renderPassBegin.RenderArea.Height = h;
    renderPassBegin.pColorClearValues = NULL;
    renderPassBegin.pDepthStencilClearValue = NULL;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = w;
    vp.Height = h;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    DrawCmd drawCmd;
    drawCmd.VertexCountPerInstance = 4;
    drawCmd.InstanceCount = 1;
    drawCmd.StartVertexLocation = 0;
    drawCmd.StartInstanceLocation = 0;

    // SSAO
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.GetLinearDepthTexture();
    GFrameResources.SamplerBindings[0].pSampler = RVCheckNearest ? NearestSampler : LinearDepthSampler;

    GFrameResources.TextureBindings[1].pTexture = &GRenderTarget.GetNormalTexture();
    GFrameResources.SamplerBindings[1].pSampler = NearestSampler;//NormalSampler;

    GFrameResources.TextureBindings[2].pTexture = &RandomMap;
    GFrameResources.SamplerBindings[2].pSampler = RandomMapSampler;

    if ( GRenderView->bPerspective ) {
        Cmd.BindPipeline( &Pipe, 0 );
    } else {
        Cmd.BindPipeline( &Pipe_ORTHO, 0 );
    }
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();
}
#endif

}
