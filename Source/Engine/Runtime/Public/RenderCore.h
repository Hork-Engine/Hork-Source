/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Core/Public/CoreMath.h>
#include <Core/Public/PodArray.h>

//
// Common constants
//

/** Max render views per frame */
constexpr int MAX_RENDER_VIEWS                      = 16;

/** Max skeleton joints */
constexpr int MAX_SKINNED_MESH_JOINTS               = 256;

/** Max skinned meshes per frame */
constexpr int MAX_SKINNED_MESH_INSTANCES_PER_FRAME  = 256;

/** Max textures per material */
constexpr int MAX_MATERIAL_TEXTURES                 = 12; // 3 textures reserved for lightmap, light clusterlookup, light cluster items, shadow map

/** Max cascades per light */
constexpr int MAX_SHADOW_CASCADES = 4;

/** Max directional lights per frame */
constexpr int MAX_DIRECTIONAL_LIGHTS = 4;

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

/** Max lights per frame. Indexed by 12 bit integer, limited by shader max uniform buffer size. */
constexpr int MAX_LIGHTS = 768;//1024

/** Max decals per frame. Indexed by 12 bit integer. */
constexpr int MAX_DECALS = 1024;

/** Max probes per frame. Indexed by 8 bit integer */
constexpr int MAX_PROBES = 256;

/** Total max items per frame. */
constexpr int MAX_ITEMS = MAX_LIGHTS + MAX_DECALS + MAX_PROBES;


//
// Vertex formats
//

struct SMeshVertex {
    // TODO: Pack to 32 byte length!

    Float3 Position;
    Float2 TexCoord;
    Float3 Tangent;
    float  Handedness;
    Float3 Normal;

    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteObject( Position );
        _Stream.WriteObject( TexCoord );
        _Stream.WriteObject( Tangent );
        _Stream.WriteFloat( Handedness );
        _Stream.WriteObject( Normal );
    }

    void Read( IStreamBase & _Stream ) {
        _Stream.ReadObject( Position );
        _Stream.ReadObject( TexCoord );
        _Stream.ReadObject( Tangent );
        Handedness = _Stream.ReadFloat();
        _Stream.ReadObject( Normal );
    }

    static SMeshVertex Lerp( SMeshVertex const & _Vertex1, SMeshVertex const & _Vertex2, float _Value = 0.5f );
};

AN_FORCEINLINE SMeshVertex SMeshVertex::Lerp( SMeshVertex const & _Vertex1, SMeshVertex const & _Vertex2, float _Value ) {
    SMeshVertex Result;

    Result.Position   = Math::Lerp( _Vertex1.Position, _Vertex2.Position, _Value );
    Result.TexCoord   = Math::Lerp( _Vertex1.TexCoord, _Vertex2.TexCoord, _Value );
    Result.Tangent    = Math::Lerp( _Vertex1.Tangent, _Vertex2.Tangent, _Value ).Normalized();
    Result.Handedness = _Value >= 0.5f ? _Vertex2.Handedness : _Vertex1.Handedness;
    Result.Normal     = Math::Lerp( _Vertex1.Normal, _Vertex2.Normal, _Value ).Normalized();

    return Result;
}

struct SMeshVertexUV {
    Float2 TexCoord;

    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteObject( TexCoord );
    }

    void Read( IStreamBase & _Stream ) {
        _Stream.ReadObject( TexCoord );
    }

    static SMeshVertexUV Lerp( SMeshVertexUV const & _Vertex1, SMeshVertexUV const & _Vertex2, float _Value = 0.5f );
};

AN_FORCEINLINE SMeshVertexUV SMeshVertexUV::Lerp( SMeshVertexUV const & _Vertex1, SMeshVertexUV const & _Vertex2, float _Value ) {
    SMeshVertexUV Result;

    Result.TexCoord   = Math::Lerp( _Vertex1.TexCoord, _Vertex2.TexCoord, _Value );

    return Result;
}

struct SMeshVertexLight {
    uint32_t VertexLight;

    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteUInt32( VertexLight );
    }

    void Read( IStreamBase & _Stream ) {
        VertexLight = _Stream.ReadUInt32();
    }

    static SMeshVertexLight Lerp( SMeshVertexLight const & _Vertex1, SMeshVertexLight const & _Vertex2, float _Value = 0.5f );
};

AN_FORCEINLINE SMeshVertexLight SMeshVertexLight::Lerp( SMeshVertexLight const & _Vertex1, SMeshVertexLight const & _Vertex2, float _Value ) {
    SMeshVertexLight Result;

    const byte * c0 = reinterpret_cast< const byte * >( &_Vertex1.VertexLight );
    const byte * c1 = reinterpret_cast< const byte * >( &_Vertex2.VertexLight );
    byte * r = reinterpret_cast< byte * >( &Result.VertexLight );

    r[0] = ( c0[0] + c1[0] ) >> 1;
    r[1] = ( c0[1] + c1[1] ) >> 1;
    r[2] = ( c0[2] + c1[2] ) >> 1;
    r[3] = ( c0[3] + c1[3] ) >> 1;

    return Result;
}

struct SMeshVertexSkin {
    uint8_t JointIndices[4];
    uint8_t JointWeights[4];

    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteBuffer( JointIndices, 8 );
    }

    void Read( IStreamBase & _Stream ) {
        _Stream.ReadBuffer( JointIndices, 8 );
    }
};

struct SHUDDrawVert {
    Float2   Position;
    Float2   TexCoord;
    uint32_t Color;
};

struct SDebugVertex {
    Float3 Position;
    uint32_t Color;
};

//
// Texture formats
//

enum ENormalMapCompression {
    NM_XYZ              = 0,
    NM_XY               = 1,
    NM_SPHEREMAP        = 2,
    NM_STEREOGRAPHIC    = 3,
    NM_PARABOLOID       = 4,
    NM_QUARTIC          = 5,
    NM_FLOAT            = 6,
    NM_DXT5             = 7
};

enum ETextureColorSpace {
    TEXTURE_COLORSPACE_RGBA,
    TEXTURE_COLORSPACE_SRGB_ALPHA,
    TEXTURE_COLORSPACE_YCOCG,
    TEXTURE_COLORSPACE_GRAYSCALED

    //TEXTURE_COLORSPACE_RGBA_INT
    //TEXTURE_COLORSPACE_RGBA_UINT
};

enum ETextureType {
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_3D,
    TEXTURE_CUBEMAP,
    TEXTURE_CUBEMAP_ARRAY,
    TEXTURE_2DNPOT,
    TEXTURE_TYPE_MAX
};

enum ETextureFilter {
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_MIPMAP_NEAREST,
    TEXTURE_FILTER_MIPMAP_BILINEAR,
    TEXTURE_FILTER_MIPMAP_NLINEAR,
    TEXTURE_FILTER_MIPMAP_TRILINEAR
};

enum ETextureAddress {
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_MIRROR,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_BORDER,
    TEXTURE_ADDRESS_MIRROR_ONCE
};

struct STextureSampler {
    ETextureType TextureType;
    ETextureFilter Filter;
    ETextureAddress AddressU;
    ETextureAddress AddressV;
    ETextureAddress AddressW;
    float MipLODBias;
    float Anisotropy;
    float MinLod;
    float MaxLod;
};

/**
 Texture pixel format
 Bits: 7 - signed uncompressed, 6 - compressed
 For uncompressed formats:
    5 - float point,
    4 - srgb
    3+2 - num components (0 - 1, 1 - 2, 2 - 3, 3 - 4)
    1+0 - bytes per channel (0 - 1, 1 - 2, 2 - 4)
 For compressed formats
    7,5 - unused
    4 - srgb
    3+2 - num components (0 - 1, 1 - 2, 2 - 3, 3 - 4)
*/
enum ETexturePixelFormat : uint8_t
{
    TEXTURE_PF_R8_SIGNED  = ( 1<<7 ) | ( 0 << 2 ) | ( 0 ),
    TEXTURE_PF_RG8_SIGNED = ( 1<<7 ) | ( 1 << 2 ) | ( 0 ),
    TEXTURE_PF_BGR8_SIGNED = ( 1<<7 ) | ( 2 << 2 ) | ( 0 ),
    TEXTURE_PF_BGRA8_SIGNED = ( 1<<7 ) | ( 3 << 2 ) | ( 0 ),

    TEXTURE_PF_R8 = ( 0 << 2 ) | ( 0 ),
    TEXTURE_PF_RG8 = ( 1 << 2 ) | ( 0 ),
    TEXTURE_PF_BGR8 = ( 2 << 2 ) | ( 0 ),
    TEXTURE_PF_BGRA8 = ( 3 << 2 ) | ( 0 ),

    TEXTURE_PF_BGR8_SRGB = ( 1 << 4 ) | ( 2 << 2 ) | ( 0 ),
    TEXTURE_PF_BGRA8_SRGB = ( 1 << 4 ) | ( 3 << 2 ) | ( 0 ),

    TEXTURE_PF_R16_SIGNED = ( 1<<7 ) | ( 0 << 2 ) | ( 1 ),
    TEXTURE_PF_RG16_SIGNED = ( 1<<7 ) | ( 1 << 2 ) | ( 1 ),
    TEXTURE_PF_BGR16_SIGNED = ( 1<<7 ) | ( 2 << 2 ) | ( 1 ),
    TEXTURE_PF_BGRA16_SIGNED = ( 1<<7 ) | ( 3 << 2 ) | ( 1 ),

    TEXTURE_PF_R16 = ( 0 << 2 ) | ( 1 ),
    TEXTURE_PF_RG16 = ( 1 << 2 ) | ( 1 ),
    TEXTURE_PF_BGR16 = ( 2 << 2 ) | ( 1 ),
    TEXTURE_PF_BGRA16 = ( 3 << 2 ) | ( 1 ),

    TEXTURE_PF_R32_SIGNED = ( 1<<7 ) | ( 0 << 2 ) | ( 2 ),
    TEXTURE_PF_RG32_SIGNED = ( 1<<7 ) | ( 1 << 2 ) | ( 2 ),
    TEXTURE_PF_BGR32_SIGNED = ( 1<<7 ) | ( 2 << 2 ) | ( 2 ),
    TEXTURE_PF_BGRA32_SIGNED = ( 1<<7 ) | ( 3 << 2 ) | ( 2 ),

    TEXTURE_PF_R32 = ( 0 << 2 ) | ( 2 ),
    TEXTURE_PF_RG32 = ( 1 << 2 ) | ( 2 ),
    TEXTURE_PF_BGR32 = ( 2 << 2 ) | ( 2 ),
    TEXTURE_PF_BGRA32 = ( 3 << 2 ) | ( 2 ),

    TEXTURE_PF_R16F = ( 1<<7 ) | ( 1<<5 ) | ( 0 << 2 ) | ( 1 ),
    TEXTURE_PF_RG16F = ( 1<<7 ) | ( 1<<5 ) | ( 1 << 2 ) | ( 1 ),
    TEXTURE_PF_BGR16F = ( 1<<7 ) | ( 1<<5 ) | ( 2 << 2 ) | ( 1 ),
    TEXTURE_PF_BGRA16F = ( 1<<7 ) | ( 1<<5 ) | ( 3 << 2 ) | ( 1 ),

    TEXTURE_PF_R32F = ( 1<<7 ) | ( 1<<5 ) | ( 0 << 2 ) | ( 2 ),
    TEXTURE_PF_RG32F = ( 1<<7 ) | ( 1<<5 ) | ( 1 << 2 ) | ( 2 ),
    TEXTURE_PF_BGR32F = ( 1<<7 ) | ( 1<<5 ) | ( 2 << 2 ) | ( 2 ),
    TEXTURE_PF_BGRA32F = ( 1<<7 ) | ( 1<<5 ) | ( 3 << 2 ) | ( 2 ),

    TEXTURE_PF_COMPRESSED_RGB_DXT1 = ( 1<<6 ) | ( 2 << 2 ),
    TEXTURE_PF_COMPRESSED_RGBA_DXT1 = ( 1<<6 ) | ( 3 << 2 ),
    TEXTURE_PF_COMPRESSED_RGBA_DXT3 = ( 1<<6 ) | ( 3 << 2 ),
    TEXTURE_PF_COMPRESSED_RGBA_DXT5 = ( 1<<6 ) | ( 3 << 2 ),

    TEXTURE_PF_COMPRESSED_SRGB_DXT1 = ( 1<<6 ) | ( 1 << 4 ) | ( 2 << 2 ),
    TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT1 = ( 1<<6 ) | ( 1 << 4 ) | ( 3 << 2 ),
    TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT3 = ( 1<<6 ) | ( 1 << 4 ) | ( 3 << 2 ),
    TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT5 = ( 1<<6 ) | ( 1 << 4 ) | ( 3 << 2 ),

    TEXTURE_PF_COMPRESSED_RED_RGTC1 = ( 1<<6 ) | ( 0 << 2 ),
    TEXTURE_PF_COMPRESSED_RG_RGTC2 = ( 1<<6 ) | ( 1 << 2 ),

    TEXTURE_PF_COMPRESSED_RGBA_BPTC_UNORM = ( 1<<6 ) | ( 3 << 2 ),
    TEXTURE_PF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM = ( 1<<6 ) | ( 1 << 4 ) | ( 3 << 2 ),
    TEXTURE_PF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT = ( 1<<6 ) | ( 2 << 2 ),
    TEXTURE_PF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = ( 1<<6 ) | ( 2 << 2 ),
};

struct STexturePixelFormat {
    ETexturePixelFormat Data;

    STexturePixelFormat() : Data( TEXTURE_PF_BGRA8_SRGB ) {}
    STexturePixelFormat( ETexturePixelFormat _PixelFormat ) : Data( _PixelFormat ) {}

    void operator = ( ETexturePixelFormat _PixelFormat ) { Data = _PixelFormat; }

    bool operator==( ETexturePixelFormat _PixelFormat ) const { return Data == _PixelFormat; }
    bool operator==( STexturePixelFormat _PixelFormat ) const { return Data == _PixelFormat.Data; }
    bool operator!=( ETexturePixelFormat _PixelFormat ) const { return Data != _PixelFormat; }
    bool operator!=( STexturePixelFormat _PixelFormat ) const { return Data != _PixelFormat.Data; }

    bool IsCompressed() const {
        return ( Data >> 6 ) & 1;
    }

    bool IsSRGB() const {
        return ( Data >> 4 ) & 1;
    }

    int SizeInBytesUncompressed() const;

    int BlockSizeCompressed() const;

    int NumComponents() const {
        return ( ( Data >> 2 ) & 3 ) + 1;
    }

    void Read( IStreamBase & _Stream ) {
        Data = (ETexturePixelFormat)_Stream.ReadUInt8();
    }

    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteUInt8( (uint8_t)Data );
    }

    static bool GetAppropriatePixelFormat( class AImage const & _Image, STexturePixelFormat & _PixelFormat );
};

enum ETextureGroup {

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


//
// Material
//

enum EMaterialType {
    MATERIAL_TYPE_UNLIT,
    MATERIAL_TYPE_BASELIGHT,
    MATERIAL_TYPE_PBR,
    MATERIAL_TYPE_HUD,
    MATERIAL_TYPE_POSTPROCESS
};

enum EMaterialFacing {
    MATERIAL_FACE_FRONT,
    MATERIAL_FACE_BACK,
    MATERIAL_FACE_FRONT_AND_BACK
};

enum EMaterialDepthHack {
    MATERIAL_DEPTH_HACK_NONE,
    MATERIAL_DEPTH_HACK_WEAPON,
    MATERIAL_DEPTH_HACK_SKYBOX
};

struct SMaterialBuildData {
    /** Size of allocated memory for this structure (in bytes) */
    int SizeInBytes;

    /** Material type */
    EMaterialType Type;

    /** Facing */
    EMaterialFacing Facing;

    /** Lightmap binding unit */
    int LightmapSlot;

    /** Have texture fetching in vertex stage. This flag allow renderer to optimize sampler/texture bindings
    during rendering. */
    bool bDepthPassTextureFetch;
    bool bColorPassTextureFetch;
    bool bWireframePassTextureFetch;
    bool bShadowMapPassTextureFetch;

    /** Have vertex deformation in vertex stage. This flag allow renderer to optimize pipeline switching
    during rendering. */
    bool bHasVertexDeform;

    /** Experemental. Depth testing. */
    bool bDepthTest_EXPEREMENTAL;

    /** Disable shadow casting (for specific materials like skybox or first person shooter weapon) */
    bool bNoCastShadow;

    /** Enable shadow map masking */
    bool bShadowMapMasking;

    int NumUniformVectors;

    /** Material samplers */
    STextureSampler Samplers[MAX_MATERIAL_TEXTURES];
    int NumSamplers;

    /** Shader source code */
    char ShaderData[1];
};

//
// GPU Resources
//

class IGPUResourceOwner {
public:
    /** GPU resource owner must override this to upload resources to GPU */
    virtual void UploadResourcesGPU() = 0;

    /** Upload all GPU resources */
    static void UploadResources();

    /** Get list of GPU resource owners */
    static IGPUResourceOwner * GetResourceOwners() { return ResourceOwners; }

    /** Intrusive iterator */
    IGPUResourceOwner * GetNext() { return pNext; }

    /** Intrusive iterator */
    IGPUResourceOwner * GetPrev() { return pPrev; }

protected:
    IGPUResourceOwner();

    ~IGPUResourceOwner();

private:
    IGPUResourceOwner * pNext;
    IGPUResourceOwner * pPrev;

    static IGPUResourceOwner * ResourceOwners;
    static IGPUResourceOwner * ResourceOwnersTail;
};

class AResourceGPU {
public:
    /** Get resource owner */
    IGPUResourceOwner * GetOwner() { return pOwner; }

    /** Get list of all GPU resources */
    static AResourceGPU * GetResources() { return GPUResources; }

    /** Intrusive iterator */
    AResourceGPU * GetNext() { return pNext; }

    /** Intrusive iterator */
    AResourceGPU * GetPrev() { return pPrev; }

private:
    // Allow render backend to create and destroy GPU resources
    friend class IRenderBackend;

    template< typename T >
    static T * CreateResource( IGPUResourceOwner * InOwner )
    {
        void * pData = GZoneMemory.AllocCleared( sizeof( T ), 1 );
        AResourceGPU * resource = new (pData) T;  // compile time check: T must be derived from AResourceGPU
        resource->pOwner = InOwner;
        return static_cast< T * >( resource );
    }

    static void DestroyResource( AResourceGPU * InResource )
    {
        InResource->~AResourceGPU();
        GZoneMemory.Dealloc( InResource );
    }

protected:
    AResourceGPU();
    virtual ~AResourceGPU();

private:
    IGPUResourceOwner * pOwner;

    AResourceGPU * pNext;
    AResourceGPU * pPrev;

    // All GPU resources in one place
    static AResourceGPU * GPUResources;
    static AResourceGPU * GPUResourcesTail;
};

class ATextureGPU : public AResourceGPU {
public:
    void * pHandleGPU;
};

class ABufferGPU : public AResourceGPU {
public:
    void * pHandleGPU;
};

class AMaterialGPU : public AResourceGPU {
public:
    EMaterialType MaterialType;

    void *  pSampler[MAX_MATERIAL_TEXTURES];
    int     NumSamplers;

    int     LightmapSlot;

    bool    bDepthPassTextureFetch;
    bool    bColorPassTextureFetch;
    bool    bWireframePassTextureFetch;
    bool    bShadowMapPassTextureFetch;

    bool    bHasVertexDeform;

    // Just helper for render frontend to prevent rendering of materials with disabled shadow casting
    bool    bNoCastShadow;

    bool    bShadowMapMasking;

    struct {
        void * Lit;
        void * Unlit;
        void * HUD;
    } ShadeModel;
};

struct SMaterialFrameData {
    AMaterialGPU * Material;
    ATextureGPU * Textures[MAX_MATERIAL_TEXTURES];
    int NumTextures;
    Float4 UniformVectors[4];
    int NumUniformVectors;
};


//
// HUD
//

enum EHUDDrawCmd {
    HUD_DRAW_CMD_ALPHA,     // fonts, primitves, textures with one alpha channel
    HUD_DRAW_CMD_TEXTURE,   // textures
    HUD_DRAW_CMD_MATERIAL,  // material instances (MATERIAL_TYPE_HUD)
    HUD_DRAW_CMD_VIEWPORT,  // viewports

    HUD_DRAW_CMD_MAX
};

enum EDebugDrawCmd {
    DBG_DRAW_CMD_POINTS,
    DBG_DRAW_CMD_POINTS_DEPTH_TEST,
    DBG_DRAW_CMD_LINES,
    DBG_DRAW_CMD_LINES_DEPTH_TEST,
    DBG_DRAW_CMD_TRIANGLE_SOUP,
    DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST,

    DBG_DRAW_CMD_MAX,

    DBG_DRAW_CMD_NOP
};

struct SDebugDrawCmd {
    EDebugDrawCmd Type;
    int FirstVertex;
    int NumVertices;
    int FirstIndex;
    int NumIndices;
};

enum EHUDSamplerType {
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

enum EColorBlending {
    COLOR_BLENDING_ALPHA,
    COLOR_BLENDING_DISABLED,
    COLOR_BLENDING_COLOR_ADD,
    COLOR_BLENDING_MULTIPLY,
    COLOR_BLENDING_SOURCE_TO_DEST,
    COLOR_BLENDING_ADD_MUL,
    COLOR_BLENDING_ADD_ALPHA,

    COLOR_BLENDING_MAX
};

struct SHUDDrawCmd {
    unsigned int    IndexCount;
    unsigned int    StartIndexLocation;
    Float2          ClipMins;
    Float2          ClipMaxs;
    EHUDDrawCmd     Type;
    EColorBlending  Blending;           // only for type DRAW_CMD_TEXTURE and DRAW_CMD_VIEWPORT
    EHUDSamplerType SamplerType;        // only for type DRAW_CMD_TEXTURE

    union {
        ATextureGPU *        Texture;               // HUD_DRAW_CMD_TEXTURE, HUD_DRAW_CMD_ALPHA
        SMaterialFrameData * MaterialFrameData;     // HUD_DRAW_CMD_MATERIAL
        int                  ViewportIndex;         // HUD_DRAW_CMD_VIEWPORT
    };
};

struct SHUDDrawList {
    int             VerticesCount;
    int             IndicesCount;
    SHUDDrawVert *  Vertices;
    unsigned short* Indices;
    int             CommandsCount;
    SHUDDrawCmd *   Commands;
    SHUDDrawList *  pNext;
};

struct SDirectionalLightDef {
    Float4   ColorAndAmbientIntensity;
    Float3x3 Matrix;            // Light rotation matrix
    int      RenderMask;
    int      MaxShadowCascades; // Max allowed cascades for light
    int      FirstCascade;      // First cascade offset
    int      NumCascades;       // Current visible cascades count for light
    bool     bCastShadow;
};


//
// Render instance
//

struct SRenderInstance {
    AMaterialGPU *      Material;
    SMaterialFrameData *MaterialInstance;

    ABufferGPU *        VertexBuffer;
    size_t              VertexBufferOffset;

    ABufferGPU *        IndexBuffer;
    size_t              IndexBufferOffset;

    ABufferGPU *        WeightsBuffer;
    size_t              WeightsBufferOffset;

    ABufferGPU *        VertexLightChannel;
    size_t              VertexLightOffset;

    ABufferGPU *        LightmapUVChannel;
    size_t              LightmapUVOffset;

    ATextureGPU *       Lightmap;
    Float4              LightmapOffset;
    Float4x4            Matrix;
    Float3x3            ModelNormalToViewSpace;
    size_t              SkeletonOffset;
    size_t              SkeletonSize;
    unsigned int        IndexCount;
    unsigned int        StartIndexLocation;
    int                 BaseVertexLocation;
    uint64_t            SortKey;
};

//
// ShadowMap Render instance
//

struct SShadowRenderInstance {
    AMaterialGPU *      Material;
    SMaterialFrameData *MaterialInstance;
    ABufferGPU *        VertexBuffer;
    size_t              VertexBufferOffset;
    ABufferGPU *        IndexBuffer;
    size_t              IndexBufferOffset;
    ABufferGPU *        WeightsBuffer;
    size_t              WeightsBufferOffset;
    Float3x4            WorldTransformMatrix;
    size_t              SkeletonOffset;
    size_t              SkeletonSize;
    unsigned int        IndexCount;
    unsigned int        StartIndexLocation;
    int                 BaseVertexLocation;
    uint16_t            CascadeMask;
    uint64_t            SortKey;
};

//
// Frustum cluster data
//

// texture3d RG32UI
// ivec2 Offset = texelFetch( ClusterLookup, TexCoord ).xy;
// Offset.X  -- item offset
// int NumProbes = Offest.Y & 0xff;
// int NumDecals = ( Offest.Y >> 8 ) & 0xff;
// int NumLights = ( Offest.Y >> 16 ) & 0xff;
// int Unused = ( Offest.Y >> 24 ) & 0xff // can be used in future
struct SClusterData {
    uint32_t ItemOffset;
    uint8_t NumProbes;
    uint8_t NumDecals;
    uint8_t NumLights;
    uint8_t Unused;
};

// texture1d R32UI
struct SClusterItemBuffer {
    /**
    Packed light, decal and probe index:

    Read indices in shader:
        uint Indices = (uint)(texelFetch( ItemList, Offset.X ).X);

    Unpack indices:
        int LightIndex = Indices & 0x3ff;
        int DecalIndex = ( Indices >> 12 ) & 0x3ff;
        int ProbeIndex = Indices >> 24;
    */
    uint32_t Indices;
};

enum EClusterLightType {
    CLUSTER_LIGHT_POINT,
    CLUSTER_LIGHT_SPOT,
};

struct SClusterLight {
    Float3 Position;     // For point and spot lights: position and radius
    float  OuterRadius;

    float LightType;     // EClusterLightType
    float InnerRadius;
    float OuterConeAngle;
    float InnerConeAngle;

    Float3 SpotDirection;
    float SpotExponent;

    Float4 Color;    // RGB, alpha - ambient intensity

    unsigned int RenderMask;
    unsigned int Padding0;
    unsigned int Padding1;
    unsigned int Padding2;
};

struct SFrameLightData {
    static constexpr int MAX_ITEM_BUFFER = 1024*128; // TODO: подобрать оптимальный размер

    SClusterData ClusterLookup[MAX_FRUSTUM_CLUSTERS_Z][MAX_FRUSTUM_CLUSTERS_Y][MAX_FRUSTUM_CLUSTERS_X];

    SClusterItemBuffer ItemBuffer[MAX_ITEM_BUFFER + MAX_CLUSTER_ITEMS*3]; //  + MAX_CLUSTER_ITEMS*3 для возможного выхода за пределы массива на максимальное количество итемов в кластере
    int TotalItems;

    SClusterLight LightBuffer[MAX_LIGHTS];
    int TotalLights;

    //SClusterDecal Probes[MAX_DECAL];
    //int TotalDecals;

    //SClusterProbe Probes[MAX_PROBES];
    //int TotalProbes;
};

//
// Render frame
//

using AArrayOfDebugVertices = TPodArrayHeap< SDebugVertex, 1024 >;
using AArrayOfDebugIndices = TPodArrayHeap< unsigned int, 1024 >;
using AArrayOfDebugDrawCmds = TPodArrayHeap< SDebugDrawCmd >;

struct SRenderView {
    // Current view index
    int ViewIndex;

    // Viewport size
    int Width;
    int Height;

    // Time parameters
    float GameRunningTimeSeconds;
    float GameplayTimeSeconds;

    // View parameters
    Float3 ViewPosition;
    Quat ViewRotation;
    Float3 ViewRightVec;
    Float3 ViewUpVec;
    Float3 ViewDir;
    Float4x4 ViewMatrix;
    float ViewZNear;
    float ViewZFar;
    float ViewFovX;
    float ViewFovY;
    Float2 ViewOrthoMins;
    Float2 ViewOrthoMaxs;
    Float3x3 NormalToViewMatrix;
    Float4x4 ProjectionMatrix;
    Float4x4 InverseProjectionMatrix;
    Float4x4 ModelviewProjection;
    Float4x4 ViewSpaceToWorldSpace;
    Float4x4 ClipSpaceToWorldSpace;
    Float4x4 ClusterProjectionMatrix;
    Float3 BackgroundColor;
    bool bClearBackground;
    bool bWireframe;
    bool bPerspective;
    bool bPadding1;

    float MaxVisibleDistance;

    int NumShadowMapCascades;
    int NumCascadedShadowMaps;

    int FirstInstance;
    int InstanceCount;

    int FirstShadowInstance;
    int ShadowInstanceCount;

    int FirstDirectionalLight;
    int NumDirectionalLights;

    //int FirstLight;
    //int NumLights;

    int FirstDebugDrawCommand;
    int DebugDrawCommandCount;

    Float4x4 LightViewProjectionMatrices[MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES];
    Float4x4 ShadowMapMatrices[MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES];

    SFrameLightData LightData;
};

struct SRenderFrame {
    // Game tick
    int FrameNumber;

    // Max surface resolution
    int AllocSurfaceWidth;
    int AllocSurfaceHeight;

    // Canvas size
    int CanvasWidth;
    int CanvasHeight;

    SRenderView RenderViews[MAX_RENDER_VIEWS];
    int NumViews;

    int ShadowCascadePoolSize;

    TPodArray< SRenderInstance *, 1024 > Instances;
    TPodArray< SShadowRenderInstance *, 1024 > ShadowInstances;
    TPodArray< SDirectionalLightDef * > DirectionalLights;

    SHUDDrawList * DrawListHead;
    SHUDDrawList * DrawListTail;

    AArrayOfDebugVertices DbgVertices;
    AArrayOfDebugIndices  DbgIndices;
    AArrayOfDebugDrawCmds DbgCmds;
};

struct SRenderFrontendDef {
    SRenderView * View;
    BvFrustum const * Frustum;
    int VisibilityMask;

    int PolyCount;
    int ShadowMapPolyCount;
};


//
// Render backend interface
//

struct STextureOffset {
    uint16_t Lod;
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct STextureDimension {
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct STextureRect {
    STextureOffset Offset;
    STextureDimension Dimension;
};

class IRenderBackend {
public:
    IRenderBackend( const char * _BackendName ) : BackendName( _BackendName ) {}

    virtual void PreInit() = 0;
    virtual void Initialize( void * _NativeWindowHandle ) = 0;
    virtual void Deinitialize() = 0;

    virtual void RenderFrame( SRenderFrame * _FrameData ) = 0;
    virtual void WaitGPU() = 0;

    virtual ATextureGPU * CreateTexture( IGPUResourceOwner * _Owner ) = 0;
    virtual void DestroyTexture( ATextureGPU * _Texture ) = 0;
    virtual void InitializeTexture1D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) = 0;
    virtual void InitializeTexture1DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) = 0;
    virtual void InitializeTexture2D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) = 0;
    virtual void InitializeTexture2DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) = 0;
    virtual void InitializeTexture3D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) = 0;
    virtual void InitializeTextureCubemap( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) = 0;
    virtual void InitializeTextureCubemapArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) = 0;
    virtual void InitializeTexture2DNPOT( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) = 0;
    virtual void WriteTexture( ATextureGPU * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem ) = 0;
    virtual void ReadTexture( ATextureGPU * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) = 0;

    virtual ABufferGPU * CreateBuffer( IGPUResourceOwner * _Owner ) = 0;
    virtual void DestroyBuffer( ABufferGPU * _Buffer ) = 0;
    virtual void InitializeBuffer( ABufferGPU * _Buffer, size_t _SizeInBytes, bool _DynamicStorage ) = 0;
    virtual void WriteBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) = 0;
    virtual void ReadBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) = 0;

    virtual AMaterialGPU * CreateMaterial( IGPUResourceOwner * _Owner ) = 0;
    virtual void DestroyMaterial( AMaterialGPU * _Material ) = 0;
    virtual void InitializeMaterial( AMaterialGPU * _Material, SMaterialBuildData const * _BuildData ) = 0;

    virtual size_t AllocateJoints( size_t _JointsCount ) = 0;
    virtual void WriteJoints( size_t _Offset, size_t _JointsCount, Float3x4 const * _Matrices ) = 0;

    const char * GetName() { return BackendName; }

protected:
    template< typename T >
    static T * CreateResource( IGPUResourceOwner * InOwner )
    {
        return AResourceGPU::CreateResource< T >( InOwner );
    }

    static void DestroyResource( AResourceGPU * InResource )
    {
        return AResourceGPU::DestroyResource( InResource );
    }

private:
    const char * BackendName = "Unnamed";
};

extern IRenderBackend * GRenderBackend;
