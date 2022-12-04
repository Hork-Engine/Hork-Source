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
class AWorld;
class ALevel;
class ATexture;
class ASceneComponent;
class ABrushModel;
class AConvexHull;
class AIndexedMesh;
class ADebugRenderer;
struct SVisArea;
struct SPrimitiveDef;
struct SPortalLink;
struct SPrimitiveLink;

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

using PRIMITIVE_RAYCAST_CALLBACK = bool (*)(SPrimitiveDef const*            Self,
                                            Float3 const&                   RayStart,
                                            Float3 const&                   RayEnd,
                                            TPodVector<STriangleHitResult>& Hits);

using PRIMITIVE_RAYCAST_CLOSEST_CALLBACK = bool (*)(SPrimitiveDef const* Self,
                                                    Float3 const&        RayStart,
                                                    Float3 const&        RayEnd,
                                                    STriangleHitResult&  Hit,
                                                    SMeshVertex const**  pVertices);

using PRIMITIVE_EVALUATE_RAYCAST_RESULT = void (*)(SPrimitiveDef*       Self,
                                                   ALevel const*        LightingLevel,
                                                   SMeshVertex const*   pVertices,
                                                   SMeshVertexUV const* pLightmapVerts,
                                                   int                  LightmapBlock,
                                                   unsigned int const*  pIndices,
                                                   Float3 const&        HitLocation,
                                                   Float2 const&        HitUV,
                                                   Float3*              Vertices,
                                                   Float2&              TexCoord,
                                                   Float3&              LightmapSample);
struct SPrimitiveDef
{
    /** Owner component */
    ASceneComponent* Owner{};

    /** List of areas where primitive located */
    SPrimitiveLink* Links{};

    /** Next primitive in level */
    SPrimitiveDef* Next{};

    /** Prev primitive in level */
    SPrimitiveDef* Prev{};

    /** Next primitive in update list */
    SPrimitiveDef* NextUpd{};

    /** Prev primitive in update list */
    SPrimitiveDef* PrevUpd{};

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

    SPrimitiveDef() : bIsOutdoor(false) {}

    void SetVisibilityGroup(VISIBILITY_GROUP VisibilityGroup)
    {
        VisGroup = VisibilityGroup;
    }

    VISIBILITY_GROUP GetVisibilityGroup() const
    {
        return VisGroup;
    }
};

struct SPrimitiveLink
{
    /** The area */
    SVisArea* Area;

    /** The primitive */
    SPrimitiveDef* Primitive;

    /** Next primitive in the area */
    SPrimitiveLink* NextInArea;

    /** Next link for the primitive */
    SPrimitiveLink* Next;
};

struct SPortalDef
{
    /** First hull vertex in array of vertices */
    int FirstVert;

    /** Hull vertex count */
    int NumVerts;

    /** Linked areas (front and back) */
    int Areas[2];
};

struct SVisPortal
{
    /** Portal to areas */
    SPortalLink* Portals[2];

    /** Visibility marker */
    int VisMark;

    /** Block visibility (for doors) */
    bool bBlocked;
};

struct SPortalLink
{
    /** Area visible from the portal */
    SVisArea* ToArea;

    /** Portal hull */
    AConvexHull* Hull;

    /** Portal plane */
    PlaneF Plane;

    /** Next portal inside an area */
    SPortalLink* Next;

    /** Visibility portal */
    SVisPortal* Portal;
};

struct SVisArea
{
    /** Area bounding box */
    BvAxisAlignedBox Bounds; // FIXME: will be removed later?

    /** Linked portals */
    SPortalLink* PortalList;

    /** Movable primitives inside the area */
    SPrimitiveLink* Links;

    /** Non-movable primitives if AABB tree is not present */
    //SPrimitiveLink * NonMovableLinks;

    /** AABB tree for non-movable primitives */
    //BvhTree * AABBTree;

    /** Baked surfaces attached to the area */
    int FirstSurface;

    /** Count of the baked surfaces attached to the area */
    int NumSurfaces;

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark;
};

struct SVisibilityQuery
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
struct SBoxHitResult
{
    /** Box owner. Null for the surfaces. */
    ASceneComponent* Object;

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
struct SWorldRaycastPrimitive
{
    /** Primitive owner. Null for surfaces. */
    ASceneComponent* Object;

    /** First hit in array of hits */
    int FirstHit;

    /** Hits count */
    int NumHits;

    /** Closest hit num */
    int ClosestHit;
};

/** Raycast result */
struct SWorldRaycastResult
{
    /** Array of hits */
    TPodVector<STriangleHitResult> Hits;

    /** Array of primitives and surfaces */
    TPodVector<SWorldRaycastPrimitive> Primitives;

    /** Sort raycast result by hit distance */
    void Sort()
    {
        struct ASortPrimitives
        {
            TPodVector<STriangleHitResult> const& Hits;

            ASortPrimitives(TPodVector<STriangleHitResult> const& _Hits) :
                Hits(_Hits) {}

            bool operator()(SWorldRaycastPrimitive const& _A, SWorldRaycastPrimitive const& _B)
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
            bool operator()(STriangleHitResult const& _A, STriangleHitResult const& _B)
            {
                return (_A.Distance < _B.Distance);
            }
        } SortHit;

        // Sort by hit distance
        for (SWorldRaycastPrimitive& primitive : Primitives)
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
struct SWorldRaycastClosestResult
{
    /** Primitive owner. Null for surfaces. */
    ASceneComponent* Object;

    /** Hit */
    STriangleHitResult TriangleHit;

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
struct SWorldRaycastFilter
{
    /** Filter objects by mask */
    VISIBILITY_GROUP VisibilityMask;
    /** VSD query mask */
    VSD_QUERY_MASK QueryMask;
    /** Sort result by the distance */
    bool bSortByDistance;

    SWorldRaycastFilter()
    {
        VisibilityMask  = VISIBILITY_GROUP_ALL;
        QueryMask       = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        bSortByDistance = true;
    }
};

struct SSurfaceDef
{
    /** Parent brush model */
    ABrushModel* Model;

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

struct SBinarySpacePlane : PlaneF
{
    /** Plane axial type */
    uint8_t Type;

    HK_FORCEINLINE float DistFast(Float3 const& InPoint) const
    {
        return (Type < 3) ? (InPoint[Type] + D) : (Math::Dot(InPoint, Normal) + D);
    }
};

struct SNodeBase
{
    /** Parent node */
    struct SBinarySpaceNode* Parent;

    /** Visited mark */
    int ViewMark;

    /** Node bounding box (for culling) */
    BvAxisAlignedBox Bounds;
};

struct SBinarySpaceNode : SNodeBase
{
    /** Node split plane */
    SBinarySpacePlane* Plane;

    /** Child indices */
    int ChildrenIdx[2];
};

struct SBinarySpaceLeaf : SNodeBase
{
    /** Leaf PVS cluster */
    int PVSCluster;

    /** Leaf PVS */
    byte const* Visdata;

    /** Baked audio */
    int AudioArea;

    /** Visibility area */
    SVisArea* Area;
};

enum EFrustumCullingType
{
    CULLING_TYPE_COMBINED,
    CULLING_TYPE_SEPARATE,
    CULLING_TYPE_SIMPLE
};

struct SLightPortalDef
{
    /** First mesh vertex in array of vertices */
    int FirstVert;

    /** Mesh vertex count */
    int NumVerts;

    int FirstIndex;

    int NumIndices;
};

class ABrushModel : public GCObject
{
public:
    /** Baked surface definitions */
    TVector<SSurfaceDef> Surfaces;

    /** Baked surface vertex data */
    TVector<SMeshVertex> Vertices;

    /** Baked surface vertex data */
    TVector<SMeshVertexUV> LightmapVerts;

    /** Baked surface vertex data */
    TVector<SMeshVertexLight> VertexLight;

    /** Baked surface triangle index data */
    TVector<unsigned int> Indices;

    /** Surface materials */
    TVector<TRef<AMaterialInstance>> SurfaceMaterials;

    /** Lighting data will be used from that level. */
    TWeakRef<ALevel> ParentLevel;
};


enum LIGHTMAP_FORMAT
{
    LIGHTMAP_GRAYSCALED16_FLOAT,
    LIGHTMAP_RGBA16_FLOAT
};

struct SNodeBaseDef
{
    /** Parent node */
    int Parent;

    /** Node bounding box (for culling) */
    BvAxisAlignedBox Bounds;
};

struct SBinarySpaceNodeDef : SNodeBaseDef
{
    /** Node split plane */
    int PlaneIndex;

    /** Child indices */
    int ChildrenIdx[2];
};

struct SBinarySpaceLeafDef : SNodeBaseDef
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

struct SVisibilityAreaDef
{
    /** Area bounding box */
    BvAxisAlignedBox Bounds;

    /** Baked surfaces attached to the area */
    int FirstSurface;

    /** Count of the baked surfaces attached to the area */
    int NumSurfaces;
};

struct SVisibilitySystemPVS
{
    byte const* Visdata;
    size_t      VisdataSize;
    int         ClustersCount;
    bool        bCompressedVisData;
};

class AVisibilityLevel;

struct SVisibilitySystemCreateInfo
{
    SVisibilityAreaDef* Areas{};
    int                 NumAreas{};

    int const* AreaSurfaces{};
    int        NumAreaSurfaces{};

    SPortalDef const* Portals{};
    int               PortalsCount{};
    Float3 const*     HullVertices{};

    SBinarySpacePlane const* Planes{};
    int                      NumPlanes{};

    SBinarySpaceNodeDef const* Nodes{};
    int                        NumNodes{};

    SBinarySpaceLeafDef const* Leafs{};
    int                        NumLeafs{};

    SVisibilitySystemPVS const* PVS{};

    ABrushModel* Model{};

    AVisibilityLevel* PersistentLevel{};
};

struct SPortalScissor
{
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;
};

struct SPortalStack
{
    enum { MAX_CULL_PLANES = 5 };

    PlaneF             AreaFrustum[MAX_CULL_PLANES];
    int                PlanesCount;
    SPortalLink const* Portal;
    SPortalScissor     Scissor;
};

class AVisibilityLevel;

class AVisibilitySystem
{
public:
    AVisibilitySystem();
    virtual ~AVisibilitySystem();

    void RegisterLevel(AVisibilityLevel* Level);
    void UnregisterLevel(AVisibilityLevel* Level);

    /** Add primitive to the level */
    void AddPrimitive(SPrimitiveDef* Primitive);

    /** Remove primitive from the level */
    void RemovePrimitive(SPrimitiveDef* Primitive);

    /** Remove all primitives in the level */
    void RemovePrimitives();

    /** Mark primitive dirty */
    void MarkPrimitive(SPrimitiveDef* Primitive);

    /** Mark all primitives in the level */
    void MarkPrimitives();

    /** Unmark all primitives in the level */
    void UnmarkPrimitives();

    void UpdatePrimitiveLinks();

    void DrawDebug(ADebugRenderer* Renderer);

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& Areas) const;

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TPodVector<SVisArea*>& Areas) const;

    void QueryVisiblePrimitives(TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& Query) const;

    bool RaycastTriangles(SWorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const;

    bool RaycastClosest(SWorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const;

    bool RaycastBounds(TPodVector<SBoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const;

    bool RaycastClosestBounds(SBoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const;

    TPodVector<AVisibilityLevel*> const& GetLevels() const { return Levels; }

    using APrimitivePool     = TPoolAllocator<SPrimitiveDef>;
    using APrimitiveLinkPool = TPoolAllocator<SPrimitiveLink>;

    static APrimitivePool     PrimitivePool;
    static APrimitiveLinkPool PrimitiveLinkPool;

    static SPrimitiveDef* AllocatePrimitive();
    static void           DeallocatePrimitive(SPrimitiveDef* Primitive);

private:
    /** Unlink primitive from the level areas */
    void UnlinkPrimitive(SPrimitiveDef* Primitive);

    TPodVector<AVisibilityLevel*> Levels;

    SPrimitiveDef* PrimitiveList          = nullptr;
    SPrimitiveDef* PrimitiveListTail      = nullptr;
    SPrimitiveDef* PrimitiveDirtyList     = nullptr;
    SPrimitiveDef* PrimitiveDirtyListTail = nullptr;
};

class AVisibilityLevel : public ARefCounted
{
public:
    using AArrayOfNodes = TPodVector<SBinarySpaceNode>;
    using AArrayOfLeafs = TPodVector<SBinarySpaceLeaf>;

    AVisibilityLevel(SVisibilitySystemCreateInfo const& CreateInfo);
    virtual ~AVisibilityLevel();

    /** Get level indoor bounding box */
    BvAxisAlignedBox const& GetIndoorBounds() const { return IndoorBounds; }

    /** Get level areas */
    TPodVector<SVisArea> const& GetAreas() const { return Areas; }

    /** Get level outdoor area */
    SVisArea const* GetOutdoorArea() const { return pOutdoorArea; }

    /** Find level leaf */
    int FindLeaf(Float3 const& _Position);

    /** Find level area */
    SVisArea* FindArea(Float3 const& _Position);

    /** Mark potentially visible leafs. Uses PVS */
    int MarkLeafs(int _ViewLeaf);

    /** BSP leafs */
    AArrayOfLeafs const& GetLeafs() const { return Leafs; }

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& Areas);

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TPodVector<SVisArea*>& Areas);

    /** Add primitive to the level areas */
    static void AddPrimitiveToLevelAreas(TPodVector<AVisibilityLevel*> const& Levels, SPrimitiveDef* Primitive);

    void DrawDebug(ADebugRenderer* Renderer);

    static void QueryVisiblePrimitives(TPodVector<AVisibilityLevel*> const& Levels, TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& Query);

    static bool RaycastTriangles(TPodVector<AVisibilityLevel*> const& Levels, SWorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter);

    static bool RaycastClosest(TPodVector<AVisibilityLevel*> const& Levels, SWorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter);

    static bool RaycastBounds(TPodVector<AVisibilityLevel*> const& Levels, TPodVector<SBoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter);

    static bool RaycastClosestBounds(TPodVector<AVisibilityLevel*> const& Levels, SBoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter);

private:
    void CreatePortals(SPortalDef const* Portals, int PortalsCount, Float3 const* HullVertices);

    byte const* LeafPVS(SBinarySpaceLeaf const* _Leaf);

    byte const* DecompressVisdata(byte const* _Data);

    void QueryOverplapAreas_r(int NodeIndex, BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& Areas);
    void QueryOverplapAreas_r(int NodeIndex, BvSphere const& Bounds, TPodVector<SVisArea*>& Areas);

    void AddBoxRecursive(int NodeIndex, SPrimitiveDef* Primitive);
    void AddSphereRecursive(int NodeIndex, SPrimitiveDef* Primitive);

    void AddPrimitiveToArea(SVisArea* Area, SPrimitiveDef* Primitive);

    void ProcessLevelVisibility(struct SVisibilityQueryContext& QueryContext, struct SVisibilityQueryResult& QueryResult);

    void LevelTraverse_r(int NodeIndex, int CullBits);

    bool CullNode(PlaneF const Frustum[SPortalStack::MAX_CULL_PLANES], BvAxisAlignedBox const& Bounds, int& CullBits);

    void FlowThroughPortals_r(SVisArea const* Area);

    bool CalcPortalStack(SPortalStack* OutStack, SPortalStack const* PrevStack, SPortalLink const* Portal);

    struct SPortalHull* CalcPortalWinding(SPortalLink const* Portal, SPortalStack const* Stack);

    void CalcPortalScissor(SPortalScissor& OutScissor, SPortalHull const* Hull, SPortalStack const* Stack);

    void CullPrimitives(SVisArea const* Area, PlaneF const* CullPlanes, const int CullPlanesCount);

    bool FaceCull(SPrimitiveDef const* Primitive);
    bool FaceCull(SSurfaceDef const* Surface);

    enum EHitProxyType
    {
        HIT_PROXY_TYPE_UNKNOWN,
        HIT_PROXY_TYPE_PRIMITIVE,
        HIT_PROXY_TYPE_SURFACE
    };

    struct SRaycast
    {
        Float3 RayStart;
        Float3 RayEnd;
        Float3 RayDir;
        Float3 InvRayDir;
        float  RayLength;
        float  HitDistanceMin;
        float  HitDistanceMax; // only for bounds test

        // For closest raycast
        EHitProxyType HitProxyType;
        union
        {
            SPrimitiveDef* HitPrimitive;
            SSurfaceDef*   HitSurface;
        };
        Float3               HitLocation;
        Float2               HitUV;
        Float3               HitNormal;
        SMeshVertex const*   pVertices;
        SMeshVertexUV const* pLightmapVerts;
        int                  LightmapBlock;
        ALevel const*        LightingLevel;
        unsigned int         Indices[3];
        AMaterialInstance*   Material;
        int                  NumHits; // For debug

        bool bClosest;

        VSD_QUERY_MASK VisQueryMask;
        VISIBILITY_GROUP VisibilityMask;
    };
    void ProcessLevelRaycast(SRaycast& Raycast, SWorldRaycastResult& Result);
    void ProcessLevelRaycastClosest(SRaycast& Raycast);
    void ProcessLevelRaycastBounds(SRaycast& Raycast, TPodVector<SBoxHitResult>& Result);
    void ProcessLevelRaycastClosestBounds(SRaycast& Raycast);
    void RaycastSurface(SSurfaceDef* Self);
    void RaycastPrimitive(SPrimitiveDef* Self);
    void RaycastArea(SVisArea* Area);
    void RaycastPrimitiveBounds(SVisArea* Area);
    void LevelRaycast_r(int NodeIndex);
    bool LevelRaycast2_r(int NodeIndex, Float3 const& RayStart, Float3 const& RayEnd);
    void LevelRaycastBounds_r(int NodeIndex);
    bool LevelRaycastBounds2_r(int NodeIndex, Float3 const& RayStart, Float3 const& RayEnd);
    void LevelRaycastPortals_r(SVisArea* Area);
    void LevelRaycastBoundsPortals_r(SVisArea* Area);

    AVisibilityLevel* PersistentLevel{};

    /** Level portals */
    TPodVector<SVisPortal> Portals;

    TVector<AConvexHull> PortalHulls;

    /** Links between the portals and areas */
    TPodVector<SPortalLink> AreaLinks;

    /** Level indoor areas */
    TPodVector<SVisArea> Areas;

    /** Level outdoor area */
    SVisArea OutdoorArea;
    SVisArea* pOutdoorArea;

    BvAxisAlignedBox IndoorBounds;

    /** Node split planes */
    TPodVector<SBinarySpacePlane> SplitPlanes;

    /** BSP nodes */
    AArrayOfNodes Nodes;

    /** BSP leafs */
    AArrayOfLeafs Leafs;

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
    TRef<ABrushModel> Model;

    // Visibility query temp vars:
    static int               VisQueryMarker;
    int                      CachedSignBits[SPortalStack::MAX_CULL_PLANES]; // sign bits of ViewFrustum planes
    PlaneF*                  ViewFrustum;
    int                      ViewFrustumPlanes;
    int                      NodeViewMark;
    SVisibilityQueryContext* pQueryContext;
    SVisibilityQueryResult*  pQueryResult;

    //Raycast temp vars
    SRaycast*                  pRaycast;
    SWorldRaycastResult*       pRaycastResult;
    TPodVector<SBoxHitResult>* pBoundsRaycastResult;
};
