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

HK_NAMESPACE_BEGIN

class ShaderFactory
{
public:
    static void sCreateShader(RenderCore::SHADER_TYPE shaderType, const char* source, Ref<RenderCore::IShaderModule>& module);
    static void sCreateShader(RenderCore::SHADER_TYPE shaderType, String const& source, Ref<RenderCore::IShaderModule>& module);
    static Ref<RenderCore::IShaderModule> sCreateShaderSpirV(RenderCore::SHADER_TYPE shaderType, BlobRef blob);

    static void sCreateVertexShader(StringView fileName, ArrayView<RenderCore::VertexAttribInfo> vertexAttribs, Ref<RenderCore::IShaderModule>& module);
    static void sCreateVertexShader(StringView fileName, RenderCore::VertexAttribInfo const* vertexAttribs, size_t numVertexAttribs, Ref<RenderCore::IShaderModule>& module);
    static void sCreateTessControlShader(StringView fileName, Ref<RenderCore::IShaderModule>& module);
    static void sCreateTessEvalShader(StringView fileName, Ref<RenderCore::IShaderModule>& module);
    static void sCreateGeometryShader(StringView fileName, Ref<RenderCore::IShaderModule>& module);
    static void sCreateFragmentShader(StringView fileName, Ref<RenderCore::IShaderModule>& module);

    static void sCreateFullscreenQuadPipeline(Ref<RenderCore::IPipeline>* ppPipeline,
                                              StringView vertexShader,
                                              StringView fragmentShader,
                                              RenderCore::PipelineResourceLayout const* pResourceLayout = nullptr,
                                              RenderCore::BLENDING_PRESET blendingPreset = RenderCore::BLENDING_NO_BLEND);

    static void sCreateFullscreenQuadPipelineGS(Ref<RenderCore::IPipeline>* ppPipeline,
                                                StringView vertexShader,
                                                StringView fragmentShader,
                                                StringView geometryShader,
                                                RenderCore::PipelineResourceLayout const* pResourceLayout = nullptr,
                                                RenderCore::BLENDING_PRESET blendingPreset = RenderCore::BLENDING_NO_BLEND);
};

HK_NAMESPACE_END
