/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "AsyncJobManager.h"
#include "HitTest.h"

#include <Geometry/ConvexHull.h>
#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/BV/BvSphere.h>



class ALevel;
class ADebugRenderer;
//class AMaterialInstance;
class ASceneComponent;
class ABrushModel;
//struct SVisibilityQuery;
//struct SBoxHitResult;
//struct SWorldRaycastClosestResult;
//struct SWorldRaycastResult;
//struct SWorldRaycastFilter;
struct SPrimitiveDef;

struct SMeshVertex;
struct SMeshVertexUV;

struct SVisArea;
struct SPrimitiveLink;
struct SPortalLink;

enum VSD_PRIMITIVE
{
    VSD_PRIMITIVE_BOX,
    VSD_PRIMITIVE_SPHERE
};

enum VSD_QUERY_MASK
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

enum ELevelVisibilityMethod
{
    LEVEL_VISIBILITY_PVS,
    LEVEL_VISIBILITY_PORTAL
};

struct SPrimitiveDef
{
    /** Owner component */
    ASceneComponent* Owner;

    /** List of areas where primitive located */
    SPrimitiveLink* Links;

    /** Next primitive in level */
    SPrimitiveDef* Next;

    /** Prev primitive in level */
    SPrimitiveDef* Prev;

    /** Next primitive in update list */
    SPrimitiveDef* NextUpd;

    /** Prev primitive in update list */
    SPrimitiveDef* PrevUpd;

    /** Callback for local raycast */
    bool (*RaycastCallback)(SPrimitiveDef const*            Self,
                            Float3 const&                   InRayStart,
                            Float3 const&                   InRayEnd,
                            TPodVector<STriangleHitResult>& Hits);

    /** Callback for closest local raycast */
    bool (*RaycastClosestCallback)(SPrimitiveDef const* Self,
                                   Float3 const&        InRayStart,
                                   Float3 const&        InRayEnd,
                                   STriangleHitResult&  Hit,
                                   SMeshVertex const**  pVertices);

    void (*EvaluateRaycastResult)(SPrimitiveDef*       Self,
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

    /** Primitive type */
    VSD_PRIMITIVE Type;

    /** Primitive bounding shape */
    //union
    //{
    /** Used if type = VSD_PRIMITIVE_BOX */
    BvAxisAlignedBox Box;

    /** Used if type = VSD_PRIMITIVE_SPHERE */
    BvSphere Sphere;
    //};

    /** Face plane. Used to perform face culling for planar surfaces */
    PlaneF Face;

    /** Visibility query group. See VSD_QUERY_MASK enum. */
    int QueryGroup;

    /** Visibility group. See VISIBILITY_GROUP enum. */
    int VisGroup;

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark;

    /** Primitve marked as visible. Used by VSD. */
    int VisPass;

    //int BakeIndex;

    /** Surface flags (ESurfaceFlags) */
    uint8_t Flags;

    /** Is primitive outdoor/indoor */
    bool bIsOutdoor : 1;

    /** Is primitive pending to remove from level */
    bool bPendingRemove : 1;

    SPrimitiveDef() = default;
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
    //ATreeAABB * AABBTree;

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
    int VisibilityMask;

    /** Result filter */
    int QueryMask;

    // FIXME: add bool bQueryPrimitives, bool bQuerySurfaces?
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
    //float FractionMin;
    //float FractionMax;

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
    int VisibilityMask;
    /** VSD query mask */
    int QueryMask;
    /** Sort result by the distance */
    bool bSortByDistance;

    SWorldRaycastFilter()
    {
        VisibilityMask  = ~0;
        QueryMask       = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        bSortByDistance = true;
    }
};

enum ESurfaceFlags : uint8_t
{
    /** Planar surface */
    SURF_PLANAR = AN_BIT(0),

    /** Two sided surface
    NOTE: This flags affects only CPU culling and raycasting.
    You must also use a material with twosided property on to have visual effect. */
    SURF_TWOSIDED = AN_BIT(1),

    /** Planar tow sided surface */
    SURF_PLANAR_TWOSIDED_MASK = SURF_PLANAR | SURF_TWOSIDED
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

    /** Surface flags (ESurfaceFlags) */
    uint8_t Flags;

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
            | ((uint64_t)(Core::MurMur3Hash32((uint64_t)Model) & 0xffffu) << 40u) | ((uint64_t)(Core::MurMur3Hash32(MaterialIndex) & 0xffffu) << 24u) | ((uint64_t)(Core::MurMur3Hash32(LightmapBlock) & 0xffffu) << 8u);
    }
};

struct SBinarySpacePlane : PlaneF
{
    /** Plane axial type */
    uint8_t Type;

    AN_FORCEINLINE float DistFast(Float3 const& InPoint) const
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

enum EBinarySpaceLeafContents
{
    BSP_CONTENTS_NORMAL,
    BSP_CONTENTS_INVISIBLE
};

struct SBinarySpaceLeaf : SNodeBase
{
    /** Leaf PVS cluster */
    int PVSCluster;

    /** Leaf PVS */
    byte const* Visdata;

    /** Leaf contents (e.g. Water, Slime) */
    int Contents; // EBinarySpaceLeafContents

    /** Baked audio */
    int AudioArea;

    /** Visibility area */
    SVisArea* Area;
};

class AVSD
{
public:
    void QueryVisiblePrimitives(TPodVector<ALevel*> const& Levels, TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& InQuery);

    bool RaycastTriangles(TPodVector<ALevel*> const& Levels, SWorldRaycastResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter);

    bool RaycastClosest(TPodVector<ALevel*> const& Levels, SWorldRaycastClosestResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter);

    bool RaycastBounds(TPodVector<ALevel*> const& Levels, TPodVector<SBoxHitResult>& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter);

    bool RaycastClosestBounds(TPodVector<ALevel*> const& Levels, SBoxHitResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter);

    void DrawDebug(ADebugRenderer* InRenderer);

private:
    enum
    {
        MAX_CULL_PLANES  = 5,   //4
        MAX_PORTAL_STACK = 128, //64
    };

    //
    // Portal stack
    //

    struct SPortalScissor
    {
        float MinX;
        float MinY;
        float MaxX;
        float MaxY;
    };

    struct SPortalStack
    {
        PlaneF             AreaFrustum[MAX_CULL_PLANES];
        int                PlanesCount;
        SPortalLink const* Portal;
        SPortalScissor     Scissor;
    };

    SPortalStack PortalStack[MAX_PORTAL_STACK];
    int          PortalStackPos;

    //
    // Portal hull
    //

#define MAX_HULL_POINTS 128

    struct SPortalHull
    {
        int    NumPoints;
        Float3 Points[MAX_HULL_POINTS];
    };

    //
    // Portal viewer
    //

    Float3  ViewPosition;
    Float3  ViewRightVec;
    Float3  ViewUpVec;
    PlaneF  ViewPlane;
    float   ViewZNear;
    Float3  ViewCenter;
    PlaneF* ViewFrustum;
    int     ViewFrustumPlanes;
    int     CachedSignBits[MAX_CULL_PLANES]; // sign bits of ViewFrustum planes

    int     VisQueryMarker = 0;
    int     VisQueryMask   = 0;
    int     VisibilityMask = 0;
    ALevel* CurLevel;
    int     NodeViewMark;

    //
    // Visibility result
    //

    TPodVector<SPrimitiveDef*>* pVisPrimitives;
    TPodVector<SSurfaceDef*>*   pVisSurfs;

    //
    // Portal scissors debug
    //

    //#define DEBUG_PORTAL_SCISSORS
#ifdef DEBUG_PORTAL_SCISSORS
    TStdVector<SPortalScissor> DebugScissors;
#endif

    //
    // Debugging counters
    //

    //#define DEBUG_TRAVERSING_COUNTERS

#ifdef DEBUG_TRAVERSING_COUNTERS
    int Dbg_SkippedByVisFrame;
    int Dbg_SkippedByPlaneOffset;
    int Dbg_CulledSubpartsCount;
    int Dbg_CulledByDotProduct;
    int Dbg_CulledByEnvCaptureBounds;
    int Dbg_ClippedPortals;
    int Dbg_PassedPortals;
    int Dbg_StackDeep;
    int Dbg_CullMiss;
#endif
    int Dbg_CulledBySurfaceBounds;
    int Dbg_CulledByPrimitiveBounds;
    int Dbg_TotalPrimitiveBounds;

    //
    // Culling, SSE, multithreading
    //

    struct SCullThreadData
    {
        BvAxisAlignedBoxSSE const* BoundingBoxes;
        int*                       CullResult;
        int                        NumObjects;
        PlaneF*                    JobCullPlanes;
        int                        JobCullPlanesCount;
    };

    struct SCullJobSubmit
    {
        int             First;
        int             NumObjects;
        PlaneF          JobCullPlanes[MAX_CULL_PLANES];
        int             JobCullPlanesCount;
        SCullThreadData ThreadData[AAsyncJobManager::MAX_WORKER_THREADS];
    };

    TPodVector<SCullJobSubmit> CullSubmits;
    TPodVector<SPrimitiveDef*> BoxPrimitives;
    using AArrayOfBoundingBoxesSSE = TPodVector<BvAxisAlignedBoxSSE, 32, 32, AHeapAllocator<16>>;
    AArrayOfBoundingBoxesSSE                        BoundingBoxesSSE;
    TPodVector<int32_t, 32, 32, AHeapAllocator<16>> CullingResult;

    void         ProcessLevelVisibility(ALevel* InLevel);
    void         FlowThroughPortals_r(SVisArea const* InArea);
    bool         CalcPortalStack(SPortalStack* OutStack, SPortalStack const* InPrevStack, SPortalLink const* InPortal);
    bool         ClipPolygonFast(Float3 const* InPoints, const int InNumPoints, SPortalHull* Out, PlaneF const& _Plane, const float InEpsilon);
    SPortalHull* CalcPortalWinding(SPortalLink const* InPortal, SPortalStack const* InStack);
    void         CalcPortalScissor(SPortalScissor& OutScissor, SPortalHull const* InHull, SPortalStack const* InStack);
    void         LevelTraverse_r(int InNodeIndex, int InCullBits);
    bool         FaceCull(SPrimitiveDef const* InPrimitive);
    bool         FaceCull(SSurfaceDef const* InSurface);
    bool         CullNode(PlaneF const InFrustum[MAX_CULL_PLANES], int const InCachedSignBits[MAX_CULL_PLANES], BvAxisAlignedBox const& InBounds, int& InCullBits);
    void         CullPrimitives(SVisArea const* InArea, PlaneF const* InCullPlanes, const int InCullPlanesCount);
    //AN_INLINE bool CullBoxSingle( PlaneF const * InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBox const & InBounds );
    //AN_INLINE bool CullSphereSingle( PlaneF const * InCullPlanes, const int InCullPlanesCount, BvSphere const & InBounds );
    static void CullBoxGeneric(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBoxSSE const* InBounds, const int InNumObjects, int* Result);
    static void CullBoxSSE(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBoxSSE const* InBounds, const int InNumObjects, int* Result);
    static void CullBoxAsync(void* InData);
    void        SubmitCullingJobs(SCullJobSubmit& InSubmit);


    //
    // Raycasting
    //

    //#define CLOSE_ENOUGH_EARLY_OUT

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
    };

    SRaycast                   Raycast;
    SWorldRaycastResult*       pRaycastResult;
    TPodVector<SBoxHitResult>* pBoundsRaycastResult;

    void RaycastSurface(SSurfaceDef* Self);
    void RaycastPrimitive(SPrimitiveDef* Self);
    void RaycastArea(SVisArea* InArea);
    void RaycastPrimitiveBounds(SVisArea* InArea);
    void LevelRaycast_r(int InNodeIndex);
    bool LevelRaycast2_r(int InNodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd);
    void LevelRaycastBounds_r(int InNodeIndex);
    bool LevelRaycastBounds2_r(int InNodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd);
    void LevelRaycastPortals_r(SVisArea* InArea);
    void LevelRaycastBoundsPortals_r(SVisArea* InArea);
    void ProcessLevelRaycast(ALevel* InLevel);
    void ProcessLevelRaycastBounds(ALevel* InLevel);
};
