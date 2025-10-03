/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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
#include <Hork/Core/Logger.h>

#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>

HK_NAMESPACE_BEGIN

void ShaderCompiler::sInitialize()
{
    glslang::InitializeProcess();
}

void ShaderCompiler::sDeinitialize()
{
    glslang::FinalizeProcess();
}

bool ShaderCompiler::sCreateSpirV(RHI::SHADER_TYPE shaderType, SourceList const& sources, HeapBlob& spirv)
{
    const char* shaderTypeMacro[] =
    {
        "#define VERTEX_SHADER\n",
        "#define FRAGMENT_SHADER\n",
        "#define TESS_CONTROL_SHADER\n",
        "#define TESS_EVALUATION_SHADER\n",
        "#define GEOMETRY_SHADER\n",
        "#define COMPUTE_SHADER\n"
    };

    SourceList _sources;
    _sources.Add("#version 450\n\n");
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
    case RHI::VERTEX_SHADER:
        stage = EShLangVertex;
        break;
    case RHI::FRAGMENT_SHADER:
        stage = EShLangFragment;
        break;
    case RHI::TESS_CONTROL_SHADER:
        stage = EShLangTessControl;
        break;
    case RHI::TESS_EVALUATION_SHADER:
        stage = EShLangTessEvaluation;
        break;
    case RHI::GEOMETRY_SHADER:
        stage = EShLangGeometry;
        break;
    case RHI::COMPUTE_SHADER:
        stage = EShLangCompute;
        break;
    default:
        return false;
    }

    const EShMessages messages = EShMsgSpvRules;

    TShader shader(stage);
    shader.setStrings(_sources.ToPtr(), _sources.Size());
    shader.setEnvInput(EShSourceGlsl, stage, EShClientOpenGL, 450);
    shader.setEnvClient(EShClientOpenGL, EShTargetOpenGL_450);
    shader.setEnvTarget(EShTargetSpv, EShTargetSpv_1_0); // FIXME: Which target version of SpirV should we use?

    const int defaultVersion = 100;
    if (!shader.parse(GetDefaultResources(), defaultVersion, false, messages))
    {
        LOG("{}\n{}\n", shader.getInfoLog(), shader.getInfoDebugLog());
        return {};
    }

    TProgram program;
    program.addShader(&shader);
    if (!program.link(messages))
    {
        LOG("{}\n{}\n", program.getInfoLog(), program.getInfoDebugLog());
        return false;
    }

    // A small code snippet to log the pre-processed code.
    //std::string preprocessedCode;
    //TShader::ForbidIncluder forbidIncluder;
    //if (shader.preprocess(GetDefaultResources(), defaultVersion, ENoProfile, false, false, messages, &preprocessedCode, forbidIncluder))
    //{
    //    if (auto file = File::OpenWrite("debug.glsl"))
    //        file.Write(preprocessedCode.c_str(), preprocessedCode.size());
    //}

    SpvOptions options;
    options.stripDebugInfo = true;
    options.disableOptimizer = false;
    options.optimizeSize = true;
    options.validate = true;

    spv::SpvBuildLogger logger;

    std::vector<uint32_t> _spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), _spirv, &logger, &options);
    spirv.Reset(_spirv.size() * sizeof(_spirv[0]), _spirv.data());

    std::string loggerMessages = logger.getAllMessages();
    if (!loggerMessages.empty())
        LOG("{}\n", loggerMessages);

    return !spirv.IsEmpty();
}

namespace
{
    /// Vertex attrubte to shader string helper
    String ShaderStringForVertexAttribs(ArrayView<RHI::VertexAttribInfo> vertexAttribs)
    {
        using namespace RHI;

        String      s;
        const char* attribType;

        const char* Types[4][4] = {
            {"float", "vec2", "vec3", "vec4"},     // Float types
            {"double", "dvec2", "dvec3", "dvec4"}, // Double types
            {"int", "ivec2", "ivec3", "ivec4"},    // Integer types
            {"uint", "uvec2", "uvec3", "uvec4"}    // Unsigned types
        };

        for (RHI::VertexAttribInfo const& attrib : vertexAttribs)
        {
            VERTEX_ATTRIB_COMPONENT typeOfComponent = attrib.TypeOfComponent();

            if (attrib.Mode == VAM_INTEGER && (typeOfComponent == COMPONENT_UBYTE || typeOfComponent == COMPONENT_USHORT || typeOfComponent == COMPONENT_UINT))
                attribType = Types[3][attrib.NumComponents() - 1];
            else
                attribType = Types[attrib.Mode][attrib.NumComponents() - 1];

            s += HK_FORMAT("layout(location = {}) in {} {};\n", attrib.Location, attribType, attrib.SemanticName);
        }

        return s;
    }
}

bool ShaderCompiler::sCreateSpirV_VertexShader(ArrayView<RHI::VertexAttribInfo> vertexAttribs, SourceList const& sources, HeapBlob& spirv)
{
    SourceList _sources;
    String attribs = ShaderStringForVertexAttribs(vertexAttribs);

    if (!attribs.IsEmpty())
        _sources.Add(attribs.CStr());
    _sources.Add(sources);

    return sCreateSpirV(RHI::VERTEX_SHADER, _sources, spirv);
}

HK_NAMESPACE_END
