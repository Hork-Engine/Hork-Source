/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#include "ShaderLoader.h"

#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_MaterialDebugMode("r_MaterialDebugMode"s,
#ifdef HK_DEBUG
                                "1"s,
#else
                                "0"s,
#endif
                                CVAR_CHEAT);

extern ConsoleVar r_SSLR;
extern ConsoleVar r_HBAO;

/** Render device */
extern RenderCore::IDevice* GDevice;

using namespace RenderCore;

void ShaderFactory::CreateShader(SHADER_TYPE ShaderType, SourceList const& Sources, TRef<IShaderModule>& Module)
{
    SourceList sources;

    const char* predefine[] = {
        "#define VERTEX_SHADER\n",
        "#define FRAGMENT_SHADER\n",
        "#define TESS_CONTROL_SHADER\n",
        "#define TESS_EVALUATION_SHADER\n",
        "#define GEOMETRY_SHADER\n",
        "#define COMPUTE_SHADER\n"};

    String predefines = predefine[ShaderType];

    switch (GDevice->GetGraphicsVendor())
    {
        case VENDOR_NVIDIA:
            predefines += "#define NVIDIA\n";
            break;
        case VENDOR_ATI:
            predefines += "#define ATI\n";
            break;
        case VENDOR_INTEL:
            predefines += "#define INTEL\n";
            break;
        default:
            // skip "not handled enumeration" warning
            break;
    }

    predefines += HK_FORMAT("#define MAX_DIRECTIONAL_LIGHTS {}\n", MAX_DIRECTIONAL_LIGHTS);
    predefines += HK_FORMAT("#define MAX_SHADOW_CASCADES {}\n", MAX_SHADOW_CASCADES);
    predefines += HK_FORMAT("#define MAX_TOTAL_SHADOW_CASCADES_PER_VIEW {}\n", MAX_TOTAL_SHADOW_CASCADES_PER_VIEW);


#ifdef SHADOWMAP_PCF
    predefines += "#define SHADOWMAP_PCF\n";
#endif
#ifdef SHADOWMAP_PCSS
    predefines += "#define SHADOWMAP_PCSS\n";
#endif
#ifdef SHADOWMAP_VSM
    predefines += "#define SHADOWMAP_VSM\n";
#endif
#ifdef SHADOWMAP_EVSM
    predefines += "#define SHADOWMAP_EVSM\n";
#endif
    if (r_MaterialDebugMode)
    {
        predefines += "#define DEBUG_RENDER_MODE\n";
    }

    predefines += "#define SRGB_GAMMA_APPROX\n";

    if (r_SSLR)
    {
        predefines += "#define WITH_SSLR\n";
    }

    if (r_HBAO)
    {
        predefines += "#define WITH_SSAO\n";
    }

    sources.Add("#version 450\n");
    sources.Add("#extension GL_ARB_bindless_texture : enable\n");
    sources.Add(predefines.CStr());
    sources.Add(Sources);

    // Print sources
#if 0
    LOG("============================ SOURCE ==============================\n");
    for ( int i = 0 ; i < sources.Size() ; i++ ) {
        LOG( "{} : {}\n", i, sources[i] );
    }
#endif

    using namespace RenderCore;

    GDevice->CreateShaderFromCode(ShaderType, sources.Size(), sources.ToPtr(), &Module);
}

void ShaderFactory::CreateShader(SHADER_TYPE ShaderType, const char* Source, TRef<IShaderModule>& Module)
{
    SourceList sources;
    sources.Add(Source);
    CreateShader(ShaderType, sources, Module);
}

void ShaderFactory::CreateShader(SHADER_TYPE ShaderType, String const& Source, TRef<IShaderModule>& Module)
{
    CreateShader(ShaderType, Source.CStr(), Module);
}

void ShaderFactory::CreateVertexShader(StringView FileName, VertexAttribInfo const* VertexAttribs, int NumVertexAttribs, TRef<IShaderModule>& Module)
{
    // TODO: here check if the shader binary is cached. Load from cache if so.

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(VertexAttribs, NumVertexAttribs);
    String source                    = ShaderLoader{}.LoadShader(FileName);

    SourceList sources;

    if (!vertexAttribsShaderString.IsEmpty())
        sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(source.CStr());

    CreateShader(VERTEX_SHADER, sources, Module);

    // TODO: Write shader binary to cache
}

void ShaderFactory::CreateTessControlShader(StringView FileName, TRef<IShaderModule>& Module)
{
    String source = ShaderLoader{}.LoadShader(FileName);
    CreateShader(TESS_CONTROL_SHADER, source, Module);
}

void ShaderFactory::CreateTessEvalShader(StringView FileName, TRef<IShaderModule>& Module)
{
    String source = ShaderLoader{}.LoadShader(FileName);
    CreateShader(TESS_EVALUATION_SHADER, source, Module);
}

void ShaderFactory::CreateGeometryShader(StringView FileName, TRef<IShaderModule>& Module)
{
    String source = ShaderLoader{}.LoadShader(FileName);
    CreateShader(GEOMETRY_SHADER, source, Module);
}

void ShaderFactory::CreateFragmentShader(StringView FileName, TRef<IShaderModule>& Module)
{
    String source = ShaderLoader{}.LoadShader(FileName);
    CreateShader(FRAGMENT_SHADER, source, Module);
}

void ShaderFactory::CreateFullscreenQuadPipeline(TRef<IPipeline>* ppPipeline, StringView VertexShader, StringView FragmentShader, PipelineResourceLayout const* pResourceLayout, BLENDING_PRESET BlendingPreset)
{
    using namespace RenderCore;

    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode              = POLYGON_CULL_FRONT;
    rsd.bScissorEnable        = false;

    BlendingStateInfo& bsd = pipelineCI.BS;

    if (BlendingPreset != BLENDING_NO_BLEND)
    {
        bsd.RenderTargetSlots[0].SetBlendingPreset(BlendingPreset);
    }

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable            = false;
    dssd.bDepthWrite             = false;

    CreateVertexShader(VertexShader, nullptr, 0, pipelineCI.pVS);
    CreateFragmentShader(FragmentShader, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology                    = PRIMITIVE_TRIANGLES;

    if (pResourceLayout)
    {
        pipelineCI.ResourceLayout = *pResourceLayout;
    }

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void ShaderFactory::CreateFullscreenQuadPipelineGS(TRef<IPipeline>* ppPipeline, StringView VertexShader, StringView FragmentShader, StringView GeometryShader, PipelineResourceLayout const* pResourceLayout, BLENDING_PRESET BlendingPreset)
{
    using namespace RenderCore;

    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode              = POLYGON_CULL_FRONT;
    rsd.bScissorEnable        = false;

    BlendingStateInfo& bsd = pipelineCI.BS;

    if (BlendingPreset != BLENDING_NO_BLEND)
    {
        bsd.RenderTargetSlots[0].SetBlendingPreset(BlendingPreset);
    }

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable            = false;
    dssd.bDepthWrite             = false;

    CreateVertexShader(VertexShader, nullptr, 0, pipelineCI.pVS);
    CreateGeometryShader(GeometryShader, pipelineCI.pGS);
    CreateFragmentShader(FragmentShader, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology                    = PRIMITIVE_TRIANGLES;

    if (pResourceLayout)
    {
        pipelineCI.ResourceLayout = *pResourceLayout;
    }

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

HK_NAMESPACE_END
