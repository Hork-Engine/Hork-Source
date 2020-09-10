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

#include "PostprocessRenderer.h"
#include "RenderBackend.h"
#include "ExposureRenderer.h"

using namespace RenderCore;

APostprocessRenderer::APostprocessRenderer()
{
    SSamplerInfo samplers[7];

    // Color texture sampler
    samplers[0].Filter = FILTER_NEAREST;
    samplers[0].AddressU = SAMPLER_ADDRESS_CLAMP;
    samplers[0].AddressV = SAMPLER_ADDRESS_CLAMP;
    samplers[0].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Color grading LUT sampler
    samplers[1].Filter = FILTER_LINEAR;
    samplers[1].AddressU = SAMPLER_ADDRESS_CLAMP;
    samplers[1].AddressV = SAMPLER_ADDRESS_CLAMP;
    samplers[1].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Bloom texture sampler
    samplers[2].Filter = FILTER_LINEAR;
    samplers[2].AddressU = SAMPLER_ADDRESS_CLAMP;
    samplers[2].AddressV = SAMPLER_ADDRESS_CLAMP;
    samplers[2].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Bloom texture sampler
    samplers[3].Filter = FILTER_LINEAR;
    samplers[3].AddressU = SAMPLER_ADDRESS_CLAMP;
    samplers[3].AddressV = SAMPLER_ADDRESS_CLAMP;
    samplers[3].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Bloom texture sampler
    samplers[4].Filter = FILTER_LINEAR;
    samplers[4].AddressU = SAMPLER_ADDRESS_CLAMP;
    samplers[4].AddressV = SAMPLER_ADDRESS_CLAMP;
    samplers[4].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Bloom texture sampler
    samplers[5].Filter = FILTER_LINEAR;
    samplers[5].AddressU = SAMPLER_ADDRESS_CLAMP;
    samplers[5].AddressV = SAMPLER_ADDRESS_CLAMP;
    samplers[5].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Exposure sampler: whatever (used texelFetch)
    samplers[6].Filter = FILTER_NEAREST;
    samplers[6].AddressU = SAMPLER_ADDRESS_CLAMP;
    samplers[6].AddressV = SAMPLER_ADDRESS_CLAMP;
    samplers[6].AddressW = SAMPLER_ADDRESS_CLAMP;

    SBufferInfo bufferInfo;
    bufferInfo.BufferType = UNIFORM_BUFFER;

    SPipelineResourceLayout resourceLayout;

    resourceLayout.NumSamplers = AN_ARRAY_SIZE( samplers );
    resourceLayout.Samplers = samplers;
    resourceLayout.NumBuffers = 1;
    resourceLayout.Buffers = &bufferInfo;

    CreateFullscreenQuadPipeline( &PostprocessPipeline, "postprocess/final.vert", "postprocess/final.frag", &resourceLayout );
}

void APostprocessRenderer::AddPass( AFrameGraph & FrameGraph,
                                    AFrameGraphTexture * ColorTexture,
                                    AFrameGraphTexture * Exposure,
                                    AFrameGraphTexture * ColorGrading,
                                    ABloomRenderer::STextures & BloomTex,
                                    AFrameGraphTexture ** ppPostprocessTexture )
{
    ARenderPass & renderPass = FrameGraph.AddTask< ARenderPass >( "Postprocess Pass" );

    renderPass.SetDynamicRenderArea( &GRenderViewArea );

    renderPass.AddResource( ColorTexture, RESOURCE_ACCESS_READ );

    renderPass.AddResource( Exposure, RESOURCE_ACCESS_READ );

    if ( ColorGrading ) {
        renderPass.AddResource( ColorGrading, RESOURCE_ACCESS_READ );
    }

    renderPass.AddResource( BloomTex.BloomTexture0, RESOURCE_ACCESS_READ );
    renderPass.AddResource( BloomTex.BloomTexture1, RESOURCE_ACCESS_READ );
    renderPass.AddResource( BloomTex.BloomTexture2, RESOURCE_ACCESS_READ );
    renderPass.AddResource( BloomTex.BloomTexture3, RESOURCE_ACCESS_READ );

    renderPass.SetColorAttachments(
    {
        {
            "Postprocess texture",
            MakeTexture( RenderCore::TEXTURE_FORMAT_RGBA16F, GetFrameResoultion() ),
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    }
    );

    renderPass.AddSubpass( { 0 }, // color attachment refs
                           [=]( ARenderPass const & RenderPass, int SubpassIndex )

    {
        rtbl->BindTexture( 0, ColorTexture->Actual() );
        if ( ColorGrading ) {
            rtbl->BindTexture( 1, ColorGrading->Actual() );
        }
        rtbl->BindTexture( 2, BloomTex.BloomTexture0->Actual() );
        rtbl->BindTexture( 3, BloomTex.BloomTexture1->Actual() );
        rtbl->BindTexture( 4, BloomTex.BloomTexture2->Actual() );
        rtbl->BindTexture( 5, BloomTex.BloomTexture3->Actual() );
        rtbl->BindTexture( 6, Exposure->Actual() );

        DrawSAQ( PostprocessPipeline );

    } );

    *ppPostprocessTexture = renderPass.GetColorAttachments()[0].Resource;
}
