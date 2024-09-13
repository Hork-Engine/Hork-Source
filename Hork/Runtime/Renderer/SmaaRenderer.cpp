/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "SmaaRenderer.h"
#include "RenderLocal.h"
#include "SMAA/AreaTex.h"
#include "SMAA/SearchTex.h"

HK_NAMESPACE_BEGIN

using namespace RHI;

SmaaRenderer::SmaaRenderer()
{
    SamplerDesc samplerCI[3];
    for (int i = 0; i < HK_ARRAY_SIZE(samplerCI); i++)
    {
        samplerCI[i].Filter   = FILTER_LINEAR;
        samplerCI[i].AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI[i].AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI[i].AddressW = SAMPLER_ADDRESS_CLAMP;
    }

    BufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_CONSTANT;

    PipelineResourceLayout resourceLayout;

    resourceLayout.NumBuffers = 1;
    resourceLayout.Buffers = &bufferInfo;
    resourceLayout.Samplers = samplerCI;

    resourceLayout.NumSamplers = 1;
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &EdgeDetectionPipeline, "postprocess/smaa/edge.vert", "postprocess/smaa/edge.frag", &resourceLayout);

    resourceLayout.NumSamplers = 3;
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &BlendingWeightCalculationPipeline, "postprocess/smaa/weights.vert", "postprocess/smaa/weights.frag", &resourceLayout);

    resourceLayout.NumSamplers = 2;
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &NeighborhoodBlendingPipeline, "postprocess/smaa/blend.vert", "postprocess/smaa/blend.frag", &resourceLayout);

    CreateTextures();
}

void SmaaRenderer::CreateTextures()
{
    /*
    Note from SMAA authors:
    You can also compress 'areaTex' and 'searchTex' using BC5 and BC4
    respectively, if you have that option in your content processor pipeline.
    When compressing then, you get a non-perceptible quality decrease, and a
    marginal performance increase.
    */

    GDevice->CreateTexture(TextureDesc()
                               .SetResolution(TextureResolution2D(AREATEX_WIDTH, AREATEX_HEIGHT))
                               .SetFormat(TEXTURE_FORMAT_RG8_UNORM)
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &AreaTex);
    AreaTex->Write(0, sizeof(areaTexBytes), 1, areaTexBytes);

    GDevice->CreateTexture(TextureDesc()
                               .SetResolution(TextureResolution2D(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT))
                               .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &SearchTex);
    SearchTex->Write(0, sizeof(searchTexBytes), 1, searchTexBytes);
}

void SmaaRenderer::AddPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* SourceTexture, RHI::FGTextureProxy** ppResultTexture)
{
    FGTextureProxy* EdgeTexture;
    FGTextureProxy* BlendTexture;

    EdgeDetectionPass(FrameGraph, SourceTexture, &EdgeTexture);
    BlendingWeightCalculationPass(FrameGraph, EdgeTexture, &BlendTexture);
    NeighborhoodBlendingPass(FrameGraph, SourceTexture, BlendTexture, ppResultTexture);

    //EdgeDetectionPass(FrameGraph, SourceTexture, &EdgeTexture);
    //BlendingWeightCalculationPass(FrameGraph, EdgeTexture, ppResultTexture);
    //HK_UNUSED(EdgeTexture);
    //HK_UNUSED(BlendTexture);
}

void SmaaRenderer::EdgeDetectionPass(FrameGraph& FrameGraph, FGTextureProxy* SourceTexture, FGTextureProxy** ppEdgeTexture)
{
    RenderPass& renderPass = FrameGraph.AddTask<RenderPass>("SMAA Edge Detection Pass");

    renderPass.SetRenderArea(GRenderViewArea);
    renderPass.AddResource(SourceTexture, FG_RESOURCE_ACCESS_READ);

    renderPass.SetColorAttachment(
        TextureAttachment("SMAA edge texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_RGBA8_UNORM)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR));

    renderPass.AddSubpass({0}, // color attachment refs
                          [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                          {
                              rtbl->BindTexture(0, SourceTexture->Actual());

                              RenderUtils::DrawSAQ(RenderPassContext.pImmediateContext, EdgeDetectionPipeline);
                          });

    *ppEdgeTexture = renderPass.GetColorAttachments()[0].pResource;
}

void SmaaRenderer::BlendingWeightCalculationPass(FrameGraph& FrameGraph, FGTextureProxy* EdgeTexture, FGTextureProxy** ppBlendTexture)
{
    RenderPass& renderPass = FrameGraph.AddTask<RenderPass>("SMAA Blending Weight Calculation Pass");

    renderPass.SetRenderArea(GRenderViewArea);
    renderPass.AddResource(EdgeTexture, FG_RESOURCE_ACCESS_READ);

    renderPass.SetColorAttachment(
        TextureAttachment("SMAA blend texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_RGBA8_UNORM)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR));

    renderPass.AddSubpass({0}, // color attachment refs
                          [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                          {
                              rtbl->BindTexture(0, EdgeTexture->Actual());
                              rtbl->BindTexture(1, AreaTex);
                              rtbl->BindTexture(2, SearchTex);

                              RenderUtils::DrawSAQ(RenderPassContext.pImmediateContext, BlendingWeightCalculationPipeline);
                          });

    *ppBlendTexture = renderPass.GetColorAttachments()[0].pResource;
}

void SmaaRenderer::NeighborhoodBlendingPass(FrameGraph& FrameGraph, FGTextureProxy* SourceTexture, FGTextureProxy* BlendTexture, FGTextureProxy** ppResultTexture)
{
    RenderPass& renderPass = FrameGraph.AddTask<RenderPass>("SMAA Neighborhood Blending Pass");

    renderPass.SetRenderArea(GRenderViewArea);
    renderPass.AddResource(SourceTexture, FG_RESOURCE_ACCESS_READ);
    renderPass.AddResource(BlendTexture, FG_RESOURCE_ACCESS_READ);

    renderPass.SetColorAttachment(
        TextureAttachment("SMAA result texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R11G11B10_FLOAT/*TEXTURE_FORMAT_RGBA8_UNORM*/)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));

    renderPass.AddSubpass({0}, // color attachment refs
                          [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                          {
                              rtbl->BindTexture(0, SourceTexture->Actual());
                              rtbl->BindTexture(1, BlendTexture->Actual());

                              RenderUtils::DrawSAQ(RenderPassContext.pImmediateContext, NeighborhoodBlendingPipeline);
                          });

    *ppResultTexture = renderPass.GetColorAttachments()[0].pResource;
}

HK_NAMESPACE_END
