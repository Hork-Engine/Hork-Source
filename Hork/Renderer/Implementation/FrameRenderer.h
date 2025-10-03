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

#pragma once

#include "DepthRenderer.h"
#include "LightRenderer.h"
#include "DebugDrawRenderer.h"
#include "WireframeRenderer.h"
#include "NormalsRenderer.h"
#include "BloomRenderer.h"
#include "ExposureRenderer.h"
#include "ColorGradingRenderer.h"
#include "PostprocessRenderer.h"
#include "FxaaRenderer.h"
#include "SmaaRenderer.h"
#include "SSAORenderer.h"
#include "ShadowMapRenderer.h"

HK_NAMESPACE_BEGIN

class FrameRenderer : public RefCounted
{
public:
    FrameRenderer();

    void Render(RHI::FrameGraph& FrameGraph, bool bVirtualTexturing, class VirtualTextureCache* PhysCacheVT);

    OmnidirectionalShadowMapPool const& GetOmniShadowMapPool() const { return m_OmniShadowMapPool; }

private:
    void AddLinearizeDepthPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* DepthTexture, RHI::FGTextureProxy** ppLinearDepth);

    void AddReconstrutNormalsPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* LinearDepth, RHI::FGTextureProxy** ppNormalTexture);

    void AddMotionBlurPass(RHI::FrameGraph&    FrameGraph,
                           RHI::FGTextureProxy*  LightTexture,
                           RHI::FGTextureProxy*  VelocityTexture,
                           RHI::FGTextureProxy*  LinearDepth,
                           RHI::FGTextureProxy** ppResultTexture);

    void AddOutlinePass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy** ppOutlineTexture);
    void AddOutlineOverlayPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* RenderTarget, RHI::FGTextureProxy* OutlineMaskTexture);
    void AddCopyPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* Source, RHI::FGTextureProxy* Dest);

    ShadowMapRenderer    m_ShadowMapRenderer;
    LightRenderer        m_LightRenderer;
    DebugDrawRenderer    m_DebugDrawRenderer;
    BloomRenderer        m_BloomRenderer;
    ExposureRenderer     m_ExposureRenderer;
    ColorGradingRenderer m_ColorGradingRenderer;
    PostprocessRenderer  m_PostprocessRenderer;
    FxaaRenderer         m_FxaaRenderer;
    SmaaRenderer         m_SmaaRenderer;
    SSAORenderer         m_SSAORenderer;

    OmnidirectionalShadowMapPool m_OmniShadowMapPool;

    Ref<RHI::IPipeline> m_LinearDepthPipe;
    Ref<RHI::IPipeline> m_LinearDepthPipe_ORTHO;
    Ref<RHI::IPipeline> m_ReconstructNormalPipe;
    Ref<RHI::IPipeline> m_ReconstructNormalPipe_ORTHO;
    Ref<RHI::IPipeline> m_MotionBlurPipeline;
    Ref<RHI::IPipeline> m_OutlineBlurPipe;
    Ref<RHI::IPipeline> m_OutlineApplyPipe;
    Ref<RHI::IPipeline> m_CopyPipeline;
};

HK_NAMESPACE_END
