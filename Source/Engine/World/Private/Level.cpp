/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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


#include <World/Public/Level.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/World.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Components/CameraComponent.h>
#include <World/Public/Components/PointLightComponent.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Resource/Texture.h>
#include <Core/Public/BV/BvIntersect.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Runtime/Public/Runtime.h>

ARuntimeVariable RVDrawLevelAreaBounds( _CTS( "DrawLevelAreaBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawLevelIndoorBounds( _CTS( "DrawLevelIndoorBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawLevelPortals( _CTS( "DrawLevelPortals" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( ALevel )

#define MAX_PRIMITIVE_LINKS 40000

class APrimitiveLinkPool {
public:
    APrimitiveLinkPool() {
        int i;
        memset( Pool, 0, sizeof( Pool ) );
        FreeLinks = Pool;
        for ( i = 0 ; i < MAX_PRIMITIVE_LINKS - 1 ; i++ ) {
            FreeLinks[i].Next = &FreeLinks[ i + 1 ];
        }
        FreeLinks[i].Next = NULL;
        UsedLinks = 0;
    }

    SPrimitiveLink * AllocateLink() {
    #if 0
        return new VSDPrimitiveLink;
    #else
        SPrimitiveLink * link = FreeLinks;
        if ( !link ) {
            GLogger.Printf( "APrimitiveLinkPool::AllocateLink: no free links (used %d from %d)\n", UsedLinks, MAX_PRIMITIVE_LINKS );
            return nullptr;
        }
        FreeLinks = FreeLinks->Next;
        ++UsedLinks;
        //GLogger.Printf( "AllocateLink: %d\n", UsedLinks );
        return link;
    #endif
    }

    void FreeLink( SPrimitiveLink * Link ) {
    #if 0
        delete Link;
    #else
        Link->Next = FreeLinks;
        FreeLinks = Link;
        --UsedLinks;
    #endif
    }

private:
    SPrimitiveLink Pool[MAX_PRIMITIVE_LINKS];
    SPrimitiveLink * FreeLinks;
    int UsedLinks;
};

class ANGIE_API AWorldspawn : public AActor {
    AN_ACTOR( AWorldspawn, AActor )

public:

protected:
    AWorldspawn();

    void PreInitializeComponents() override;
    void PostInitializeComponents() override;
    void BeginPlay() override;
    void Tick( float _TimeStep ) override;

    //void DrawDebug( ADebugRenderer * InRenderer ) override;

private:
    void UpdateAmbientVolume( float _TimeStep );

    TStdVector< TRef< AAudioControlCallback > > AmbientControl;
    ASceneComponent * AudioInstigator;
    APhysicalBody * WorldCollision;
};

static APrimitiveLinkPool GPrimitiveLinkPool;

static SPrimitiveLink ** LastLink;
static SVisArea * TopArea;

static void AddPrimitiveToArea( SVisArea * Area, SPrimitiveDef * Primitive ) {
    if ( !TopArea ) {
        TopArea = Area;
    }

    SPrimitiveLink * link = GPrimitiveLinkPool.AllocateLink();
    if ( !link ) {
        return;
    }

    link->Primitive = Primitive;

    // Create the primitive link
    *LastLink = link;
    LastLink = &link->Next;
    link->Next = nullptr;

    // Create the leaf links
    link->Area = Area;
    link->NextArea = Area->Links;
    Area->Links = link;
}

void ALevel::DestroyActors() {
    for ( AActor * actor : Actors ) {
        actor->Destroy();
    }
}

void ALevel::OnAddLevelToWorld() {
}

void ALevel::OnRemoveLevelFromWorld() {
}

int ALevel::AddArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint ) {

    SVisArea * area = &Areas.Append();

    memset( area, 0, sizeof( *area ) );
    area->ReferencePoint = _ReferencePoint;
    //area->ParentLevel = this;
    Float3 halfExtents = _Extents * 0.5f;
    area->Bounds.Mins = _Position - halfExtents;
    area->Bounds.Maxs = _Position + halfExtents;

    return Areas.Size() - 1;
}

int ALevel::AddPortal( Float3 const * _HullPoints, int _NumHullPoints, int _Area1, int _Area2 ) {
    if ( _Area1 == _Area2 ) {
        return -1;
    }

    SVisPortal * portal = &Portals.Append();
    portal->Hull = AConvexHull::CreateFromPoints( _HullPoints, _NumHullPoints );
    portal->Plane = portal->Hull->CalcPlane();
    portal->Area[0] = _Area1 >= 0 ? &Areas[_Area1] : &OutdoorArea;
    portal->Area[1] = _Area2 >= 0 ? &Areas[_Area2] : &OutdoorArea;
    //portal->ParentLevel = this;

    return Portals.Size() - 1;
}

void ALevel::Purge() {
    DestroyActors();

    if ( OwnerWorld )
    {
        OwnerWorld->RemovePrimitives();
    }

    if ( Worldspawn )
    {
        Worldspawn->Destroy();
        Worldspawn = nullptr;
    }

    RemoveLightmapUVChannels();
    RemoveVertexLightChannels();

    PurgePortals();

    Areas.Free();

    for ( SVisPortal & portal : Portals ) {
        AConvexHull::Destroy( portal.Hull );
    }

    Portals.Free();

    Nodes.Free();

    Leafs.Free();

    SplitPlanes.Free();

    IndoorBounds.Clear();

    PVSClustersCount = 0;

    HugeFree( Visdata );
    Visdata = nullptr;

    HugeFree( DecompressedVisData );
    DecompressedVisData = nullptr;

    HugeFree( LightData );
    LightData = nullptr;

    AreaSurfaces.Free();

    Model.Reset();

    AudioAreas.Free();
    AudioClips.Free();

    Lightmaps.Free();

    if ( OwnerWorld )
    {
        OwnerWorld->MarkPrimitives();
    }
}

void ALevel::PurgePortals() {
    //RemovePrimitives();

    for ( SPortalLink & areaPortal : AreaPortals ) {
        AConvexHull::Destroy( areaPortal.Hull );
    }

    AreaPortals.Free();
}

void ALevel::Initialize() {

    PurgePortals();

    IndoorBounds.Clear();

    for ( SVisArea & area : Areas ) {
        IndoorBounds.AddAABB( area.Bounds );

        // Clear area portals
        area.PortalList = NULL;
    }

    AreaPortals.ResizeInvalidate( Portals.Size() << 1 );

    int areaPortalId = 0;

    for ( SVisPortal & portal : Portals ) {
        SVisArea * a1 = portal.Area[0];
        SVisArea * a2 = portal.Area[1];

        if ( a1 == &OutdoorArea ) {
            StdSwap( a1, a2 );
        }

        // Check area position relative to portal plane
        EPlaneSide offset = portal.Plane.SideOffset( a1->ReferencePoint, 0.0f );

        // If area position is on back side of plane, then reverse hull vertices and plane
        int id = offset == EPlaneSide::Back ? 1 : 0;

        SPortalLink * portalLink;

        portalLink = &AreaPortals[ areaPortalId++ ];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a2;
        if ( id & 1 ) {
            portalLink->Hull = portal.Hull->Reversed();
            portalLink->Plane = -portal.Plane;
        } else {
            portalLink->Hull = portal.Hull->Duplicate();
            portalLink->Plane = portal.Plane;
        }
        portalLink->Next = a1->PortalList;
        portalLink->Portal = &portal;
        a1->PortalList = portalLink;

        id = ( id + 1 ) & 1;

        portalLink = &AreaPortals[ areaPortalId++ ];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a1;
        if ( id & 1 ) {
            portalLink->Hull = portal.Hull->Reversed();
            portalLink->Plane = -portal.Plane;
        } else {
            portalLink->Hull = portal.Hull->Duplicate();
            portalLink->Plane = portal.Plane;
        }
        portalLink->Next = a2->PortalList;
        portalLink->Portal = &portal;
        a2->PortalList = portalLink;
    }

    //AddPrimitives();

    if ( Worldspawn )
    {
        Worldspawn->Destroy();
    }

    Worldspawn = OwnerWorld->SpawnActor< AWorldspawn >( this );

    ViewMark = 0;
    ViewCluster = -1;

    if ( bCompressedVisData && Visdata && PVSClustersCount > 0 )
    {
        // Allocate decompressed vis data
        //DecompressedVisData = (byte *)HugeAlloc( 0x20000/*Leafs.Size() / 8 + 1*/ );

        DecompressedVisData = (byte *)HugeAlloc( PVSClustersCount / 8 + 1 );
        
    }
}

ALevel::ALevel() {
    ViewCluster = -1;

    Float3 extents( CONVEX_HULL_MAX_BOUNDS * 2 );

    memset( &OutdoorArea, 0, sizeof( OutdoorArea ) );

    //OutdoorArea.ParentLevel = this;
    OutdoorArea.Bounds.Mins = -extents * 0.5f;
    OutdoorArea.Bounds.Maxs = extents * 0.5f;

    IndoorBounds.Clear();
}

ALevel::~ALevel() {
    Purge();
}

int ALevel::FindLeaf( Float3 const & _Position ) {
    SBinarySpaceNode * node;
    SBinarySpacePlane * plane;
    float d;
    int nodeIndex;

    if ( !Nodes.Size() ) {
        return -1;
    }

    node = Nodes.ToPtr();
    while ( 1 ) {
        plane = node->Plane;

        if ( plane->Type < 3 ) {
            // Simple case for axial planes
            d = _Position[ plane->Type ] + plane->D;
        } else {
            // General case for non-axial planes
            d = Math::Dot( _Position, plane->Normal ) + plane->D;
        }

        // Choose child
        nodeIndex = node->ChildrenIdx[ ( d <= 0 ) ];

        if ( nodeIndex <= 0 ) {
            // solid if node index == 0 or leaf if node index < 0
            return -1 - nodeIndex;
        }

        node = Nodes.ToPtr() + nodeIndex;
    }
    return -1;
}

SVisArea * ALevel::FindArea( Float3 const & _Position ) {
    if ( !Nodes.IsEmpty() ) {
        int leaf = FindLeaf( _Position );
        if ( leaf < 0 ) {
            // solid
            return &OutdoorArea;
        }
        return Leafs[leaf].Area;
    }

    // Bruteforce TODO: remove this!
    for ( int i = 0 ; i < Areas.Size() ; i++ ) {
        if (    _Position.X >= Areas[i].Bounds.Mins.X
             && _Position.Y >= Areas[i].Bounds.Mins.Y
             && _Position.Z >= Areas[i].Bounds.Mins.Z
             && _Position.X <  Areas[i].Bounds.Maxs.X
             && _Position.Y <  Areas[i].Bounds.Maxs.Y
             && _Position.Z <  Areas[i].Bounds.Maxs.Z ) {
            return &Areas[i];
        }
    }

    return &OutdoorArea;
}

byte const * ALevel::DecompressVisdata( byte const * InCompressedData ) {
    int count;

    int row = ( PVSClustersCount + 7 ) >> 3;
    byte * pDecompressed = DecompressedVisData;

    do {
        // Copy raw data
        if ( *InCompressedData ) {
            *pDecompressed++ = *InCompressedData++;
            continue;
        }

        // Zeros count
        count = InCompressedData[ 1 ];

        // Clamp zeros count if invalid
        if ( pDecompressed - DecompressedVisData + count > row ) {
            count = row - ( pDecompressed - DecompressedVisData );
        }

        // Move to the next sequence
        InCompressedData += 2;

        while ( count-- ) {
            *pDecompressed++ = 0;
        }
    } while ( pDecompressed - DecompressedVisData < row );

    return DecompressedVisData;
}

byte const * ALevel::LeafPVS( SBinarySpaceLeaf const * _Leaf ) {
    if ( bCompressedVisData ) {
        return _Leaf->Visdata ? DecompressVisdata( _Leaf->Visdata ) : nullptr;
    } else {
        return _Leaf->Visdata;
    }
}

int ALevel::MarkLeafs( int _ViewLeaf ) {
    if ( _ViewLeaf < 0 ) {
        return ViewMark;
    }

    SBinarySpaceLeaf * viewLeaf = &Leafs[ _ViewLeaf ];

    if ( ViewCluster == viewLeaf->Cluster ) {
        return ViewMark;
    }

    ViewMark++;
    ViewCluster = viewLeaf->Cluster;

    byte const * vis = LeafPVS( viewLeaf );
    if ( vis )
    {
        int cluster;
        for ( SBinarySpaceLeaf & leaf : Leafs ) {

            cluster = leaf.Cluster;
            if ( cluster < 0 || cluster >= PVSClustersCount ) {
                continue;
            }

            if ( !( vis[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) {
                continue;
            }

            // TODO: check for door connection here

            SBinarySpaceNode * parent = ( SBinarySpaceNode* )&leaf;
            do {
                if ( parent->ViewMark == ViewMark ) {
                    break;
                }
                parent->ViewMark = ViewMark;
                parent = parent->Parent;
            } while ( parent );
        }
    } else {
        // Mark all
        int cluster;
        for ( SBinarySpaceLeaf & leaf : Leafs ) {
            cluster = leaf.Cluster;
            if ( cluster < 0 || cluster >= PVSClustersCount ) {
                continue;
            }
            SBinarySpaceNode * parent = ( SBinarySpaceNode* )&leaf;
            do {
                if ( parent->ViewMark == ViewMark ) {
                    break;
                }
                parent->ViewMark = ViewMark;
                parent = parent->Parent;
            } while ( parent );
        }
    }

    return ViewMark;
}

void ALevel::Tick( float _TimeStep ) {

}

void ALevel::DrawDebug( ADebugRenderer * InRenderer ) {

    //AConvexHull * hull = AConvexHull::CreateForPlane( PlaneF(Float3(0,0,1), Float3(0.0f) ), 100.0f );

    //InRenderer->SetDepthTest( false );
    //InRenderer->SetColor( AColor4( 0, 1, 1, 0.5f ) );
    //InRenderer->DrawConvexPoly( hull->Points, hull->NumPoints, false );

    //Float3 normal = hull->CalcNormal();

    //InRenderer->SetColor( AColor4::White() );
    //InRenderer->DrawLine( Float3(0.0f), Float3( 0, 0, 1 )*100.0f );

    //AConvexHull::Destroy( hull );



    if ( RVDrawLevelAreaBounds ) {
        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( AColor4( 0,1,0,0.5f) );
        for ( SVisArea & area : Areas ) {
            InRenderer->DrawAABB( area.Bounds );
        }
    }

    if ( RVDrawLevelPortals ) {
//        InRenderer->SetDepthTest( false );
//        InRenderer->SetColor(1,0,0,1);
//        for ( ALevelPortal & portal : Portals ) {
//            InRenderer->DrawLine( portal.Hull->Points, portal.Hull->NumPoints, true );
//        }

        InRenderer->SetDepthTest( false );
        //InRenderer->SetColor( AColor4( 0,0,1,0.4f ) );

        /*if ( LastVisitedArea >= 0 && LastVisitedArea < Areas.Size() ) {
            VSDArea * area = &Areas[ LastVisitedArea ];
            VSDPortalLink * portals = area->PortalList;

            for ( VSDPortalLink * p = portals; p; p = p->Next ) {
                InRenderer->DrawConvexPoly( p->Hull->Points, p->Hull->NumPoints, true );
            }
        } else */{

#if 0
            for ( SVisPortal & portal : Portals ) {

                if ( portal.VisMark == InRenderer->GetVisPass() ) {
                    InRenderer->SetColor( AColor4( 1,0,0,0.4f ) );
                } else {
                    InRenderer->SetColor( AColor4( 0,1,0,0.4f ) );
                }
                InRenderer->DrawConvexPoly( portal.Hull->Points, portal.Hull->NumPoints, true );
            }
#else
            SPortalLink * portals = OutdoorArea.PortalList;

            for ( SPortalLink * p = portals; p; p = p->Next ) {

                if ( p->Portal->VisMark == InRenderer->GetVisPass() ) {
                    InRenderer->SetColor( AColor4( 1, 0, 0, 0.4f ) );
                } else {
                    InRenderer->SetColor( AColor4( 0, 1, 0, 0.4f ) );
                }

                InRenderer->DrawConvexPoly( p->Hull->Points, p->Hull->NumPoints, false );
            }

            for ( SVisArea & area : Areas ) {
                portals = area.PortalList;

                for ( SPortalLink * p = portals; p; p = p->Next ) {

                    if ( p->Portal->VisMark == InRenderer->GetVisPass() ) {
                        InRenderer->SetColor( AColor4( 1, 0, 0, 0.4f ) );
                    } else {
                        InRenderer->SetColor( AColor4( 0, 1, 0, 0.4f ) );
                    }

                    InRenderer->DrawConvexPoly( p->Hull->Points, p->Hull->NumPoints, false );
                }
            }
#endif
        }
    }

    if ( RVDrawLevelIndoorBounds ) {
        InRenderer->SetDepthTest( false );
        InRenderer->DrawAABB( IndoorBounds );
    }
}

void ALevel::AddBoxRecursive( SPrimitiveDef * InPrimitive ) {
    SBinarySpaceNode * node = Nodes.ToPtr();

    // TODO: precalc signbits
    int sideMask = BvBoxOverlapPlaneSideMask( InPrimitive->Box, *node->Plane, node->Plane->Type, node->Plane->SignBits() );

    if ( sideMask & 1 ) {
        AddBoxRecursive( node->ChildrenIdx[0], InPrimitive );
    }

    if ( sideMask & 2 ) {
        AddBoxRecursive( node->ChildrenIdx[1], InPrimitive );
    }
}

void ALevel::AddBoxRecursive( int _NodeIndex, SPrimitiveDef * InPrimitive ) {
    if ( _NodeIndex == 0 ) {
        // solid
        return;
    }

    if ( _NodeIndex < 0 ) {
        // leaf
        AddPrimitiveToArea( Leafs[-1 - _NodeIndex].Area, InPrimitive );
        return;
    }

    SBinarySpaceNode * node = &Nodes[ _NodeIndex ];

    // TODO: precalc signbits
    int sideMask = BvBoxOverlapPlaneSideMask( InPrimitive->Box, *node->Plane, node->Plane->Type, node->Plane->SignBits() );

    if ( sideMask & 1 ) {
        AddBoxRecursive( node->ChildrenIdx[0], InPrimitive );
    }

    if ( sideMask & 2 ) {
        AddBoxRecursive( node->ChildrenIdx[1], InPrimitive );
    }
}

void ALevel::AddSphereRecursive( SPrimitiveDef * InPrimitive ) {
    SBinarySpaceNode * node = Nodes.ToPtr();

    float d;

    if ( node->Plane->Type < 3 ) {
        //d = InPrimitive->Sphere.Center[ node->Plane->Type ] * node->Plane->Normal[ node->Plane->Type ] + node->Plane->D;
        d = InPrimitive->Sphere.Center[ node->Plane->Type ] + node->Plane->D;
    } else {
        d = Math::Dot( InPrimitive->Sphere.Center, node->Plane->Normal ) + node->Plane->D;
    }

    if ( d > -InPrimitive->Sphere.Radius ) {
        AddSphereRecursive( node->ChildrenIdx[0], InPrimitive );
    }

    if ( d < InPrimitive->Sphere.Radius ) {
        AddSphereRecursive( node->ChildrenIdx[1], InPrimitive );
    }
}

void ALevel::AddSphereRecursive( int _NodeIndex, SPrimitiveDef * InPrimitive ) {
    if ( _NodeIndex == 0 ) {
        // solid
        return;
    }

    if ( _NodeIndex < 0 ) {
        // leaf
        AddPrimitiveToArea( Leafs[ -1 - _NodeIndex ].Area, InPrimitive );
        return;
    }

    SBinarySpaceNode * node = &Nodes[ _NodeIndex ];

    float d;

    if ( node->Plane->Type < 3 ) {
        //d = InPrimitive->Sphere.Center[ node->Plane->Type ] * node->Plane->Normal[ node->Plane->Type ] + node->Plane->D;
        d = InPrimitive->Sphere.Center[ node->Plane->Type ] + node->Plane->D;
    } else {
        d = Math::Dot( InPrimitive->Sphere.Center, node->Plane->Normal ) + node->Plane->D;
    }

    if ( d > -InPrimitive->Sphere.Radius ) {
        AddSphereRecursive( node->ChildrenIdx[0], InPrimitive );
    }

    if ( d < InPrimitive->Sphere.Radius ) {
        AddSphereRecursive( node->ChildrenIdx[1], InPrimitive );
    }
}

void ALevel::AddPrimitive( AWorld * InWorld, SPrimitiveDef * InPrimitive ) {

    LastLink = &InPrimitive->Links;
    TopArea = nullptr;

    if ( !InPrimitive->bPendingRemove )
    {
        if ( InPrimitive->bMovable )
        {
            // Add movable primitives to all the levels
            for ( ALevel * level : InWorld->GetArrayOfLevels() )
            {
                level->AddPrimitiveToLevel( InPrimitive );
            }
        }
        else
        {
            ALevel * level = InPrimitive->Owner->GetLevel();

            level->AddPrimitiveToLevel( InPrimitive );
        }
    }

    InPrimitive->TopArea = TopArea;
}

void ALevel::AddPrimitiveToLevel( SPrimitiveDef * InPrimitive ) {

    bool bHaveBinaryTree = Nodes.Size() > 0;

    if ( bHaveBinaryTree ) {
        switch ( InPrimitive->Type ) {
            case VSD_PRIMITIVE_BOX:
                AddBoxRecursive( InPrimitive );
                break;
            case VSD_PRIMITIVE_SPHERE:
                AddSphereRecursive( InPrimitive );
                break;
        }
    } else {

        // No binary tree. Use bruteforce.

        // TODO: remove this path

        if ( InPrimitive->bIsOutdoor ) {
            // add to outdoor
            AddPrimitiveToArea( &OutdoorArea, InPrimitive );
        }
        else {
            int numAreas = Areas.Size();
            SVisArea * area;

            bool bInsideIndoorArea = false;

            switch( InPrimitive->Type ) {
                case VSD_PRIMITIVE_BOX:
                {
                    if ( BvBoxOverlapBox( IndoorBounds, InPrimitive->Box ) ) {
                        for ( int i = 0 ; i < numAreas ; i++ ) {
                            area = &Areas[i];

                            if ( BvBoxOverlapBox( area->Bounds, InPrimitive->Box ) ) {
                                AddPrimitiveToArea( area, InPrimitive );

                                bInsideIndoorArea = true;
                            }
                        }
                    }
                    break;
                }

                case VSD_PRIMITIVE_SPHERE:
                {
                    if ( BvBoxOverlapSphere( IndoorBounds, InPrimitive->Sphere ) ) {
                        for ( int i = 0 ; i < numAreas ; i++ ) {
                            area = &Areas[i];

                            if ( BvBoxOverlapSphere( area->Bounds, InPrimitive->Sphere ) ) {
                                AddPrimitiveToArea( area, InPrimitive );

                                bInsideIndoorArea = true;
                            }
                        }
                    }
                    break;
                }
            }

            if ( !bInsideIndoorArea ) {
                AddPrimitiveToArea( &OutdoorArea, InPrimitive );
            }
        }
    }
}

void ALevel::RemovePrimitive( SPrimitiveDef * InPrimitive ) {
    SPrimitiveLink * link = InPrimitive->Links;

    while ( link ) {
        SPrimitiveLink ** prev = &link->Area->Links;
        while ( 1 ) {
            SPrimitiveLink * walk = *prev;

            if ( !walk ) {
                break;
            }

            if ( walk == link ) {
                // remove this link
                *prev = link->NextArea;
                break;
            }

            prev = &walk->NextArea;
        }

        SPrimitiveLink * free = link;
        link = link->Next;

        GPrimitiveLinkPool.FreeLink( free );
    }

    InPrimitive->Links = nullptr;
}

ALightmapUV * ALevel::CreateLightmapUVChannel( AIndexedMesh * InSourceMesh ) {
    ALightmapUV * lightmapUV = NewObject< ALightmapUV >();
    lightmapUV->AddRef();
    lightmapUV->Initialize( InSourceMesh, this, false );
    LightmapUVs.Append( lightmapUV );
    return lightmapUV;
}

void ALevel::RemoveLightmapUVChannels() {
    for ( ALightmapUV * lightmapUV : LightmapUVs ) {
        lightmapUV->Purge();
        lightmapUV->RemoveRef();
    }
    LightmapUVs.Free();
}

AVertexLight * ALevel::CreateVertexLightChannel( AIndexedMesh * InSourceMesh ) {
    AVertexLight * vertexLight = NewObject< AVertexLight >();
    vertexLight->AddRef();
    vertexLight->Initialize( InSourceMesh, this, false );
    VertexLightChannels.Append( vertexLight );
    return vertexLight;
}

void ALevel::RemoveVertexLightChannels() {
    for ( AVertexLight * vertexLight : VertexLightChannels ) {
        vertexLight->Purge();
        vertexLight->RemoveRef();
    }
    VertexLightChannels.Free();
}




AN_CLASS_META( AWorldspawn )

AWorldspawn::AWorldspawn() {
    AudioInstigator = CreateComponent< ASceneComponent >( "AudioInstigator" );

    WorldCollision = CreateComponent< APhysicalBody >( "WorldCollision" );
    WorldCollision->SetPhysicsBehavior( PB_STATIC );
    WorldCollision->SetCollisionGroup( CM_WORLD_STATIC );
    WorldCollision->SetCollisionMask( CM_ALL );
    WorldCollision->SetAINavigationBehavior( AI_NAVIGATION_BEHAVIOR_STATIC );

    bCanEverTick = true;
}

void AWorldspawn::PreInitializeComponents() {
    Super::PreInitializeComponents();

    ALevel * level = GetLevel();

    // Setup world collision
    if ( level->Model ) {
        level->Model->BodyComposition.Duplicate( WorldCollision->BodyComposition );
    }

    // Create ambient control
    int ambientCount = level->AudioClips.Size();
    AmbientControl.Resize( ambientCount );
    for ( int i = 0 ; i < ambientCount ; i++ ) {
        AmbientControl[i] = NewObject< AAudioControlCallback >();
        AmbientControl[i]->VolumeScale = 0;
    }
}

void AWorldspawn::PostInitializeComponents() {
    Super::PostInitializeComponents();
}

void AWorldspawn::BeginPlay() {
    Super::BeginPlay();

    ALevel * level = GetLevel();

    SSoundSpawnParameters ambient;

    ambient.Location = AUDIO_STAY_BACKGROUND;
    ambient.Priority = AUDIO_CHANNEL_PRIORITY_AMBIENT;
    ambient.bVirtualizeWhenSilent = true;
    ambient.Volume = 0.1f;// 0.02f;
    ambient.Pitch = 1;
    ambient.bLooping = true;
    ambient.bStopWhenInstigatorDead = true;

    int ambientCount = level->AudioClips.Size();

    for ( int i = 0 ; i < ambientCount ; i++ ) {
        ambient.ControlCallback = AmbientControl[i];
        GAudioSystem.PlaySound( level->AudioClips[i], AudioInstigator, &ambient );
    }
}

void AWorldspawn::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    UpdateAmbientVolume( _TimeStep );
}

void AWorldspawn::UpdateAmbientVolume( float _TimeStep ) {
    ALevel * level = GetLevel();

    int leaf = level->FindLeaf( GAudioSystem.GetListenerPosition() );

    if ( leaf < 0 ) {
        int ambientCount = AmbientControl.Size();

        for ( int i = 0 ; i < ambientCount ; i++ ) {
            AmbientControl[i]->VolumeScale = 0.0f;
        }
        return;
    }

    int audioAreaNum = level->Leafs[leaf].AudioArea;

    SAudioArea const * audioArea = &level->AudioAreas[audioAreaNum];

    const float step = _TimeStep;
    for ( int i = 0 ; i < MAX_AMBIENT_SOUNDS_IN_AREA ; i++ ) {
        uint16_t soundIndex = audioArea->AmbientSound[i];
        float volume = (float)audioArea->AmbientVolume[i]/255.0f;

        AAudioControlCallback * control = AmbientControl[soundIndex];

        if ( control->VolumeScale < volume ) {
            control->VolumeScale += step;

            if ( control->VolumeScale > volume ) {
                control->VolumeScale = volume;
            }
        } else if ( control->VolumeScale > volume ) {
            control->VolumeScale -= step;

            if ( control->VolumeScale < 0 ) {
                control->VolumeScale = 0;
            }
        }
    }
}

AN_CLASS_META( ABrushModel )
void ABrushModel::Purge() {
    Surfaces.Free();

    Vertices.Free();

    LightmapVerts.Free();

    VertexLight.Free();

    Indices.Free();

    SurfaceMaterials.Free();

    BodyComposition.Clear();
}
