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

#include <Engine/Image/Image.h>
#include <Engine/Core/Color.h>
#include <Engine/Core/Containers/Vector.h>
#include <Engine/Math/Quat.h>
#include <Engine/Geometry/VertexFormat.h>
#include <Engine/Geometry/BV/BvFrustum.h>

#include <Engine/RenderCore/Device.h>

HK_NAMESPACE_BEGIN

//
// Common constants
//

/** Max textures per material */
constexpr int MAX_MATERIAL_TEXTURES = 11; // Reserved texture slots for AOLookup, ClusterItemTBO, ClusterLookup, ShadowMapShadow, Lightmap

constexpr int MAX_MATERIAL_UNIFORMS        = 16;
constexpr int MAX_MATERIAL_UNIFORM_VECTORS = 16 >> 2;

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

constexpr int TERRAIN_CLIPMAP_SIZE = 256;

struct TerrainVertex
{
    /*unsigned*/ short X;
    /*unsigned*/ short Y;
};

struct DebugVertex
{
    Float3   Position;
    uint32_t Color;
};

enum TEXTURE_COLOR_SPACE
{
    TEXTURE_COLOR_SPACE_RGBA,
    TEXTURE_COLOR_SPACE_SRGB_ALPHA,
    TEXTURE_COLOR_SPACE_YCOCG,
    TEXTURE_COLOR_SPACE_GRAYSCALED

    //TEXTURE_COLORSPACE_RGBA_INT
    //TEXTURE_COLORSPACE_RGBA_UINT
};

enum TEXTURE_FILTER
{
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_MIPMAP_NEAREST,
    TEXTURE_FILTER_MIPMAP_BILINEAR,
    TEXTURE_FILTER_MIPMAP_NLINEAR,
    TEXTURE_FILTER_MIPMAP_TRILINEAR
};

enum TEXTURE_ADDRESS
{
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_MIRROR,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_BORDER,
    TEXTURE_ADDRESS_MIRROR_ONCE
};

struct TextureSampler
{
    TEXTURE_TYPE    TextureType;
    TEXTURE_FILTER  Filter;
    TEXTURE_ADDRESS AddressU;
    TEXTURE_ADDRESS AddressV;
    TEXTURE_ADDRESS AddressW;
    float           MipLODBias;
    float           Anisotropy;
    float           MinLod;
    float           MaxLod;
};


//
// Material
//

enum MATERIAL_TYPE
{
    MATERIAL_TYPE_UNLIT,
    MATERIAL_TYPE_BASELIGHT,
    MATERIAL_TYPE_PBR,
    MATERIAL_TYPE_HUD,
    MATERIAL_TYPE_POSTPROCESS
};

enum MATERIAL_DEPTH_HACK
{
    MATERIAL_DEPTH_HACK_NONE,
    MATERIAL_DEPTH_HACK_WEAPON,
    MATERIAL_DEPTH_HACK_SKYBOX
};

enum BLENDING_MODE
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

enum TESSELLATION_METHOD : uint8_t
{
    TESSELLATION_DISABLED,
    TESSELLATION_FLAT,
    TESSELLATION_PN
};

/** Rendering priorities for materials. RENDERING_PRIORITY is mixed with RENDERING_GEOMETRY_PRIORITY. */
enum RENDERING_PRIORITY : uint8_t
{
    /** Weapon rendered first */
    RENDERING_PRIORITY_WEAPON = 0 << 4,

    RENDERING_PRIORITY_FOLIAGE  = 1 << 4,

    /** Default priority */
    RENDERING_PRIORITY_DEFAULT    = 2 << 4,

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
};

/** Rendering priorities for geometry. RENDERING_PRIORITY is mixed with RENDERING_GEOMETRY_PRIORITY. */
enum RENDERING_GEOMETRY_PRIORITY : uint8_t
{
    /** Static geometry */
    RENDERING_GEOMETRY_PRIORITY_STATIC = 0,

    /** Dynamic geometry */
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

struct MaterialSource
{
    /** The source name */
    String SourceName;

    /** Source code */
    String Code;

    MaterialSource() = default;
    MaterialSource(StringView inSourceName, StringView inCode) :
        SourceName(inSourceName), Code(inCode)
    {}
};

class CompiledMaterial : public RefCounted
{
public:
    /** Material type (Unlit,baselight,pbr,etc) */
    MATERIAL_TYPE Type{MATERIAL_TYPE_PBR};

    /** Blending mode (FIXME: only for UNLIT materials?) */
    BLENDING_MODE Blending{COLOR_BLENDING_DISABLED};

    TESSELLATION_METHOD TessellationMethod{TESSELLATION_DISABLED};

    RENDERING_PRIORITY RenderingPriority{RENDERING_PRIORITY_DEFAULT};

    /** Lightmap binding unit */
    uint32_t LightmapSlot{};

    /** Texture binding count for different passes. This allow renderer to optimize sampler/texture bindings
    during rendering. */
    int DepthPassTextureCount{};
    int LightPassTextureCount{};
    int WireframePassTextureCount{};
    int NormalsPassTextureCount{};
    int ShadowMapPassTextureCount{};

    int NumUniformVectors{};

    /** Material samplers */
    StaticVector<TextureSampler, MAX_MATERIAL_TEXTURES> Samplers;

    /** Material shaders */
    Vector<MaterialSource> Shaders;

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

    CompiledMaterial()
    {
        bHasVertexDeform          = false;
        bDepthTest_EXPERIMENTAL   = false;
        bNoCastShadow             = false;
        bAlphaMasking             = false;
        bShadowMapMasking         = false;
        bDisplacementAffectShadow = false;
        bTranslucent              = false;
        bTwoSided                 = false;
    }

    CompiledMaterial(IBinaryStreamReadInterface& Stream)
    {
        Read(Stream);
    }

    ~CompiledMaterial() = default;

    CompiledMaterial(CompiledMaterial&& Rhs) noexcept = default;

    CompiledMaterial& operator=(CompiledMaterial&& Rhs) noexcept = default;

    void AddShader(StringView SourceName, StringView SourceCode)
    {
        Shaders.EmplaceBack(SourceName, SourceCode);
    }

    void Read(IBinaryStreamReadInterface& Stream)
    {
        Type                      = (MATERIAL_TYPE)Stream.ReadUInt8();
        Blending                  = (BLENDING_MODE)Stream.ReadUInt8();
        TessellationMethod        = (TESSELLATION_METHOD)Stream.ReadUInt8();
        RenderingPriority         = (RENDERING_PRIORITY)Stream.ReadUInt8();
        LightmapSlot              = Stream.ReadUInt16();
        DepthPassTextureCount     = Stream.ReadUInt8();
        LightPassTextureCount     = Stream.ReadUInt8();
        WireframePassTextureCount = Stream.ReadUInt8();
        NormalsPassTextureCount   = Stream.ReadUInt8();
        ShadowMapPassTextureCount = Stream.ReadUInt8();
        bHasVertexDeform          = Stream.ReadBool();
        bDepthTest_EXPERIMENTAL   = Stream.ReadBool();
        bNoCastShadow             = Stream.ReadBool();
        bAlphaMasking             = Stream.ReadBool();
        bShadowMapMasking         = Stream.ReadBool();
        bDisplacementAffectShadow = Stream.ReadBool();
        //bParallaxMappingSelfShadowing = Stream.ReadBool(); // TODO
        bTranslucent      = Stream.ReadBool();
        bTwoSided         = Stream.ReadBool();
        NumUniformVectors = Stream.ReadUInt8();

        Samplers.Clear();
        Samplers.Resize(Stream.ReadUInt8());
        for (auto& sampler : Samplers)
        {
            sampler.TextureType = (TEXTURE_TYPE)Stream.ReadUInt8();
            sampler.Filter      = (TEXTURE_FILTER)Stream.ReadUInt8();
            sampler.AddressU    = (TEXTURE_ADDRESS)Stream.ReadUInt8();
            sampler.AddressV    = (TEXTURE_ADDRESS)Stream.ReadUInt8();
            sampler.AddressW    = (TEXTURE_ADDRESS)Stream.ReadUInt8();
            sampler.MipLODBias  = Stream.ReadFloat();
            sampler.Anisotropy  = Stream.ReadFloat();
            sampler.MinLod      = Stream.ReadFloat();
            sampler.MaxLod      = Stream.ReadFloat();
        }

        auto numShaders = Stream.ReadUInt16();

        Shaders.Clear();
        Shaders.Reserve(numShaders);

        String sourceName, sourceCode;
        for (int i = 0; i < numShaders; i++)
        {
            sourceName = Stream.ReadString();
            sourceCode = Stream.ReadString();
            AddShader(sourceName, sourceCode);
        }
    }

    void Write(IBinaryStreamWriteInterface& Stream) const
    {
        Stream.WriteUInt8(Type);
        Stream.WriteUInt8(Blending);
        Stream.WriteUInt8(TessellationMethod);
        Stream.WriteUInt8(RenderingPriority);        
        Stream.WriteUInt16(LightmapSlot);
        Stream.WriteUInt8(DepthPassTextureCount);
        Stream.WriteUInt8(LightPassTextureCount);
        Stream.WriteUInt8(WireframePassTextureCount);
        Stream.WriteUInt8(NormalsPassTextureCount);
        Stream.WriteUInt8(ShadowMapPassTextureCount);
        Stream.WriteBool(bHasVertexDeform);
        Stream.WriteBool(bDepthTest_EXPERIMENTAL);
        Stream.WriteBool(bNoCastShadow);
        Stream.WriteBool(bAlphaMasking);
        Stream.WriteBool(bShadowMapMasking);
        Stream.WriteBool(bDisplacementAffectShadow);
        //Stream.WriteBool( bParallaxMappingSelfShadowing ); // TODO
        Stream.WriteBool(bTranslucent);
        Stream.WriteBool(bTwoSided);
        Stream.WriteUInt8(NumUniformVectors);
        Stream.WriteUInt8(Samplers.Size());
        for (auto& sampler : Samplers)
        {
            Stream.WriteUInt8(sampler.TextureType);
            Stream.WriteUInt8(sampler.Filter);
            Stream.WriteUInt8(sampler.AddressU);
            Stream.WriteUInt8(sampler.AddressV);
            Stream.WriteUInt8(sampler.AddressW);
            Stream.WriteFloat(sampler.MipLODBias);
            Stream.WriteFloat(sampler.Anisotropy);
            Stream.WriteFloat(sampler.MinLod);
            Stream.WriteFloat(sampler.MaxLod);
        }

        Stream.WriteUInt16(Shaders.Size());

        for (MaterialSource const& s : Shaders)
        {
            Stream.WriteString(s.SourceName);
            Stream.WriteString(s.Code);
        }
    }    
};

class MaterialGPU;

struct MaterialFrameData
{
    MaterialGPU*                   Material;
    RenderCore::ITexture*          Textures[MAX_MATERIAL_TEXTURES];
    int                            NumTextures;
    Float4                         UniformVectors[4];
    int                            NumUniformVectors;
    //class VirtualTextureResource* VirtualTexture;
};


//
// Debug draw
//

enum DBG_DRAW_CMD
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

struct DebugDrawCmd
{
    DBG_DRAW_CMD Type;
    int          FirstVertex;
    int          NumVertices;
    int          FirstIndex;
    int          NumIndices;
};

//
// CANVAS
//

enum CANVAS_SHADER_TYPE
{
    CANVAS_SHADER_FILLGRAD,
    CANVAS_SHADER_FILLIMG,
    CANVAS_SHADER_SIMPLE,
    CANVAS_SHADER_IMAGE
};

enum CANVAS_IMAGE_FLAGS : uint32_t
{
    CANVAS_IMAGE_DEFAULT = 0,

    /** Repeat image in X direction. */
    CANVAS_IMAGE_REPEATX = 1 << 1,

    /** Repeat image in Y direction. */
    CANVAS_IMAGE_REPEATY = 1 << 2,

    /** Flips(inverses) image in Y direction when rendered. */
    CANVAS_IMAGE_FLIPY = 1 << 3,

    /** Image data has premultiplied alpha. */
    CANVAS_IMAGE_PREMULTIPLIED = 1 << 4,

    /** Image interpolation is Nearest, default is Linear. */
    CANVAS_IMAGE_NEAREST = 1 << 5,
};

HK_FLAG_ENUM_OPERATORS(CANVAS_IMAGE_FLAGS)

struct alignas(16) CanvasUniforms
{
    Color4 InnerColor;
    Color4 OuterColor;

    Float3x4 ScissorMat;
    Float3x4 PaintMat;

    float ScissorExt[2];
    float ScissorScale[2];

    float Extent[2];
    float Radius;
    float Feather;

    float StrokeMult;
    float StrokeThr;
    int TexType;
    int Type;
};

enum CANVAS_DRAW_COMMAND : uint8_t
{
    CANVAS_DRAW_COMMAND_NONE = 0,
    CANVAS_DRAW_COMMAND_FILL,
    CANVAS_DRAW_COMMAND_CONVEXFILL,
    CANVAS_DRAW_COMMAND_STROKE,
    CANVAS_DRAW_COMMAND_STENCIL_STROKE,
    CANVAS_DRAW_COMMAND_TRIANGLES
};

enum CANVAS_COMPOSITE : uint8_t
{
    /**
    Display the source image wherever the source image is opaque.
    Display the destination image elsewhere.
    */
    CANVAS_COMPOSITE_SOURCE_OVER,

    /**
    Display the source image wherever both the source image and destination image are opaque.
    Display transparency elsewhere.
    */
    CANVAS_COMPOSITE_SOURCE_IN,

    /**
    The source image is copied out of the destination image.
    The source image is displayed where the source is opaque and the destination is transparent.
    Other regions are transparent.
    */
    CANVAS_COMPOSITE_SOURCE_OUT,

    /**
    Display the source image wherever both images are opaque.
    Display the destination image wherever the destination image is opaque but the source image is transparent.
    Display transparency elsewhere.
    */
    CANVAS_COMPOSITE_ATOP,

    /**
    Display the source image wherever the source image is opaque.
    Display the destination image elsewhere.
    lighter	A plus B
    */
    CANVAS_COMPOSITE_DESTINATION_OVER,

    /**
    Display the source image wherever both the source image and destination image are opaque.
    Display transparency elsewhere.
    */
    CANVAS_COMPOSITE_DESTINATION_IN,

    /**
    The source image is copied out of the destination image.
    The source image is displayed where the source is opaque and the destination is transparent.
    Other regions are transparent.
    */
    CANVAS_COMPOSITE_DESTINATION_OUT,

    /**
    Display the source image wherever both images are opaque.
    Display the destination image wherever the destination image is opaque but the source image is transparent.
    Display transparency elsewhere.
    */
    CANVAS_COMPOSITE_DESTINATION_ATOP,

    /**
    Display the sum of the source image and destination image, with color values approaching 255 (100%) as a limit.
    */
    CANVAS_COMPOSITE_LIGHTER,

    /**
    Display the source image instead of the destination image.
    */
    CANVAS_COMPOSITE_COPY,

    /**
    Exclusive OR of the source image and destination image.
    */
    CANVAS_COMPOSITE_XOR,

    CANVAS_COMPOSITE_LAST = CANVAS_COMPOSITE_XOR
};

class TextureView;

struct CanvasDrawCmd
{
    RenderCore::ITexture* Texture;
    CANVAS_DRAW_COMMAND   Type;
    CANVAS_COMPOSITE      Composite;
    CANVAS_IMAGE_FLAGS    TextureFlags;
    int                   FirstPath;
    int                   PathCount;
    int                   FirstVertex;
    int                   VertexCount;
    int                   UniformOffset;
};

struct CanvasPath
{
    int FillOffset;
    int FillCount;
    int StrokeOffset;
    int StrokeCount;
};

struct CanvasVertex
{
    float x, y, u, v;
};

class CanvasDrawData
{
public:
    CanvasDrawCmd* DrawCommands{};
    int            MaxDrawCommands{};
    int            NumDrawCommands{};
    CanvasPath*    Paths{};
    int            MaxPaths{};
    int            NumPaths{};
    CanvasVertex*  Vertices{};
    int            MaxVerts{};
    int            VertexCount{};
    uint8_t*       Uniforms{};
    int            MaxUniforms{};
    int            UniformCount{};
};


/**

Directional light render instance

*/
struct DirectionalLightInstance
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
struct RenderInstance
{
    MaterialGPU*       Material;
    MaterialFrameData* MaterialInstance;

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

    bool bPerObjectMotionBlur;

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
        SortKey = ((uint64_t)(Priority) << 56u) | ((uint64_t)(HashTraits::Murmur3Hash64((uint64_t)Material) & 0xffffu) << 40u) | ((uint64_t)(HashTraits::Murmur3Hash64((uint64_t)MaterialInstance) & 0xffffu) << 24u) | ((uint64_t)(HashTraits::Murmur3Hash64(Mesh) & 0xffffu) << 8u);
    }
};


/**

Shadowmap render instance

*/
struct ShadowRenderInstance
{
    MaterialGPU*         Material;
    MaterialFrameData*   MaterialInstance;
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
        SortKey = ((uint64_t)(Priority) << 56u) | ((uint64_t)(HashTraits::Murmur3Hash64((uint64_t)Material) & 0xffffu) << 40u) | ((uint64_t)(HashTraits::Murmur3Hash64((uint64_t)MaterialInstance) & 0xffffu) << 24u) | ((uint64_t)(HashTraits::Murmur3Hash64(Mesh) & 0xffffu) << 8u);
    }
};


/**

Light portal render instance

*/
struct LightPortalRenderInstance
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
struct LightShadowmap
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
struct ClusterHeader
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
struct ClusterPackedIndex
{
    uint32_t Indices;
};


/**

Light type (point/spot)

*/
enum CLUSTER_LIGHT_TYPE
{
    CLUSTER_LIGHT_POINT,
    CLUSTER_LIGHT_SPOT,
};


/**

Point & spot light shader parameters

*/
struct LightParameters
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
struct ProbeParameters
{
    Float3 Position;
    float  Radius;

    uint64_t IrradianceMap;
    uint64_t ReflectionMap;
};


/**

Terrain patch parameters

*/
struct TerrainPatchInstance
{
    Int2   VertexScale;
    Int2   VertexTranslate;
    Int2   TexcoordOffset;
    Color4 QuadColor; // Just for debug. Will be removed later
};


/**

Terrain render instance

*/
struct TerrainRenderInstance
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


enum ANTIALIASING_TYPE
{
    ANTIALIASING_DISABLED,
    ANTIALIASING_SMAA,
    ANTIALIASING_FXAA
};


/**

Rendering data for one view
Keep it POD

*/
struct RenderViewData
{
    /** Local frame number */
    int FrameNumber;

    /** Viewport size (scaled by dynamic resolution) */
    uint32_t Width;
    /** Viewport size (scaled by dynamic resolution) */
    uint32_t Height;

    /** Viewport size on previous frame (scaled by dynamic resolution) */
    uint32_t WidthP;
    /** Viewport size on previous frame (scaled by dynamic resolution) */
    uint32_t HeightP;

    /** Real viewport size */
    uint32_t WidthR;
    /** Real viewport size */
    uint32_t HeightR;

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
    bool     bAllowHBAO;
    bool     bAllowMotionBlur;
    ANTIALIASING_TYPE AntialiasingType;

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

    /** Final texture data */
    RenderCore::ITexture* RenderTarget;

    /** Deinterleaved depth buffers for HBAO rendering */
    RenderCore::ITexture* HBAOMaps;

    /** Virtual texture feedback data (experimental) */
    class VirtualTextureFeedback* VTFeedback;

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
    LightParameters* PointLights;
    int              NumPointLights;
    size_t           PointLightsStreamHandle;
    size_t           PointLightsStreamSize;

    int FirstOmnidirectionalShadowMap;
    int NumOmnidirectionalShadowMaps;

    /** Reflection probes for render view */
    ProbeParameters* Probes;
    int              NumProbes;
    size_t           ProbeStreamHandle;
    size_t           ProbeStreamSize;

    /** Cluster headers */
    ClusterHeader* ClusterLookup;
    size_t         ClusterLookupStreamHandle;

    /** Cluster packed indices */
    ClusterPackedIndex* ClusterPackedIndices;
    size_t              ClusterPackedIndicesStreamHandle;
    int                 ClusterPackedIndexCount;

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
struct RenderFrameData
{
    /** Game tick */
    int FrameNumber;

    /** Render views */
    RenderViewData* RenderViews;
    /** Render view count */
    int NumViews;

    /** Opaque instances */
    Vector<RenderInstance*> Instances;
    /** Translucent instances */
    Vector<RenderInstance*> TranslucentInstances;
    /** Outline instances */
    Vector<RenderInstance*> OutlineInstances;
    /** Shadowmap instances */
    Vector<ShadowRenderInstance*> ShadowInstances;
    /** Light portal instances */
    Vector<LightPortalRenderInstance*> LightPortals;
    /** Directional light instances */
    Vector<DirectionalLightInstance*> DirectionalLights;
    /** Shadow maps */
    Vector<LightShadowmap> LightShadowmaps;
    /** Terrain instances */
    Vector<TerrainRenderInstance*> TerrainInstances;

    /** Canvas draw commands */
    CanvasDrawData const* pCanvasDrawData;
    size_t CanvasVertexData;

    /** Debug draw commands */
    DebugDrawCmd const* DbgCmds;
    size_t DbgVertexStreamOffset;
    size_t DbgIndexStreamOffset;
};

HK_NAMESPACE_END
