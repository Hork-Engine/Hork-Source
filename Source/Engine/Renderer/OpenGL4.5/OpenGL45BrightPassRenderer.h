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

#include "OpenGL45PassRenderer.h"

namespace OpenGL45 {

class ABrightPassRenderer : public APassRenderer {
public:
    void Initialize();
    void Deinitialize();

    void Render( GHI::Texture & _SrcTexture );

    GHI::RenderPass * GetRenderPass() { return &BrightPass; }

private:
    void CreateBrightPipeline();
    void CreateBlurPipeline();
    void CreateLuminancePipeline();
    void CreateSampler();

    GHI::RenderPass BrightPass;
    GHI::Pipeline BrightPipeline;
    GHI::Pipeline BlurPipeline0;
    GHI::Pipeline BlurPipeline1;
    GHI::Pipeline BlurFinalPipeline0;
    GHI::Pipeline BlurFinalPipeline1;
    GHI::RenderPass LuminancePass;
    GHI::Pipeline MakeLuminanceMapPipe;
    GHI::Pipeline SumLuminanceMapPipe;
    GHI::Pipeline DynamicExposurePipe;
    GHI::Sampler NearestSampler;
    GHI::Sampler LinearSampler;
    GHI::Sampler DitherSampler;
    GHI::Sampler LuminanceSampler;
    GHI::ShaderModule BlurFragmentShaderModule;
    GHI::ShaderModule BlurFinalFragmentShaderModule;
};

extern ABrightPassRenderer GBrightPassRenderer;

}

