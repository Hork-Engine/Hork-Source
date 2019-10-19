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
#include <Engine/Core/Public/Logger.h>

//
// Common constants
//

constexpr int MAX_RENDER_VIEWS                      = 16;
constexpr int MAX_SKINNED_MESH_JOINTS               = 256;
constexpr int MAX_SKINNED_MESH_INSTANCES_PER_FRAME  = 256;
constexpr int MAX_MATERIAL_TEXTURES                 = 15;


//
// Vertex formats
//

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

struct FHUDDrawVert {
    Float2   Position;
    Float2   TexCoord;
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

    //TEXTURE_COLORSPACE_GRAYSCALED
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

struct FTextureSampler {
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

// Texture pixel format
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

struct FTexturePixelFormat {
    ETexturePixelFormat Data;

    FTexturePixelFormat() : Data( TEXTURE_PF_BGRA8_SRGB ) {}
    FTexturePixelFormat( ETexturePixelFormat _PixelFormat ) : Data( _PixelFormat ) {}

    void operator = ( ETexturePixelFormat _PixelFormat ) { Data = _PixelFormat; }

    bool operator==( ETexturePixelFormat _PixelFormat ) const { return Data == _PixelFormat; }
    bool operator==( FTexturePixelFormat _PixelFormat ) const { return Data == _PixelFormat.Data; }
    bool operator!=( ETexturePixelFormat _PixelFormat ) const { return Data != _PixelFormat; }
    bool operator!=( FTexturePixelFormat _PixelFormat ) const { return Data != _PixelFormat.Data; }

    bool IsCompressed() const {
        return ( Data >> 6 ) & 1;
    }

    bool IsSRGB() const {
        return ( Data >> 4 ) & 1;
    }

    int SizeInBytesUncompressed() const {

        if ( IsCompressed() ) {
            GLogger.Printf( "SizeInBytesUncompressed: called for compressed pixel format\n" );
            return 0;
        }

        int bytesPerChannel = 1 << ( Data & 3 );
        int channelsCount = ( ( Data >> 2 ) & 3 ) + 1;

        return bytesPerChannel * channelsCount;
    }

    int BlockSizeCompressed() const {

        if ( !IsCompressed() ) {
            GLogger.Printf( "BlockSizeCompressed: called for uncompressed pixel format\n" );
            return 0;
        }

        // TODO
        AN_Assert( 0 );
        return 0;
    }

    int NumComponents() const {
        return ( ( Data >> 2 ) & 3 ) + 1;
    }
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
    MATERIAL_TYPE_PBR,
    MATERIAL_TYPE_HUD
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

    bool bDepthTest;

    // Helper. This flag allow renderer to optimize pipeline rendering order.
    //byte RenderOrder;

    int NumUniformVectors;

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
    FTextureSampler Samplers[MAX_MATERIAL_TEXTURES];
    int NumSamplers;

    // Shader source code
    char ShaderData[1];
};

//
// GPU Resources
//

class FResourceGPU;

class IGPUResourceOwner {
public:
    virtual void UploadResourceGPU( FResourceGPU * _Resource ) = 0;
};

class FResourceGPU {
public:
    IGPUResourceOwner * pOwner;

    FResourceGPU * pNext;
    FResourceGPU * pPrev;

    static FResourceGPU * GPUResources;
    static FResourceGPU * GPUResourcesTail;
};

class FTextureGPU : public FResourceGPU {
public:
    void * pHandleGPU;
};

class FBufferGPU : public FResourceGPU {
public:
    void * pHandleGPU;
};

class FMaterialGPU : public FResourceGPU {
public:
    EMaterialType MaterialType;

    void *  pSampler[MAX_MATERIAL_TEXTURES];
    int     NumSamplers;

    int     LightmapSlot;

    bool    bVertexTextureFetch;
    bool    bNoVertexDeform;

    struct {
        void * PBR;
        void * Unlit;
        void * HUD;
    } ShadeModel;
};

struct FMaterialFrameData {
    FMaterialGPU * Material;
    FTextureGPU * Textures[MAX_MATERIAL_TEXTURES];
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

struct FDebugDrawCmd {
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

struct FHUDDrawCmd {
    unsigned int    IndexCount;
    unsigned int    StartIndexLocation;
    Float2          ClipMins;
    Float2          ClipMaxs;
    EHUDDrawCmd     Type;
    EColorBlending  Blending;           // only for type DRAW_CMD_TEXTURE and DRAW_CMD_VIEWPORT
    EHUDSamplerType SamplerType;        // only for type DRAW_CMD_TEXTURE

    union {
        FTextureGPU *        Texture;               // HUD_DRAW_CMD_TEXTURE, HUD_DRAW_CMD_ALPHA
        FMaterialFrameData * MaterialFrameData;     // HUD_DRAW_CMD_MATERIAL
        int                  ViewportIndex;         // HUD_DRAW_CMD_VIEWPORT
    };
};

struct FHUDDrawList {
    int             VerticesCount;
    int             IndicesCount;
    FHUDDrawVert *  Vertices;
    unsigned short* Indices;
    int             CommandsCount;
    FHUDDrawCmd *   Commands;
    FHUDDrawList *  pNext;
};


//
// Render instance
//

struct FRenderInstance {
    FMaterialGPU *      Material;
    FMaterialFrameData *MaterialInstance;
    FBufferGPU *        VertexBuffer;
    FBufferGPU *        IndexBuffer;
    FBufferGPU *        WeightsBuffer;
    FBufferGPU *        VertexLightChannel;
    FBufferGPU *        LightmapUVChannel;
    FTextureGPU *       Lightmap;
    Float4              LightmapOffset;
    Float4x4            Matrix;
    Float3x3            ModelNormalToViewSpace;
    size_t              SkeletonOffset;
    size_t              SkeletonSize;
    unsigned int        IndexCount;
    unsigned int        StartIndexLocation;
    int                 BaseVertexLocation;
    byte                RenderingOrder;
    bool                bLightPass;
};


//
// Render frame
//

using FArrayOfDebugVertices = TPodArray< FDebugVertex, 1024 >;
using FArrayOfDebugIndices = TPodArray< unsigned int, 1024 >;
using FArrayOfDebugDrawCmds = TPodArray< FDebugDrawCmd >;

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
    Float3 ViewRightVec;
    Float3 ViewUpVec;
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

    FRenderView RenderViews[MAX_RENDER_VIEWS];
    int NumViews;

    TPodArray< FRenderInstance *, 1024 > Instances;

    FHUDDrawList * DrawListHead;
    FHUDDrawList * DrawListTail;

    FArrayOfDebugVertices DbgVertices;
    FArrayOfDebugIndices  DbgIndices;
    FArrayOfDebugDrawCmds DbgCmds;
};

struct FRenderFrontendDef {
    FRenderView * View;
    BvFrustum const * Frustum;
    int RenderingMask;
    int VisMarker;

    int PolyCount;
};


//
// Render backend interface
//

struct FTextureOffset {
    uint16_t Lod;
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct FTextureDimension {
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct FTextureRect {
    FTextureOffset Offset;
    FTextureDimension Dimension;
};

class IRenderBackend {
public:
    IRenderBackend( const char * _BackendName ) : BackendName( _BackendName ) {}

    virtual void PreInit() = 0;
    virtual void Initialize( void * _NativeWindowHandle ) = 0;
    virtual void Deinitialize() = 0;

    virtual void RenderFrame( FRenderFrame * _FrameData ) = 0;
    virtual void WaitGPU() = 0;

    virtual FTextureGPU * CreateTexture( IGPUResourceOwner * _Owner ) = 0;
    virtual void DestroyTexture( FTextureGPU * _Texture ) = 0;
    virtual void InitializeTexture1D( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) = 0;
    virtual void InitializeTexture1DArray( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) = 0;
    virtual void InitializeTexture2D( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) = 0;
    virtual void InitializeTexture2DArray( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) = 0;
    virtual void InitializeTexture3D( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) = 0;
    virtual void InitializeTextureCubemap( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) = 0;
    virtual void InitializeTextureCubemapArray( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) = 0;
    virtual void InitializeTexture2DNPOT( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) = 0;
    virtual void WriteTexture( FTextureGPU * _Texture, FTextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem ) = 0;
    virtual void ReadTexture( FTextureGPU * _Texture, FTextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) = 0;

    virtual FBufferGPU * CreateBuffer( IGPUResourceOwner * _Owner ) = 0;
    virtual void DestroyBuffer( FBufferGPU * _Buffer ) = 0;
    virtual void InitializeBuffer( FBufferGPU * _Buffer, size_t _SizeInBytes, bool _DynamicStorage ) = 0;
    virtual void WriteBuffer( FBufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) = 0;
    virtual void ReadBuffer( FBufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) = 0;

    virtual FMaterialGPU * CreateMaterial( IGPUResourceOwner * _Owner ) = 0;
    virtual void DestroyMaterial( FMaterialGPU * _Material ) = 0;
    virtual void InitializeMaterial( FMaterialGPU * _Material, FMaterialBuildData const * _BuildData ) = 0;

    virtual size_t AllocateJoints( size_t _JointsCount ) = 0;
    virtual void WriteJoints( size_t _Offset, size_t _JointsCount, Float3x4 const * _Matrices ) = 0;

    static void RegisterGPUResource( FResourceGPU * _Resource );
    static void UnregisterGPUResource( FResourceGPU * _Resource );
    static void UploadGPUResources();

    const char * GetName() { return BackendName; }

private:
    const char * BackendName = "Unnamed";
};

extern IRenderBackend * GRenderBackend;
