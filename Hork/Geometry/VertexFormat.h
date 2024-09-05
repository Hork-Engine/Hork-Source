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

#pragma once

#include <Hork/Core/Color.h>
#include <Hork/Math/VectorMath.h>

HK_NAMESPACE_BEGIN

//
// Vertex formats
//

struct MeshVertex
{
    Float3  Position;    // 4 * 3 = 12 bytes
    Half    TexCoord[2]; // 4 * 2 = 8 bytes      half: 4 bytes
    Half    Normal[3];   // 4 * 3 = 12 bytes     half: 6 bytes   byte: 3 bytes
    Half    Tangent[3];  // 4 * 3 = 12 bytes     half: 6 bytes   byte: 3 bytes
    int8_t  Handedness;  // 4 * 1 = 4 bytes      byte: 1 bytes
    uint8_t Pad[3];

    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteObject(Position);
        stream.WriteHalf(TexCoord[0]);
        stream.WriteHalf(TexCoord[1]);
        stream.WriteHalf(Normal[0]);
        stream.WriteHalf(Normal[1]);
        stream.WriteHalf(Normal[2]);
        stream.WriteHalf(Tangent[0]);
        stream.WriteHalf(Tangent[1]);
        stream.WriteHalf(Tangent[2]);
        stream.WriteInt8(Handedness);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        stream.ReadObject(Position);
        TexCoord[0] = stream.ReadHalf();
        TexCoord[1] = stream.ReadHalf();
        Normal[0] = stream.ReadHalf();
        Normal[1] = stream.ReadHalf();
        Normal[2] = stream.ReadHalf();
        Tangent[0] = stream.ReadHalf();
        Tangent[1] = stream.ReadHalf();
        Tangent[2] = stream.ReadHalf();
        Handedness = stream.ReadInt8();
    }

    void SetTexCoord(Half S, Half T)
    {
        TexCoord[0] = S;
        TexCoord[1] = T;
    }

    void SetTexCoord(Float2 const& texCoord)
    {
        TexCoord[0] = texCoord.X;
        TexCoord[1] = texCoord.Y;
    }

    const Float2 GetTexCoord() const
    {
        return Float2(TexCoord[0], TexCoord[1]);
    }

    void SetNormal(Half x, Half y, Half z)
    {
        Normal[0] = x;
        Normal[1] = y;
        Normal[2] = z;
    }

    void SetNormal(Float3 const& normal)
    {
        Normal[0] = normal.X;
        Normal[1] = normal.Y;
        Normal[2] = normal.Z;
    }

    Float3 GetNormal() const
    {
        return Float3(Normal[0], Normal[1], Normal[2]);
    }

    void SetTangent(Half x, Half y, Half z)
    {
        Tangent[0] = x;
        Tangent[1] = y;
        Tangent[2] = z;
    }

    void SetTangent(Float3 const& tangent)
    {
        Tangent[0] = tangent.X;
        Tangent[1] = tangent.Y;
        Tangent[2] = tangent.Z;
    }

    Float3 GetTangent() const
    {
        return Float3(Tangent[0], Tangent[1], Tangent[2]);
    }

    static MeshVertex sLerp(MeshVertex const& vertex1, MeshVertex const& vertex2, float frac = 0.5f);
};

static_assert(sizeof(MeshVertex) == 32, "Keep 32b vertex size");

HK_FORCEINLINE const MeshVertex MakeMeshVertex(Float3 const& position, Float2 const& texCoord, Float3 const& tangent, float handedness, Float3 const& normal)
{
    MeshVertex v;
    v.Position = position;
    v.SetTexCoord(texCoord);
    v.SetNormal(normal);
    v.SetTangent(tangent);
    v.Handedness = handedness > 0.0f ? 1 : -1;
    return v;
}

HK_FORCEINLINE MeshVertex MeshVertex::sLerp(MeshVertex const& vertex1, MeshVertex const& vertex2, float frac)
{
    MeshVertex Result;

    Result.Position = Math::Lerp(vertex1.Position, vertex2.Position, frac);
    Result.SetTexCoord(Math::Lerp(vertex1.GetTexCoord(), vertex2.GetTexCoord(), frac));
    Result.SetNormal(Math::Lerp(vertex1.GetNormal(), vertex2.GetNormal(), frac).Normalized());
    Result.SetTangent(Math::Lerp(vertex1.GetTangent(), vertex2.GetTangent(), frac).Normalized());
    Result.Handedness = frac >= 0.5f ? vertex2.Handedness : vertex1.Handedness;

    return Result;
}

struct MeshVertexUV
{
    Float2 TexCoord;

    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteObject(TexCoord);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        stream.ReadObject(TexCoord);
    }

    static MeshVertexUV sLerp(MeshVertexUV const& vertex1, MeshVertexUV const& vertex2, float frac = 0.5f);
};

HK_FORCEINLINE MeshVertexUV MeshVertexUV::sLerp(MeshVertexUV const& vertex1, MeshVertexUV const& vertex2, float frac)
{
    MeshVertexUV Result;

    Result.TexCoord = Math::Lerp(vertex1.TexCoord, vertex2.TexCoord, frac);

    return Result;
}

struct MeshVertexLight
{
    uint32_t VertexLight;

    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteUInt32(VertexLight);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        VertexLight = stream.ReadUInt32();
    }

    static MeshVertexLight sLerp(MeshVertexLight const& vertex1, MeshVertexLight const& vertex2, float frac = 0.5f);
};

HK_FORCEINLINE MeshVertexLight MeshVertexLight::sLerp(MeshVertexLight const& vertex1, MeshVertexLight const& vertex2, float frac)
{
    MeshVertexLight Result;

    const byte* c0 = reinterpret_cast<const byte*>(&vertex1.VertexLight);
    const byte* c1 = reinterpret_cast<const byte*>(&vertex2.VertexLight);
    byte*       r  = reinterpret_cast<byte*>(&Result.VertexLight);

#if 0
    r[0] = ( c0[0] + c1[0] ) >> 1;
    r[1] = ( c0[1] + c1[1] ) >> 1;
    r[2] = ( c0[2] + c1[2] ) >> 1;
    r[3] = ( c0[3] + c1[3] ) >> 1;
#else
    float linearColor1[3];
    float linearColor2[3];
    float resultColor[3];

    DecodeRGBE(linearColor1, c0);
    DecodeRGBE(linearColor2, c1);

    resultColor[0] = Math::Lerp(linearColor1[0], linearColor2[0], frac);
    resultColor[1] = Math::Lerp(linearColor1[1], linearColor2[1], frac);
    resultColor[2] = Math::Lerp(linearColor1[2], linearColor2[2], frac);

    EncodeRGBE(r, resultColor);
#endif

    return Result;
}

struct SkinVertex
{
    uint16_t    JointIndices[4];
    uint8_t     JointWeights[4];

    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteUInt16(JointIndices[0]);
        stream.WriteUInt16(JointIndices[1]);
        stream.WriteUInt16(JointIndices[2]);
        stream.WriteUInt16(JointIndices[3]);
        stream.Write(JointWeights, 4);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        JointIndices[0] = stream.ReadUInt16();
        JointIndices[1] = stream.ReadUInt16();
        JointIndices[2] = stream.ReadUInt16();
        JointIndices[3] = stream.ReadUInt16();
        stream.Read(JointWeights, 4);
    }
};

HK_NAMESPACE_END
