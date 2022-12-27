/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "WireframeRenderer.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

static bool BindMaterialWireframePass(IImmediateContext* immediateCtx, RenderInstance const* instance)
{
    MaterialGPU* pMaterial = instance->Material;

    HK_ASSERT(pMaterial);

    int bSkinned = instance->SkeletonSize > 0;

    IPipeline* pPipeline = pMaterial->WireframePass[bSkinned];
    if (!pPipeline)
    {
        return false;
    }

    immediateCtx->BindPipeline(pPipeline);

    if (bSkinned)
    {
        immediateCtx->BindVertexBuffer(1, instance->WeightsBuffer, instance->WeightsBufferOffset);
    }
    else
    {
        immediateCtx->BindVertexBuffer(1, nullptr, 0);
    }

    BindVertexAndIndexBuffers(immediateCtx, instance);

    return true;
}

void AddWireframePass(FrameGraph& FrameGraph, FGTextureProxy* RenderTarget)
{
    if (!GRenderView->bWireframe)
    {
        return;
    }

    RenderPass& wireframePass = FrameGraph.AddTask<RenderPass>("Wireframe Pass");

    wireframePass.SetRenderArea(GRenderViewArea);

    wireframePass.SetColorAttachment(
        TextureAttachment(RenderTarget)
            .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

    //wireframePass.SetCondition( []() { return GRenderView->bWireframe; } );

    wireframePass.AddSubpass({0}, // color attachment refs
                             [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)

                             {
                                 IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                                 for (int i = 0; i < GRenderView->TerrainInstanceCount; i++)
                                 {
                                     TerrainRenderInstance const* instance = GFrameData->TerrainInstances[GRenderView->FirstTerrainInstance + i];

                                     TerrainInstanceConstantBuffer* drawCall = MapDrawCallConstants<TerrainInstanceConstantBuffer>();
                                     drawCall->LocalViewProjection            = instance->LocalViewProjection;
                                     StoreFloat3x3AsFloat3x4Transposed(instance->ModelNormalToViewSpace, drawCall->ModelNormalToViewSpace);
                                     drawCall->ViewPositionAndHeight = instance->ViewPositionAndHeight;
                                     drawCall->TerrainClipMin        = instance->ClipMin;
                                     drawCall->TerrainClipMax        = instance->ClipMax;

                                     rtbl->BindTexture(0, instance->Clipmaps);
                                     immediateCtx->BindPipeline(GTerrainWireframePipeline);
                                     immediateCtx->BindVertexBuffer(0, instance->VertexBuffer);
                                     immediateCtx->BindVertexBuffer(1, GStreamBuffer, instance->InstanceBufferStreamHandle);
                                     immediateCtx->BindIndexBuffer(instance->IndexBuffer, INDEX_TYPE_UINT16);
                                     immediateCtx->MultiDrawIndexedIndirect(instance->IndirectBufferDrawCount,
                                                                            GStreamBuffer,
                                                                            instance->IndirectBufferStreamHandle,
                                                                            sizeof(DrawIndexedIndirectCmd));
                                 }

                                 DrawIndexedCmd drawCmd;
                                 drawCmd.InstanceCount         = 1;
                                 drawCmd.StartInstanceLocation = 0;

                                 for (int i = 0; i < GRenderView->InstanceCount; i++)
                                 {
                                     RenderInstance const* instance = GFrameData->Instances[GRenderView->FirstInstance + i];

                                     if (!BindMaterialWireframePass(immediateCtx, instance))
                                     {
                                         continue;
                                     }

                                     BindTextures(instance->MaterialInstance, instance->Material->WireframePassTextureCount);
                                     BindSkeleton(instance->SkeletonOffset, instance->SkeletonSize);
                                     BindInstanceConstants(instance);

                                     drawCmd.IndexCountPerInstance = instance->IndexCount;
                                     drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                                     drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                                     immediateCtx->Draw(&drawCmd);
                                 }

                                 for (int i = 0; i < GRenderView->TranslucentInstanceCount; i++)
                                 {
                                     RenderInstance const* instance = GFrameData->TranslucentInstances[GRenderView->FirstTranslucentInstance + i];

                                     if (!BindMaterialWireframePass(immediateCtx, instance))
                                     {
                                         continue;
                                     }

                                     BindTextures(instance->MaterialInstance, instance->Material->WireframePassTextureCount);
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
