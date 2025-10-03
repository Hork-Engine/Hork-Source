/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "DepthRenderer.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RHI;

static bool BindMaterialDepthPass(IImmediateContext* immediateCtx, RenderInstance const* instance)
{
    MaterialGPU* pMaterial = instance->Material;

    HK_ASSERT(pMaterial);

    int bSkinned = instance->SkeletonSize > 0;

    IPipeline* pPipeline;
    if (GRenderView->bAllowMotionBlur && instance->bPerObjectMotionBlur)
    {
        pPipeline = pMaterial->Passes[bSkinned ? MaterialPass::DepthVelocityPass_Skin : MaterialPass::DepthVelocityPass];
    }
    else
    {
        pPipeline = pMaterial->Passes[bSkinned ? MaterialPass::DepthPass_Skin : MaterialPass::DepthPass];
    }

    if (!pPipeline)
    {
        return false;
    }

    // Bind pipeline
    immediateCtx->BindPipeline(pPipeline);

    // Bind second vertex buffer
    if (bSkinned)
    {
        immediateCtx->BindVertexBuffer(1, instance->WeightsBuffer, instance->WeightsBufferOffset);
    }
    else
    {
        immediateCtx->BindVertexBuffer(1, nullptr, 0);
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers(immediateCtx, instance);

    return true;
}

void AddDepthPass( FrameGraph & FrameGraph, FGTextureProxy ** ppDepthTexture, FGTextureProxy ** ppVelocity )
{
    RenderPass & depthPass = FrameGraph.AddTask< RenderPass >( "Depth Pre-Pass" );

    depthPass.SetRenderArea(GRenderViewArea);

    depthPass.SetDepthStencilAttachment(
        TextureAttachment("Depth texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_D24S8)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR));

    if (GRenderView->bAllowMotionBlur)
    {
        Float2 velocity( 1, 1 );

        depthPass.SetColorAttachment(
            TextureAttachment("Velocity texture",
                               TextureDesc()
                                   .SetFormat(TEXTURE_FORMAT_RG8_UNORM)
                                   .SetResolution(GetFrameResoultion()))
                .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)
                .SetClearValue(RHI::MakeClearColorValue(velocity.X, velocity.Y, 0.0f, 0.0f)));

        *ppVelocity = depthPass.GetColorAttachments()[0].pResource;

        depthPass.AddSubpass( { 0 }, // color attachments
                             [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
        {
            IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;
            for ( int i = 0 ; i < GRenderView->TerrainInstanceCount ; i++ ) {
                TerrainRenderInstance const * instance = GFrameData->TerrainInstances[GRenderView->FirstTerrainInstance + i];

                TerrainInstanceConstantBuffer * drawCall = MapDrawCallConstants< TerrainInstanceConstantBuffer >();
                drawCall->LocalViewProjection = instance->LocalViewProjection;
                StoreFloat3x3AsFloat3x4Transposed( instance->ModelNormalToViewSpace, drawCall->ModelNormalToViewSpace );
                drawCall->ViewPositionAndHeight = instance->ViewPositionAndHeight;
                drawCall->TerrainClipMin = instance->ClipMin;
                drawCall->TerrainClipMax = instance->ClipMax;

                rtbl->BindTexture( 0, instance->Clipmaps );
                immediateCtx->BindPipeline(GTerrainDepthPipeline);
                immediateCtx->BindVertexBuffer(0, instance->VertexBuffer);
                immediateCtx->BindVertexBuffer(1, GStreamBuffer, instance->InstanceBufferStreamHandle);
                immediateCtx->BindIndexBuffer(instance->IndexBuffer, INDEX_TYPE_UINT16);
                immediateCtx->MultiDrawIndexedIndirect(instance->IndirectBufferDrawCount,
                                                GStreamBuffer,
                                                instance->IndirectBufferStreamHandle,
                                                sizeof( DrawIndexedIndirectCmd ) );
            }

            DrawIndexedCmd drawCmd;
            drawCmd.InstanceCount = 1;
            drawCmd.StartInstanceLocation = 0;

            for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
                RenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

                if (!BindMaterialDepthPass(immediateCtx, instance))
                {
                    continue;
                }

                BindTextures( instance->MaterialInstance, instance->Material->DepthPassTextureCount );
                BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
                BindSkeletonMotionBlur( instance->SkeletonOffsetMB, instance->SkeletonSize );
                BindInstanceConstants( instance );

                drawCmd.IndexCountPerInstance = instance->IndexCount;
                drawCmd.StartIndexLocation = instance->StartIndexLocation;
                drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

                immediateCtx->Draw(&drawCmd);
            }

        } );
    }
    else {
        *ppVelocity = nullptr;

        depthPass.AddSubpass( {}, // no color attachments
                             [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
        {
            IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

            for ( int i = 0 ; i < GRenderView->TerrainInstanceCount ; i++ ) {
                TerrainRenderInstance const * instance = GFrameData->TerrainInstances[GRenderView->FirstTerrainInstance + i];

                TerrainInstanceConstantBuffer * drawCall = MapDrawCallConstants< TerrainInstanceConstantBuffer >();
                drawCall->LocalViewProjection = instance->LocalViewProjection;
                StoreFloat3x3AsFloat3x4Transposed( instance->ModelNormalToViewSpace, drawCall->ModelNormalToViewSpace );
                drawCall->ViewPositionAndHeight = instance->ViewPositionAndHeight;
                drawCall->TerrainClipMin = instance->ClipMin;
                drawCall->TerrainClipMax = instance->ClipMax;

                rtbl->BindTexture( 0, instance->Clipmaps );
                immediateCtx->BindPipeline(GTerrainDepthPipeline);
                immediateCtx->BindVertexBuffer(0, instance->VertexBuffer);
                immediateCtx->BindVertexBuffer(1, GStreamBuffer, instance->InstanceBufferStreamHandle);
                immediateCtx->BindIndexBuffer(instance->IndexBuffer, INDEX_TYPE_UINT16);
                immediateCtx->MultiDrawIndexedIndirect(instance->IndirectBufferDrawCount,
                                                GStreamBuffer,
                                                instance->IndirectBufferStreamHandle,
                                                sizeof( DrawIndexedIndirectCmd ) );
            }

            DrawIndexedCmd drawCmd;
            drawCmd.InstanceCount = 1;
            drawCmd.StartInstanceLocation = 0;

            for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
                RenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

                if (!BindMaterialDepthPass(immediateCtx, instance))
                {
                    continue;
                }

                BindTextures( instance->MaterialInstance, instance->Material->DepthPassTextureCount );
                BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
                BindInstanceConstants( instance );

                drawCmd.IndexCountPerInstance = instance->IndexCount;
                drawCmd.StartIndexLocation = instance->StartIndexLocation;
                drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

                immediateCtx->Draw(&drawCmd);
            }
        } );
    }

    *ppDepthTexture = depthPass.GetDepthStencilAttachment().pResource;
}

HK_NAMESPACE_END
