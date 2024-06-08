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

#include "GpuMaterial.h"
#include "RenderLocal.h"
#include "GpuMaterial.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

static constexpr SAMPLER_FILTER SamplerFilterLUT[] = {
    FILTER_LINEAR,
    FILTER_NEAREST,
    FILTER_MIPMAP_NEAREST,
    FILTER_MIPMAP_BILINEAR,
    FILTER_MIPMAP_NLINEAR,
    FILTER_MIPMAP_TRILINEAR};

static constexpr SAMPLER_ADDRESS_MODE SamplerAddressLUT[] = {
    SAMPLER_ADDRESS_WRAP,
    SAMPLER_ADDRESS_MIRROR,
    SAMPLER_ADDRESS_CLAMP,
    SAMPLER_ADDRESS_BORDER,
    SAMPLER_ADDRESS_MIRROR_ONCE};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;

static SamplerDesc LightmapSampler;
static SamplerDesc ReflectSampler;
static SamplerDesc ReflectDepthSampler;
static SamplerDesc VirtualTextureSampler;
static SamplerDesc VirtualTextureIndirectionSampler;
static SamplerDesc ShadowDepthSamplerPCF;
static SamplerDesc ShadowDepthSamplerVSM;
static SamplerDesc ShadowDepthSamplerEVSM;
static SamplerDesc ShadowDepthSamplerPCSS0;
static SamplerDesc ShadowDepthSamplerPCSS1;
static SamplerDesc OmniShadowMapSampler;
static SamplerDesc IESSampler;
static SamplerDesc ClusterLookupSampler;
static SamplerDesc SSAOSampler;
static SamplerDesc LookupBRDFSampler;

struct InitMaterialSamplers
{
    InitMaterialSamplers()
    {
        LightmapSampler.Filter = FILTER_LINEAR;
        LightmapSampler.AddressU = SAMPLER_ADDRESS_WRAP;
        LightmapSampler.AddressV = SAMPLER_ADDRESS_WRAP;
        LightmapSampler.AddressW = SAMPLER_ADDRESS_WRAP;

        ReflectSampler.Filter = FILTER_MIPMAP_BILINEAR;
        ReflectSampler.AddressU = SAMPLER_ADDRESS_BORDER;
        ReflectSampler.AddressV = SAMPLER_ADDRESS_BORDER;
        ReflectSampler.AddressW = SAMPLER_ADDRESS_BORDER;

        ReflectDepthSampler.Filter = FILTER_NEAREST;
        ReflectDepthSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        ReflectDepthSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        ReflectDepthSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

        VirtualTextureSampler.Filter = FILTER_LINEAR;
        VirtualTextureSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        VirtualTextureSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        VirtualTextureSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

        VirtualTextureIndirectionSampler.Filter = FILTER_MIPMAP_NEAREST;
        VirtualTextureIndirectionSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        VirtualTextureIndirectionSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        VirtualTextureIndirectionSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

        ShadowDepthSamplerPCF.Filter = FILTER_LINEAR;
        ShadowDepthSamplerPCF.AddressU = SAMPLER_ADDRESS_MIRROR; //SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCF.AddressV = SAMPLER_ADDRESS_MIRROR; //SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCF.AddressW = SAMPLER_ADDRESS_MIRROR; //SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCF.MipLODBias = 0;
        //ShadowDepthSamplerPCF.ComparisonFunc = CMPFUNC_LEQUAL;
        ShadowDepthSamplerPCF.ComparisonFunc = CMPFUNC_LESS;
        ShadowDepthSamplerPCF.bCompareRefToTexture = true;

        ShadowDepthSamplerVSM.Filter = FILTER_LINEAR;
        ShadowDepthSamplerVSM.AddressU = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerVSM.AddressV = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerVSM.AddressW = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerVSM.MipLODBias = 0;
        ShadowDepthSamplerVSM.BorderColor[0] = VSM_ClearValue.X;
        ShadowDepthSamplerVSM.BorderColor[1] = VSM_ClearValue.Y;
        ShadowDepthSamplerVSM.BorderColor[2] = VSM_ClearValue.Z;
        ShadowDepthSamplerVSM.BorderColor[3] = VSM_ClearValue.W;

        ShadowDepthSamplerEVSM.Filter = FILTER_LINEAR;
        ShadowDepthSamplerEVSM.AddressU = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerEVSM.AddressV = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerEVSM.AddressW = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerEVSM.MipLODBias = 0;
        ShadowDepthSamplerEVSM.BorderColor[0] = EVSM_ClearValue.X;
        ShadowDepthSamplerEVSM.BorderColor[1] = EVSM_ClearValue.Y;
        ShadowDepthSamplerEVSM.BorderColor[2] = EVSM_ClearValue.Z;
        ShadowDepthSamplerEVSM.BorderColor[3] = EVSM_ClearValue.W;

        ShadowDepthSamplerPCSS0.AddressU = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCSS0.AddressV = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCSS0.AddressW = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCSS0.MipLODBias = 0;
        //ShadowDepthSamplerPCSS0.BorderColor[0] = ShadowDepthSamplerPCSS0.BorderColor[1] = ShadowDepthSamplerPCSS0.BorderColor[2] = ShadowDepthSamplerPCSS0.BorderColor[3] = 1.0f;
        // Find blocker point sampler
        ShadowDepthSamplerPCSS0.Filter = FILTER_NEAREST; //FILTER_LINEAR;
        //ShadowDepthSamplerPCSS0.ComparisonFunc = CMPFUNC_GREATER;//CMPFUNC_GEQUAL;
        //ShadowDepthSamplerPCSS0.CompareRefToTexture = true;

        // PCF_Sampler
        ShadowDepthSamplerPCSS1.AddressU = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCSS1.AddressV = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCSS1.AddressW = SAMPLER_ADDRESS_BORDER;
        ShadowDepthSamplerPCSS1.MipLODBias = 0;
        ShadowDepthSamplerPCSS1.Filter = FILTER_LINEAR; //GHI_Filter_Min_LinearMipmapLinear_Mag_Linear; // D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR  Is the same?
        ShadowDepthSamplerPCSS1.ComparisonFunc = CMPFUNC_LESS;
        ShadowDepthSamplerPCSS1.bCompareRefToTexture = true;
        ShadowDepthSamplerPCSS1.BorderColor[0] = ShadowDepthSamplerPCSS1.BorderColor[1] = ShadowDepthSamplerPCSS1.BorderColor[2] = ShadowDepthSamplerPCSS1.BorderColor[3] = 1.0f; // FIXME?

        OmniShadowMapSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        OmniShadowMapSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        OmniShadowMapSampler.AddressW = SAMPLER_ADDRESS_CLAMP;
        OmniShadowMapSampler.Filter = FILTER_LINEAR;

        IESSampler.Filter = FILTER_LINEAR;
        IESSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        IESSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        IESSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

        ClusterLookupSampler.Filter = FILTER_NEAREST;
        ClusterLookupSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        ClusterLookupSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        ClusterLookupSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

        SSAOSampler.Filter = FILTER_NEAREST;
        SSAOSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        SSAOSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        SSAOSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

        LookupBRDFSampler.Filter = FILTER_LINEAR;
        LookupBRDFSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
        LookupBRDFSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
        LookupBRDFSampler.AddressW = SAMPLER_ADDRESS_CLAMP;
    }
};

static InitMaterialSamplers InitSamplers;

static void CopyMaterialSamplers(SamplerDesc* Dest, TextureSampler const* Samplers, int NumSamplers)
{
    SamplerDesc samplerCI;
    samplerCI.Filter = FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressV = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressW = SAMPLER_ADDRESS_WRAP;
    samplerCI.MipLODBias = 0;
    samplerCI.MaxAnisotropy = 0;
    samplerCI.ComparisonFunc = CMPFUNC_LEQUAL;
    samplerCI.bCompareRefToTexture = false;
    samplerCI.BorderColor[0] = 0;
    samplerCI.BorderColor[1] = 0;
    samplerCI.BorderColor[2] = 0;
    samplerCI.BorderColor[3] = 0;
    samplerCI.MinLOD = -1000;
    samplerCI.MaxLOD = 1000;
    for (int i = 0; i < NumSamplers; i++)
    {
        TextureSampler const* desc = &Samplers[i];
        samplerCI.Filter = SamplerFilterLUT[desc->Filter];
        samplerCI.AddressU = SamplerAddressLUT[desc->AddressU];
        samplerCI.AddressV = SamplerAddressLUT[desc->AddressV];
        samplerCI.AddressW = SamplerAddressLUT[desc->AddressW];
        samplerCI.MipLODBias = desc->MipLODBias;
        samplerCI.MaxAnisotropy = desc->Anisotropy;
        samplerCI.MinLOD = desc->MinLod;
        samplerCI.MaxLOD = desc->MaxLod;
        samplerCI.bCubemapSeamless = true; // FIXME: use desc->bCubemapSeamless ?
        *Dest++ = samplerCI;
    }
}

static const VertexAttribInfo VertexAttribsSkinned[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)},
    {"InJointIndices",
     5, // location
     1, // buffer input slot
     VAT_UBYTE4,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertexSkin, JointIndices)},
    {"InJointWeights",
     6, // location
     1, // buffer input slot
     VAT_UBYTE4N,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertexSkin, JointWeights)}};

static const VertexAttribInfo VertexAttribsStatic[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)}};

static const VertexAttribInfo VertexAttribsStaticLightmap[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)},
    {"InLightmapTexCoord",
     5, // location
     1, // buffer input slot
     VAT_FLOAT2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertexUV, TexCoord)}};

static const VertexAttribInfo VertexAttribsStaticVertexLight[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)},
    {"InVertexLight",
     5,          // location
     1,          // buffer input slot
     VAT_UBYTE4, //VAT_UBYTE4N,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertexLight, VertexLight)}};

static const VertexAttribInfo VertexAttribsTerrain[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_SHORT2,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(TerrainVertex, X)}};

static const VertexAttribInfo VertexAttribsTerrainInstanced[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_SHORT2,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(TerrainVertex, X)},
    {"VertexScaleAndTranslate",
     1, // location
     1, // buffer input slot
     VAT_INT4,
     VAM_INTEGER,
     1, // InstanceDataStepRate
     HK_OFS(TerrainPatchInstance, VertexScale)},
    {"TexcoordOffset",
     2, // location
     1, // buffer input slot
     VAT_INT2,
     VAM_INTEGER,
     1, // InstanceDataStepRate
     HK_OFS(TerrainPatchInstance, TexcoordOffset)},
    {"QuadColor",
     3, // location
     1, // buffer input slot
     VAT_FLOAT4,
     VAM_FLOAT,
     1, // InstanceDataStepRate
     HK_OFS(TerrainPatchInstance, QuadColor)}};

void CreateDepthPassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, bool _AlphaMasking, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GEQUAL; //CMPFUNC_GREATER;

    BlendingStateInfo& bs = pipelineCI.BS;
    bs.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;
    }

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_DEPTH\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_DEPTH\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_DEPTH\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    if (_AlphaMasking)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_DEPTH\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);
    }

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[3];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton

    pipelineCI.ResourceLayout.NumBuffers = _Skinned ? 3 : 2;
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateDepthVelocityPassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GEQUAL; //CMPFUNC_GREATER;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;
    }

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_DEPTH\n");
    sources.Add("#define DEPTH_WITH_VELOCITY_MAP\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_DEPTH\n");
        sources.Add("#define DEPTH_WITH_VELOCITY_MAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_DEPTH\n");
        sources.Add("#define DEPTH_WITH_VELOCITY_MAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_DEPTH\n");
    sources.Add("#define DEPTH_WITH_VELOCITY_MAP\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[8];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade
    buffers[4].BufferBinding = BUFFER_BIND_CONSTANT; // light buffer
    buffers[5].BufferBinding = BUFFER_BIND_CONSTANT; // IBL buffer
    buffers[6].BufferBinding = BUFFER_BIND_CONSTANT; // VT buffer
    buffers[7].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton for motion blur

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateWireframePassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    BlendingStateInfo& bsd = pipelineCI.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset(BLENDING_ALPHA);

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;
    }

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_WIREFRAME\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_WIREFRAME\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(GEOMETRY_SHADER, sources, pipelineCI.pGS);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_WIREFRAME\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_WIREFRAME\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_WIREFRAME\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    BufferInfo buffers[3];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton

    pipelineCI.ResourceLayout.NumBuffers = _Skinned ? 3 : 2;
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateNormalsPassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, bool _Skinned, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    BlendingStateInfo& bsd = pipelineCI.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset(BLENDING_ALPHA);

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;
    }

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_NORMALS\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_NORMALS\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(GEOMETRY_SHADER, sources, pipelineCI.pGS);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_NORMALS\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_POINTS;

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    BufferInfo buffers[3];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton

    pipelineCI.ResourceLayout.NumBuffers = _Skinned ? 3 : 2;
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

static RenderCore::BLENDING_PRESET GetBlendingPreset(BLENDING_MODE _Blending)
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

void CreateLightPassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _DepthTest, bool _Translucent, BLENDING_MODE _Blending, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    BlendingStateInfo& bsd = pipelineCI.BS;
    if (_Translucent)
    {
        bsd.RenderTargetSlots[0].SetBlendingPreset(GetBlendingPreset(_Blending));
    }

    DepthStencilStateInfo& dssd = pipelineCI.DSS;

    dssd.bDepthWrite = false;
    if (_Translucent)
    {
        dssd.DepthFunc = CMPFUNC_GREATER;
    }
    else
    {
        dssd.DepthFunc = CMPFUNC_EQUAL;
    }

    dssd.bDepthEnable = _DepthTest;

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;

        String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define SKINNED_MESH\n");
        sources.Add(vertexAttribsShaderString.CStr());
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define SKINNED_MESH\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

        pipelineCI.NumVertexBindings = 2;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;

        String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add(vertexAttribsShaderString.CStr());
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

        pipelineCI.NumVertexBindings = 1;
    }

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    pipelineCI.pVertexBindings = vertexBinding;

    SamplerDesc samplers[20];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);
    // lightmap is in last sample
    samplers[NumSamplers] = LightmapSampler;

    //if ( bUseVT ) { // TODO
    //    samplers[6] = VirtualTextureSampler;
    //    samplers[7] = VirtualTextureIndirectionSampler;
    //}

    //if ( AllowSSLR ) {
    samplers[8] = ReflectDepthSampler;
    samplers[9] = ReflectSampler;
    //}
    samplers[10] = IESSampler;
    samplers[11] = LookupBRDFSampler;
    samplers[12] = SSAOSampler;
    samplers[13] = ClusterLookupSampler;
    samplers[14] = ClusterLookupSampler;
    samplers[15] =
        samplers[16] =
            samplers[17] =
                samplers[18] = ShadowDepthSamplerPCF;
    samplers[19] = OmniShadowMapSampler;

    pipelineCI.ResourceLayout.NumSamplers = HK_ARRAY_SIZE(samplers);
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[7];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade
    buffers[4].BufferBinding = BUFFER_BIND_CONSTANT; // light buffer
    buffers[5].BufferBinding = BUFFER_BIND_CONSTANT; // IBL buffer
    buffers[6].BufferBinding = BUFFER_BIND_CONSTANT; // VT buffer

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateLightPassLightmapPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _DepthTest, bool _Translucent, BLENDING_MODE _Blending, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    BlendingStateInfo& bsd = pipelineCI.BS;
    if (_Translucent)
    {
        bsd.RenderTargetSlots[0].SetBlendingPreset(GetBlendingPreset(_Blending));
    }

    DepthStencilStateInfo& dssd = pipelineCI.DSS;

    // Depth pre-pass
    dssd.bDepthWrite = false;
    if (_Translucent)
    {
        dssd.DepthFunc = CMPFUNC_GREATER;
    }
    else
    {
        dssd.DepthFunc = CMPFUNC_EQUAL;
    }

    dssd.bDepthEnable = _DepthTest;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStaticLightmap);
    pipelineCI.pVertexAttribs = VertexAttribsStaticLightmap;

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_COLOR\n");
    sources.Add("#define USE_LIGHTMAP\n");
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_COLOR\n");
    sources.Add("#define USE_LIGHTMAP\n");
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define USE_LIGHTMAP\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define USE_LIGHTMAP\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexUV);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE(vertexBinding);
    pipelineCI.pVertexBindings = vertexBinding;

    SamplerDesc samplers[20];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);
    // lightmap is in last sample
    samplers[NumSamplers] = LightmapSampler;

    //if ( bUseVT ) { // TODO
    //    samplers[6] = VirtualTextureSampler;
    //    samplers[7] = VirtualTextureIndirectionSampler;
    //}

    //if ( AllowSSLR ) {
    samplers[8] = ReflectDepthSampler;
    samplers[9] = ReflectSampler;
    //}
    samplers[10] = IESSampler;
    samplers[11] = LookupBRDFSampler;
    samplers[12] = SSAOSampler;
    samplers[13] = ClusterLookupSampler;
    samplers[14] = ClusterLookupSampler;
    samplers[15] =
        samplers[16] =
            samplers[17] =
                samplers[18] = ShadowDepthSamplerPCF;
    samplers[19] = OmniShadowMapSampler;

    pipelineCI.ResourceLayout.NumSamplers = HK_ARRAY_SIZE(samplers);
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[7];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade
    buffers[4].BufferBinding = BUFFER_BIND_CONSTANT; // light buffer
    buffers[5].BufferBinding = BUFFER_BIND_CONSTANT; // IBL buffer
    buffers[6].BufferBinding = BUFFER_BIND_CONSTANT; // VT buffer

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateLightPassVertexLightPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _DepthTest, bool _Translucent, BLENDING_MODE _Blending, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    BlendingStateInfo& bsd = pipelineCI.BS;
    if (_Translucent)
    {
        bsd.RenderTargetSlots[0].SetBlendingPreset(GetBlendingPreset(_Blending));
    }

    DepthStencilStateInfo& dssd = pipelineCI.DSS;

    // Depth pre-pass
    dssd.bDepthWrite = false;
    if (_Translucent)
    {
        dssd.DepthFunc = CMPFUNC_GREATER;
    }
    else
    {
        dssd.DepthFunc = CMPFUNC_EQUAL;
    }

    dssd.bDepthEnable = _DepthTest;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStaticVertexLight);
    pipelineCI.pVertexAttribs = VertexAttribsStaticVertexLight;

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_COLOR\n");
    sources.Add("#define USE_VERTEX_LIGHT\n");
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_COLOR\n");
    sources.Add("#define USE_VERTEX_LIGHT\n");
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define USE_VERTEX_LIGHT\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_COLOR\n");
        sources.Add("#define USE_VERTEX_LIGHT\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexLight);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE(vertexBinding);
    pipelineCI.pVertexBindings = vertexBinding;

    SamplerDesc samplers[20];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);
    // lightmap is in last sample
    samplers[NumSamplers] = LightmapSampler;

    //if ( bUseVT ) { // TODO
    //    samplers[6] = VirtualTextureSampler;
    //    samplers[7] = VirtualTextureIndirectionSampler;
    //}

    //if ( AllowSSLR ) {
    samplers[8] = ReflectDepthSampler;
    samplers[9] = ReflectSampler;
    //}
    samplers[10] = IESSampler;
    samplers[11] = LookupBRDFSampler;
    samplers[12] = SSAOSampler;
    samplers[13] = ClusterLookupSampler;
    samplers[14] = ClusterLookupSampler;
    samplers[15] =
        samplers[16] =
            samplers[17] =
                samplers[18] = ShadowDepthSamplerPCF;
    samplers[19] = OmniShadowMapSampler;

    pipelineCI.ResourceLayout.NumSamplers = HK_ARRAY_SIZE(samplers);
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[7];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade
    buffers[4].BufferBinding = BUFFER_BIND_CONSTANT; // light buffer
    buffers[5].BufferBinding = BUFFER_BIND_CONSTANT; // IBL buffer
    buffers[6].BufferBinding = BUFFER_BIND_CONSTANT; // VT buffer

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateShadowMapPassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, bool _ShadowMasking, bool _TwoSided, bool _Skinned, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
#if defined SHADOWMAP_VSM
    //Desc.CullMode = POLYGON_CULL_FRONT; // Less light bleeding
    Desc.CullMode = POLYGON_CULL_DISABLED;
#else
    //rsd.CullMode = POLYGON_CULL_BACK;
    //rsd.CullMode = POLYGON_CULL_DISABLED; // Less light bleeding
    rsd.CullMode = _TwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
#endif
    //rsd.CullMode = POLYGON_CULL_DISABLED;

    //BlendingStateInfo & bsd = pipelineCI.BS;
    //bsd.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;  // FIXME: there is no fragment shader, so we realy need to disable color mask?
#if defined SHADOWMAP_VSM
    bsd.RenderTargetSlots[0].SetBlendingPreset(BLENDING_NO_BLEND);
#endif

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_LESS;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;
    }

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_SHADOWMAP\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_SHADOWMAP\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(GEOMETRY_SHADER, sources, pipelineCI.pGS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_SHADOWMAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_SHADOWMAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
    bVSM = true;
#endif

    if (_ShadowMasking || bVSM)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_SHADOWMAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);
    }

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[4];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateOmniShadowMapPassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, bool _ShadowMasking, bool _TwoSided, bool _Skinned, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _TwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
    //rsd.CullMode = POLYGON_CULL_DISABLED;

    //BlendingStateInfo & bsd = pipelineCI.BS;
    //bsd.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;  // FIXME: there is no fragment shader, so we realy need to disable color mask?

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GREATER;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;
    }

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_OMNI_SHADOWMAP\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_OMNI_SHADOWMAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_OMNI_SHADOWMAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
    bVSM = true;
#endif

    if (_ShadowMasking || bVSM)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_OMNI_SHADOWMAP\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);
    }

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[4];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow projection matrix

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}


void CreateFeedbackPassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthWrite = true;
    dssd.DepthFunc = CMPFUNC_GREATER;
    dssd.bDepthEnable = true;

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;

        String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_FEEDBACK\n");
        sources.Add("#define SKINNED_MESH\n");
        sources.Add(vertexAttribsShaderString.CStr());
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_FEEDBACK\n");
        sources.Add("#define SKINNED_MESH\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

        pipelineCI.NumVertexBindings = 2;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;

        String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_FEEDBACK\n");
        sources.Add(vertexAttribsShaderString.CStr());
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_FEEDBACK\n");
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

        pipelineCI.NumVertexBindings = 1;
    }

    pipelineCI.pVertexBindings = vertexBinding;

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[7];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade
    buffers[4].BufferBinding = BUFFER_BIND_CONSTANT; // light buffer
    buffers[5].BufferBinding = BUFFER_BIND_CONSTANT; // IBL buffer
    buffers[6].BufferBinding = BUFFER_BIND_CONSTANT; // VT buffer

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

void CreateOutlinePassPipeline(TRef<RenderCore::IPipeline>* ppPipeline, const char* _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation, TextureSampler const* Samplers, int NumSamplers)
{
    PipelineDesc pipelineCI;
    ShaderFactory::SourceList sources;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
#if 0
    dssd.DepthFunc = CMPFUNC_GEQUAL;//CMPFUNC_GREATER;
#else
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;
#endif

    //BlendingStateInfo & bs = pipelineCI.BS;
    //bs.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(MeshVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(MeshVertexSkin);
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if (_Skinned)
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsSkinned);
        pipelineCI.pVertexAttribs = VertexAttribsSkinned;
    }
    else
    {
        pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsStatic);
        pipelineCI.pVertexAttribs = VertexAttribsStatic;
    }

    String vertexAttribsShaderString = ShaderStringForVertexAttribs<String>(pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs);

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_OUTLINE\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(vertexAttribsShaderString.CStr());
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(VERTEX_SHADER, sources, pipelineCI.pVS);

    if (_Tessellation)
    {
        sources.Clear();
        sources.Add("#define MATERIAL_PASS_OUTLINE\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_CONTROL_SHADER, sources, pipelineCI.pTCS);

        sources.Clear();
        sources.Add("#define MATERIAL_PASS_OUTLINE\n");
        if (_Skinned)
        {
            sources.Add("#define SKINNED_MESH\n");
        }
        sources.Add(_SourceCode);
        ShaderFactory::CreateShader(TESS_EVALUATION_SHADER, sources, pipelineCI.pTES);
    }

    sources.Clear();
    sources.Add("#define MATERIAL_PASS_OUTLINE\n");
    if (_Skinned)
    {
        sources.Add("#define SKINNED_MESH\n");
    }
    sources.Add(_SourceCode);
    ShaderFactory::CreateShader(FRAGMENT_SHADER, sources, pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;

    SamplerDesc samplers[MAX_SAMPLER_SLOTS];

    CopyMaterialSamplers(&samplers[0], Samplers, NumSamplers);

    pipelineCI.ResourceLayout.NumSamplers = NumSamplers;
    pipelineCI.ResourceLayout.Samplers = samplers;

    // TODO: Specify only used buffers
    BufferInfo buffers[3];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton

    pipelineCI.ResourceLayout.NumBuffers = _Skinned ? 3 : 2;
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, ppPipeline);
}

TRef<RenderCore::IPipeline> CreateTerrainMaterialDepth()
{
    TRef<RenderCore::IPipeline> pipeline;

    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GEQUAL; //CMPFUNC_GREATER;

    BlendingStateInfo& bs = pipelineCI.BS;
    bs.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(TerrainVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(TerrainPatchInstance);
    vertexBinding[1].InputRate = INPUT_RATE_PER_INSTANCE;

    pipelineCI.NumVertexBindings = 2;
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsTerrainInstanced);
    pipelineCI.pVertexAttribs = VertexAttribsTerrainInstanced;

    ShaderFactory::CreateVertexShader("terrain_depth.vert", pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs, pipelineCI.pVS);
    ShaderFactory::CreateFragmentShader("terrain_depth.frag", pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;

    SamplerDesc clipmapSampler;
    clipmapSampler.Filter = FILTER_NEAREST;

    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.Samplers = &clipmapSampler;

    BufferInfo buffers[2];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    pipelineCI.ResourceLayout.NumBuffers = 2;
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, &pipeline);
    return pipeline;
}

TRef<RenderCore::IPipeline> CreateTerrainMaterialLight()
{
    TRef<RenderCore::IPipeline> pipeline;

    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
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

    pipelineCI.NumVertexBindings = 2;
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsTerrainInstanced);
    pipelineCI.pVertexAttribs = VertexAttribsTerrainInstanced;

    ShaderFactory::CreateVertexShader("terrain_color.vert", pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs, pipelineCI.pVS);
    ShaderFactory::CreateFragmentShader("terrain_color.frag", pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;

    SamplerDesc samplers[20];

    SamplerDesc& clipmapSampler = samplers[0];
    clipmapSampler.Filter = FILTER_NEAREST;

    SamplerDesc& normalmapSampler = samplers[1];
    normalmapSampler.Filter = FILTER_LINEAR;

    // lightmap is in last sample
    //samplers[NumSamplers] = LightmapSampler;

    //if ( bUseVT ) { // TODO
    //    samplers[6] = VirtualTextureSampler;
    //    samplers[7] = VirtualTextureIndirectionSampler;
    //}

    //if ( AllowSSLR ) {
    samplers[8] = ReflectDepthSampler;
    samplers[9] = ReflectSampler;
    //}
    samplers[10] = IESSampler;
    samplers[11] = LookupBRDFSampler;
    samplers[12] = SSAOSampler;
    samplers[13] = ClusterLookupSampler;
    samplers[14] = ClusterLookupSampler;
    samplers[15] =
        samplers[16] =
            samplers[17] =
                samplers[18] = ShadowDepthSamplerPCF;
    samplers[19] = OmniShadowMapSampler;

    pipelineCI.ResourceLayout.NumSamplers = HK_ARRAY_SIZE(samplers);
    pipelineCI.ResourceLayout.Samplers = samplers;

    BufferInfo buffers[7];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    buffers[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    buffers[3].BufferBinding = BUFFER_BIND_CONSTANT; // shadow cascade
    buffers[4].BufferBinding = BUFFER_BIND_CONSTANT; // light buffer
    buffers[5].BufferBinding = BUFFER_BIND_CONSTANT; // IBL buffer
    buffers[6].BufferBinding = BUFFER_BIND_CONSTANT; // VT buffer

    pipelineCI.ResourceLayout.NumBuffers = 7;
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, &pipeline);
    return pipeline;
}

TRef<RenderCore::IPipeline> CreateTerrainMaterialWireframe()
{
    TRef<RenderCore::IPipeline> pipeline;

    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

    BlendingStateInfo& bsd = pipelineCI.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset(BLENDING_ALPHA);

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof(TerrainVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof(TerrainPatchInstance);
    vertexBinding[1].InputRate = INPUT_RATE_PER_INSTANCE;

    pipelineCI.NumVertexBindings = 2;
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(VertexAttribsTerrainInstanced);
    pipelineCI.pVertexAttribs = VertexAttribsTerrainInstanced;

    ShaderFactory::CreateVertexShader("terrain_wireframe.vert", pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs, pipelineCI.pVS);
    ShaderFactory::CreateGeometryShader("terrain_wireframe.geom", pipelineCI.pGS);
    ShaderFactory::CreateFragmentShader("terrain_wireframe.frag", pipelineCI.pFS);

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;

    SamplerDesc clipmapSampler;
    clipmapSampler.Filter = FILTER_NEAREST;

    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.Samplers = &clipmapSampler;

    BufferInfo buffers[2];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    buffers[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    pipelineCI.ResourceLayout.NumBuffers = 2;
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, &pipeline);
    return pipeline;
}


MaterialGPU::MaterialGPU(CompiledMaterial const* pCompiledMaterial, String const& code)
{
    HK_ASSERT(pCompiledMaterial);

    MaterialType = pCompiledMaterial->Type;
    LightmapSlot = pCompiledMaterial->LightmapSlot;
    DepthPassTextureCount = pCompiledMaterial->DepthPassTextureCount;
    LightPassTextureCount = pCompiledMaterial->LightPassTextureCount;
    WireframePassTextureCount = pCompiledMaterial->WireframePassTextureCount;
    NormalsPassTextureCount = pCompiledMaterial->NormalsPassTextureCount;
    ShadowMapPassTextureCount = pCompiledMaterial->ShadowMapPassTextureCount;

    POLYGON_CULL cullMode = pCompiledMaterial->bTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;

    //{
    //    File f = File::OpenWrite( "test.txt" );
    //    f.WriteBuffer( code.CStr(), code.Length() );
    //}

    bool bTessellation = pCompiledMaterial->TessellationMethod != TESSELLATION_DISABLED;
    bool bTessellationShadowMap = bTessellation && pCompiledMaterial->bDisplacementAffectShadow;

    switch (MaterialType)
    {
        case MATERIAL_TYPE_PBR:
        case MATERIAL_TYPE_BASELIGHT:
        case MATERIAL_TYPE_UNLIT: {
            for (int i = 0; i < 2; i++)
            {
                bool bSkinned = !!i;

                CreateDepthPassPipeline(&DepthPass[i], code.CStr(), pCompiledMaterial->bAlphaMasking, cullMode, bSkinned, bTessellation, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->DepthPassTextureCount);
                CreateDepthVelocityPassPipeline(&DepthVelocityPass[i], code.CStr(), cullMode, bSkinned, bTessellation, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->DepthPassTextureCount);
                CreateLightPassPipeline(&LightPass[i], code.CStr(), cullMode, bSkinned, pCompiledMaterial->bDepthTest_EXPERIMENTAL, pCompiledMaterial->bTranslucent, pCompiledMaterial->Blending, bTessellation, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->LightPassTextureCount);
                CreateWireframePassPipeline(&WireframePass[i], code.CStr(), cullMode, bSkinned, bTessellation, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->WireframePassTextureCount);
                CreateNormalsPassPipeline(&NormalsPass[i], code.CStr(), bSkinned, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->NormalsPassTextureCount);
                CreateShadowMapPassPipeline(&ShadowPass[i], code.CStr(), pCompiledMaterial->bShadowMapMasking, pCompiledMaterial->bTwoSided, bSkinned, bTessellationShadowMap, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->ShadowMapPassTextureCount);
                CreateOmniShadowMapPassPipeline(&OmniShadowPass[i], code.CStr(), pCompiledMaterial->bShadowMapMasking, pCompiledMaterial->bTwoSided, bSkinned, bTessellationShadowMap, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->ShadowMapPassTextureCount);
                CreateFeedbackPassPipeline(&FeedbackPass[i], code.CStr(), cullMode, bSkinned, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->LightPassTextureCount); // FIXME: Add FeedbackPassTextureCount
                CreateOutlinePassPipeline(&OutlinePass[i], code.CStr(), cullMode, bSkinned, bTessellation, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->DepthPassTextureCount);
            }

            if (MaterialType != MATERIAL_TYPE_UNLIT)
            {
                CreateLightPassLightmapPipeline(&LightPassLightmap, code.CStr(), cullMode, pCompiledMaterial->bDepthTest_EXPERIMENTAL, pCompiledMaterial->bTranslucent, pCompiledMaterial->Blending, bTessellation, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->LightPassTextureCount);
                CreateLightPassVertexLightPipeline(&LightPassVertexLight, code.CStr(), cullMode, pCompiledMaterial->bDepthTest_EXPERIMENTAL, pCompiledMaterial->bTranslucent, pCompiledMaterial->Blending, bTessellation, pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->LightPassTextureCount);
            }
            break;
        }
        case MATERIAL_TYPE_HUD:
        case MATERIAL_TYPE_POSTPROCESS: {
#if 0
            CreateHUDPipeline(&HUDPipeline, code.CStr(), pCompiledMaterial->Samplers.ToPtr(), pCompiledMaterial->Samplers.Size());
#endif
            break;
        }
        default:
            HK_ASSERT(0);
    }
}

HK_NAMESPACE_END
