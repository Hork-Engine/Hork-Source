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

#include "Material.h"
#include "SoundResource.h"
#include "HitTest.h"
#include <Renderer/RenderDefs.h>
#include <Platform/Memory/PoolAllocator.h>

class AActor;
class World;
class Level;
class Texture;
class SceneComponent;
class BrushModel;
class ConvexHull;
class IndexedMesh;
class DebugRenderer;
struct VisArea;
struct PrimitiveDef;
struct PortalLink;
struct PrimitiveLink;

enum VSD_PRIMITIVE
{
    VSD_PRIMITIVE_BOX,
    VSD_PRIMITIVE_SPHERE
};

enum VSD_QUERY_MASK : uint32_t
{
    VSD_QUERY_MASK_VISIBLE   = 0x00000001,
    VSD_QUERY_MASK_INVISIBLE = 0x00000002,

    VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS   = 0x00000004,
    VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS = 0x00000008,

    VSD_QUERY_MASK_SHADOW_CAST    = 0x00000010,
    VSD_QUERY_MASK_NO_SHADOW_CAST = 0x00000020,

    VSD_QUERY_LIGHTMAP_EXPERIMENTAL = 0x00000040,

    // Reserved for future
    VSD_QUERY_MASK_RESERVED1 = 0x00000080,
    VSD_QUERY_MASK_RESERVED2 = 0x00000100,
    VSD_QUERY_MASK_RESERVED3 = 0x00000200,
    VSD_QUERY_MASK_RESERVED4 = 0x00000400,
    VSD_QUERY_MASK_RESERVED5 = 0x00000800,
    VSD_QUERY_MASK_RESERVED6 = 0x00001000,
    VSD_QUERY_MASK_RESERVED7 = 0x00002000,
    VSD_QUERY_MASK_RESERVED8 = 0x00004000,
    VSD_QUERY_MASK_RESERVED9 = 0x00008000,

    // User filter mask
    VSD_QUERY_MASK_USER0  = 0x00010000,
    VSD_QUERY_MASK_USER1  = 0x00020000,
    VSD_QUERY_MASK_USER2  = 0x00040000,
    VSD_QUERY_MASK_USER3  = 0x00080000,
    VSD_QUERY_MASK_USER4  = 0x00100000,
    VSD_QUERY_MASK_USER5  = 0x00200000,
    VSD_QUERY_MASK_USER6  = 0x00400000,
    VSD_QUERY_MASK_USER7  = 0x00800000,
    VSD_QUERY_MASK_USER8  = 0x01000000,
    VSD_QUERY_MASK_USER9  = 0x02000000,
    VSD_QUERY_MASK_USER10 = 0x04000000,
    VSD_QUERY_MASK_USER11 = 0x08000000,
    VSD_QUERY_MASK_USER12 = 0x10000000,
    VSD_QUERY_MASK_USER13 = 0x20000000,
    VSD_QUERY_MASK_USER14 = 0x40000000,
    VSD_QUERY_MASK_USER15 = 0x80000000,
};

HK_FLAG_ENUM_OPERATORS(VSD_QUERY_MASK)

enum VISIBILITY_GROUP : uint32_t
{
    VISIBILITY_GROUP_DEFAULT = 1,
    VISIBILITY_GROUP_SKYBOX  = 2,
    VISIBILITY_GROUP_TERRAIN = 4,

    VISIBILITY_GROUP_ALL = ~0u
};

HK_FLAG_ENUM_OPERATORS(VISIBILITY_GROUP)

enum LEVEL_VISIBILITY_METHOD
{
    LEVEL_VISIBILITY_PVS,
    LEVEL_VISIBILITY_PORTAL
};

enum SURFACE_FLAGS : uint8_t
{
    /** Planar surface */
    SURF_PLANAR = HK_BIT(0),

    /** Two sided surface
    NOTE: This flags affects only CPU culling and raycasting.
    You must also use a material with twosided property on to have visual effect. */
    SURF_TWOSIDED = HK_BIT(1),

    /** Planar tow sided surface */
    SURF_PLANAR_TWOSIDED_MASK = SURF_PLANAR | SURF_TWOSIDED
};

HK_FLAG_ENUM_OPERATORS(SURFACE_FLAGS)

using PRIMITIVE_RAYCAST_CALLBACK = bool (*)(PrimitiveDef const*            Self,
                                            Float3 const&                   RayStart,
                                            Float3 const&                   RayEnd,
                                            TPodVector<TriangleHitResult>& Hits);

using PRIMITIVE_RAYCAST_CLOSEST_CALLBACK = bool (*)(PrimitiveDef const* Self,
                                                    Float3 const&        RayStart,
                                                    Float3 const&        RayEnd,
                                                    TriangleHitResult&  Hit,
                                                    MeshVertex const**  pVertices);

using PRIMITIVE_EVALUATE_RAYCAST_RESULT = void (*)(PrimitiveDef*       Self,
                                                   Level const*        LightingLevel,
                                                   MeshVertex const*   pVertices,
                                                   MeshVertexUV const* pLightmapVerts,
                                                   int                  LightmapBlock,
                                                   unsigned int const*  pIndices,
                                                   Float3 const&        HitLocation,
                                                   Float2 const&        HitUV,
                                                   Float3*              Vertices,
                                                   Float2&              TexCoord,
                                                   Float3&              LightmapSample);
struct PrimitiveDef
{
    /** Owner component */
    SceneComponent* Owner{};

    /** List of areas where primitive located */
    PrimitiveLink* Links{};

    /** Next primitive in level */
    PrimitiveDef* Next{};

    /** Prev primitive in level */
    PrimitiveDef* Prev{};

    /** Next primitive in update list */
    PrimitiveDef* NextUpd{};

    /** Prev primitive in update list */
    PrimitiveDef* PrevUpd{};

    /** Callback for local raycast */
    PRIMITIVE_RAYCAST_CALLBACK RaycastCallback{};

    /** Callback for closest local raycast */
    PRIMITIVE_RAYCAST_CLOSEST_CALLBACK RaycastClosestCallback{};

    PRIMITIVE_EVALUATE_RAYCAST_RESULT EvaluateRaycastResult{};

    /** Primitive type */
    VSD_PRIMITIVE Type{VSD_PRIMITIVE_BOX};

    /** Primitive bounding shape. Used if type = VSD_PRIMITIVE_BOX */
    BvAxisAlignedBox Box{BvAxisAlignedBox::Empty()};

    /** Primitive bounding shape. Used if type = VSD_PRIMITIVE_SPHERE */
    BvSphere Sphere;

    /** Face plane. Used to perform face culling for planar surfaces */
    PlaneF Face;

    /** Visibility query group. See VSD_QUERY_MASK enum. */
    VSD_QUERY_MASK QueryGroup{};

    /** Visibility group. See VISIBILITY_GROUP enum. */
    VISIBILITY_GROUP VisGroup{VISIBILITY_GROUP_DEFAULT};

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark{};

    /** Primitve marked as visible. Used by VSD. */
    int VisPass{};

    /** Surface flags (SURFACE_FLAGS) */
    SURFACE_FLAGS Flags{};

    /** Is primitive outdoor/indoor */
    bool bIsOutdoor : 1;

    PrimitiveDef() : bIsOutdoor(false) {}

    void SetVisibilityGroup(VISIBILITY_GROUP VisibilityGroup)
    {
        VisGroup = VisibilityGroup;
    }

    VISIBILITY_GROUP GetVisibilityGroup() const
    {
        return VisGroup;
    }
};

struct PrimitiveLink
{
    /** The area */
    VisArea* Area;

    /** The primitive */
    PrimitiveDef* Primitive;

    /** Next primitive in the area */
    PrimitiveLink* NextInArea;

    /** Next link for the primitive */
    PrimitiveLink* Next;
};

struct PortalDef
{
    /** First hull vertex in array of vertices */
    int FirstVert;

    /** Hull vertex count */
    int NumVerts;

    /** Linked areas (front and back) */
    int Areas[2];
};

struct VisPortal
{
    /** Portal to areas */
    PortalLink* Portals[2];

    /** Visibility marker */
    int VisMark;

    /** Block visibility (for doors) */
    bool bBlocked;
};

struct PortalLink
{
    /** Area visible from the portal */
    VisArea* ToArea;

    /** Portal hull */
    ConvexHull* Hull;

    /** Portal plane */
    PlaneF Plane;

    /** Next portal inside an area */
    PortalLink* Next;

    /** Visibility portal */
    VisPortal* Portal;
};

struct VisArea
{
    /** Area bounding box */
    BvAxisAlignedBox Bounds; // FIXME: will be removed later?

    /** Linked portals */
    PortalLink* PortalList;

    /** Movable primitives inside the area */
    PrimitiveLink* Links;

    /** Non-movable primitives if AABB tree is not present */
    //PrimitiveLink * NonMovableLinks;

    /** AABB tree for non-movable primitives */
    //BvhTree * AABBTree;

    /** Baked surfaces attached to the area */
    int FirstSurface;

    /** Count of the baked surfaces attached to the area */
    int NumSurfaces;

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark;
};

struct VisibilityQuery
{
    /** View frustum planes */
    PlaneF const* FrustumPlanes[6];

    /** View origin */
    Float3 ViewPosition;

    /** View right vector */
    Float3 ViewRightVec;

    /** View up vector */
    Float3 ViewUpVec;

    /** Result filter */
    VISIBILITY_GROUP VisibilityMask;

    /** Result filter */
    VSD_QUERY_MASK QueryMask;
};

/** Box hit result */
struct BoxHitResult
{
    /** Box owner. Null for the surfaces. */
    SceneComponent* Object;

    Float3 LocationMin;
    Float3 LocationMax;
    float  DistanceMin;
    float  DistanceMax;

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }
};

/** Raycast primitive */
struct WorldRaycastPrimitive
{
    /** Primitive owner. Null for surfaces. */
    SceneComponent* Object;

    /** First hit in array of hits */
    int FirstHit;

    /** Hits count */
    int NumHits;

    /** Closest hit num */
    int ClosestHit;
};

/** Raycast result */
struct WorldRaycastResult
{
    /** Array of hits */
    TPodVector<TriangleHitResult> Hits;

    /** Array of primitives and surfaces */
    TPodVector<WorldRaycastPrimitive> Primitives;

    /** Sort raycast result by hit distance */
    void Sort()
    {
        struct SortPrimitives
        {
            TPodVector<TriangleHitResult> const& Hits;

            SortPrimitives(TPodVector<TriangleHitResult> const& _Hits) :
                Hits(_Hits) {}

            bool operator()(WorldRaycastPrimitive const& _A, WorldRaycastPrimitive const& _B)
            {
                const float hitDistanceA = Hits[_A.ClosestHit].Distance;
                const float hitDistanceB = Hits[_B.ClosestHit].Distance;

                return (hitDistanceA < hitDistanceB);
            }
        } SortPrimitives(Hits);

        // Sort by primitives distance
        std::sort(Primitives.ToPtr(), Primitives.ToPtr() + Primitives.Size(), SortPrimitives);

        struct ASortHit
        {
            bool operator()(TriangleHitResult const& _A, TriangleHitResult const& _B)
            {
                return (_A.Distance < _B.Distance);
            }
        } SortHit;

        // Sort by hit distance
        for (WorldRaycastPrimitive& primitive : Primitives)
        {
            std::sort(Hits.ToPtr() + primitive.FirstHit, Hits.ToPtr() + (primitive.FirstHit + primitive.NumHits), SortHit);
            primitive.ClosestHit = primitive.FirstHit;
        }
    }

    /** Clear raycast result */
    void Clear()
    {
        Hits.Clear();
        Primitives.Clear();
    }
};

/** Closest hit result */
struct WorldRaycastClosestResult
{
    /** Primitive owner. Null for surfaces. */
    SceneComponent* Object;

    /** Hit */
    TriangleHitResult TriangleHit;

    /** Hit fraction */
    float Fraction;

    /** Triangle vertices in world coordinates */
    Float3 Vertices[3];

    /** Triangle texture coordinate for the hit */
    Float2 Texcoord;

    Float3 LightmapSample_Experimental;

    /** Clear raycast result */
    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }
};

/** World raycast filter */
struct WorldRaycastFilter
{
    /** Filter objects by mask */
    VISIBILITY_GROUP VisibilityMask;
    /** VSD query mask */
    VSD_QUERY_MASK QueryMask;
    /** Sort result by the distance */
    bool bSortByDistance;

    WorldRaycastFilter()
    {
        VisibilityMask  = VISIBILITY_GROUP_ALL;
        QueryMask       = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        bSortByDistance = true;
    }
};

struct SurfaceDef
{
    /** Parent brush model */
    BrushModel* Model;

    /** Bounding box of the surface */
    BvAxisAlignedBox Bounds;

    /** Vertex offset */
    int FirstVertex;

    /** Vertex count */
    int NumVertices;

    /** Index offset */
    int FirstIndex;

    /** Index count */
    int NumIndices;
#if 0
    /** Patch size for the bezier surfaces */
    int PatchWidth;
    int PatchHeight;
#endif
    /** Index in array of materials */
    uint32_t MaterialIndex;

    /** Sort key. Used for surface batching. */
    uint32_t SortKey;

    /** Surface flags (SURFACE_FLAGS) */
    SURFACE_FLAGS Flags;

    /** Plane for planar surface */
    PlaneF Face;

    /** Lightmap atlas index */
    int LightmapBlock;

    /** Size of the lightmap */
    int LightmapWidth;

    /** Size of the lightmap */
    int LightmapHeight;

    /** Offset in the lightmap */
    int LightmapOffsetX;

    /** Offset in the lightmap */
    int LightmapOffsetY;

    /** Visibility query group. See VSD_QUERY_MASK enum. */
    int QueryGroup;

    /** Visibility group. See VISIBILITY_GROUP enum. */
    int VisGroup;

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark;

    /** Surface marked as visible. Used by VSD. */
    int VisPass;

    /** Drawable rendering order */
    //uint8_t RenderingOrder;

    /** Generate sort key. Call this after RenderingOrder/Model/MaterialIndex/LightmapBlock have changed. */
    void RegenerateSortKey()
    {
        // NOTE: 8 bits are still unused. We can use it in future.
        SortKey = 0
            //| ((uint64_t)(RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(HashTraits::Murmur3Hash32((uint64_t)Model) & 0xffffu) << 40u) | ((uint64_t)(HashTraits::Murmur3Hash32(MaterialIndex) & 0xffffu) << 24u) | ((uint64_t)(HashTraits::Murmur3Hash32(LightmapBlock) & 0xffffu) << 8u);
    }
};

struct BinarySpacePlane : PlaneF
{
    /** Plane axial type */
    uint8_t Type;

    HK_FORCEINLINE float DistFast(Float3 const& InPoint) const
    {
        return (Type < 3) ? (InPoint[Type] + D) : (Math::Dot(InPoint, Normal) + D);
    }
};

struct NodeBase
{
    /** Parent node */
    struct BinarySpaceNode* Parent;

    /** Visited mark */
    int ViewMark;

    /** Node bounding box (for culling) */
    BvAxisAlignedBox Bounds;
};

struct BinarySpaceNode : NodeBase
{
    /** Node split plane */
    BinarySpacePlane* Plane;

    /** Child indices */
    int ChildrenIdx[2];
};

struct BinarySpaceLeaf : NodeBase
{
    /** Leaf PVS cluster */
    int PVSCluster;

    /** Leaf PVS */
    byte const* Visdata;

    /** Baked audio */
    int AudioArea;

    /** Visibility area */
    VisArea* Area;
};

enum FRUSTUM_CULLING_TYPE
{
    FRUSTUM_CULLING_COMBINED,
    FRUSTUM_CULLING_SEPARATE,
    FRUSTUM_CULLING_SIMPLE
};

struct LightPortalDef
{
    /** First mesh vertex in array of vertices */
    int FirstVert;

    /** Mesh vertex count */
    int NumVerts;

    int FirstIndex;

    int NumIndices;
};

class BrushModel : public GCObject
{
public:
    /** Baked surface definitions */
    TVector<SurfaceDef> Surfaces;

    /** Baked surface vertex data */
    TVector<MeshVertex> Vertices;

    /** Baked surface vertex data */
    TVector<MeshVertexUV> LightmapVerts;

    /** Baked surface vertex data */
    TVector<MeshVertexLight> VertexLight;

    /** Baked surface triangle index data */
    TVector<unsigned int> Indices;

    /** Surface materials */
    TVector<TRef<MaterialInstance>> SurfaceMaterials;

    /** Lighting data will be used from that level. */
    TWeakRef<Level> ParentLevel;
};

enum LIGHTMAP_FORMAT
{
    LIGHTMAP_GRAYSCALED16_FLOAT,
    LIGHTMAP_RGBA16_FLOAT
};

struct NodeBaseDef
{
    /** Parent node */
    int Parent;

    /** Node bounding box (for culling) */
    BvAxisAlignedBox Bounds;
};

struct BinarySpaceNodeDef : NodeBaseDef
{
    /** Node split plane */
    int PlaneIndex;

    /** Child indices */
    int ChildrenIdx[2];
};

struct BinarySpaceLeafDef : NodeBaseDef
{
    /** Leaf PVS cluster */
    int PVSCluster;

    /** Leaf PVS */
    size_t VisdataOffset;

    /** Baked audio */
    int AudioArea;

    /** Visibility area */
    int AreaNum;
};

struct VisibilityAreaDef
{
    /** Area bounding box */
    BvAxisAlignedBox Bounds;

    /** Baked surfaces attached to the area */
    int FirstSurface;

    /** Count of the baked surfaces attached to the area */
    int NumSurfaces;
};

struct VisibilitySystemPVS
{
    byte const* Visdata;
    size_t      VisdataSize;
    int         ClustersCount;
    bool        bCompressedVisData;
};

class VisibilityLevel;

struct VisibilitySystemCreateInfo
{
    VisibilityAreaDef* Areas{};
    int                NumAreas{};

    int const* AreaSurfaces{};
    int        NumAreaSurfaces{};

    PortalDef const* Portals{};
    int             PortalsCount{};
    Float3 const*     HullVertices{};

    BinarySpacePlane const* Planes{};
    int                     NumPlanes{};

    BinarySpaceNodeDef const* Nodes{};
    int                       NumNodes{};

    BinarySpaceLeafDef const* Leafs{};
    int                       NumLeafs{};

    VisibilitySystemPVS const* PVS{};

    BrushModel* Model{};

    VisibilityLevel* PersistentLevel{};
};

struct PortalScissor
{
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;
};

struct PortalStack
{
    enum { MAX_CULL_PLANES = 5 };

    PlaneF            AreaFrustum[MAX_CULL_PLANES];
    int               PlanesCount;
    PortalLink const* Portal;
    PortalScissor     Scissor;
};

class VisibilityLevel;

class VisibilitySystem
{
public:
    VisibilitySystem();
    virtual ~VisibilitySystem();

    void RegisterLevel(VisibilityLevel* Level);
    void UnregisterLevel(VisibilityLevel* Level);

    /** Add primitive to the level */
    void AddPrimitive(PrimitiveDef* Primitive);

    /** Remove primitive from the level */
    void RemovePrimitive(PrimitiveDef* Primitive);

    /** Remove all primitives in the level */
    void RemovePrimitives();

    /** Mark primitive dirty */
    void MarkPrimitive(PrimitiveDef* Primitive);

    /** Mark all primitives in the level */
    void MarkPrimitives();

    /** Unmark all primitives in the level */
    void UnmarkPrimitives();

    void UpdatePrimitiveLinks();

    void DrawDebug(DebugRenderer* Renderer);

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<VisArea*>& Areas) const;

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TPodVector<VisArea*>& Areas) const;

    void QueryVisiblePrimitives(TPodVector<PrimitiveDef*>& VisPrimitives, TPodVector<SurfaceDef*>& VisSurfs, int* VisPass, VisibilityQuery const& Query) const;

    bool RaycastTriangles(WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    bool RaycastClosest(WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    bool RaycastBounds(TPodVector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    bool RaycastClosestBounds(BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    TPodVector<VisibilityLevel*> const& GetLevels() const { return Levels; }

    static TPoolAllocator<PrimitiveDef>  PrimitivePool;
    static TPoolAllocator<PrimitiveLink> PrimitiveLinkPool;

    static PrimitiveDef* AllocatePrimitive();
    static void          DeallocatePrimitive(PrimitiveDef* Primitive);

private:
    /** Unlink primitive from the level areas */
    void UnlinkPrimitive(PrimitiveDef* Primitive);

    TPodVector<VisibilityLevel*> Levels;

    PrimitiveDef* PrimitiveList          = nullptr;
    PrimitiveDef* PrimitiveListTail      = nullptr;
    PrimitiveDef* PrimitiveDirtyList     = nullptr;
    PrimitiveDef* PrimitiveDirtyListTail = nullptr;
};

class VisibilityLevel : public RefCounted
{
public:
    using ArrayOfNodes = TPodVector<BinarySpaceNode>;
    using ArrayOfLeafs = TPodVector<BinarySpaceLeaf>;

    VisibilityLevel(VisibilitySystemCreateInfo const& CreateInfo);
    virtual ~VisibilityLevel();

    /** Get level indoor bounding box */
    BvAxisAlignedBox const& GetIndoorBounds() const { return IndoorBounds; }

    /** Get level areas */
    TPodVector<VisArea> const& GetAreas() const { return Areas; }

    /** Get level outdoor area */
    VisArea const* GetOutdoorArea() const { return pOutdoorArea; }

    /** Find level leaf */
    int FindLeaf(Float3 const& _Position);

    /** Find level area */
    VisArea* FindArea(Float3 const& _Position);

    /** Mark potentially visible leafs. Uses PVS */
    int MarkLeafs(int _ViewLeaf);

    /** BSP leafs */
    ArrayOfLeafs const& GetLeafs() const { return Leafs; }

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<VisArea*>& Areas);

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TPodVector<VisArea*>& Areas);

    /** Add primitive to the level areas */
    static void AddPrimitiveToLevelAreas(TPodVector<VisibilityLevel*> const& Levels, PrimitiveDef* Primitive);

    void DrawDebug(DebugRenderer* Renderer);

    static void QueryVisiblePrimitives(TPodVector<VisibilityLevel*> const& Levels, TPodVector<PrimitiveDef*>& VisPrimitives, TPodVector<SurfaceDef*>& VisSurfs, int* VisPass, VisibilityQuery const& Query);

    static bool RaycastTriangles(TPodVector<VisibilityLevel*> const& Levels, WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

    static bool RaycastClosest(TPodVector<VisibilityLevel*> const& Levels, WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

    static bool RaycastBounds(TPodVector<VisibilityLevel*> const& Levels, TPodVector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

    static bool RaycastClosestBounds(TPodVector<VisibilityLevel*> const& Levels, BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

private:
    void CreatePortals(PortalDef const* Portals, int PortalsCount, Float3 const* HullVertices);

    byte const* LeafPVS(BinarySpaceLeaf const* _Leaf);

    byte const* DecompressVisdata(byte const* _Data);

    void QueryOverplapAreas_r(int NodeIndex, BvAxisAlignedBox const& Bounds, TPodVector<VisArea*>& Areas);
    void QueryOverplapAreas_r(int NodeIndex, BvSphere const& Bounds, TPodVector<VisArea*>& Areas);

    void AddBoxRecursive(int NodeIndex, PrimitiveDef* Primitive);
    void AddSphereRecursive(int NodeIndex, PrimitiveDef* Primitive);

    void AddPrimitiveToArea(VisArea* Area, PrimitiveDef* Primitive);

    void ProcessLevelVisibility(struct VisibilityQueryContext& QueryContext, struct VisibilityQueryResult& QueryResult);

    void LevelTraverse_r(int NodeIndex, int CullBits);

    bool CullNode(PlaneF const Frustum[PortalStack::MAX_CULL_PLANES], BvAxisAlignedBox const& Bounds, int& CullBits);

    void FlowThroughPortals_r(VisArea const* Area);

    bool CalcPortalStack(PortalStack* OutStack, PortalStack const* PrevStack, PortalLink const* Portal);

    struct PortalHull* CalcPortalWinding(PortalLink const* Portal, PortalStack const* Stack);

    void CalcPortalScissor(PortalScissor& OutScissor, PortalHull const* Hull, PortalStack const* Stack);

    void CullPrimitives(VisArea const* Area, PlaneF const* CullPlanes, const int CullPlanesCount);

    bool FaceCull(PrimitiveDef const* Primitive);
    bool FaceCull(SurfaceDef const* Surface);

    enum HIT_PROXY_TYPE
    {
        HIT_PROXY_UNKNOWN,
        HIT_PROXY_PRIMITIVE,
        HIT_PROXY_SURFACE
    };

    struct VisRaycast
    {
        Float3 RayStart;
        Float3 RayEnd;
        Float3 RayDir;
        Float3 InvRayDir;
        float  RayLength;
        float  HitDistanceMin;
        float  HitDistanceMax; // only for bounds test

        // For closest raycast
        HIT_PROXY_TYPE HitProxyType;
        union
        {
            PrimitiveDef* HitPrimitive;
            SurfaceDef*   HitSurface;
        };
        Float3              HitLocation;
        Float2              HitUV;
        Float3              HitNormal;
        MeshVertex const*   pVertices;
        MeshVertexUV const* pLightmapVerts;
        int                 LightmapBlock;
        Level const*        LightingLevel;
        unsigned int        Indices[3];
        MaterialInstance*   Material;
        int                 NumHits; // For debug

        bool bClosest;

        VSD_QUERY_MASK VisQueryMask;
        VISIBILITY_GROUP VisibilityMask;
    };
    void ProcessLevelRaycast(VisRaycast& Raycast, WorldRaycastResult& Result);
    void ProcessLevelRaycastClosest(VisRaycast& Raycast);
    void ProcessLevelRaycastBounds(VisRaycast& Raycast, TPodVector<BoxHitResult>& Result);
    void ProcessLevelRaycastClosestBounds(VisRaycast& Raycast);
    void RaycastSurface(SurfaceDef* Self);
    void RaycastPrimitive(PrimitiveDef* Self);
    void RaycastArea(VisArea* Area);
    void RaycastPrimitiveBounds(VisArea* Area);
    void LevelRaycast_r(int NodeIndex);
    bool LevelRaycast2_r(int NodeIndex, Float3 const& RayStart, Float3 const& RayEnd);
    void LevelRaycastBounds_r(int NodeIndex);
    bool LevelRaycastBounds2_r(int NodeIndex, Float3 const& RayStart, Float3 const& RayEnd);
    void LevelRaycastPortals_r(VisArea* Area);
    void LevelRaycastBoundsPortals_r(VisArea* Area);

    VisibilityLevel* PersistentLevel{};

    /** Level portals */
    TPodVector<VisPortal> Portals;

    TVector<ConvexHull> PortalHulls;

    /** Links between the portals and areas */
    TPodVector<PortalLink> AreaLinks;

    /** Level indoor areas */
    TPodVector<VisArea> Areas;

    /** Level outdoor area */
    VisArea OutdoorArea;
    VisArea* pOutdoorArea;

    BvAxisAlignedBox IndoorBounds;

    /** Node split planes */
    TPodVector<BinarySpacePlane> SplitPlanes;

    /** BSP nodes */
    ArrayOfNodes Nodes;

    /** BSP leafs */
    ArrayOfLeafs Leafs;

    /** PVS data */
    byte* Visdata = nullptr;

    byte* DecompressedVisData = nullptr;

    /** Is PVS data compressed or not (ZRLE) */
    bool bCompressedVisData = false;

    /** Count of a clusters in PVS data */
    int PVSClustersCount = 0;

    /** Visibility method */
    LEVEL_VISIBILITY_METHOD VisibilityMethod = LEVEL_VISIBILITY_PORTAL;

    /** Node visitor mark */
    int ViewMark = 0;

    /** Cluster index for view origin */
    int ViewCluster = -1;

    /** Surface to area attachments */
    TPodVector<int> AreaSurfaces;

    /** Baked surface data */
    TRef<BrushModel> Model;

    // Visibility query temp vars:
    static int               VisQueryMarker;
    int                      CachedSignBits[PortalStack::MAX_CULL_PLANES]; // sign bits of ViewFrustum planes
    PlaneF*                  ViewFrustum;
    int                      ViewFrustumPlanes;
    int                      NodeViewMark;
    VisibilityQueryContext* pQueryContext;
    VisibilityQueryResult*  pQueryResult;

    //Raycast temp vars
    VisRaycast*               pRaycast;
    WorldRaycastResult*       pRaycastResult;
    TPodVector<BoxHitResult>* pBoundsRaycastResult;
};
