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

#pragma once

#include "OpenGL45DepthRenderer.h"
#include "OpenGL45LightRenderer.h"
#include "OpenGL45DebugDrawRenderer.h"
#include "OpenGL45WireframeRenderer.h"
#include "OpenGL45NormalsRenderer.h"
#include "OpenGL45BloomRenderer.h"
#include "OpenGL45ExposureRenderer.h"
#include "OpenGL45ColorGradingRenderer.h"
#include "OpenGL45PostprocessRenderer.h"
#include "OpenGL45FxaaRenderer.h"
#include "OpenGL45SSAORenderer.h"
#include "OpenGL45ShadowMapRenderer.h"

namespace OpenGL45 {

class AFrameRenderer
{
public:
    AFrameRenderer();

    struct SFrameGraphCaptured
    {
        AFrameGraphTextureStorage * FinalTexture;
    };

    void Render( AFrameGraph & FrameGraph, SFrameGraphCaptured & CapturedResources );

private:
    AFrameGraphTextureStorage * AddLinearizeDepthPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * DepthTexture );

    AFrameGraphTextureStorage * AddReconstrutNormalsPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * LinearDepth );

    AShadowMapRenderer ShadowMapRenderer;
    ADepthRenderer DepthRenderer;
    ALightRenderer LightRenderer;
    AWireframeRenderer WireframeRenderer;
    ANormalsRenderer NormalsRenderer;
    ADebugDrawRenderer DebugDrawRenderer;
    ABloomRenderer BloomRenderer;
    AExposureRenderer ExposureRenderer;
    AColorGradingRenderer ColorGradingRenderer;
    APostprocessRenderer PostprocessRenderer;
    AFxaaRenderer FxaaRenderer;
    ASSAORenderer SSAORenderer;

    GHI::Pipeline LinearDepthPipe;
    GHI::Pipeline LinearDepthPipe_ORTHO;
    GHI::Pipeline ReconstructNormalPipe;
    GHI::Pipeline ReconstructNormalPipe_ORTHO;

    GHI::Sampler NearestSampler;
};

}
