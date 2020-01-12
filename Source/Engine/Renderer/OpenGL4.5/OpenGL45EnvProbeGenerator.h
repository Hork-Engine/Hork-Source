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

struct SRoughnessUniformBuffer {
    Float4x4 Transform[6];
    Float4 Roughness;
};

class AEnvProbeGenerator {
public:
    void Initialize();
    void Deinitialize();
    void GenerateArray( GHI::Texture & _CubemapArray, int _MaxLod, int _CubemapsCount, GHI::Texture ** _Cubemaps );
    void Generate( GHI::Texture & _Cubemap, int _MaxLod, GHI::Texture * _SourceCubemap );

private:
    GHI::Buffer m_VertexBuffer;
    GHI::Buffer m_IndexBuffer;
    GHI::Buffer m_UniformBuffer;
    SRoughnessUniformBuffer m_UniformBufferData;
    GHI::Pipeline m_Pipeline;
    GHI::Sampler m_Sampler;
    GHI::RenderPass m_RP;
    int m_IndexCount;
};

}
