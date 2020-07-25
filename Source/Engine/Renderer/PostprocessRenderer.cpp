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
    CreateFullscreenQuadPipeline( &PostprocessPipeline, "postprocess/final.vert", "postprocess/final.frag" );
    CreateSampler();
}

void APostprocessRenderer::CreateSampler()
{
    SSamplerCreateInfo samplerCI;

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    GDevice->GetOrCreateSampler( samplerCI, &PostprocessSampler );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    GDevice->GetOrCreateSampler( samplerCI, &BloomSampler );

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    GDevice->GetOrCreateSampler( samplerCI, &LuminanceSampler );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    GDevice->GetOrCreateSampler( samplerCI, &ColorGradingSampler );
}

AFrameGraphTexture * APostprocessRenderer::AddPass( AFrameGraph & FrameGraph,
                                                          AFrameGraphTexture * ColorTexture,
                                                          AFrameGraphTexture * Exposure,
                                                          AFrameGraphTexture * ColorGrading,
                                                          ABloomRenderer::STextures & BloomTex )
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
        GFrameResources.TextureBindings[0].pTexture = ColorTexture->Actual();
        GFrameResources.SamplerBindings[0].pSampler = PostprocessSampler;

        if ( ColorGrading ) {
            GFrameResources.TextureBindings[1].pTexture = ColorGrading->Actual();
            GFrameResources.SamplerBindings[1].pSampler = ColorGradingSampler;
        }

        GFrameResources.TextureBindings[2].pTexture = BloomTex.BloomTexture0->Actual();
        GFrameResources.SamplerBindings[2].pSampler = BloomSampler;

        GFrameResources.TextureBindings[3].pTexture = BloomTex.BloomTexture1->Actual();
        GFrameResources.SamplerBindings[3].pSampler = BloomSampler;

        GFrameResources.TextureBindings[4].pTexture = BloomTex.BloomTexture2->Actual();
        GFrameResources.SamplerBindings[4].pSampler = BloomSampler;

        GFrameResources.TextureBindings[5].pTexture = BloomTex.BloomTexture3->Actual();
        GFrameResources.SamplerBindings[5].pSampler = BloomSampler;

        GFrameResources.TextureBindings[6].pTexture = Exposure->Actual();
        GFrameResources.SamplerBindings[6].pSampler = LuminanceSampler; // whatever (used texelFetch)

        rcmd->BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( PostprocessPipeline );

    } );

    return renderPass.GetColorAttachments()[0].Resource;
}
