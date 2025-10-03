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

#include "RenderLocal.h"

#include <Hork/Core/Platform.h>

HK_NAMESPACE_BEGIN

using namespace RHI;

/// Render device
IDevice* GDevice;

/// Render context
IImmediateContext* rcmd;

/// Render resource table
IResourceTable* rtbl;

/// Render frame data
RenderFrameData const* GFrameData;

/// Canvas draw data
CanvasDrawData const* GCanvasData;

/// Render frame view
RenderViewData const* GRenderView;

/// Render view area
Rect2D GRenderViewArea;

/// Stream buffer
IBuffer*            GStreamBuffer;
StreamedMemoryGPU* GStreamedMemory;

/// Circular buffer. Contains constant data for single draw call.
/// Don't use to store long-live data.
Ref<CircularBuffer> GCircularBuffer;

/// Simple white texture
Ref<ITexture> GWhiteTexture;

/// Cluster lookcup 3D texture
Ref<ITexture> GLookupBRDF;

/// Cluster lookcup 3D texture
Ref<ITexture> GClusterLookup;

/// Cluster item references
Ref<IBuffer> GClusterItemBuffer;

/// Cluster item references view
Ref<IBufferView> GClusterItemTBO;

Vector<RenderViewContext> GRenderViewContext;

VirtualTextureFeedbackAnalyzer* GFeedbackAnalyzerVT;
VirtualTextureCache*            GPhysCacheVT{};

IPipeline* GTerrainDepthPipeline;
IPipeline* GTerrainLightPipeline;
IPipeline* GTerrainWireframePipeline;

TextureResolution2D GetFrameResoultion()
{
    return TextureResolution2D(GRenderView->Width, GRenderView->Height);
    //return TextureResolution2D(GFrameData->RenderTargetMaxWidth, GFrameData->RenderTargetMaxHeight);
}

void DrawSAQ(IImmediateContext* immediateCtx, IPipeline* Pipeline, unsigned int InstanceCount)
{
    const DrawCmd drawCmd = {3, InstanceCount, 0, 0};
    immediateCtx->BindPipeline(Pipeline);
    immediateCtx->BindVertexBuffer(0, nullptr, 0);
    immediateCtx->BindIndexBuffer(nullptr, INDEX_TYPE_UINT16, 0);
    immediateCtx->Draw(&drawCmd);
}

void BindTextures(IResourceTable* Rtbl, MaterialFrameData* Instance, int MaxTextures)
{
    HK_ASSERT(Instance);

    int n = Math::Min(Instance->NumTextures, MaxTextures);

    for (int t = 0; t < n; t++)
    {
        Rtbl->BindTexture(t, Instance->Textures[t]);
    }
}

void BindTextures(MaterialFrameData* Instance, int MaxTextures)
{
    HK_ASSERT(Instance);

    int n = Math::Min(Instance->NumTextures, MaxTextures);

    for (int t = 0; t < n; t++)
    {
        rtbl->BindTexture(t, Instance->Textures[t]);
    }
}

void BindVertexAndIndexBuffers(IImmediateContext* immediateCtx, RenderInstance const* Instance)
{
    immediateCtx->BindVertexBuffer(0, Instance->VertexBuffer, Instance->VertexBufferOffset);
    immediateCtx->BindIndexBuffer(Instance->IndexBuffer, INDEX_TYPE_UINT32, Instance->IndexBufferOffset);
}

void BindVertexAndIndexBuffers(IImmediateContext* immediateCtx, ShadowRenderInstance const* Instance)
{
    immediateCtx->BindVertexBuffer(0, Instance->VertexBuffer, Instance->VertexBufferOffset);
    immediateCtx->BindIndexBuffer(Instance->IndexBuffer, INDEX_TYPE_UINT32, Instance->IndexBufferOffset);
}

void BindVertexAndIndexBuffers(IImmediateContext* immediateCtx, LightPortalRenderInstance const* Instance)
{
    immediateCtx->BindVertexBuffer(0, Instance->VertexBuffer, Instance->VertexBufferOffset);
    immediateCtx->BindIndexBuffer(Instance->IndexBuffer, INDEX_TYPE_UINT32, Instance->IndexBufferOffset);
}

void BindSkeleton(size_t _Offset, size_t _Size)
{
    rtbl->BindBuffer(2, GStreamBuffer, _Offset, _Size);
}

void BindSkeletonMotionBlur(size_t _Offset, size_t _Size)
{
    rtbl->BindBuffer(7, GStreamBuffer, _Offset, _Size);
}

void BindInstanceConstants(RenderInstance const* Instance)
{
    size_t offset = GCircularBuffer->Allocate(sizeof(InstanceConstantBuffer));

    InstanceConstantBuffer* pConstantBuf = reinterpret_cast<InstanceConstantBuffer*>(GCircularBuffer->GetMappedMemory() + offset);

    Core::Memcpy(&pConstantBuf->TransformMatrix, &Instance->Matrix, sizeof(pConstantBuf->TransformMatrix));
    Core::Memcpy(&pConstantBuf->TransformMatrixP, &Instance->MatrixP, sizeof(pConstantBuf->TransformMatrixP));
    StoreFloat3x3AsFloat3x4Transposed(Instance->ModelNormalToViewSpace, pConstantBuf->ModelNormalToViewSpace);
    Core::Memcpy(&pConstantBuf->LightmapOffset, &Instance->LightmapOffset, sizeof(pConstantBuf->LightmapOffset));
    Core::Memcpy(&pConstantBuf->uaddr_0, Instance->MaterialInstance->UniformVectors, sizeof(Float4) * Instance->MaterialInstance->NumUniformVectors);

    // TODO:
    pConstantBuf->VTOffset = Float2(0.0f); //Instance->VTOffset;
    pConstantBuf->VTScale  = Float2(1.0f); //Instance->VTScale;
    pConstantBuf->VTUnit   = 0;            //Instance->VTUnit;

    rtbl->BindBuffer(1, GCircularBuffer->GetBuffer(), offset, sizeof(InstanceConstantBuffer));
}

void BindInstanceConstantsFB(RenderInstance const* Instance)
{
    size_t offset = GCircularBuffer->Allocate(sizeof(FeedbackConstantBuffer));

    FeedbackConstantBuffer* pConstantBuf = reinterpret_cast<FeedbackConstantBuffer*>(GCircularBuffer->GetMappedMemory() + offset);

    Core::Memcpy(&pConstantBuf->TransformMatrix, &Instance->Matrix, sizeof(pConstantBuf->TransformMatrix));

    // TODO:
    pConstantBuf->VTOffset = Float2(0.0f); //Instance->VTOffset;
    pConstantBuf->VTScale  = Float2(1.0f); //Instance->VTScale;
    pConstantBuf->VTUnit   = 0;            //Instance->VTUnit;

    rtbl->BindBuffer(1, GCircularBuffer->GetBuffer(), offset, sizeof(FeedbackConstantBuffer));
}

void BindShadowInstanceConstants(ShadowRenderInstance const* Instance)
{
    size_t offset = GCircularBuffer->Allocate(sizeof(ShadowInstanceConstantBuffer));

    ShadowInstanceConstantBuffer* pConstantBuf = reinterpret_cast<ShadowInstanceConstantBuffer*>(GCircularBuffer->GetMappedMemory() + offset);

    StoreFloat3x4AsFloat4x4Transposed(Instance->WorldTransformMatrix, pConstantBuf->TransformMatrix);

    if (Instance->MaterialInstance)
    {
        Core::Memcpy(&pConstantBuf->uaddr_0, Instance->MaterialInstance->UniformVectors, sizeof(Float4) * Instance->MaterialInstance->NumUniformVectors);
    }

    pConstantBuf->CascadeMask = Instance->CascadeMask;

    rtbl->BindBuffer(1, GCircularBuffer->GetBuffer(), offset, sizeof(ShadowInstanceConstantBuffer));
}

void BindShadowInstanceConstants(ShadowRenderInstance const* Instance, int FaceIndex, Float3 const& LightPosition)
{
    size_t offset = GCircularBuffer->Allocate(sizeof(ShadowInstanceConstantBuffer));

    ShadowInstanceConstantBuffer* pConstantBuf = reinterpret_cast<ShadowInstanceConstantBuffer*>(GCircularBuffer->GetMappedMemory() + offset);

    //StoreFloat3x4AsFloat4x4Transposed(Instance->WorldTransformMatrix, pConstantBuf->TransformMatrix);

    Float4x4 lightViewMatrix;
    lightViewMatrix    = Float4x4::sGetCubeFaceMatrices()[FaceIndex];
    lightViewMatrix[3] = Float4(Float3x3(lightViewMatrix) * -LightPosition, 1.0f);

    pConstantBuf->TransformMatrix = (lightViewMatrix * Instance->WorldTransformMatrix);

    if (Instance->MaterialInstance)
    {
        Core::Memcpy(&pConstantBuf->uaddr_0, Instance->MaterialInstance->UniformVectors, sizeof(Float4) * Instance->MaterialInstance->NumUniformVectors);
    }

    pConstantBuf->CascadeMask = Instance->CascadeMask;

    rtbl->BindBuffer(1, GCircularBuffer->GetBuffer(), offset, sizeof(ShadowInstanceConstantBuffer));
}

void* MapDrawCallConstants(size_t SizeInBytes)
{
    size_t offset = GCircularBuffer->Allocate(SizeInBytes);

    rtbl->BindBuffer(1, GCircularBuffer->GetBuffer(), offset, SizeInBytes);

    return GCircularBuffer->GetMappedMemory() + offset;
}

void BindShadowMatrix()
{
    rtbl->BindBuffer(3, GStreamBuffer, GRenderView->ShadowMapMatricesStreamHandle, MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof(Float4x4));
}

void BindShadowCascades(size_t StreamHandle)
{
    rtbl->BindBuffer(3, GStreamBuffer, StreamHandle, MAX_SHADOW_CASCADES * sizeof(Float4x4));
}

void BindOmniShadowProjection(int FaceIndex)
{
    //rtbl->BindBuffer(3, GOmniProjectionMatrixBuffer, FaceIndex * sizeof(Float4x4), sizeof(Float4x4));
}

HK_NAMESPACE_END
