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

#include "ShaderUtils.h"
#include "ShaderCompiler.h"
#include "ShaderLoader.h"

HK_NAMESPACE_BEGIN

namespace ShaderUtils
{

using namespace RHI;

Ref<IShaderModule> CreateShaderSpirV(IDevice* device, SHADER_TYPE shaderType, BlobRef blob)
{
    ShaderBinaryData binaryData;
    binaryData.ShaderType = shaderType;
    binaryData.BinaryFormat = SHADER_BINARY_FORMAT_SPIR_V_ARB;
    binaryData.BinaryCode = blob.GetData();
    binaryData.BinarySize = blob.Size();

    Ref<IShaderModule> module;
    device->CreateShaderFromBinary(&binaryData, &module);
    return module;
}

void CreateShader(IDevice* device, SHADER_TYPE shaderType, const char* source, Ref<IShaderModule>& module)
{
    HeapBlob spirv;

    ShaderCompiler::SourceList sources;
    sources.Add(source);
    if (!ShaderCompiler::sCreateSpirV(shaderType, sources, spirv))
        return;

    module = CreateShaderSpirV(device, shaderType, spirv);
}

void CreateShader(IDevice* device, SHADER_TYPE shaderType, String const& source, Ref<IShaderModule>& module)
{
    CreateShader(device, shaderType, source.CStr(), module);
}

void CreateVertexShader(IDevice* device, StringView fileName, ArrayView<VertexAttribInfo> vertexAttribs, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);

    HeapBlob spirv;

    ShaderCompiler::SourceList sources;
    sources.Add(source.CStr());
    if (!ShaderCompiler::sCreateSpirV_VertexShader(vertexAttribs, sources, spirv))
        return;

    module = CreateShaderSpirV(device, VERTEX_SHADER, spirv);
}

void CreateVertexShader(IDevice* device, StringView fileName, VertexAttribInfo const* vertexAttribs, size_t numVertexAttribs, Ref<IShaderModule>& module)
{
    CreateVertexShader(device, fileName, {vertexAttribs, numVertexAttribs}, module);
}

void CreateTessControlShader(IDevice* device, StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    CreateShader(device, TESS_CONTROL_SHADER, source, module);
}

void CreateTessEvalShader(IDevice* device, StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    CreateShader(device, TESS_EVALUATION_SHADER, source, module);
}

void CreateGeometryShader(IDevice* device, StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    CreateShader(device, GEOMETRY_SHADER, source, module);
}

void CreateFragmentShader(IDevice* device, StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    CreateShader(device, FRAGMENT_SHADER, source, module);
}

void CreateFullscreenQuadPipeline(IDevice* device, Ref<IPipeline>* ppPipeline, StringView vertexShader, StringView fragmentShader, PipelineResourceLayout const* pResourceLayout, BLENDING_PRESET blendingPreset)
{
    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

    BlendingStateInfo& bsd = pipelineCI.BS;

    if (blendingPreset != BLENDING_NO_BLEND)
        bsd.RenderTargetSlots[0].SetBlendingPreset(blendingPreset);

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

    CreateVertexShader(device, vertexShader, {}, pipelineCI.pVS);
    CreateFragmentShader(device, fragmentShader, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;

    if (pResourceLayout)
        pipelineCI.ResourceLayout = *pResourceLayout;

    device->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateFullscreenQuadPipelineGS(IDevice* device, Ref<IPipeline>* ppPipeline, StringView vertexShader, StringView fragmentShader, StringView geometryShader, PipelineResourceLayout const* pResourceLayout, BLENDING_PRESET blendingPreset)
{
    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

    BlendingStateInfo& bsd = pipelineCI.BS;

    if (blendingPreset != BLENDING_NO_BLEND)
        bsd.RenderTargetSlots[0].SetBlendingPreset(blendingPreset);

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

    CreateVertexShader(device, vertexShader, {}, pipelineCI.pVS);
    CreateGeometryShader(device, geometryShader, pipelineCI.pGS);
    CreateFragmentShader(device, fragmentShader, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;

    if (pResourceLayout)
        pipelineCI.ResourceLayout = *pResourceLayout;

    device->CreatePipeline(pipelineCI, ppPipeline);
}

} // namespace ShaderUtils

HK_NAMESPACE_END
