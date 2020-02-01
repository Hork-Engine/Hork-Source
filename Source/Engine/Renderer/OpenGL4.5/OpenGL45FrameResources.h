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

#include "OpenGL45Common.h"

namespace OpenGL45 {

struct SViewUniformBuffer {
    Float4x4 OrthoProjection;
    Float4x4 ModelviewProjection;
    Float4x4 InverseProjectionMatrix;
    Float3x4 WorldNormalToViewSpace;

    // ViewportParams
    Float2 InvViewportSize;
    float ZNear;
    float ZFar;

    // Timers
    float GameRunningTimeSeconds;
    float GameplayTimeSeconds;
    float Padding0;
    float Padding1;

    Float3 ViewPostion;
    float Padding2;

    uint64_t EnvProbeSampler;
    uint64_t Padding3;

    int32_t NumDirectionalLights;
    int32_t Padding4;
    int32_t Padding5;
    int32_t DebugMode;

    Float4 LightDirs[MAX_DIRECTIONAL_LIGHTS];            // Direction, W-channel is not used
    Float4 LightColors[MAX_DIRECTIONAL_LIGHTS];          // RGB, alpha - ambient intensity
    uint32_t LightParameters[MAX_DIRECTIONAL_LIGHTS][4];      // RenderMask, FirstCascade, NumCascades, W-channel is not used
    
    //Float4 Viewport;                     // x,y,width,height
    //Float3x4 PositionToViewSpace;
    //Float3x4 FromObjectToWorldSpace;
};

struct SInstanceUniformBuffer {
    Float4x4 TransformMatrix;
    Float3x4 ModelNormalToViewSpace;
    Float4 LightmapOffset;
    Float4 uaddr_0;
    Float4 uaddr_1;
    Float4 uaddr_2;
    Float4 uaddr_3;
};

struct SShadowInstanceUniformBuffer {
    Float4x4 TransformMatrix; // TODO: 3x4
    // For material with vertex deformations:
    Float4 uaddr_0;
    Float4 uaddr_1;
    Float4 uaddr_2;
    Float4 uaddr_3;
};

class AFrameResources {
public:
    void Initialize();
    void Deinitialize();

    void UploadUniforms();

    GHI::Buffer ViewUniformBuffer;

    GHI::Buffer InstanceUniformBuffer;
    int         InstanceUniformBufferSize;
    int         InstanceUniformBufferSizeof;

    GHI::Buffer ShadowInstanceUniformBuffer;
    int         ShadowInstanceUniformBufferSize;
    int         ShadowInstanceUniformBufferSizeof;

    GHI::Buffer CascadeViewProjectionBuffer;
    GHI::Texture EnvProbe;
    GHI::Sampler EnvProbeSampler;
    GHI::BindlessSampler EnvProbeBindless;
    SViewUniformBuffer ViewUniformBufferUniformData;
    GHI::Texture ClusterLookup;
    GHI::Sampler ClusterLookupSampler;
    GHI::Texture ClusterItemTBO;
    GHI::Buffer  ClusterItemBuffer;
    GHI::Buffer  LightBuffer;

    GHI::ShaderResources        Resources;
    GHI::ShaderBufferBinding    BufferBinding[5];
    GHI::ShaderBufferBinding *  ViewUniformBufferBinding;
    GHI::ShaderBufferBinding *  InstanceUniformBufferBinding;
    GHI::ShaderBufferBinding *  SkeletonBufferBinding;
    GHI::ShaderBufferBinding *  CascadeBufferBinding;
    GHI::ShaderBufferBinding *  LightBufferBinding;
    GHI::ShaderTextureBinding   TextureBindings[16];
    GHI::ShaderSamplerBinding   SamplerBindings[16];

private:
    void SetViewUniforms();

    TPodArrayHeap< byte > TempData;
};

extern AFrameResources GFrameResources;

}
