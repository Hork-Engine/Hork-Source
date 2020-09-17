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

#include "FxaaRenderer.h"
#include "RenderCommon.h"

using namespace RenderCore;

AFxaaRenderer::AFxaaRenderer()
{
    SSamplerInfo samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;

    SBufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_UNIFORM;

    SPipelineResourceLayout resourceLayout;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &samplerCI;
    resourceLayout.NumBuffers = 1;
    resourceLayout.Buffers = &bufferInfo;

    CreateFullscreenQuadPipeline( &FxaaPipeline, "postprocess/fxaa.vert", "postprocess/fxaa.frag", &resourceLayout );
}

void AFxaaRenderer::AddPass( AFrameGraph & FrameGraph, AFrameGraphTexture * SourceTexture, AFrameGraphTexture ** ppFxaaTexture )
{
    ARenderPass & renderPass = FrameGraph.AddTask< ARenderPass >( "FXAA Pass" );

    renderPass.SetDynamicRenderArea( &GRenderViewArea );

    renderPass.AddResource( SourceTexture, RESOURCE_ACCESS_READ );

    renderPass.SetColorAttachments(
    {
        {
            "FXAA texture",
            RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_R11F_G11F_B10F, GetFrameResoultion() ),
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    }
    );

    renderPass.AddSubpass( { 0 }, // color attachment refs
                           [=]( ARenderPass const & RenderPass, int SubpassIndex )

    {
        rtbl->BindTexture( 0, SourceTexture->Actual() );

        DrawSAQ( FxaaPipeline );
    } );

    *ppFxaaTexture = renderPass.GetColorAttachments()[0].Resource;
}
