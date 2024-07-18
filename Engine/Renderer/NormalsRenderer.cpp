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

#include "NormalsRenderer.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

static bool BindMaterialNormalPass(IImmediateContext* immediateCtx, RenderInstance const* instance)
{
    MaterialGPU* pMaterial = instance->Material;

    HK_ASSERT(pMaterial);

    int bSkinned = instance->SkeletonSize > 0;

    IPipeline* pPipeline = pMaterial->Passes[bSkinned ? MaterialPass::NormalsPass_Skin : MaterialPass::NormalsPass];
    if (!pPipeline)
    {
        return false;
    }

    immediateCtx->BindPipeline(pPipeline);

    if (bSkinned)
    {
        IBuffer* pSecondVertexBuffer = instance->WeightsBuffer;
        immediateCtx->BindVertexBuffer(1, pSecondVertexBuffer, instance->WeightsBufferOffset);
    }
    else
    {
        immediateCtx->BindVertexBuffer(1, nullptr, 0);
    }

    BindVertexAndIndexBuffers(immediateCtx, instance);

    return true;
}

void AddNormalsPass(FrameGraph& FrameGraph, FGTextureProxy* RenderTarget)
{
    RenderPass& normalPass = FrameGraph.AddTask<RenderPass>("Normal Pass");

    normalPass.SetRenderArea(GRenderViewArea);

    normalPass.SetColorAttachment(
        TextureAttachment(RenderTarget)
            .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

    normalPass.AddSubpass({0}, // color attachment refs
                          [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)

                          {
                              IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                              DrawIndexedCmd drawCmd;
                              drawCmd.InstanceCount         = 1;
                              drawCmd.StartInstanceLocation = 0;

                              for (int i = 0; i < GRenderView->InstanceCount; i++)
                              {
                                  RenderInstance const* instance = GFrameData->Instances[GRenderView->FirstInstance + i];

                                  if (!BindMaterialNormalPass(immediateCtx, instance))
                                  {
                                      continue;
                                  }

                                  BindTextures(instance->MaterialInstance, instance->Material->NormalsPassTextureCount);
                                  BindSkeleton(instance->SkeletonOffset, instance->SkeletonSize);
                                  BindInstanceConstants(instance);

                                  drawCmd.IndexCountPerInstance = instance->IndexCount;
                                  drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                                  drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                                  immediateCtx->Draw(&drawCmd);
                              }
                          });
}

HK_NAMESPACE_END
