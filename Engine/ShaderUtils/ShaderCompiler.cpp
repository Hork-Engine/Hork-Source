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

#include "ShaderCompiler.h"
#include <Engine/Core/Logger.h>

#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>

HK_NAMESPACE_BEGIN

void ShaderCompiler::Initialize()
{
    glslang::InitializeProcess();
}

void ShaderCompiler::Deinitialize()
{
    glslang::FinalizeProcess();
}

bool ShaderCompiler::CreateSpirV(RenderCore::SHADER_TYPE shaderType, SourceList const& sources, HeapBlob& spirv)
{
    SourceList _sources;

    const char* shaderTypeMacro[] =
    {
        "#define VERTEX_SHADER\n",
        "#define FRAGMENT_SHADER\n",
        "#define TESS_CONTROL_SHADER\n",
        "#define TESS_EVALUATION_SHADER\n",
        "#define GEOMETRY_SHADER\n",
        "#define COMPUTE_SHADER\n"
    };

    _sources.Add("#version 450\n");
    _sources.Add("#extension GL_GOOGLE_cpp_style_line_directive : enable\n");
    _sources.Add("#extension GL_EXT_control_flow_attributes : enable\n");
    _sources.Add("#extension GL_EXT_control_flow_attributes2 : enable\n");
    _sources.Add("#extension GL_ARB_fragment_coord_conventions : enable\n");
    _sources.Add("#define SRGB_GAMMA_APPROX\n");
    _sources.Add(shaderTypeMacro[shaderType]);
    _sources.Add(sources);

    using namespace glslang;

    EShLanguage stage;

    switch (shaderType)
    {
    case RenderCore::VERTEX_SHADER:
        stage = EShLangVertex;
        break;
    case RenderCore::FRAGMENT_SHADER:
        stage = EShLangFragment;
        break;
    case RenderCore::TESS_CONTROL_SHADER:
        stage = EShLangTessControl;
        break;
    case RenderCore::TESS_EVALUATION_SHADER:
        stage = EShLangTessEvaluation;
        break;
    case RenderCore::GEOMETRY_SHADER:
        stage = EShLangGeometry;
        break;
    case RenderCore::COMPUTE_SHADER:
        stage = EShLangCompute;
        break;
    default:
        return false;
    }

    TShader shader(stage);

    TProgram program;

    EShMessages messages = EShMsgSpvRules;

    shader.setStrings(_sources.ToPtr(), _sources.Size());
    shader.setEnvInput(EShSourceGlsl, stage, EShClientOpenGL, 450);
    shader.setEnvClient(EShClientOpenGL, EShTargetOpenGL_450);
    shader.setEnvTarget(EShTargetSpv, EShTargetSpv_1_5);

    const int defaultVersion = 110; // use 100 for ES environment, 110 for desktop
    if (!shader.parse(GetDefaultResources(), defaultVersion, false, messages))
    {
        LOG("{}\n{}\n", shader.getInfoLog(), shader.getInfoDebugLog());
        return {};
    }

    program.addShader(&shader);

    if (!program.link(messages))
    {
        LOG("{}\n{}\n", program.getInfoLog(), program.getInfoDebugLog());
        return false;
    }

    SpvOptions options;
    options.stripDebugInfo = true;
    options.disableOptimizer = false;
    options.optimizeSize = true;
    options.validate = true;

    std::vector<uint32_t> _spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), _spirv, &options);
    spirv.Reset(_spirv.size() * sizeof(_spirv[0]), _spirv.data());
    return true;
}

namespace
{
    /// Vertex attrubte to shader string helper
    String ShaderStringForVertexAttribs(ArrayView<RenderCore::VertexAttribInfo> vertexAttribs)
    {
        using namespace RenderCore;

        String      s;
        const char* attribType;

        const char* Types[4][4] = {
            {"float", "vec2", "vec3", "vec4"},     // Float types
            {"double", "dvec2", "dvec3", "dvec4"}, // Double types
            {"int", "ivec2", "ivec3", "ivec4"},    // Integer types
            {"uint", "uvec2", "uvec3", "uvec4"}    // Unsigned types
        };

        for (RenderCore::VertexAttribInfo const& attrib : vertexAttribs)
        {
            VERTEX_ATTRIB_COMPONENT typeOfComponent = attrib.TypeOfComponent();

            if (attrib.Mode == VAM_INTEGER && (typeOfComponent == COMPONENT_UBYTE || typeOfComponent == COMPONENT_USHORT || typeOfComponent == COMPONENT_UINT))
            {
                attribType = Types[3][attrib.NumComponents() - 1];
            }
            else
            {
                attribType = Types[attrib.Mode][attrib.NumComponents() - 1];
            }

            s += HK_FORMAT("layout(location = {}) in {} {};\n", attrib.Location, attribType, attrib.SemanticName);
        }

        return s;
    }
}

bool ShaderCompiler::CreateSpirV_VertexShader(ArrayView<RenderCore::VertexAttribInfo> vertexAttribs, SourceList const& sources, HeapBlob& spirv)
{
    SourceList _sources;
    String attribs = ShaderStringForVertexAttribs(vertexAttribs);

    if (!attribs.IsEmpty())
        _sources.Add(attribs.CStr());
    _sources.Add(sources);

    return CreateSpirV(RenderCore::VERTEX_SHADER, _sources, spirv);
}

HK_NAMESPACE_END
