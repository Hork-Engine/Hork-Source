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

#include "Material.h"
#include "ResourceManager.h"
#include "MaterialGraph.h"
#include "Engine.h"

#include <Engine/Assets/Asset.h>
#include <Engine/Core/Platform/Logger.h>
#include <Engine/Core/Parse.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(Material)
HK_CLASS_META(MaterialInstance)

TList<Material> Material::m_MaterialRegistry;

Material::Material()
{
    m_MaterialRegistry.Add(this);
}

Material::Material(CompiledMaterial* pCompiledMaterial) :
    m_pCompiledMaterial(pCompiledMaterial)
{
    HK_ASSERT(pCompiledMaterial);

    m_GpuMaterial = MakeRef<MaterialGPU>(pCompiledMaterial);

    m_MaterialRegistry.Add(this);
}

Material::~Material()
{
    m_MaterialRegistry.Remove(this);
}

MaterialInstance* Material::Instantiate()
{
    return NewObj<MaterialInstance>(this);
}

uint32_t Material::GetTextureSlotByName(StringView Name) const
{
    // TODO
    return ~0u;
}

uint32_t Material::GetConstantOffsetByName(StringView Name) const
{
    // TODO
    return ~0u;
}

uint32_t Material::NumTextureSlots() const
{
    return m_pCompiledMaterial->Samplers.Size();
}

bool Material::LoadResource(IBinaryStreamReadInterface& Stream)
{
    uint32_t fileFormat = Stream.ReadUInt32();

    if (fileFormat != ASSET_VERSION_MATERIAL)
    {
        LOG("Expected file format {}\n", ASSET_VERSION_MATERIAL);
        return false;
    }

    uint32_t fileVersion = Stream.ReadUInt32();

    if (fileVersion != ASSET_VERSION_MATERIAL)
    {
        LOG("Expected file version {}\n", ASSET_VERSION_MATERIAL);
        return false;
    }

    m_pCompiledMaterial = MakeRef<CompiledMaterial>(Stream);
    m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);

    return true;
}

bool WriteMaterial(String const& Path, CompiledMaterial const* pCompiledMaterial)
{
    File f = File::OpenWrite(Path);
    if (!f)
    {
        return false;
    }

    f.WriteUInt32(ASSET_VERSION_MATERIAL);
    f.WriteUInt32(ASSET_VERSION_MATERIAL);

    pCompiledMaterial->Write(f);

    return true;
}

void Material::LoadInternalResource(StringView Path)
{
    if (!Path.Icmp("/Default/Materials/Unlit"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        graph->BindInput("Color",
                         graph->Add2<MGTextureLoad>()
                             .BindInput("TexCoord", graph->Add2<MGInTexCoord>())
                             .BindInput("Texture", diffuseTexture));

        graph->MaterialType = MATERIAL_TYPE_UNLIT;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/UnlitMask"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        auto diffuseTexture    = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        graph->BindInput("Color", textureSampler["RGBA"]);
        graph->BindInput("AlphaMask", textureSampler["A"]);
        graph->BindInput("ShadowMask", textureSampler["A"]);
        graph->bTwoSided = true;

        graph->MaterialType = MATERIAL_TYPE_UNLIT;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/UnlitOpacity"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        graph->BindInput("Color", textureSampler["RGBA"]);
        graph->BindInput("Opacity", textureSampler["A"]);

        // FIXME: ShadowMask?

        graph->MaterialType = MATERIAL_TYPE_UNLIT;
        graph->Blending     = COLOR_BLENDING_ALPHA;
        graph->bTranslucent = true;
        graph->bTwoSided    = true;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/BaseLight"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        graph->BindInput("Color", textureSampler["RGBA"]);

        graph->MaterialType = MATERIAL_TYPE_BASELIGHT;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/DefaultPBR"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* metallicTexture = graph->GetTexture(1);
        metallicTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* normalTexture = graph->GetTexture(2);
        normalTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* roughnessTexture = graph->GetTexture(3);
        roughnessTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        auto& normalSampler = graph->Add2<MGNormalLoad>();
        normalSampler.BindInput("TexCoord", inTexCoord);
        normalSampler.BindInput("Texture", normalTexture);
        normalSampler.Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

        auto& metallicSampler = graph->Add2<MGTextureLoad>();
        metallicSampler.BindInput("TexCoord", inTexCoord);
        metallicSampler.BindInput("Texture", metallicTexture);

        auto& roughnessSampler = graph->Add2<MGTextureLoad>();
        roughnessSampler.BindInput("TexCoord", inTexCoord);
        roughnessSampler.BindInput("Texture", roughnessTexture);

        graph->BindInput("Color", textureSampler);
        graph->BindInput("Normal", normalSampler["XYZ"]);
        graph->BindInput("Metallic", metallicSampler["R"]);
        graph->BindInput("Roughness", roughnessSampler["R"]);

        graph->MaterialType = MATERIAL_TYPE_PBR;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/PBRMetallicRoughness"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* metallicRoughnessTexture = graph->GetTexture(1);
        metallicRoughnessTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* normalTexture = graph->GetTexture(2);
        normalTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* ambientTexture = graph->GetTexture(3);
        ambientTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* emissiveTexture = graph->GetTexture(4);
        emissiveTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        auto& normalSampler = graph->Add2<MGNormalLoad>();
        normalSampler.BindInput("TexCoord", inTexCoord);
        normalSampler.BindInput("Texture", normalTexture);
        normalSampler.Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

        auto& metallicRoughnessSampler = graph->Add2<MGTextureLoad>();
        metallicRoughnessSampler.BindInput("TexCoord", inTexCoord);
        metallicRoughnessSampler.BindInput("Texture", metallicRoughnessTexture);

        auto& ambientSampler = graph->Add2<MGTextureLoad>();
        ambientSampler.BindInput("TexCoord", inTexCoord);
        ambientSampler.BindInput("Texture", ambientTexture);

        auto& emissiveSampler = graph->Add2<MGTextureLoad>();
        emissiveSampler.BindInput("TexCoord", inTexCoord);
        emissiveSampler.BindInput("Texture", emissiveTexture);

        graph->BindInput("Color", textureSampler);
        graph->BindInput("Normal", normalSampler["XYZ"]);
        graph->BindInput("Metallic", metallicRoughnessSampler["B"]);
        graph->BindInput("Roughness", metallicRoughnessSampler["G"]);
        graph->BindInput("AmbientOcclusion", ambientSampler["R"]);
        graph->BindInput("Emissive", emissiveSampler);

        graph->MaterialType = MATERIAL_TYPE_PBR;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/PBRMetallicRoughnessMask"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* metallicRoughnessTexture = graph->GetTexture(1);
        metallicRoughnessTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* normalTexture = graph->GetTexture(2);
        normalTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* ambientTexture = graph->GetTexture(3);
        ambientTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* emissiveTexture = graph->GetTexture(4);
        emissiveTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        auto& normalSampler = graph->Add2<MGNormalLoad>();
        normalSampler.BindInput("TexCoord", inTexCoord);
        normalSampler.BindInput("Texture", normalTexture);
        normalSampler.Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

        auto& metallicRoughnessSampler = graph->Add2<MGTextureLoad>();
        metallicRoughnessSampler.BindInput("TexCoord", inTexCoord);
        metallicRoughnessSampler.BindInput("Texture", metallicRoughnessTexture);

        auto& ambientSampler = graph->Add2<MGTextureLoad>();
        ambientSampler.BindInput("TexCoord", inTexCoord);
        ambientSampler.BindInput("Texture", ambientTexture);

        auto& emissiveSampler = graph->Add2<MGTextureLoad>();
        emissiveSampler.BindInput("TexCoord", inTexCoord);
        emissiveSampler.BindInput("Texture", emissiveTexture);

        graph->BindInput("Color", textureSampler);
        graph->BindInput("Normal", normalSampler["XYZ"]);
        graph->BindInput("Metallic", metallicRoughnessSampler["B"]);
        graph->BindInput("Roughness", metallicRoughnessSampler["G"]);
        graph->BindInput("AmbientOcclusion", ambientSampler["R"]);
        graph->BindInput("Emissive", emissiveSampler);
        graph->BindInput("AlphaMask", textureSampler["A"]);
        graph->BindInput("ShadowMask", textureSampler["A"]);
        graph->bTwoSided = true;

        graph->MaterialType = MATERIAL_TYPE_PBR;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/PBRMetallicRoughnessOpacity"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* metallicRoughnessTexture = graph->GetTexture(1);
        metallicRoughnessTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* normalTexture = graph->GetTexture(2);
        normalTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* ambientTexture = graph->GetTexture(3);
        ambientTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* emissiveTexture = graph->GetTexture(4);
        emissiveTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        auto& normalSampler = graph->Add2<MGNormalLoad>();
        normalSampler.BindInput("TexCoord", inTexCoord);
        normalSampler.BindInput("Texture", normalTexture);
        normalSampler.Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

        auto& metallicRoughnessSampler = graph->Add2<MGTextureLoad>();
        metallicRoughnessSampler.BindInput("TexCoord", inTexCoord);
        metallicRoughnessSampler.BindInput("Texture", metallicRoughnessTexture);

        auto& ambientSampler = graph->Add2<MGTextureLoad>();
        ambientSampler.BindInput("TexCoord", inTexCoord);
        ambientSampler.BindInput("Texture", ambientTexture);

        auto& emissiveSampler = graph->Add2<MGTextureLoad>();
        emissiveSampler.BindInput("TexCoord", inTexCoord);
        emissiveSampler.BindInput("Texture", emissiveTexture);

        graph->BindInput("Color", textureSampler);
        graph->BindInput("Normal", normalSampler["XYZ"]);
        graph->BindInput("Metallic", metallicRoughnessSampler["B"]);
        graph->BindInput("Roughness", metallicRoughnessSampler["G"]);
        graph->BindInput("AmbientOcclusion", ambientSampler["R"]);
        graph->BindInput("Emissive", emissiveSampler);
        graph->BindInput("Opacity", textureSampler["A"]);
        graph->BindInput("ShadowMask", textureSampler["A"]);

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->Blending     = COLOR_BLENDING_ALPHA;
        graph->bTranslucent = true;
        graph->bTwoSided    = true;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/PBRMetallicRoughnessFactor"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* metallicRoughnessTexture = graph->GetTexture(1);
        metallicRoughnessTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* normalTexture = graph->GetTexture(2);
        normalTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* ambientTexture = graph->GetTexture(3);
        ambientTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* emissiveTexture = graph->GetTexture(4);
        emissiveTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        auto& normalSampler = graph->Add2<MGNormalLoad>();
        normalSampler.BindInput("TexCoord", inTexCoord);
        normalSampler.BindInput("Texture", normalTexture);
        normalSampler.Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

        auto& metallicRoughnessSampler = graph->Add2<MGTextureLoad>();
        metallicRoughnessSampler.BindInput("TexCoord", inTexCoord);
        metallicRoughnessSampler.BindInput("Texture", metallicRoughnessTexture);

        auto& ambientSampler = graph->Add2<MGTextureLoad>();
        ambientSampler.BindInput("TexCoord", inTexCoord);
        ambientSampler.BindInput("Texture", ambientTexture);

        auto& emissiveSampler = graph->Add2<MGTextureLoad>();
        emissiveSampler.BindInput("TexCoord", inTexCoord);
        emissiveSampler.BindInput("Texture", emissiveTexture);

        auto& baseColorFactor       = graph->Add2<MGUniformAddress>();
        baseColorFactor.UniformType = MG_UNIFORM_TYPE_FLOAT4;
        baseColorFactor.Address     = 0;

        auto& metallicFactor       = graph->Add2<MGUniformAddress>();
        metallicFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        metallicFactor.Address     = 4;

        auto& roughnessFactor       = graph->Add2<MGUniformAddress>();
        roughnessFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        roughnessFactor.Address     = 5;

        auto& emissiveFactor       = graph->Add2<MGUniformAddress>();
        emissiveFactor.UniformType = MG_UNIFORM_TYPE_FLOAT3;
        emissiveFactor.Address     = 8;

        auto& colorMul = graph->Add2<MGMul>();
        colorMul.BindInput("A", textureSampler);
        colorMul.BindInput("B", baseColorFactor);

        auto& metallicMul = graph->Add2<MGMul>();
        metallicMul.BindInput("A", metallicRoughnessSampler["B"]);
        metallicMul.BindInput("B", metallicFactor);

        auto& roughnessMul = graph->Add2<MGMul>();
        roughnessMul.BindInput("A", metallicRoughnessSampler["G"]);
        roughnessMul.BindInput("B", roughnessFactor);

        auto& emissiveMul = graph->Add2<MGMul>();
        emissiveMul.BindInput("A", emissiveSampler["RGB"]);
        emissiveMul.BindInput("B", emissiveFactor);

        graph->BindInput("Color", colorMul);
        graph->BindInput("Normal", normalSampler["XYZ"]);
        graph->BindInput("Metallic", metallicMul);
        graph->BindInput("Roughness", roughnessMul);
        graph->BindInput("AmbientOcclusion", ambientSampler["R"]);
        graph->BindInput("Emissive", emissiveMul);

        graph->MaterialType = MATERIAL_TYPE_PBR;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/PBRMetallicRoughnessFactorMask"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* metallicRoughnessTexture = graph->GetTexture(1);
        metallicRoughnessTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* normalTexture = graph->GetTexture(2);
        normalTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* ambientTexture = graph->GetTexture(3);
        ambientTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* emissiveTexture = graph->GetTexture(4);
        emissiveTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        auto& normalSampler = graph->Add2<MGNormalLoad>();
        normalSampler.BindInput("TexCoord", inTexCoord);
        normalSampler.BindInput("Texture", normalTexture);
        normalSampler.Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

        auto& metallicRoughnessSampler = graph->Add2<MGTextureLoad>();
        metallicRoughnessSampler.BindInput("TexCoord", inTexCoord);
        metallicRoughnessSampler.BindInput("Texture", metallicRoughnessTexture);

        auto& ambientSampler = graph->Add2<MGTextureLoad>();
        ambientSampler.BindInput("TexCoord", inTexCoord);
        ambientSampler.BindInput("Texture", ambientTexture);

        auto& emissiveSampler = graph->Add2<MGTextureLoad>();
        emissiveSampler.BindInput("TexCoord", inTexCoord);
        emissiveSampler.BindInput("Texture", emissiveTexture);

        auto& baseColorFactor       = graph->Add2<MGUniformAddress>();
        baseColorFactor.UniformType = MG_UNIFORM_TYPE_FLOAT4;
        baseColorFactor.Address = 0;

        auto& metallicFactor       = graph->Add2<MGUniformAddress>();
        metallicFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        metallicFactor.Address = 4;

        auto& roughnessFactor       = graph->Add2<MGUniformAddress>();
        roughnessFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        roughnessFactor.Address = 5;

        auto& emissiveFactor       = graph->Add2<MGUniformAddress>();
        emissiveFactor.UniformType = MG_UNIFORM_TYPE_FLOAT3;
        emissiveFactor.Address = 8;

        auto& colorMul = graph->Add2<MGMul>();
        colorMul.BindInput("A", textureSampler);
        colorMul.BindInput("B", baseColorFactor);

        auto& metallicMul = graph->Add2<MGMul>();
        metallicMul.BindInput("A", metallicRoughnessSampler["B"]);
        metallicMul.BindInput("B", metallicFactor);

        auto& roughnessMul = graph->Add2<MGMul>();
        roughnessMul.BindInput("A", metallicRoughnessSampler["G"]);
        roughnessMul.BindInput("B", roughnessFactor);

        auto& emissiveMul = graph->Add2<MGMul>();
        emissiveMul.BindInput("A", emissiveSampler["RGB"]);
        emissiveMul.BindInput("B", emissiveFactor);

        graph->BindInput("Color", colorMul);
        graph->BindInput("Normal", normalSampler["XYZ"]);
        graph->BindInput("Metallic", metallicMul);
        graph->BindInput("Roughness", roughnessMul);
        graph->BindInput("AmbientOcclusion", ambientSampler["R"]);
        graph->BindInput("Emissive", emissiveMul);
        graph->BindInput("AlphaMask", textureSampler["A"]);
        graph->BindInput("ShadowMask", textureSampler["A"]);
        graph->bTwoSided = true;

        graph->MaterialType = MATERIAL_TYPE_PBR;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/PBRMetallicRoughnessFactorOpacity"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inTexCoord = graph->Add2<MGInTexCoord>();

        MGTextureSlot* diffuseTexture = graph->GetTexture(0);
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* metallicRoughnessTexture = graph->GetTexture(1);
        metallicRoughnessTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* normalTexture = graph->GetTexture(2);
        normalTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* ambientTexture = graph->GetTexture(3);
        ambientTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot* emissiveTexture = graph->GetTexture(4);
        emissiveTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        auto& textureSampler = graph->Add2<MGTextureLoad>();
        textureSampler.BindInput("TexCoord", inTexCoord);
        textureSampler.BindInput("Texture", diffuseTexture);

        auto& normalSampler = graph->Add2<MGNormalLoad>();
        normalSampler.BindInput("TexCoord", inTexCoord);
        normalSampler.BindInput("Texture", normalTexture);
        normalSampler.Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

        auto& metallicRoughnessSampler = graph->Add2<MGTextureLoad>();
        metallicRoughnessSampler.BindInput("TexCoord", inTexCoord);
        metallicRoughnessSampler.BindInput("Texture", metallicRoughnessTexture);

        auto& ambientSampler = graph->Add2<MGTextureLoad>();
        ambientSampler.BindInput("TexCoord", inTexCoord);
        ambientSampler.BindInput("Texture", ambientTexture);

        auto& emissiveSampler = graph->Add2<MGTextureLoad>();
        emissiveSampler.BindInput("TexCoord", inTexCoord);
        emissiveSampler.BindInput("Texture", emissiveTexture);

        auto& baseColorFactor       = graph->Add2<MGUniformAddress>();
        baseColorFactor.UniformType = MG_UNIFORM_TYPE_FLOAT4;
        baseColorFactor.Address = 0;

        auto& metallicFactor       = graph->Add2<MGUniformAddress>();
        metallicFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        metallicFactor.Address = 4;

        auto& roughnessFactor       = graph->Add2<MGUniformAddress>();
        roughnessFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        roughnessFactor.Address = 5;

        auto& emissiveFactor       = graph->Add2<MGUniformAddress>();
        emissiveFactor.UniformType = MG_UNIFORM_TYPE_FLOAT3;
        emissiveFactor.Address = 8;

        auto& colorMul = graph->Add2<MGMul>();
        colorMul.BindInput("A", textureSampler);
        colorMul.BindInput("B", baseColorFactor);

        auto& metallicMul = graph->Add2<MGMul>();
        metallicMul.BindInput("A", metallicRoughnessSampler["B"]);
        metallicMul.BindInput("B", metallicFactor);

        auto& roughnessMul = graph->Add2<MGMul>();
        roughnessMul.BindInput("A", metallicRoughnessSampler["G"]);
        roughnessMul.BindInput("B", roughnessFactor);

        auto& emissiveMul = graph->Add2<MGMul>();
        emissiveMul.BindInput("A", emissiveSampler["RGB"]);
        emissiveMul.BindInput("B", emissiveFactor);

        graph->BindInput("Color", colorMul);
        graph->BindInput("Normal", normalSampler["XYZ"]);
        graph->BindInput("Metallic", metallicMul);
        graph->BindInput("Roughness", roughnessMul);
        graph->BindInput("AmbientOcclusion", ambientSampler["R"]);
        graph->BindInput("Emissive", emissiveMul);
        graph->BindInput("Opacity", textureSampler["A"]);
        graph->BindInput("ShadowMask", textureSampler["A"]);

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->Blending     = COLOR_BLENDING_ALPHA;
        graph->bTranslucent = true;
        graph->bTwoSided    = true;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/PBRMetallicRoughnessNoTex"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& baseColorFactor       = graph->Add2<MGUniformAddress>();
        baseColorFactor.UniformType = MG_UNIFORM_TYPE_FLOAT4;
        baseColorFactor.Address = 0;

        auto& metallicFactor       = graph->Add2<MGUniformAddress>();
        metallicFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        metallicFactor.Address = 4;

        auto& roughnessFactor       = graph->Add2<MGUniformAddress>();
        roughnessFactor.UniformType = MG_UNIFORM_TYPE_FLOAT1;
        roughnessFactor.Address = 5;

        auto& emissiveFactor       = graph->Add2<MGUniformAddress>();
        emissiveFactor.UniformType = MG_UNIFORM_TYPE_FLOAT3;
        emissiveFactor.Address = 8;

        graph->BindInput("Color", baseColorFactor);
        graph->BindInput("Metallic", metallicFactor);
        graph->BindInput("Roughness", roughnessFactor);
        graph->BindInput("Emissive", emissiveFactor);

        graph->MaterialType = MATERIAL_TYPE_PBR;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    if (!Path.Icmp("/Default/Materials/Skybox"))
    {
        MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

        auto& inPosition = graph->Add2<MGInPosition>();

        MGTextureSlot* cubemapTexture = graph->GetTexture(0);
        cubemapTexture->TextureType = TEXTURE_CUBE;
        cubemapTexture->Filter      = TEXTURE_FILTER_LINEAR;
        cubemapTexture->AddressU    = TEXTURE_ADDRESS_CLAMP;
        cubemapTexture->AddressV    = TEXTURE_ADDRESS_CLAMP;
        cubemapTexture->AddressW    = TEXTURE_ADDRESS_CLAMP;

        auto& cubemapSampler = graph->Add2<MGTextureLoad>();
        cubemapSampler.BindInput("TexCoord", inPosition);
        cubemapSampler.BindInput("Texture", cubemapTexture);

        graph->BindInput("Color", cubemapSampler);

        graph->MaterialType = MATERIAL_TYPE_UNLIT;
        graph->DepthHack    = MATERIAL_DEPTH_HACK_SKYBOX;

        m_pCompiledMaterial = graph->Compile();
        m_GpuMaterial       = MakeRef<MaterialGPU>(m_pCompiledMaterial);
        return;
    }

    LOG("Unknown internal material {}\n", Path);

    //LoadInternalResource( "/Default/Materials/Unlit" );
    LoadInternalResource("/Default/Materials/BaseLight");
}

MaterialInstance::MaterialInstance()
{
    static TStaticResourceFinder<Material> MaterialResource("/Default/Materials/Unlit"s);
    static TStaticResourceFinder<Texture>  TextureResource("/Common/grid8.webp"s);

    m_pMaterial = MaterialResource;
    SetTexture(0, TextureResource);
}

MaterialInstance::MaterialInstance(Material* pMaterial) :
    m_pMaterial(pMaterial)
{
    HK_ASSERT(pMaterial);

    if (!pMaterial)
    {
        static TStaticResourceFinder<Material> MaterialResource("/Default/Materials/Unlit"s);
        static TStaticResourceFinder<Texture>  TextureResource("/Common/grid8.webp"s);

        m_pMaterial = MaterialResource;
        SetTexture(0, TextureResource);
    }
}

void MaterialInstance::LoadInternalResource(StringView Path)
{
    if (!Path.Icmp("/Default/MaterialInstance/BaseLight"))
    {
        static TStaticResourceFinder<Material> MaterialResource("/Default/Materials/BaseLight"s);
        static TStaticResourceFinder<Texture>  TextureResource("/Common/grid8.webp"s);

        m_pMaterial = MaterialResource;

        SetTexture(0, TextureResource);
        return;
    }
    if (!Path.Icmp("/Default/MaterialInstance/Metal"))
    {
        static TStaticResourceFinder<Material> MaterialResource("/Default/Materials/PBRMetallicRoughnessNoTex"s);

        m_pMaterial = MaterialResource;

        // Base color
        m_UniformVectors[0] = Float4(0.8f, 0.8f, 0.8f, 1.0f);
        // Metallic
        m_UniformVectors[1].X = 1;
        // Roughness
        m_UniformVectors[1].Y = 0.5f;
        // Emissive
        m_UniformVectors[2] = Float4(0.0f);
        return;
    }
    if (!Path.Icmp("/Default/MaterialInstance/Dielectric") || !Path.Icmp("/Default/MaterialInstance/Default"))
    {
        static TStaticResourceFinder<Material> MaterialResource("/Default/Materials/PBRMetallicRoughnessNoTex"s);

        m_pMaterial = MaterialResource;

        // Base color
        m_UniformVectors[0] = Float4(0.8f, 0.8f, 0.8f, 1.0f);
        // Metallic
        m_UniformVectors[1].X = 0.0f;
        // Roughness
        m_UniformVectors[1].Y = 0.5f;
        // Emissive
        m_UniformVectors[2] = Float4(0.0f);
        return;
    }
    LOG("Unknown internal material instance {}\n", Path);

    LoadInternalResource("/Default/MaterialInstance/Default");
}

bool MaterialInstance::LoadResource(IBinaryStreamReadInterface& Stream)
{
    uint32_t fileFormat;
    uint32_t fileVersion;

    fileFormat = Stream.ReadUInt32();

    if (fileFormat != ASSET_MATERIAL_INSTANCE)
    {
        //LOG( "Expected file format {}\n", ASSET_MATERIAL_INSTANCE );

        Stream.Rewind();
        return LoadTextVersion(Stream);
    }

    fileVersion = Stream.ReadUInt32();

    if (fileVersion != ASSET_VERSION_MATERIAL_INSTANCE)
    {
        LOG("Expected file version {}\n", ASSET_VERSION_MATERIAL_INSTANCE);
        return false;
    }

    String material = Stream.ReadString();

    m_pMaterial = GetOrCreateResource<Material>(material);

    uint32_t texCount = Stream.ReadUInt32();
    for (uint32_t slot = 0; slot < texCount; slot++)
    {
        String textureGUID = Stream.ReadString();

        SetTexture(slot, GetOrCreateResource<Texture>(textureGUID));
    }

    for (int i = 0; i < MAX_MATERIAL_UNIFORMS; i++)
    {
        m_Uniforms[i] = Stream.ReadFloat();
    }

    return true;
}

bool MaterialInstance::LoadTextVersion(IBinaryStreamReadInterface& Stream)
{
    String text = Stream.AsString();

    DocumentDeserializeInfo deserializeInfo;
    deserializeInfo.pDocumentData = text.CStr();
    deserializeInfo.bInsitu       = true;

    Document doc;
    doc.DeserializeFromString(deserializeInfo);

    DocumentMember* member;

    member = doc.FindMember("Material");

    m_pMaterial = GetOrCreateResource<Material>(member ? member->GetStringView() : "/Default/Materials/Unlit");

    member = doc.FindMember("Textures");
    if (member)
    {
        DocumentValue* values  = member->GetArrayValues();
        int        texSlot = 0;
        for (DocumentValue* v = values; v && texSlot < MAX_MATERIAL_TEXTURES; v = v->GetNext())
        {
            SetTexture(texSlot++, GetOrCreateResource<Texture>(v->GetStringView()));
        }
    }

    member = doc.FindMember("Uniforms");
    if (member)
    {
        DocumentValue* values     = member->GetArrayValues();
        int        uniformNum = 0;
        for (DocumentValue* v = values; v && uniformNum < MAX_MATERIAL_UNIFORMS; v = v->GetNext())
        {
            m_Uniforms[uniformNum++] = Core::ParseFloat(v->GetStringView());
        }
    }

    return true;
}

void MaterialInstance::SetTexture(StringView Name, TextureView* pView)
{
    uint32_t slot = GetTextureSlotByName(Name);
    if (slot < NumTextureSlots())
        m_Textures[slot] = pView;
    else
        LOG("MaterialInstance::SetTexture: Unknown texture slot {}\n", Name);
}

void MaterialInstance::SetTexture(StringView Name, Texture* pTexture)
{
    SetTexture(Name, pTexture->GetView());
}

void MaterialInstance::SetTexture(uint32_t Slot, TextureView* pView)
{
    if (Slot < NumTextureSlots())
        m_Textures[Slot] = pView;
    else
        LOG("MaterialInstance::SetTexture: Invalid texture slot {}\n", Slot);
}

void MaterialInstance::SetTexture(uint32_t Slot, Texture* pTexture)
{
    SetTexture(Slot, pTexture->GetView());
}

TextureView* MaterialInstance::GetTexture(StringView Name)
{
    uint32_t slot = GetTextureSlotByName(Name);
    if (slot < NumTextureSlots())
        return m_Textures[slot];
    LOG("MaterialInstance::GetTexture: Unknown texture slot {}\n", Name);
    return nullptr;
}

TextureView* MaterialInstance::GetTexture(uint32_t Slot)
{
    if (Slot < NumTextureSlots())
        return m_Textures[Slot];
    LOG("MaterialInstance::GetTexture: Invalid texture slot {}\n", Slot);
    return nullptr;
}

void MaterialInstance::UnsetTextures()
{
    for (uint32_t slot = 0; slot < MAX_MATERIAL_TEXTURES; slot++)
        m_Textures[slot].Reset();
}

void MaterialInstance::SetConstant(StringView Name, float Value)
{
    uint32_t offset = GetConstantOffsetByName(Name);
    if (offset < MAX_MATERIAL_UNIFORMS)
        m_Uniforms[offset] = Value;
    else
        LOG("MaterialInstance::SetConstant: Unknown constant {}\n", Name);
}

void MaterialInstance::SetConstant(uint32_t Offset, float Value)
{
    if (Offset < MAX_MATERIAL_UNIFORMS)
        m_Uniforms[Offset] = Value;
    else
        LOG("MaterialInstance::SetConstant: Invalid offset {}\n", Offset);
}

float MaterialInstance::GetConstant(StringView Name) const
{
    uint32_t offset = GetConstantOffsetByName(Name);
    if (offset < MAX_MATERIAL_UNIFORMS)
        return m_Uniforms[offset];
    LOG("MaterialInstance::GetConstant: Unknown constant {}\n", Name);
    return 0.0f;
}

float MaterialInstance::GetConstant(uint32_t Offset) const
{
    if (Offset < MAX_MATERIAL_UNIFORMS)
        return m_Uniforms[Offset];
    LOG("MaterialInstance::GetConstant: Invalid offset {}\n", Offset);
    return 0.0f;
}

void MaterialInstance::SetVector(StringView Name, Float4 const& Value)
{
    uint32_t offset = GetConstantOffsetByName(Name);
    if (offset < MAX_MATERIAL_UNIFORM_VECTORS)
        m_UniformVectors[offset] = Value;
    else
        LOG("MaterialInstance::SetVector: Unknown vector {}\n", Name);
}

void MaterialInstance::SetVector(uint32_t Offset, Float4 const& Value)
{
    if (Offset < MAX_MATERIAL_UNIFORM_VECTORS)
        m_UniformVectors[Offset] = Value;
    else
        LOG("MaterialInstance::SetVector: Invalid offset {}\n", Offset);
}

Float4 const& MaterialInstance::GetVector(StringView Name) const
{
    uint32_t offset = GetConstantOffsetByName(Name);
    if (offset < MAX_MATERIAL_UNIFORM_VECTORS)
        return m_UniformVectors[offset];
    LOG("MaterialInstance::GetVector: Unknown vector {}\n", Name);
    return Float4::Zero();
}

Float4 const& MaterialInstance::GetVector(uint32_t Offset) const
{
    if (Offset < MAX_MATERIAL_UNIFORM_VECTORS)
        return m_UniformVectors[Offset];
    LOG("MaterialInstance::GetVector: Invalid offset {}\n", Offset);
    return Float4::Zero();
}

uint32_t MaterialInstance::GetTextureSlotByName(StringView Name) const
{
    return m_pMaterial->GetTextureSlotByName(Name);
}

uint32_t MaterialInstance::GetConstantOffsetByName(StringView Name) const
{
    return m_pMaterial->GetConstantOffsetByName(Name);
}

uint32_t MaterialInstance::NumTextureSlots() const
{
    return m_pMaterial->NumTextureSlots();
}

/** Get material. Never return null. */
Material* MaterialInstance::GetMaterial() const
{
    return m_pMaterial;
}

void MaterialInstance::SetVirtualTexture(VirtualTextureResource* VirtualTex)
{
    m_VirtualTexture = VirtualTex;
}

MaterialFrameData* MaterialInstance::PreRenderUpdate(FrameLoop* FrameLoop, int _FrameNumber)
{
    if (m_VisFrame == _FrameNumber)
    {
        return m_FrameData;
    }

    m_VisFrame = _FrameNumber;

    m_FrameData = (MaterialFrameData*)FrameLoop->AllocFrameMem(sizeof(MaterialFrameData));

    m_FrameData->Material = m_pMaterial->GetGPUResource();

    RenderCore::ITexture** textures = m_FrameData->Textures;
    m_FrameData->NumTextures        = NumTextureSlots();

    HK_ASSERT(NumTextureSlots() <= MAX_MATERIAL_TEXTURES);

    for (int i = 0, count = NumTextureSlots(); i < count; ++i)
    {
        RenderCore::ITexture* texture = m_Textures[i] ? m_Textures[i]->GetResource() : nullptr;

        if (!texture)
        {
            m_FrameData = nullptr;
            LOG("Texture not set\n");
            return nullptr;
        }

        textures[i] = texture;
    }

    m_FrameData->NumUniformVectors = m_pMaterial->NumUniformVectors();
    Platform::Memcpy(m_FrameData->UniformVectors, m_UniformVectors, sizeof(Float4) * m_FrameData->NumUniformVectors);

    m_FrameData->VirtualTexture = m_VirtualTexture;

    return m_FrameData;
}

HK_NAMESPACE_END
