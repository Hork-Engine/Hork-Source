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

#include "OpenGL45FxaaRenderer.h"
#include "OpenGL45Common.h"

using namespace GHI;

namespace OpenGL45 {

AFxaaRenderer::AFxaaRenderer()
{
    CreateFullscreenQuadPipeline( FxaaPipeline, "postprocess/fxaa.vert", "postprocess/fxaa.frag" );
    CreateSampler();
}

void AFxaaRenderer::CreateSampler()
{
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    FxaaSampler = GDevice.GetOrCreateSampler( samplerCI );
}

AFrameGraphTextureStorage * AFxaaRenderer::AddPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * SourceTexture )
{
    ARenderPass & renderPass = FrameGraph.AddTask< ARenderPass >( "FXAA Pass" );

    renderPass.SetDynamicRenderArea( &GRenderViewArea );

    renderPass.AddResource( SourceTexture, RESOURCE_ACCESS_READ );

    renderPass.SetColorAttachments(
    {
        {
            "FXAA texture",
            GHI::MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_R11F_G11F_B10F, GetFrameResoultion() ),
            GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    }
    );

    renderPass.AddSubpass( { 0 }, // color attachment refs
                           [=]( ARenderPass const & RenderPass, int SubpassIndex )

    {
        GFrameResources.TextureBindings[0].pTexture = SourceTexture->Actual();
        GFrameResources.SamplerBindings[0].pSampler = FxaaSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &FxaaPipeline );
    } );

    return renderPass.GetColorAttachments()[0].Resource;
}

}
