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

#include <Hork/RHI/Common/VertexMemoryGPU.h>

#include <Hork/VirtualTexture/VirtualTextureAnalyzer.h>
#include <Hork/VirtualTexture/VirtualTexturePhysCache.h>

#include "VirtualTextureFeedback.h"

HK_NAMESPACE_BEGIN

// NOTE: The rendering backend should be used as a singleton object. (This should be fixed later)
class RenderBackend final : public Noncopyable
{
public:
                                RenderBackend(RHI::IDevice* device);
                                ~RenderBackend();

    void                        RenderFrame(StreamedMemoryGPU* streamedMemory, RHI::ITexture* backBuffer, RenderFrameData const* frameData, CanvasDrawData const* canvasData);

    int                         ClusterPackedIndicesAlignment() const;
    int                         MaxOmnidirectionalShadowMapsPerView() const;

private:
    void                        RenderView(int viewportIndex, RenderViewData* renderView);
    void                        SetViewConstants(int viewportIndex);
    void                        UploadShaderResources(int viewportIndex);

    Ref<RHI::FrameGraph>        m_FrameGraph;

    Ref<class CanvasRenderer>   m_CanvasRenderer;
    Ref<class FrameRenderer>    m_FrameRenderer;

    Ref<RHI::IQueryPool>        m_TimeQuery;

    Ref<RHI::IQueryPool>        m_TimeStamp1;
    Ref<RHI::IQueryPool>        m_TimeStamp2;

    Ref<VirtualTextureFeedbackAnalyzer> m_FeedbackAnalyzerVT;
    Ref<VirtualTextureCache>    m_PhysCacheVT;

    Ref<RHI::IPipeline>         m_TerrainDepthPipeline;
    Ref<RHI::IPipeline>         m_TerrainLightPipeline;
    Ref<RHI::IPipeline>         m_TerrainWireframePipeline;

    // Just for test
    Ref<VirtualTexture>         m_TestVT;
};

HK_NAMESPACE_END
