/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Core/Public/CoreMath.h>
#include <Engine/Core/Public/PodArray.h>

#include "Runtime.h"

#define MAX_RENDER_VIEWS 16

struct FMeshVertex {
    Float3 Position;
    Float2 TexCoord;
    Float3 Tangent;
    float  Handedness;
    Float3 Normal;

    static FMeshVertex Lerp( FMeshVertex const & _Vertex1, FMeshVertex const & _Vertex2, float _Value = 0.5f );
};

struct FMeshLightmapUV {
    Float2 TexCoord;

    static FMeshLightmapUV Lerp( FMeshLightmapUV const & _Vertex1, FMeshLightmapUV const & _Vertex2, float _Value = 0.5f );
};

struct FMeshVertexLight {
    uint32_t VertexLight;

    static FMeshVertexLight Lerp( FMeshVertexLight const & _Vertex1, FMeshVertexLight const & _Vertex2, float _Value = 0.5f );
};

struct FMeshVertexJoint {
    byte   JointIndices[4];
    byte   JointWeights[4];
};

struct FDrawVert {
    Float2  Position;
    Float2  TexCoord;
    uint32_t Color;
};

struct FDebugVertex {
    Float3 Position;
    uint32_t Color;
};

AN_FORCEINLINE FMeshVertex FMeshVertex::Lerp( FMeshVertex const & _Vertex1, FMeshVertex const & _Vertex2, float _Value ) {
    FMeshVertex Result;

    Result.Position   = _Vertex1.Position.Lerp( _Vertex2.Position, _Value );
    Result.TexCoord   = _Vertex1.TexCoord.Lerp( _Vertex2.TexCoord, _Value );
    Result.Tangent    = _Vertex1.Tangent.Lerp( _Vertex2.Tangent, _Value ).Normalized(); // TODO: Use spherical lerp?
    Result.Handedness = _Value >= 0.5f ? _Vertex2.Handedness : _Vertex1.Handedness; // FIXME
    Result.Normal     = _Vertex1.Normal.Lerp( _Vertex2.Normal, _Value ).Normalized(); // TODO: Use spherical lerp?

    return Result;
}

AN_FORCEINLINE FMeshLightmapUV FMeshLightmapUV::Lerp( FMeshLightmapUV const & _Vertex1, FMeshLightmapUV const & _Vertex2, float _Value ) {
    FMeshLightmapUV Result;

    Result.TexCoord   = _Vertex1.TexCoord.Lerp( _Vertex2.TexCoord, _Value );

    return Result;
}

AN_FORCEINLINE FMeshVertexLight FMeshVertexLight::Lerp( FMeshVertexLight const & _Vertex1, FMeshVertexLight const & _Vertex2, float _Value ) {
    FMeshVertexLight Result;

    const byte * c0 = reinterpret_cast< const byte * >( &_Vertex1.VertexLight );
    const byte * c1 = reinterpret_cast< const byte * >( &_Vertex2.VertexLight );
    byte * r = reinterpret_cast< byte * >( &Result.VertexLight );

    r[0] = ( c0[0] + c1[0] ) >> 1;
    r[1] = ( c0[1] + c1[1] ) >> 1;
    r[2] = ( c0[2] + c1[2] ) >> 1;
    r[3] = ( c0[3] + c1[3] ) >> 1;

    return Result;
}

enum EVertexType {
    VERT_DEBUGVERTEX = 0,
    VERT_MESHVERTEX  = 1
};

enum EIndexType {
    INDEX_UINT16 = 0,
    INDEX_UINT32 = 1
};

enum ETextureColorSpace {
    TEXTURE_COLORSPACE_RGBA              = 0,
    TEXTURE_COLORSPACE_sRGB_ALPHA        = 1,
    TEXTURE_COLORSPACE_YCOCG             = 2,
    TEXTURE_COLORSPACE_NM_XY             = 3,
    TEXTURE_COLORSPACE_NM_XYZ            = 4,
    TEXTURE_COLORSPACE_NM_SPHEREMAP      = 5,
    TEXTURE_COLORSPACE_NM_XY_STEREOGRAPHIC = 6,
    TEXTURE_COLORSPACE_NM_XY_PARABOLOID  = 7,
    TEXTURE_COLORSPACE_NM_XY_QUARTIC     = 8,
    TEXTURE_COLORSPACE_NM_FLOAT          = 9,
    TEXTURE_COLORSPACE_NM_DXT5           = 10,
    TEXTURE_COLORSPACE_GRAYSCALED        = 11,
    TEXTURE_COLORSPACE_RGBA_INT          = 12,
    TEXTURE_COLORSPACE_RGBA_UINT         = 13
};

enum ETextureType {
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    //TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_ARRAY,
    //TEXTURE_2D_ARRAY_MULTISAMPLE,
    TEXTURE_3D,
    TEXTURE_CUBEMAP,
    TEXTURE_CUBEMAP_ARRAY,
    TEXTURE_RECT,

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

struct FSamplerDesc {
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

// ETexturePixelFormat
// Bits: 7 - signed uncompressed, 6 - compressed
// For uncompressed formats:
//    5 - float point,
//    4 - srgb
//    3+2 - num components (0 - 1, 1 - 2, 2 - 3, 3 - 4)
//    1+0 - bytes per channel (0 - 1, 1 - 2, 2 - 4)
// For compressed formats
//    7,5 - unused
//    4 - srgb
//    3+2 - num components (0 - 1, 1 - 2, 2 - 3, 3 - 4)
enum ETexturePixelFormat {
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

bool IsTextureCompressed( ETexturePixelFormat _PixelFormat );
int UncompressedPixelByteLength( ETexturePixelFormat _PixelFormat );
int CompressedTextureBlockLength( ETexturePixelFormat _PixelFormat );
int NumPixelComponents( ETexturePixelFormat _PixelFormat );
bool IsTextureSRGB( ETexturePixelFormat _PixelFormat );

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

class IRenderProxyOwner {
public:
    virtual void OnLost() {}
};

enum ERenderProxyType {
    RENDER_PROXY_INDEXED_MESH,
    RENDER_PROXY_LIGHTMAP_UV_CHANNEL,
    RENDER_PROXY_VERTEX_LIGHT_CHANNEL,
    RENDER_PROXY_SKELETON,
    RENDER_PROXY_TEXTURE,
    RENDER_PROXY_MATERIAL
};

class FRenderProxy {
    AN_FORBID_COPY( FRenderProxy )

public:
    template< typename T > static T * NewProxy();

    ERenderProxyType GetType() const { return Type; }

    void SetOwner( IRenderProxyOwner * _Owner ) { Owner = _Owner; }
    IRenderProxyOwner * GetOwner() { return Owner; }

    bool IsSubmittedToRenderThread() const { return bSubmittedToRenderThread; }
    bool IsPendingKill() const { return bPendingKill; }

    void MarkUpdated();
    void KillProxy();

    FRenderProxy * GetNextFreeProxy() { return NextFreeProxy; }

    // Internal

    // Accessed only by render thread:
    FRenderProxy * Next;            // list of uploaded proxies
    FRenderProxy * Prev;            // list of uploaded proxies

    FRenderProxy * NextUpload;
    FRenderProxy * PrevUpload;

protected:
    FRenderProxy() {}
public:
    virtual ~FRenderProxy() {}

protected:
    ERenderProxyType Type;
    IRenderProxyOwner * Owner;

private:
    bool bSubmittedToRenderThread;
    bool bPendingKill;
    FRenderProxy * NextFreeProxy;
};

template< typename T >
AN_FORCEINLINE T * FRenderProxy::NewProxy() {
    FRenderProxy * proxy = ( T * )GMainMemoryZone.AllocCleared( sizeof( T ), 1 );
    return new (proxy) T();
}

struct FVertexChunk {
    FVertexChunk * Next;
    FVertexChunk * Prev;
    int StartVertexLocation;
    int VerticesCount;
    FMeshVertex Vertices[1];
};

struct FVertexJointChunk {
    FVertexJointChunk * Next;
    FVertexJointChunk * Prev;
    int StartVertexLocation;
    int VerticesCount;
    FMeshVertexJoint Vertices[1];
};

struct FIndexChunk {
    FIndexChunk * Next;
    FIndexChunk * Prev;
    int StartIndexLocation;
    int IndexCount;
    unsigned int Indices[1];
};

struct FLightmapChunk {
    FLightmapChunk * Next;
    FLightmapChunk * Prev;
    int StartVertexLocation;
    int VerticesCount;
    FMeshLightmapUV Vertices[1];
};

struct FVertexLightChunk {
    FVertexLightChunk * Next;
    FVertexLightChunk * Prev;
    int StartVertexLocation;
    int VerticesCount;
    FMeshVertexLight Vertices[1];
};

struct FJointTransformChunk {
    FJointTransformChunk * Next;
    FJointTransformChunk * Prev;
    int StartJointLocation;
    int JointsCount;
    Float3x4 Transforms[1];
};

struct FTextureChunk {
    FTextureChunk * Next;
    FTextureChunk * Prev;
    int LocationX;
    int LocationY;
    int LocationZ;
    int Width;
    int Height;
    int LodNum;
    int Pixels[1];
};

class FRenderProxy_IndexedMesh : public FRenderProxy {
    AN_FORBID_COPY( FRenderProxy_IndexedMesh )

    friend class FRenderProxy;

public:
    struct FrameData {
        int VerticesCount;
        int IndicesCount;
        int IndexType;

        bool bSkinnedMesh;
        bool bDynamicStorage;

        FVertexChunk * VertexChunks;
        FVertexChunk * VertexChunksTail;
        FVertexJointChunk * VertexJointChunks;
        FVertexJointChunk * VertexJointChunksTail;
        FIndexChunk * IndexChunks;
        FIndexChunk * IndexChunksTail;

        bool bReallocated;
    };

    FrameData Data;

    // Accessed only by render thread:
    enum { MAX_HANDLES = 3 };
    size_t Handles[MAX_HANDLES];
    int VertexCount;
    int IndexCount;
    int IndexType;

protected:
    FRenderProxy_IndexedMesh();
    ~FRenderProxy_IndexedMesh();
};

class FRenderProxy_LightmapUVChannel : public FRenderProxy {
    AN_FORBID_COPY( FRenderProxy_LightmapUVChannel )

    friend class FRenderProxy;

public:
    struct FrameData {
        int VerticesCount;
        bool bDynamicStorage;

        FLightmapChunk * Chunks;
        FLightmapChunk * ChunksTail;

        bool bReallocated;
    };

    FrameData Data;

    // Accessed only by render thread:
    size_t Handle;
    int VertexCount;

protected:
    FRenderProxy_LightmapUVChannel();
    ~FRenderProxy_LightmapUVChannel();
};

class FRenderProxy_VertexLightChannel : public FRenderProxy {
    AN_FORBID_COPY( FRenderProxy_VertexLightChannel )

    friend class FRenderProxy;

public:
    struct FrameData {
        int VerticesCount;
        bool bDynamicStorage;

        FVertexLightChunk * Chunks;
        FVertexLightChunk * ChunksTail;

        bool bReallocated;
    };

    FrameData Data;

    // Accessed only by render thread:
    size_t Handle;
    int VertexCount;

protected:
    FRenderProxy_VertexLightChannel();
    ~FRenderProxy_VertexLightChannel();
};

class FRenderProxy_Skeleton : public FRenderProxy {
    AN_FORBID_COPY( FRenderProxy_Skeleton )

    friend class FRenderProxy;

public:
    struct FrameData {
        int JointsCount;

        FJointTransformChunk * Chunks;
        FJointTransformChunk * ChunksTail;

        bool bReallocated;
    };

    FrameData Data;

    // Accessed only by render thread:
    size_t Handle;
    int JointsCount;

protected:
    FRenderProxy_Skeleton();
    ~FRenderProxy_Skeleton();
};

class FRenderProxy_Texture : public FRenderProxy {
    AN_FORBID_COPY( FRenderProxy_Texture )

    friend class FRenderProxy;

public:
    struct FrameData {
        int TextureType;
        ETexturePixelFormat PixelFormat;

        int Width;
        int Height;
        int Depth;
        int NumLods;

        FTextureChunk * Chunks;
        FTextureChunk * ChunksTail;

        bool bReallocated;
    };

    FrameData Data;

    // Accessed only by render thread:
    enum { MAX_HANDLES = 1 };
    size_t Handles[MAX_HANDLES];

protected:
    FRenderProxy_Texture();
    ~FRenderProxy_Texture();
};

enum EMaterialType {
    MATERIAL_TYPE_UNLIT,
    MATERIAL_TYPE_PBR,
    MATERIAL_TYPE_HUD
};

enum EMaterialFacing {
    MATERIAL_FACE_FRONT,
    MATERIAL_FACE_BACK,
    MATERIAL_FACE_FRONT_AND_BACK
};

enum {
    MAX_MATERIAL_TEXTURES = 15
};

struct FMaterialBuildData {
    // Size of allocated memory for this structure (in bytes)
    int Size;

    // Material type
    EMaterialType Type;

    // Facing
    EMaterialFacing Facing;

    // Lightmap binding unit
    int LightmapSlot;

    // Have texture fetching in vertex stage. This flag allow renderer to optimize sampler/texture bindings
    // during rendering.
    bool bVertexTextureFetch;

    // Have vertex deformation in vertex stage. This flag allow renderer to optimize pipeline switching
    // during rendering.
    bool bNoVertexDeform;

    int NumUniformVectors;

    // TODO:
    // Surface specific for tricks with depth buffer
    //enum ESurfaceSpecific {
    //    SURF_SPEC_COMMON,
    //    SURF_SPEC_WEAPON,
    //    SURF_SPEC_SKY
    //};
    //ESurfaceSpecific SurfaceSpecific;

    // Vertex source code lump
    int VertexSourceOffset;
    int VertexSourceLength;

    // Fragment source code lump
    int FragmentSourceOffset;
    int FragmentSourceLength;

    // Geometry source code lump
    int GeometrySourceOffset;
    int GeometrySourceLength;

    // Material samplers
    FSamplerDesc Samplers[MAX_MATERIAL_TEXTURES];
    int NumSamplers;

    // Shader source code
    char ShaderData[1];
};

class FRenderProxy_Material : public FRenderProxy {
    AN_FORBID_COPY( FRenderProxy_Material )

    friend class FRenderProxy;

public:
    FMaterialBuildData * Data;

    // Accessed only by render thread:
    enum {
        MAX_SAMPLER_HANDLES = MAX_MATERIAL_TEXTURES,

        PIPELINE_PBR_DEPTH_PASS = 0,
        PIPELINE_PBR_DEPTH_PASS_SKINNED,
        PIPELINE_PBR_WIREFRAME_PASS,
        PIPELINE_PBR_WIREFRAME_PASS_SKINNED,
        PIPELINE_PBR_COLOR_PASS_SIMPLE,
        PIPELINE_PBR_COLOR_PASS_SKINNED,
        PIPELINE_PBR_COLOR_PASS_LIGHTMAP,
        PIPELINE_PBR_COLOR_PASS_VERTEX_LIGHT,        
        PIPELINE_PBR_MAX_HANDLES,

        PIPELINE_UNLIT_DEPTH_PASS = 0,
        PIPELINE_UNLIT_DEPTH_PASS_SKINNED,
        PIPELINE_UNLIT_WIREFRAME_PASS,
        PIPELINE_UNLIT_WIREFRAME_PASS_SKINNED,
        PIPELINE_UNLIT_COLOR_PASS_SIMPLE,
        PIPELINE_UNLIT_COLOR_PASS_SKINNED,        
        PIPELINE_UNLIT_MAX_HANDLES,

        PIPELINE_HUD_COLOR_PASS_SIMPLE = 0,
        PIPELINE_HUD_MAX_HANDLES,

        PIPELINE_MAX_HANDLES = ( PIPELINE_PBR_MAX_HANDLES > PIPELINE_UNLIT_MAX_HANDLES )
                                ? PIPELINE_PBR_MAX_HANDLES
                                : ( PIPELINE_UNLIT_MAX_HANDLES > PIPELINE_HUD_MAX_HANDLES )
                                ? PIPELINE_UNLIT_MAX_HANDLES
                                : PIPELINE_HUD_MAX_HANDLES
                                //std::max( std::max( PIPELINE_PBR_MAX_HANDLES, PIPELINE_UNLIT_MAX_HANDLES ), PIPELINE_HUD_MAX_HANDLES )

    };

    EMaterialType MaterialType;

    size_t  Samplers[MAX_SAMPLER_HANDLES];
    int     NumSamplers;

    size_t  Pipelines[PIPELINE_MAX_HANDLES];
    int     NumPipelines;

    int     LightmapSlot;

    bool    bVertexTextureFetch;
    bool    bNoVertexDeform;

protected:
    FRenderProxy_Material();
    ~FRenderProxy_Material();
};

struct FMaterialInstanceFrameData {
    FRenderProxy_Material * Material;
    FRenderProxy_Texture * Textures[MAX_MATERIAL_TEXTURES];
    int NumTextures;
    Float4 UniformVectors[4];
    int NumUniformVectors;
};

enum FCanvasDrawCmd {
    CANVAS_DRAW_CMD_ALPHA,         // fonts, primitves, textures with one alpha channel
    CANVAS_DRAW_CMD_TEXTURE,       // textures
    CANVAS_DRAW_CMD_MATERIAL,      // material instances (MATERIAL_TYPE_HUD)
    CANVAS_DRAW_CMD_VIEWPORT,      // viewports

    CANVAS_DRAW_CMD_MAX
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

struct FDebugDrawCmd {
    EDebugDrawCmd Type;
    int FirstVertex;
    int NumVertices;
    int FirstIndex;
    int NumIndices;
};

//TEXTURE_SAMPLER_WRAP,
//TEXTURE_SAMPLER_MIRROR,
//TEXTURE_SAMPLER_CLAMP,
//TEXTURE_SAMPLER_BORDER,
//TEXTURE_SAMPLER_MIRROR_ONCE

enum ESamplerType {
    SAMPLER_TYPE_TILED_LINEAR,
    SAMPLER_TYPE_TILED_NEAREST,

    SAMPLER_TYPE_MIRROR_LINEAR,
    SAMPLER_TYPE_MIRROR_NEAREST,

    SAMPLER_TYPE_CLAMPED_LINEAR,
    SAMPLER_TYPE_CLAMPED_NEAREST,

    SAMPLER_TYPE_BORDER_LINEAR,
    SAMPLER_TYPE_BORDER_NEAREST,

    SAMPLER_TYPE_MIRROR_ONCE_LINEAR,
    SAMPLER_TYPE_MIRROR_ONCE_NEAREST,

    SAMPLER_TYPE_MAX
};

struct FDrawCmd {
    unsigned int    IndexCount;
    unsigned int    StartIndexLocation;
    Float2          ClipMins;
    Float2          ClipMaxs;
    FCanvasDrawCmd  Type;
    EColorBlending  Blending;           // only for type DRAW_CMD_TEXTURE and DRAW_CMD_VIEWPORT
    ESamplerType    SamplerType;        // only for type DRAW_CMD_TEXTURE

    union {
        FRenderProxy_Texture * Texture;
        FMaterialInstanceFrameData * MaterialInstance;
        int ViewportIndex;
    };
};

struct FDrawList {
    int VerticesCount;
    int IndicesCount;
    FDrawVert * Vertices;
    unsigned short * Indices;
    int CommandsCount;
    FDrawCmd * Commands;
    FDrawList * Next;
};

using FArrayOfDebugVertices = TPodArray< FDebugVertex, 1024 >;
using FArrayOfDebugIndices = TPodArray< unsigned int, 1024 >;
using FArrayOfDebugDrawCmds = TPodArray< FDebugDrawCmd >;

struct FRenderInstance {
    FRenderProxy_Material *             Material;
    FMaterialInstanceFrameData *        MaterialInstance;
    FRenderProxy_IndexedMesh *          MeshRenderProxy;
    FRenderProxy_Skeleton *             Skeleton;
    FRenderProxy_VertexLightChannel *   VertexLightChannel;
    FRenderProxy_LightmapUVChannel *    LightmapUVChannel;
    FRenderProxy_Texture *              Lightmap;
    Float4                              LightmapOffset;
    Float4x4                            Matrix;
    Float3x3                            ModelNormalToViewSpace;
    unsigned int                        IndexCount;
    unsigned int                        StartIndexLocation;
    int                                 BaseVertexLocation;
};

struct FRenderView {
    // Current view index
    int ViewIndex;

    // Viewport size
    int Width;
    int Height;

    // Time parameters
    float GameRunningTimeSeconds;
    float GameplayTimeSeconds;

    // View parameters
    Float3 ViewPostion;
    Quat ViewRotation;
    Float4x4 ViewMatrix;
    Float3x3 NormalToViewMatrix;
    Float4x4 ProjectionMatrix;
    Float4x4 InverseProjectionMatrix;
    Float4x4 ModelviewProjection;
    Float4x4 ViewSpaceToWorldSpace;
    Float4x4 ClipSpaceToWorldSpace;
    Float3 BackgroundColor;
    bool bClearBackground;
    bool bWireframe;

    int PresentCmd;

    int FirstInstance;
    int InstanceCount;

    int FirstDbgCmd;
    int DbgCmdCount;
};

struct FRenderFrame {
    // Game tick
    int FrameNumber;

    // Max surface resolution
    int AllocSurfaceWidth;
    int AllocSurfaceHeight;

    // Canvas size
    int CanvasWidth;
    int CanvasHeight;

    // Memory
    size_t FrameMemoryUsed;
    size_t FrameMemorySize;
    void * pFrameMemory;

    FRenderView RenderViews[MAX_RENDER_VIEWS];
    int NumViews;

    FRenderProxy * RenderProxyUploadHead;
    FRenderProxy * RenderProxyUploadTail;
    FRenderProxy * RenderProxyFree;

    TPodArray< FRenderInstance *, 1024 > Instances;

    FDrawList * DrawListHead;
    FDrawList * DrawListTail;

    FArrayOfDebugVertices DbgVertices;
    FArrayOfDebugIndices  DbgIndices;
    FArrayOfDebugDrawCmds DbgCmds;

    void * AllocFrameData( size_t _BytesCount );
};

struct FRenderBackendFeatures {
    bool bSwapControl;
    bool bSwapControlTear;
};

struct FRenderFeatures {
    int VSyncMode;
};

struct FRenderBackend {
    const char * Name;
    void ( *PreInit )();
    void ( *Initialize )( void ** _Devices, int _DeviceCount, FRenderBackendFeatures * _Features );
    void ( *Deinitialize )();
    void ( *SetRenderFeatures )( FRenderFeatures const & _Features );
    void ( *RenderFrame )( FRenderFrame * _FrameData );
    void ( *CleanupFrame )( FRenderFrame * _FrameData );
    void ( *WaitGPU )();
    FRenderBackend * Next;
};

// Get backend list
FRenderBackend const * GetRenderBackends();

// Find backend by name
FRenderBackend const * FindRenderBackend( const char * _Name );

// Register backend
#define REGISTER_RENDER_BACKEND( _Backend ) \
{ \
    extern FRenderBackend _Backend; \
    void RegisterRenderBackend( FRenderBackend * _BackendInterface ); \
    RegisterRenderBackend( &_Backend ); \
}

void * AllocateBufferData( size_t _Size );
void * ExtendBufferData( void * _Data, size_t _OldSize, size_t _NewSize, bool _KeepOld );
void DeallocateBufferData( void * _Data );

// Current backend
extern FRenderBackend const * GRenderBackend;
extern FRenderProxy * GRenderProxyHead;
extern FRenderProxy * GRenderProxyTail;
