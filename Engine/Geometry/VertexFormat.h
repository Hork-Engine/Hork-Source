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

#pragma once

#include <Engine/Core/Color.h>
#include <Engine/Geometry/VectorMath.h>

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

    void Write(IBinaryStreamWriteInterface& Stream) const
    {
        Stream.WriteObject(Position);
        Stream.WriteObject(GetTexCoord());
        Stream.WriteObject(GetTangent());
        Stream.WriteFloat((float)Handedness);
        Stream.WriteObject(GetNormal());
    }

    void Read(IBinaryStreamReadInterface& Stream)
    {
        Float2 texCoord;
        Float3 normal;
        Float3 tangent;
        Stream.ReadObject(Position);
        Stream.ReadObject(texCoord);
        Stream.ReadObject(tangent);
        Handedness = (Stream.ReadFloat() > 0.0f) ? 1 : -1;
        Stream.ReadObject(normal);
        SetTexCoord(texCoord);
        SetNormal(normal);
        SetTangent(tangent);
    }

    void SetTexCoord(Half S, Half T)
    {
        TexCoord[0] = S;
        TexCoord[1] = T;
    }

    void SetTexCoord(Float2 const& _TexCoord)
    {
        TexCoord[0] = _TexCoord.X;
        TexCoord[1] = _TexCoord.Y;
    }

    const Float2 GetTexCoord() const
    {
        return Float2(TexCoord[0], TexCoord[1]);
    }

    void SetNormal(Half X, Half Y, Half Z)
    {
        Normal[0] = X;
        Normal[1] = Y;
        Normal[2] = Z;
    }

    void SetNormal(Float3 const& _Normal)
    {
        Normal[0] = _Normal.X;
        Normal[1] = _Normal.Y;
        Normal[2] = _Normal.Z;
    }

    const Float3 GetNormal() const
    {
        return Float3(Normal[0], Normal[1], Normal[2]);
    }

    void SetTangent(Half X, Half Y, Half Z)
    {
        Tangent[0] = X;
        Tangent[1] = Y;
        Tangent[2] = Z;
    }

    void SetTangent(Float3 const& _Tangent)
    {
        Tangent[0] = _Tangent.X;
        Tangent[1] = _Tangent.Y;
        Tangent[2] = _Tangent.Z;
    }

    const Float3 GetTangent() const
    {
        return Float3(Tangent[0], Tangent[1], Tangent[2]);
    }

    static MeshVertex Lerp(MeshVertex const& Vertex1, MeshVertex const& Vertex2, float Value = 0.5f);
};

static_assert(sizeof(MeshVertex) == 32, "Keep 32b vertex size");

HK_FORCEINLINE const MeshVertex MakeMeshVertex(Float3 const& Position, Float2 const& TexCoord, Float3 const& Tangent, float Handedness, Float3 const& Normal)
{
    MeshVertex v;
    v.Position = Position;
    v.SetTexCoord(TexCoord);
    v.SetNormal(Normal);
    v.SetTangent(Tangent);
    v.Handedness = Handedness > 0.0f ? 1 : -1;
    return v;
}

HK_FORCEINLINE MeshVertex MeshVertex::Lerp(MeshVertex const& Vertex1, MeshVertex const& Vertex2, float Value)
{
    MeshVertex Result;

    Result.Position = Math::Lerp(Vertex1.Position, Vertex2.Position, Value);
    Result.SetTexCoord(Math::Lerp(Vertex1.GetTexCoord(), Vertex2.GetTexCoord(), Value));
    Result.SetNormal(Math::Lerp(Vertex1.GetNormal(), Vertex2.GetNormal(), Value).Normalized());
    Result.SetTangent(Math::Lerp(Vertex1.GetTangent(), Vertex2.GetTangent(), Value).Normalized());
    Result.Handedness = Value >= 0.5f ? Vertex2.Handedness : Vertex1.Handedness;

    return Result;
}

struct MeshVertexUV
{
    Float2 TexCoord;

    void Write(IBinaryStreamWriteInterface& Stream) const
    {
        Stream.WriteObject(TexCoord);
    }

    void Read(IBinaryStreamReadInterface& Stream)
    {
        Stream.ReadObject(TexCoord);
    }

    static MeshVertexUV Lerp(MeshVertexUV const& Vertex1, MeshVertexUV const& Vertex2, float Value = 0.5f);
};

HK_FORCEINLINE MeshVertexUV MeshVertexUV::Lerp(MeshVertexUV const& Vertex1, MeshVertexUV const& Vertex2, float Value)
{
    MeshVertexUV Result;

    Result.TexCoord = Math::Lerp(Vertex1.TexCoord, Vertex2.TexCoord, Value);

    return Result;
}

struct MeshVertexLight
{
    uint32_t VertexLight;

    void Write(IBinaryStreamWriteInterface& Stream) const
    {
        Stream.WriteUInt32(VertexLight);
    }

    void Read(IBinaryStreamReadInterface& Stream)
    {
        VertexLight = Stream.ReadUInt32();
    }

    static MeshVertexLight Lerp(MeshVertexLight const& Vertex1, MeshVertexLight const& Vertex2, float Value = 0.5f);
};

HK_FORCEINLINE MeshVertexLight MeshVertexLight::Lerp(MeshVertexLight const& Vertex1, MeshVertexLight const& Vertex2, float Value)
{
    MeshVertexLight Result;

    const byte* c0 = reinterpret_cast<const byte*>(&Vertex1.VertexLight);
    const byte* c1 = reinterpret_cast<const byte*>(&Vertex2.VertexLight);
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

    resultColor[0] = Math::Lerp(linearColor1[0], linearColor2[0], Value);
    resultColor[1] = Math::Lerp(linearColor1[1], linearColor2[1], Value);
    resultColor[2] = Math::Lerp(linearColor1[2], linearColor2[2], Value);

    EncodeRGBE(r, resultColor);
#endif

    return Result;
}

struct MeshVertexSkin
{
    uint8_t JointIndices[4];
    uint8_t JointWeights[4];

    void Write(IBinaryStreamWriteInterface& Stream) const
    {
        Stream.Write(JointIndices, 8);
    }

    void Read(IBinaryStreamReadInterface& Stream)
    {
        Stream.Read(JointIndices, 8);
    }
};

HK_NAMESPACE_END
