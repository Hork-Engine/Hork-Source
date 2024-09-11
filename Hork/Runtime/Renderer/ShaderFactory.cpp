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

#include "ShaderFactory.h"

#include <Hork/ShaderUtils/ShaderCompiler.h>
#include <Hork/ShaderUtils/ShaderLoader.h>

HK_NAMESPACE_BEGIN

/// Render device
extern RHI::IDevice* GDevice;

using namespace RHI;

Ref<IShaderModule> ShaderFactory::sCreateShaderSpirV(SHADER_TYPE shaderType, BlobRef blob)
{
    ShaderBinaryData binaryData;
    binaryData.ShaderType = shaderType;
    binaryData.BinaryFormat = SHADER_BINARY_FORMAT_SPIR_V_ARB;
    binaryData.BinaryCode = blob.GetData();
    binaryData.BinarySize = blob.Size();

    Ref<IShaderModule> module;
    GDevice->CreateShaderFromBinary(&binaryData, &module);
    return module;
}

void ShaderFactory::sCreateShader(SHADER_TYPE shaderType, const char* source, Ref<IShaderModule>& module)
{
    HeapBlob spirv;

    ShaderCompiler::SourceList sources;
    sources.Add(source);
    if (!ShaderCompiler::sCreateSpirV(shaderType, sources, spirv))
        return;

    module = sCreateShaderSpirV(shaderType, spirv);
}

void ShaderFactory::sCreateShader(SHADER_TYPE shaderType, String const& source, Ref<IShaderModule>& module)
{
    sCreateShader(shaderType, source.CStr(), module);
}

void ShaderFactory::sCreateVertexShader(StringView fileName, ArrayView<VertexAttribInfo> vertexAttribs, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);

    HeapBlob spirv;

    ShaderCompiler::SourceList sources;
    sources.Add(source.CStr());
    if (!ShaderCompiler::sCreateSpirV_VertexShader(vertexAttribs, sources, spirv))
        return;

    module = sCreateShaderSpirV(VERTEX_SHADER, spirv);
}

void ShaderFactory::sCreateVertexShader(StringView fileName, VertexAttribInfo const* vertexAttribs, size_t numVertexAttribs, Ref<IShaderModule>& module)
{
    sCreateVertexShader(fileName, {vertexAttribs, numVertexAttribs}, module);
}

void ShaderFactory::sCreateTessControlShader(StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    sCreateShader(TESS_CONTROL_SHADER, source, module);
}

void ShaderFactory::sCreateTessEvalShader(StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    sCreateShader(TESS_EVALUATION_SHADER, source, module);
}

void ShaderFactory::sCreateGeometryShader(StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    sCreateShader(GEOMETRY_SHADER, source, module);
}

void ShaderFactory::sCreateFragmentShader(StringView fileName, Ref<IShaderModule>& module)
{
    String source = LoadShader(fileName);
    sCreateShader(FRAGMENT_SHADER, source, module);
}

void ShaderFactory::sCreateFullscreenQuadPipeline(Ref<IPipeline>* ppPipeline, StringView vertexShader, StringView fragmentShader, PipelineResourceLayout const* pResourceLayout, BLENDING_PRESET blendingPreset)
{
    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode              = POLYGON_CULL_FRONT;
    rsd.bScissorEnable        = false;

    BlendingStateInfo& bsd = pipelineCI.BS;

    if (blendingPreset != BLENDING_NO_BLEND)
        bsd.RenderTargetSlots[0].SetBlendingPreset(blendingPreset);

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable            = false;
    dssd.bDepthWrite             = false;

    sCreateVertexShader(vertexShader, {}, pipelineCI.pVS);
    sCreateFragmentShader(fragmentShader, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology                    = PRIMITIVE_TRIANGLES;

    if (pResourceLayout)
        pipelineCI.ResourceLayout = *pResourceLayout;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void ShaderFactory::sCreateFullscreenQuadPipelineGS(Ref<IPipeline>* ppPipeline, StringView vertexShader, StringView fragmentShader, StringView geometryShader, PipelineResourceLayout const* pResourceLayout, BLENDING_PRESET blendingPreset)
{
    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode              = POLYGON_CULL_FRONT;
    rsd.bScissorEnable        = false;

    BlendingStateInfo& bsd = pipelineCI.BS;

    if (blendingPreset != BLENDING_NO_BLEND)
        bsd.RenderTargetSlots[0].SetBlendingPreset(blendingPreset);

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable            = false;
    dssd.bDepthWrite             = false;

    sCreateVertexShader(vertexShader, {}, pipelineCI.pVS);
    sCreateGeometryShader(geometryShader, pipelineCI.pGS);
    sCreateFragmentShader(fragmentShader, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology                    = PRIMITIVE_TRIANGLES;

    if (pResourceLayout)
        pipelineCI.ResourceLayout = *pResourceLayout;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

HK_NAMESPACE_END
