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

#include "MaterialCompiler.h"

#include <Hork/RenderDefs/VertexAttribs.h>
#include <Hork/MaterialGraph/MaterialSamplers.h>
#include <Hork/ShaderUtils/ShaderUtils.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

Ref<RHI::IPipeline> CreateMaterialPass(MaterialBinary::MaterialPassData const& pass, Vector<Ref<RHI::IShaderModule>> const& shaders)
{
    using namespace RHI;

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
    GameApplication::sGetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<RHI::IPipeline> CreateTerrainMaterialDepth(RHI::IDevice* device)
{
    using namespace RHI;

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

    ShaderUtils::CreateVertexShader(device, "terrain_depth.vert", desc.pVertexAttribs, desc.NumVertexAttribs, desc.pVS);
    ShaderUtils::CreateFragmentShader(device, "terrain_depth.frag", desc.pFS);

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
    GameApplication::sGetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<RHI::IPipeline> CreateTerrainMaterialLight(RHI::IDevice* device)
{
    using namespace RHI;

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

    ShaderUtils::CreateVertexShader(device, "terrain_color.vert", desc.pVertexAttribs, desc.NumVertexAttribs, desc.pVS);
    ShaderUtils::CreateFragmentShader(device, "terrain_color.frag", desc.pFS);

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
    GameApplication::sGetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<RHI::IPipeline> CreateTerrainMaterialWireframe(RHI::IDevice* device)
{
    using namespace RHI;

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

    ShaderUtils::CreateVertexShader(device, "terrain_wireframe.vert", desc.pVertexAttribs, desc.NumVertexAttribs, desc.pVS);
    ShaderUtils::CreateGeometryShader(device, "terrain_wireframe.geom", desc.pGS);
    ShaderUtils::CreateFragmentShader(device, "terrain_wireframe.frag", desc.pFS);

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
    GameApplication::sGetRenderDevice()->CreatePipeline(desc, &pipeline);
    return pipeline;
}

Ref<MaterialGPU> CompileMaterial(MaterialBinary const& binary)
{
    Vector<Ref<RHI::IShaderModule>> compiledShaders;
    RHI::IDevice* device = GameApplication::sGetRenderDevice();

    compiledShaders.Reserve(binary.Shaders.Size());
    for (MaterialBinary::Shader const& shader : binary.Shaders)
    {
        compiledShaders.Add(ShaderUtils::CreateShaderSpirV(device, shader.m_Type, shader.m_Blob));
        if (!compiledShaders.Last())
            return {};
    }

    Ref<MaterialGPU> materialGPU = MakeRef<MaterialGPU>();
    materialGPU->MaterialType = binary.MaterialType;
    materialGPU->LightmapSlot = binary.LightmapSlot;
    materialGPU->DepthPassTextureCount = binary.DepthPassTextureCount;
    materialGPU->LightPassTextureCount = binary.LightPassTextureCount;
    materialGPU->WireframePassTextureCount = binary.WireframePassTextureCount;
    materialGPU->NormalsPassTextureCount = binary.NormalsPassTextureCount;
    materialGPU->ShadowMapPassTextureCount = binary.ShadowMapPassTextureCount;

    for (MaterialBinary::MaterialPassData const& pass : binary.Passes)
    {
        if (!(materialGPU->Passes[pass.Type] = CreateMaterialPass(pass, compiledShaders)))
            return {};
    }
    return materialGPU;
}

HK_NAMESPACE_END
