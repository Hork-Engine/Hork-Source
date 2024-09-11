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

HK_NAMESPACE_BEGIN

uint32_t MaterialBinary::AddShader(RenderCore::SHADER_TYPE shaderType, HeapBlob blob)
{
    if (blob.IsEmpty())
        return ~0u;

    uint32_t index = Shaders.Size();
    Shaders.EmplaceBack(shaderType, std::move(blob));
    return index;
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
