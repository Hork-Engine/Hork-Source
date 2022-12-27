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

#pragma once

#include "RenderDefs.h"

#include <RenderCore/FrameGraph.h>

HK_NAMESPACE_BEGIN

class CanvasRenderer : public RefCounted
{
public:
    CanvasRenderer();
    ~CanvasRenderer();

    void Render(RenderCore::FrameGraph& FrameGraph, RenderCore::ITexture* pBackBuffer);

private:
    void RenderVG(RenderCore::IImmediateContext* immediateCtx, CanvasDrawData const* pDrawData);
    void DrawFill(CanvasDrawCmd const* drawCommand, RenderCore::ITexture* pTexture);
    void DrawConvexFill(CanvasDrawCmd const* drawCommand, RenderCore::ITexture* pTexture);
    void DrawStroke(CanvasDrawCmd const* drawCommand, RenderCore::ITexture* pTexture, bool bStencilStroke = false);
    void DrawTriangles(CanvasDrawCmd const* drawCommand, RenderCore::ITexture* pTexture);
    void SetUniforms(int uniformOffset, RenderCore::ITexture* pTexture);
    void SetBuffers();
    void BuildFanIndices(int numIndices, bool bRebind = true);
    void BindPipeline(int Topology);

    enum
    {
        TOPOLOGY_TRIANGLE_LIST = 0,
        TOPOLOGY_TRIANGLE_STRIP,
        TOPOLOGY_MAX,

        RASTER_STATE_CULL   = 0,
        RASTER_STATE_MAX,

        BLEND_STATE_MAX = CANVAS_COMPOSITE_LAST + 1,

        DEPTH_STENCIL_DRAW_AA = 0,
        DEPTH_STENCIL_FILL,
        DEPTH_STENCIL_DEFAULT,
        DEPTH_STENCIL_STROKE_FILL,
        DEPTH_STENCIL_MAX,

        SAMPLER_STATE_CLAMP_CLAMP_LINEAR = 0,
        SAMPLER_STATE_WRAP_CLAMP_LINEAR,
        SAMPLER_STATE_CLAMP_WRAP_LINEAR,
        SAMPLER_STATE_WRAP_WRAP_LINEAR,
        SAMPLER_STATE_CLAMP_CLAMP_NEAREST,
        SAMPLER_STATE_WRAP_CLAMP_NEAREST,
        SAMPLER_STATE_CLAMP_WRAP_NEAREST,
        SAMPLER_STATE_WRAP_WRAP_NEAREST,

        SAMPLER_STATE_MAX
    };

    RenderCore::IImmediateContext*  m_ImmediateCtx;
    CanvasDrawData const*           m_pDrawData;
    bool                            m_bEdgeAntialias;
    TRef<RenderCore::IShaderModule> m_VertexShader;
    TRef<RenderCore::IShaderModule> m_FragmentShader;
    TRef<RenderCore::IBuffer>       m_pFanIndexBuffer;
    TVector<uint32_t>               m_FanIndices;
    TRef<RenderCore::IPipeline>     m_PipelinePermut[TOPOLOGY_MAX][RASTER_STATE_MAX][BLEND_STATE_MAX][DEPTH_STENCIL_MAX][SAMPLER_STATE_MAX];
    TRef<RenderCore::IPipeline>     m_PipelineShapes;
    TRef<RenderCore::IPipeline>     m_PipelineClearStencil;

    int m_RasterState  = RASTER_STATE_CULL;
    int m_BlendState   = CANVAS_COMPOSITE_SOURCE_OVER;
    int m_DepthStencil = DEPTH_STENCIL_DRAW_AA;
    int m_SamplerState = SAMPLER_STATE_CLAMP_CLAMP_LINEAR;    
};

HK_NAMESPACE_END
