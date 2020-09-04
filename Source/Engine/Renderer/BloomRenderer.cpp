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

#include "BloomRenderer.h"
#include "RenderBackend.h"

ARuntimeVariable RVBloomTextureFormat( _CTS( "BloomTextureFormat" ), _CTS( "0" ) );
ARuntimeVariable RVBloomStart( _CTS( "BloomStart" ), _CTS( "1" ) );
ARuntimeVariable RVBloomThreshold( _CTS( "BloomThreshold" ), _CTS( "1" ) );

using namespace RenderCore;

ABloomRenderer::ABloomRenderer()
{
    SSamplerInfo samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;

    CreateFullscreenQuadPipeline( &BrightPipeline, "postprocess/brightpass.vert", "postprocess/brightpass.frag", &samplerCI, 1 );
    CreateFullscreenQuadPipeline( &CopyPipeline, "postprocess/copy.vert", "postprocess/copy.frag", &samplerCI, 1 );
    CreateFullscreenQuadPipeline( &BlurPipeline, "postprocess/gauss.vert", "postprocess/gauss.frag", &samplerCI, 1 );
}

void ABloomRenderer::AddPasses( AFrameGraph & FrameGraph, AFrameGraphTexture * SourceTexture, ABloomRenderer::STextures * pResult )
{
    RenderCore::TEXTURE_FORMAT pf;

    switch ( RVBloomTextureFormat.GetInteger() ) {
    case 0:
        pf = RenderCore::TEXTURE_FORMAT_R11F_G11F_B10F;
        break;
    case 1:
        pf = RenderCore::TEXTURE_FORMAT_RGB16F;
        break;
    default:
        // TODO: We can use RGB8 format, but it need some way of bloom compression to not lose in quality.
        pf = RenderCore::TEXTURE_FORMAT_RGB8;
        break;
    }

    STextureResolution2D bloomResolution = GetFrameResoultion();
    bloomResolution.Width >>= 1;
    bloomResolution.Height >>= 1;

    AFrameGraphTexture * BrightTexture, *BrightBlurXTexture, *BrightBlurTexture;
    AFrameGraphTexture * BrightTexture2, *BrightBlurXTexture2, *BrightBlurTexture2;
    AFrameGraphTexture * BrightTexture4, *BrightBlurXTexture4, *BrightBlurTexture4;
    AFrameGraphTexture * BrightTexture6, *BrightBlurXTexture6, *BrightBlurTexture6;

    // Make bright texture
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: Bright Pass" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( SourceTexture, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright texture",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SBrightPassDrawCall
            {
                Float4 BloomStart;
                Float4 BloomThreshold;
            };

            SBrightPassDrawCall * drawCall = SetDrawCallUniforms< SBrightPassDrawCall >();
            drawCall->BloomStart = Float4( RVBloomStart.GetFloat() );
            drawCall->BloomThreshold = Float4( RVBloomThreshold.GetFloat() );

            GFrameResources.TextureBindings[0]->pTexture = SourceTexture->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BrightPipeline );
        } );

        BrightTexture = pass.GetColorAttachments()[0].Resource;
    }


    //
    // Perform gaussian blur
    //

    // X pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: X pass. Result in BrightBlurXTexture" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightTexture, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright Blur X texture",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            RenderCore::SResourceTable const & resourceTable = GFrameResources.Resources;//.PostProccessResources;

            //resourceTable.Reset();

            //SResourceBufferBinding * viewUniformBuffer = resourceTable.AddBuffer( UNIFORM_BUFFER );
            //viewUniformBuffer->BindingOffset = GFrameResources.ViewUniformBufferBinding->BindingOffset;
            //viewUniformBuffer->BindingSize = GFrameResources.ViewUniformBufferBinding->BindingSize;
            //viewUniformBuffer->pBuffer = GFrameResources.ViewUniformBufferBinding->pBuffer;

            //SResourceBufferBinding * uniformBuffer = resourceTable.AddBuffer( UNIFORM_BUFFER );

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >( /*uniformBuffer*/ );
            drawCall->InvSize.X = 1.0f / RenderPass.GetRenderArea().Width;
            drawCall->InvSize.Y = 0;

            //SResourceSamplerBinding * sampler = resourceTable.AddSampler();
            //sampler->pTexture = BrightTexture->Actual();

            GFrameResources.TextureBindings[0]->pTexture = BrightTexture->Actual();

            rcmd->BindResourceTable( &resourceTable );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurXTexture = pass.GetColorAttachments()[0].Resource;
    }

    // Y pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: Y pass. Result in BrightBlurTexture" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightBlurXTexture, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright Blur texture",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >();
            drawCall->InvSize.X = 0;
            drawCall->InvSize.Y = 1.0f / RenderPass.GetRenderArea().Height;

            GFrameResources.TextureBindings[0]->pTexture = BrightBlurXTexture->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurTexture = pass.GetColorAttachments()[0].Resource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Downsample BrightBlurTexture to BrightTexture2" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightBlurTexture, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright texture 2",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            GFrameResources.TextureBindings[0]->pTexture = BrightBlurTexture->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( CopyPipeline );
        } );

        BrightTexture2 = pass.GetColorAttachments()[0].Resource;
    }

    // X pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: X pass. Result in BrightBlurXTexture2" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightTexture2, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright blur X texture 2",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >();
            drawCall->InvSize.X = 1.0f / RenderPass.GetRenderArea().Width;
            drawCall->InvSize.Y = 0;

            GFrameResources.TextureBindings[0]->pTexture = BrightTexture2->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurXTexture2 = pass.GetColorAttachments()[0].Resource;
    }

    // Y pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: Y pass. Result in BrightBlurTexture2" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightBlurXTexture2, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright blur texture 2",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >();
            drawCall->InvSize.X = 0;
            drawCall->InvSize.Y = 1.0f / RenderPass.GetRenderArea().Height;

            GFrameResources.TextureBindings[0]->pTexture = BrightBlurXTexture2->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurTexture2 = pass.GetColorAttachments()[0].Resource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Downsample BrightBlurTexture2 to BrightTexture4" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightBlurTexture2, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright texture 4",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            GFrameResources.TextureBindings[0]->pTexture = BrightBlurTexture2->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( CopyPipeline );
        } );

        BrightTexture4 = pass.GetColorAttachments()[0].Resource;
    }
    
    // X pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: X pass.Result in BrightBlurXTexture4" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightTexture4, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright blur X texture 4",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >();
            drawCall->InvSize.X = 1.0f / RenderPass.GetRenderArea().Width;
            drawCall->InvSize.Y = 0;

            GFrameResources.TextureBindings[0]->pTexture = BrightTexture4->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurXTexture4 = pass.GetColorAttachments()[0].Resource;
    }

    // Y pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: Y pass. Result in BrightBlurTexture4" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightBlurXTexture4, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright blur texture 4",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >();
            drawCall->InvSize.X = 0;
            drawCall->InvSize.Y = 1.0f / RenderPass.GetRenderArea().Height;

            GFrameResources.TextureBindings[0]->pTexture = BrightBlurXTexture4->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurTexture4 = pass.GetColorAttachments()[0].Resource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Downsample BrightBlurTexture4 to BrightTexture6" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightBlurTexture4, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright texture 6",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            GFrameResources.TextureBindings[0]->pTexture = BrightBlurTexture4->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( CopyPipeline );
        } );

        BrightTexture6 = pass.GetColorAttachments()[0].Resource;
    }

    // X pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: X pass. Result in BrightBlurXTexture6" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightTexture6, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright blur X texture 6",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >();
            drawCall->InvSize.X = 1.0f / RenderPass.GetRenderArea().Width;
            drawCall->InvSize.Y = 0;

            GFrameResources.TextureBindings[0]->pTexture = BrightTexture6->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurXTexture6 = pass.GetColorAttachments()[0].Resource;
    }

    // Y pass
    {
        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Bloom: Y pass. Result in BrightBlurTexture6" );
        pass.SetRenderArea( bloomResolution.Width, bloomResolution.Height );
        pass.AddResource( BrightBlurXTexture6, RESOURCE_ACCESS_READ );
        pass.SetColorAttachments(
        {
            {
                "Bright blur texture 6",
                MakeTexture( pf, bloomResolution ),
                RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        } );
        pass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            struct SDrawCall {
                Float2 InvSize;
            };

            SDrawCall * drawCall = SetDrawCallUniforms< SDrawCall >();
            drawCall->InvSize.X = 0;
            drawCall->InvSize.Y = 1.0f / RenderPass.GetRenderArea().Height;

            GFrameResources.TextureBindings[0]->pTexture = BrightBlurXTexture6->Actual();

            rcmd->BindResourceTable( &GFrameResources.Resources );

            DrawSAQ( BlurPipeline );
        } );

        BrightBlurTexture6 = pass.GetColorAttachments()[0].Resource;
    }

    pResult->BloomTexture0 = BrightBlurTexture;
    pResult->BloomTexture1 = BrightBlurTexture2;
    pResult->BloomTexture2 = BrightBlurTexture4;
    pResult->BloomTexture3 = BrightBlurTexture6;
}
