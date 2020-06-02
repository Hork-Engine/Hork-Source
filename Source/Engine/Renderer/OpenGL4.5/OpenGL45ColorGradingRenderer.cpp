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

#include "OpenGL45ColorGradingRenderer.h"
#include "OpenGL45RenderBackend.h"

using namespace GHI;

namespace OpenGL45 {

AColorGradingRenderer::AColorGradingRenderer()
{
    CreateFullscreenQuadPipelineGS( PipelineLUT,
                                    "postprocess/colorgrading.vert",
                                    "postprocess/colorgrading.frag",
                                    "postprocess/colorgrading.geom",
                                    GHI::BLENDING_ALPHA );

    CreateFullscreenQuadPipelineGS( PipelineProcedural,
                                    "postprocess/colorgrading.vert",
                                    "postprocess/colorgrading_procedural.frag",
                                    "postprocess/colorgrading.geom",
                                    GHI::BLENDING_ALPHA );

    CreateSamplers();
}

void AColorGradingRenderer::CreateSamplers()
{
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();

    samplerCI.Filter = FILTER_NEAREST;// FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    ColorGradingSampler = GDevice.GetOrCreateSampler( samplerCI );
}

AFrameGraphTextureStorage * AColorGradingRenderer::AddPass( AFrameGraph & FrameGraph )
{
    if ( !GRenderView->CurrentColorGradingLUT ) {
        return nullptr;
    }

    auto dest = FrameGraph.AddExternalResource< GHI::TextureStorageCreateInfo, GHI::Texture >(
        "CurrentColorGradingLUT",
        MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_RGB16F, TextureResolution3D( 16,16,16 ) ),
        GPUTextureHandle( GRenderView->CurrentColorGradingLUT ) );

    if ( GRenderView->ColorGradingLUT )
    {
        auto source = FrameGraph.AddExternalResource< GHI::TextureStorageCreateInfo, GHI::Texture >(
            "ColorGradingLUT",
            MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_RGB16F, TextureResolution3D( 16,16,16 ) ),
            GPUTextureHandle( GRenderView->ColorGradingLUT ) );

        ARenderPass & renderPass = FrameGraph.AddTask< ARenderPass >( "Color Grading Pass" );

        renderPass.SetRenderArea( 16, 16 );
        renderPass.SetColorAttachments(
        {
            { dest, GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD ) }
        }
        );
        renderPass.AddResource( source, RESOURCE_ACCESS_READ );
        renderPass.AddSubpass( { 0 },
                               [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            GFrameResources.TextureBindings[0].pTexture = source->Actual();
            GFrameResources.SamplerBindings[0].pSampler = ColorGradingSampler;

            Cmd.BindShaderResources( &GFrameResources.Resources );

            DrawSAQ( &PipelineLUT );
        } );

        return dest;
    }
    else
    {
        ARenderPass & renderPass = FrameGraph.AddTask< ARenderPass >( "Color Grading Procedural Pass" );

        renderPass.SetRenderArea( 16, 16 );
        renderPass.SetColorAttachments(
        {
            { dest, GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD ) }
        }
        );
        renderPass.AddSubpass( { 0 },
                               [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            Cmd.BindShaderResources( &GFrameResources.Resources );

            DrawSAQ( &PipelineProcedural );
        } );

        return dest;
    }
}

}
