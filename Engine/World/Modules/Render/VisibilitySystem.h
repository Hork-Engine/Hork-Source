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

#include <Engine/Core/GarbageCollector.h>
#include <Engine/Renderer/RenderDefs.h>
#include <Engine/Core/Allocators/PoolAllocator.h>

HK_NAMESPACE_BEGIN

class Level;
class SceneComponent;
class ConvexHull;
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
    VSD_QUERY_MASK_VISIBLE = 0x00000001,
    VSD_QUERY_MASK_INVISIBLE = 0x00000002,

    VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS = 0x00000004,
    VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS = 0x00000008,

    VSD_QUERY_MASK_SHADOW_CAST = 0x00000010,
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
    VSD_QUERY_MASK_USER0 = 0x00010000,
    VSD_QUERY_MASK_USER1 = 0x00020000,
    VSD_QUERY_MASK_USER2 = 0x00040000,
    VSD_QUERY_MASK_USER3 = 0x00080000,
    VSD_QUERY_MASK_USER4 = 0x00100000,
    VSD_QUERY_MASK_USER5 = 0x00200000,
    VSD_QUERY_MASK_USER6 = 0x00400000,
    VSD_QUERY_MASK_USER7 = 0x00800000,
    VSD_QUERY_MASK_USER8 = 0x01000000,
    VSD_QUERY_MASK_USER9 = 0x02000000,
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
    VISIBILITY_GROUP_SKYBOX = 2,
    VISIBILITY_GROUP_TERRAIN = 4,

    VISIBILITY_GROUP_ALL = ~0u
};

HK_FLAG_ENUM_OPERATORS(VISIBILITY_GROUP)

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

struct TriangleHitResult
{
    Float3 Location;
    Float3 Normal;
    Float2 UV;
    float Distance;
    unsigned int Indices[3];
    //MaterialInstance* Material;
};

using PRIMITIVE_RAYCAST_CALLBACK = bool (*)(PrimitiveDef const* Self,
                                            Float3 const& RayStart,
                                            Float3 const& RayEnd,
                                            TVector<TriangleHitResult>& Hits);

using PRIMITIVE_RAYCAST_CLOSEST_CALLBACK = bool (*)(PrimitiveDef const* Self,
                                                    Float3 const& RayStart,
                                                    Float3 const& RayEnd,
                                                    TriangleHitResult& Hit,
                                                    MeshVertex const** pVertices);

using PRIMITIVE_EVALUATE_RAYCAST_RESULT = void (*)(PrimitiveDef* Self,
                                                   Level const* LightingLevel,
                                                   MeshVertex const* pVertices,
                                                   MeshVertexUV const* pLightmapVerts,
                                                   int LightmapBlock,
                                                   unsigned int const* pIndices,
                                                   Float3 const& HitLocation,
                                                   Float2 const& HitUV,
                                                   Float3* Vertices,
                                                   Float2& TexCoord,
                                                   Float3& LightmapSample);
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

    PrimitiveDef() :
        bIsOutdoor(false) {}

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
    /** Box owner. */
    SceneComponent* Object;

    Float3 LocationMin;
    Float3 LocationMax;
    float DistanceMin;
    float DistanceMax;

    void Clear()
    {
        Core::ZeroMem(this, sizeof(*this));
    }
};

/** Raycast primitive */
struct WorldRaycastPrimitive
{
    /** Primitive owner. */
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
    TVector<TriangleHitResult> Hits;

    /** Array of primitives */
    TVector<WorldRaycastPrimitive> Primitives;

    /** Sort raycast result by hit distance */
    void Sort()
    {
        struct SortPrimitives
        {
            TVector<TriangleHitResult> const& Hits;

            SortPrimitives(TVector<TriangleHitResult> const& _Hits) :
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
    /** Primitive owner. */
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
        Core::ZeroMem(this, sizeof(*this));
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
        VisibilityMask = VISIBILITY_GROUP_ALL;
        QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        bSortByDistance = true;
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
    /** Baked audio */
    int AudioArea;

    /** Visibility area */
    int AreaNum;
};

struct VisibilityAreaDef
{
    /** Area bounding box */
    BvAxisAlignedBox Bounds;
};

class VisibilityLevel;

struct VisibilitySystemCreateInfo
{
    VisibilityAreaDef* Areas{};
    int NumAreas{};

    PortalDef const* Portals{};
    int PortalsCount{};
    Float3 const* HullVertices{};

    BinarySpacePlane const* Planes{};
    int NumPlanes{};

    BinarySpaceNodeDef const* Nodes{};
    int NumNodes{};

    BinarySpaceLeafDef const* Leafs{};
    int NumLeafs{};

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
    enum
    {
        MAX_CULL_PLANES = 5
    };

    PlaneF AreaFrustum[MAX_CULL_PLANES];
    int PlanesCount;
    PortalLink const* Portal;
    PortalScissor Scissor;
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
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TVector<VisArea*>& Areas) const;

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TVector<VisArea*>& Areas) const;

    void QueryVisiblePrimitives(TVector<PrimitiveDef*>& VisPrimitives, int* VisPass, VisibilityQuery const& Query) const;

    bool RaycastTriangles(WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    bool RaycastClosest(WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    bool RaycastBounds(TVector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    bool RaycastClosestBounds(BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const;

    TVector<VisibilityLevel*> const& GetLevels() const { return m_Levels; }

    static TPoolAllocator<PrimitiveDef> PrimitivePool;
    static TPoolAllocator<PrimitiveLink> PrimitiveLinkPool;

    static PrimitiveDef* AllocatePrimitive();
    static void DeallocatePrimitive(PrimitiveDef* Primitive);

private:
    /** Unlink primitive from the level areas */
    void UnlinkPrimitive(PrimitiveDef* Primitive);

    TVector<VisibilityLevel*> m_Levels;

    PrimitiveDef* m_PrimitiveList = nullptr;
    PrimitiveDef* m_PrimitiveListTail = nullptr;
    PrimitiveDef* m_PrimitiveDirtyList = nullptr;
    PrimitiveDef* m_PrimitiveDirtyListTail = nullptr;
};

class VisibilityLevel : public RefCounted
{
public:
    using ArrayOfNodes = TVector<BinarySpaceNode>;
    using ArrayOfLeafs = TVector<BinarySpaceLeaf>;

    VisibilityLevel(VisibilitySystemCreateInfo const& CreateInfo);
    virtual ~VisibilityLevel();

    /** Get level indoor bounding box */
    BvAxisAlignedBox const& GetIndoorBounds() const { return m_IndoorBounds; }

    /** Get level areas */
    TVector<VisArea> const& GetAreas() const { return m_Areas; }

    /** Get level outdoor area */
    VisArea const* GetOutdoorArea() const { return m_pOutdoorArea; }

    /** Find level leaf */
    int FindLeaf(Float3 const& _Position);

    /** Find level area */
    VisArea* FindArea(Float3 const& _Position);

    /** BSP leafs */
    ArrayOfLeafs const& GetLeafs() const { return m_Leafs; }

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TVector<VisArea*>& Areas);

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TVector<VisArea*>& Areas);

    /** Add primitive to the level areas */
    static void AddPrimitiveToLevelAreas(TVector<VisibilityLevel*> const& Levels, PrimitiveDef* Primitive);

    void DrawDebug(DebugRenderer* Renderer);

    static void QueryVisiblePrimitives(TVector<VisibilityLevel*> const& Levels, TVector<PrimitiveDef*>& VisPrimitives, int* VisPass, VisibilityQuery const& Query);

    static bool RaycastTriangles(TVector<VisibilityLevel*> const& Levels, WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

    static bool RaycastClosest(TVector<VisibilityLevel*> const& Levels, WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

    static bool RaycastBounds(TVector<VisibilityLevel*> const& Levels, TVector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

    static bool RaycastClosestBounds(TVector<VisibilityLevel*> const& Levels, BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter);

private:
    void CreatePortals(PortalDef const* Portals, int PortalsCount, Float3 const* HullVertices);

    void QueryOverplapAreas_r(int NodeIndex, BvAxisAlignedBox const& Bounds, TVector<VisArea*>& Areas);
    void QueryOverplapAreas_r(int NodeIndex, BvSphere const& Bounds, TVector<VisArea*>& Areas);

    void AddBoxRecursive(int NodeIndex, PrimitiveDef* Primitive);
    void AddSphereRecursive(int NodeIndex, PrimitiveDef* Primitive);

    void AddPrimitiveToArea(VisArea* Area, PrimitiveDef* Primitive);

    void ProcessLevelVisibility(struct VisibilityQueryContext& QueryContext, struct VisibilityQueryResult& QueryResult);

    void FlowThroughPortals_r(VisArea const* Area);

    bool CalcPortalStack(PortalStack* OutStack, PortalStack const* PrevStack, PortalLink const* Portal);

    struct PortalHull* CalcPortalWinding(PortalLink const* Portal, PortalStack const* Stack);

    void CalcPortalScissor(PortalScissor& OutScissor, PortalHull const* Hull, PortalStack const* Stack);

    void CullPrimitives(VisArea const* Area, PlaneF const* CullPlanes, const int CullPlanesCount);

    bool FaceCull(PrimitiveDef const* Primitive);

    struct VisRaycast
    {
        Float3 RayStart;
        Float3 RayEnd;
        Float3 RayDir;
        Float3 InvRayDir;
        float RayLength;
        float HitDistanceMin;
        float HitDistanceMax; // only for bounds test

        // For closest raycast
        PrimitiveDef* HitPrimitive;
        Float3 HitLocation;
        Float2 HitUV;
        Float3 HitNormal;
        MeshVertex const* pVertices;
        MeshVertexUV const* pLightmapVerts;
        int LightmapBlock;
        Level const* LightingLevel;
        unsigned int Indices[3];
        //MaterialInstance* Material;
        int NumHits; // For debug

        bool bClosest;

        VSD_QUERY_MASK VisQueryMask;
        VISIBILITY_GROUP VisibilityMask;
    };
    void ProcessLevelRaycast(VisRaycast& Raycast, WorldRaycastResult& Result);
    void ProcessLevelRaycastClosest(VisRaycast& Raycast);
    void ProcessLevelRaycastBounds(VisRaycast& Raycast, TVector<BoxHitResult>& Result);
    void ProcessLevelRaycastClosestBounds(VisRaycast& Raycast);
    void RaycastPrimitive(PrimitiveDef* Self);
    void RaycastArea(VisArea* Area);
    void RaycastPrimitiveBounds(VisArea* Area);
    void LevelRaycastPortals_r(VisArea* Area);
    void LevelRaycastBoundsPortals_r(VisArea* Area);

    VisibilityLevel* m_PersistentLevel{};

    /** Level portals */
    TVector<VisPortal> m_Portals;

    TVector<ConvexHull> m_PortalHulls;

    /** Links between the portals and areas */
    TVector<PortalLink> m_AreaLinks;

    /** Level indoor areas */
    TVector<VisArea> m_Areas;

    /** Level outdoor area */
    VisArea m_OutdoorArea;
    VisArea* m_pOutdoorArea;

    BvAxisAlignedBox m_IndoorBounds;

    /** Node split planes */
    TVector<BinarySpacePlane> m_SplitPlanes;

    /** BSP nodes */
    ArrayOfNodes m_Nodes;

    /** BSP leafs */
    ArrayOfLeafs m_Leafs;

    // Visibility query temp vars:
    static int m_VisQueryMarker;
    VisibilityQueryContext* m_pQueryContext;
    VisibilityQueryResult* m_pQueryResult;

    //Raycast temp vars
    VisRaycast* m_pRaycast;
    WorldRaycastResult* m_pRaycastResult;
    TVector<BoxHitResult>* m_pBoundsRaycastResult;
};

HK_NAMESPACE_END
