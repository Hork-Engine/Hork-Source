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

#include "MaterialCode.h"
#include "MaterialBinary.h"
#include "MaterialSamplers.h"
#include <Engine/ShaderUtils/ShaderCompiler.h>
#include <Engine/Renderer/VertexAttribs.h>
#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

using namespace RenderCore;

ConsoleVar r_MaterialDebugMode("r_MaterialDebugMode"_s,
#ifdef HK_DEBUG
    "1"_s,
#else
    "0"_s,
#endif
    CVAR_CHEAT);

extern ConsoleVar r_SSLR;
extern ConsoleVar r_HBAO;

namespace
{
    constexpr SAMPLER_FILTER SamplerFilterLUT[] =
    {
        FILTER_LINEAR,
        FILTER_NEAREST,
        FILTER_MIPMAP_NEAREST,
        FILTER_MIPMAP_BILINEAR,
        FILTER_MIPMAP_NLINEAR,
        FILTER_MIPMAP_TRILINEAR
    };

    constexpr SAMPLER_ADDRESS_MODE SamplerAddressLUT[] =
    {
        SAMPLER_ADDRESS_WRAP,
        SAMPLER_ADDRESS_MIRROR,
        SAMPLER_ADDRESS_CLAMP,
        SAMPLER_ADDRESS_BORDER,
        SAMPLER_ADDRESS_MIRROR_ONCE
    };

    void AddSamplers(Vector<SamplerDesc>& samplers, ArrayView<TextureSampler> input)
    {
        samplers.Reserve(input.Size());
        for (auto& in : input)
        {
            samplers.EmplaceBack()
                .SetFilter(SamplerFilterLUT[in.Filter])
                .SetAddressU(SamplerAddressLUT[in.AddressU])
                .SetAddressV(SamplerAddressLUT[in.AddressV])
                .SetAddressW(SamplerAddressLUT[in.AddressW])
                .SetMaxAnisotropy(in.Anisotropy)
                .SetCubemapSeamless(true)
                .SetMipLODBias(in.MipLODBias)
                .SetMinLOD(in.MinLod)
                .SetMaxLOD(in.MaxLod);
        }
    }

    RenderCore::BLENDING_PRESET GetBlendingPreset(BLENDING_MODE _Blending)
    {
        switch (_Blending)
        {
        case COLOR_BLENDING_ALPHA:
            return RenderCore::BLENDING_ALPHA;
        case COLOR_BLENDING_DISABLED:
            return RenderCore::BLENDING_NO_BLEND;
        case COLOR_BLENDING_PREMULTIPLIED_ALPHA:
            return RenderCore::BLENDING_PREMULTIPLIED_ALPHA;
        case COLOR_BLENDING_COLOR_ADD:
            return RenderCore::BLENDING_COLOR_ADD;
        case COLOR_BLENDING_MULTIPLY:
            return RenderCore::BLENDING_MULTIPLY;
        case COLOR_BLENDING_SOURCE_TO_DEST:
            return RenderCore::BLENDING_SOURCE_TO_DEST;
        case COLOR_BLENDING_ADD_MUL:
            return RenderCore::BLENDING_ADD_MUL;
        case COLOR_BLENDING_ADD_ALPHA:
            return RenderCore::BLENDING_ADD_ALPHA;
        default:
            HK_ASSERT(0);
        }
        return RenderCore::BLENDING_NO_BLEND;
    }
}

MaterialCode::MaterialCode()
{
    HasVertexDeform          = false;
    DepthTest_EXPERIMENTAL   = false;
    NoCastShadow             = false;
    HasAlphaMasking          = false;
    HasShadowMapMasking      = false;
    DisplacementAffectShadow = false;
    IsTranslucent            = false;
    IsTwoSided               = false;
}

void MaterialCode::AddCodeBlock(String sourceName, String sourceCode)
{
    CodeBlocks.EmplaceBack(std::move(sourceName), std::move(sourceCode));
}

void MaterialCode::Read(IBinaryStreamReadInterface& stream)
{
    Type                      = (MATERIAL_TYPE)stream.ReadUInt8();
    Blending                  = (BLENDING_MODE)stream.ReadUInt8();
    TessellationMethod        = (TESSELLATION_METHOD)stream.ReadUInt8();
    RenderingPriority         = (RENDERING_PRIORITY)stream.ReadUInt8();
    LightmapSlot              = stream.ReadUInt16();
    DepthPassTextureCount     = stream.ReadUInt8();
    LightPassTextureCount     = stream.ReadUInt8();
    WireframePassTextureCount = stream.ReadUInt8();
    NormalsPassTextureCount   = stream.ReadUInt8();
    ShadowMapPassTextureCount = stream.ReadUInt8();
    HasVertexDeform           = stream.ReadBool();
    DepthTest_EXPERIMENTAL    = stream.ReadBool();
    NoCastShadow              = stream.ReadBool();
    HasAlphaMasking           = stream.ReadBool();
    HasShadowMapMasking       = stream.ReadBool();
    DisplacementAffectShadow  = stream.ReadBool();
    //ParallaxMappingSelfShadowing = stream.ReadBool(); // TODO
    IsTranslucent             = stream.ReadBool();
    IsTwoSided                = stream.ReadBool();
    NumUniformVectors         = stream.ReadUInt8();

    Samplers.Clear();
    Samplers.Resize(stream.ReadUInt8());
    for (auto& sampler : Samplers)
    {
        sampler.TextureType = (TEXTURE_TYPE)stream.ReadUInt8();
        sampler.Filter      = (TEXTURE_FILTER)stream.ReadUInt8();
        sampler.AddressU    = (TEXTURE_ADDRESS)stream.ReadUInt8();
        sampler.AddressV    = (TEXTURE_ADDRESS)stream.ReadUInt8();
        sampler.AddressW    = (TEXTURE_ADDRESS)stream.ReadUInt8();
        sampler.MipLODBias  = stream.ReadFloat();
        sampler.Anisotropy  = stream.ReadFloat();
        sampler.MinLod      = stream.ReadFloat();
        sampler.MaxLod      = stream.ReadFloat();
    }

    auto numShaders = stream.ReadUInt16();

    CodeBlocks.Clear();
    CodeBlocks.Reserve(numShaders);

    for (int i = 0; i < numShaders; i++)
    {
        String sourceName = stream.ReadString();
        String sourceCode = stream.ReadString();
        AddCodeBlock(std::move(sourceName), std::move(sourceCode));
    }
}

void MaterialCode::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt8(Type);
    stream.WriteUInt8(Blending);
    stream.WriteUInt8(TessellationMethod);
    stream.WriteUInt8(RenderingPriority);        
    stream.WriteUInt16(LightmapSlot);
    stream.WriteUInt8(DepthPassTextureCount);
    stream.WriteUInt8(LightPassTextureCount);
    stream.WriteUInt8(WireframePassTextureCount);
    stream.WriteUInt8(NormalsPassTextureCount);
    stream.WriteUInt8(ShadowMapPassTextureCount);
    stream.WriteBool(HasVertexDeform);
    stream.WriteBool(DepthTest_EXPERIMENTAL);
    stream.WriteBool(NoCastShadow);
    stream.WriteBool(HasAlphaMasking);
    stream.WriteBool(HasShadowMapMasking);
    stream.WriteBool(DisplacementAffectShadow);
    //stream.WriteBool(ParallaxMappingSelfShadowing); // TODO
    stream.WriteBool(IsTranslucent);
    stream.WriteBool(IsTwoSided);
    stream.WriteUInt8(NumUniformVectors);

    stream.WriteUInt8(Samplers.Size());
    for (auto& sampler : Samplers)
    {
        stream.WriteUInt8(sampler.TextureType);
        stream.WriteUInt8(sampler.Filter);
        stream.WriteUInt8(sampler.AddressU);
        stream.WriteUInt8(sampler.AddressV);
        stream.WriteUInt8(sampler.AddressW);
        stream.WriteFloat(sampler.MipLODBias);
        stream.WriteFloat(sampler.Anisotropy);
        stream.WriteFloat(sampler.MinLod);
        stream.WriteFloat(sampler.MaxLod);
    }

    stream.WriteUInt16(CodeBlocks.Size());
    for (CodeBlock const& s : CodeBlocks)
    {
        stream.WriteString(s.Name);
        stream.WriteString(s.Code);
    }
}

struct MaterialCommonProperties
{
    bool Tessellation;
    bool TessellationShadowMap;
    bool AlphaMasking;
    bool ShadowMasking;
    String Code;
};

struct MaterialPassTranslator
{
    HeapBlob VertexShader_Static;
    HeapBlob VertexShader_Skinned;
    HeapBlob FragmentShader;
    HeapBlob FragmentShader2;
    HeapBlob TessControlShader;
    HeapBlob TessEvalShader;
    HeapBlob GeometryShader;

private:
    void Clear()
    {
        VertexShader_Static.Reset();
        VertexShader_Skinned.Reset();
        FragmentShader.Reset();
        FragmentShader2.Reset();
        TessControlShader.Reset();
        TessEvalShader.Reset();
        GeometryShader.Reset();
    }

    bool CreateVertexShaders(ShaderCompiler::SourceList const& sources)
    {
        if (!ShaderCompiler::CreateSpirV_VertexShader(g_VertexAttribsStatic, sources, VertexShader_Static))
            return false;

        ShaderCompiler::SourceList _sources;
        _sources.Add("#define SKINNED_MESH\n");
        _sources.Add(sources);

        if (!ShaderCompiler::CreateSpirV_VertexShader(g_VertexAttribsSkinned, _sources, VertexShader_Skinned))
            return false;

        return true;
    }

    bool CreateTessShaders(ShaderCompiler::SourceList const& sources)
    {
        if (!ShaderCompiler::CreateSpirV(RenderCore::TESS_CONTROL_SHADER, sources, TessControlShader))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::TESS_EVALUATION_SHADER, sources, TessEvalShader))
            return false;

        return true;
    }

    void AddPredefines(ShaderCompiler::SourceList& sources)
    {
        if (r_MaterialDebugMode)
            sources.Add("#define DEBUG_RENDER_MODE\n");
        if (r_SSLR)
            sources.Add("#define WITH_SSLR\n");
        if (r_HBAO)
            sources.Add("#define WITH_SSAO\n");
    }

public:
    bool CreateDepth(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_DEPTH\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (properties.Tessellation)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        if (properties.AlphaMasking)
        {
            if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
                return false;
        }

        return true;
    }

    bool CreateDepthVelocity(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_DEPTH\n");
        sources.Add("#define DEPTH_WITH_VELOCITY_MAP\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (properties.Tessellation)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        return true;
    }

    bool CreateWireframe(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_WIREFRAME\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::GEOMETRY_SHADER, sources, GeometryShader))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        sources.InsertAt(0, "#define SKINNED_MESH\n");
        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader2))
            return false;

        if (properties.Tessellation)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        return true;
    }

    bool CreateNormals(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_NORMALS\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::GEOMETRY_SHADER, sources, GeometryShader))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        return true;
    }

    bool CreateLight(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        if (properties.Tessellation)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        return true;
    }

    bool CreateLightmap(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define USE_LIGHTMAP\n");
        sources.Add(properties.Code.CStr());

        if (!ShaderCompiler::CreateSpirV_VertexShader(g_VertexAttribsStaticLightmap, sources, VertexShader_Static))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        if (properties.Tessellation)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        return true;
    }

    bool CreateVertexLight(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define USE_VERTEX_LIGHT\n");
        sources.Add(properties.Code.CStr());

        if (!ShaderCompiler::CreateSpirV_VertexShader(g_VertexAttribsStaticVertexLight, sources, VertexShader_Static))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        if (properties.Tessellation)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        return true;
    }

    bool CreateShadowMap(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_SHADOWMAP\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::GEOMETRY_SHADER, sources, GeometryShader))
            return false;

        if (properties.TessellationShadowMap)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
        bVSM = true;
#endif
        if (properties.ShadowMasking || bVSM)
        {
            if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
                return false;
        }

        return true;
    }

    bool CreateOmniShadowMap(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_OMNI_SHADOWMAP\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (properties.TessellationShadowMap)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        return true;
    }

    bool CreateFeedback(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_FEEDBACK\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        return true;
    }

    bool CreateOutline(MaterialCommonProperties const& properties)
    {
        Clear();

        ShaderCompiler::SourceList sources;
        AddPredefines(sources);
        sources.Add("#define MATERIAL_PASS_OUTLINE\n");
        sources.Add(properties.Code.CStr());

        if (!CreateVertexShaders(sources))
            return false;

        if (properties.Tessellation)
        {
            if (!CreateTessShaders(sources))
                return false;
        }

        if (!ShaderCompiler::CreateSpirV(RenderCore::FRAGMENT_SHADER, sources, FragmentShader))
            return false;

        return true;
    }
};

UniqueRef<MaterialBinary> MaterialCode::Translate()
{
    UniqueRef<MaterialBinary> binary = MakeUnique<MaterialBinary>();

    binary->MaterialType = Type;
    binary->IsCastShadow = !NoCastShadow;
    binary->IsTranslucent = IsTranslucent;
    binary->RenderingPriority = RenderingPriority;
    binary->TextureCount = Samplers.Size();
    binary->UniformVectorCount = NumUniformVectors;
    binary->LightmapSlot = LightmapSlot;
    binary->DepthPassTextureCount = DepthPassTextureCount;
    binary->LightPassTextureCount = LightPassTextureCount;
    binary->WireframePassTextureCount = WireframePassTextureCount;
    binary->NormalsPassTextureCount = NormalsPassTextureCount;
    binary->ShadowMapPassTextureCount = ShadowMapPassTextureCount;

    MaterialCommonProperties properties;
    properties.Tessellation = TessellationMethod != TESSELLATION_DISABLED;
    properties.TessellationShadowMap = TessellationMethod != TESSELLATION_DISABLED && DisplacementAffectShadow;
    properties.AlphaMasking = HasAlphaMasking;
    properties.ShadowMasking = HasShadowMapMasking;
    properties.Code = LoadShader("material.glsl", CodeBlocks);

    if (Type == MATERIAL_TYPE_PBR || Type == MATERIAL_TYPE_BASELIGHT || Type == MATERIAL_TYPE_UNLIT)
    {
        {
            MaterialPassTranslator translator;
            if (!translator.CreateDepth(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::DepthPass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_GEQUAL;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.RenderTargets.EmplaceBack().ColorWriteMask = COLOR_WRITE_DISABLED;
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), DepthPassTextureCount));
            }

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::DepthPass_Skin;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_GEQUAL;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.RenderTargets.EmplaceBack().ColorWriteMask = COLOR_WRITE_DISABLED;
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), DepthPassTextureCount));
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateDepthVelocity(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::DepthVelocityPass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_GEQUAL;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton for motion blur
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), DepthPassTextureCount));
            }
            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::DepthVelocityPass_Skin;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_GEQUAL;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton for motion blur
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), DepthPassTextureCount));
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateLight(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::LightPass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthFunc = IsTranslucent ? CMPFUNC_GREATER : CMPFUNC_EQUAL;
                pass.DepthTest = DepthTest_EXPERIMENTAL;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
                if (IsTranslucent)
                    pass.RenderTargets.EmplaceBack().SetBlendingPreset(GetBlendingPreset(Blending));
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), LightPassTextureCount));

#if 1
                pass.Samplers.Resize(20);
                // lightmap is in last sample
                pass.Samplers[LightPassTextureCount] = g_MaterialSamplers.LightmapSampler;
                //if (UseVT) { // TODO
                //    pass.Samplers[6] = g_MaterialSamplers.VirtualTextureSampler;
                //    pass.Samplers[7] = g_MaterialSamplers.VirtualTextureIndirectionSampler;
                //}
                //if ( AllowSSLR ) {
                pass.Samplers[8] = g_MaterialSamplers.ReflectDepthSampler;
                pass.Samplers[9] = g_MaterialSamplers.ReflectSampler;
                //}
                pass.Samplers[10] = g_MaterialSamplers.IESSampler;
                pass.Samplers[11] = g_MaterialSamplers.LookupBRDFSampler;
                pass.Samplers[12] = g_MaterialSamplers.SSAOSampler;
                pass.Samplers[13] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[14] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[15] = pass.Samplers[16] = pass.Samplers[17] = pass.Samplers[18] = g_MaterialSamplers.ShadowDepthSamplerPCF;
                pass.Samplers[19] = g_MaterialSamplers.OmniShadowMapSampler;
#endif
            }
            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::LightPass_Skin;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthFunc = IsTranslucent ? CMPFUNC_GREATER : CMPFUNC_EQUAL;
                pass.DepthTest = DepthTest_EXPERIMENTAL;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
                if (IsTranslucent)
                    pass.RenderTargets.EmplaceBack().SetBlendingPreset(GetBlendingPreset(Blending));
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), LightPassTextureCount));

                // TODO
#if 1
                pass.Samplers.Resize(20);
                // lightmap is in last sample
                pass.Samplers[LightPassTextureCount] = g_MaterialSamplers.LightmapSampler;
                //if (UseVT) { // TODO
                //    pass.Samplers[6] = g_MaterialSamplers.VirtualTextureSampler;
                //    pass.Samplers[7] = g_MaterialSamplers.VirtualTextureIndirectionSampler;
                //}
                //if ( AllowSSLR ) {
                pass.Samplers[8] = g_MaterialSamplers.ReflectDepthSampler;
                pass.Samplers[9] = g_MaterialSamplers.ReflectSampler;
                //}
                pass.Samplers[10] = g_MaterialSamplers.IESSampler;
                pass.Samplers[11] = g_MaterialSamplers.LookupBRDFSampler;
                pass.Samplers[12] = g_MaterialSamplers.SSAOSampler;
                pass.Samplers[13] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[14] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[15] = pass.Samplers[16] = pass.Samplers[17] = pass.Samplers[18] = g_MaterialSamplers.ShadowDepthSamplerPCF;
                pass.Samplers[19] = g_MaterialSamplers.OmniShadowMapSampler;
#endif
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateShadowMap(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::ShadowMapPass;
#if defined SHADOWMAP_VSM
                //pass.CullMode = POLYGON_CULL_FRONT; // Less light bleeding
                pass.CullMode = POLYGON_CULL_DISABLED;
#else
                //pass.CullMode = POLYGON_CULL_BACK;
                //pass.CullMode = POLYGON_CULL_DISABLED; // Less light bleeding
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
#endif
                //pass.CullMode = POLYGON_CULL_DISABLED;
                pass.DepthFunc = CMPFUNC_LESS;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.TessellationShadowMap ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
#if defined SHADOWMAP_VSM
                pass.RenderTarget.EmplaceBack().SetBlendingPreset(BLENDING_NO_BLEND);
#endif
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), ShadowMapPassTextureCount));
            }
            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::ShadowMapPass_Skin;
#if defined SHADOWMAP_VSM
                //pass.CullMode = POLYGON_CULL_FRONT; // Less light bleeding
                pass.CullMode = POLYGON_CULL_DISABLED;
#else
                //pass.CullMode = POLYGON_CULL_BACK;
                //pass.CullMode = POLYGON_CULL_DISABLED; // Less light bleeding
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
#endif
                //pass.CullMode = POLYGON_CULL_DISABLED;
                pass.DepthFunc = CMPFUNC_LESS;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.TessellationShadowMap ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
#if defined SHADOWMAP_VSM
                pass.RenderTarget.EmplaceBack().SetBlendingPreset(BLENDING_NO_BLEND);
#endif
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), ShadowMapPassTextureCount));
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateOmniShadowMap(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::OmniShadowMapPass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_LESS;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.TessellationShadowMap ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow projection matrix
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), ShadowMapPassTextureCount));
            }
            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::OmniShadowMapPass_Skin;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_LESS;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.TessellationShadowMap ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow projection matrix
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), ShadowMapPassTextureCount));
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateFeedback(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::FeedbackPass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_GREATER;
                pass.DepthWrite = true;
                pass.DepthTest = true;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), LightPassTextureCount));
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
            }
            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::FeedbackPass_Skin;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthFunc = CMPFUNC_GREATER;
                pass.DepthWrite = true;
                pass.DepthTest = true;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), LightPassTextureCount));
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateOutline(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::OutlinePass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthTest = false;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), DepthPassTextureCount));
            }

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::OutlinePass_Skin;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthTest = false;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), DepthPassTextureCount));
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateWireframe(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t fragmentShaderSkin  = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader2));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::WireframePass;
                pass.RenderTargets.EmplaceBack().SetBlendingPreset(BLENDING_ALPHA);
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthTest = false;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), WireframePassTextureCount));
            }

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::WireframePass_Skin;
                pass.RenderTargets.EmplaceBack().SetBlendingPreset(BLENDING_ALPHA);
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthTest = false;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShaderSkin;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), WireframePassTextureCount));
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateNormals(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t vertexShaderSkinned = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Skinned));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::NormalsPass;
                pass.RenderTargets.EmplaceBack().SetBlendingPreset(BLENDING_ALPHA);
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthTest = false;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = PRIMITIVE_POINTS;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), NormalsPassTextureCount));
            }

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::NormalsPass_Skin;
                pass.RenderTargets.EmplaceBack().SetBlendingPreset(BLENDING_ALPHA);
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthTest = false;
                pass.VertFormat = MaterialBinary::VertexFormat::SkinnedMesh;
                pass.VertexShader = vertexShaderSkinned;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = PRIMITIVE_POINTS;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), NormalsPassTextureCount));
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateLightmap(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::LightmapPass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthFunc = IsTranslucent ? CMPFUNC_GREATER : CMPFUNC_EQUAL;
                pass.DepthTest = DepthTest_EXPERIMENTAL;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh_Lightmap;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
                if (IsTranslucent)
                    pass.RenderTargets.EmplaceBack().SetBlendingPreset(GetBlendingPreset(Blending));
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), LightPassTextureCount));

                // TODO
#if 1
                pass.Samplers.Resize(20);
                // lightmap is in last sample
                pass.Samplers[LightPassTextureCount] = g_MaterialSamplers.LightmapSampler;
                //if (UseVT) { // TODO
                //    pass.Samplers[6] = g_MaterialSamplers.VirtualTextureSampler;
                //    pass.Samplers[7] = g_MaterialSamplers.VirtualTextureIndirectionSampler;
                //}
                //if ( AllowSSLR ) {
                pass.Samplers[8] = g_MaterialSamplers.ReflectDepthSampler;
                pass.Samplers[9] = g_MaterialSamplers.ReflectSampler;
                //}
                pass.Samplers[10] = g_MaterialSamplers.IESSampler;
                pass.Samplers[11] = g_MaterialSamplers.LookupBRDFSampler;
                pass.Samplers[12] = g_MaterialSamplers.SSAOSampler;
                pass.Samplers[13] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[14] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[15] = pass.Samplers[16] = pass.Samplers[17] = pass.Samplers[18] = g_MaterialSamplers.ShadowDepthSamplerPCF;
                pass.Samplers[19] = g_MaterialSamplers.OmniShadowMapSampler;
#endif
            }
        }

        {
            MaterialPassTranslator translator;
            if (!translator.CreateVertexLight(properties))
                return {};

            uint32_t vertexShaderStatic  = binary->AddShader(VERTEX_SHADER, std::move(translator.VertexShader_Static));
            uint32_t fragmentShader      = binary->AddShader(FRAGMENT_SHADER, std::move(translator.FragmentShader));
            uint32_t tessControlShader   = binary->AddShader(TESS_CONTROL_SHADER, std::move(translator.TessControlShader));
            uint32_t tessEvalShader      = binary->AddShader(TESS_EVALUATION_SHADER, std::move(translator.TessEvalShader));
            uint32_t geometryShader      = binary->AddShader(GEOMETRY_SHADER, std::move(translator.GeometryShader));

            {
                MaterialBinary::MaterialPassData& pass = binary->Passes.EmplaceBack();

                pass.Type = MaterialPass::VertexLightPass;
                pass.CullMode = IsTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
                pass.DepthWrite = false;
                pass.DepthFunc = IsTranslucent ? CMPFUNC_GREATER : CMPFUNC_EQUAL;
                pass.DepthTest = DepthTest_EXPERIMENTAL;
                pass.VertFormat = MaterialBinary::VertexFormat::StaticMesh_VertexLight;
                pass.VertexShader = vertexShaderStatic;
                pass.TessControlShader = tessControlShader;
                pass.TessEvalShader = tessEvalShader;
                pass.GeometryShader = geometryShader;
                pass.FragmentShader = fragmentShader;
                pass.Topology = properties.Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
                // TODO: Specify only used buffers
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // view constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // drawcall constants
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // skeleton
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // shadow cascade
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // light buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // IBL buffer
                pass.BufferBindings.EmplaceBack(BUFFER_BIND_CONSTANT); // VT buffer
                if (IsTranslucent)
                    pass.RenderTargets.EmplaceBack().SetBlendingPreset(GetBlendingPreset(Blending));
                AddSamplers(pass.Samplers, ArrayView<TextureSampler>(Samplers.ToPtr(), LightPassTextureCount));

                // TODO
#if 1
                pass.Samplers.Resize(20);
                // lightmap is in last sample
                pass.Samplers[LightPassTextureCount] = g_MaterialSamplers.LightmapSampler;
                //if (UseVT) { // TODO
                //    pass.Samplers[6] = g_MaterialSamplers.VirtualTextureSampler;
                //    pass.Samplers[7] = g_MaterialSamplers.VirtualTextureIndirectionSampler;
                //}
                //if ( AllowSSLR ) {
                pass.Samplers[8] = g_MaterialSamplers.ReflectDepthSampler;
                pass.Samplers[9] = g_MaterialSamplers.ReflectSampler;
                //}
                pass.Samplers[10] = g_MaterialSamplers.IESSampler;
                pass.Samplers[11] = g_MaterialSamplers.LookupBRDFSampler;
                pass.Samplers[12] = g_MaterialSamplers.SSAOSampler;
                pass.Samplers[13] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[14] = g_MaterialSamplers.ClusterLookupSampler;
                pass.Samplers[15] = pass.Samplers[16] = pass.Samplers[17] = pass.Samplers[18] = g_MaterialSamplers.ShadowDepthSamplerPCF;
                pass.Samplers[19] = g_MaterialSamplers.OmniShadowMapSampler;
#endif
            }
        }
    }
    return binary;
}

HK_NAMESPACE_END
