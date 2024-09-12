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

#include <Hork/RHI/Common/Device.h>

HK_NAMESPACE_BEGIN

namespace ShaderUtils
{

void CreateShader(RHI::IDevice* device, RHI::SHADER_TYPE shaderType, const char* source, Ref<RHI::IShaderModule>& module);
void CreateShader(RHI::IDevice* device, RHI::SHADER_TYPE shaderType, String const& source, Ref<RHI::IShaderModule>& module);
Ref<RHI::IShaderModule> CreateShaderSpirV(RHI::IDevice* device, RHI::SHADER_TYPE shaderType, BlobRef blob);

void CreateVertexShader(RHI::IDevice* device, StringView fileName, ArrayView<RHI::VertexAttribInfo> vertexAttribs, Ref<RHI::IShaderModule>& module);
void CreateVertexShader(RHI::IDevice* device, StringView fileName, RHI::VertexAttribInfo const* vertexAttribs, size_t numVertexAttribs, Ref<RHI::IShaderModule>& module);
void CreateTessControlShader(RHI::IDevice* device, StringView fileName, Ref<RHI::IShaderModule>& module);
void CreateTessEvalShader(RHI::IDevice* device, StringView fileName, Ref<RHI::IShaderModule>& module);
void CreateGeometryShader(RHI::IDevice* device, StringView fileName, Ref<RHI::IShaderModule>& module);
void CreateFragmentShader(RHI::IDevice* device, StringView fileName, Ref<RHI::IShaderModule>& module);

void CreateFullscreenQuadPipeline(RHI::IDevice* device, Ref<RHI::IPipeline>* ppPipeline, StringView vertexShader, StringView fragmentShader, RHI::PipelineResourceLayout const* pResourceLayout = nullptr, RHI::BLENDING_PRESET blendingPreset = RHI::BLENDING_NO_BLEND);
void CreateFullscreenQuadPipelineGS(RHI::IDevice* device, Ref<RHI::IPipeline>* ppPipeline, StringView vertexShader, StringView fragmentShader, StringView geometryShader, RHI::PipelineResourceLayout const* pResourceLayout = nullptr, RHI::BLENDING_PRESET blendingPreset = RHI::BLENDING_NO_BLEND);

}; // namespace ShaderUtils

HK_NAMESPACE_END
