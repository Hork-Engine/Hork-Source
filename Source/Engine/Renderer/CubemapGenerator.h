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

#include "RenderCommon.h"

struct SCubemapGeneratorUniformBuffer {
    Float4x4 Transform[6];
    Float4 Index;
};

class ACubemapGenerator {
public:
    ACubemapGenerator();

    void GenerateArray( RenderCore::TEXTURE_FORMAT _Format, int _Resolution, int _SourcesCount, RenderCore::ITexture ** _Sources, TRef< RenderCore::ITexture > * ppTextureArray );
    void Generate( RenderCore::TEXTURE_FORMAT _Format, int _Resolution, RenderCore::ITexture * _Source, TRef< RenderCore::ITexture > * ppTexture );

private:
    TRef< RenderCore::IBuffer > m_UniformBuffer;
    SCubemapGeneratorUniformBuffer m_UniformBufferData;
    TRef< RenderCore::IPipeline > m_Pipeline;
    TRef< RenderCore::IRenderPass > m_RP;
};
