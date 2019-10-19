/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

struct FViewUniformBuffer {
    Float4 Timer;
    Float4 ViewPostion;
//            Float4x4 InverseProjectionMatrix;
//            Float4x4 ProjectTranslateViewMatrix;
//            Float4 Viewport;                     // x,y,width,height
//            Float4 InvViewportSize;              // 1/viewportSize, zNear, zFar

//            Float3x4 WorldNormalToViewSpace;
//            Float3x4 ModelNormalToViewSpace;

//            Float3x4 PositionToViewSpace;

//            Float4 ViewWorldPosition;            // x, y, z, t

//            Int4 Counters;                    // Counterd.x = NumDirectionalLights

//            Float3x4 FromObjectToWorldSpace;
};

struct FInstanceUniformBuffer {
    Float4x4 ProjectTranslateViewMatrix;
    Float3x4 ModelNormalToViewSpace;
    Float4 LightmapOffset;
    Float4 uaddr_0;
    Float4 uaddr_1;
    Float4 uaddr_2;
    Float4 uaddr_3;
};

constexpr size_t InstanceUniformBufferSizeof = GHI::UBOAligned( sizeof( FInstanceUniformBuffer ) );

class FFrameResources {
public:
    void Initialize();
    void Deinitialize();

    void UploadUniforms();

    GHI::Buffer ViewUniformBuffer;
    GHI::Buffer InstanceUniformBuffer;
    int         InstanceUniformBufferSize;
    GHI::Buffer UniformBuffer;

    GHI::ShaderResources        Resources;
    GHI::ShaderBufferBinding    BufferBinding[3];
    GHI::ShaderBufferBinding *  ViewUniformBufferBinding;
    GHI::ShaderBufferBinding *  UniformBufferBinding;
    GHI::ShaderBufferBinding *  SkeletonBufferBinding;
    GHI::ShaderTextureBinding   TextureBindings[16];
    GHI::ShaderSamplerBinding   SamplerBindings[16];

private:
    void SetViewUniforms();

    TPodArray< byte > TempData;
};

extern FFrameResources GFrameResources;

}
