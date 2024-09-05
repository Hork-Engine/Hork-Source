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

#pragma once

#include "RenderDefs.h"

#include "VT/VirtualTextureAnalyzer.h"
#include "VT/VirtualTextureFeedback.h"
#include "VT/VirtualTexturePhysCache.h"

#include "CanvasRenderer.h"
#include "FrameRenderer.h"

#include <Hork/RenderCore/VertexMemoryGPU.h>

HK_NAMESPACE_BEGIN

class RenderBackend final : public Noncopyable
{
public:
    RenderBackend(RenderCore::IDevice* pDevice);
    ~RenderBackend();

    void GenerateIrradianceMap(RenderCore::ITexture* pCubemap, Ref<RenderCore::ITexture>* ppTexture);
    void GenerateReflectionMap(RenderCore::ITexture* pCubemap, Ref<RenderCore::ITexture>* ppTexture);
    void GenerateSkybox(TEXTURE_FORMAT Format, uint32_t Resolution, Float3 const& LightDir, Ref<RenderCore::ITexture>* ppTexture);

    bool         GenerateAndSaveEnvironmentMap(ImageStorage const& Skybox, StringView EnvmapFile);
    bool         GenerateAndSaveEnvironmentMap(SkyboxImportSettings const& ImportSettings, StringView EnvmapFile);
    ImageStorage GenerateAtmosphereSkybox(SKYBOX_IMPORT_TEXTURE_FORMAT Format, uint32_t Resolution, Float3 const& LightDir);

    void RenderFrame(StreamedMemoryGPU* StreamedMemory, RenderCore::ITexture* pBackBuffer, RenderFrameData* pFrameData);

    int ClusterPackedIndicesAlignment() const;

    int MaxOmnidirectionalShadowMapsPerView() const;

private:
    void RenderView(int ViewportIndex, RenderViewData* pRenderView);
    void SetViewConstants(int ViewportIndex);
    void UploadShaderResources(int ViewportIndex);

    Ref<RenderCore::FrameGraph>       m_FrameGraph;

    Ref<CanvasRenderer> m_CanvasRenderer;
    Ref<FrameRenderer> m_FrameRenderer;

    Ref<RenderCore::IQueryPool> m_TimeQuery;

    Ref<RenderCore::IQueryPool> m_TimeStamp1;
    Ref<RenderCore::IQueryPool> m_TimeStamp2;

    Ref<VirtualTextureFeedbackAnalyzer> m_FeedbackAnalyzerVT;
    Ref<VirtualTextureCache> m_PhysCacheVT;

    Ref<RenderCore::IPipeline> m_TerrainDepthPipeline;
    Ref<RenderCore::IPipeline> m_TerrainLightPipeline;
    Ref<RenderCore::IPipeline> m_TerrainWireframePipeline;

    // Just for test
    Ref<VirtualTexture> m_TestVT;
};

HK_NAMESPACE_END
