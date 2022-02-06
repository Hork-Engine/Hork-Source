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

// TODO: Future: HIZ culler, Software depth rasterizer, Occluders:
// CPU Frustum cull / SSE/ MT = for outdoor
// Portal cull / Area PVS = for indoor
// Occluders (inverse kind of frustum culling) = for indoor & outdoor
// Software occluder rasterizer + HIZ occludee culling
// AABB-tree for static outdoor/indoor geometry

// FIXME: Replace AABB culling to OBB culling?

#include "VSD.h"
#include "Level.h"
#include "Engine.h"

#include <Core/ScopedTimer.h>
#include <Geometry/ConvexHull.h>
#include <Geometry/BV/BvIntersect.h>
#include <Geometry/BV/BvFrustum.h>
#include <Renderer/RenderDefs.h>

static const SWorldRaycastFilter DefaultRaycastFilter;

AConsoleVar vsd_FrustumCullingMT(_CTS("vsd_FrustumCullingMT"), _CTS("1"));
AConsoleVar vsd_FrustumCullingSSE(_CTS("vsd_FrustumCullingSSE"), _CTS("1"));
AConsoleVar vsd_FrustumCullingType(_CTS("vsd_FrustumCullingType"), _CTS("0"), 0, _CTS("0 - combined, 1 - separate, 2 - simple"));

enum EFrustumCullingType
{
    CULLING_TYPE_COMBINED,
    CULLING_TYPE_SEPARATE,
    CULLING_TYPE_SIMPLE
};

void AVSD::ProcessLevelVisibility(ALevel* InLevel)
{
    CurLevel = InLevel;

    ViewFrustum       = PortalStack[0].AreaFrustum;
    ViewFrustumPlanes = PortalStack[0].PlanesCount; // Can be 4 or 5

    int cullBits = 0;

    for (int i = 0; i < ViewFrustumPlanes; i++)
    {
        CachedSignBits[i] = ViewFrustum[i].SignBits();

        cullBits |= 1 << i;
    }

    if (InLevel->VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has PVS

        int leaf = InLevel->FindLeaf(ViewPosition);

        NodeViewMark = InLevel->MarkLeafs(leaf);

        //GLogger.Printf( "NodeViewMark %d\n", NodeViewMark );

        //HK_ASSERT( NodeViewMark != 0 );

        LevelTraverse_r(0, cullBits);
    }
    else if (InLevel->VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = InLevel->FindArea(ViewPosition);

        FlowThroughPortals_r(area);
    }
}

void AVSD::QueryVisiblePrimitives(TPodVector<ALevel*> const& Levels, TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& InQuery)
{
    //int QueryVisiblePrimitivesTime = GEngine->SysMicroseconds();

    ++VisQueryMarker;

    if (VisPass)
    {
        *VisPass = VisQueryMarker;
    }

    VisQueryMask   = InQuery.QueryMask;
    VisibilityMask = InQuery.VisibilityMask;

    pVisPrimitives = &VisPrimitives;
    pVisPrimitives->Clear();

    pVisSurfs = &VisSurfs;
    pVisSurfs->Clear();

    BoxPrimitives.Clear();
    BoundingBoxesSSE.Clear();
    CullingResult.Clear();
    CullSubmits.Clear();

#ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_SkippedByVisFrame        = 0;
    Dbg_SkippedByPlaneOffset     = 0;
    Dbg_CulledSubpartsCount      = 0;
    Dbg_CulledByDotProduct       = 0;
    Dbg_CulledByEnvCaptureBounds = 0;
    Dbg_ClippedPortals           = 0;
    Dbg_PassedPortals            = 0;
    Dbg_StackDeep                = 0;
    Dbg_CullMiss                 = 0;
#endif
    Dbg_CulledBySurfaceBounds   = 0;
    Dbg_CulledByPrimitiveBounds = 0;
    Dbg_TotalPrimitiveBounds    = 0;

#ifdef DEBUG_PORTAL_SCISSORS
    DebugScissors.Clear();
#endif

    ViewPosition = InQuery.ViewPosition;
    ViewRightVec = InQuery.ViewRightVec;
    ViewUpVec    = InQuery.ViewUpVec;
    ViewPlane    = *InQuery.FrustumPlanes[FRUSTUM_PLANE_NEAR];
    ViewZNear    = -ViewPlane.DistanceToPoint(ViewPosition); //Camera->GetZNear();
    ViewCenter   = ViewPlane.Normal * ViewZNear;

    // Get corner at left-bottom of frustum
    Float3 corner = Math::Cross(InQuery.FrustumPlanes[FRUSTUM_PLANE_BOTTOM]->Normal, InQuery.FrustumPlanes[FRUSTUM_PLANE_LEFT]->Normal);

    // Project left-bottom corner to near plane
    corner = corner * (ViewZNear / Math::Dot(ViewPlane.Normal, corner));

    const float x = Math::Dot(ViewRightVec, corner);
    const float y = Math::Dot(ViewUpVec, corner);

    // w = tan( half_fov_x_rad ) * znear * 2;
    // h = tan( half_fov_y_rad ) * znear * 2;

    PortalStackPos                = 0;
    PortalStack[0].AreaFrustum[0] = *InQuery.FrustumPlanes[0];
    PortalStack[0].AreaFrustum[1] = *InQuery.FrustumPlanes[1];
    PortalStack[0].AreaFrustum[2] = *InQuery.FrustumPlanes[2];
    PortalStack[0].AreaFrustum[3] = *InQuery.FrustumPlanes[3];
    PortalStack[0].AreaFrustum[4] = *InQuery.FrustumPlanes[4]; // far plane
    PortalStack[0].PlanesCount    = 5;
    PortalStack[0].Portal         = NULL;
    PortalStack[0].Scissor.MinX   = x;
    PortalStack[0].Scissor.MinY   = y;
    PortalStack[0].Scissor.MaxX   = -x;
    PortalStack[0].Scissor.MaxY   = -y;

    for (ALevel* level : Levels)
    {
        ProcessLevelVisibility(level);
    }

    if (vsd_FrustumCullingType.GetInteger() == CULLING_TYPE_COMBINED)
    {
        CullingResult.ResizeInvalidate(Align(BoundingBoxesSSE.Size(), 4));

        for (SCullJobSubmit& submit : CullSubmits)
        {
            SubmitCullingJobs(submit);

            Dbg_TotalPrimitiveBounds += submit.NumObjects;
        }

        // Wait when it's done
        GEngine->RenderFrontendJobList->Wait();

        {
            AScopedTimer TimeCheck("Evaluate submits");

            for (SCullJobSubmit& submit : CullSubmits)
            {

                SPrimitiveDef** boxes      = BoxPrimitives.ToPtr() + submit.First;
                int*            cullResult = CullingResult.ToPtr() + submit.First;

                for (int n = 0; n < submit.NumObjects; n++)
                {

                    SPrimitiveDef* primitive = boxes[n];

                    if (primitive->VisMark != VisQueryMarker)
                    {

                        if (!cullResult[n])
                        { // TODO: Use atomic increment and store only visible objects?
                            // Mark primitive visibility processed
                            primitive->VisMark = VisQueryMarker;

                            // Mark primitive visible
                            primitive->VisPass = VisQueryMarker;

                            pVisPrimitives->Append(primitive);
                        }
                        else
                        {
#ifdef DEBUG_TRAVERSING_COUNTERS
                            Dbg_CulledByPrimitiveBounds++;
#endif
                        }
                    }
                }
            }
        }
    }

#ifdef DEBUG_TRAVERSING_COUNTERS
    GLogger.Printf("VSD: VisFrame %d\n", Dbg_SkippedByVisFrame);
    GLogger.Printf("VSD: PlaneOfs %d\n", Dbg_SkippedByPlaneOffset);
    GLogger.Printf("VSD: FaceCull %d\n", Dbg_CulledByDotProduct);
    GLogger.Printf("VSD: AABBCull %d\n", Dbg_CulledByPrimitiveBounds);
    GLogger.Printf("VSD: AABBCull (subparts) %d\n", Dbg_CulledSubpartsCount);
    GLogger.Printf("VSD: Clipped %d\n", Dbg_ClippedPortals);
    GLogger.Printf("VSD: PassedPortals %d\n", Dbg_PassedPortals);
    GLogger.Printf("VSD: StackDeep %d\n", Dbg_StackDeep);
    GLogger.Printf("VSD: CullMiss: %d\n", Dbg_CullMiss);
#endif

    //QueryVisiblePrimitivesTime = GEngine->SysMicroseconds() - QueryVisiblePrimitivesTime;

    //GLogger.Printf( "QueryVisiblePrimitivesTime: %d microsec\n", QueryVisiblePrimitivesTime );

    //GLogger.Printf( "Frustum culling time %d microsec. Culled %d from %d primitives. Submits %d\n", Dbg_FrustumCullingTime, Dbg_CulledByPrimitiveBounds, Dbg_TotalPrimitiveBounds, CullSubmits.Size() );
}

void AVSD::FlowThroughPortals_r(SVisArea const* InArea)
{
    SPortalStack* prevStack = &PortalStack[PortalStackPos];
    SPortalStack* stack     = prevStack + 1;

    CullPrimitives(InArea, prevStack->AreaFrustum, prevStack->PlanesCount);

    if (PortalStackPos == (MAX_PORTAL_STACK - 1))
    {
        GLogger.Printf("MAX_PORTAL_STACK hit\n");
        return;
    }

    ++PortalStackPos;

#ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_StackDeep = Math::Max(Dbg_StackDeep, PortalStackPos);
#endif

    for (SPortalLink const* portal = InArea->PortalList; portal; portal = portal->Next)
    {

        //if ( portal->Portal->VisFrame == VisQueryMarker ) {
        //    #ifdef DEBUG_TRAVERSING_COUNTERS
        //    Dbg_SkippedByVisFrame++;
        //    #endif
        //    continue;
        //}

        if (portal->Portal->bBlocked)
        {
            // Portal is closed
            continue;
        }

        if (!CalcPortalStack(stack, prevStack, portal))
        {
            continue;
        }

        // Mark visited
        portal->Portal->VisMark = VisQueryMarker;

        FlowThroughPortals_r(portal->ToArea);
    }

    --PortalStackPos;
}

bool AVSD::CalcPortalStack(SPortalStack* OutStack, SPortalStack const* InPrevStack, SPortalLink const* InPortal)
{
    const float d = InPortal->Plane.DistanceToPoint(ViewPosition);
    if (d <= 0.0f)
    {
#ifdef DEBUG_TRAVERSING_COUNTERS
        Dbg_SkippedByPlaneOffset++;
#endif
        return false;
    }

    if (d <= ViewZNear)
    {
        // View intersecting the portal

        for (int i = 0; i < InPrevStack->PlanesCount; i++)
        {
            OutStack->AreaFrustum[i] = InPrevStack->AreaFrustum[i];
        }
        OutStack->PlanesCount = InPrevStack->PlanesCount;
        OutStack->Scissor     = InPrevStack->Scissor;
    }
    else
    {

        //for ( int i = 0 ; i < PortalStackPos ; i++ ) {
        //    if ( PortalStack[ i ].Portal == InPortal ) {
        //        GLogger.Printf( "Recursive!\n" );
        //    }
        //}

        SPortalHull* portalWinding = CalcPortalWinding(InPortal, InPrevStack);

        if (portalWinding->NumPoints < 3)
        {
// Invisible
#ifdef DEBUG_TRAVERSING_COUNTERS
            Dbg_ClippedPortals++;
#endif
            return false;
        }

        CalcPortalScissor(OutStack->Scissor, portalWinding, InPrevStack);

        if (OutStack->Scissor.MinX >= OutStack->Scissor.MaxX || OutStack->Scissor.MinY >= OutStack->Scissor.MaxY)
        {
// invisible
#ifdef DEBUG_TRAVERSING_COUNTERS
            Dbg_ClippedPortals++;
#endif
            return false;
        }

        // Compute 3D frustum to cull objects inside vis area
        if (portalWinding->NumPoints <= 4)
        {
            OutStack->PlanesCount = portalWinding->NumPoints;

            // Compute based on portal winding
            for (int i = 0; i < OutStack->PlanesCount; i++)
            {
                // CW
                //OutStack->AreaFrustum[ i ].FromPoints( ViewPosition, portalWinding->Points[ ( i + 1 ) % portalWinding->NumPoints ], portalWinding->Points[ i ] );

                // CCW
                OutStack->AreaFrustum[i].FromPoints(ViewPosition, portalWinding->Points[i], portalWinding->Points[(i + 1) % portalWinding->NumPoints]);
            }

            // Copy far plane
            OutStack->AreaFrustum[OutStack->PlanesCount++] = InPrevStack->AreaFrustum[InPrevStack->PlanesCount - 1];
        }
        else
        {
            // Compute based on portal scissor
            const Float3 rightMin = ViewRightVec * OutStack->Scissor.MinX + ViewCenter;
            const Float3 rightMax = ViewRightVec * OutStack->Scissor.MaxX + ViewCenter;
            const Float3 upMin    = ViewUpVec * OutStack->Scissor.MinY;
            const Float3 upMax    = ViewUpVec * OutStack->Scissor.MaxY;
            const Float3 corners[4] =
                {
                    rightMin + upMin,
                    rightMax + upMin,
                    rightMax + upMax,
                    rightMin + upMax};

            Float3 p;

            // bottom
            p                               = Math::Cross(corners[1], corners[0]);
            OutStack->AreaFrustum[0].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[0].D      = -Math::Dot(OutStack->AreaFrustum[0].Normal, ViewPosition);

            // right
            p                               = Math::Cross(corners[2], corners[1]);
            OutStack->AreaFrustum[1].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[1].D      = -Math::Dot(OutStack->AreaFrustum[1].Normal, ViewPosition);

            // top
            p                               = Math::Cross(corners[3], corners[2]);
            OutStack->AreaFrustum[2].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[2].D      = -Math::Dot(OutStack->AreaFrustum[2].Normal, ViewPosition);

            // left
            p                               = Math::Cross(corners[0], corners[3]);
            OutStack->AreaFrustum[3].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[3].D      = -Math::Dot(OutStack->AreaFrustum[3].Normal, ViewPosition);

            //OutStack->PlanesCount = 4;

            // Copy far plane
            OutStack->AreaFrustum[4] = InPrevStack->AreaFrustum[InPrevStack->PlanesCount - 1];

            OutStack->PlanesCount = 5;
        }
    }

#ifdef DEBUG_PORTAL_SCISSORS
    DebugScissors.Append(OutStack->Scissor);
#endif

#ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_PassedPortals++;
#endif

    OutStack->Portal = InPortal;

    return true;
}

//
// Fast polygon clipping. Without memory allocations.
//

static float      ClipDistances[MAX_HULL_POINTS];
static PLANE_SIDE ClipSides[MAX_HULL_POINTS];

bool AVSD::ClipPolygonFast(Float3 const* InPoints, const int InNumPoints, SPortalHull* Out, PlaneF const& InClipPlane, const float InEpsilon)
{
    int   front = 0;
    int   back  = 0;
    int   i;
    float d;

    HK_ASSERT(InNumPoints + 4 <= MAX_HULL_POINTS);

    // Classify hull points
    for (i = 0; i < InNumPoints; i++)
    {
        d = InClipPlane.DistanceToPoint(InPoints[i]);

        ClipDistances[i] = d;

        if (d > InEpsilon)
        {
            ClipSides[i] = PLANE_SIDE_FRONT;
            front++;
        }
        else if (d < -InEpsilon)
        {
            ClipSides[i] = PLANE_SIDE_BACK;
            back++;
        }
        else
        {
            ClipSides[i] = PLANE_SIDE_ON;
        }
    }

    if (!front)
    {
        // All points are behind the plane
        Out->NumPoints = 0;
        return true;
    }

    if (!back)
    {
        // All points are on the front
        return false;
    }

    Out->NumPoints = 0;

    ClipSides[i]     = ClipSides[0];
    ClipDistances[i] = ClipDistances[0];

    for (i = 0; i < InNumPoints; i++)
    {
        Float3 const& v = InPoints[i];

        if (ClipSides[i] == PLANE_SIDE_ON)
        {
            Out->Points[Out->NumPoints++] = v;
            continue;
        }

        if (ClipSides[i] == PLANE_SIDE_FRONT)
        {
            Out->Points[Out->NumPoints++] = v;
        }

        PLANE_SIDE nextSide = ClipSides[i + 1];

        if (nextSide == PLANE_SIDE_ON || nextSide == ClipSides[i])
        {
            continue;
        }

        Float3& newVertex = Out->Points[Out->NumPoints++];

        newVertex = InPoints[(i + 1) % InNumPoints];

        d = ClipDistances[i] / (ClipDistances[i] - ClipDistances[i + 1]);

        newVertex = v + d * (newVertex - v);
    }

    return true;
}

AVSD::SPortalHull* AVSD::CalcPortalWinding(SPortalLink const* InPortal, SPortalStack const* InStack)
{
    static SPortalHull PortalHull[2];

    int flip = 0;

    // Clip portal hull by view plane
    if (!ClipPolygonFast(InPortal->Hull->Points, InPortal->Hull->NumPoints, &PortalHull[flip], ViewPlane, 0.0f))
    {

        HK_ASSERT(InPortal->Hull->NumPoints <= MAX_HULL_POINTS);

        Platform::Memcpy(PortalHull[flip].Points, InPortal->Hull->Points, InPortal->Hull->NumPoints * sizeof(Float3));
        PortalHull[flip].NumPoints = InPortal->Hull->NumPoints;
    }

    if (PortalHull[flip].NumPoints >= 3)
    {
        for (int i = 0; i < InStack->PlanesCount; i++)
        {
            if (ClipPolygonFast(PortalHull[flip].Points, PortalHull[flip].NumPoints, &PortalHull[(flip + 1) & 1], InStack->AreaFrustum[i], 0.0f))
            {
                flip = (flip + 1) & 1;

                if (PortalHull[flip].NumPoints < 3)
                {
                    break;
                }
            }
        }
    }

    return &PortalHull[flip];
}

void AVSD::CalcPortalScissor(SPortalScissor& OutScissor, SPortalHull const* InHull, SPortalStack const* InStack)
{
    OutScissor.MinX = 99999999.0f;
    OutScissor.MinY = 99999999.0f;
    OutScissor.MaxX = -99999999.0f;
    OutScissor.MaxY = -99999999.0f;

    for (int i = 0; i < InHull->NumPoints; i++)
    {

        // Project portal vertex to view plane
        const Float3 vec = InHull->Points[i] - ViewPosition;

        const float d = Math::Dot(ViewPlane.Normal, vec);

        //if ( d < ViewZNear ) {
        //    HK_ASSERT(0);
        //}

        const Float3 p = d < ViewZNear ? vec : vec * (ViewZNear / d);

        // Compute relative coordinates
        const float x = Math::Dot(ViewRightVec, p);
        const float y = Math::Dot(ViewUpVec, p);

        // Compute bounds
        OutScissor.MinX = Math::Min(x, OutScissor.MinX);
        OutScissor.MinY = Math::Min(y, OutScissor.MinY);

        OutScissor.MaxX = Math::Max(x, OutScissor.MaxX);
        OutScissor.MaxY = Math::Max(y, OutScissor.MaxY);
    }

    // Clip bounds by current scissor bounds
    OutScissor.MinX = Math::Max(InStack->Scissor.MinX, OutScissor.MinX);
    OutScissor.MinY = Math::Max(InStack->Scissor.MinY, OutScissor.MinY);
    OutScissor.MaxX = Math::Min(InStack->Scissor.MaxX, OutScissor.MaxX);
    OutScissor.MaxY = Math::Min(InStack->Scissor.MaxY, OutScissor.MaxY);
}

HK_FORCEINLINE bool AVSD::FaceCull(SPrimitiveDef const* InPrimitive)
{
    return InPrimitive->Face.DistanceToPoint(ViewPosition) < 0.0f;
}

HK_FORCEINLINE bool AVSD::FaceCull(SSurfaceDef const* InSurface)
{
    return InSurface->Face.DistanceToPoint(ViewPosition) < 0.0f;
}

HK_INLINE bool VSD_CullBoxSingle(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBox const& InBounds)
{
    bool inside = true;

    for (int i = 0; i < InCullPlanesCount; i++)
    {

        PlaneF const* p = &InCullPlanes[i];

        inside &= (Math::Max(InBounds.Mins.X * p->Normal.X, InBounds.Maxs.X * p->Normal.X) + Math::Max(InBounds.Mins.Y * p->Normal.Y, InBounds.Maxs.Y * p->Normal.Y) + Math::Max(InBounds.Mins.Z * p->Normal.Z, InBounds.Maxs.Z * p->Normal.Z) + p->D) > 0.0f;
    }

    return !inside;
}

HK_INLINE bool VSD_CullSphereSingle(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvSphere const& InBounds)
{
#if 0
    bool cull = false;
    for ( int i = 0 ; i < InCullPlanesCount ; i++ ) {

        PlaneF const * p = &InCullPlanes[i];

        if ( Math::Dot( p->Normal, InBounds->Center ) + p->D <= -InBounds->Radius ) {
            cull = true;
        }
    }
    return cull;
#endif

    bool inside = true;
    for (int i = 0; i < InCullPlanesCount; i++)
    {

        PlaneF const* p = &InCullPlanes[i];

        inside &= (Math::Dot(p->Normal, InBounds.Center) + p->D > -InBounds.Radius);
    }
    return !inside;
}

void AVSD::CullPrimitives(SVisArea const* InArea, PlaneF const* InCullPlanes, const int InCullPlanesCount)
{
    if (vsd_FrustumCullingType.GetInteger() != CULLING_TYPE_COMBINED)
    {
        BoxPrimitives.Clear();
        BoundingBoxesSSE.Clear();
        CullSubmits.Clear();
    }

    int numBoxes          = 0;
    int firstBoxPrimitive = BoxPrimitives.Size();

    if (InArea->NumSurfaces > 0)
    {
        ABrushModel* model = CurLevel->Model;

        int const* pSurfaceIndex = &CurLevel->AreaSurfaces[InArea->FirstSurface];

        for (int i = 0; i < InArea->NumSurfaces; i++, pSurfaceIndex++)
        {

            SSurfaceDef* surf = &model->Surfaces[*pSurfaceIndex];

            if (surf->VisMark == VisQueryMarker)
            {
                // Surface visibility already processed
                continue;
            }

            // Mark surface visibility processed
            surf->VisMark = VisQueryMarker;

            // Filter query group
            if ((surf->QueryGroup & VisQueryMask) != VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & VisibilityMask) == 0)
            {
                continue;
            }

            // Perform face culling
            if ((surf->Flags & SURF_PLANAR_TWOSIDED_MASK) == SURF_PLANAR && FaceCull(surf))
            {
                continue;
            }

            if (VSD_CullBoxSingle(InCullPlanes, InCullPlanesCount, surf->Bounds))
            {

#ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_CulledBySurfaceBounds++;
#endif

                continue;
            }

            // Mark as visible
            surf->VisPass = VisQueryMarker;

            pVisSurfs->Append(surf);
        }
    }

    for (SPrimitiveLink* link = InArea->Links; link; link = link->NextInArea)
    {

        HK_ASSERT(link->Area == InArea);

        SPrimitiveDef* primitive = link->Primitive;

        if (primitive->VisMark == VisQueryMarker)
        {
            // Primitive visibility already processed
            continue;
        }

        // Filter query group
        if ((primitive->QueryGroup & VisQueryMask) != VisQueryMask)
        {
            // Mark primitive visibility processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & VisibilityMask) == 0)
        {
            // Mark primitive visibility processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        if ((primitive->Flags & SURF_PLANAR_TWOSIDED_MASK) == SURF_PLANAR)
        {
            // Perform face culling
            if (FaceCull(primitive))
            {
                // Face successfully culled
                primitive->VisMark = VisQueryMarker;

// Update debug counter
#ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_CulledByDotProduct++;
#endif

                continue;
            }
        }

        switch (primitive->Type)
        {
            case VSD_PRIMITIVE_BOX: {
                if (vsd_FrustumCullingType.GetInteger() == CULLING_TYPE_SIMPLE)
                {
                    if (VSD_CullBoxSingle(InCullPlanes, InCullPlanesCount, primitive->Box))
                    {

#ifdef DEBUG_TRAVERSING_COUNTERS
                        Dbg_CulledByPrimitiveBounds++;
#endif

                        continue;
                    }
                }
                else
                {
                    // Prepare primitive for frustum culling
                    BoxPrimitives.Append(primitive);
                    BoundingBoxesSSE.Append() = primitive->Box;
                    numBoxes++;
                    continue;
                }
                break;
            }
            case VSD_PRIMITIVE_SPHERE: {
                if (VSD_CullSphereSingle(InCullPlanes, InCullPlanesCount, primitive->Sphere))
                {
#ifdef DEBUG_TRAVERSING_COUNTERS
                    Dbg_CulledByPrimitiveBounds++;
#endif
                    continue;
                }
                break;
            }
        }

        // Mark primitive visibility processed
        primitive->VisMark = VisQueryMarker;

        // Mark primitive visible
        primitive->VisPass = VisQueryMarker;

        // Add primitive to vis list
        pVisPrimitives->Append(primitive);
    }

    if (numBoxes > 0)
    {
        // Create job submit

        SCullJobSubmit& submit = CullSubmits.Append();

        submit.First      = firstBoxPrimitive;
        submit.NumObjects = numBoxes;
        for (int i = 0; i < InCullPlanesCount; i++)
        {
            submit.JobCullPlanes[i] = InCullPlanes[i];
        }
        submit.JobCullPlanesCount = InCullPlanesCount;

        if (BoxPrimitives.Size() & 3)
        {
            // Apply objects count alignment
            int count = (BoxPrimitives.Size() & ~3) + 4;

            BoxPrimitives.Resize(count);
            BoundingBoxesSSE.Resize(count);
        }

        if (vsd_FrustumCullingType.GetInteger() == CULLING_TYPE_SEPARATE)
        {
            SubmitCullingJobs(submit);

            // Wait when it's done
            GEngine->RenderFrontendJobList->Wait();

            Dbg_TotalPrimitiveBounds += numBoxes;

            CullingResult.ResizeInvalidate(Align(BoundingBoxesSSE.Size(), 4));

            SPrimitiveDef** boxes      = BoxPrimitives.ToPtr() + submit.First;
            const int*      cullResult = CullingResult.ToPtr() + submit.First;

            for (int n = 0; n < submit.NumObjects; n++)
            {

                SPrimitiveDef* primitive = boxes[n];

                if (primitive->VisMark != VisQueryMarker)
                {

                    if (!cullResult[n])
                    { // TODO: Use atomic increment and store only visible objects?
                        // Mark primitive visibility processed
                        primitive->VisMark = VisQueryMarker;

                        // Mark primitive visible
                        primitive->VisPass = VisQueryMarker;

                        pVisPrimitives->Append(primitive);
                    }
                    else
                    {
#ifdef DEBUG_TRAVERSING_COUNTERS
                        Dbg_CulledByPrimitiveBounds++;
#endif
                    }
                }
            }
        }
    }
}

static constexpr int CullIndices[8][6] = {
    {0, 4, 5, 3, 1, 2},
    {3, 4, 5, 0, 1, 2},
    {0, 1, 5, 3, 4, 2},
    {3, 1, 5, 0, 4, 2},
    {0, 4, 2, 3, 1, 5},
    {3, 4, 2, 0, 1, 5},
    {0, 1, 2, 3, 4, 5},
    {3, 1, 2, 0, 4, 5}};

bool AVSD::CullNode(PlaneF const InFrustum[MAX_CULL_PLANES], int const InCachedSignBits[MAX_CULL_PLANES], BvAxisAlignedBox const& InBounds, int& InCullBits)
{
    Float3 p;

    float const* pBounds = InBounds.ToPtr();
    int const*   pIndices;
    if (InCullBits & 1)
    {
        pIndices = CullIndices[InCachedSignBits[0]];

        p[0] = pBounds[pIndices[0]];
        p[1] = pBounds[pIndices[1]];
        p[2] = pBounds[pIndices[2]];

        if (Math::Dot(p, InFrustum[0].Normal) <= -InFrustum[0].D)
        {
            return true;
        }

        p[0] = pBounds[pIndices[3]];
        p[1] = pBounds[pIndices[4]];
        p[2] = pBounds[pIndices[5]];

        if (Math::Dot(p, InFrustum[0].Normal) >= -InFrustum[0].D)
        {
            InCullBits &= ~1;
        }
    }

    if (InCullBits & 2)
    {
        pIndices = CullIndices[InCachedSignBits[1]];

        p[0] = pBounds[pIndices[0]];
        p[1] = pBounds[pIndices[1]];
        p[2] = pBounds[pIndices[2]];

        if (Math::Dot(p, InFrustum[1].Normal) <= -InFrustum[1].D)
        {
            return true;
        }

        p[0] = pBounds[pIndices[3]];
        p[1] = pBounds[pIndices[4]];
        p[2] = pBounds[pIndices[5]];

        if (Math::Dot(p, InFrustum[1].Normal) >= -InFrustum[1].D)
        {
            InCullBits &= ~2;
        }
    }

    if (InCullBits & 4)
    {
        pIndices = CullIndices[InCachedSignBits[2]];

        p[0] = pBounds[pIndices[0]];
        p[1] = pBounds[pIndices[1]];
        p[2] = pBounds[pIndices[2]];

        if (Math::Dot(p, InFrustum[2].Normal) <= -InFrustum[2].D)
        {
            return true;
        }

        p[0] = pBounds[pIndices[3]];
        p[1] = pBounds[pIndices[4]];
        p[2] = pBounds[pIndices[5]];

        if (Math::Dot(p, InFrustum[2].Normal) >= -InFrustum[2].D)
        {
            InCullBits &= ~4;
        }
    }

    if (InCullBits & 8)
    {
        pIndices = CullIndices[InCachedSignBits[3]];

        p[0] = pBounds[pIndices[0]];
        p[1] = pBounds[pIndices[1]];
        p[2] = pBounds[pIndices[2]];

        if (Math::Dot(p, InFrustum[3].Normal) <= -InFrustum[3].D)
        {
            return true;
        }

        p[0] = pBounds[pIndices[3]];
        p[1] = pBounds[pIndices[4]];
        p[2] = pBounds[pIndices[5]];

        if (Math::Dot(p, InFrustum[3].Normal) >= -InFrustum[3].D)
        {
            InCullBits &= ~8;
        }
    }

    if (InCullBits & 16)
    {
        pIndices = CullIndices[InCachedSignBits[4]];

        p[0] = pBounds[pIndices[0]];
        p[1] = pBounds[pIndices[1]];
        p[2] = pBounds[pIndices[2]];

        if (Math::Dot(p, InFrustum[4].Normal) <= -InFrustum[4].D)
        {
            return true;
        }

        p[0] = pBounds[pIndices[3]];
        p[1] = pBounds[pIndices[4]];
        p[2] = pBounds[pIndices[5]];

        if (Math::Dot(p, InFrustum[4].Normal) >= -InFrustum[4].D)
        {
            InCullBits &= ~16;
        }
    }

    return false;
}

void AVSD::LevelTraverse_r(int InNodeIndex, int InCullBits)
{
    SNodeBase const* node;

    while (1)
    {
        if (InNodeIndex < 0)
        {
            node = &CurLevel->Leafs[-1 - InNodeIndex];
        }
        else
        {
            node = CurLevel->Nodes.ToPtr() + InNodeIndex;
        }

        if (node->ViewMark != NodeViewMark)
            return;

        if (CullNode(ViewFrustum, CachedSignBits, node->Bounds, InCullBits))
        {
            //TotalCulled++;
            return;
        }

#if 0
        if ( VSD_CullBoxSingle( ViewFrustum, ViewFrustumPlanes, node->Bounds ) ) {
            Dbg_CullMiss++;
        }
#endif

        if (InNodeIndex < 0)
        {
            // leaf
            break;
        }

        LevelTraverse_r(static_cast<SBinarySpaceNode const*>(node)->ChildrenIdx[0], InCullBits);

        InNodeIndex = static_cast<SBinarySpaceNode const*>(node)->ChildrenIdx[1];
    }

    SBinarySpaceLeaf const* pleaf = static_cast<SBinarySpaceLeaf const*>(node);

    CullPrimitives(pleaf->Area, ViewFrustum, ViewFrustumPlanes);
}

void AVSD::CullBoxGeneric(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBoxSSE const* InBounds, const int InNumObjects, int* Result)
{
    for (BvAxisAlignedBoxSSE const* Last = InBounds + InNumObjects; InBounds < Last; InBounds++)
    {

        bool inside = true;

        for (int i = 0; i < InCullPlanesCount; i++)
        {

            PlaneF const* p = &InCullPlanes[i];

            inside &= (Math::Max(InBounds->Mins.X * p->Normal.X, InBounds->Maxs.X * p->Normal.X) + Math::Max(InBounds->Mins.Y * p->Normal.Y, InBounds->Maxs.Y * p->Normal.Y) + Math::Max(InBounds->Mins.Z * p->Normal.Z, InBounds->Maxs.Z * p->Normal.Z) + p->D) > 0.0f;
        }

        *Result++ = !inside;
    }
}

void AVSD::CullBoxSSE(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBoxSSE const* InBounds, const int InNumObjects, int* Result)
{
    const float* pBoundingBoxData = reinterpret_cast<const float*>(&InBounds->Mins.X);
    int*         pCullingResult   = &Result[0];
    int          i, j;

    __m128 x[MAX_CULL_PLANES];
    __m128 y[MAX_CULL_PLANES];
    __m128 z[MAX_CULL_PLANES];
    __m128 d[MAX_CULL_PLANES];

    for (i = 0; i < InCullPlanesCount; i++)
    {
        x[i] = _mm_set1_ps(InCullPlanes[i].Normal.X);
        y[i] = _mm_set1_ps(InCullPlanes[i].Normal.Y);
        z[i] = _mm_set1_ps(InCullPlanes[i].Normal.Z);
        d[i] = _mm_set1_ps(InCullPlanes[i].D);
    }

    __m128 zero = _mm_setzero_ps();

    // Process 4 objects per step
    for (i = 0; i < InNumObjects; i += 4)
    {
        // Load bounding mins
        __m128 aabb_min_x = _mm_load_ps(pBoundingBoxData);
        __m128 aabb_min_y = _mm_load_ps(pBoundingBoxData + 8);
        __m128 aabb_min_z = _mm_load_ps(pBoundingBoxData + 16);
        __m128 aabb_min_w = _mm_load_ps(pBoundingBoxData + 24);

        // Load bounding maxs
        __m128 aabb_max_x = _mm_load_ps(pBoundingBoxData + 4);
        __m128 aabb_max_y = _mm_load_ps(pBoundingBoxData + 12);
        __m128 aabb_max_z = _mm_load_ps(pBoundingBoxData + 20);
        __m128 aabb_max_w = _mm_load_ps(pBoundingBoxData + 28);

        pBoundingBoxData += 32;

        // For now we have points in vectors aabb_min_x..w, but for calculations we need to xxxx yyyy zzzz vectors representation.
        // Just transpose data.
        _MM_TRANSPOSE4_PS(aabb_min_x, aabb_min_y, aabb_min_z, aabb_min_w);
        _MM_TRANSPOSE4_PS(aabb_max_x, aabb_max_y, aabb_max_z, aabb_max_w);

        // Set bitmask to zero
        __m128 intersection_res = _mm_setzero_ps();

        for (j = 0; j < InCullPlanesCount; j++)
        {
            // Pick closest point to plane and check if it begind the plane. if yes - object outside frustum

            // Dot product, separate for each coordinate, for min & max aabb points
            __m128 mins_mul_plane_x = _mm_mul_ps(aabb_min_x, x[j]);
            __m128 mins_mul_plane_y = _mm_mul_ps(aabb_min_y, y[j]);
            __m128 mins_mul_plane_z = _mm_mul_ps(aabb_min_z, z[j]);

            __m128 maxs_mul_plane_x = _mm_mul_ps(aabb_max_x, x[j]);
            __m128 maxs_mul_plane_y = _mm_mul_ps(aabb_max_y, y[j]);
            __m128 maxs_mul_plane_z = _mm_mul_ps(aabb_max_z, z[j]);

            // We have 8 box points, but we need pick closest point to plane.
            __m128 res_x = _mm_max_ps(mins_mul_plane_x, maxs_mul_plane_x);
            __m128 res_y = _mm_max_ps(mins_mul_plane_y, maxs_mul_plane_y);
            __m128 res_z = _mm_max_ps(mins_mul_plane_z, maxs_mul_plane_z);

            // Distance to plane = dot( aabb_point.xyz, plane.xyz ) + plane.d
            __m128 sum_xy            = _mm_add_ps(res_x, res_y);
            __m128 sum_zw            = _mm_add_ps(res_z, d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            // Dist from closest point to plane < 0 ?
            __m128 plane_res = _mm_cmple_ps(distance_to_plane, zero);

            // If yes - aabb behind the plane & outside frustum
            intersection_res = _mm_or_ps(intersection_res, plane_res);
        }

        // Convert packed single-precision (32-bit) floating point elements to
        // packed 32-bit integers
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);

        // Store result
        _mm_store_si128((__m128i*)&pCullingResult[i], intersection_res_i);
    }
}

void AVSD::CullBoxAsync(void* InData)
{
    SCullThreadData& threadData = *(SCullThreadData*)InData;

    if (vsd_FrustumCullingSSE)
    {
        CullBoxSSE(threadData.JobCullPlanes, threadData.JobCullPlanesCount, threadData.BoundingBoxes, threadData.NumObjects, threadData.CullResult);
    }
    else
    {
        CullBoxGeneric(threadData.JobCullPlanes, threadData.JobCullPlanesCount, threadData.BoundingBoxes, threadData.NumObjects, threadData.CullResult);
    }
}

void AVSD::SubmitCullingJobs(SCullJobSubmit& InSubmit)
{
    int i, firstObject;

    const int threasCount = vsd_FrustumCullingMT ? GEngine->AsyncJobManager->GetNumWorkerThreads() : 1;

    const int MIN_OBJECTS_PER_THREAD = 4; // TODO: choose appropriate value

    int cullObjectsPerThread;

    if (threasCount > 1)
    {
        cullObjectsPerThread = (InSubmit.NumObjects / threasCount) & ~3;
    }
    else
    {
        cullObjectsPerThread = 0;
    }

    HK_ASSERT(InSubmit.JobCullPlanesCount <= MAX_CULL_PLANES);

    if (threasCount <= 1 || cullObjectsPerThread < MIN_OBJECTS_PER_THREAD)
    {
        // Multithreading is disabled or too few objects

        if (vsd_FrustumCullingSSE)
        {
            CullBoxSSE(InSubmit.JobCullPlanes,
                       InSubmit.JobCullPlanesCount,
                       BoundingBoxesSSE.ToPtr() + InSubmit.First,
                       Align(InSubmit.NumObjects, 4),
                       CullingResult.ToPtr() + InSubmit.First);
        }
        else
        {
            CullBoxGeneric(InSubmit.JobCullPlanes,
                           InSubmit.JobCullPlanesCount,
                           BoundingBoxesSSE.ToPtr() + InSubmit.First,
                           InSubmit.NumObjects,
                           CullingResult.ToPtr() + InSubmit.First);
        }
        return;
    }

    // Configure jobs
    for (i = 0, firstObject = 0; i < threasCount; i++, firstObject += cullObjectsPerThread)
    {
        int first                                 = InSubmit.First + firstObject;
        InSubmit.ThreadData[i].BoundingBoxes      = BoundingBoxesSSE.ToPtr() + first;
        InSubmit.ThreadData[i].CullResult         = CullingResult.ToPtr() + first;
        InSubmit.ThreadData[i].NumObjects         = cullObjectsPerThread;
        InSubmit.ThreadData[i].JobCullPlanes      = InSubmit.JobCullPlanes;
        InSubmit.ThreadData[i].JobCullPlanesCount = InSubmit.JobCullPlanesCount;

        GEngine->RenderFrontendJobList->AddJob(CullBoxAsync, &InSubmit.ThreadData[i]);
    }

    // Do jobs
    GEngine->RenderFrontendJobList->Submit();

    // Process residual objects
    const int Residual = InSubmit.NumObjects - firstObject;
    if (Residual > 0)
    {
        if (vsd_FrustumCullingSSE)
        {
            CullBoxSSE(InSubmit.JobCullPlanes,
                       InSubmit.JobCullPlanesCount,
                       BoundingBoxesSSE.ToPtr() + InSubmit.First + firstObject,
                       Align(Residual, 4),
                       CullingResult.ToPtr() + InSubmit.First + firstObject);
        }
        else
        {
            CullBoxGeneric(InSubmit.JobCullPlanes,
                           InSubmit.JobCullPlanesCount,
                           BoundingBoxesSSE.ToPtr() + InSubmit.First + firstObject,
                           Residual,
                           CullingResult.ToPtr() + InSubmit.First + firstObject);
        }
    }
}

HK_INLINE bool RayIntersectTriangleFast(Float3 const& _RayStart, Float3 const& _RayDir, Float3 const& _P0, Float3 const& _P1, Float3 const& _P2, float& _U, float& _V)
{
    const Float3 e1 = _P1 - _P0;
    const Float3 e2 = _P2 - _P0;
    const Float3 h  = Math::Cross(_RayDir, e2);

    // calc determinant
    const float det = Math::Dot(e1, h);

    if (det > -0.00001 && det < 0.00001)
    {
        return false;
    }

    // calc inverse determinant to minimalize math divisions in next calculations
    const float invDet = 1 / det;

    // calc vector from ray origin to P0
    const Float3 s = _RayStart - _P0;

    // calc U
    _U = invDet * Math::Dot(s, h);
    if (_U < 0.0f || _U > 1.0f)
    {
        return false;
    }

    // calc perpendicular to compute V
    const Float3 q = Math::Cross(s, e1);

    // calc V
    _V = invDet * Math::Dot(_RayDir, q);
    if (_V < 0.0f || _U + _V > 1.0f)
    {
        return false;
    }

    return true;
}

void AVSD::RaycastSurface(SSurfaceDef* Self)
{
    float d, u, v;
    float boxMin, boxMax;

    if (Self->Flags & SURF_PLANAR)
    {
        // Calculate distance from ray origin to plane
        const float d1 = Math::Dot(Raycast.RayStart, Self->Face.Normal) + Self->Face.D;
        float       d2;

        if (Self->Flags & SURF_TWOSIDED)
        {
            // Check ray direction
            d2 = Math::Dot(Self->Face.Normal, Raycast.RayDir);
            if (Math::Abs(d2) < 0.0001f)
            {
                // ray is parallel
                return;
            }
        }
        else
        {

            // Perform face culling
            if (d1 <= 0.0f) return;

            // Check ray direction
            d2 = Math::Dot(Self->Face.Normal, Raycast.RayDir);
            if (d2 >= 0.0f)
            {
                // ray is parallel or has wrong direction
                return;
            }

#if 0
            // Code for back face culling

            // Perform face culling
            if ( d1 >= 0.0f ) return;

            // Check ray direction
            d2 = Math::Dot( Self->Face.Normal, Raycast.RayDir );
            if ( d2 <= 0.0f ) {
                // ray is parallel or has wrong direction
                return;
            }
#endif
        }

        // Calculate distance from ray origin to plane intersection
        d = -(d1 / d2);

        if (d <= 0.0f)
        {
            return;
        }

        if (d >= Raycast.HitDistanceMin)
        {
            // distance is too far
            return;
        }

        ABrushModel const* brushModel = Self->Model;

        SMeshVertex const*  pVertices = brushModel->Vertices.ToPtr() + Self->FirstVertex;
        unsigned int const* pIndices  = brushModel->Indices.ToPtr() + Self->FirstIndex;

        if (Raycast.bClosest)
        {
            for (int i = 0; i < Self->NumIndices; i += 3)
            {
                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (RayIntersectTriangleFast(Raycast.RayStart, Raycast.RayDir, v0, v1, v2, u, v))
                {
                    Raycast.HitProxyType   = HIT_PROXY_TYPE_SURFACE;
                    Raycast.HitSurface     = Self;
                    Raycast.HitLocation    = Raycast.RayStart + Raycast.RayDir * d;
                    Raycast.HitDistanceMin = d;
                    Raycast.HitUV.X        = u;
                    Raycast.HitUV.Y        = v;
                    Raycast.pVertices      = brushModel->Vertices.ToPtr();
                    Raycast.pLightmapVerts = brushModel->LightmapVerts.ToPtr();
                    Raycast.LightmapBlock  = Self->LightmapBlock;
                    Raycast.LightingLevel  = brushModel->ParentLevel.GetObject();
                    Raycast.Indices[0]     = Self->FirstVertex + triangleIndices[0];
                    Raycast.Indices[1]     = Self->FirstVertex + triangleIndices[1];
                    Raycast.Indices[2]     = Self->FirstVertex + triangleIndices[2];
                    Raycast.Material       = brushModel->SurfaceMaterials[Self->MaterialIndex];
                    Raycast.NumHits++;

                    // Mark as visible
                    Self->VisPass = VisQueryMarker;

                    break;
                }
            }
        }
        else
        {
            for (int i = 0; i < Self->NumIndices; i += 3)
            {

                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (RayIntersectTriangleFast(Raycast.RayStart, Raycast.RayDir, v0, v1, v2, u, v))
                {

                    STriangleHitResult& hitResult = pRaycastResult->Hits.Append();
                    hitResult.Location            = Raycast.RayStart + Raycast.RayDir * d;
                    hitResult.Normal              = Self->Face.Normal;
                    hitResult.Distance            = d;
                    hitResult.UV.X                = u;
                    hitResult.UV.Y                = v;
                    hitResult.Indices[0]          = Self->FirstVertex + triangleIndices[0];
                    hitResult.Indices[1]          = Self->FirstVertex + triangleIndices[1];
                    hitResult.Indices[2]          = Self->FirstVertex + triangleIndices[2];
                    hitResult.Material            = brushModel->SurfaceMaterials[Self->MaterialIndex];

                    SWorldRaycastPrimitive& rcPrimitive = pRaycastResult->Primitives.Append();
                    rcPrimitive.Object                  = nullptr;
                    rcPrimitive.FirstHit = rcPrimitive.ClosestHit = pRaycastResult->Hits.Size() - 1;
                    rcPrimitive.NumHits                           = 1;

                    // Mark as visible
                    Self->VisPass = VisQueryMarker;

                    break;
                }
            }
        }
    }
    else
    {
        bool cullBackFaces = !(Self->Flags & SURF_TWOSIDED);

        // Perform AABB raycast
        if (!BvRayIntersectBox(Raycast.RayStart, Raycast.InvRayDir, Self->Bounds, boxMin, boxMax))
        {
            return;
        }

        if (boxMin >= Raycast.HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            return;
        }

        ABrushModel const* brushModel = Self->Model;

        SMeshVertex const*  pVertices = brushModel->Vertices.ToPtr() + Self->FirstVertex;
        unsigned int const* pIndices  = brushModel->Indices.ToPtr() + Self->FirstIndex;

        if (Raycast.bClosest)
        {
            for (int i = 0; i < Self->NumIndices; i += 3)
            {
                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (BvRayIntersectTriangle(Raycast.RayStart, Raycast.RayDir, v0, v1, v2, d, u, v, cullBackFaces))
                {
                    if (Raycast.HitDistanceMin > d)
                    {

                        Raycast.HitProxyType   = HIT_PROXY_TYPE_SURFACE;
                        Raycast.HitSurface     = Self;
                        Raycast.HitLocation    = Raycast.RayStart + Raycast.RayDir * d;
                        Raycast.HitDistanceMin = d;
                        Raycast.HitUV.X        = u;
                        Raycast.HitUV.Y        = v;
                        Raycast.pVertices      = brushModel->Vertices.ToPtr();
                        Raycast.pLightmapVerts = brushModel->LightmapVerts.ToPtr();
                        Raycast.LightmapBlock  = Self->LightmapBlock;
                        Raycast.LightingLevel  = brushModel->ParentLevel.GetObject();
                        Raycast.Indices[0]     = Self->FirstVertex + triangleIndices[0];
                        Raycast.Indices[1]     = Self->FirstVertex + triangleIndices[1];
                        Raycast.Indices[2]     = Self->FirstVertex + triangleIndices[2];
                        Raycast.Material       = brushModel->SurfaceMaterials[Self->MaterialIndex];

                        // Mark as visible
                        Self->VisPass = VisQueryMarker;
                    }
                }
            }
        }
        else
        {
            int firstHit   = pRaycastResult->Hits.Size();
            int closestHit = firstHit;

            for (int i = 0; i < Self->NumIndices; i += 3)
            {

                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (BvRayIntersectTriangle(Raycast.RayStart, Raycast.RayDir, v0, v1, v2, d, u, v, cullBackFaces))
                {
                    if (Raycast.RayLength > d)
                    {
                        STriangleHitResult& hitResult = pRaycastResult->Hits.Append();
                        hitResult.Location            = Raycast.RayStart + Raycast.RayDir * d;
                        hitResult.Normal              = Math::Cross(v1 - v0, v2 - v0).Normalized();
                        hitResult.Distance            = d;
                        hitResult.UV.X                = u;
                        hitResult.UV.Y                = v;
                        hitResult.Indices[0]          = Self->FirstVertex + triangleIndices[0];
                        hitResult.Indices[1]          = Self->FirstVertex + triangleIndices[1];
                        hitResult.Indices[2]          = Self->FirstVertex + triangleIndices[2];
                        hitResult.Material            = brushModel->SurfaceMaterials[Self->MaterialIndex];

                        // Mark as visible
                        Self->VisPass = VisQueryMarker;

                        // Find closest hit
                        if (d < pRaycastResult->Hits[closestHit].Distance)
                        {
                            closestHit = pRaycastResult->Hits.Size() - 1;
                        }
                    }
                }
            }

            if (Self->VisPass == VisQueryMarker)
            {
                SWorldRaycastPrimitive& rcPrimitive = pRaycastResult->Primitives.Append();
                rcPrimitive.Object                  = nullptr;
                rcPrimitive.FirstHit                = firstHit;
                rcPrimitive.NumHits                 = pRaycastResult->Hits.Size() - firstHit;
                rcPrimitive.ClosestHit              = closestHit;
            }
        }
    }
}

void AVSD::RaycastPrimitive(SPrimitiveDef* Self)
{
    // FIXME: What about two sided primitives? Use TwoSided flag directly from material or from primitive?

    if (Raycast.bClosest)
    {
        STriangleHitResult hit;

        if (Self->RaycastClosestCallback && Self->RaycastClosestCallback(Self, Raycast.RayStart, Raycast.HitLocation, hit, &Raycast.pVertices))
        {
            Raycast.HitProxyType   = HIT_PROXY_TYPE_PRIMITIVE;
            Raycast.HitPrimitive   = Self;
            Raycast.HitLocation    = hit.Location;
            Raycast.HitNormal      = hit.Normal;
            Raycast.HitUV          = hit.UV;
            Raycast.HitDistanceMin = hit.Distance;
            Raycast.Indices[0]     = hit.Indices[0];
            Raycast.Indices[1]     = hit.Indices[1];
            Raycast.Indices[2]     = hit.Indices[2];
            Raycast.Material       = hit.Material;

            // TODO:
            //Raycast.pLightmapVerts = Self->Owner->LightmapUVChannel->GetVertices();
            //Raycast.LightmapBlock = Self->Owner->LightmapBlock;
            //Raycast.LightingLevel = Self->Owner->ParentLevel.GetObject();

            // Mark primitive visible
            Self->VisPass = VisQueryMarker;
        }
    }
    else
    {
        int firstHit = pRaycastResult->Hits.Size();
        if (Self->RaycastCallback && Self->RaycastCallback(Self, Raycast.RayStart, Raycast.RayEnd, pRaycastResult->Hits))
        {

            int numHits = pRaycastResult->Hits.Size() - firstHit;

            // Find closest hit
            int closestHit = firstHit;
            for (int i = 0; i < numHits; i++)
            {
                int                 hitNum    = firstHit + i;
                STriangleHitResult& hitResult = pRaycastResult->Hits[hitNum];

                if (hitResult.Distance < pRaycastResult->Hits[closestHit].Distance)
                {
                    closestHit = hitNum;
                }
            }

            SWorldRaycastPrimitive& rcPrimitive = pRaycastResult->Primitives.Append();

            rcPrimitive.Object     = Self->Owner;
            rcPrimitive.FirstHit   = firstHit;
            rcPrimitive.NumHits    = pRaycastResult->Hits.Size() - firstHit;
            rcPrimitive.ClosestHit = closestHit;

            // Mark primitive visible
            Self->VisPass = VisQueryMarker;
        }
    }
}

void AVSD::RaycastArea(SVisArea* InArea)
{
    float boxMin, boxMax;

    if (InArea->VisMark == VisQueryMarker)
    {
        // Area raycast already processed
        //GLogger.Printf( "Area raycast already processed\n" );
        return;
    }

    // Mark area raycast processed
    InArea->VisMark = VisQueryMarker;

    if (InArea->NumSurfaces > 0)
    {
        ABrushModel* model = CurLevel->Model;

        int const* pSurfaceIndex = &CurLevel->AreaSurfaces[InArea->FirstSurface];

        for (int i = 0; i < InArea->NumSurfaces; i++, pSurfaceIndex++)
        {

            SSurfaceDef* surf = &model->Surfaces[*pSurfaceIndex];

            if (surf->VisMark == VisQueryMarker)
            {
                // Surface raycast already processed
                continue;
            }

            // Mark surface raycast processed
            surf->VisMark = VisQueryMarker;

            // Filter query group
            if ((surf->QueryGroup & VisQueryMask) != VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & VisibilityMask) == 0)
            {
                continue;
            }

            RaycastSurface(surf);

#ifdef CLOSE_ENOUGH_EARLY_OUT
            // hit is close enough to stop ray casting?
            if (Raycast.HitDistanceMin < 0.0001f)
            {
                return;
            }
#endif
        }
    }

    for (SPrimitiveLink* link = InArea->Links; link; link = link->NextInArea)
    {
        SPrimitiveDef* primitive = link->Primitive;

        if (primitive->VisMark == VisQueryMarker)
        {
            // Primitive raycast already processed
            continue;
        }

        // Filter query group
        if ((primitive->QueryGroup & VisQueryMask) != VisQueryMask)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & VisibilityMask) == 0)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        if ((primitive->Flags & SURF_PLANAR_TWOSIDED_MASK) == SURF_PLANAR)
        {
            // Perform face culling
            if (FaceCull(primitive))
            {
                // Face successfully culled
                primitive->VisMark = VisQueryMarker;
                continue;
            }
        }

        switch (primitive->Type)
        {
            case VSD_PRIMITIVE_BOX: {
                // Perform AABB raycast
                if (!BvRayIntersectBox(Raycast.RayStart, Raycast.InvRayDir, primitive->Box, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            case VSD_PRIMITIVE_SPHERE: {
                // Perform Sphere raycast
                if (!BvRayIntersectSphere(Raycast.RayStart, Raycast.RayDir, primitive->Sphere, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            default:
            {
                HK_ASSERT(0);
                continue;
            }
        }

        if (boxMin >= Raycast.HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            continue;
        }

        // Mark primitive raycast processed
        primitive->VisMark = VisQueryMarker;

        RaycastPrimitive(primitive);

#ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if (Raycast.HitDistanceMin < 0.0001f)
        {
            return;
        }
#endif
    }
}

void AVSD::RaycastPrimitiveBounds(SVisArea* InArea)
{
    float boxMin, boxMax;

    if (InArea->VisMark == VisQueryMarker)
    {
        // Area raycast already processed
        //GLogger.Printf( "Area raycast already processed\n" );
        return;
    }

    // Mark area raycast processed
    InArea->VisMark = VisQueryMarker;

    if (InArea->NumSurfaces > 0)
    {
        ABrushModel* model = CurLevel->Model;

        int const* pSurfaceIndex = &CurLevel->AreaSurfaces[InArea->FirstSurface];

        for (int i = 0; i < InArea->NumSurfaces; i++, pSurfaceIndex++)
        {

            SSurfaceDef* surf = &model->Surfaces[*pSurfaceIndex];

            if (surf->VisMark == VisQueryMarker)
            {
                // Surface raycast already processed
                continue;
            }

            // Mark surface raycast processed
            surf->VisMark = VisQueryMarker;

            // Filter query group
            if ((surf->QueryGroup & VisQueryMask) != VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & VisibilityMask) == 0)
            {
                continue;
            }

            if (surf->Flags & SURF_PLANAR)
            {
                // FIXME: planar surface has no bounds?
                continue;
            }

            // Perform AABB raycast
            if (!BvRayIntersectBox(Raycast.RayStart, Raycast.InvRayDir, surf->Bounds, boxMin, boxMax))
            {
                continue;
            }
            if (boxMin >= Raycast.HitDistanceMin)
            {
                // Ray intersects the box, but box is too far
                continue;
            }

            // Mark as visible
            surf->VisPass = VisQueryMarker;

            if (Raycast.bClosest)
            {
                Raycast.HitProxyType   = HIT_PROXY_TYPE_SURFACE;
                Raycast.HitSurface     = surf;
                Raycast.HitDistanceMin = boxMin;
                Raycast.HitDistanceMax = boxMax;

#ifdef CLOSE_ENOUGH_EARLY_OUT
                // hit is close enough to stop ray casting?
                if (Raycast.HitDistanceMin < 0.0001f)
                {
                    break;
                }
#endif
            }
            else
            {
                SBoxHitResult& hitResult = pBoundsRaycastResult->Append();

                hitResult.Object      = nullptr;
                hitResult.LocationMin = Raycast.RayStart + Raycast.RayDir * boxMin;
                hitResult.LocationMax = Raycast.RayStart + Raycast.RayDir * boxMax;
                hitResult.DistanceMin = boxMin;
                hitResult.DistanceMax = boxMax;
            }
        }
    }

    for (SPrimitiveLink* link = InArea->Links; link; link = link->NextInArea)
    {
        SPrimitiveDef* primitive = link->Primitive;

        if (primitive->VisMark == VisQueryMarker)
        {
            // Primitive raycast already processed
            continue;
        }

        // Filter query group
        if ((primitive->QueryGroup & VisQueryMask) != VisQueryMask)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & VisibilityMask) == 0)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        switch (primitive->Type)
        {
            case VSD_PRIMITIVE_BOX: {
                // Perform AABB raycast
                if (!BvRayIntersectBox(Raycast.RayStart, Raycast.InvRayDir, primitive->Box, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            case VSD_PRIMITIVE_SPHERE: {
                // Perform Sphere raycast
                if (!BvRayIntersectSphere(Raycast.RayStart, Raycast.RayDir, primitive->Sphere, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            default:
            {
                HK_ASSERT(0);
                continue;
            }
        }

        if (boxMin >= Raycast.HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            continue;
        }

        // Mark primitive raycast processed
        primitive->VisMark = VisQueryMarker;

        // Mark primitive visible
        primitive->VisPass = VisQueryMarker;

        if (Raycast.bClosest)
        {
            Raycast.HitProxyType   = HIT_PROXY_TYPE_PRIMITIVE;
            Raycast.HitPrimitive   = primitive;
            Raycast.HitDistanceMin = boxMin;
            Raycast.HitDistanceMax = boxMax;

#ifdef CLOSE_ENOUGH_EARLY_OUT
            // hit is close enough to stop ray casting?
            if (Raycast.HitDistanceMin < 0.0001f)
            {
                break;
            }
#endif
        }
        else
        {
            SBoxHitResult& hitResult = pBoundsRaycastResult->Append();

            hitResult.Object      = primitive->Owner;
            hitResult.LocationMin = Raycast.RayStart + Raycast.RayDir * boxMin;
            hitResult.LocationMax = Raycast.RayStart + Raycast.RayDir * boxMax;
            hitResult.DistanceMin = boxMin;
            hitResult.DistanceMax = boxMax;
        }
    }
}

#if 0
void AVSD::LevelRaycast_r( int InNodeIndex ) {
    SNodeBase const * node;
    float boxMin, boxMax;

    while ( 1 ) {
        if ( InNodeIndex < 0 ) {
            node = &CurLevel->Leafs[-1 - InNodeIndex];
        } else {
            node = CurLevel->Nodes.ToPtr() + InNodeIndex;
        }

        if ( node->ViewMark != NodeViewMark )
            return;

        if ( !BvRayIntersectBox( Raycast.RayStart, Raycast.InvRayDir, node->Bounds, boxMin, boxMax ) ) {
            return;
        }

        if ( boxMin >= Raycast.HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return;
        }

        if ( InNodeIndex < 0 ) {
            // leaf
            break;
        }

        SBinarySpaceNode const * n = static_cast< SBinarySpaceNode const * >( node );

        LevelRaycast_r( n->ChildrenIdx[0] );

#    ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if ( Raycast.HitDistanceMin < 0.0001f ) {
            return;
        }
#    endif

        InNodeIndex = n->ChildrenIdx[1];
    }

    SBinarySpaceLeaf const * pleaf = static_cast< SBinarySpaceLeaf const * >( node );

    RaycastArea( pleaf->Area );
}
#else
bool AVSD::LevelRaycast2_r(int InNodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd)
{

    if (InNodeIndex < 0)
    {

        SBinarySpaceLeaf const* leaf = &CurLevel->Leafs[-1 - InNodeIndex];

#    if 0
        // FIXME: Add this additional checks?
        float boxMin, boxMax;
        if ( !BvRayIntersectBox( Raycast.RayStart, Raycast.InvRayDir, leaf->Bounds, boxMin, boxMax ) ) {
            return false;
        }
        if ( boxMin >= Raycast.HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return false;
        }
#    endif

        RaycastArea(leaf->Area);

#    if 0
        if ( Raycast.RayLength > Raycast.HitDistanceMin ) {
        //if ( d >= Raycast.HitDistanceMin ) {
            // stop raycasting
            return true;
        }
#    endif

        // continue raycasting
        return false;
    }

    SBinarySpaceNode const* node = CurLevel->Nodes.ToPtr() + InNodeIndex;

    float d1, d2;

    if (node->Plane->Type < 3)
    {
        // Calc front distance
        d1 = InRayStart[node->Plane->Type] + node->Plane->D;

        // Calc back distance
        d2 = InRayEnd[node->Plane->Type] + node->Plane->D;
    }
    else
    {
        // Calc front distance
        d1 = node->Plane->DistanceToPoint(InRayStart);

        // Calc back distance
        d2 = node->Plane->DistanceToPoint(InRayEnd);
    }

    int side = d1 < 0;

    int front = node->ChildrenIdx[side];

    if ((d2 < 0) == side) // raystart & rayend on the same side of plane
    {
        if (front == 0)
        {
            // Solid
            return false;
        }

        return LevelRaycast2_r(front, InRayStart, InRayEnd);
    }

    // Calc intersection point
    float hitFraction;
#    if 0
#        define DIST_EPSILON 0.03125f
    if ( d1 < 0 ) {
        hitFraction = ( d1 + DIST_EPSILON ) / ( d1 - d2 );
    } else {
        hitFraction = ( d1 - DIST_EPSILON ) / ( d1 - d2 );
    }
#    else
    hitFraction = d1 / (d1 - d2);
#    endif
    hitFraction = Math::Clamp(hitFraction, 0.0f, 1.0f);

    Float3 mid = InRayStart + (InRayEnd - InRayStart) * hitFraction;

    // Traverse front side first
    if (front != 0 && LevelRaycast2_r(front, InRayStart, mid))
    {
        // Found closest ray intersection
        return true;
    }

    // Traverse back side
    int back = node->ChildrenIdx[side ^ 1];
    return back != 0 && LevelRaycast2_r(back, mid, InRayEnd);
}
#endif

#if 0
void AVSD::LevelRaycastBounds_r( int InNodeIndex ) {
    SNodeBase const * node;
    float boxMin, boxMax;

    while ( 1 ) {
        if ( InNodeIndex < 0 ) {
            node = &CurLevel->Leafs[-1 - InNodeIndex];
        } else {
            node = CurLevel->Nodes.ToPtr() + InNodeIndex;
        }

        if ( node->ViewMark != NodeViewMark )
            return;

        if ( !BvRayIntersectBox( Raycast.RayStart, Raycast.InvRayDir, node->Bounds, boxMin, boxMax ) ) {
            return;
        }

        if ( boxMin >= Raycast.HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return;
        }

        if ( InNodeIndex < 0 ) {
            // leaf
            break;
        }

        SBinarySpaceNode const * n = static_cast< SBinarySpaceNode const * >( node );

        LevelRaycastBounds_r( n->ChildrenIdx[0] );

#    ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if ( Raycast.HitDistanceMin < 0.0001f ) {
            return;
        }
#    endif

        InNodeIndex = n->ChildrenIdx[1];
    }

    SBinarySpaceLeaf const * pleaf = static_cast< SBinarySpaceLeaf const * >( node );

    RaycastPrimitiveBounds( pleaf->Area );
}
#else
bool AVSD::LevelRaycastBounds2_r(int InNodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd)
{

    if (InNodeIndex < 0)
    {

        SBinarySpaceLeaf const* leaf = &CurLevel->Leafs[-1 - InNodeIndex];

#    if 0
        float boxMin, boxMax;
        if ( !BvRayIntersectBox( InRayStart, Raycast.InvRayDir, leaf->Bounds, boxMin, boxMax ) ) {
            return false;
        }

        if ( boxMin >= Raycast.HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return false;
        }
#    endif

        RaycastPrimitiveBounds(leaf->Area);

        if (Raycast.RayLength > Raycast.HitDistanceMin)
        {
            //if ( d >= Raycast.HitDistanceMin ) {
            // stop raycasting
            return true;
        }

        // continue raycasting
        return false;
    }

    SBinarySpaceNode const* node = CurLevel->Nodes.ToPtr() + InNodeIndex;

    float d1, d2;

    if (node->Plane->Type < 3)
    {
        // Calc front distance
        d1 = InRayStart[node->Plane->Type] + node->Plane->D;

        // Calc back distance
        d2 = InRayEnd[node->Plane->Type] + node->Plane->D;
    }
    else
    {
        // Calc front distance
        d1 = node->Plane->DistanceToPoint(InRayStart);

        // Calc back distance
        d2 = node->Plane->DistanceToPoint(InRayEnd);
    }

    int side = d1 < 0;

    int front = node->ChildrenIdx[side];

    if ((d2 < 0) == side) // raystart & rayend on the same side of plane
    {
        if (front == 0)
        {
            // Solid
            return false;
        }

        return LevelRaycastBounds2_r(front, InRayStart, InRayEnd);
    }

    // Calc intersection point
    float hitFraction;
#    if 0
#        define DIST_EPSILON 0.03125f
    if ( d1 < 0 ) {
        hitFraction = ( d1 + DIST_EPSILON ) / ( d1 - d2 );
    } else {
        hitFraction = ( d1 - DIST_EPSILON ) / ( d1 - d2 );
    }
#    else
    hitFraction = d1 / (d1 - d2);
#    endif
    hitFraction = Math::Clamp(hitFraction, 0.0f, 1.0f);

    Float3 mid = InRayStart + (InRayEnd - InRayStart) * hitFraction;

    // Traverse front side first
    if (front != 0 && LevelRaycastBounds2_r(front, InRayStart, mid))
    {
        // Found closest ray intersection
        return true;
    }

    // Traverse back side
    int back = node->ChildrenIdx[side ^ 1];
    return back != 0 && LevelRaycastBounds2_r(back, mid, InRayEnd);
}
#endif

void AVSD::LevelRaycastPortals_r(SVisArea* InArea)
{

    RaycastArea(InArea);

    for (SPortalLink const* portal = InArea->PortalList; portal; portal = portal->Next)
    {

        if (portal->Portal->VisMark == VisQueryMarker)
        {
            // Already visited
            continue;
        }

        // Mark visited
        portal->Portal->VisMark = VisQueryMarker;

        if (portal->Portal->bBlocked)
        {
            // Portal is closed
            continue;
        }
#if 1
        // Calculate distance from ray origin to plane
        const float d1 = portal->Plane.DistanceToPoint(Raycast.RayStart);
        if (d1 <= 0.0f)
        {
            // ray is behind
            continue;
        }

        // Check ray direction
        const float d2 = Math::Dot(portal->Plane.Normal, Raycast.RayDir);
        if (d2 >= 0.0f)
        {
            // ray is parallel or has wrong direction
            continue;
        }

        // Calculate distance from ray origin to plane intersection
        const float dist = -(d1 / d2);

        HK_ASSERT(dist > 0.0f); // -0.0f
#else
        float dist;
        if (!BvRayIntersectPlane(Raycast.RayStart, Raycast.RayDir, portal->Plane, dist))
        {
            continue;
        }
        if (dist <= 0.0f)
        {
            continue;
        }
#endif

        if (dist >= Raycast.HitDistanceMin)
        {
            // Ray intersects the portal plane, but portal is too far
            continue;
        }

        const Float3 p = Raycast.RayStart + Raycast.RayDir * dist;

        if (!BvPointInConvexHullCCW(p, portal->Plane.Normal, portal->Hull->Points, portal->Hull->NumPoints))
        {
            continue;
        }

        LevelRaycastPortals_r(portal->ToArea);
    }
}

void AVSD::LevelRaycastBoundsPortals_r(SVisArea* InArea)
{

    RaycastPrimitiveBounds(InArea);

    for (SPortalLink const* portal = InArea->PortalList; portal; portal = portal->Next)
    {

        if (portal->Portal->VisMark == VisQueryMarker)
        {
            // Already visited
            continue;
        }

        // Mark visited
        portal->Portal->VisMark = VisQueryMarker;

        if (portal->Portal->bBlocked)
        {
            // Portal is closed
            continue;
        }

#if 1
        // Calculate distance from ray origin to plane
        const float d1 = portal->Plane.DistanceToPoint(Raycast.RayStart);
        if (d1 <= 0.0f)
        {
            // ray is behind
            continue;
        }

        // Check ray direction
        const float d2 = Math::Dot(portal->Plane.Normal, Raycast.RayDir);
        if (d2 >= 0.0f)
        {
            // ray is parallel or has wrong direction
            continue;
        }

        // Calculate distance from ray origin to plane intersection
        const float dist = -(d1 / d2);

        HK_ASSERT(dist > 0.0f); // -0.0f
#else
        float dist;
        if (!BvRayIntersectPlane(Raycast.RayStart, Raycast.RayDir, portal->Plane, dist))
        {
            continue;
        }
        if (dist <= 0.0f)
        {
            continue;
        }
#endif

        if (dist >= Raycast.HitDistanceMin)
        {
            // Ray intersects the portal plane, but portal is too far
            continue;
        }

        const Float3 p = Raycast.RayStart + Raycast.RayDir * dist;

        if (!BvPointInConvexHullCCW(p, portal->Plane.Normal, portal->Hull->Points, portal->Hull->NumPoints))
        {
            continue;
        }

        LevelRaycastBoundsPortals_r(portal->ToArea);
    }
}

void AVSD::ProcessLevelRaycast(ALevel* InLevel)
{
    CurLevel = InLevel;

    // TODO: check level bounds (ray/aabb overlap)?

    if (InLevel->VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = InLevel->FindLeaf( Raycast.RayStart );

        NodeViewMark = InLevel->MarkLeafs( leaf );

        LevelRaycast_r( 0 );
#else
        LevelRaycast2_r(0, Raycast.RayStart, Raycast.RayEnd);


        //InLevel-
#endif
    }
    else if (InLevel->VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = InLevel->FindArea(Raycast.RayStart);

        LevelRaycastPortals_r(area);
    }
}

void AVSD::ProcessLevelRaycastBounds(ALevel* InLevel)
{
    CurLevel = InLevel;

    // TODO: check level bounds (ray/aabb overlap)?

    if (InLevel->VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = InLevel->FindLeaf( Raycast.RayStart );

        NodeViewMark = InLevel->MarkLeafs( leaf );

        LevelRaycastBounds_r( 0 );
#else
        LevelRaycastBounds2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (InLevel->VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = InLevel->FindArea(Raycast.RayStart);

        LevelRaycastBoundsPortals_r(area);
    }
}

bool AVSD::RaycastTriangles(TPodVector<ALevel*> const& Levels, SWorldRaycastResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    VisQueryMask   = InFilter->QueryMask;
    VisibilityMask = InFilter->VisibilityMask;

    pRaycastResult = &Result;
    pRaycastResult->Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart    = InRayStart;
    Raycast.RayEnd      = InRayEnd;
    Raycast.RayDir      = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z = 1.0f / Raycast.RayDir.Z;
    //Raycast.HitObject is unused
    //Raycast.HitLocation is unused
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.bClosest       = false;

    // Set view position for face culling
    ViewPosition = Raycast.RayStart;

    for (ALevel* level : Levels)
    {
        ProcessLevelRaycast(level);
    }

    if (Result.Primitives.IsEmpty())
    {
        return false;
    }

    if (InFilter->bSortByDistance)
    {
        Result.Sort();
    }

    return true;
}

bool AVSD::RaycastClosest(TPodVector<ALevel*> const& Levels, SWorldRaycastClosestResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    VisQueryMask   = InFilter->QueryMask;
    VisibilityMask = InFilter->VisibilityMask;

    //pRaycastResult = &Result;
    //pRaycastResult->Clear();
    Result.Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart       = InRayStart;
    Raycast.RayEnd         = InRayEnd;
    Raycast.RayDir         = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X    = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y    = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z    = 1.0f / Raycast.RayDir.Z;
    Raycast.HitProxyType   = HIT_PROXY_TYPE_UNKNOWN;
    Raycast.HitLocation    = InRayEnd;
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.bClosest       = true;
    Raycast.pVertices      = nullptr;
    Raycast.pLightmapVerts = nullptr;
    Raycast.NumHits        = 0;

    // Set view position for face culling
    ViewPosition = Raycast.RayStart;

    for (ALevel* level : Levels)
    {
        ProcessLevelRaycast(level);

#ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if (Raycast.HitDistanceMin < 0.0001f)
        {
            break;
        }
#endif
    }

    //GLogger.Printf( "NumHits %d\n", Raycast.NumHits );

    if (Raycast.HitProxyType == HIT_PROXY_TYPE_PRIMITIVE)
    {
        Raycast.HitPrimitive->EvaluateRaycastResult(Raycast.HitPrimitive,
                                                    Raycast.LightingLevel,
                                                    Raycast.pVertices,
                                                    Raycast.pLightmapVerts,
                                                    Raycast.LightmapBlock,
                                                    Raycast.Indices,
                                                    Raycast.HitLocation,
                                                    Raycast.HitUV,
                                                    Result.Vertices,
                                                    Result.Texcoord,
                                                    Result.LightmapSample_Experimental);

        Result.Object = Raycast.HitPrimitive->Owner;
    }
    else if (Raycast.HitProxyType == HIT_PROXY_TYPE_SURFACE)
    {
        SMeshVertex const* vertices = Raycast.pVertices;

        Float3 const& v0 = vertices[Raycast.Indices[0]].Position;
        Float3 const& v1 = vertices[Raycast.Indices[1]].Position;
        Float3 const& v2 = vertices[Raycast.Indices[2]].Position;

        // surface vertices already in world space, so there is no need to transform them
        Result.Vertices[0] = v0;
        Result.Vertices[1] = v1;
        Result.Vertices[2] = v2;

        Result.Object = nullptr; // surfaces has no parent objects

        Raycast.HitNormal = Math::Cross(Result.Vertices[1] - Result.Vertices[0], Result.Vertices[2] - Result.Vertices[0]);
        Raycast.HitNormal.NormalizeSelf();

        const float hitW = 1.0f - Raycast.HitUV[0] - Raycast.HitUV[1];

        Float2 uv0      = vertices[Raycast.Indices[0]].GetTexCoord();
        Float2 uv1      = vertices[Raycast.Indices[1]].GetTexCoord();
        Float2 uv2      = vertices[Raycast.Indices[2]].GetTexCoord();
        Result.Texcoord = uv0 * hitW + uv1 * Raycast.HitUV[0] + uv2 * Raycast.HitUV[1];

        if (Raycast.pLightmapVerts && Raycast.LightingLevel && Raycast.LightmapBlock >= 0)
        {
            Float2 const& lm0             = Raycast.pLightmapVerts[Raycast.Indices[0]].TexCoord;
            Float2 const& lm1             = Raycast.pLightmapVerts[Raycast.Indices[1]].TexCoord;
            Float2 const& lm2             = Raycast.pLightmapVerts[Raycast.Indices[2]].TexCoord;
            Float2        lighmapTexcoord = lm0 * hitW + lm1 * Raycast.HitUV[0] + lm2 * Raycast.HitUV[1];

            ALevel const* level = Raycast.LightingLevel;

            Result.LightmapSample_Experimental = level->SampleLight(Raycast.LightmapBlock, lighmapTexcoord);
        }
    }
    else
    {
        // No intersection
        return false;
    }

    Result.Fraction = Raycast.HitDistanceMin / Raycast.RayLength;

    STriangleHitResult& triangleHit = Result.TriangleHit;
    triangleHit.Normal              = Raycast.HitNormal;
    triangleHit.Location            = Raycast.HitLocation;
    triangleHit.Distance            = Raycast.HitDistanceMin;
    triangleHit.Indices[0]          = Raycast.Indices[0];
    triangleHit.Indices[1]          = Raycast.Indices[1];
    triangleHit.Indices[2]          = Raycast.Indices[2];
    triangleHit.Material            = Raycast.Material;
    triangleHit.UV                  = Raycast.HitUV;

    return true;
}

bool AVSD::RaycastBounds(TPodVector<ALevel*> const& Levels, TPodVector<SBoxHitResult>& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    VisQueryMask   = InFilter->QueryMask;
    VisibilityMask = InFilter->VisibilityMask;

    pBoundsRaycastResult = &Result;
    pBoundsRaycastResult->Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart    = InRayStart;
    Raycast.RayEnd      = InRayEnd;
    Raycast.RayDir      = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z = 1.0f / Raycast.RayDir.Z;
    //Raycast.HitObject is unused
    //Raycast.HitLocation is unused
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.bClosest       = false;

    for (ALevel* level : Levels)
    {
        ProcessLevelRaycastBounds(level);
    }

    if (Result.IsEmpty())
    {
        return false;
    }

    if (InFilter->bSortByDistance)
    {
        struct ASortHit
        {
            bool operator()(SBoxHitResult const& _A, SBoxHitResult const& _B)
            {
                return (_A.DistanceMin < _B.DistanceMin);
            }
        } SortHit;

        std::sort(Result.ToPtr(), Result.ToPtr() + Result.Size(), SortHit);
    }

    return true;
}

bool AVSD::RaycastClosestBounds(TPodVector<ALevel*> const& Levels, SBoxHitResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    VisQueryMask   = InFilter->QueryMask;
    VisibilityMask = InFilter->VisibilityMask;

    Result.Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart     = InRayStart;
    Raycast.RayEnd       = InRayEnd;
    Raycast.RayDir       = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X  = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y  = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z  = 1.0f / Raycast.RayDir.Z;
    Raycast.HitProxyType = HIT_PROXY_TYPE_UNKNOWN;
    //Raycast.HitLocation is unused
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.HitDistanceMax = Raycast.RayLength;
    Raycast.bClosest       = true;

    for (ALevel* level : Levels)
    {
        ProcessLevelRaycastBounds(level);

#ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if (Raycast.HitDistanceMin < 0.0001f)
        {
            break;
        }
#endif
    }

    if (Raycast.HitProxyType == HIT_PROXY_TYPE_PRIMITIVE)
    {
        Result.Object = Raycast.HitPrimitive->Owner;
    }
    else if (Raycast.HitProxyType == HIT_PROXY_TYPE_SURFACE)
    {
        Result.Object = nullptr;
    }
    else
    {
        return false;
    }

    Result.LocationMin = InRayStart + Raycast.RayDir * Raycast.HitDistanceMin;
    Result.LocationMax = InRayStart + Raycast.RayDir * Raycast.HitDistanceMax;
    Result.DistanceMin = Raycast.HitDistanceMin;
    Result.DistanceMax = Raycast.HitDistanceMax;
    //Result.HitFractionMin = hitDistanceMin / rayLength;
    //Result.HitFractionMax = hitDistanceMax / rayLength;

    return true;
}

void AVSD::DrawDebug(ADebugRenderer* InRenderer)
{
#ifdef DEBUG_PORTAL_SCISSORS
    InRenderer->SetDepthTest(false);
    InRenderer->SetColor(Color4(0, 1, 0));

    Float3 Corners[4];

    for (SPortalScissor const& scissor : DebugScissors)
    {
        const Float3 Center   = ViewPosition + ViewPlane.Normal * ViewZNear;
        const Float3 RightMin = ViewRightVec * scissor.MinX + Center;
        const Float3 RightMax = ViewRightVec * scissor.MaxX + Center;
        const Float3 UpMin    = ViewUpVec * scissor.MinY;
        const Float3 UpMax    = ViewUpVec * scissor.MaxY;
        Corners[0]            = RightMin + UpMin;
        Corners[1]            = RightMax + UpMin;
        Corners[2]            = RightMax + UpMax;
        Corners[3]            = RightMin + UpMax;

        InRenderer->DrawLine(Corners, 4, true);
    }
#endif
}
