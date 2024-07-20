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

#include "MaterialBinary.h"
#include "MaterialSamplers.h"
#include <Engine/Renderer/ShaderFactory.h>
#include <Engine/Renderer/VertexAttribs.h>
#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

uint32_t MaterialBinary::AddShader(RenderCore::SHADER_TYPE shaderType, HeapBlob blob)
{
    if (blob.IsEmpty())
        return ~0u;

    uint32_t index = Shaders.Size();
    Shaders.EmplaceBack(shaderType, std::move(blob));
    return index;
}

Ref<RenderCore::IPipeline> CreateMaterialPass(MaterialBinary::MaterialPassData const& pass, Vector<Ref<RenderCore::IShaderModule>> const& shaders)
{
    using namespace RenderCore;

    PipelineDesc desc;

    if (pass.VertexShader != -1)
        desc.pVS = shaders[pass.VertexShader];

    if (pass.FragmentShader != -1)
        desc.pFS = shaders[pass.FragmentShader];

    if (pass.TessControlShader != -1)
        desc.pTCS = shaders[pass.TessControlShader];

    if (pass.TessEvalShader != -1)
        desc.pTES = shaders[pass.TessEvalShader];

    if (pass.GeometryShader != -1)
        desc.pGS = shaders[pass.GeometryShader];

    desc.RS.CullMode = pass.CullMode;
    desc.DSS.DepthFunc = pass.DepthFunc;
    desc.DSS.bDepthWrite = pass.DepthWrite;
    desc.DSS.bDepthEnable = pass.DepthTest;
    desc.IA.Topology = pass.Topology;

    StaticVector<VertexBindingInfo, 2> vertexBinding;

    switch (pass.VertFormat)
    {
        case MaterialBinary::VertexFormat::StaticMesh:
        {
            auto& binding = vertexBinding.EmplaceBack();
            binding.InputSlot = 0;
            binding.Stride = sizeof(MeshVertex);
            binding.InputRate = INPUT_RATE_PER_VERTEX;

            desc.NumVertexAttribs = g_VertexAttribsStatic.Size();
            desc.pVertexAttribs = g_VertexAttribsStatic.ToPtr();

            break;
        }
        case MaterialBinary::VertexFormat::SkinnedMesh:
        {
            auto& binding0 = vertexBinding.EmplaceBack();
            binding0.InputSlot = 0;
            binding0.Stride = sizeof(MeshVertex);
            binding0.InputRate = INPUT_RATE_PER_VERTEX;

            auto& binding1 = vertexBinding.EmplaceBack();
            binding1.InputSlot = 1;
            binding1.Stride = sizeof(SkinVertex);
            binding1.InputRate = INPUT_RATE_PER_VERTEX;

            desc.NumVertexAttribs = g_VertexAttribsSkinned.Size();
            desc.pVertexAttribs = g_VertexAttribsSkinned.ToPtr();

            break;
        }
        case MaterialBinary::VertexFormat::StaticMesh_Lightmap:
        {
            auto& binding0 = vertexBinding.EmplaceBack();
            binding0.InputSlot = 0;
            binding0.Stride = sizeof(MeshVertex);
            binding0.InputRate = INPUT_RATE_PER_VERTEX;

            auto& binding1 = vertexBinding.EmplaceBack();
            binding1.InputSlot = 1;
            binding1.Stride = sizeof(MeshVertexUV);
            binding1.InputRate = INPUT_RATE_PER_VERTEX;

            desc.NumVertexAttribs = g_VertexAttribsStaticLightmap.Size();
            desc.pVertexAttribs = g_VertexAttribsStaticLightmap.ToPtr();

            break;
        }
        case MaterialBinary::VertexFormat::StaticMesh_VertexLight:
        {
            auto& binding0 = vertexBinding.EmplaceBack();
            binding0.InputSlot = 0;
            binding0.Stride = sizeof(MeshVertex);
            binding0.InputRate = INPUT_RATE_PER_VERTEX;

            auto& binding1 = vertexBinding.EmplaceBack();
            binding1.InputSlot = 1;
            binding1.Stride = sizeof(MeshVertexLight);
            binding1.InputRate = INPUT_RATE_PER_VERTEX;

            desc.NumVertexAttribs = g_VertexAttribsStaticVertexLight.Size();
            desc.pVertexAttribs = g_VertexAttribsStaticVertexLight.ToPtr();

            break;
        }
        default:
            return {};
    }

    desc.NumVertexBindings = vertexBinding.Size();
    desc.pVertexBindings = vertexBinding.ToPtr();

    int index = 0;
    for (auto& renderTarget : pass.RenderTargets)
        desc.BS.RenderTargetSlots[index++] = renderTarget;

    desc.ResourceLayout.NumBuffers = pass.BufferBindings.Size();
    desc.ResourceLayout.Buffers = pass.BufferBindings.ToPtr();

    desc.ResourceLayout.NumSamplers = pass.Samplers.Size();
    desc.ResourceLayout.Samplers = pass.Samplers.ToPtr();

    Ref<IPipeline> pipeline;
    GameApplication::GetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<RenderCore::IPipeline> CreateTerrainMaterialDepth()
{
    using namespace RenderCore;

    PipelineDesc desc;

    RasterizerStateInfo& rsd = desc.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;

    DepthStencilStateInfo& dssd = desc.DSS;
    dssd.DepthFunc = CMPFUNC_GEQUAL; //CMPFUNC_GREATER;

    BlendingStateInfo& bs = desc.BS;
    bs.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(TerrainVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(TerrainPatchInstance);
    vertexBinding[1].InputRate = INPUT_RATE_PER_INSTANCE;

    desc.NumVertexBindings = 2;
    desc.pVertexBindings = vertexBinding;

    desc.NumVertexAttribs = g_VertexAttribsTerrainInstanced.Size();
    desc.pVertexAttribs = g_VertexAttribsTerrainInstanced.ToPtr();

    ShaderFactory::CreateVertexShader("terrain_depth.vert", desc.pVertexAttribs, desc.NumVertexAttribs, desc.pVS);
    ShaderFactory::CreateFragmentShader("terrain_depth.frag", desc.pFS);

    PipelineInputAssemblyInfo& inputAssembly = desc.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;

    SamplerDesc clipmapSampler;
    clipmapSampler.Filter = FILTER_NEAREST;

    desc.ResourceLayout.NumSamplers = 1;
    desc.ResourceLayout.Samplers = &clipmapSampler;

    BufferInfo buffers[2];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    desc.ResourceLayout.NumBuffers = 2;
    desc.ResourceLayout.Buffers = buffers;

    Ref<IPipeline> pipeline;
    GameApplication::GetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<RenderCore::IPipeline> CreateTerrainMaterialLight()
{
    using namespace RenderCore;

    PipelineDesc desc;

    RasterizerStateInfo& rsd = desc.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;

    DepthStencilStateInfo& dssd = desc.DSS;
    dssd.bDepthWrite = false;
    dssd.DepthFunc = CMPFUNC_EQUAL;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(TerrainVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    // TODO: second buffer for instancing
    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(TerrainPatchInstance);
    vertexBinding[1].InputRate = INPUT_RATE_PER_INSTANCE;

    desc.NumVertexBindings = 2;
    desc.pVertexBindings = vertexBinding;

    desc.NumVertexAttribs = g_VertexAttribsTerrainInstanced.Size();
    desc.pVertexAttribs = g_VertexAttribsTerrainInstanced.ToPtr();

    ShaderFactory::CreateVertexShader("terrain_color.vert", desc.pVertexAttribs, desc.NumVertexAttribs, desc.pVS);
    ShaderFactory::CreateFragmentShader("terrain_color.frag", desc.pFS);

    PipelineInputAssemblyInfo& inputAssembly = desc.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;

    SamplerDesc samplers[20];

    SamplerDesc& clipmapSampler = samplers[0];
    clipmapSampler.Filter = FILTER_NEAREST;

    SamplerDesc& normalmapSampler = samplers[1];
    normalmapSampler.Filter = FILTER_LINEAR;

    // lightmap is in last sample
    //samplers[NumSamplers] = g_MaterialSamplers.LightmapSampler;

    //if (UseVT) { // TODO
    //    samplers[6] = g_MaterialSamplers.VirtualTextureSampler;
    //    samplers[7] = g_MaterialSamplers.VirtualTextureIndirectionSampler;
    //}

    //if ( AllowSSLR ) {
    samplers[8] = g_MaterialSamplers.ReflectDepthSampler;
    samplers[9] = g_MaterialSamplers.ReflectSampler;
    //}
    samplers[10] = g_MaterialSamplers.IESSampler;
    samplers[11] = g_MaterialSamplers.LookupBRDFSampler;
    samplers[12] = g_MaterialSamplers.SSAOSampler;
    samplers[13] = g_MaterialSamplers.ClusterLookupSampler;
    samplers[14] = g_MaterialSamplers.ClusterLookupSampler;
    samplers[15] =
        samplers[16] =
        samplers[17] =
        samplers[18] = g_MaterialSamplers.ShadowDepthSamplerPCF;
    samplers[19] = g_MaterialSamplers.OmniShadowMapSampler;

    desc.ResourceLayout.NumSamplers = HK_ARRAY_SIZE(samplers);
    desc.ResourceLayout.Samplers = samplers;

    BufferInfo buffers[7];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade
    buffers[4].BufferBinding = BUFFER_BIND_CONSTANT; // light buffer
    buffers[5].BufferBinding = BUFFER_BIND_CONSTANT; // IBL buffer
    buffers[6].BufferBinding = BUFFER_BIND_CONSTANT; // VT buffer

    desc.ResourceLayout.NumBuffers = 7;
    desc.ResourceLayout.Buffers = buffers;

    Ref<IPipeline> pipeline;
    GameApplication::GetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<RenderCore::IPipeline> CreateTerrainMaterialWireframe()
{
    using namespace RenderCore;

    PipelineDesc desc;

    RasterizerStateInfo& rsd = desc.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;

    DepthStencilStateInfo& dssd = desc.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

    BlendingStateInfo& bsd = desc.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset(BLENDING_ALPHA);

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(TerrainVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(TerrainPatchInstance);
    vertexBinding[1].InputRate = INPUT_RATE_PER_INSTANCE;

    desc.NumVertexBindings = 2;
    desc.pVertexBindings = vertexBinding;

    desc.NumVertexAttribs = g_VertexAttribsTerrainInstanced.Size();
    desc.pVertexAttribs = g_VertexAttribsTerrainInstanced.ToPtr();

    ShaderFactory::CreateVertexShader("terrain_wireframe.vert", desc.pVertexAttribs, desc.NumVertexAttribs, desc.pVS);
    ShaderFactory::CreateGeometryShader("terrain_wireframe.geom", desc.pGS);
    ShaderFactory::CreateFragmentShader("terrain_wireframe.frag", desc.pFS);

    PipelineInputAssemblyInfo& inputAssembly = desc.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;

    SamplerDesc clipmapSampler;
    clipmapSampler.Filter = FILTER_NEAREST;

    desc.ResourceLayout.NumSamplers = 1;
    desc.ResourceLayout.Samplers = &clipmapSampler;

    BufferInfo buffers[2];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    desc.ResourceLayout.NumBuffers = 2;
    desc.ResourceLayout.Buffers = buffers;

    Ref<IPipeline> pipeline;
    GameApplication::GetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<MaterialGPU> MaterialBinary::Compile()
{
    Vector<Ref<RenderCore::IShaderModule>> compiledShaders;

    compiledShaders.Reserve(Shaders.Size());
    for (Shader const& shader : Shaders)
    {
        compiledShaders.Add(ShaderFactory::CreateShaderSpirV(shader.m_Type, shader.m_Blob));
        if (!compiledShaders.Last())
            return {};
    }

    Ref<MaterialGPU> materialGPU = MakeRef<MaterialGPU>();
    materialGPU->MaterialType = MaterialType;
    materialGPU->LightmapSlot = LightmapSlot;
    materialGPU->DepthPassTextureCount = DepthPassTextureCount;
    materialGPU->LightPassTextureCount = LightPassTextureCount;
    materialGPU->WireframePassTextureCount = WireframePassTextureCount;
    materialGPU->NormalsPassTextureCount = NormalsPassTextureCount;
    materialGPU->ShadowMapPassTextureCount = ShadowMapPassTextureCount;

    for (MaterialPassData const& pass : Passes)
    {
        if (!(materialGPU->Passes[pass.Type] = CreateMaterialPass(pass, compiledShaders)))
            return {};
    }
    return materialGPU;
}

void MaterialBinary::Read(IBinaryStreamReadInterface& stream)
{
    MaterialType = MATERIAL_TYPE(stream.ReadUInt8());
    IsCastShadow = stream.ReadBool();
    IsTranslucent = stream.ReadBool();
    RenderingPriority = RENDERING_PRIORITY(stream.ReadUInt8());
    TextureCount = stream.ReadUInt8();
    UniformVectorCount = stream.ReadUInt8();
    LightmapSlot = stream.ReadUInt8();
    DepthPassTextureCount = stream.ReadUInt8();
    LightPassTextureCount = stream.ReadUInt8();
    WireframePassTextureCount = stream.ReadUInt8();
    NormalsPassTextureCount = stream.ReadUInt8();
    ShadowMapPassTextureCount= stream.ReadUInt8();
    stream.ReadArray(Shaders);
    stream.ReadArray(Passes);
}

void MaterialBinary::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt8(MaterialType);
    stream.WriteBool(IsCastShadow);
    stream.WriteBool(IsTranslucent);
    stream.WriteUInt8(RenderingPriority);
    stream.WriteUInt8(TextureCount);
    stream.WriteUInt8(UniformVectorCount);
    stream.WriteUInt8(LightmapSlot);
    stream.WriteUInt8(DepthPassTextureCount);
    stream.WriteUInt8(LightPassTextureCount);
    stream.WriteUInt8(WireframePassTextureCount);
    stream.WriteUInt8(NormalsPassTextureCount);
    stream.WriteUInt8(ShadowMapPassTextureCount);
    stream.WriteArray(Shaders);
    stream.WriteArray(Passes);
}

void MaterialBinary::Shader::Read(IBinaryStreamReadInterface& stream)
{
    m_Type = RenderCore::SHADER_TYPE(stream.ReadUInt8());
    uint32_t blobSize = stream.ReadUInt32();
    m_Blob = stream.ReadBlob(blobSize);
}

void MaterialBinary::Shader::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt8(m_Type);
    stream.WriteUInt32(m_Blob.Size());
    stream.WriteBlob(m_Blob);
}

void MaterialBinary::MaterialPassData::Read(IBinaryStreamReadInterface& stream)
{
    Type = MaterialPass::Type(stream.ReadUInt8());
    CullMode = RenderCore::POLYGON_CULL(stream.ReadUInt8());
    DepthFunc = RenderCore::COMPARISON_FUNCTION(stream.ReadUInt8());
    DepthWrite = stream.ReadBool();
    DepthTest = stream.ReadBool();
    Topology = RenderCore::PRIMITIVE_TOPOLOGY(stream.ReadUInt8());
    VertFormat = VertexFormat(stream.ReadUInt8());
    VertexShader = stream.ReadUInt32();
    FragmentShader = stream.ReadUInt32();
    TessControlShader = stream.ReadUInt32();
    TessEvalShader = stream.ReadUInt32();
    GeometryShader = stream.ReadUInt32();
    uint32_t numBufferBindings = stream.ReadUInt32();
    BufferBindings.Resize(numBufferBindings);
    for (uint32_t n = 0; n < numBufferBindings; ++n)
        BufferBindings[n].BufferBinding = RenderCore::BUFFER_BINDING(stream.ReadUInt8());
    uint32_t numRenderTargets = stream.ReadUInt32();
    RenderTargets.Resize(numRenderTargets);
    for (uint32_t n = 0; n < numRenderTargets; ++n)
    {
        RenderTargets[n].Op.ColorRGB = RenderCore::BLEND_OP(stream.ReadUInt8());
        RenderTargets[n].Op.Alpha = RenderCore::BLEND_OP(stream.ReadUInt8());
        RenderTargets[n].Func.SrcFactorRGB = RenderCore::BLEND_FUNC(stream.ReadUInt8());
        RenderTargets[n].Func.DstFactorRGB = RenderCore::BLEND_FUNC(stream.ReadUInt8());
        RenderTargets[n].Func.SrcFactorAlpha = RenderCore::BLEND_FUNC(stream.ReadUInt8());
        RenderTargets[n].Func.DstFactorAlpha = RenderCore::BLEND_FUNC(stream.ReadUInt8());
        RenderTargets[n].bBlendEnable = stream.ReadBool();
        RenderTargets[n].ColorWriteMask = RenderCore::COLOR_WRITE_MASK(stream.ReadUInt8());
    }
    uint32_t numSamplers = stream.ReadUInt32();
    Samplers.Resize(numSamplers);
    for (uint32_t n = 0; n < numSamplers; ++n)
    {
        Samplers[n].Filter = RenderCore::SAMPLER_FILTER(stream.ReadUInt8());
        Samplers[n].AddressU = RenderCore::SAMPLER_ADDRESS_MODE(stream.ReadUInt8());
        Samplers[n].AddressV = RenderCore::SAMPLER_ADDRESS_MODE(stream.ReadUInt8());
        Samplers[n].AddressW = RenderCore::SAMPLER_ADDRESS_MODE(stream.ReadUInt8());
        Samplers[n].MaxAnisotropy = stream.ReadUInt8();
        Samplers[n].ComparisonFunc = RenderCore::COMPARISON_FUNCTION(stream.ReadUInt8());
        Samplers[n].bCompareRefToTexture = stream.ReadBool();
        Samplers[n].bCubemapSeamless     = stream.ReadBool();
        Samplers[n].MipLODBias = stream.ReadFloat();
        Samplers[n].MinLOD = stream.ReadFloat();
        Samplers[n].MaxLOD = stream.ReadFloat();
        Samplers[n].BorderColor[0] = stream.ReadFloat();
        Samplers[n].BorderColor[1] = stream.ReadFloat();
        Samplers[n].BorderColor[2] = stream.ReadFloat();
        Samplers[n].BorderColor[3] = stream.ReadFloat();
    }
}

void MaterialBinary::MaterialPassData::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt8(Type);
    stream.WriteUInt8(CullMode);
    stream.WriteUInt8(DepthFunc);
    stream.WriteBool(DepthWrite);
    stream.WriteBool(DepthTest);
    stream.WriteUInt8(Topology);
    stream.WriteUInt8(uint8_t(VertFormat));
    stream.WriteUInt32(VertexShader);
    stream.WriteUInt32(FragmentShader);
    stream.WriteUInt32(TessControlShader);
    stream.WriteUInt32(TessEvalShader);
    stream.WriteUInt32(GeometryShader);
    stream.WriteUInt32(BufferBindings.Size());
    for (uint32_t n = 0; n < BufferBindings.Size(); ++n)
        stream.WriteUInt8(BufferBindings[n].BufferBinding);
    stream.WriteUInt32(RenderTargets.Size());
    for (uint32_t n = 0; n < RenderTargets.Size(); ++n)
    {
         stream.WriteUInt8(RenderTargets[n].Op.ColorRGB);
         stream.WriteUInt8(RenderTargets[n].Op.Alpha);
         stream.WriteUInt8(RenderTargets[n].Func.SrcFactorRGB);
         stream.WriteUInt8(RenderTargets[n].Func.DstFactorRGB);
         stream.WriteUInt8(RenderTargets[n].Func.SrcFactorAlpha);
         stream.WriteUInt8(RenderTargets[n].Func.DstFactorAlpha);
         stream.WriteBool(RenderTargets[n].bBlendEnable);
         stream.WriteUInt8(RenderTargets[n].ColorWriteMask);
    }
    stream.WriteUInt32(Samplers.Size());
    for (uint32_t n = 0; n < Samplers.Size(); ++n)
    {
         stream.WriteUInt8(Samplers[n].Filter);
         stream.WriteUInt8(Samplers[n].AddressU);
         stream.WriteUInt8(Samplers[n].AddressV);
         stream.WriteUInt8(Samplers[n].AddressW);
         stream.WriteUInt8(Samplers[n].MaxAnisotropy);
         stream.WriteUInt8(Samplers[n].ComparisonFunc);
         stream.WriteBool(Samplers[n].bCompareRefToTexture);
         stream.WriteBool(Samplers[n].bCubemapSeamless);
         stream.WriteFloat(Samplers[n].MipLODBias);
         stream.WriteFloat(Samplers[n].MinLOD);
         stream.WriteFloat(Samplers[n].MaxLOD);
         stream.WriteFloat(Samplers[n].BorderColor[0]);
         stream.WriteFloat(Samplers[n].BorderColor[1]);
         stream.WriteFloat(Samplers[n].BorderColor[2]);
         stream.WriteFloat(Samplers[n].BorderColor[3]);
    }
}

HK_NAMESPACE_END
