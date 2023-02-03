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

#include "VisibilitySystem.h"
#include "Level.h"
#include "Texture.h"
#include "Engine.h"

#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/Geometry/ConvexHull.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawLevelAreaBounds("com_DrawLevelAreaBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawLevelIndoorBounds("com_DrawLevelIndoorBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawLevelPortals("com_DrawLevelPortals"s, "0"s, CVAR_CHEAT);

//ConsoleVar vsd_FrustumCullingType("vsd_FrustumCullingType"s, "0"s, 0, "0 - combined, 1 - separate, 2 - simple"s);

#define MAX_HULL_POINTS 128
struct PortalHull
{
    int NumPoints;
    Float3 Points[MAX_HULL_POINTS];
};

int VisibilityLevel::m_VisQueryMarker = 0;

static const WorldRaycastFilter DefaultRaycastFilter;

struct VisibilityQueryContext
{
    enum
    {
        MAX_PORTAL_STACK = 128, //64
    };


    PortalStack PStack[MAX_PORTAL_STACK];
    int PortalStackPos;

    Float3 ViewPosition;
    Float3 ViewRightVec;
    Float3 ViewUpVec;
    PlaneF ViewPlane;
    float ViewZNear;
    Float3 ViewCenter;

    VSD_QUERY_MASK VisQueryMask = VSD_QUERY_MASK(0);
    VISIBILITY_GROUP VisibilityMask = VISIBILITY_GROUP(0);
};

struct VisibilityQueryResult
{
    TPodVector<PrimitiveDef*>* pVisPrimitives;
    TPodVector<SurfaceDef*>* pVisSurfs;
};

VisibilityLevel::VisibilityLevel(VisibilitySystemCreateInfo const& CreateInfo)
{
    Float3 extents(CONVEX_HULL_MAX_BOUNDS * 2);

    Platform::ZeroMem(&m_OutdoorArea, sizeof(m_OutdoorArea));

    m_OutdoorArea.Bounds.Mins = -extents * 0.5f;
    m_OutdoorArea.Bounds.Maxs = extents * 0.5f;

    m_PersistentLevel = CreateInfo.PersistentLevel;

    m_pOutdoorArea = m_PersistentLevel ? &m_PersistentLevel->m_OutdoorArea : &m_OutdoorArea;

    m_IndoorBounds.Clear();

    m_Areas.Resize(CreateInfo.NumAreas);
    m_Areas.ZeroMem();
    for (int i = 0; i < CreateInfo.NumAreas; i++)
    {
        VisArea& area = m_Areas[i];
        VisibilityAreaDef& src = CreateInfo.Areas[i];

        area.Bounds = src.Bounds;
        area.FirstSurface = src.FirstSurface;
        area.NumSurfaces = src.NumSurfaces;

        m_IndoorBounds.AddAABB(area.Bounds);
    }

    m_SplitPlanes.Resize(CreateInfo.NumPlanes);
    Platform::Memcpy(m_SplitPlanes.ToPtr(), CreateInfo.Planes, CreateInfo.NumPlanes * sizeof(m_SplitPlanes[0]));

    m_Nodes.Resize(CreateInfo.NumNodes);
    for (int i = 0; i < CreateInfo.NumNodes; i++)
    {
        BinarySpaceNode& dst = m_Nodes[i];
        BinarySpaceNodeDef const& src = CreateInfo.Nodes[i];

        dst.Parent = src.Parent != -1 ? (m_Nodes.ToPtr() + src.Parent) : nullptr;
        dst.ViewMark = 0;
        dst.Bounds = src.Bounds;
        dst.Plane = m_SplitPlanes.ToPtr() + src.PlaneIndex;
        dst.ChildrenIdx[0] = src.ChildrenIdx[0];
        dst.ChildrenIdx[1] = src.ChildrenIdx[1];
    }

    m_Leafs.Resize(CreateInfo.NumLeafs);
    for (int i = 0; i < CreateInfo.NumLeafs; i++)
    {
        BinarySpaceLeaf& dst = m_Leafs[i];
        BinarySpaceLeafDef const& src = CreateInfo.Leafs[i];

        dst.Parent = src.Parent != -1 ? (m_Nodes.ToPtr() + src.Parent) : nullptr;
        dst.ViewMark = 0;
        dst.Bounds = src.Bounds;
        dst.PVSCluster = src.PVSCluster;
        dst.Visdata = nullptr;
        dst.AudioArea = src.AudioArea;
        dst.Area = m_Areas.ToPtr() + src.AreaNum;
    }

    if (CreateInfo.PortalsCount > 0)
    {
        m_VisibilityMethod = LEVEL_VISIBILITY_PORTAL;

        CreatePortals(CreateInfo.Portals, CreateInfo.PortalsCount, CreateInfo.HullVertices);
    }
    else if (CreateInfo.PVS)
    {
        m_VisibilityMethod = LEVEL_VISIBILITY_PVS;

        m_Visdata = (byte*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(CreateInfo.PVS->VisdataSize);
        Platform::Memcpy(m_Visdata, CreateInfo.PVS->Visdata, CreateInfo.PVS->VisdataSize);

        m_bCompressedVisData = CreateInfo.PVS->bCompressedVisData;
        m_PVSClustersCount = CreateInfo.PVS->ClustersCount;

        for (int i = 0; i < CreateInfo.NumLeafs; i++)
        {
            BinarySpaceLeaf& dst = m_Leafs[i];
            BinarySpaceLeafDef const& src = CreateInfo.Leafs[i];

            if (src.VisdataOffset < CreateInfo.PVS->VisdataSize)
            {
                dst.Visdata = m_Visdata + src.VisdataOffset;
            }
            else
            {
                dst.Visdata = nullptr;
            }
        }
    }

    if (m_bCompressedVisData && m_Visdata && m_PVSClustersCount > 0)
    {
        // Allocate decompressed vis data
        m_DecompressedVisData = (byte*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc((m_PVSClustersCount + 7) >> 3);
    }

    m_AreaSurfaces.Resize(CreateInfo.NumAreaSurfaces);
    Platform::Memcpy(m_AreaSurfaces.ToPtr(), CreateInfo.AreaSurfaces, sizeof(m_AreaSurfaces[0]) * CreateInfo.NumAreaSurfaces);

    m_Model = CreateInfo.Model;
}

VisibilityLevel::~VisibilityLevel()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_Visdata);
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_DecompressedVisData);
}

void VisibilityLevel::CreatePortals(PortalDef const* InPortals, int InPortalsCount, Float3 const* InHullVertices)
{
    PlaneF hullPlane;
    PortalLink* portalLink;
    int portalLinkNum;

    m_Portals.ResizeInvalidate(InPortalsCount);
    m_AreaLinks.ResizeInvalidate(m_Portals.Size() << 1);
    m_PortalHulls.ResizeInvalidate(InPortalsCount * 2);

    portalLinkNum = 0;

    for (int i = 0; i < InPortalsCount; i++)
    {
        PortalDef const* def = InPortals + i;
        VisPortal& portal = m_Portals[i];

        VisArea* a1 = def->Areas[0] >= 0 ? &m_Areas[def->Areas[0]] : m_pOutdoorArea;
        VisArea* a2 = def->Areas[1] >= 0 ? &m_Areas[def->Areas[1]] : m_pOutdoorArea;
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
        ConvexHull& hull = m_PortalHulls[i * 2];
        ConvexHull& hullReversed = m_PortalHulls[i * 2 + 1];

        hull.FromPoints(InHullVertices + def->FirstVert, def->NumVerts);
        hullReversed = hull.Reversed();

        hullPlane = hull.CalcPlane();

        portalLink = &m_AreaLinks[portalLinkNum++];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a2;
        if (id & 1)
        {
            portalLink->Hull = &hull;
            portalLink->Plane = hullPlane;
        }
        else
        {
            portalLink->Hull = &hullReversed;
            portalLink->Plane = -hullPlane;
        }
        portalLink->Next = a1->PortalList;
        portalLink->Portal = &portal;
        a1->PortalList = portalLink;

        id = (id + 1) & 1;

        portalLink = &m_AreaLinks[portalLinkNum++];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a1;
        if (id & 1)
        {
            portalLink->Hull = &hull;
            portalLink->Plane = hullPlane;
        }
        else
        {
            portalLink->Hull = &hullReversed;
            portalLink->Plane = -hullPlane;
        }
        portalLink->Next = a2->PortalList;
        portalLink->Portal = &portal;
        a2->PortalList = portalLink;

        portal.bBlocked = false;
    }
}

int VisibilityLevel::FindLeaf(Float3 const& InPosition)
{
    BinarySpaceNode* node;
    float d;
    int nodeIndex;

    if (m_Nodes.IsEmpty())
    {
        return -1;
    }

    node = m_Nodes.ToPtr();
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

        node = m_Nodes.ToPtr() + nodeIndex;
    }
    return -1;
}

VisArea* VisibilityLevel::FindArea(Float3 const& InPosition)
{
    if (!m_Nodes.IsEmpty())
    {
        int leaf = FindLeaf(InPosition);
        if (leaf < 0)
        {
            // solid
            return m_pOutdoorArea;
        }
        return m_Leafs[leaf].Area;
    }

    // Bruteforce TODO: remove this!
    for (int i = 0; i < m_Areas.Size(); i++)
    {
        if (InPosition.X >= m_Areas[i].Bounds.Mins.X && InPosition.Y >= m_Areas[i].Bounds.Mins.Y && InPosition.Z >= m_Areas[i].Bounds.Mins.Z && InPosition.X < m_Areas[i].Bounds.Maxs.X && InPosition.Y < m_Areas[i].Bounds.Maxs.Y && InPosition.Z < m_Areas[i].Bounds.Maxs.Z)
        {
            return &m_Areas[i];
        }
    }

    return m_pOutdoorArea;
}

byte const* VisibilityLevel::DecompressVisdata(byte const* InCompressedData)
{
    int count;

    int row = (m_PVSClustersCount + 7) >> 3;
    byte* pDecompressed = m_DecompressedVisData;

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
        if (pDecompressed - m_DecompressedVisData + count > row)
        {
            count = row - (pDecompressed - m_DecompressedVisData);
        }

        // Move to the next sequence
        InCompressedData += 2;

        while (count--)
        {
            *pDecompressed++ = 0;
        }
    } while (pDecompressed - m_DecompressedVisData < row);

    return m_DecompressedVisData;
}

byte const* VisibilityLevel::LeafPVS(BinarySpaceLeaf const* InLeaf)
{
    if (m_bCompressedVisData)
    {
        return InLeaf->Visdata ? DecompressVisdata(InLeaf->Visdata) : nullptr;
    }
    else
    {
        return InLeaf->Visdata;
    }
}

int VisibilityLevel::MarkLeafs(int InViewLeaf)
{
    if (m_VisibilityMethod != LEVEL_VISIBILITY_PVS)
    {
        LOG("Level::MarkLeafs: expect LEVEL_VISIBILITY_PVS\n");
        return m_ViewMark;
    }

    if (InViewLeaf < 0)
    {
        return m_ViewMark;
    }

    BinarySpaceLeaf* pLeaf = &m_Leafs[InViewLeaf];

    if (m_ViewCluster == pLeaf->PVSCluster)
    {
        return m_ViewMark;
    }

    m_ViewMark++;
    m_ViewCluster = pLeaf->PVSCluster;

    byte const* pVisibility = LeafPVS(pLeaf);
    if (pVisibility)
    {
        int cluster;
        for (BinarySpaceLeaf& leaf : m_Leafs)
        {

            cluster = leaf.PVSCluster;

            if (cluster < 0 || cluster >= m_PVSClustersCount || !(pVisibility[cluster >> 3] & (1 << (cluster & 7))))
            {
                continue;
            }

            // TODO: check doors here

            NodeBase* parent = &leaf;
            do {
                if (parent->ViewMark == m_ViewMark)
                {
                    break;
                }
                parent->ViewMark = m_ViewMark;
                parent = parent->Parent;
            } while (parent);
        }
    }
    else
    {
        // Mark all
        for (BinarySpaceLeaf& leaf : m_Leafs)
        {
            NodeBase* parent = &leaf;
            do {
                if (parent->ViewMark == m_ViewMark)
                {
                    break;
                }
                parent->ViewMark = m_ViewMark;
                parent = parent->Parent;
            } while (parent);
        }
    }

    return m_ViewMark;
}

void VisibilityLevel::QueryOverplapAreas_r(int NodeIndex, BvAxisAlignedBox const& Bounds, TPodVector<VisArea*>& OverlappedAreas)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            VisArea* area = m_Leafs[-1 - NodeIndex].Area;
            OverlappedAreas.AddUnique(area);
            return;
        }

        BinarySpaceNode* node = &m_Nodes[NodeIndex];

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

void VisibilityLevel::QueryOverplapAreas_r(int NodeIndex, BvSphere const& Bounds, TPodVector<VisArea*>& OverlappedAreas)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            VisArea* area = m_Leafs[-1 - NodeIndex].Area;
            OverlappedAreas.AddUnique(area);
            return;
        }

        BinarySpaceNode* node = &m_Nodes[NodeIndex];

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

void VisibilityLevel::QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<VisArea*>& OverlappedAreas)
{
    if (m_Nodes.IsEmpty())
    {
        return;
    }

    QueryOverplapAreas_r(0, Bounds, OverlappedAreas);
}

void VisibilityLevel::QueryOverplapAreas(BvSphere const& Bounds, TPodVector<VisArea*>& OverlappedAreas)
{
    if (m_Nodes.IsEmpty())
    {
        return;
    }

    QueryOverplapAreas_r(0, Bounds, OverlappedAreas);
}

static PrimitiveLink** LastLink;

static HK_FORCEINLINE bool IsPrimitiveInArea(PrimitiveDef const* Primitive, VisArea const* InArea)
{
    for (PrimitiveLink const* link = Primitive->Links; link; link = link->Next)
    {
        if (link->Area == InArea)
        {
            return true;
        }
    }
    return false;
}

void VisibilityLevel::AddPrimitiveToArea(VisArea* Area, PrimitiveDef* Primitive)
{
    if (IsPrimitiveInArea(Primitive, Area))
    {
        return;
    }

    PrimitiveLink* link = VisibilitySystem::PrimitiveLinkPool.Allocate();
    if (!link)
    {
        return;
    }

    link->Primitive = Primitive;

    // Create the primitive link
    *LastLink = link;
    LastLink = &link->Next;
    link->Next = nullptr;

    // Create the area links
    link->Area = Area;
    link->NextInArea = Area->Links;
    Area->Links = link;
}

void VisibilityLevel::AddBoxRecursive(int NodeIndex, PrimitiveDef* Primitive)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            AddPrimitiveToArea(m_Leafs[-1 - NodeIndex].Area, Primitive);
            return;
        }

        BinarySpaceNode* node = &m_Nodes[NodeIndex];

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

void VisibilityLevel::AddSphereRecursive(int NodeIndex, PrimitiveDef* Primitive)
{
    do {
        if (NodeIndex < 0)
        {
            // leaf
            AddPrimitiveToArea(m_Leafs[-1 - NodeIndex].Area, Primitive);
            return;
        }

        BinarySpaceNode* node = &m_Nodes[NodeIndex];

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

void VisibilityLevel::AddPrimitiveToLevelAreas(TPodVector<VisibilityLevel*> const& m_Levels, PrimitiveDef* Primitive)
{
    bool bInsideArea{};

    if (m_Levels.IsEmpty())
        return;

    LastLink = &Primitive->Links;

    if (Primitive->bIsOutdoor)
    {
        // add to outdoor
        m_Levels[0]->AddPrimitiveToArea(m_Levels[0]->m_pOutdoorArea, Primitive);
        return;
    }

    // TODO: Check overlap with portal polygons between indoor and outdoor areas

    //LastLink = &Primitive->Links;

    for (VisibilityLevel* level : m_Levels)
    {
        bool bHaveBinaryTree = level->m_Nodes.Size() > 0;

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

            int numAreas = level->m_Areas.Size();

            switch (Primitive->Type)
            {
                case VSD_PRIMITIVE_BOX: {
                    if (BvBoxOverlapBox(level->m_IndoorBounds, Primitive->Box))
                    {
                        for (int i = 0; i < numAreas; i++)
                        {
                            VisArea* area = &level->m_Areas[i];

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
                    if (BvBoxOverlapSphere(level->m_IndoorBounds, Primitive->Sphere))
                    {
                        for (int i = 0; i < numAreas; i++)
                        {
                            VisArea* area = &level->m_Areas[i];

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
        m_Levels[0]->AddPrimitiveToArea(m_Levels[0]->m_pOutdoorArea, Primitive);
    }
}

void VisibilityLevel::DrawDebug(DebugRenderer* InRenderer)
{
    if (com_DrawLevelAreaBounds)
    {
        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(0, 1, 0, 0.5f));
        for (VisArea& area : m_Areas)
        {
            InRenderer->DrawAABB(area.Bounds);
        }
    }

    if (com_DrawLevelPortals)
    {
        //        InRenderer->SetDepthTest( false );
        //        InRenderer->SetColor(1,0,0,1);
        //        for ( ALevelPortal & portal : m_Portals ) {
        //            InRenderer->DrawLine( portal.Hull->Points, portal.Hull->NumPoints, true );
        //        }

        InRenderer->SetDepthTest(false);
        //InRenderer->SetColor( Color4( 0,0,1,0.4f ) );

        /*if ( LastVisitedArea >= 0 && LastVisitedArea < m_Areas.Size() ) {
            VSDArea * area = &m_Areas[ LastVisitedArea ];
            VSDPortalLink * portals = area->PortalList;

        for ( VSDPortalLink * p = portals; p; p = p->Next ) {
            InRenderer->DrawConvexPoly( p->Hull->Points, p->Hull->NumPoints, true );
        }
    } else */
        {

#if 0
            for ( VisPortal & portal : m_Portals ) {

                if ( portal.VisMark == InRenderer->GetVisPass() ) {
                    InRenderer->SetColor( Color4( 1,0,0,0.4f ) );
                } else {
                    InRenderer->SetColor( Color4( 0,1,0,0.4f ) );
                }
                InRenderer->DrawConvexPoly( portal.Hull->Points, portal.Hull->NumPoints, true );
            }
#else
            if (!m_PersistentLevel)
            {
                PortalLink* portals = m_OutdoorArea.PortalList;

                for (PortalLink* p = portals; p; p = p->Next)
                {

                    if (p->Portal->VisMark == InRenderer->GetVisPass())
                    {
                        InRenderer->SetColor(Color4(1, 0, 0, 0.4f));
                    }
                    else
                    {
                        InRenderer->SetColor(Color4(0, 1, 0, 0.4f));
                    }

                    InRenderer->DrawConvexPoly(p->Hull->GetVector(), false);
                }
            }

            for (VisArea& area : m_Areas)
            {
                PortalLink* portals = area.PortalList;

                for (PortalLink* p = portals; p; p = p->Next)
                {

                    if (p->Portal->VisMark == InRenderer->GetVisPass())
                    {
                        InRenderer->SetColor(Color4(1, 0, 0, 0.4f));
                    }
                    else
                    {
                        InRenderer->SetColor(Color4(0, 1, 0, 0.4f));
                    }

                    InRenderer->DrawConvexPoly(p->Hull->GetVector(), false);
                }
            }
#endif
        }
    }

    if (com_DrawLevelIndoorBounds)
    {
        InRenderer->SetDepthTest(false);
        InRenderer->DrawAABB(m_IndoorBounds);
    }

#ifdef DEBUG_PORTAL_SCISSORS
    InRenderer->SetDepthTest(false);
    InRenderer->SetColor(Color4(0, 1, 0));

    Float3 Corners[4];

    for (PortalScissor const& scissor : DebugScissors)
    {
        const Float3 Center = ViewPosition + ViewPlane.Normal * ViewZNear;
        const Float3 RightMin = ViewRightVec * scissor.MinX + Center;
        const Float3 RightMax = ViewRightVec * scissor.MaxX + Center;
        const Float3 UpMin = ViewUpVec * scissor.MinY;
        const Float3 UpMax = ViewUpVec * scissor.MaxY;
        Corners[0] = RightMin + UpMin;
        Corners[1] = RightMax + UpMin;
        Corners[2] = RightMax + UpMax;
        Corners[3] = RightMin + UpMax;

        InRenderer->DrawLine(Corners, 4, true);
    }
#endif
}

void VisibilityLevel::ProcessLevelVisibility(VisibilityQueryContext& QueryContext, VisibilityQueryResult& QueryResult)
{
    m_pQueryContext = &QueryContext;
    m_pQueryResult = &QueryResult;

    m_ViewFrustum = QueryContext.PStack[0].AreaFrustum;
    m_ViewFrustumPlanes = QueryContext.PStack[0].PlanesCount; // Can be 4 or 5

    int cullBits = 0;

    for (int i = 0; i < m_ViewFrustumPlanes; i++)
    {
        m_CachedSignBits[i] = m_ViewFrustum[i].SignBits();

        cullBits |= 1 << i;
    }

    if (m_VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has PVS

        int leaf = FindLeaf(QueryContext.ViewPosition);

        m_NodeViewMark = MarkLeafs(leaf);

        //DEBUG( "m_NodeViewMark {}\n", m_NodeViewMark );

        //HK_ASSERT( m_NodeViewMark != 0 );

        LevelTraverse_r(0, cullBits);
    }
    else if (m_VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        VisArea* area = FindArea(QueryContext.ViewPosition);

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

bool VisibilityLevel::CullNode(PlaneF const InFrustum[PortalStack::MAX_CULL_PLANES], BvAxisAlignedBox const& Bounds, int& InCullBits)
{
    Float3 p;

    float const* pBounds = Bounds.ToPtr();
    int const* pIndices;
    if (InCullBits & 1)
    {
        pIndices = CullIndices[m_CachedSignBits[0]];

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
        pIndices = CullIndices[m_CachedSignBits[1]];

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
        pIndices = CullIndices[m_CachedSignBits[2]];

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
        pIndices = CullIndices[m_CachedSignBits[3]];

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
        pIndices = CullIndices[m_CachedSignBits[4]];

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

void VisibilityLevel::LevelTraverse_r(int NodeIndex, int InCullBits)
{
    NodeBase const* node;

    while (1)
    {
        if (NodeIndex < 0)
        {
            node = &m_Leafs[-1 - NodeIndex];
        }
        else
        {
            node = m_Nodes.ToPtr() + NodeIndex;
        }

        if (node->ViewMark != m_NodeViewMark)
            return;

        if (CullNode(m_ViewFrustum, node->Bounds, InCullBits))
        {
            //TotalCulled++;
            return;
        }

#if 0
        if ( VSD_CullBoxSingle( m_ViewFrustum, m_ViewFrustumPlanes, node->Bounds ) ) {
            Dbg_CullMiss++;
        }
#endif

        if (NodeIndex < 0)
        {
            // leaf
            break;
        }

        LevelTraverse_r(static_cast<BinarySpaceNode const*>(node)->ChildrenIdx[0], InCullBits);

        NodeIndex = static_cast<BinarySpaceNode const*>(node)->ChildrenIdx[1];
    }

    BinarySpaceLeaf const* pleaf = static_cast<BinarySpaceLeaf const*>(node);

    CullPrimitives(pleaf->Area, m_ViewFrustum, m_ViewFrustumPlanes);
}

void VisibilityLevel::FlowThroughPortals_r(VisArea const* InArea)
{
    PortalStack* prevStack = &m_pQueryContext->PStack[m_pQueryContext->PortalStackPos];
    PortalStack* stack = prevStack + 1;

    CullPrimitives(InArea, prevStack->AreaFrustum, prevStack->PlanesCount);

    if (m_pQueryContext->PortalStackPos == (VisibilityQueryContext::MAX_PORTAL_STACK - 1))
    {
        LOG("MAX_PORTAL_STACK hit\n");
        return;
    }

    ++m_pQueryContext->PortalStackPos;

#ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_StackDeep = Math::Max(Dbg_StackDeep, PortalStackPos);
#endif

    for (PortalLink const* portal = InArea->PortalList; portal; portal = portal->Next)
    {

        //if ( portal->Portal->VisFrame == m_VisQueryMarker ) {
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
        portal->Portal->VisMark = m_VisQueryMarker;

        FlowThroughPortals_r(portal->ToArea);
    }

    --m_pQueryContext->PortalStackPos;
}

bool VisibilityLevel::CalcPortalStack(PortalStack* OutStack, PortalStack const* InPrevStack, PortalLink const* InPortal)
{
    const float d = InPortal->Plane.DistanceToPoint(m_pQueryContext->ViewPosition);
    if (d <= 0.0f)
    {
#ifdef DEBUG_TRAVERSING_COUNTERS
        Dbg_SkippedByPlaneOffset++;
#endif
        return false;
    }

    if (d <= m_pQueryContext->ViewZNear)
    {
        // View intersecting the portal

        for (int i = 0; i < InPrevStack->PlanesCount; i++)
        {
            OutStack->AreaFrustum[i] = InPrevStack->AreaFrustum[i];
        }
        OutStack->PlanesCount = InPrevStack->PlanesCount;
        OutStack->Scissor = InPrevStack->Scissor;
    }
    else
    {

        //for ( int i = 0 ; i < PortalStackPos ; i++ ) {
        //    if ( PortalStack[ i ].Portal == InPortal ) {
        //        LOG( "Recursive!\n" );
        //    }
        //}

        PortalHull* portalWinding = CalcPortalWinding(InPortal, InPrevStack);

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
                OutStack->AreaFrustum[i].FromPoints(m_pQueryContext->ViewPosition, portalWinding->Points[i], portalWinding->Points[(i + 1) % portalWinding->NumPoints]);
            }

            // Copy far plane
            OutStack->AreaFrustum[OutStack->PlanesCount++] = InPrevStack->AreaFrustum[InPrevStack->PlanesCount - 1];
        }
        else
        {
            // Compute based on portal scissor
            const Float3 rightMin = m_pQueryContext->ViewRightVec * OutStack->Scissor.MinX + m_pQueryContext->ViewCenter;
            const Float3 rightMax = m_pQueryContext->ViewRightVec * OutStack->Scissor.MaxX + m_pQueryContext->ViewCenter;
            const Float3 upMin = m_pQueryContext->ViewUpVec * OutStack->Scissor.MinY;
            const Float3 upMax = m_pQueryContext->ViewUpVec * OutStack->Scissor.MaxY;
            const Float3 corners[4] =
                {
                    rightMin + upMin,
                    rightMax + upMin,
                    rightMax + upMax,
                    rightMin + upMax};

            Float3 p;

            // bottom
            p = Math::Cross(corners[1], corners[0]);
            OutStack->AreaFrustum[0].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[0].D = -Math::Dot(OutStack->AreaFrustum[0].Normal, m_pQueryContext->ViewPosition);

            // right
            p = Math::Cross(corners[2], corners[1]);
            OutStack->AreaFrustum[1].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[1].D = -Math::Dot(OutStack->AreaFrustum[1].Normal, m_pQueryContext->ViewPosition);

            // top
            p = Math::Cross(corners[3], corners[2]);
            OutStack->AreaFrustum[2].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[2].D = -Math::Dot(OutStack->AreaFrustum[2].Normal, m_pQueryContext->ViewPosition);

            // left
            p = Math::Cross(corners[0], corners[3]);
            OutStack->AreaFrustum[3].Normal = p * Math::RSqrt(Math::Dot(p, p));
            OutStack->AreaFrustum[3].D = -Math::Dot(OutStack->AreaFrustum[3].Normal, m_pQueryContext->ViewPosition);

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

static float ClipDistances[MAX_HULL_POINTS];
static PLANE_SIDE ClipSides[MAX_HULL_POINTS];

static bool ClipPolygonFast(Float3 const* InPoints, const int InNumPoints, PortalHull* Out, PlaneF const& InClipPlane, const float InEpsilon)
{
    int front = 0;
    int back = 0;
    int i;
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

    ClipSides[i] = ClipSides[0];
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

PortalHull* VisibilityLevel::CalcPortalWinding(PortalLink const* InPortal, PortalStack const* InStack)
{
    static PortalHull PortalHull[2];

    int flip = 0;

    Float3 const* hullPoints = InPortal->Hull->GetPoints();
    int numPoints = InPortal->Hull->NumPoints();

    // Clip portal hull by view plane
    if (!ClipPolygonFast(hullPoints, numPoints, &PortalHull[flip], m_pQueryContext->ViewPlane, 0.0f))
    {
        HK_ASSERT(numPoints <= MAX_HULL_POINTS);

        Platform::Memcpy(PortalHull[flip].Points, hullPoints, numPoints * sizeof(Float3));
        PortalHull[flip].NumPoints = numPoints;
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

void VisibilityLevel::CalcPortalScissor(PortalScissor& OutScissor, PortalHull const* InHull, PortalStack const* InStack)
{
    OutScissor.MinX = 99999999.0f;
    OutScissor.MinY = 99999999.0f;
    OutScissor.MaxX = -99999999.0f;
    OutScissor.MaxY = -99999999.0f;

    for (int i = 0; i < InHull->NumPoints; i++)
    {
        // Project portal vertex to view plane
        const Float3 vec = InHull->Points[i] - m_pQueryContext->ViewPosition;

        const float d = Math::Dot(m_pQueryContext->ViewPlane.Normal, vec);

        //if ( d < ViewZNear ) {
        //    HK_ASSERT(0);
        //}

        const Float3 p = d < m_pQueryContext->ViewZNear ? vec : vec * (m_pQueryContext->ViewZNear / d);

        // Compute relative coordinates
        const float x = Math::Dot(m_pQueryContext->ViewRightVec, p);
        const float y = Math::Dot(m_pQueryContext->ViewUpVec, p);

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

HK_FORCEINLINE bool VisibilityLevel::FaceCull(PrimitiveDef const* InPrimitive)
{
    return InPrimitive->Face.DistanceToPoint(m_pQueryContext->ViewPosition) < 0.0f;
}

HK_FORCEINLINE bool VisibilityLevel::FaceCull(SurfaceDef const* InSurface)
{
    return InSurface->Face.DistanceToPoint(m_pQueryContext->ViewPosition) < 0.0f;
}

void VisibilityLevel::CullPrimitives(VisArea const* InArea, PlaneF const* InCullPlanes, const int InCullPlanesCount)
{
    /*!!!
    if (vsd_FrustumCullingType.GetInteger() != FRUSTUM_CULLING_COMBINED)
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
        BrushModel* model = m_Model;

        int const* pSurfaceIndex = &m_AreaSurfaces[InArea->FirstSurface];

        for (int i = 0; i < InArea->NumSurfaces; i++, pSurfaceIndex++)
        {
            SurfaceDef* surf = &model->Surfaces[*pSurfaceIndex];

            if (surf->VisMark == m_VisQueryMarker)
            {
                // Surface visibility already processed
                continue;
            }

            // Mark surface visibility processed
            surf->VisMark = m_VisQueryMarker;

            // Filter query group
            if ((surf->QueryGroup & m_pQueryContext->VisQueryMask) != m_pQueryContext->VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & m_pQueryContext->VisibilityMask) == 0)
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
            surf->VisPass = m_VisQueryMarker;

            m_pQueryResult->pVisSurfs->Add(surf);
        }
    }

    for (PrimitiveLink* link = InArea->Links; link; link = link->NextInArea)
    {
        HK_ASSERT(link->Area == InArea);

        PrimitiveDef* primitive = link->Primitive;

        if (primitive->VisMark == m_VisQueryMarker)
        {
            // Primitive visibility already processed
            continue;
        }

        // Filter query group
        if ((primitive->QueryGroup & m_pQueryContext->VisQueryMask) != m_pQueryContext->VisQueryMask)
        {
            // Mark primitive visibility processed
            primitive->VisMark = m_VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & m_pQueryContext->VisibilityMask) == 0)
        {
            // Mark primitive visibility processed
            primitive->VisMark = m_VisQueryMarker;
            continue;
        }

        if ((primitive->Flags & SURF_PLANAR_TWOSIDED_MASK) == SURF_PLANAR)
        {
            // Perform face culling
            if (FaceCull(primitive))
            {
                // Face successfully culled
                primitive->VisMark = m_VisQueryMarker;

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
                /*!!!                if (vsd_FrustumCullingType.GetInteger() == FRUSTUM_CULLING_SIMPLE) */
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
        primitive->VisMark = m_VisQueryMarker;

        // Mark primitive visible
        primitive->VisPass = m_VisQueryMarker;

        // Add primitive to vis list
        m_pQueryResult->pVisPrimitives->Add(primitive);
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

        if (vsd_FrustumCullingType.GetInteger() == FRUSTUM_CULLING_SEPARATE)
        {
            SubmitCullingJobs(submit);

            // Wait when it's done
            GEngine->RenderFrontendJobList->Wait();

            Dbg_TotalPrimitiveBounds += numBoxes;

            CullingResult.ResizeInvalidate(Align(BoundingBoxesSSE.Size(), 4));

            PrimitiveDef** boxes      = BoxPrimitives.ToPtr() + submit.First;
            const int*      cullResult = CullingResult.ToPtr() + submit.First;

            for (int n = 0; n < submit.NumObjects; n++)
            {
                PrimitiveDef* primitive = boxes[n];

                if (primitive->VisMark != m_VisQueryMarker)
                {

                    if (!cullResult[n])
                    { // TODO: Use atomic increment and store only visible objects?
                        // Mark primitive visibility processed
                        primitive->VisMark = m_VisQueryMarker;

                        // Mark primitive visible
                        primitive->VisPass = m_VisQueryMarker;

                        m_pQueryResult->pVisPrimitives->Add(primitive);
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

void VisibilityLevel::QueryVisiblePrimitives(TPodVector<VisibilityLevel*> const& m_Levels, TPodVector<PrimitiveDef*>& VisPrimitives, TPodVector<SurfaceDef*>& VisSurfs, int* VisPass, VisibilityQuery const& InQuery)
{
    //int QueryVisiblePrimitivesTime = GEngine->SysMicroseconds();
    VisibilityQueryContext QueryContext;
    VisibilityQueryResult QueryResult;

    ++m_VisQueryMarker;

    if (VisPass)
    {
        *VisPass = m_VisQueryMarker;
    }

    QueryContext.VisQueryMask = InQuery.QueryMask;
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
    QueryContext.ViewUpVec = InQuery.ViewUpVec;
    QueryContext.ViewPlane = *InQuery.FrustumPlanes[FRUSTUM_PLANE_NEAR];
    QueryContext.ViewZNear = -QueryContext.ViewPlane.DistanceToPoint(QueryContext.ViewPosition); //Camera->GetZNear();
    QueryContext.ViewCenter = QueryContext.ViewPlane.Normal * QueryContext.ViewZNear;

    // Get corner at left-bottom of frustum
    Float3 corner = Math::Cross(InQuery.FrustumPlanes[FRUSTUM_PLANE_BOTTOM]->Normal, InQuery.FrustumPlanes[FRUSTUM_PLANE_LEFT]->Normal);

    // Project left-bottom corner to near plane
    corner = corner * (QueryContext.ViewZNear / Math::Dot(QueryContext.ViewPlane.Normal, corner));

    const float x = Math::Dot(QueryContext.ViewRightVec, corner);
    const float y = Math::Dot(QueryContext.ViewUpVec, corner);

    // w = tan( half_fov_x_rad ) * znear * 2;
    // h = tan( half_fov_y_rad ) * znear * 2;

    QueryContext.PortalStackPos = 0;
    QueryContext.PStack[0].AreaFrustum[0] = *InQuery.FrustumPlanes[0];
    QueryContext.PStack[0].AreaFrustum[1] = *InQuery.FrustumPlanes[1];
    QueryContext.PStack[0].AreaFrustum[2] = *InQuery.FrustumPlanes[2];
    QueryContext.PStack[0].AreaFrustum[3] = *InQuery.FrustumPlanes[3];
    QueryContext.PStack[0].AreaFrustum[4] = *InQuery.FrustumPlanes[4]; // far plane
    QueryContext.PStack[0].PlanesCount = 5;
    QueryContext.PStack[0].Portal = NULL;
    QueryContext.PStack[0].Scissor.MinX = x;
    QueryContext.PStack[0].Scissor.MinY = y;
    QueryContext.PStack[0].Scissor.MaxX = -x;
    QueryContext.PStack[0].Scissor.MaxY = -y;

    for (VisibilityLevel* level : m_Levels)
    {
        level->ProcessLevelVisibility(QueryContext, QueryResult);
    }
    /*!!!
    if (vsd_FrustumCullingType.GetInteger() == FRUSTUM_CULLING_COMBINED)
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
        ScopedTimer TimeCheck("Evaluate submits");

        for (SCullJobSubmit& submit : CullSubmits)
        {

            PrimitiveDef** boxes      = BoxPrimitives.ToPtr() + submit.First;
            int*            cullResult = CullingResult.ToPtr() + submit.First;

            for (int n = 0; n < submit.NumObjects; n++)
            {

                PrimitiveDef* primitive = boxes[n];

                if (primitive->VisMark != m_VisQueryMarker)
                {

                    if (!cullResult[n])
                    { // TODO: Use atomic increment and store only visible objects?
                        // Mark primitive visibility processed
                        primitive->VisMark = m_VisQueryMarker;

                        // Mark primitive visible
                        primitive->VisPass = m_VisQueryMarker;

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
    const Float3 h = Math::Cross(_RayDir, e2);

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

void VisibilityLevel::RaycastSurface(SurfaceDef* Self)
{
    float d, u, v;
    float boxMin, boxMax;

    if (Self->Flags & SURF_PLANAR)
    {
        // Calculate distance from ray origin to plane
        const float d1 = Math::Dot(m_pRaycast->RayStart, Self->Face.Normal) + Self->Face.D;
        float d2;

        if (Self->Flags & SURF_TWOSIDED)
        {
            // Check ray direction
            d2 = Math::Dot(Self->Face.Normal, m_pRaycast->RayDir);
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
            d2 = Math::Dot(Self->Face.Normal, m_pRaycast->RayDir);
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
            d2 = Math::Dot( Self->Face.Normal, m_pRaycast->RayDir );
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

        if (d >= m_pRaycast->HitDistanceMin)
        {
            // distance is too far
            return;
        }

        BrushModel const* brushModel = Self->Model;

        MeshVertex const* pVertices = brushModel->Vertices.ToPtr() + Self->FirstVertex;
        unsigned int const* pIndices = brushModel->Indices.ToPtr() + Self->FirstIndex;

        if (m_pRaycast->bClosest)
        {
            for (int i = 0; i < Self->NumIndices; i += 3)
            {
                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (RayIntersectTriangleFast(m_pRaycast->RayStart, m_pRaycast->RayDir, v0, v1, v2, u, v))
                {
                    m_pRaycast->HitProxyType = HIT_PROXY_SURFACE;
                    m_pRaycast->HitSurface = Self;
                    m_pRaycast->HitLocation = m_pRaycast->RayStart + m_pRaycast->RayDir * d;
                    m_pRaycast->HitDistanceMin = d;
                    m_pRaycast->HitUV.X = u;
                    m_pRaycast->HitUV.Y = v;
                    m_pRaycast->pVertices = brushModel->Vertices.ToPtr();
                    m_pRaycast->pLightmapVerts = brushModel->LightmapVerts.ToPtr();
                    m_pRaycast->LightmapBlock = Self->LightmapBlock;
                    m_pRaycast->LightingLevel = brushModel->ParentLevel.GetObject();
                    m_pRaycast->Indices[0] = Self->FirstVertex + triangleIndices[0];
                    m_pRaycast->Indices[1] = Self->FirstVertex + triangleIndices[1];
                    m_pRaycast->Indices[2] = Self->FirstVertex + triangleIndices[2];
                    m_pRaycast->Material = brushModel->SurfaceMaterials[Self->MaterialIndex];
                    m_pRaycast->NumHits++;

                    // Mark as visible
                    Self->VisPass = m_VisQueryMarker;

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

                if (RayIntersectTriangleFast(m_pRaycast->RayStart, m_pRaycast->RayDir, v0, v1, v2, u, v))
                {

                    TriangleHitResult& hitResult = m_pRaycastResult->Hits.Add();
                    hitResult.Location = m_pRaycast->RayStart + m_pRaycast->RayDir * d;
                    hitResult.Normal = Self->Face.Normal;
                    hitResult.Distance = d;
                    hitResult.UV.X = u;
                    hitResult.UV.Y = v;
                    hitResult.Indices[0] = Self->FirstVertex + triangleIndices[0];
                    hitResult.Indices[1] = Self->FirstVertex + triangleIndices[1];
                    hitResult.Indices[2] = Self->FirstVertex + triangleIndices[2];
                    hitResult.Material = brushModel->SurfaceMaterials[Self->MaterialIndex];

                    WorldRaycastPrimitive& rcPrimitive = m_pRaycastResult->Primitives.Add();
                    rcPrimitive.Object = nullptr;
                    rcPrimitive.FirstHit = rcPrimitive.ClosestHit = m_pRaycastResult->Hits.Size() - 1;
                    rcPrimitive.NumHits = 1;

                    // Mark as visible
                    Self->VisPass = m_VisQueryMarker;

                    break;
                }
            }
        }
    }
    else
    {
        bool cullBackFaces = !(Self->Flags & SURF_TWOSIDED);

        // Perform AABB raycast
        if (!BvRayIntersectBox(m_pRaycast->RayStart, m_pRaycast->InvRayDir, Self->Bounds, boxMin, boxMax))
        {
            return;
        }

        if (boxMin >= m_pRaycast->HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            return;
        }

        BrushModel const* brushModel = Self->Model;

        MeshVertex const* pVertices = brushModel->Vertices.ToPtr() + Self->FirstVertex;
        unsigned int const* pIndices = brushModel->Indices.ToPtr() + Self->FirstIndex;

        if (m_pRaycast->bClosest)
        {
            for (int i = 0; i < Self->NumIndices; i += 3)
            {
                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (BvRayIntersectTriangle(m_pRaycast->RayStart, m_pRaycast->RayDir, v0, v1, v2, d, u, v, cullBackFaces))
                {
                    if (m_pRaycast->HitDistanceMin > d)
                    {

                        m_pRaycast->HitProxyType = HIT_PROXY_SURFACE;
                        m_pRaycast->HitSurface = Self;
                        m_pRaycast->HitLocation = m_pRaycast->RayStart + m_pRaycast->RayDir * d;
                        m_pRaycast->HitDistanceMin = d;
                        m_pRaycast->HitUV.X = u;
                        m_pRaycast->HitUV.Y = v;
                        m_pRaycast->pVertices = brushModel->Vertices.ToPtr();
                        m_pRaycast->pLightmapVerts = brushModel->LightmapVerts.ToPtr();
                        m_pRaycast->LightmapBlock = Self->LightmapBlock;
                        m_pRaycast->LightingLevel = brushModel->ParentLevel.GetObject();
                        m_pRaycast->Indices[0] = Self->FirstVertex + triangleIndices[0];
                        m_pRaycast->Indices[1] = Self->FirstVertex + triangleIndices[1];
                        m_pRaycast->Indices[2] = Self->FirstVertex + triangleIndices[2];
                        m_pRaycast->Material = brushModel->SurfaceMaterials[Self->MaterialIndex];

                        // Mark as visible
                        Self->VisPass = m_VisQueryMarker;
                    }
                }
            }
        }
        else
        {
            int firstHit = m_pRaycastResult->Hits.Size();
            int closestHit = firstHit;

            for (int i = 0; i < Self->NumIndices; i += 3)
            {

                unsigned int const* triangleIndices = pIndices + i;

                Float3 const& v0 = pVertices[triangleIndices[0]].Position;
                Float3 const& v1 = pVertices[triangleIndices[1]].Position;
                Float3 const& v2 = pVertices[triangleIndices[2]].Position;

                if (BvRayIntersectTriangle(m_pRaycast->RayStart, m_pRaycast->RayDir, v0, v1, v2, d, u, v, cullBackFaces))
                {
                    if (m_pRaycast->RayLength > d)
                    {
                        TriangleHitResult& hitResult = m_pRaycastResult->Hits.Add();
                        hitResult.Location = m_pRaycast->RayStart + m_pRaycast->RayDir * d;
                        hitResult.Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                        hitResult.Distance = d;
                        hitResult.UV.X = u;
                        hitResult.UV.Y = v;
                        hitResult.Indices[0] = Self->FirstVertex + triangleIndices[0];
                        hitResult.Indices[1] = Self->FirstVertex + triangleIndices[1];
                        hitResult.Indices[2] = Self->FirstVertex + triangleIndices[2];
                        hitResult.Material = brushModel->SurfaceMaterials[Self->MaterialIndex];

                        // Mark as visible
                        Self->VisPass = m_VisQueryMarker;

                        // Find closest hit
                        if (d < m_pRaycastResult->Hits[closestHit].Distance)
                        {
                            closestHit = m_pRaycastResult->Hits.Size() - 1;
                        }
                    }
                }
            }

            if (Self->VisPass == m_VisQueryMarker)
            {
                WorldRaycastPrimitive& rcPrimitive = m_pRaycastResult->Primitives.Add();
                rcPrimitive.Object = nullptr;
                rcPrimitive.FirstHit = firstHit;
                rcPrimitive.NumHits = m_pRaycastResult->Hits.Size() - firstHit;
                rcPrimitive.ClosestHit = closestHit;
            }
        }
    }
}

void VisibilityLevel::RaycastPrimitive(PrimitiveDef* Self)
{
    // FIXME: What about two sided primitives? Use TwoSided flag directly from material or from primitive?

    if (m_pRaycast->bClosest)
    {
        TriangleHitResult hit;

        if (Self->RaycastClosestCallback && Self->RaycastClosestCallback(Self, m_pRaycast->RayStart, m_pRaycast->HitLocation, hit, &m_pRaycast->pVertices))
        {
            m_pRaycast->HitProxyType = HIT_PROXY_PRIMITIVE;
            m_pRaycast->HitPrimitive = Self;
            m_pRaycast->HitLocation = hit.Location;
            m_pRaycast->HitNormal = hit.Normal;
            m_pRaycast->HitUV = hit.UV;
            m_pRaycast->HitDistanceMin = hit.Distance;
            m_pRaycast->Indices[0] = hit.Indices[0];
            m_pRaycast->Indices[1] = hit.Indices[1];
            m_pRaycast->Indices[2] = hit.Indices[2];
            m_pRaycast->Material = hit.Material;

            // TODO:
            //m_pRaycast->pLightmapVerts = Self->Owner->LightmapUVChannel->GetVertices();
            //m_pRaycast->LightmapBlock = Self->Owner->LightmapBlock;
            //m_pRaycast->LightingLevel = Self->Owner->ParentLevel.GetObject();

            // Mark primitive visible
            Self->VisPass = m_VisQueryMarker;
        }
    }
    else
    {
        int firstHit = m_pRaycastResult->Hits.Size();
        if (Self->RaycastCallback && Self->RaycastCallback(Self, m_pRaycast->RayStart, m_pRaycast->RayEnd, m_pRaycastResult->Hits))
        {

            int numHits = m_pRaycastResult->Hits.Size() - firstHit;

            // Find closest hit
            int closestHit = firstHit;
            for (int i = 0; i < numHits; i++)
            {
                int hitNum = firstHit + i;
                TriangleHitResult& hitResult = m_pRaycastResult->Hits[hitNum];

                if (hitResult.Distance < m_pRaycastResult->Hits[closestHit].Distance)
                {
                    closestHit = hitNum;
                }
            }

            WorldRaycastPrimitive& rcPrimitive = m_pRaycastResult->Primitives.Add();

            rcPrimitive.Object = Self->Owner;
            rcPrimitive.FirstHit = firstHit;
            rcPrimitive.NumHits = m_pRaycastResult->Hits.Size() - firstHit;
            rcPrimitive.ClosestHit = closestHit;

            // Mark primitive visible
            Self->VisPass = m_VisQueryMarker;
        }
    }
}

void VisibilityLevel::RaycastArea(VisArea* InArea)
{
    float boxMin, boxMax;

    if (InArea->VisMark == m_VisQueryMarker)
    {
        // Area raycast already processed
        //LOG( "Area raycast already processed\n" );
        return;
    }

    // Mark area raycast processed
    InArea->VisMark = m_VisQueryMarker;

    if (InArea->NumSurfaces > 0)
    {
        BrushModel* model = m_Model;

        int const* pSurfaceIndex = &m_AreaSurfaces[InArea->FirstSurface];

        for (int i = 0; i < InArea->NumSurfaces; i++, pSurfaceIndex++)
        {

            SurfaceDef* surf = &model->Surfaces[*pSurfaceIndex];

            if (surf->VisMark == m_VisQueryMarker)
            {
                // Surface raycast already processed
                continue;
            }

            // Mark surface raycast processed
            surf->VisMark = m_VisQueryMarker;

            // Filter query group
            if ((surf->QueryGroup & m_pRaycast->VisQueryMask) != m_pRaycast->VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & m_pRaycast->VisibilityMask) == 0)
            {
                continue;
            }

            RaycastSurface(surf);

#ifdef CLOSE_ENOUGH_EARLY_OUT
            // hit is close enough to stop ray casting?
            if (m_pRaycast->HitDistanceMin < 0.0001f)
            {
                return;
            }
#endif
        }
    }

    for (PrimitiveLink* link = InArea->Links; link; link = link->NextInArea)
    {
        PrimitiveDef* primitive = link->Primitive;

        if (primitive->VisMark == m_VisQueryMarker)
        {
            // Primitive raycast already processed
            continue;
        }

        // Filter query group
        if ((primitive->QueryGroup & m_pRaycast->VisQueryMask) != m_pRaycast->VisQueryMask)
        {
            // Mark primitive raycast processed
            primitive->VisMark = m_VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & m_pRaycast->VisibilityMask) == 0)
        {
            // Mark primitive raycast processed
            primitive->VisMark = m_VisQueryMarker;
            continue;
        }

        if ((primitive->Flags & SURF_PLANAR_TWOSIDED_MASK) == SURF_PLANAR)
        {
            // Perform face culling
            if (primitive->Face.DistanceToPoint(m_pRaycast->RayStart) < 0.0f)
            {
                // Face successfully culled
                primitive->VisMark = m_VisQueryMarker;
                continue;
            }
        }

        switch (primitive->Type)
        {
            case VSD_PRIMITIVE_BOX: {
                // Perform AABB raycast
                if (!BvRayIntersectBox(m_pRaycast->RayStart, m_pRaycast->InvRayDir, primitive->Box, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            case VSD_PRIMITIVE_SPHERE: {
                // Perform Sphere raycast
                if (!BvRayIntersectSphere(m_pRaycast->RayStart, m_pRaycast->RayDir, primitive->Sphere, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            default: {
                HK_ASSERT(0);
                continue;
            }
        }

        if (boxMin >= m_pRaycast->HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            continue;
        }

        // Mark primitive raycast processed
        primitive->VisMark = m_VisQueryMarker;

        RaycastPrimitive(primitive);

#ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if (m_pRaycast->HitDistanceMin < 0.0001f)
        {
            return;
        }
#endif
    }
}

void VisibilityLevel::RaycastPrimitiveBounds(VisArea* InArea)
{
    float boxMin, boxMax;

    if (InArea->VisMark == m_VisQueryMarker)
    {
        // Area raycast already processed
        //DEBUG( "Area raycast already processed\n" );
        return;
    }

    // Mark area raycast processed
    InArea->VisMark = m_VisQueryMarker;

    if (InArea->NumSurfaces > 0)
    {
        BrushModel* model = m_Model;

        int const* pSurfaceIndex = &m_AreaSurfaces[InArea->FirstSurface];

        for (int i = 0; i < InArea->NumSurfaces; i++, pSurfaceIndex++)
        {

            SurfaceDef* surf = &model->Surfaces[*pSurfaceIndex];

            if (surf->VisMark == m_VisQueryMarker)
            {
                // Surface raycast already processed
                continue;
            }

            // Mark surface raycast processed
            surf->VisMark = m_VisQueryMarker;

            // Filter query group
            if ((surf->QueryGroup & m_pRaycast->VisQueryMask) != m_pRaycast->VisQueryMask)
            {
                continue;
            }

            // Check surface visibility group is not visible
            if ((surf->VisGroup & m_pRaycast->VisibilityMask) == 0)
            {
                continue;
            }

            if (surf->Flags & SURF_PLANAR)
            {
                // FIXME: planar surface has no bounds?
                continue;
            }

            // Perform AABB raycast
            if (!BvRayIntersectBox(m_pRaycast->RayStart, m_pRaycast->InvRayDir, surf->Bounds, boxMin, boxMax))
            {
                continue;
            }
            if (boxMin >= m_pRaycast->HitDistanceMin)
            {
                // Ray intersects the box, but box is too far
                continue;
            }

            // Mark as visible
            surf->VisPass = m_VisQueryMarker;

            if (m_pRaycast->bClosest)
            {
                m_pRaycast->HitProxyType = HIT_PROXY_SURFACE;
                m_pRaycast->HitSurface = surf;
                m_pRaycast->HitDistanceMin = boxMin;
                m_pRaycast->HitDistanceMax = boxMax;

#ifdef CLOSE_ENOUGH_EARLY_OUT
                // hit is close enough to stop ray casting?
                if (m_pRaycast->HitDistanceMin < 0.0001f)
                {
                    break;
                }
#endif
            }
            else
            {
                BoxHitResult& hitResult = m_pBoundsRaycastResult->Add();

                hitResult.Object = nullptr;
                hitResult.LocationMin = m_pRaycast->RayStart + m_pRaycast->RayDir * boxMin;
                hitResult.LocationMax = m_pRaycast->RayStart + m_pRaycast->RayDir * boxMax;
                hitResult.DistanceMin = boxMin;
                hitResult.DistanceMax = boxMax;
            }
        }
    }

    for (PrimitiveLink* link = InArea->Links; link; link = link->NextInArea)
    {
        PrimitiveDef* primitive = link->Primitive;

        if (primitive->VisMark == m_VisQueryMarker)
        {
            // Primitive raycast already processed
            continue;
        }

        // Filter query group
        if ((primitive->QueryGroup & m_pRaycast->VisQueryMask) != m_pRaycast->VisQueryMask)
        {
            // Mark primitive raycast processed
            primitive->VisMark = m_VisQueryMarker;
            continue;
        }

        // Check primitive visibility group is not visible
        if ((primitive->VisGroup & m_pRaycast->VisibilityMask) == 0)
        {
            // Mark primitive raycast processed
            primitive->VisMark = m_VisQueryMarker;
            continue;
        }

        switch (primitive->Type)
        {
            case VSD_PRIMITIVE_BOX: {
                // Perform AABB raycast
                if (!BvRayIntersectBox(m_pRaycast->RayStart, m_pRaycast->InvRayDir, primitive->Box, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            case VSD_PRIMITIVE_SPHERE: {
                // Perform Sphere raycast
                if (!BvRayIntersectSphere(m_pRaycast->RayStart, m_pRaycast->RayDir, primitive->Sphere, boxMin, boxMax))
                {
                    continue;
                }
                break;
            }
            default: {
                HK_ASSERT(0);
                continue;
            }
        }

        if (boxMin >= m_pRaycast->HitDistanceMin)
        {
            // Ray intersects the box, but box is too far
            continue;
        }

        // Mark primitive raycast processed
        primitive->VisMark = m_VisQueryMarker;

        // Mark primitive visible
        primitive->VisPass = m_VisQueryMarker;

        if (m_pRaycast->bClosest)
        {
            m_pRaycast->HitProxyType = HIT_PROXY_PRIMITIVE;
            m_pRaycast->HitPrimitive = primitive;
            m_pRaycast->HitDistanceMin = boxMin;
            m_pRaycast->HitDistanceMax = boxMax;

#ifdef CLOSE_ENOUGH_EARLY_OUT
            // hit is close enough to stop ray casting?
            if (m_pRaycast->HitDistanceMin < 0.0001f)
            {
                break;
            }
#endif
        }
        else
        {
            BoxHitResult& hitResult = m_pBoundsRaycastResult->Add();

            hitResult.Object = primitive->Owner;
            hitResult.LocationMin = m_pRaycast->RayStart + m_pRaycast->RayDir * boxMin;
            hitResult.LocationMax = m_pRaycast->RayStart + m_pRaycast->RayDir * boxMax;
            hitResult.DistanceMin = boxMin;
            hitResult.DistanceMax = boxMax;
        }
    }
}

#if 0
void VisibilitySystem::LevelRaycast_r( int NodeIndex ) {
    NodeBase const * node;
    float boxMin, boxMax;

    while ( 1 ) {
        if ( NodeIndex < 0 ) {
            node = &m_Leafs[-1 - NodeIndex];
        } else {
            node = m_Nodes.ToPtr() + NodeIndex;
        }

        if ( node->ViewMark != m_NodeViewMark )
            return;

        if ( !BvRayIntersectBox( m_pRaycast->RayStart, m_pRaycast->InvRayDir, node->Bounds, boxMin, boxMax ) ) {
            return;
        }

        if ( boxMin >= m_pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return;
        }

        if ( NodeIndex < 0 ) {
            // leaf
            break;
        }

        BinarySpaceNode const * n = static_cast< BinarySpaceNode const * >( node );

        LevelRaycast_r( n->ChildrenIdx[0] );

#    ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if ( m_pRaycast->HitDistanceMin < 0.0001f ) {
            return;
        }
#    endif

        NodeIndex = n->ChildrenIdx[1];
    }

    BinarySpaceLeaf const * pleaf = static_cast< BinarySpaceLeaf const * >( node );

    RaycastArea( pleaf->Area );
}
#else
bool VisibilityLevel::LevelRaycast2_r(int NodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd)
{
    if (NodeIndex < 0)
    {

        BinarySpaceLeaf const* leaf = &m_Leafs[-1 - NodeIndex];

#    if 0
        // FIXME: Add this additional checks?
        float boxMin, boxMax;
        if ( !BvRayIntersectBox( m_pRaycast->RayStart, m_pRaycast->InvRayDir, leaf->Bounds, boxMin, boxMax ) ) {
            return false;
        }
        if ( boxMin >= m_pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return false;
        }
#    endif

        RaycastArea(leaf->Area);

#    if 0
        if ( m_pRaycast->RayLength > m_pRaycast->HitDistanceMin ) {
        //if ( d >= m_pRaycast->HitDistanceMin ) {
            // stop raycasting
            return true;
        }
#    endif

        // continue raycasting
        return false;
    }

    BinarySpaceNode const* node = m_Nodes.ToPtr() + NodeIndex;

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
void VisibilitySystem::LevelRaycastBounds_r( int NodeIndex ) {
    NodeBase const * node;
    float boxMin, boxMax;

    while ( 1 ) {
        if ( NodeIndex < 0 ) {
            node = &m_Leafs[-1 - NodeIndex];
        } else {
            node = m_Nodes.ToPtr() + NodeIndex;
        }

        if ( node->ViewMark != m_NodeViewMark )
            return;

        if ( !BvRayIntersectBox( m_pRaycast->RayStart, m_pRaycast->InvRayDir, node->Bounds, boxMin, boxMax ) ) {
            return;
        }

        if ( boxMin >= m_pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return;
        }

        if ( NodeIndex < 0 ) {
            // leaf
            break;
        }

        BinarySpaceNode const * n = static_cast< BinarySpaceNode const * >( node );

        LevelRaycastBounds_r( n->ChildrenIdx[0] );

#    ifdef CLOSE_ENOUGH_EARLY_OUT
        // hit is close enough to stop ray casting?
        if ( m_pRaycast->HitDistanceMin < 0.0001f ) {
            return;
        }
#    endif

        NodeIndex = n->ChildrenIdx[1];
    }

    BinarySpaceLeaf const * pleaf = static_cast< BinarySpaceLeaf const * >( node );

    RaycastPrimitiveBounds( pleaf->Area );
}
#else
bool VisibilityLevel::LevelRaycastBounds2_r(int NodeIndex, Float3 const& InRayStart, Float3 const& InRayEnd)
{

    if (NodeIndex < 0)
    {

        BinarySpaceLeaf const* leaf = &m_Leafs[-1 - NodeIndex];

#    if 0
        float boxMin, boxMax;
        if ( !BvRayIntersectBox( InRayStart, m_pRaycast->InvRayDir, leaf->Bounds, boxMin, boxMax ) ) {
            return false;
        }

        if ( boxMin >= m_pRaycast->HitDistanceMin ) {
            // Ray intersects the box, but box is too far
            return false;
        }
#    endif

        RaycastPrimitiveBounds(leaf->Area);

        if (m_pRaycast->RayLength > m_pRaycast->HitDistanceMin)
        {
            //if ( d >= m_pRaycast->HitDistanceMin ) {
            // stop raycasting
            return true;
        }

        // continue raycasting
        return false;
    }

    BinarySpaceNode const* node = m_Nodes.ToPtr() + NodeIndex;

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

void VisibilityLevel::LevelRaycastPortals_r(VisArea* InArea)
{
    RaycastArea(InArea);

    for (PortalLink const* portal = InArea->PortalList; portal; portal = portal->Next)
    {

        if (portal->Portal->VisMark == m_VisQueryMarker)
        {
            // Already visited
            continue;
        }

        // Mark visited
        portal->Portal->VisMark = m_VisQueryMarker;

        if (portal->Portal->bBlocked)
        {
            // Portal is closed
            continue;
        }
#if 1
        // Calculate distance from ray origin to plane
        const float d1 = portal->Plane.DistanceToPoint(m_pRaycast->RayStart);
        if (d1 <= 0.0f)
        {
            // ray is behind
            continue;
        }

        // Check ray direction
        const float d2 = Math::Dot(portal->Plane.Normal, m_pRaycast->RayDir);
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
        if (!BvRayIntersectPlane(m_pRaycast->RayStart, m_pRaycast->RayDir, portal->Plane, dist))
        {
            continue;
        }
        if (dist <= 0.0f)
        {
            continue;
        }
#endif

        if (dist >= m_pRaycast->HitDistanceMin)
        {
            // Ray intersects the portal plane, but portal is too far
            continue;
        }

        const Float3 p = m_pRaycast->RayStart + m_pRaycast->RayDir * dist;

        if (!BvPointInConvexHullCCW(p, portal->Plane.Normal, portal->Hull->GetPoints(), portal->Hull->NumPoints()))
        {
            continue;
        }

        LevelRaycastPortals_r(portal->ToArea);
    }
}

void VisibilityLevel::LevelRaycastBoundsPortals_r(VisArea* InArea)
{

    RaycastPrimitiveBounds(InArea);

    for (PortalLink const* portal = InArea->PortalList; portal; portal = portal->Next)
    {

        if (portal->Portal->VisMark == m_VisQueryMarker)
        {
            // Already visited
            continue;
        }

        // Mark visited
        portal->Portal->VisMark = m_VisQueryMarker;

        if (portal->Portal->bBlocked)
        {
            // Portal is closed
            continue;
        }

#if 1
        // Calculate distance from ray origin to plane
        const float d1 = portal->Plane.DistanceToPoint(m_pRaycast->RayStart);
        if (d1 <= 0.0f)
        {
            // ray is behind
            continue;
        }

        // Check ray direction
        const float d2 = Math::Dot(portal->Plane.Normal, m_pRaycast->RayDir);
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
        if (!BvRayIntersectPlane(m_pRaycast->RayStart, m_pRaycast->RayDir, portal->Plane, dist))
        {
            continue;
        }
        if (dist <= 0.0f)
        {
            continue;
        }
#endif

        if (dist >= m_pRaycast->HitDistanceMin)
        {
            // Ray intersects the portal plane, but portal is too far
            continue;
        }

        const Float3 p = m_pRaycast->RayStart + m_pRaycast->RayDir * dist;

        if (!BvPointInConvexHullCCW(p, portal->Plane.Normal, portal->Hull->GetPoints(), portal->Hull->NumPoints()))
        {
            continue;
        }

        LevelRaycastBoundsPortals_r(portal->ToArea);
    }
}

void VisibilityLevel::ProcessLevelRaycast(VisRaycast& Raycast, WorldRaycastResult& Result)
{
    m_pRaycast = &Raycast;
    m_pRaycastResult = &Result;

    // TODO: check level bounds (ray/aabb overlap)?

    if (m_VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( m_pRaycast->RayStart );

        m_NodeViewMark = MarkLeafs( leaf );

        LevelRaycast_r( 0 );
#else
        LevelRaycast2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (m_VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        VisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastPortals_r(area);
    }
}

void VisibilityLevel::ProcessLevelRaycastClosest(VisRaycast& Raycast)
{
    m_pRaycast = &Raycast;
    m_pRaycastResult = nullptr;

    // TODO: check level bounds (ray/aabb overlap)?

    if (m_VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( m_pRaycast->RayStart );

        m_NodeViewMark = MarkLeafs( leaf );

        LevelRaycast_r( 0 );
#else
        LevelRaycast2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (m_VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        VisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastPortals_r(area);
    }
}

void VisibilityLevel::ProcessLevelRaycastBounds(VisRaycast& Raycast, TPodVector<BoxHitResult>& Result)
{
    m_pRaycast = &Raycast;
    m_pBoundsRaycastResult = &Result;

    // TODO: check level bounds (ray/aabb overlap)?

    if (m_VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( Raycast.RayStart );

        m_NodeViewMark = MarkLeafs( leaf );

        LevelRaycastBounds_r( 0 );
#else
        LevelRaycastBounds2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (m_VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        VisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastBoundsPortals_r(area);
    }
}

void VisibilityLevel::ProcessLevelRaycastClosestBounds(VisRaycast& Raycast)
{
    m_pRaycast = &Raycast;
    m_pBoundsRaycastResult = nullptr;

    // TODO: check level bounds (ray/aabb overlap)?

    if (m_VisibilityMethod == LEVEL_VISIBILITY_PVS)
    {
        // Level has precomputed visibility

#if 0
        int leaf = FindLeaf( Raycast.RayStart );

        m_NodeViewMark = MarkLeafs( leaf );

        LevelRaycastBounds_r( 0 );
#else
        LevelRaycastBounds2_r(0, Raycast.RayStart, Raycast.RayEnd);
#endif
    }
    else if (m_VisibilityMethod == LEVEL_VISIBILITY_PORTAL)
    {
        VisArea* area = FindArea(Raycast.RayStart);

        LevelRaycastBoundsPortals_r(area);
    }
}

bool VisibilityLevel::RaycastTriangles(TPodVector<VisibilityLevel*> const& m_Levels, WorldRaycastResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, WorldRaycastFilter const* InFilter)
{
    VisRaycast Raycast;

    ++m_VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

    Result.Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart = InRayStart;
    Raycast.RayEnd = InRayEnd;
    Raycast.RayDir = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z = 1.0f / Raycast.RayDir.Z;
    //Raycast.HitObject is unused
    //Raycast.HitLocation is unused
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.bClosest = false;

    for (VisibilityLevel* level : m_Levels)
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

bool VisibilityLevel::RaycastClosest(TPodVector<VisibilityLevel*> const& m_Levels, WorldRaycastClosestResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, WorldRaycastFilter const* InFilter)
{
    VisRaycast Raycast;

    ++m_VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

    Result.Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart = InRayStart;
    Raycast.RayEnd = InRayEnd;
    Raycast.RayDir = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z = 1.0f / Raycast.RayDir.Z;
    Raycast.HitProxyType = HIT_PROXY_UNKNOWN;
    Raycast.HitLocation = InRayEnd;
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.bClosest = true;
    Raycast.pVertices = nullptr;
    Raycast.pLightmapVerts = nullptr;
    Raycast.NumHits = 0;

    for (VisibilityLevel* level : m_Levels)
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

    if (Raycast.HitProxyType == HIT_PROXY_PRIMITIVE)
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
    else if (Raycast.HitProxyType == HIT_PROXY_SURFACE)
    {
        MeshVertex const* vertices = Raycast.pVertices;

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

        Float2 uv0 = vertices[Raycast.Indices[0]].GetTexCoord();
        Float2 uv1 = vertices[Raycast.Indices[1]].GetTexCoord();
        Float2 uv2 = vertices[Raycast.Indices[2]].GetTexCoord();
        Result.Texcoord = uv0 * hitW + uv1 * Raycast.HitUV[0] + uv2 * Raycast.HitUV[1];

        if (Raycast.pLightmapVerts && Raycast.LightingLevel && Raycast.LightmapBlock >= 0)
        {
            Float2 const& lm0 = Raycast.pLightmapVerts[Raycast.Indices[0]].TexCoord;
            Float2 const& lm1 = Raycast.pLightmapVerts[Raycast.Indices[1]].TexCoord;
            Float2 const& lm2 = Raycast.pLightmapVerts[Raycast.Indices[2]].TexCoord;
            Float2 lighmapTexcoord = lm0 * hitW + lm1 * Raycast.HitUV[0] + lm2 * Raycast.HitUV[1];

            Level const* level = Raycast.LightingLevel;

            Result.LightmapSample_Experimental = level->SampleLight(Raycast.LightmapBlock, lighmapTexcoord);
        }
    }
    else
    {
        // No intersection
        return false;
    }

    Result.Fraction = Raycast.HitDistanceMin / Raycast.RayLength;

    TriangleHitResult& triangleHit = Result.TriangleHit;
    triangleHit.Normal = Raycast.HitNormal;
    triangleHit.Location = Raycast.HitLocation;
    triangleHit.Distance = Raycast.HitDistanceMin;
    triangleHit.Indices[0] = Raycast.Indices[0];
    triangleHit.Indices[1] = Raycast.Indices[1];
    triangleHit.Indices[2] = Raycast.Indices[2];
    triangleHit.Material = Raycast.Material;
    triangleHit.UV = Raycast.HitUV;

    return true;
}

bool VisibilityLevel::RaycastBounds(TPodVector<VisibilityLevel*> const& m_Levels, TPodVector<BoxHitResult>& Result, Float3 const& InRayStart, Float3 const& InRayEnd, WorldRaycastFilter const* InFilter)
{
    VisRaycast Raycast;

    ++m_VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

    Result.Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart = InRayStart;
    Raycast.RayEnd = InRayEnd;
    Raycast.RayDir = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z = 1.0f / Raycast.RayDir.Z;
    //Raycast.HitObject is unused
    //Raycast.HitLocation is unused
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.bClosest = false;

    for (VisibilityLevel* level : m_Levels)
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
            bool operator()(BoxHitResult const& _A, BoxHitResult const& _B)
            {
                return (_A.DistanceMin < _B.DistanceMin);
            }
        } SortHit;

        std::sort(Result.ToPtr(), Result.ToPtr() + Result.Size(), SortHit);
    }

    return true;
}

bool VisibilityLevel::RaycastClosestBounds(TPodVector<VisibilityLevel*> const& m_Levels, BoxHitResult& Result, Float3 const& InRayStart, Float3 const& InRayEnd, WorldRaycastFilter const* InFilter)
{
    VisRaycast Raycast;

    ++m_VisQueryMarker;

    InFilter = InFilter ? InFilter : &DefaultRaycastFilter;

    Raycast.VisQueryMask = InFilter->QueryMask;
    Raycast.VisibilityMask = InFilter->VisibilityMask;

    Result.Clear();

    Float3 rayVec = InRayEnd - InRayStart;

    Raycast.RayLength = rayVec.Length();

    if (Raycast.RayLength < 0.0001f)
    {
        return false;
    }

    Raycast.RayStart = InRayStart;
    Raycast.RayEnd = InRayEnd;
    Raycast.RayDir = rayVec / Raycast.RayLength;
    Raycast.InvRayDir.X = 1.0f / Raycast.RayDir.X;
    Raycast.InvRayDir.Y = 1.0f / Raycast.RayDir.Y;
    Raycast.InvRayDir.Z = 1.0f / Raycast.RayDir.Z;
    Raycast.HitProxyType = HIT_PROXY_UNKNOWN;
    //Raycast.HitLocation is unused
    Raycast.HitDistanceMin = Raycast.RayLength;
    Raycast.HitDistanceMax = Raycast.RayLength;
    Raycast.bClosest = true;

    for (VisibilityLevel* level : m_Levels)
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

    if (Raycast.HitProxyType == HIT_PROXY_PRIMITIVE)
    {
        Result.Object = Raycast.HitPrimitive->Owner;
    }
    else if (Raycast.HitProxyType == HIT_PROXY_SURFACE)
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


TPoolAllocator<PrimitiveDef> VisibilitySystem::PrimitivePool;
TPoolAllocator<PrimitiveLink> VisibilitySystem::PrimitiveLinkPool;

PrimitiveDef* VisibilitySystem::AllocatePrimitive()
{
    PrimitiveDef* primitive = new (PrimitivePool.Allocate()) PrimitiveDef;
    return primitive;
}

void VisibilitySystem::DeallocatePrimitive(PrimitiveDef* Primitive)
{
    Primitive->~PrimitiveDef();
    PrimitivePool.Deallocate(Primitive);
}

VisibilitySystem::VisibilitySystem()
{}

VisibilitySystem::~VisibilitySystem()
{
    HK_ASSERT(m_Levels.IsEmpty());
}

void VisibilitySystem::RegisterLevel(VisibilityLevel* Level)
{
    if (m_Levels.Contains(Level))
        return;

    m_Levels.Add(Level);
    Level->AddRef();

    MarkPrimitives();
}

void VisibilitySystem::UnregisterLevel(VisibilityLevel* Level)
{
    auto i = m_Levels.IndexOf(Level);
    if (i == Core::NPOS)
        return;

    m_Levels[i]->RemoveRef();
    m_Levels.Remove(i);

    MarkPrimitives();
    UpdatePrimitiveLinks();
}

void VisibilitySystem::AddPrimitive(PrimitiveDef* Primitive)
{
    if (INTRUSIVE_EXISTS(Primitive, Next, Prev, m_PrimitiveList, m_PrimitiveListTail))
    {
        // Already added
        return;
    }

    INTRUSIVE_ADD(Primitive, Next, Prev, m_PrimitiveList, m_PrimitiveListTail);

    VisibilityLevel::AddPrimitiveToLevelAreas(m_Levels, Primitive);
}

void VisibilitySystem::RemovePrimitive(PrimitiveDef* Primitive)
{
    if (!INTRUSIVE_EXISTS(Primitive, Next, Prev, m_PrimitiveList, m_PrimitiveListTail))
    {
        // Not added at all
        return;
    }

    INTRUSIVE_REMOVE(Primitive, Next, Prev, m_PrimitiveList, m_PrimitiveListTail);
    INTRUSIVE_REMOVE(Primitive, NextUpd, PrevUpd, m_PrimitiveDirtyList, m_PrimitiveDirtyListTail);

    UnlinkPrimitive(Primitive);
}

void VisibilitySystem::RemovePrimitives()
{
    UnmarkPrimitives();

    PrimitiveDef* next;
    for (PrimitiveDef* primitive = m_PrimitiveList; primitive; primitive = next)
    {
        UnlinkPrimitive(primitive);

        next = primitive->Next;
        primitive->Prev = primitive->Next = nullptr;
    }

    m_PrimitiveList = m_PrimitiveListTail = nullptr;
}

void VisibilitySystem::MarkPrimitive(PrimitiveDef* Primitive)
{
    if (!INTRUSIVE_EXISTS(Primitive, Next, Prev, m_PrimitiveList, m_PrimitiveListTail))
    {
        // Not added at all
        return;
    }

    INTRUSIVE_ADD_UNIQUE(Primitive, NextUpd, PrevUpd, m_PrimitiveDirtyList, m_PrimitiveDirtyListTail);
}

void VisibilitySystem::MarkPrimitives()
{
    for (PrimitiveDef* primitive = m_PrimitiveList; primitive; primitive = primitive->Next)
    {
        MarkPrimitive(primitive);
    }
}

void VisibilitySystem::UnmarkPrimitives()
{
    PrimitiveDef* next;
    for (PrimitiveDef* primitive = m_PrimitiveDirtyList; primitive; primitive = next)
    {
        next = primitive->NextUpd;
        primitive->PrevUpd = primitive->NextUpd = nullptr;
    }
    m_PrimitiveDirtyList = m_PrimitiveDirtyListTail = nullptr;
}

void VisibilitySystem::UpdatePrimitiveLinks()
{
    PrimitiveDef* next;

    // First Pass: remove primitives from the areas
    for (PrimitiveDef* primitive = m_PrimitiveDirtyList; primitive; primitive = primitive->NextUpd)
    {
        UnlinkPrimitive(primitive);
    }

    // Second Pass: add primitives to the areas
    for (PrimitiveDef* primitive = m_PrimitiveDirtyList; primitive; primitive = next)
    {
        VisibilityLevel::AddPrimitiveToLevelAreas(m_Levels, primitive);

        next = primitive->NextUpd;
        primitive->PrevUpd = primitive->NextUpd = nullptr;
    }

    m_PrimitiveDirtyList = m_PrimitiveDirtyListTail = nullptr;
}

void VisibilitySystem::UnlinkPrimitive(PrimitiveDef* Primitive)
{
    PrimitiveLink* link = Primitive->Links;

    while (link)
    {
        HK_ASSERT(link->Area);

        PrimitiveLink** prev = &link->Area->Links;
        while (1)
        {
            PrimitiveLink* walk = *prev;

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

        PrimitiveLink* free = link;
        link = link->Next;

        PrimitiveLinkPool.Deallocate(free);
    }

    Primitive->Links = nullptr;
}

void VisibilitySystem::DrawDebug(DebugRenderer* Renderer)
{
    for (VisibilityLevel* level : m_Levels)
    {
        level->DrawDebug(Renderer);
    }
}

void VisibilitySystem::QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<VisArea*>& m_Areas) const
{
    for (VisibilityLevel* level : m_Levels)
    {
        level->QueryOverplapAreas(Bounds, m_Areas);
    }
}

void VisibilitySystem::QueryOverplapAreas(BvSphere const& Bounds, TPodVector<VisArea*>& m_Areas) const
{
    for (VisibilityLevel* level : m_Levels)
    {
        level->QueryOverplapAreas(Bounds, m_Areas);
    }
}

void VisibilitySystem::QueryVisiblePrimitives(TPodVector<PrimitiveDef*>& VisPrimitives, TPodVector<SurfaceDef*>& VisSurfs, int* VisPass, VisibilityQuery const& Query) const
{
    VisibilityLevel::QueryVisiblePrimitives(m_Levels, VisPrimitives, VisSurfs, VisPass, Query);
}

bool VisibilitySystem::RaycastTriangles(WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilityLevel::RaycastTriangles(m_Levels, Result, RayStart, RayEnd, Filter);
}

bool VisibilitySystem::RaycastClosest(WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilityLevel::RaycastClosest(m_Levels, Result, RayStart, RayEnd, Filter);
}

bool VisibilitySystem::RaycastBounds(TPodVector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilityLevel::RaycastBounds(m_Levels, Result, RayStart, RayEnd, Filter);
}

bool VisibilitySystem::RaycastClosestBounds(BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilityLevel::RaycastClosestBounds(m_Levels, Result, RayStart, RayEnd, Filter);
}

HK_NAMESPACE_END
