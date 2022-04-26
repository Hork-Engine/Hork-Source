/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Core/Image.h>
#include <Core/Color.h>
#include <Containers/PodVector.h>
#include <Geometry/Quat.h>
#include <Geometry/BV/BvFrustum.h>

#include <RenderCore/Device.h>

//
// Common constants
//

/** Max skeleton joints */
constexpr int MAX_SKINNED_MESH_JOINTS = 256;

/** Max textures per material */
constexpr int MAX_MATERIAL_TEXTURES = 11; // Reserved texture slots for AOLookup, ClusterItemTBO, ClusterLookup, ShadowMapShadow, Lightmap

/** Max directional lights per view */
constexpr int MAX_DIRECTIONAL_LIGHTS = 4;

/** Max cascades per light */
constexpr int MAX_SHADOW_CASCADES = 4;

/** Max cascades per view */
constexpr int MAX_TOTAL_SHADOW_CASCADES_PER_VIEW = MAX_SHADOW_CASCADES * MAX_DIRECTIONAL_LIGHTS;

/** Frustum width */
constexpr int MAX_FRUSTUM_CLUSTERS_X = 16;

/** Frustum height */
constexpr int MAX_FRUSTUM_CLUSTERS_Y = 8;

/** Frustum depth */
constexpr int MAX_FRUSTUM_CLUSTERS_Z = 24;

/** Frustum projection matrix ZNear */
constexpr float FRUSTUM_CLUSTER_ZNEAR = 0.0125f;

/** Frustum projection matrix ZFar */
constexpr float FRUSTUM_CLUSTER_ZFAR = 512;

/** Frustum projection matrix ZRange */
constexpr float FRUSTUM_CLUSTER_ZRANGE = FRUSTUM_CLUSTER_ZFAR - FRUSTUM_CLUSTER_ZNEAR;

/** Width of single cluster */
constexpr float FRUSTUM_CLUSTER_WIDTH = 2.0f / MAX_FRUSTUM_CLUSTERS_X;

/** Height of single cluster */
constexpr float FRUSTUM_CLUSTER_HEIGHT = 2.0f / MAX_FRUSTUM_CLUSTERS_Y;

constexpr int FRUSTUM_SLICE_OFFSET = 20;

extern float FRUSTUM_SLICE_SCALE;

extern float FRUSTUM_SLICE_BIAS;

extern float FRUSTUM_SLICE_ZCLIP[MAX_FRUSTUM_CLUSTERS_Z + 1];

/** Max lights, Max decals, Max probes per cluster */
constexpr int MAX_CLUSTER_ITEMS = 256;

/** Max lights per cluster */
constexpr int MAX_CLUSTER_LIGHTS = MAX_CLUSTER_ITEMS;

/** Max decals per cluster */
constexpr int MAX_CLUSTER_DECALS = MAX_CLUSTER_ITEMS;

/** Max probes per cluster */
constexpr int MAX_CLUSTER_PROBES = MAX_CLUSTER_ITEMS;

constexpr int MAX_TOTAL_CLUSTER_ITEMS = 512 * 1024; // NOTE: must be power of two // TODO: подобрать оптимальный размер

/** Max lights per view. Indexed by 12 bit integer, limited by shader max constant buffer block size. */
constexpr int MAX_LIGHTS = 768; //1024

/** Max decals per view. Indexed by 12 bit integer. */
constexpr int MAX_DECALS = 1024;

/** Max probes per view. Indexed by 8 bit integer */
constexpr int MAX_PROBES = 256;

/** Total max items per view. */
constexpr int MAX_ITEMS = MAX_LIGHTS + MAX_DECALS + MAX_PROBES;


//
// Vertex formats
//

struct SMeshVertex
{
    Float3   Position;    // 4 * 3 = 12 bytes
    uint16_t TexCoord[2]; // 4 * 2 = 8 bytes      half: 4 bytes
    uint16_t Normal[3];   // 4 * 3 = 12 bytes     half: 6 bytes   byte: 3 bytes
    uint16_t Tangent[3];  // 4 * 3 = 12 bytes     half: 6 bytes   byte: 3 bytes
    int8_t   Handedness;  // 4 * 1 = 4 bytes      byte: 1 bytes
    uint8_t  Pad[3];

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        _Stream.WriteObject(Position);
        _Stream.WriteObject(GetTexCoord());
        _Stream.WriteObject(GetTangent());
        _Stream.WriteFloat((float)Handedness);
        _Stream.WriteObject(GetNormal());
    }

    void Read(IBinaryStreamReadInterface& _Stream)
    {
        Float2 texCoord;
        Float3 normal;
        Float3 tangent;
        _Stream.ReadObject(Position);
        _Stream.ReadObject(texCoord);
        _Stream.ReadObject(tangent);
        Handedness = (_Stream.ReadFloat() > 0.0f) ? 1 : -1;
        _Stream.ReadObject(normal);
        SetTexCoord(texCoord);
        SetNormal(normal);
        SetTangent(tangent);
    }

    void SetTexCoordNative(uint16_t S, uint16_t T)
    {
        TexCoord[0] = S;
        TexCoord[1] = T;
    }

    void SetTexCoord(float S, float T)
    {
        TexCoord[0] = Math::FloatToHalf(S);
        TexCoord[1] = Math::FloatToHalf(T);
    }

    void SetTexCoord(Float2 const& _TexCoord)
    {
        TexCoord[0] = Math::FloatToHalf(_TexCoord.X);
        TexCoord[1] = Math::FloatToHalf(_TexCoord.Y);
    }

    const Float2 GetTexCoord() const
    {
        return Float2(Math::HalfToFloat(TexCoord[0]), Math::HalfToFloat(TexCoord[1]));
    }

    void SetNormalNative(uint16_t X, uint16_t Y, uint16_t Z)
    {
        Normal[0] = X;
        Normal[1] = Y;
        Normal[2] = Z;
    }

    void SetNormal(float X, float Y, float Z)
    {
        Normal[0] = Math::FloatToHalf(X);
        Normal[1] = Math::FloatToHalf(Y);
        Normal[2] = Math::FloatToHalf(Z);
    }

    void SetNormal(Float3 const& _Normal)
    {
        Normal[0] = Math::FloatToHalf(_Normal.X);
        Normal[1] = Math::FloatToHalf(_Normal.Y);
        Normal[2] = Math::FloatToHalf(_Normal.Z);
    }

    const Float3 GetNormal() const
    {
        return Float3(Math::HalfToFloat(Normal[0]), Math::HalfToFloat(Normal[1]), Math::HalfToFloat(Normal[2]));
    }

    void SetTangentNative(uint16_t X, uint16_t Y, uint16_t Z)
    {
        Tangent[0] = X;
        Tangent[1] = Y;
        Tangent[2] = Z;
    }

    void SetTangent(float X, float Y, float Z)
    {
        Tangent[0] = Math::FloatToHalf(X);
        Tangent[1] = Math::FloatToHalf(Y);
        Tangent[2] = Math::FloatToHalf(Z);
    }

    void SetTangent(Float3 const& _Tangent)
    {
        Tangent[0] = Math::FloatToHalf(_Tangent.X);
        Tangent[1] = Math::FloatToHalf(_Tangent.Y);
        Tangent[2] = Math::FloatToHalf(_Tangent.Z);
    }

    const Float3 GetTangent() const
    {
        return Float3(Math::HalfToFloat(Tangent[0]), Math::HalfToFloat(Tangent[1]), Math::HalfToFloat(Tangent[2]));
    }

    static SMeshVertex Lerp(SMeshVertex const& _Vertex1, SMeshVertex const& _Vertex2, float _Value = 0.5f);
};

static_assert(sizeof(SMeshVertex) == 32, "Keep 32b vertex size");

HK_FORCEINLINE const SMeshVertex MakeMeshVertex(Float3 const& Position, Float2 const& TexCoord, Float3 const& Tangent, float Handedness, Float3 const& Normal)
{
    SMeshVertex v;
    v.Position = Position;
    v.SetTexCoord(TexCoord);
    v.SetNormal(Normal);
    v.SetTangent(Tangent);
    v.Handedness = Handedness > 0.0f ? 1 : -1;
    return v;
}

HK_FORCEINLINE SMeshVertex SMeshVertex::Lerp(SMeshVertex const& _Vertex1, SMeshVertex const& _Vertex2, float _Value)
{
    SMeshVertex Result;

    Result.Position = Math::Lerp(_Vertex1.Position, _Vertex2.Position, _Value);
    Result.SetTexCoord(Math::Lerp(_Vertex1.GetTexCoord(), _Vertex2.GetTexCoord(), _Value));
    Result.SetNormal(Math::Lerp(_Vertex1.GetNormal(), _Vertex2.GetNormal(), _Value).Normalized());
    Result.SetTangent(Math::Lerp(_Vertex1.GetTangent(), _Vertex2.GetTangent(), _Value).Normalized());
    Result.Handedness = _Value >= 0.5f ? _Vertex2.Handedness : _Vertex1.Handedness;

    return Result;
}

struct SMeshVertexUV
{
    Float2 TexCoord;

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        _Stream.WriteObject(TexCoord);
    }

    void Read(IBinaryStreamReadInterface& _Stream)
    {
        _Stream.ReadObject(TexCoord);
    }

    static SMeshVertexUV Lerp(SMeshVertexUV const& _Vertex1, SMeshVertexUV const& _Vertex2, float _Value = 0.5f);
};

HK_FORCEINLINE SMeshVertexUV SMeshVertexUV::Lerp(SMeshVertexUV const& _Vertex1, SMeshVertexUV const& _Vertex2, float _Value)
{
    SMeshVertexUV Result;

    Result.TexCoord = Math::Lerp(_Vertex1.TexCoord, _Vertex2.TexCoord, _Value);

    return Result;
}

struct SMeshVertexLight
{
    uint32_t VertexLight;

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        _Stream.WriteUInt32(VertexLight);
    }

    void Read(IBinaryStreamReadInterface& _Stream)
    {
        VertexLight = _Stream.ReadUInt32();
    }

    static SMeshVertexLight Lerp(SMeshVertexLight const& _Vertex1, SMeshVertexLight const& _Vertex2, float _Value = 0.5f);
};

HK_FORCEINLINE SMeshVertexLight SMeshVertexLight::Lerp(SMeshVertexLight const& _Vertex1, SMeshVertexLight const& _Vertex2, float _Value)
{
    SMeshVertexLight Result;

    const byte* c0 = reinterpret_cast<const byte*>(&_Vertex1.VertexLight);
    const byte* c1 = reinterpret_cast<const byte*>(&_Vertex2.VertexLight);
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

    resultColor[0] = Math::Lerp(linearColor1[0], linearColor2[0], _Value);
    resultColor[1] = Math::Lerp(linearColor1[1], linearColor2[1], _Value);
    resultColor[2] = Math::Lerp(linearColor1[2], linearColor2[2], _Value);

    EncodeRGBE(r, resultColor);
#endif

    return Result;
}

struct SMeshVertexSkin
{
    uint8_t JointIndices[4];
    uint8_t JointWeights[4];

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        _Stream.Write(JointIndices, 8);
    }

    void Read(IBinaryStreamReadInterface& _Stream)
    {
        _Stream.Read(JointIndices, 8);
    }
};

struct STerrainVertex
{
    /*unsigned*/ short X;
    /*unsigned*/ short Y;
};

struct SHUDDrawVert
{
    Float2   Position;
    Float2   TexCoord;
    uint32_t Color;
};

struct SDebugVertex
{
    Float3   Position;
    uint32_t Color;
};

//
// Texture formats
//

enum ENormalMapCompression
{
    NM_XYZ           = 0,
    NM_XY            = 1,
    NM_SPHEREMAP     = 2,
    NM_STEREOGRAPHIC = 3,
    NM_PARABOLOID    = 4,
    NM_QUARTIC       = 5,
    NM_FLOAT         = 6,
    NM_DXT5          = 7
};

enum ETextureColorSpace
{
    TEXTURE_COLORSPACE_RGBA,
    TEXTURE_COLORSPACE_SRGB_ALPHA,
    TEXTURE_COLORSPACE_YCOCG,
    TEXTURE_COLORSPACE_GRAYSCALED

    //TEXTURE_COLORSPACE_RGBA_INT
    //TEXTURE_COLORSPACE_RGBA_UINT
};

enum ETextureType
{
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_3D,
    TEXTURE_CUBEMAP,
    TEXTURE_CUBEMAP_ARRAY,
    TEXTURE_TYPE_MAX
};

enum ETextureFilter
{
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_MIPMAP_NEAREST,
    TEXTURE_FILTER_MIPMAP_BILINEAR,
    TEXTURE_FILTER_MIPMAP_NLINEAR,
    TEXTURE_FILTER_MIPMAP_TRILINEAR
};

enum ETextureAddress
{
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_MIRROR,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_BORDER,
    TEXTURE_ADDRESS_MIRROR_ONCE
};

struct STextureSampler
{
    ETextureType    TextureType;
    ETextureFilter  Filter;
    ETextureAddress AddressU;
    ETextureAddress AddressV;
    ETextureAddress AddressW;
    float           MipLODBias;
    float           Anisotropy;
    float           MinLod;
    float           MaxLod;
};

/**
Texture pixel format
*/
enum ETexturePixelFormat : uint8_t
{
    TEXTURE_PF_R8_SNORM,
    TEXTURE_PF_RG8_SNORM,
    //TEXTURE_PF_BGR8_SNORM,
    TEXTURE_PF_BGRA8_SNORM,

    TEXTURE_PF_R8_UNORM,
    TEXTURE_PF_RG8_UNORM,
    //TEXTURE_PF_BGR8_UNORM,
    TEXTURE_PF_BGRA8_UNORM,

    //TEXTURE_PF_BGR8_SRGB,
    TEXTURE_PF_BGRA8_SRGB,

    TEXTURE_PF_R16I,
    TEXTURE_PF_RG16I,
    //TEXTURE_PF_BGR16I,
    TEXTURE_PF_BGRA16I,

    TEXTURE_PF_R16UI,
    TEXTURE_PF_RG16UI,
    //TEXTURE_PF_BGR16UI,
    TEXTURE_PF_BGRA16UI,

    TEXTURE_PF_R32I,
    TEXTURE_PF_RG32I,
    TEXTURE_PF_BGR32I,
    TEXTURE_PF_BGRA32I,

    TEXTURE_PF_R32UI,
    TEXTURE_PF_RG32UI,
    TEXTURE_PF_BGR32UI,
    TEXTURE_PF_BGRA32UI,

    TEXTURE_PF_R16F,
    TEXTURE_PF_RG16F,
    //TEXTURE_PF_BGR16F,
    TEXTURE_PF_BGRA16F,

    TEXTURE_PF_R32F,
    TEXTURE_PF_RG32F,
    TEXTURE_PF_BGR32F,
    TEXTURE_PF_BGRA32F,

    TEXTURE_PF_R11F_G11F_B10F,

    // Compressed formats

    // RGB
    TEXTURE_PF_COMPRESSED_BC1_RGB,
    TEXTURE_PF_COMPRESSED_BC1_SRGB,

    // RGB A-4bit / RGB (not the best quality, it is better to use BC3)
    TEXTURE_PF_COMPRESSED_BC2_RGBA,
    TEXTURE_PF_COMPRESSED_BC2_SRGB_ALPHA,

    // RGB A-8bit
    TEXTURE_PF_COMPRESSED_BC3_RGBA,
    TEXTURE_PF_COMPRESSED_BC3_SRGB_ALPHA,

    // R single channel texture (use for metalmap, glossmap, etc)
    TEXTURE_PF_COMPRESSED_BC4_R,
    TEXTURE_PF_COMPRESSED_BC4_R_SIGNED,

    // RG two channel texture (use for normal map or two grayscale maps)
    TEXTURE_PF_COMPRESSED_BC5_RG,
    TEXTURE_PF_COMPRESSED_BC5_RG_SIGNED,

    // RGB half float HDR
    TEXTURE_PF_COMPRESSED_BC6H,
    TEXTURE_PF_COMPRESSED_BC6H_SIGNED,

    // RGB[A], best quality, every block is compressed different
    TEXTURE_PF_COMPRESSED_BC7_RGBA,
    TEXTURE_PF_COMPRESSED_BC7_SRGB_ALPHA,

    TEXTURE_PF_MAX
};

RenderCore::DATA_FORMAT GetTextureDataFormat(ETexturePixelFormat PixelFormat);

RenderCore::TEXTURE_FORMAT GetTextureFormat(ETexturePixelFormat PixelFormat);

struct STexturePixelFormat
{
    ETexturePixelFormat Data;

    STexturePixelFormat() :
        Data(TEXTURE_PF_BGRA8_SRGB) {}
    STexturePixelFormat(ETexturePixelFormat _PixelFormat) :
        Data(_PixelFormat) {}

    void operator=(ETexturePixelFormat _PixelFormat) { Data = _PixelFormat; }

    bool operator==(ETexturePixelFormat _PixelFormat) const { return Data == _PixelFormat; }
    bool operator==(STexturePixelFormat _PixelFormat) const { return Data == _PixelFormat.Data; }
    bool operator!=(ETexturePixelFormat _PixelFormat) const { return Data != _PixelFormat; }
    bool operator!=(STexturePixelFormat _PixelFormat) const { return Data != _PixelFormat.Data; }

    bool IsCompressed() const;

    bool IsSRGB() const;

    int SizeInBytesUncompressed() const;

    int BlockSizeCompressed() const;

    int NumComponents() const;

    void Read(IBinaryStreamReadInterface& _Stream)
    {
        Data = (ETexturePixelFormat)_Stream.ReadUInt8();
    }

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        _Stream.WriteUInt8((uint8_t)Data);
    }

    RenderCore::DATA_FORMAT GetTextureDataFormat() const
    {
        return ::GetTextureDataFormat(Data);
    }

    RenderCore::TEXTURE_FORMAT GetTextureFormat() const
    {
        return ::GetTextureFormat(Data);
    }

    static bool GetAppropriatePixelFormat(EImagePixelFormat const& _ImagePixelFormat, STexturePixelFormat& _PixelFormat);
};

#if 0
enum ETextureGroup
{

    //
    // Albedo color map
    //

    Color,
    Color_sRGB_16bit,
    Color_Compressed,           // BC4, BC5, BC3
    Color_Compressed_BC7,
    Color_Compressed_YCoCg,     // DXT5 (BC3)
    //Color_Compressed_BC1,
    //Color_Compressed_BC1a,

    //
    // Normal maps (linear)
    //

    NormalMap_XY_8bit,         // x 8bit, y 8bit
    NormalMap_XYZ_8bit,        // x 8bit, y 8bit, z 8bit
    NormalMap_SphereMap_8bit,  // x 8bit, y 8bit sphere mapped
    NormalMap_Float16,         // x float16, y float16
    NormalMap_Float32,         // x float32, y float32
    NormalMap_Compressed_BC1,
    NormalMap_Compressed_BC5_Orthographic,       // BC5 compressed x, y
    NormalMap_Compressed_BC5_Stereographic,       // BC5 compressed x, y
    NormalMap_Compressed_BC5_Paraboloid,       // BC5 compressed x, y
    NormalMap_Compressed_BC5_Quartic,       // BC5 compressed x, y
    NormalMap_Compressed_DXT5,      // same as BC3n?, TODO: compare my compressor with NVidia compressor

    //
    // Linear single channel grayscaled images like Metallic, Roughness
    //

    Grayscaled,
    Grayscaled_Compressed_BC4,

    //
    // High dynamic range images (linear)
    //

    HDRI_Grayscaled_Compressed_BC6H,
    HDRI_16,
    HDRI_32,
    HDRI_Compressed_BC6H

    //
    // Integer maps (not supported yet)
    //

    //SignedInteger,        // Unnormalized, uncompressed R,RG,RGB,RGBA/8,16,32
    //UnsignedInteger,      // Unnormalized, uncompressed R,RG,RGB,RGBA/8,16,32

};
#endif

//
// Material
//

enum EMaterialType
{
    MATERIAL_TYPE_UNLIT,
    MATERIAL_TYPE_BASELIGHT,
    MATERIAL_TYPE_PBR,
    MATERIAL_TYPE_HUD,
    MATERIAL_TYPE_POSTPROCESS
};

enum EMaterialDepthHack
{
    MATERIAL_DEPTH_HACK_NONE,
    MATERIAL_DEPTH_HACK_WEAPON,
    MATERIAL_DEPTH_HACK_SKYBOX
};

enum EColorBlending
{
    COLOR_BLENDING_ALPHA,
    COLOR_BLENDING_DISABLED,
    COLOR_BLENDING_PREMULTIPLIED_ALPHA,
    COLOR_BLENDING_COLOR_ADD,
    COLOR_BLENDING_MULTIPLY,
    COLOR_BLENDING_SOURCE_TO_DEST,
    COLOR_BLENDING_ADD_MUL,
    COLOR_BLENDING_ADD_ALPHA,

    COLOR_BLENDING_MAX
};

enum ETessellationMethod : uint8_t
{
    TESSELLATION_DISABLED,
    TESSELLATION_FLAT,
    TESSELLATION_PN
};

/** Mixed material and geometry rendering priorities.
Combine them in that way: material_priority | geometry_priority */
enum ERenderingPriority : uint8_t
{
    /** Weapon rendered first */
    RENDERING_PRIORITY_WEAPON = 0 << 4,

    /** Default priority */
    RENDERING_PRIORITY_DEFAULT = 1 << 4,

    RENDERING_PRIORITY_RESERVED2  = 2 << 4,
    RENDERING_PRIORITY_RESERVED3  = 3 << 4,
    RENDERING_PRIORITY_RESERVED4  = 4 << 4,
    RENDERING_PRIORITY_RESERVED5  = 5 << 4,
    RENDERING_PRIORITY_RESERVED6  = 6 << 4,
    RENDERING_PRIORITY_RESERVED7  = 7 << 4,
    RENDERING_PRIORITY_RESERVED8  = 8 << 4,
    RENDERING_PRIORITY_RESERVED9  = 9 << 4,
    RENDERING_PRIORITY_RESERVED10 = 10 << 4,
    RENDERING_PRIORITY_RESERVED11 = 11 << 4,
    RENDERING_PRIORITY_RESERVED12 = 12 << 4,
    RENDERING_PRIORITY_RESERVED13 = 13 << 4,
    RENDERING_PRIORITY_RESERVED14 = 14 << 4,

    /** Skybox rendered last */
    RENDERING_PRIORITY_SKYBOX = 15 << 4,

    /** Static geometry */
    RENDERING_GEOMETRY_PRIORITY_STATIC = 0,

    /** Static geometry */
    RENDERING_GEOMETRY_PRIORITY_DYNAMIC = 1,

    RENDERING_GEOMETRY_PRIORITY_RESERVED2  = 2,
    RENDERING_GEOMETRY_PRIORITY_RESERVED3  = 3,
    RENDERING_GEOMETRY_PRIORITY_RESERVED4  = 4,
    RENDERING_GEOMETRY_PRIORITY_RESERVED5  = 5,
    RENDERING_GEOMETRY_PRIORITY_RESERVED6  = 6,
    RENDERING_GEOMETRY_PRIORITY_RESERVED7  = 7,
    RENDERING_GEOMETRY_PRIORITY_RESERVED8  = 8,
    RENDERING_GEOMETRY_PRIORITY_RESERVED9  = 9,
    RENDERING_GEOMETRY_PRIORITY_RESERVED10 = 10,
    RENDERING_GEOMETRY_PRIORITY_RESERVED11 = 11,
    RENDERING_GEOMETRY_PRIORITY_RESERVED12 = 12,
    RENDERING_GEOMETRY_PRIORITY_RESERVED13 = 13,
    RENDERING_GEOMETRY_PRIORITY_RESERVED14 = 14,
    RENDERING_GEOMETRY_PRIORITY_RESERVED15 = 15
};

struct SPredefinedShaderSource
{
    /** Pointer to next source */
    SPredefinedShaderSource* pNext;

    /** The source name */
    char* SourceName;

    /** Source code */
    char* Code;
};

struct SMaterialDef
{
    /** Material type (Unlit,baselight,pbr,etc) */
    EMaterialType Type;

    /** Blending mode (FIXME: only for UNLIT materials?) */
    EColorBlending Blending;

    ETessellationMethod TessellationMethod;

    ERenderingPriority RenderingPriority;

    /** Lightmap binding unit */
    int LightmapSlot;

    /** Texture binding count for different passes. This allow renderer to optimize sampler/texture bindings
    during rendering. */
    int DepthPassTextureCount;
    int LightPassTextureCount;
    int WireframePassTextureCount;
    int NormalsPassTextureCount;
    int ShadowMapPassTextureCount;

    /** Have vertex deformation in vertex stage. This flag allow renderer to optimize pipeline switching
    during rendering. */
    bool bHasVertexDeform : 1;

    /** Experimental. Depth testing. */
    bool bDepthTest_EXPERIMENTAL : 1;

    /** Disable shadow casting (for specific materials like skybox or first person shooter weapon) */
    bool bNoCastShadow : 1;

    /** Enable alpha masking */
    bool bAlphaMasking : 1;

    /** Enable shadow map masking */
    bool bShadowMapMasking : 1;

    /** Use tessellation for shadow maps */
    bool bDisplacementAffectShadow : 1;

    /** Apply fake shadows. Used with parallax technique */
    //bool bParallaxMappingSelfShadowing : 1;

    /** Translusent materials with alpha test */
    bool bTranslucent : 1;

    /** Disable backface culling */
    bool bTwoSided : 1;

    int NumUniformVectors;

    /** Material samplers */
    STextureSampler Samplers[MAX_MATERIAL_TEXTURES];
    int             NumSamplers;

    /** Material shaders */
    SPredefinedShaderSource* Shaders;

    SMaterialDef()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }

    ~SMaterialDef()
    {
        RemoveShaders();
    }

    void AddShader(AStringView SourceName, AStringView SourceCode);

    void RemoveShaders();
};

class AMaterialGPU
{
public:
    EMaterialType MaterialType;

    int LightmapSlot;

    int DepthPassTextureCount;
    int LightPassTextureCount;
    int WireframePassTextureCount;
    int NormalsPassTextureCount;
    int ShadowMapPassTextureCount;

    TRef<RenderCore::IPipeline> DepthPass[2];
    TRef<RenderCore::IPipeline> DepthVelocityPass[2];
    TRef<RenderCore::IPipeline> WireframePass[2];
    TRef<RenderCore::IPipeline> NormalsPass[2];
    TRef<RenderCore::IPipeline> LightPass[2];
    TRef<RenderCore::IPipeline> LightPassLightmap;
    TRef<RenderCore::IPipeline> LightPassVertexLight;
    TRef<RenderCore::IPipeline> ShadowPass[2];
    TRef<RenderCore::IPipeline> OmniShadowPass[2];
    TRef<RenderCore::IPipeline> FeedbackPass[2];
    TRef<RenderCore::IPipeline> OutlinePass[2];
    TRef<RenderCore::IPipeline> HUDPipeline;
};

struct SMaterialFrameData
{
    AMaterialGPU*                  Material;
    RenderCore::ITexture*          Textures[MAX_MATERIAL_TEXTURES];
    int                            NumTextures;
    Float4                         UniformVectors[4];
    int                            NumUniformVectors;
    class AVirtualTextureResource* VirtualTexture;
};


//
// Debug draw
//

enum EDebugDrawCmd
{
    DBG_DRAW_CMD_POINTS,
    DBG_DRAW_CMD_POINTS_DEPTH_TEST,
    DBG_DRAW_CMD_LINES,
    DBG_DRAW_CMD_LINES_DEPTH_TEST,
    DBG_DRAW_CMD_TRIANGLE_SOUP,
    DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST,

    DBG_DRAW_CMD_MAX,

    DBG_DRAW_CMD_NOP
};

struct SDebugDrawCmd
{
    EDebugDrawCmd Type;
    int           FirstVertex;
    int           NumVertices;
    int           FirstIndex;
    int           NumIndices;
};

//
// HUD
//

enum EHUDDrawCmd
{
    HUD_DRAW_CMD_ALPHA,    // fonts, primitves, textures with one alpha channel
    HUD_DRAW_CMD_TEXTURE,  // textures
    HUD_DRAW_CMD_MATERIAL, // material instances (MATERIAL_TYPE_HUD)
    HUD_DRAW_CMD_VIEWPORT, // viewports

    HUD_DRAW_CMD_MAX
};

enum EHUDSamplerType
{
    HUD_SAMPLER_TILED_LINEAR,
    HUD_SAMPLER_TILED_NEAREST,

    HUD_SAMPLER_MIRROR_LINEAR,
    HUD_SAMPLER_MIRROR_NEAREST,

    HUD_SAMPLER_CLAMPED_LINEAR,
    HUD_SAMPLER_CLAMPED_NEAREST,

    HUD_SAMPLER_BORDER_LINEAR,
    HUD_SAMPLER_BORDER_NEAREST,

    HUD_SAMPLER_MIRROR_ONCE_LINEAR,
    HUD_SAMPLER_MIRROR_ONCE_NEAREST,

    HUD_SAMPLER_MAX
};

struct SHUDDrawCmd
{
    unsigned int    IndexCount;
    unsigned int    StartIndexLocation;
    unsigned int    BaseVertexLocation;
    Float2          ClipMins;
    Float2          ClipMaxs;
    EHUDDrawCmd     Type;
    EColorBlending  Blending;    // only for type DRAW_CMD_TEXTURE and DRAW_CMD_VIEWPORT
    EHUDSamplerType SamplerType; // only for type DRAW_CMD_TEXTURE

    union
    {
        RenderCore::ITexture* Texture;           // HUD_DRAW_CMD_TEXTURE, HUD_DRAW_CMD_ALPHA
        SMaterialFrameData*   MaterialFrameData; // HUD_DRAW_CMD_MATERIAL
        int                   ViewportIndex;     // HUD_DRAW_CMD_VIEWPORT
    };
};

struct SHUDDrawList
{
    size_t        VertexStreamOffset;
    size_t        IndexStreamOffset;
    int           CommandsCount;
    SHUDDrawCmd*  Commands;
    SHUDDrawList* pNext;
};


/**

Directional light render instance

*/
struct SDirectionalLightInstance
{
    Float4   ColorAndAmbientIntensity;
    Float3x3 Matrix; // Light rotation matrix
    int      RenderMask;
    int      MaxShadowCascades; // Max allowed cascades for light
    int      ShadowmapIndex;
    int      ShadowCascadeResolution;
    int      FirstCascade;         // First cascade offset
    int      NumCascades;          // Current visible cascades count for light
    size_t   ViewProjStreamHandle; // Transform from world space to light view projection for each cascade
};


/**

Render instance (opaque & translucent meshes)

*/
struct SRenderInstance
{
    AMaterialGPU*       Material;
    SMaterialFrameData* MaterialInstance;

    RenderCore::IBuffer* VertexBuffer;
    size_t               VertexBufferOffset;

    RenderCore::IBuffer* IndexBuffer;
    size_t               IndexBufferOffset;

    RenderCore::IBuffer* WeightsBuffer;
    size_t               WeightsBufferOffset;

    RenderCore::IBuffer* VertexLightChannel;
    size_t               VertexLightOffset;

    RenderCore::IBuffer* LightmapUVChannel;
    size_t               LightmapUVOffset;

    RenderCore::ITexture* Lightmap;
    Float4                LightmapOffset;

    Float4x4 Matrix;
    Float4x4 MatrixP;

    Float3x3 ModelNormalToViewSpace;

    size_t SkeletonOffset;
    size_t SkeletonOffsetMB;
    size_t SkeletonSize;

    unsigned int IndexCount;
    unsigned int StartIndexLocation;
    int          BaseVertexLocation;

    uint64_t SortKey;

    uint8_t GetRenderingPriority() const
    {
        return (SortKey >> 56) & 0xf0;
    }

    uint8_t GetGeometryPriority() const
    {
        return (SortKey >> 56) & 0x0f;
    }

    void GenerateSortKey(uint8_t Priority, uint64_t Mesh)
    {
        // NOTE: 8 bits are still unused. We can use it in future.
        SortKey = ((uint64_t)(Priority) << 56u) | ((uint64_t)(Core::MurMur3Hash64((uint64_t)Material) & 0xffffu) << 40u) | ((uint64_t)(Core::MurMur3Hash64((uint64_t)MaterialInstance) & 0xffffu) << 24u) | ((uint64_t)(Core::MurMur3Hash64(Mesh) & 0xffffu) << 8u);
    }
};


/**

Shadowmap render instance

*/
struct SShadowRenderInstance
{
    AMaterialGPU*        Material;
    SMaterialFrameData*  MaterialInstance;
    RenderCore::IBuffer* VertexBuffer;
    size_t               VertexBufferOffset;
    RenderCore::IBuffer* IndexBuffer;
    size_t               IndexBufferOffset;
    RenderCore::IBuffer* WeightsBuffer;
    size_t               WeightsBufferOffset;
    Float3x4             WorldTransformMatrix;
    size_t               SkeletonOffset;
    size_t               SkeletonSize;
    unsigned int         IndexCount;
    unsigned int         StartIndexLocation;
    int                  BaseVertexLocation;
    uint16_t             CascadeMask; // Cascade mask for directional lights or face index for point/spot lights
    uint64_t             SortKey;

    void GenerateSortKey(uint8_t Priority, uint64_t Mesh)
    {
        // NOTE: 8 bits are still unused. We can use it in future.
        SortKey = ((uint64_t)(Priority) << 56u) | ((uint64_t)(Core::MurMur3Hash64((uint64_t)Material) & 0xffffu) << 40u) | ((uint64_t)(Core::MurMur3Hash64((uint64_t)MaterialInstance) & 0xffffu) << 24u) | ((uint64_t)(Core::MurMur3Hash64(Mesh) & 0xffffu) << 8u);
    }
};


/**

Light portal render instance

*/
struct SLightPortalRenderInstance
{
    RenderCore::IBuffer* VertexBuffer;
    size_t               VertexBufferOffset;
    RenderCore::IBuffer* IndexBuffer;
    size_t               IndexBufferOffset;
    unsigned int         IndexCount;
    unsigned int         StartIndexLocation;
    int                  BaseVertexLocation;
};


/**

Shadowmap definition

*/
struct SLightShadowmap
{
    int FirstShadowInstance;
    int ShadowInstanceCount;

    int FirstLightPortal;
    int LightPortalsCount;

    Float3 LightPosition;
};


/**

Cluster header

uvec2 header = texelFetch( ClusterLookup, TexCoord ).xy;
int FirstPackedIndex = header.x;
int NumProbes = header.y & 0xff;
int NumDecals = ( header.y >> 8 ) & 0xff;
int NumLights = ( header.y >> 16 ) & 0xff;
int Pad0 = ( header.y >> 24 ) & 0xff // can be used in future

texture3d RG32UI

*/
struct SClusterHeader
{
    uint32_t FirstPackedIndex;
    uint8_t  NumProbes;
    uint8_t  NumDecals;
    uint8_t  NumLights;
    uint8_t  Pad0;
};


/**

Packed light, decal and probe index:

Read indices in shader:
    uint packedIndex = (uint)(texelFetch( ItemList, Offset.X ).x);

Unpack indices:
    int LightIndex = packedIndex & 0x3ff;
    int DecalIndex = ( packedIndex >> 12 ) & 0x3ff;
    int ProbeIndex = packedIndex >> 24;

texture1d R32UI

*/
struct SClusterPackedIndex
{
    uint32_t Indices;
};


/**

Light type (point/spot)

*/
enum EClusterLightType
{
    CLUSTER_LIGHT_POINT,
    CLUSTER_LIGHT_SPOT,
};


/**

Point & spot light shader parameters

*/
struct SLightParameters
{
    Float3 Position;
    float  Radius;

    float CosHalfOuterConeAngle;
    float CosHalfInnerConeAngle;
    float InverseSquareRadius; // 1 / (Radius*Radius)
    float Pad1;

    Float3 Direction;    // For spot and photometric lights
    float  SpotExponent; // For spot lights

    Float3 Color; // Light color
    float  Pad2;

    unsigned int LightType;
    unsigned int RenderMask;
    unsigned int PhotometricProfile;
    int          ShadowmapIndex;
};


/**

Reflection probe shader parameters

*/
struct SProbeParameters
{
    Float3 Position;
    float  Radius;

    uint64_t IrradianceMap;
    uint64_t ReflectionMap;
};


/**

Terrain patch parameters

*/
struct STerrainPatchInstance
{
    Int2   VertexScale;
    Int2   VertexTranslate;
    Int2   TexcoordOffset;
    Color4 QuadColor; // Just for debug. Will be removed later
};


/**

Terrain render instance

*/
struct STerrainRenderInstance
{
    RenderCore::IBuffer*  VertexBuffer;
    RenderCore::IBuffer*  IndexBuffer;
    size_t                InstanceBufferStreamHandle;
    size_t                IndirectBufferStreamHandle;
    int                   IndirectBufferDrawCount;
    RenderCore::ITexture* Clipmaps;
    RenderCore::ITexture* Normals;
    Float4                ViewPositionAndHeight;
    Float4x4              LocalViewProjection;
    Float3x3              ModelNormalToViewSpace;
    Int2                  ClipMin;
    Int2                  ClipMax;
};


/**

Rendering data for one view
Keep it POD

*/
struct SRenderView
{
    /** Current view index */
    int ViewIndex;

    /** Viewport size (scaled by dynamic resolution) */
    int Width;
    /** Viewport size (scaled by dynamic resolution) */
    int Height;

    /** Viewport size on previous frame (scaled by dynamic resolution) */
    int WidthP;
    /** Viewport size on previous frame (scaled by dynamic resolution) */
    int HeightP;

    /** Real viewport size */
    int WidthR;
    /** Real viewport size */
    int HeightR;

    /** Time parameters */
    float GameRunningTimeSeconds;
    float GameplayTimeSeconds;
    float GameplayTimeStep;

    /** View parameters */
    Float3   ViewPosition;
    Quat     ViewRotation;
    Float3   ViewRightVec;
    Float3   ViewUpVec;
    Float3   ViewDir;
    Float4x4 ViewMatrix;
    Float4x4 ViewMatrixP;
    float    ViewZNear;
    float    ViewZFar;
    float    ViewFovX;
    float    ViewFovY;
    Float2   ViewOrthoMins;
    Float2   ViewOrthoMaxs;
    Float3x3 NormalToViewMatrix;
    Float4x4 ProjectionMatrix;
    Float4x4 ProjectionMatrixP;
    Float4x4 InverseProjectionMatrix;
    Float4x4 ViewProjection;
    Float4x4 ViewProjectionP;
    Float4x4 ViewSpaceToWorldSpace;
    Float4x4 ClipSpaceToWorldSpace;
    Float4x4 ClusterProjectionMatrix;
    Float4x4 ClusterViewProjection;
    Float4x4 ClusterViewProjectionInversed;
    Float3   BackgroundColor;
    bool     bClearBackground;
    bool     bWireframe;
    bool     bPerspective;
    bool     bPadding1;

    /** Farthest distance to geometry in view */
    float MaxVisibleDistance;

    /** Vignette parameters */
    Float4 VignetteColorIntensity;
    float  VignetteOuterRadiusSqr;
    float  VignetteInnerRadiusSqr;

    /** Source color grading texture */
    RenderCore::ITexture* ColorGradingLUT;
    /** Current color grading texture */
    RenderCore::ITexture* CurrentColorGradingLUT;

    /** Blending speed between current and source color grading textures */
    float ColorGradingAdaptationSpeed;

    /** Procedural color grading */
    Float3 ColorGradingGrain;
    Float3 ColorGradingGamma;
    Float3 ColorGradingLift;
    Float3 ColorGradingPresaturation;
    Float3 ColorGradingTemperatureScale;
    Float3 ColorGradingTemperatureStrength;
    float  ColorGradingBrightnessNormalization;

    /** Current exposure texture */
    RenderCore::ITexture* CurrentExposure;

    /** Light photometric lookup map (IES) */
    RenderCore::ITexture* PhotometricProfiles;

    /** Texture with light data */
    RenderCore::ITexture* LightTexture;

    /** Texture with depth data */
    RenderCore::ITexture* DepthTexture;

    /** Virtual texture feedback data (experimental) */
    class AVirtualTextureFeedback* VTFeedback;

    /** Total cascades for all shadow maps in view */
    int NumShadowMapCascades;
    /** Total shadow maps in view */
    int NumCascadedShadowMaps;

    /** Opaque geometry */
    int FirstInstance;
    int InstanceCount;

    /** Translucent geometry */
    int FirstTranslucentInstance;
    int TranslucentInstanceCount;

    /** Outlined geometry */
    int FirstOutlineInstance;
    int OutlineInstanceCount;

    /** Directional lights */
    int FirstDirectionalLight;
    int NumDirectionalLights;

    /** Debug draw commands */
    int FirstDebugDrawCommand;
    int DebugDrawCommandCount;

    /** Transform from view clip space to texture space */
    Float4x4* ShadowMapMatrices;
    size_t    ShadowMapMatricesStreamHandle;

    /** Point and spot lights for render view */
    SLightParameters* PointLights;
    int               NumPointLights;
    size_t            PointLightsStreamHandle;
    size_t            PointLightsStreamSize;

    int FirstOmnidirectionalShadowMap;
    int NumOmnidirectionalShadowMaps;

    /** Reflection probes for render view */
    SProbeParameters* Probes;
    int               NumProbes;
    size_t            ProbeStreamHandle;
    size_t            ProbeStreamSize;

    /** Cluster headers */
    SClusterHeader* ClusterLookup;
    size_t          ClusterLookupStreamHandle;

    /** Cluster packed indices */
    SClusterPackedIndex* ClusterPackedIndices;
    size_t               ClusterPackedIndicesStreamHandle;
    int                  ClusterPackedIndexCount;

    /** Terrain instances */
    int FirstTerrainInstance;
    int TerrainInstanceCount;

    /** Global reflection & irradiance */
    uint64_t GlobalIrradianceMap;
    uint64_t GlobalReflectionMap;
};


/**

Rendering data for one frame

*/
struct SRenderFrame
{
    /** Game tick */
    int FrameNumber;

    /** Render target max resolution */
    int RenderTargetMaxWidth;
    /** Render target max resolution */
    int RenderTargetMaxHeight;
    /** Render target max resolution at prev frame */
    int RenderTargetMaxWidthP;
    /** Render target max resolution at prev frame */
    int RenderTargetMaxHeightP;

    /** Canvas resolution */
    int CanvasWidth;
    /** Canvas resolution */
    int CanvasHeight;
    /** Canvas projection matrix */
    Float4x4 CanvasOrthoProjection;

    /** Render views */
    SRenderView* RenderViews;
    /** Render view count */
    int NumViews;

    /** Opaque instances */
    TPodVector<SRenderInstance*, 1024> Instances;
    /** Translucent instances */
    TPodVector<SRenderInstance*, 1024> TranslucentInstances;
    /** Outline instances */
    TPodVector<SRenderInstance*> OutlineInstances;
    /** Shadowmap instances */
    TPodVector<SShadowRenderInstance*, 1024> ShadowInstances;
    /** Light portal instances */
    TPodVector<SLightPortalRenderInstance*, 1024> LightPortals;
    /** Directional light instances */
    TPodVector<SDirectionalLightInstance*> DirectionalLights;
    /** Shadow maps */
    TPodVector<SLightShadowmap> LightShadowmaps;
    /** Terrain instances */
    TPodVector<STerrainRenderInstance*> TerrainInstances;

    /** Hud draw commands */
    SHUDDrawList* DrawListHead;
    SHUDDrawList* DrawListTail;

    /** Debug draw commands */
    SDebugDrawCmd const* DbgCmds;
    size_t               DbgVertexStreamOffset;
    size_t               DbgIndexStreamOffset;
};
