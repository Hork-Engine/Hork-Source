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

#include "VisibilitySystem.h"
#include "Level.h"
#include "Actor.h"
#include "World.h"
#include "SkinnedComponent.h"
#include "CameraComponent.h"
#include "PointLightComponent.h"
#include "PlayerController.h"
#include "Texture.h"
#include "Engine.h"

#include <Geometry/BV/BvIntersect.h>
#include <Geometry/ConvexHull.h>
#include <Core/IntrusiveLinkedListMacro.h>
#include <Core/ConsoleVar.h>

AConsoleVar com_DrawLevelAreaBounds("com_DrawLevelAreaBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawLevelIndoorBounds("com_DrawLevelIndoorBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawLevelPortals("com_DrawLevelPortals"s, "0"s, CVAR_CHEAT);

//AConsoleVar vsd_FrustumCullingType("vsd_FrustumCullingType"s, "0"s, 0, "0 - combined, 1 - separate, 2 - simple"s);

#define MAX_HULL_POINTS 128
struct SPortalHull
{
    int    NumPoints;
    Float3 Points[MAX_HULL_POINTS];
};

int AVisibilityLevel::VisQueryMarker = 0;

static const SWorldRaycastFilter DefaultRaycastFilter;

struct SVisibilityQueryContext
{
    enum
    {
        MAX_PORTAL_STACK = 128, //64
    };


    SPortalStack PortalStack[MAX_PORTAL_STACK];
    int          PortalStackPos;

    Float3 ViewPosition;
    Float3 ViewRightVec;
    Float3 ViewUpVec;
    PlaneF ViewPlane;
    float  ViewZNear;
    Float3 ViewCenter;

    VSD_QUERY_MASK   VisQueryMask   = VSD_QUERY_MASK(0);
    VISIBILITY_GROUP VisibilityMask = VISIBILITY_GROUP(0);
};

struct SVisibilityQueryResult
{
    TPodVector<SPrimitiveDef*>* pVisPrimitives;
    TPodVector<SSurfaceDef*>*   pVisSurfs;
};

AVisibilityLevel::AVisibilityLevel(SVisibilitySystemCreateInfo const& CreateInfo)
{
    Float3 extents(CONVEX_HULL_MAX_BOUNDS * 2);

    Platform::ZeroMem(&OutdoorArea, sizeof(OutdoorArea));

    OutdoorArea.Bounds.Mins = -extents * 0.5f;
    OutdoorArea.Bounds.Maxs = extents * 0.5f;

    PersistentLevel = CreateInfo.PersistentLevel;
    
    pOutdoorArea = PersistentLevel ? &PersistentLevel->OutdoorArea : &OutdoorArea;

    IndoorBounds.Clear();

    Areas.Resize(CreateInfo.NumAreas);
    Areas.ZeroMem();
    for (int i = 0 ; i < CreateInfo.NumAreas ; i++)
    {
        SVisArea& area = Areas[i];
        SVisibilityAreaDef& src = CreateInfo.Areas[i];

        area.Bounds = src.Bounds;
        area.FirstSurface = src.FirstSurface;
        area.NumSurfaces = src.NumSurfaces;

        IndoorBounds.AddAABB(area.Bounds);
    }

    SplitPlanes.Resize(CreateInfo.NumPlanes);
    Platform::Memcpy(SplitPlanes.ToPtr(), CreateInfo.Planes, CreateInfo.NumPlanes * sizeof(SplitPlanes[0]));

    Nodes.Resize(CreateInfo.NumNodes);
    for (int i = 0; i < CreateInfo.NumNodes; i++)
    {
        SBinarySpaceNode&          dst = Nodes[i];
        SBinarySpaceNodeDef const& src = CreateInfo.Nodes[i];

        dst.Parent         = src.Parent != -1 ? (Nodes.ToPtr() + src.Parent) : nullptr;
        dst.ViewMark       = 0;
        dst.Bounds         = src.Bounds;
        dst.Plane          = SplitPlanes.ToPtr() + src.PlaneIndex;
        dst.ChildrenIdx[0] = src.ChildrenIdx[0];
        dst.ChildrenIdx[1] = src.ChildrenIdx[1];
    }

    Leafs.Resize(CreateInfo.NumLeafs);
    for (int i = 0; i < CreateInfo.NumLeafs; i++)
    {
        SBinarySpaceLeaf&          dst = Leafs[i];
        SBinarySpaceLeafDef const& src = CreateInfo.Leafs[i];

        dst.Parent     = src.Parent != -1 ? (Nodes.ToPtr() + src.Parent) : nullptr;
        dst.ViewMark   = 0;
        dst.Bounds     = src.Bounds;
        dst.PVSCluster = src.PVSCluster;
        dst.Visdata    = nullptr;
        dst.AudioArea  = src.AudioArea;
        dst.Area       = Areas.ToPtr() + src.AreaNum;
    }

    if (CreateInfo.PortalsCount > 0)
    {
        VisibilityMethod = LEVEL_VISIBILITY_PORTAL;

        CreatePortals(CreateInfo.Portals, CreateInfo.PortalsCount, CreateInfo.HullVertices);
    }
    else if (CreateInfo.PVS)
    {
        VisibilityMethod = LEVEL_VISIBILITY_PVS;

        Visdata = (byte*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(CreateInfo.PVS->VisdataSize);
        Platform::Memcpy(Visdata, CreateInfo.PVS->Visdata, CreateInfo.PVS->VisdataSize);

        bCompressedVisData = CreateInfo.PVS->bCompressedVisData;
        PVSClustersCount   = CreateInfo.PVS->ClustersCount;

        for (int i = 0; i < CreateInfo.NumLeafs; i++)
        {
            SBinarySpaceLeaf&          dst = Leafs[i];
            SBinarySpaceLeafDef const& src = CreateInfo.Leafs[i];

            if (src.VisdataOffset < CreateInfo.PVS->VisdataSize)
            {
                dst.Visdata = Visdata + src.VisdataOffset;
            }
            else
            {
                dst.Visdata = nullptr;
            }
        }
    }

    if (bCompressedVisData && Visdata && PVSClustersCount > 0)
    {
        // Allocate decompressed vis data
        DecompressedVisData = (byte*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc((PVSClustersCount + 7) >> 3);
    }

    AreaSurfaces.Resize(CreateInfo.NumAreaSurfaces);
    Platform::Memcpy(AreaSurfaces.ToPtr(), CreateInfo.AreaSurfaces, sizeof(AreaSurfaces[0]) * CreateInfo.NumAreaSurfaces);

    Model = CreateInfo.Model;
}

AVisibilityLevel::~AVisibilityLevel()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(Visdata);
    Platform::GetHeapAllocator<HEAP_MISC>().Free(DecompressedVisData);
}

void AVisibilityLevel::CreatePortals(SPortalDef const* InPortals, int InPortalsCount, Float3 const* InHullVertices)
{
    AConvexHull* hull;
    PlaneF       hullPlane;
    SPortalLink* portalLink;
    int          portalLinkNum;

    Portals.ResizeInvalidate(InPortalsCount);
    AreaLinks.ResizeInvalidate(Portals.Size() << 1);

    portalLinkNum = 0;

    for (int i = 0; i < InPortalsCount; i++)
    {
        SPortalDef const* def    = InPortals + i;
        SVisPortal&       portal = Portals[i];

        SVisArea* a1 = def->Areas[0] >= 0 ? &Areas[def->Areas[0]] : pOutdoorArea;
        SVisArea* a2 = def->Areas[1] >= 0 ? &Areas[def->Areas[1]] : pOutdoorArea;
#if 0
        if ( a1 == pOutdoorArea ) {
            std::swap( a1, a2 );
        }

        // Check area position relative to portal plane
        float d = portal.Plane.Dist( a1->ReferencePoint );

        // If area position is on back side of plane, then reverse hull vertices and plane
        int id = d < 0.0f;
#else
        int id = 0;
#endif

        hull      = AConvexHull::CreateFromPoints(InHullVertices + def->FirstVert, def->NumVerts);
        hullPlane = hull->CalcPlane();

        portalLink         = &AreaLinks[portalLinkNum++];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a2;
        if (id & 1)
        {
            portalLink->Hull  = hull;
            portalLink->Plane = hullPlane;
        }
        else
        {
            portalLink->Hull  = hull->Reversed();
            portalLink->Plane = -hullPlane;
        }
        portalLink->Next   = a1->PortalList;
        portalLink->Portal = &portal;
        a1->PortalList     = portalLink;

        id = (id + 1) & 1;

        portalLink         = &AreaLinks[portalLinkNum++];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a1;
        if (id & 1)
        {
            portalLink->Hull  = hull;
            portalLink->Plane = hullPlane;
        }
        else
        {
            portalLink->Hull  = hull->Reversed();
            portalLink->Plane = -hullPlane;
        }
        portalLink->Next   = a2->PortalList;
        portalLink->Portal = &portal;
        a2->PortalList     = portalLink;

        portal.bBlocked = false;
    }
}

int AVisibilityLevel::FindLeaf(Float3 const& InPosition)
{
    SBinarySpaceNode* node;
    float             d;
    int               nodeIndex;

    if (Nodes.IsEmpty())
    {
        return -1;
    }

    node = Nodes.ToPtr();
    while (1)
    {
        d = node->Plane->DistFast(InPosition);

        // Choose child
        nodeIndex = node->ChildrenIdx[d <= 0];

        if (nodeIndex <= 0)
        {
            // solid if node index == 0 or leaf if node index < 0
            return -1 - nodeIndex;
        }

        node = Nodes.ToPtr() + nodeIndex;
    }
    return -1;
}

SVisArea* AVisibilityLevel::FindArea(Float3 const& InPosition)
{
    if (!Nodes.IsEmpty())
    {
        int leaf = FindLeaf(InPosition);
        if (leaf < 0)
        {
            // solid
            return pOutdoorArea;
        }
        return Leafs[leaf].Area;
    }

    // Bruteforce TODO: remove this!
    for (int i = 0; i < Areas.Size(); i++)
    {
        if (InPosition.X >= Areas[i].Bounds.Mins.X && InPosition.Y >= Areas[i].Bounds.Mins.Y && InPosition.Z >= Areas[i].Bounds.Mins.Z && InPosition.X < Areas[i].Bounds.Maxs.X && InPosition.Y < Areas[i].Bounds.Maxs.Y && InPosition.Z < Areas[i].Bounds.Maxs.Z)
        {
            return &Areas[i];
        }
    }

    return pOutdoorArea;
}

byte const* AVisibilityLevel::DecompressVisdata(byte const* InCompressedData)
{
    int count;

    int   row           = (PVSClustersCount + 7) >> 3;
    byte* pDecompressed = DecompressedVisData;

    do {
        // Copy raw data
        if (*InCompressedData)
        {
            *pDecompressed++ = *InCompressedData++;
            continue;
        }

        // Zeros count
        count = InCompressedData[1];

        // Clamp zeros count if invalid. This can be moved to preprocess stage.
        if (pDecompressed - DecompressedVisData + count > row)
        {
            count = row - (pDecompressed - DecompressedVisData);
        }

        // Move to the next sequence
        InCompressedData += 2;

        while (count--)
        {
            *pDecompressed++ = 0;
        }
    } while (pDecompressed - DecompressedVisData < row);

    return DecompressedVisData;
}

byte const* AVisibilityLevel::LeafPVS(SBinarySpaceLeaf const* InLeaf)
{
    if (bCompressedVisData)
    {
        return InLeaf->Visdata ? DecompressVisdata(InLeaf->Visdata) : nullptr;
    }
    else
    {
        return InLeaf->Visdata;
    }
}

int AVisibilityLevel::MarkLeafs(int InViewLeaf)
{
    if (VisibilityMethod != LEVEL_VISIBILITY_PVS)
    {
        LOG("ALevel::MarkLeafs: expect LEVEL_VISIBILITY_PVS\n");
        return ViewMark;
    }

    if (InViewLeaf < 0)
    {
        return ViewMark;
    }

    SBinarySpaceLeaf* pLeaf = &Leafs[InViewLeaf];

    if (ViewCluster == pLeaf->PVSCluster)
    {
        return ViewMark;
    }

    ViewMark++;
    ViewCluster = pLeaf->PVSCluster;

    byte const* pVisibility = LeafPVS(pLeaf);
    if (pVisibility)
    {
        int cluster;
        for (SBinarySpaceLeaf& leaf : Leafs)
        {

            cluster = leaf.PVSCluster;

            if (cluster < 0 || cluster >= PVSClustersCount || !(pVisibility[cluster >> 3] & (1 << (cluster & 7))))
            {
                continue;
            }

            // TODO: check doors here

            SNodeBase* parent = &leaf;
            do {
                if (parent->ViewMark == ViewMark)
                {
                    break;
                }
                parent->ViewMark = ViewMark;
                parent           = parent->Parent;
            } while (parent);
        }
    }
    else
    {
        // Mark all
        for (SBinarySpaceLeaf& leaf : Leafs)
        {
            SNodeBase* parent = &leaf;
            do {
                if (parent->ViewMark == ViewMark)
                {
                    break;
                }
                parent->ViewMark = ViewMark;
                parent           = parent->Parent;
            } while (parent);
        }
    }

    return ViewMark;
}

void AVisibilityLevel::QueryOverplapAreas_r(int NodeIndex, BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& OverlappedAreas)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            SVisArea* area = Leafs[-1 - NodeIndex].Area;
            OverlappedAreas.AddUnique(area);
            return;
        }

        SBinarySpaceNode* node = &Nodes[NodeIndex];

        // TODO: precalc signbits
        int sideMask = BvBoxOverlapPlaneSideMask(Bounds, *node->Plane, node->Plane->Type, node->Plane->SignBits());

        if (sideMask == 1)
        {
            NodeIndex = node->ChildrenIdx[0];
        }
        else if (sideMask == 2)
        {
            NodeIndex = node->ChildrenIdx[1];
        }
        else
        {
            if (node->ChildrenIdx[1] != 0)
            {
                QueryOverplapAreas_r(node->ChildrenIdx[1], Bounds, OverlappedAreas);
            }
            NodeIndex = node->ChildrenIdx[0];
        }
    } while (NodeIndex != 0);
}

void AVisibilityLevel::QueryOverplapAreas_r(int NodeIndex, BvSphere const& Bounds, TPodVector<SVisArea*>& OverlappedAreas)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            SVisArea* area = Leafs[-1 - NodeIndex].Area;
            OverlappedAreas.AddUnique(area);
            return;
        }

        SBinarySpaceNode* node = &Nodes[NodeIndex];

        float d = node->Plane->DistFast(Bounds.Center);

        if (d > Bounds.Radius)
        {
            NodeIndex = node->ChildrenIdx[0];
        }
        else if (d < -Bounds.Radius)
        {
            NodeIndex = node->ChildrenIdx[1];
        }
        else
        {
            if (node->ChildrenIdx[1] != 0)
            {
                QueryOverplapAreas_r(node->ChildrenIdx[1], Bounds, OverlappedAreas);
            }
            NodeIndex = node->ChildrenIdx[0];
        }
    } while (NodeIndex != 0);
}

void AVisibilityLevel::QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& OverlappedAreas)
{
    if (Nodes.IsEmpty())
    {
        return;
    }

    QueryOverplapAreas_r(0, Bounds, OverlappedAreas);
}

void AVisibilityLevel::QueryOverplapAreas(BvSphere const& Bounds, TPodVector<SVisArea*>& OverlappedAreas)
{
    if (Nodes.IsEmpty())
    {
        return;
    }

    QueryOverplapAreas_r(0, Bounds, OverlappedAreas);
}

static SPrimitiveLink** LastLink;

static HK_FORCEINLINE bool IsPrimitiveInArea(SPrimitiveDef const* Primitive, SVisArea const* InArea)
{
    for (SPrimitiveLink const* link = Primitive->Links; link; link = link->Next)
    {
        if (link->Area == InArea)
        {
            return true;
        }
    }
    return false;
}

void AVisibilityLevel::AddPrimitiveToArea(SVisArea* Area, SPrimitiveDef* Primitive)
{
    if (IsPrimitiveInArea(Primitive, Area))
    {
        return;
    }

    SPrimitiveLink* link = AVisibilitySystem::PrimitiveLinkPool.Allocate();
    if (!link)
    {
        return;
    }

    link->Primitive = Primitive;

    // Create the primitive link
    *LastLink  = link;
    LastLink   = &link->Next;
    link->Next = nullptr;

    // Create the area links
    link->Area       = Area;
    link->NextInArea = Area->Links;
    Area->Links      = link;
}

void AVisibilityLevel::AddBoxRecursive(int NodeIndex, SPrimitiveDef* Primitive)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            AddPrimitiveToArea(Leafs[-1 - NodeIndex].Area, Primitive);
            return;
        }

        SBinarySpaceNode* node = &Nodes[NodeIndex];

        // TODO: precalc signbits
        int sideMask = BvBoxOverlapPlaneSideMask(Primitive->Box, *node->Plane, node->Plane->Type, node->Plane->SignBits());

        if (sideMask == 1)
        {
            NodeIndex = node->ChildrenIdx[0];
        }
        else if (sideMask == 2)
        {
            NodeIndex = node->ChildrenIdx[1];
        }
        else
        {
            if (node->ChildrenIdx[1] != 0)
            {
                AddBoxRecursive(node->ChildrenIdx[1], Primitive);
            }
            NodeIndex = node->ChildrenIdx[0];
        }
    } while (NodeIndex != 0);
}

void AVisibilityLevel::AddSphereRecursive(int NodeIndex, SPrimitiveDef* Primitive)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            AddPrimitiveToArea(Leafs[-1 - NodeIndex].Area, Primitive);
            return;
        }

        SBinarySpaceNode* node = &Nodes[NodeIndex];

        float d = node->Plane->DistFast(Primitive->Sphere.Center);

        if (d > Primitive->Sphere.Radius)
        {
            NodeIndex = node->ChildrenIdx[0];
        }
        else if (d < -Primitive->Sphere.Radius)
        {
            NodeIndex = node->ChildrenIdx[1];
        }
        else
        {
            if (node->ChildrenIdx[1] != 0)
            {
                AddSphereRecursive(node->ChildrenIdx[1], Primitive);
            }
            NodeIndex = node->ChildrenIdx[0];
        }
    } while (NodeIndex != 0);
}

void AVisibilityLevel::AddPrimitiveToLevelAreas(TPodVector<AVisibilityLevel*> const& Levels, SPrimitiveDef* Primitive)
{
    bool bInsideArea{};

    if (Levels.IsEmpty())
        return;

    if (Primitive->bIsOutdoor)
    {
        // add to outdoor
        Levels[0]->AddPrimitiveToArea(Levels[0]->pOutdoorArea, Primitive);
        return;
    }

    // TODO: Check overlap with portal polygons between indoor and outdoor areas

    LastLink = &Primitive->Links;

    for (AVisibilityLevel* level : Levels)
    {
        bool bHaveBinaryTree = level->Nodes.Size() > 0;

        if (bHaveBinaryTree)
        {
            switch (Primitive->Type)
            {
                case VSD_PRIMITIVE_BOX:
                    level->AddBoxRecursive(0, Primitive);
                    break;
                case VSD_PRIMITIVE_SPHERE:
                    level->AddSphereRecursive(0, Primitive);
                    break;
            }
            bInsideArea = true;
        }
        else
        {
            // No binary tree. Use bruteforce.

            // TODO: remove this path

            int numAreas = level->Areas.Size();

            switch (Primitive->Type)
            {
                case VSD_PRIMITIVE_BOX: {
                    if (BvBoxOverlapBox(level->IndoorBounds, Primitive->Box))
                    {
                        for (int i = 0; i < numAreas; i++)
                        {
                            SVisArea* area = &level->Areas[i];

                            if (BvBoxOverlapBox(area->Bounds, Primitive->Box))
                            {
                                level->AddPrimitiveToArea(area, Primitive);

                                bInsideArea = true;
                            }
                        }
                    }
                    break;
                }

                case VSD_PRIMITIVE_SPHERE: {
                    if (BvBoxOverlapSphere(level->IndoorBounds, Primitive->Sphere))
                    {
                        for (int i = 0; i < numAreas; i++)
                        {
                            SVisArea* area = &level->Areas[i];

                            if (BvBoxOverlapSphere(area->Bounds, Primitive->Sphere))
                            {
                                level->AddPrimitiveToArea(area, Primitive);

                                bInsideArea = true;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    if (!bInsideArea)
    {
        // add to outdoor
        Levels[0]->AddPrimitiveToArea(Levels[0]->pOutdoorArea, Primitive);
    }
}

void AVisibilityLevel::DrawDebug(ADebugRenderer* InRenderer)
{
    if (com_DrawLevelAreaBounds)
    {
        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(0, 1, 0, 0.5f));
        for (SVisArea& area : Areas)
        {
            InRenderer->DrawAABB(area.Bounds);
        }
    }

    if (com_DrawLevelPortals)
    {
        //        InRenderer->SetDepthTest( false );
        //        InRenderer->SetColor(1,0,0,1);
        //        for ( ALevelPortal & portal : Portals ) {
        //            InRenderer->DrawLine( portal.Hull->Points, portal.Hull->NumPoints, true );
        //        }

        InRenderer->SetDepthTest(false);
        //InRenderer->SetColor( Color4( 0,0,1,0.4f ) );

        /*if ( LastVisitedArea >= 0 && LastVisitedArea < Areas.Size() ) {
            VSDArea * area = &Areas[ LastVisitedArea ];
            VSDPortalLink * portals = area->PortalList;

        for ( VSDPortalLink * p = portals; p; p = p->Next ) {
            InRenderer->DrawConvexPoly( p->Hull->Points, p->Hull->NumPoints, true );
        }
    } else */
        {

#if 0
            for ( SVisPortal & portal : Portals ) {

                if ( portal.VisMark == InRenderer->GetVisPass() ) {
                    InRenderer->SetColor( Color4( 1,0,0,0.4f ) );
                } else {
                    InRenderer->SetColor( Color4( 0,1,0,0.4f ) );
                }
                InRenderer->DrawConvexPoly( portal.Hull->Points, portal.Hull->NumPoints, true );
            }
#else
            if (!PersistentLevel)
            {
                SPortalLink* portals = OutdoorArea.PortalList;

                for (SPortalLink* p = portals; p; p = p->Next)
                {

                    if (p->Portal->VisMark == InRenderer->GetVisPass())
                    {
                        InRenderer->SetColor(Color4(1, 0, 0, 0.4f));
                    }
                    else
                    {
                        InRenderer->SetColor(Color4(0, 1, 0, 0.4f));
                    }

                    InRenderer->DrawConvexPoly({p->Hull->Points, (size_t)p->Hull->NumPoints}, false);
                }
            }

            for (SVisArea& area : Areas)
            {
                SPortalLink* portals = area.PortalList;

                for (SPortalLink* p = portals; p; p = p->Next)
                {

                    if (p->Portal->VisMark == InRenderer->GetVisPass())
                    {
                        InRenderer->SetColor(Color4(1, 0, 0, 0.4f));
                    }
                    else
                    {
                        InRenderer->SetColor(Color4(0, 1, 0, 0.4f));
                    }

                    InRenderer->DrawConvexPoly({p->Hull->Points, (size_t)p->Hull->NumPoints}, false);
                }
            }
#endif
        }
    }

    if (com_DrawLevelIndoorBounds)
    {
        InRenderer->SetDepthTest(false);
        InRenderer->DrawAABB(IndoorBounds);
    }

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

void AVisibilityLevel::ProcessLevelVisibility(SVisibilityQueryContext& QueryContext, SVisibilityQueryResult& QueryResult)
{
    pQueryContext = &QueryContext;
    pQueryResult = &QueryResult;

    ViewFrustum       = QueryContext.PortalStack[0].AreaFrustum;
    ViewFrustumPlanes = QueryContext.PortalStack[0].PlanesCount; // Can be 4 or 5

    int cullBits = 0;

    for (int i = 0; i < ViewFrustumPlanes; i++)
    {
        CachedSignBits[i] = ViewFrustum[i].SignBits();

        cullBits |= 1 << i;
    }

    if (VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has PVS

        int leaf = FindLeaf(QueryContext.ViewPosition);

        NodeViewMark = MarkLeafs(leaf);

        //DEBUG( "NodeViewMark {}\n", NodeViewMark );

        //HK_ASSERT( NodeViewMark != 0 );

        LevelTraverse_r(0, cullBits);
    }
    else if (VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = FindArea(QueryContext.ViewPosition);

        FlowThroughPortals_r(area);
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

bool AVisibilityLevel::CullNode(PlaneF const InFrustum[SPortalStack::MAX_CULL_PLANES], BvAxisAlignedBox const& Bounds, int& InCullBits)
{
    Float3 p;

    float const* pBounds = Bounds.ToPtr();
    int const*   pIndices;
    if (InCullBits & 1)
    {
        pIndices = CullIndices[CachedSignBits[0]];

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
        pIndices = CullIndices[CachedSignBits[1]];

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
        pIndices = CullIndices[CachedSignBits[2]];

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
        pIndices = CullIndices[CachedSignBits[3]];

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
        pIndices = CullIndices[CachedSignBits[4]];

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

HK_INLINE bool VSD_CullBoxSingle(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvAxisAlignedBox const& Bounds)
{
    bool inside = true;

    for (int i = 0; i < InCullPlanesCount; i++)
    {

        PlaneF const* p = &InCullPlanes[i];

        inside &= (Math::Max(Bounds.Mins.X * p->Normal.X, Bounds.Maxs.X * p->Normal.X) + Math::Max(Bounds.Mins.Y * p->Normal.Y, Bounds.Maxs.Y * p->Normal.Y) + Math::Max(Bounds.Mins.Z * p->Normal.Z, Bounds.Maxs.Z * p->Normal.Z) + p->D) > 0.0f;
    }

    return !inside;
}

HK_INLINE bool VSD_CullSphereSingle(PlaneF const* InCullPlanes, const int InCullPlanesCount, BvSphere const& Bounds)
{
#if 0
    bool cull = false;
    for ( int i = 0 ; i < InCullPlanesCount ; i++ ) {

        PlaneF const * p = &InCullPlanes[i];

        if ( Math::Dot( p->Normal, Bounds->Center ) + p->D <= -Bounds->Radius ) {
            cull = true;
        }
    }
    return cull;
#endif

    bool inside = true;
    for (int i = 0; i < InCullPlanesCount; i++)
    {

        PlaneF const* p = &InCullPlanes[i];

        inside &= (Math::Dot(p->Normal, Bounds.Center) + p->D > -Bounds.Radius);
    }
    return !inside;
}

void AVisibilityLevel::LevelTraverse_r(int NodeIndex, int InCullBits)
{
    SNodeBase const* node;

    while (1)
    {
        if (NodeIndex < 0)
        {
            node = &Leafs[-1 - NodeIndex];
        }
        else
        {
            node = Nodes.ToPtr() + NodeIndex;
        }

        if (node->ViewMark != NodeViewMark)
            return;

        if (CullNode(ViewFrustum, node->Bounds, InCullBits))
        {
            //TotalCulled++;
            return;
        }

#if 0
        if ( VSD_CullBoxSingle( ViewFrustum, ViewFrustumPlanes, node->Bounds ) ) {
            Dbg_CullMiss++;
        }
#endif

        if (NodeIndex < 0)
        {
            // leaf
            break;
        }

        LevelTraverse_r(static_cast<SBinarySpaceNode const*>(node)->ChildrenIdx[0], InCullBits);

        NodeIndex = static_cast<SBinarySpaceNode const*>(node)->ChildrenIdx[1];
    }

    SBinarySpaceLeaf const* pleaf = static_cast<SBinarySpaceLeaf const*>(node);

    CullPrimitives(pleaf->Area, ViewFrustum, ViewFrustumPlanes);
}

void AVisibilityLevel::FlowThroughPortals_r(SVisArea const* InArea)
{
    SPortalStack* prevStack = &pQueryContext->PortalStack[pQueryContext->PortalStackPos];
    SPortalStack* stack     = prevStack + 1;

    CullPrimitives(InArea, prevStack->AreaFrustum, prevStack->PlanesCount);

    if (pQueryContext->PortalStackPos == (SVisibilityQueryContext::MAX_PORTAL_STACK - 1))
    {
        LOG("MAX_PORTAL_STACK hit\n");
        return;
    }

    ++pQueryContext->PortalStackPos;

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

    --pQueryContext->PortalStackPos;
}

bool AVisibilityLevel::CalcPortalStack(SPortalStack* OutStack, SPortalStack const* InPrevStack, SPortalLink const* InPortal)
{
    const float d = InPortal->Plane.DistanceToPoint(pQueryContext->ViewPosition);
    if (d <= 0.0f)
    {
#ifdef DEBUG_TRAVERSING_COUNTERS
        Dbg_SkippedByPlaneOffset++;
#endif
        return false;
    }

    if (d <= pQueryContext->ViewZNear)
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
        //        LOG( "Recursive!\n" );
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
                OutStack->AreaFrustum[i].FromPoints(pQueryContext->ViewPosition, portalWinding->Points[i], portalWinding->Points[(i + 1) % portalWinding->NumPoints]);
            }

            // Copy far plane
            OutStack->AreaFrustum[OutStack->PlanesCount++] = InPrevStack->AreaFrustum[InPrevStack->PlanesCount - 1];
        }
        else
        {
            // Compute based on portal scissor
            const Float3 rightMin = pQueryContext->ViewRightVec * OutStack->Scissor.MinX + pQueryContext->ViewCenter;
            const Float3 rightMax = pQueryContext->ViewRightVec * OutStack->Scissor.MaxX + pQueryContext->ViewCenter;
            const Float3 upMin    = pQueryContext->ViewUpVec * OutStack->Scissor.MinY;
            const Float3 upMax    = pQueryContext->ViewUpVec * OutStack->Scissor.MaxY;
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
            OutStack->AreaFrustum[0].D      = -Math::Dot(OutStack->AreaFrustum[0].Normal, pQueryContext->ViewPosition);

            // right
            p                               = Math::Cross(corners[2], corners[1]);
            OutStack->AreaFrustum[1].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[1].D      = -Math::Dot(OutStack->AreaFrustum[1].Normal, pQueryContext->ViewPosition);

            // top
            p                               = Math::Cross(corners[3], corners[2]);
            OutStack->AreaFrustum[2].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[2].D      = -Math::Dot(OutStack->AreaFrustum[2].Normal, pQueryContext->ViewPosition);

            // left
            p                               = Math::Cross(corners[0], corners[3]);
            OutStack->AreaFrustum[3].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[3].D      = -Math::Dot(OutStack->AreaFrustum[3].Normal, pQueryContext->ViewPosition);

            //OutStack->PlanesCount = 4;

            // Copy far plane
            OutStack->AreaFrustum[4] = InPrevStack->AreaFrustum[InPrevStack->PlanesCount - 1];

            OutStack->PlanesCount = 5;
        }
    }

#ifdef DEBUG_PORTAL_SCISSORS
    DebugScissors.Add(OutStack->Scissor);
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

static bool ClipPolygonFast(Float3 const* InPoints, const int InNumPoints, SPortalHull* Out, PlaneF const& InClipPlane, const float InEpsilon)
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

SPortalHull* AVisibilityLevel::CalcPortalWinding(SPortalLink const* InPortal, SPortalStack const* InStack)
{
    static SPortalHull PortalHull[2];

    int flip = 0;

    // Clip portal hull by view plane
    if (!ClipPolygonFast(InPortal->Hull->Points, InPortal->Hull->NumPoints, &PortalHull[flip], pQueryContext->ViewPlane, 0.0f))
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

void AVisibilityLevel::CalcPortalScissor(SPortalScissor& OutScissor, SPortalHull const* InHull, SPortalStack const* InStack)
{
    OutScissor.MinX = 99999999.0f;
    OutScissor.MinY = 99999999.0f;
    OutScissor.MaxX = -99999999.0f;
    OutScissor.MaxY = -99999999.0f;

    for (int i = 0; i < InHull->NumPoints; i++)
    {
        // Project portal vertex to view plane
        const Float3 vec = InHull->Points[i] - pQueryContext->ViewPosition;

        const float d = Math::Dot(pQueryContext->ViewPlane.Normal, vec);

        //if ( d < ViewZNear ) {
        //    HK_ASSERT(0);
        //}

        const Float3 p = d < pQueryContext->ViewZNear ? vec : vec * (pQueryContext->ViewZNear / d);

        // Compute relative coordinates
        const float x = Math::Dot(pQueryContext->ViewRightVec, p);
        const float y = Math::Dot(pQueryContext->ViewUpVec, p);

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

HK_FORCEINLINE bool AVisibilityLevel::FaceCull(SPrimitiveDef const* InPrimitive)
{
    return InPrimitive->Face.DistanceToPoint(pQueryContext->ViewPosition) < 0.0f;
}

HK_FORCEINLINE bool AVisibilityLevel::FaceCull(SSurfaceDef const* InSurface)
{
    return InSurface->Face.DistanceToPoint(pQueryContext->ViewPosition) < 0.0f;
}

void AVisibilityLevel::CullPrimitives(SVisArea const* InArea, PlaneF const* InCullPlanes, const int InCullPlanesCount)
{
/*!!!
    if (vsd_FrustumCullingType.GetInteger() != CULLING_TYPE_COMBINED)
    {
        BoxPrimitives.Clear();
        BoundingBoxesSSE.Clear();
        CullSubmits.Clear();
    }

    int numBoxes          = 0;
    int firstBoxPrimitive = BoxPrimitives.Size();
*/

    if (InArea->NumSurfaces > 0)
    {
        ABrushModel* model = Model;

        int const* pSurfaceIndex = &AreaSurfaces[InArea->FirstSurface];

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
            if ((surf->QueryGroup & pQueryContext->VisQueryMask) != pQueryContext->VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & pQueryContext->VisibilityMask) == 0)
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

            pQueryResult->pVisSurfs->Add(surf);
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
        if ((primitive->QueryGroup & pQueryContext->VisQueryMask) != pQueryContext->VisQueryMask)
        {
            // Mark primitive visibility processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & pQueryContext->VisibilityMask) == 0)
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
/*!!!                if (vsd_FrustumCullingType.GetInteger() == CULLING_TYPE_SIMPLE) */
                {
                    if (VSD_CullBoxSingle(InCullPlanes, InCullPlanesCount, primitive->Box))
                    {

#ifdef DEBUG_TRAVERSING_COUNTERS
                        Dbg_CulledByPrimitiveBounds++;
#endif

                        continue;
                    }
                }
/*!!!
                else
                {
                    // Prepare primitive for frustum culling
                    BoxPrimitives.Add(primitive);
                    BoundingBoxesSSE.Add() = primitive->Box;
                    numBoxes++;
                    continue;
                }
*/
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
        pQueryResult->pVisPrimitives->Add(primitive);
    }

/*!!!
    if (numBoxes > 0)
    {
        // Create job submit

        SCullJobSubmit& submit = CullSubmits.Add();

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

                        pQueryResult->pVisPrimitives->Add(primitive);
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
*/
}

void AVisibilityLevel::QueryVisiblePrimitives(TPodVector<AVisibilityLevel*> const& Levels, TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& InQuery)
{
    //int QueryVisiblePrimitivesTime = GEngine->SysMicroseconds();
    SVisibilityQueryContext QueryContext;
    SVisibilityQueryResult QueryResult;

    ++VisQueryMarker;

    if (VisPass)
    {
        *VisPass = VisQueryMarker;
    }

    QueryContext.VisQueryMask   = InQuery.QueryMask;
    QueryContext.VisibilityMask = InQuery.VisibilityMask;

    QueryResult.pVisPrimitives = &VisPrimitives;
    QueryResult.pVisPrimitives->Clear();

    QueryResult.pVisSurfs = &VisSurfs;
    QueryResult.pVisSurfs->Clear();

/*!!!
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
*/
    QueryContext.ViewPosition = InQuery.ViewPosition;
    QueryContext.ViewRightVec = InQuery.ViewRightVec;
    QueryContext.ViewUpVec    = InQuery.ViewUpVec;
    QueryContext.ViewPlane    = *InQuery.FrustumPlanes[FRUSTUM_PLANE_NEAR];
    QueryContext.ViewZNear    = -QueryContext.ViewPlane.DistanceToPoint(QueryContext.ViewPosition); //Camera->GetZNear();
    QueryContext.ViewCenter   = QueryContext.ViewPlane.Normal * QueryContext.ViewZNear;

    // Get corner at left-bottom of frustum
    Float3 corner = Math::Cross(InQuery.FrustumPlanes[FRUSTUM_PLANE_BOTTOM]->Normal, InQuery.FrustumPlanes[FRUSTUM_PLANE_LEFT]->Normal);

    // Project left-bottom corner to near plane
    corner = corner * (QueryContext.ViewZNear / Math::Dot(QueryContext.ViewPlane.Normal, corner));

    const float x = Math::Dot(QueryContext.ViewRightVec, corner);
    const float y = Math::Dot(QueryContext.ViewUpVec, corner);

    // w = tan( half_fov_x_rad ) * znear * 2;
    // h = tan( half_fov_y_rad ) * znear * 2;

    QueryContext.PortalStackPos                = 0;
    QueryContext.PortalStack[0].AreaFrustum[0] = *InQuery.FrustumPlanes[0];
    QueryContext.PortalStack[0].AreaFrustum[1] = *InQuery.FrustumPlanes[1];
    QueryContext.PortalStack[0].AreaFrustum[2] = *InQuery.FrustumPlanes[2];
    QueryContext.PortalStack[0].AreaFrustum[3] = *InQuery.FrustumPlanes[3];
    QueryContext.PortalStack[0].AreaFrustum[4] = *InQuery.FrustumPlanes[4]; // far plane
    QueryContext.PortalStack[0].PlanesCount    = 5;
    QueryContext.PortalStack[0].Portal         = NULL;
    QueryContext.PortalStack[0].Scissor.MinX   = x;
    QueryContext.PortalStack[0].Scissor.MinY   = y;
    QueryContext.PortalStack[0].Scissor.MaxX   = -x;
    QueryContext.PortalStack[0].Scissor.MaxY   = -y;

    for (AVisibilityLevel* level : Levels)
    {
        level->ProcessLevelVisibility(QueryContext, QueryResult);
    }
    /*!!!
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

                        QueryResult.pVisPrimitives->Add(primitive);
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
*/

#ifdef DEBUG_TRAVERSING_COUNTERS
    DEBUG("VSD: VisFrame {}\n", Dbg_SkippedByVisFrame);
    DEBUG("VSD: PlaneOfs {}\n", Dbg_SkippedByPlaneOffset);
    DEBUG("VSD: FaceCull {}\n", Dbg_CulledByDotProduct);
    DEBUG("VSD: AABBCull {}\n", Dbg_CulledByPrimitiveBounds);
    DEBUG("VSD: AABBCull (subparts) {}\n", Dbg_CulledSubpartsCount);
    DEBUG("VSD: Clipped {}\n", Dbg_ClippedPortals);
    DEBUG("VSD: PassedPortals {}\n", Dbg_PassedPortals);
    DEBUG("VSD: StackDeep {}\n", Dbg_StackDeep);
    DEBUG("VSD: CullMiss: {}\n", Dbg_CullMiss);
#endif

    //QueryVisiblePrimitivesTime = GEngine->SysMicroseconds() - QueryVisiblePrimitivesTime;

    //DEBUG( "QueryVisiblePrimitivesTime: {} microsec\n", QueryVisiblePrimitivesTime );

    //DEBUG( "Frustum culling time {} microsec. Culled {} from {} primitives. Submits {}\n", Dbg_FrustumCullingTime, Dbg_CulledByPrimitiveBounds, Dbg_TotalPrimitiveBounds, CullSubmits.Size() );
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

void AVisibilityLevel::RaycastSurface(SSurfaceDef* Self)
{
    float d, u, v;
    float boxMin, boxMax;

    if (Self->Flags & SURF_PLANAR)
    {
        // Calculate distance from ray origin to plane
        const float d1 = Math::Dot(pRaycast->RayStart, Self->Face.Normal) + Self->Face.D;
        float       d2;

        if (Self->Flags & SURF_TWOSIDED)
        {
            // Check ray direction
            d2 = Math::Dot(Self->Face.Normal, pRaycast->RayDir);
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
            d2 = Math::Dot(Self->Face.Normal, pRaycast->RayDir);
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
            d2 = Math::Dot( Self->Face.Normal, pRaycast->RayDir );
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

        if (d >= pRaycast->HitDistanceMin)
        {
            // distance is too far
            return;
        }

        ABrushModel const* brushModel = Self->Model;

        SMeshVertex const*  pVertices = brushModel->Vertices.ToPtr() + Self->FirstVertex;
        unsigned int const* pIndices  = brushModel->Indices.ToPtr() + Self->FirstIndex;

        if (pRaycast->bClosest)
        {
            for (int i = 0; i < Self->NumIndices; i += 3)
            {
                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (RayIntersectTriangleFast(pRaycast->RayStart, pRaycast->RayDir, v0, v1, v2, u, v))
                {
                    pRaycast->HitProxyType   = HIT_PROXY_TYPE_SURFACE;
                    pRaycast->HitSurface     = Self;
                    pRaycast->HitLocation    = pRaycast->RayStart + pRaycast->RayDir * d;
                    pRaycast->HitDistanceMin = d;
                    pRaycast->HitUV.X        = u;
                    pRaycast->HitUV.Y        = v;
                    pRaycast->pVertices      = brushModel->Vertices.ToPtr();
                    pRaycast->pLightmapVerts = brushModel->LightmapVerts.ToPtr();
                    pRaycast->LightmapBlock  = Self->LightmapBlock;
                    pRaycast->LightingLevel  = brushModel->ParentLevel.GetObject();
                    pRaycast->Indices[0]     = Self->FirstVertex + triangleIndices[0];
                    pRaycast->Indices[1]     = Self->FirstVertex + triangleIndices[1];
                    pRaycast->Indices[2]     = Self->FirstVertex + triangleIndices[2];
                    pRaycast->Material       = brushModel->SurfaceMaterials[Self->MaterialIndex];
                    pRaycast->NumHits++;

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

                if (RayIntersectTriangleFast(pRaycast->RayStart, pRaycast->RayDir, v0, v1, v2, u, v))
                {

                    STriangleHitResult& hitResult = pRaycastResult->Hits.Add();
                    hitResult.Location            = pRaycast->RayStart + pRaycast->RayDir * d;
                    hitResult.Normal              = Self->Face.Normal;
                    hitResult.Distance            = d;
                    hitResult.UV.X                = u;
                    hitResult.UV.Y                = v;
                    hitResult.Indices[0]          = Self->FirstVertex + triangleIndices[0];
                    hitResult.Indices[1]          = Self->FirstVertex + triangleIndices[1];
                    hitResult.Indices[2]          = Self->FirstVertex + triangleIndices[2];
                    hitResult.Material            = brushModel->SurfaceMaterials[Self->MaterialIndex];

                    SWorldRaycastPrimitive& rcPrimitive = pRaycastResult->Primitives.Add();
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
        if (!BvRayIntersectBox(pRaycast->RayStart, pRaycast->InvRayDir, Self->Bounds, boxMin, boxMax))
        {
            return;
        }

        if (boxMin >= pRaycast->HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            return;
        }

        ABrushModel const* brushModel = Self->Model;

        SMeshVertex const*  pVertices = brushModel->Vertices.ToPtr() + Self->FirstVertex;
        unsigned int const* pIndices  = brushModel->Indices.ToPtr() + Self->FirstIndex;

        if (pRaycast->bClosest)
        {
            for (int i = 0; i < Self->NumIndices; i += 3)
            {
                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (BvRayIntersectTriangle(pRaycast->RayStart, pRaycast->RayDir, v0, v1, v2, d, u, v, cullBackFaces))
                {
                    if (pRaycast->HitDistanceMin > d)
                    {

                        pRaycast->HitProxyType   = HIT_PROXY_TYPE_SURFACE;
                        pRaycast->HitSurface     = Self;
                        pRaycast->HitLocation    = pRaycast->RayStart + pRaycast->RayDir * d;
                        pRaycast->HitDistanceMin = d;
                        pRaycast->HitUV.X        = u;
                        pRaycast->HitUV.Y        = v;
                        pRaycast->pVertices      = brushModel->Vertices.ToPtr();
                        pRaycast->pLightmapVerts = brushModel->LightmapVerts.ToPtr();
                        pRaycast->LightmapBlock  = Self->LightmapBlock;
                        pRaycast->LightingLevel  = brushModel->ParentLevel.GetObject();
                        pRaycast->Indices[0]     = Self->FirstVertex + triangleIndices[0];
                        pRaycast->Indices[1]     = Self->FirstVertex + triangleIndices[1];
                        pRaycast->Indices[2]     = Self->FirstVertex + triangleIndices[2];
                        pRaycast->Material       = brushModel->SurfaceMaterials[Self->MaterialIndex];

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

                if (BvRayIntersectTriangle(pRaycast->RayStart, pRaycast->RayDir, v0, v1, v2, d, u, v, cullBackFaces))
                {
                    if (pRaycast->RayLength > d)
                    {
                        STriangleHitResult& hitResult = pRaycastResult->Hits.Add();
                        hitResult.Location            = pRaycast->RayStart + pRaycast->RayDir * d;
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
                SWorldRaycastPrimitive& rcPrimitive = pRaycastResult->Primitives.Add();
                rcPrimitive.Object                  = nullptr;
                rcPrimitive.FirstHit                = firstHit;
                rcPrimitive.NumHits                 = pRaycastResult->Hits.Size() - firstHit;
                rcPrimitive.ClosestHit              = closestHit;
            }
        }
    }
}

void AVisibilityLevel::RaycastPrimitive(SPrimitiveDef* Self)
{
    // FIXME: What about two sided primitives? Use TwoSided flag directly from material or from primitive?

    if (pRaycast->bClosest)
    {
        STriangleHitResult hit;

        if (Self->RaycastClosestCallback && Self->RaycastClosestCallback(Self, pRaycast->RayStart, pRaycast->HitLocation, hit, &pRaycast->pVertices))
        {
            pRaycast->HitProxyType   = HIT_PROXY_TYPE_PRIMITIVE;
            pRaycast->HitPrimitive   = Self;
            pRaycast->HitLocation    = hit.Location;
            pRaycast->HitNormal      = hit.Normal;
            pRaycast->HitUV          = hit.UV;
            pRaycast->HitDistanceMin = hit.Distance;
            pRaycast->Indices[0]     = hit.Indices[0];
            pRaycast->Indices[1]     = hit.Indices[1];
            pRaycast->Indices[2]     = hit.Indices[2];
            pRaycast->Material       = hit.Material;

            // TODO:
            //pRaycast->pLightmapVerts = Self->Owner->LightmapUVChannel->GetVertices();
            //pRaycast->LightmapBlock = Self->Owner->LightmapBlock;
            //pRaycast->LightingLevel = Self->Owner->ParentLevel.GetObject();

            // Mark primitive visible
            Self->VisPass = VisQueryMarker;
        }
    }
    else
    {
        int firstHit = pRaycastResult->Hits.Size();
        if (Self->RaycastCallback && Self->RaycastCallback(Self, pRaycast->RayStart, pRaycast->RayEnd, pRaycastResult->Hits))
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

            SWorldRaycastPrimitive& rcPrimitive = pRaycastResult->Primitives.Add();

            rcPrimitive.Object     = Self->Owner;
            rcPrimitive.FirstHit   = firstHit;
            rcPrimitive.NumHits    = pRaycastResult->Hits.Size() - firstHit;
            rcPrimitive.ClosestHit = closestHit;

            // Mark primitive visible
            Self->VisPass = VisQueryMarker;
        }
    }
}

void AVisibilityLevel::RaycastArea(SVisArea* InArea)
{
    float boxMin, boxMax;

    if (InArea->VisMark == VisQueryMarker)
    {
        // Area raycast already processed
        //LOG( "Area raycast already processed\n" );
        return;
    }

    // Mark area raycast processed
    InArea->VisMark = VisQueryMarker;

    if (InArea->NumSurfaces > 0)
    {
        ABrushModel* model = Model;

        int const* pSurfaceIndex = &AreaSurfaces[InArea->FirstSurface];

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
            if ((surf->QueryGroup & pRaycast->VisQueryMask) != pRaycast->VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & pRaycast->VisibilityMask) == 0)
            {
                continue;
            }

            RaycastSurface(surf);

#ifdef CLOSE_ENOUGH_EARLY_OUT
            // hit is close enough to stop ray casting?
            if (pRaycast->HitDistanceMin < 0.0001f)
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
        if ((primitive->QueryGroup & pRaycast->VisQueryMask) != pRaycast->VisQueryMask)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & pRaycast->VisibilityMask) == 0)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        if ((primitive->Flags & SURF_PLANAR_TWOSIDED_MASK) == SURF_PLANAR)
        {
            // Perform face culling
            if (primitive->Face.DistanceToPoint(pRaycast->RayStart) < 0.0f)
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
                if (!BvRayIntersectBox(pRaycast->RayStart, pRaycast->InvRayDir, primitive->Box, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            case VSD_PRIMITIVE_SPHERE: {
                // Perform Sphere raycast
                if (!BvRayIntersectSphere(pRaycast->RayStart, pRaycast->RayDir, primitive->Sphere, boxMin, boxMax))
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

        if (boxMin >= pRaycast->HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            continue;
        }

        // Mark primitive raycast processed
        primitive->VisMark = VisQueryMarker;

        RaycastPrimitive(primitive);

#ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if (pRaycast->HitDistanceMin < 0.0001f)
        {
            return;
        }
#endif
    }
}

void AVisibilityLevel::RaycastPrimitiveBounds(SVisArea* InArea)
{
    float boxMin, boxMax;

    if (InArea->VisMark == VisQueryMarker)
    {
        // Area raycast already processed
        //DEBUG( "Area raycast already processed\n" );
        return;
    }

    // Mark area raycast processed
    InArea->VisMark = VisQueryMarker;

    if (InArea->NumSurfaces > 0)
    {
        ABrushModel* model = Model;

        int const* pSurfaceIndex = &AreaSurfaces[InArea->FirstSurface];

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
            if ((surf->QueryGroup & pRaycast->VisQueryMask) != pRaycast->VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & pRaycast->VisibilityMask) == 0)
            {
                continue;
            }

            if (surf->Flags & SURF_PLANAR)
            {
                // FIXME: planar surface has no bounds?
                continue;
            }

            // Perform AABB raycast
            if (!BvRayIntersectBox(pRaycast->RayStart, pRaycast->InvRayDir, surf->Bounds, boxMin, boxMax))
            {
                continue;
            }
            if (boxMin >= pRaycast->HitDistanceMin)
            {
                // Ray intersects the box, but box is too far
                continue;
            }

            // Mark as visible
            surf->VisPass = VisQueryMarker;

            if (pRaycast->bClosest)
            {
                pRaycast->HitProxyType   = HIT_PROXY_TYPE_SURFACE;
                pRaycast->HitSurface     = surf;
                pRaycast->HitDistanceMin = boxMin;
                pRaycast->HitDistanceMax = boxMax;

#ifdef CLOSE_ENOUGH_EARLY_OUT
                // hit is close enough to stop ray casting?
                if (pRaycast->HitDistanceMin < 0.0001f)
                {
                    break;
                }
#endif
            }
            else
            {
                SBoxHitResult& hitResult = pBoundsRaycastResult->Add();

                hitResult.Object      = nullptr;
                hitResult.LocationMin = pRaycast->RayStart + pRaycast->RayDir * boxMin;
                hitResult.LocationMax = pRaycast->RayStart + pRaycast->RayDir * boxMax;
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
        if ((primitive->QueryGroup & pRaycast->VisQueryMask) != pRaycast->VisQueryMask)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & pRaycast->VisibilityMask) == 0)
        {
            // Mark primitive raycast processed
            primitive->VisMark = VisQueryMarker;
            continue;
        }

        switch (primitive->Type)
        {
            case VSD_PRIMITIVE_BOX: {
                // Perform AABB raycast
                if (!BvRayIntersectBox(pRaycast->RayStart, pRaycast->InvRayDir, primitive->Box, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            case VSD_PRIMITIVE_SPHERE: {
                // Perform Sphere raycast
                if (!BvRayIntersectSphere(pRaycast->RayStart, pRaycast->RayDir, primitive->Sphere, boxMin, boxMax))
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

        if (boxMin >= pRaycast->HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            continue;
        }

        // Mark primitive raycast processed
        primitive->VisMark = VisQueryMarker;

        // Mark primitive visible
        primitive->VisPass = VisQueryMarker;

        if (pRaycast->bClosest)
        {
            pRaycast->HitProxyType   = HIT_PROXY_TYPE_PRIMITIVE;
            pRaycast->HitPrimitive   = primitive;
            pRaycast->HitDistanceMin = boxMin;
            pRaycast->HitDistanceMax = boxMax;

#ifdef CLOSE_ENOUGH_EARLY_OUT
            // hit is close enough to stop ray casting?
            if (pRaycast->HitDistanceMin < 0.0001f)
            {
                break;
            }
#endif
        }
        else
        {
            SBoxHitResult& hitResult = pBoundsRaycastResult->Add();

            hitResult.Object      = primitive->Owner;
            hitResult.LocationMin = pRaycast->RayStart + pRaycast->RayDir * boxMin;
            hitResult.LocationMax = pRaycast->RayStart + pRaycast->RayDir * boxMax;
            hitResult.DistanceMin = boxMin;
            hitResult.DistanceMax = boxMax;
        }
    }
}

#if 0
void AVisibilitySystem::LevelRaycast_r( int NodeIndex ) {
    SNodeBase const * node;
    float boxMin, boxMax;

    while ( 1 ) {
        if ( NodeIndex < 0 ) {
            node = &Leafs[-1 - NodeIndex];
        } else {
            node = Nodes.ToPtr() + NodeIndex;
        }

        if ( node->ViewMark != NodeViewMark )
            return;

        if ( !BvRayIntersectBox( pRaycast->RayStart, pRaycast->InvRayDir, node->Bounds, boxMin, boxMax ) ) {
            return;
        }

        if ( boxMin >= pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return;
        }

        if ( NodeIndex < 0 ) {
            // leaf
            break;
        }

        SBinarySpaceNode const * n = static_cast< SBinarySpaceNode const * >( node );

        LevelRaycast_r( n->ChildrenIdx[0] );

#    ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if ( pRaycast->HitDistanceMin < 0.0001f ) {
            return;
        }
#    endif

        NodeIndex = n->ChildrenIdx[1];
    }

    SBinarySpaceLeaf const * pleaf = static_cast< SBinarySpaceLeaf const * >( node );

    RaycastArea( pleaf->Area );
}
#else
bool AVisibilityLevel::LevelRaycast2_r(int NodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd)
{
    if (NodeIndex < 0)
    {

        SBinarySpaceLeaf const* leaf = &Leafs[-1 - NodeIndex];

#    if 0
        // FIXME: Add this additional checks?
        float boxMin, boxMax;
        if ( !BvRayIntersectBox( pRaycast->RayStart, pRaycast->InvRayDir, leaf->Bounds, boxMin, boxMax ) ) {
            return false;
        }
        if ( boxMin >= pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return false;
        }
#    endif

        RaycastArea(leaf->Area);

#    if 0
        if ( pRaycast->RayLength > pRaycast->HitDistanceMin ) {
        //if ( d >= pRaycast->HitDistanceMin ) {
            // stop raycasting
            return true;
        }
#    endif

        // continue raycasting
        return false;
    }

    SBinarySpaceNode const* node = Nodes.ToPtr() + NodeIndex;

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
void AVisibilitySystem::LevelRaycastBounds_r( int NodeIndex ) {
    SNodeBase const * node;
    float boxMin, boxMax;

    while ( 1 ) {
        if ( NodeIndex < 0 ) {
            node = &Leafs[-1 - NodeIndex];
        } else {
            node = Nodes.ToPtr() + NodeIndex;
        }

        if ( node->ViewMark != NodeViewMark )
            return;

        if ( !BvRayIntersectBox( pRaycast->RayStart, pRaycast->InvRayDir, node->Bounds, boxMin, boxMax ) ) {
            return;
        }

        if ( boxMin >= pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return;
        }

        if ( NodeIndex < 0 ) {
            // leaf
            break;
        }

        SBinarySpaceNode const * n = static_cast< SBinarySpaceNode const * >( node );

        LevelRaycastBounds_r( n->ChildrenIdx[0] );

#    ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if ( pRaycast->HitDistanceMin < 0.0001f ) {
            return;
        }
#    endif

        NodeIndex = n->ChildrenIdx[1];
    }

    SBinarySpaceLeaf const * pleaf = static_cast< SBinarySpaceLeaf const * >( node );

    RaycastPrimitiveBounds( pleaf->Area );
}
#else
bool AVisibilityLevel::LevelRaycastBounds2_r(int NodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd)
{

    if (NodeIndex < 0)
    {

        SBinarySpaceLeaf const* leaf = &Leafs[-1 - NodeIndex];

#    if 0
        float boxMin, boxMax;
        if ( !BvRayIntersectBox( InRayStart, pRaycast->InvRayDir, leaf->Bounds, boxMin, boxMax ) ) {
            return false;
        }

        if ( boxMin >= pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return false;
        }
#    endif

        RaycastPrimitiveBounds(leaf->Area);

        if (pRaycast->RayLength > pRaycast->HitDistanceMin)
        {
            //if ( d >= pRaycast->HitDistanceMin ) {
            // stop raycasting
            return true;
        }

        // continue raycasting
        return false;
    }

    SBinarySpaceNode const* node = Nodes.ToPtr() + NodeIndex;

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

void AVisibilityLevel::LevelRaycastPortals_r(SVisArea* InArea)
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
        const float d1 = portal->Plane.DistanceToPoint(pRaycast->RayStart);
        if (d1 <= 0.0f)
        {
            // ray is behind
            continue;
        }

        // Check ray direction
        const float d2 = Math::Dot(portal->Plane.Normal, pRaycast->RayDir);
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
        if (!BvRayIntersectPlane(pRaycast->RayStart, pRaycast->RayDir, portal->Plane, dist))
        {
            continue;
        }
        if (dist <= 0.0f)
        {
            continue;
        }
#endif

        if (dist >= pRaycast->HitDistanceMin)
        {
            // Ray intersects the portal plane, but portal is too far
            continue;
        }

        const Float3 p = pRaycast->RayStart + pRaycast->RayDir * dist;

        if (!BvPointInConvexHullCCW(p, portal->Plane.Normal, portal->Hull->Points, portal->Hull->NumPoints))
        {
            continue;
        }

        LevelRaycastPortals_r(portal->ToArea);
    }
}

void AVisibilityLevel::LevelRaycastBoundsPortals_r(SVisArea* InArea)
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
        const float d1 = portal->Plane.DistanceToPoint(pRaycast->RayStart);
        if (d1 <= 0.0f)
        {
            // ray is behind
            continue;
        }

        // Check ray direction
        const float d2 = Math::Dot(portal->Plane.Normal, pRaycast->RayDir);
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
        if (!BvRayIntersectPlane(pRaycast->RayStart, pRaycast->RayDir, portal->Plane, dist))
        {
            continue;
        }
        if (dist <= 0.0f)
        {
            continue;
        }
#endif

        if (dist >= pRaycast->HitDistanceMin)
        {
            // Ray intersects the portal plane, but portal is too far
            continue;
        }

        const Float3 p = pRaycast->RayStart + pRaycast->RayDir * dist;

        if (!BvPointInConvexHullCCW(p, portal->Plane.Normal, portal->Hull->Points, portal->Hull->NumPoints))
        {
            continue;
        }

        LevelRaycastBoundsPortals_r(portal->ToArea);
    }
}

void AVisibilityLevel::ProcessLevelRaycast(SRaycast& Raycast, SWorldRaycastResult& Result)
{
    pRaycast = &Raycast;
    pRaycastResult = &Result;

    // TODO: check level bounds (ray/aabb overlap)?

    if (VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( pRaycast->RayStart );

        NodeViewMark = MarkLeafs( leaf );

        LevelRaycast_r( 0 );
#else
        LevelRaycast2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastPortals_r(area);
    }
}

void AVisibilityLevel::ProcessLevelRaycastClosest(SRaycast& Raycast)
{
    pRaycast = &Raycast;
    pRaycastResult = nullptr;

    // TODO: check level bounds (ray/aabb overlap)?

    if (VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( pRaycast->RayStart );

        NodeViewMark = MarkLeafs( leaf );

        LevelRaycast_r( 0 );
#else
        LevelRaycast2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastPortals_r(area);
    }
}

void AVisibilityLevel::ProcessLevelRaycastBounds(SRaycast& Raycast, TPodVector<SBoxHitResult>& Result)
{
    pRaycast = &Raycast;
    pBoundsRaycastResult = &Result;

    // TODO: check level bounds (ray/aabb overlap)?

    if (VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( Raycast.RayStart );

        NodeViewMark = MarkLeafs( leaf );

        LevelRaycastBounds_r( 0 );
#else
        LevelRaycastBounds2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastBoundsPortals_r(area);
    }
}

void AVisibilityLevel::ProcessLevelRaycastClosestBounds(SRaycast& Raycast)
{
    pRaycast = &Raycast;
    pBoundsRaycastResult = nullptr;

    // TODO: check level bounds (ray/aabb overlap)?

    if (VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( Raycast.RayStart );

        NodeViewMark = MarkLeafs( leaf );

        LevelRaycastBounds_r( 0 );
#else
        LevelRaycastBounds2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        SVisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastBoundsPortals_r(area);
    }
}

bool AVisibilityLevel::RaycastTriangles(TPodVector<AVisibilityLevel*> const& Levels, SWorldRaycastResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    SRaycast Raycast;

    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask   = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

    Result.Clear();

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

    for (AVisibilityLevel* level : Levels)
    {
        level->ProcessLevelRaycast(Raycast, Result);
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

bool AVisibilityLevel::RaycastClosest(TPodVector<AVisibilityLevel*> const& Levels, SWorldRaycastClosestResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    SRaycast Raycast;

    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask   = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

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

    for (AVisibilityLevel* level : Levels)
    {
        level->ProcessLevelRaycastClosest(Raycast);

#ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if (Raycast.HitDistanceMin < 0.0001f)
        {
            break;
        }
#endif
    }

    //DEBUG( "NumHits %d\n", Raycast.NumHits );

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

bool AVisibilityLevel::RaycastBounds(TPodVector<AVisibilityLevel*> const& Levels, TPodVector<SBoxHitResult>& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    SRaycast Raycast;

    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask   = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

    Result.Clear();

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

    for (AVisibilityLevel* level : Levels)
    {
        level->ProcessLevelRaycastBounds(Raycast, Result);
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

bool AVisibilityLevel::RaycastClosestBounds(TPodVector<AVisibilityLevel*> const& Levels, SBoxHitResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, SWorldRaycastFilter const* InFilter)
{
    SRaycast Raycast;

    ++VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask   = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

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

    for (AVisibilityLevel* level : Levels)
    {
        level->ProcessLevelRaycastClosestBounds(Raycast);

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


AVisibilitySystem::APrimitivePool     AVisibilitySystem::PrimitivePool;
AVisibilitySystem::APrimitiveLinkPool AVisibilitySystem::PrimitiveLinkPool;

SPrimitiveDef* AVisibilitySystem::AllocatePrimitive()
{
    SPrimitiveDef* primitive = new (PrimitivePool.Allocate()) SPrimitiveDef;
    return primitive;
}

void AVisibilitySystem::DeallocatePrimitive(SPrimitiveDef* Primitive)
{
    Primitive->~SPrimitiveDef();
    PrimitivePool.Deallocate(Primitive);
}

AVisibilitySystem::AVisibilitySystem()
{}

AVisibilitySystem::~AVisibilitySystem()
{
    HK_ASSERT(Levels.IsEmpty());
}

void AVisibilitySystem::RegisterLevel(AVisibilityLevel* Level)
{
    if (Levels.Contains(Level))
        return;

    Levels.Add(Level);
    Level->AddRef();

    MarkPrimitives();
}

void AVisibilitySystem::UnregisterLevel(AVisibilityLevel* Level)
{
    auto i = Levels.IndexOf(Level);
    if (i == Core::NPOS)
        return;

    Levels[i]->RemoveRef();
    Levels.Remove(i);

    MarkPrimitives();
    UpdatePrimitiveLinks();
}

void AVisibilitySystem::AddPrimitive(SPrimitiveDef* Primitive)
{
    if (INTRUSIVE_EXISTS(Primitive, Next, Prev, PrimitiveList, PrimitiveListTail))
    {
        // Already added
        return;
    }

    INTRUSIVE_ADD(Primitive, Next, Prev, PrimitiveList, PrimitiveListTail);

    AVisibilityLevel::AddPrimitiveToLevelAreas(Levels, Primitive);
}

void AVisibilitySystem::RemovePrimitive(SPrimitiveDef* Primitive)
{
    if (!INTRUSIVE_EXISTS(Primitive, Next, Prev, PrimitiveList, PrimitiveListTail))
    {
        // Not added at all
        return;
    }

    INTRUSIVE_REMOVE(Primitive, Next, Prev, PrimitiveList, PrimitiveListTail);
    INTRUSIVE_REMOVE(Primitive, NextUpd, PrevUpd, PrimitiveDirtyList, PrimitiveDirtyListTail);

    UnlinkPrimitive(Primitive);
}

void AVisibilitySystem::RemovePrimitives()
{
    UnmarkPrimitives();

    SPrimitiveDef* next;
    for (SPrimitiveDef* primitive = PrimitiveList; primitive; primitive = next)
    {
        UnlinkPrimitive(primitive);

        next            = primitive->Next;
        primitive->Prev = primitive->Next = nullptr;
    }

    PrimitiveList = PrimitiveListTail = nullptr;
}

void AVisibilitySystem::MarkPrimitive(SPrimitiveDef* Primitive)
{
    if (!INTRUSIVE_EXISTS(Primitive, Next, Prev, PrimitiveList, PrimitiveListTail))
    {
        // Not added at all
        return;
    }

    INTRUSIVE_ADD_UNIQUE(Primitive, NextUpd, PrevUpd, PrimitiveDirtyList, PrimitiveDirtyListTail);
}

void AVisibilitySystem::MarkPrimitives()
{
    for (SPrimitiveDef* primitive = PrimitiveList; primitive; primitive = primitive->Next)
    {
        MarkPrimitive(primitive);
    }
}

void AVisibilitySystem::UnmarkPrimitives()
{
    SPrimitiveDef* next;
    for (SPrimitiveDef* primitive = PrimitiveDirtyList; primitive; primitive = next)
    {
        next               = primitive->NextUpd;
        primitive->PrevUpd = primitive->NextUpd = nullptr;
    }
    PrimitiveDirtyList = PrimitiveDirtyListTail = nullptr;
}

void AVisibilitySystem::UpdatePrimitiveLinks()
{
    SPrimitiveDef* next;

    // First Pass: remove primitives from the areas
    for (SPrimitiveDef* primitive = PrimitiveDirtyList; primitive; primitive = primitive->NextUpd)
    {
        UnlinkPrimitive(primitive);
    }

    // Second Pass: add primitives to the areas
    for (SPrimitiveDef* primitive = PrimitiveDirtyList; primitive; primitive = next)
    {
        AVisibilityLevel::AddPrimitiveToLevelAreas(Levels, primitive);

        next               = primitive->NextUpd;
        primitive->PrevUpd = primitive->NextUpd = nullptr;
    }

    PrimitiveDirtyList = PrimitiveDirtyListTail = nullptr;
}

void AVisibilitySystem::UnlinkPrimitive(SPrimitiveDef* Primitive)
{
    SPrimitiveLink* link = Primitive->Links;

    while (link)
    {
        HK_ASSERT(link->Area);

        SPrimitiveLink** prev = &link->Area->Links;
        while (1)
        {
            SPrimitiveLink* walk = *prev;

            if (!walk)
            {
                break;
            }

            if (walk == link)
            {
                // remove this link
                *prev = link->NextInArea;
                break;
            }

            prev = &walk->NextInArea;
        }

        SPrimitiveLink* free = link;
        link                 = link->Next;

        PrimitiveLinkPool.Deallocate(free);
    }

    Primitive->Links = nullptr;
}

void AVisibilitySystem::DrawDebug(ADebugRenderer* Renderer)
{
    for (AVisibilityLevel* level : Levels)
    {
        level->DrawDebug(Renderer);
    }
}

void AVisibilitySystem::QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& Areas) const
{
    for (AVisibilityLevel* level : Levels)
    {
        level->QueryOverplapAreas(Bounds, Areas);
    }
}

void AVisibilitySystem::QueryOverplapAreas(BvSphere const& Bounds, TPodVector<SVisArea*>& Areas) const
{
    for (AVisibilityLevel* level : Levels)
    {
        level->QueryOverplapAreas(Bounds, Areas);
    }
}

void AVisibilitySystem::QueryVisiblePrimitives(TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& Query) const
{
    AVisibilityLevel::QueryVisiblePrimitives(Levels, VisPrimitives, VisSurfs, VisPass, Query);
}

bool AVisibilitySystem::RaycastTriangles(SWorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return AVisibilityLevel::RaycastTriangles(Levels, Result, RayStart, RayEnd, Filter);
}

bool AVisibilitySystem::RaycastClosest(SWorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return AVisibilityLevel::RaycastClosest(Levels, Result, RayStart, RayEnd, Filter);
}

bool AVisibilitySystem::RaycastBounds(TPodVector<SBoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return AVisibilityLevel::RaycastBounds(Levels, Result, RayStart, RayEnd, Filter);
}

bool AVisibilitySystem::RaycastClosestBounds(SBoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return AVisibilityLevel::RaycastClosestBounds(Levels, Result, RayStart, RayEnd, Filter);
}

HK_CLASS_META(ABrushModel)
