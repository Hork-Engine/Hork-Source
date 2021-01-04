/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <World/Public/World.h>
#include <Runtime/Public/Runtime.h>

class AVSD
{
public:
    void QueryVisiblePrimitives( AWorld * InWorld, TPodArray< SPrimitiveDef * > & VisPrimitives, TPodArray< SSurfaceDef * > & VisSurfs, int * VisPass, SVisibilityQuery const & InQuery );

    bool RaycastTriangles( AWorld * InWorld, SWorldRaycastResult & Result, Float3 const & InRayStart, Float3 const & InRayEnd, SWorldRaycastFilter const * InFilter );

    bool RaycastClosest( AWorld * InWorld, SWorldRaycastClosestResult & Result, Float3 const & InRayStart, Float3 const & InRayEnd, SWorldRaycastFilter const * InFilter );

    bool RaycastBounds( AWorld * InWorld, TPodArray< SBoxHitResult > & Result, Float3 const & InRayStart, Float3 const & InRayEnd, SWorldRaycastFilter const * InFilter );

    bool RaycastClosestBounds( AWorld * InWorld, SBoxHitResult & Result, Float3 const & InRayStart, Float3 const & InRayEnd, SWorldRaycastFilter const * InFilter );

    void DrawDebug( ADebugRenderer * InRenderer );

private:
    enum
    {
        MAX_CULL_PLANES = 5,//4
        MAX_PORTAL_STACK = 128,//64
    };

    //
    // Portal stack
    //

    struct SPortalScissor {
        float MinX;
        float MinY;
        float MaxX;
        float MaxY;
    };

    struct SPortalStack {
        PlaneF AreaFrustum[MAX_CULL_PLANES];
        int PlanesCount;
        SPortalLink const * Portal;
        SPortalScissor Scissor;
    };

    SPortalStack PortalStack[MAX_PORTAL_STACK];
    int PortalStackPos;

    //
    // Portal hull
    //

#define MAX_HULL_POINTS 128

    struct SPortalHull {
        int NumPoints;
        Float3 Points[MAX_HULL_POINTS];
    };

    //
    // Portal viewer
    //

    Float3 ViewPosition;
    Float3 ViewRightVec;
    Float3 ViewUpVec;
    PlaneF ViewPlane;
    float  ViewZNear;
    Float3 ViewCenter;
    PlaneF * ViewFrustum;
    int ViewFrustumPlanes;
    int CachedSignBits[MAX_CULL_PLANES]; // sign bits of ViewFrustum planes

    int VisQueryMarker = 0;
    int VisQueryMask = 0;
    int VisibilityMask = 0;
    ALevel * CurLevel;
    int NodeViewMark;

    //
    // Visibility result
    //

    TPodArray< SPrimitiveDef * > * pVisPrimitives;
    TPodArray< SSurfaceDef * > * pVisSurfs;

    //
    // Portal scissors debug
    //

    //#define DEBUG_PORTAL_SCISSORS
#ifdef DEBUG_PORTAL_SCISSORS
    TStdVector< SPortalScissor > DebugScissors;
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

    struct SCullThreadData {
        BvAxisAlignedBoxSSE const * BoundingBoxes;
        int * CullResult;
        int NumObjects;
        PlaneF * JobCullPlanes;
        int JobCullPlanesCount;
    };

    struct SCullJobSubmit {
        int First;
        int NumObjects;
        PlaneF JobCullPlanes[MAX_CULL_PLANES];
        int JobCullPlanesCount;
        SCullThreadData ThreadData[AAsyncJobManager::MAX_WORKER_THREADS];
    };

    TPodArray< SCullJobSubmit > CullSubmits;
    TPodArray< SPrimitiveDef * > BoxPrimitives;
    using AArrayOfBoundingBoxesSSE = TPodArray< BvAxisAlignedBoxSSE, 32, 32, AHeapAllocator >;
    AArrayOfBoundingBoxesSSE BoundingBoxesSSE;
    TPodArray< int32_t, 32, 32, AHeapAllocator > CullingResult;

    void ProcessLevelVisibility( ALevel * InLevel );
    void FlowThroughPortals_r( SVisArea const * InArea );
    bool CalcPortalStack( SPortalStack * OutStack, SPortalStack const * InPrevStack, SPortalLink const * InPortal );
    bool ClipPolygonFast( Float3 const * InPoints, const int InNumPoints, SPortalHull * Out, PlaneF const & _Plane, const float InEpsilon );
    SPortalHull * CalcPortalWinding( SPortalLink const * InPortal, SPortalStack const * InStack );
    void CalcPortalScissor( SPortalScissor & OutScissor, SPortalHull const * InHull, SPortalStack const * InStack );
    void LevelTraverse_r( int InNodeIndex, int InCullBits );
    bool FaceCull( SPrimitiveDef const * InPrimitive );
    bool FaceCull( SSurfaceDef const * InSurface );
    bool CullNode( PlaneF const InFrustum[MAX_CULL_PLANES], int const InCachedSignBits[MAX_CULL_PLANES], BvAxisAlignedBox const & InBounds, int & InCullBits );
    void CullPrimitives( SVisArea const * InArea, PlaneF const * InCullPlanes, const int InCullPlanesCount );
    //AN_INLINE bool CullBoxSingle( PlaneF const * InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBox const & InBounds );
    //AN_INLINE bool CullSphereSingle( PlaneF const * InCullPlanes, const int InCullPlanesCount, BvSphere const & InBounds );
    static void CullBoxGeneric( PlaneF const * InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBoxSSE const * InBounds, const int InNumObjects, int * Result );
    static void CullBoxSSE( PlaneF const * InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBoxSSE const * InBounds, const int InNumObjects, int * Result );
    static void CullBoxAsync( void * InData );
    void SubmitCullingJobs( SCullJobSubmit & InSubmit );


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
        float RayLength;
        float HitDistanceMin;
        float HitDistanceMax; // only for bounds test

                              // For closest raycast
        EHitProxyType HitProxyType;
        union
        {
            SPrimitiveDef * HitPrimitive;
            SSurfaceDef * HitSurface;
        };
        Float3 HitLocation;
        Float2 HitUV;
        Float3 HitNormal;
        SMeshVertex const * pVertices;
        SMeshVertexUV const * pLightmapVerts;
        int LightmapBlock;
        ALevel const * LightingLevel;
        unsigned int Indices[3];
        AMaterialInstance * Material;
        int NumHits; // For debug

        bool bClosest;
    };

    SRaycast Raycast;
    SWorldRaycastResult * pRaycastResult;
    TPodArray< SBoxHitResult > * pBoundsRaycastResult;
    const SWorldRaycastFilter DefaultRaycastFilter;

    void RaycastSurface( SSurfaceDef * Self );
    void RaycastPrimitive( SPrimitiveDef * Self );
    void RaycastArea( SVisArea * InArea );
    void RaycastPrimitiveBounds( SVisArea * InArea );
    void LevelRaycast_r( int InNodeIndex );
    bool LevelRaycast2_r( int InNodeIndex, Float3 const & InRayStart, Float3 const & InRayEnd );
    void LevelRaycastBounds_r( int InNodeIndex );
    bool LevelRaycastBounds2_r( int InNodeIndex, Float3 const & InRayStart, Float3 const & InRayEnd );
    void LevelRaycastPortals_r( SVisArea * InArea );
    void LevelRaycastBoundsPortals_r( SVisArea * InArea );
    void ProcessLevelRaycast( ALevel * InLevel );
    void ProcessLevelRaycastBounds( ALevel * InLevel );
};
