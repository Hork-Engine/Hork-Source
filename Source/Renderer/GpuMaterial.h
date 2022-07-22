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

class AMaterialGPU : public ARefCounted
{
public:
    AMaterialGPU(ACompiledMaterial const* pCompiledMaterial);

    AMaterialGPU(AMaterialGPU&& Rhs) = default;
    AMaterialGPU& operator=(AMaterialGPU&& Rhs) = default;

    // Internal. Do not modify:

    MATERIAL_TYPE MaterialType{MATERIAL_TYPE_PBR};

    int LightmapSlot{};

    int DepthPassTextureCount{};
    int LightPassTextureCount{};
    int WireframePassTextureCount{};
    int NormalsPassTextureCount{};
    int ShadowMapPassTextureCount{};

    using PipelineRef = TRef<RenderCore::IPipeline>;

    // NOTE: 0 - Static geometry, 1 - Skinned geometry

    PipelineRef DepthPass[2];
    PipelineRef DepthVelocityPass[2];
    PipelineRef WireframePass[2];
    PipelineRef NormalsPass[2];
    PipelineRef LightPass[2];
    PipelineRef LightPassLightmap;
    PipelineRef LightPassVertexLight;
    PipelineRef ShadowPass[2];
    PipelineRef OmniShadowPass[2];
    PipelineRef FeedbackPass[2];
    PipelineRef OutlinePass[2];
    PipelineRef HUDPipeline;
};
