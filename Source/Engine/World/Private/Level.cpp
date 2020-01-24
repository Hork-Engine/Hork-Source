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
#include "PrimitiveLinkPool.h"

ARuntimeVariable RVDrawLevelAreaBounds( _CTS( "DrawLevelAreaBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawLevelIndoorBounds( _CTS( "DrawLevelIndoorBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawLevelPortals( _CTS( "DrawLevelPortals" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( ALevel )

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
        DecompressedVisData = (byte *)HugeAlloc( ( PVSClustersCount + 7 ) >> 3 );
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

int ALevel::FindLeaf( Float3 const & InPosition ) {
    SBinarySpaceNode * node;
    float d;
    int nodeIndex;

    if ( Nodes.IsEmpty() ) {
        return -1;
    }

    node = Nodes.ToPtr();
    while ( 1 ) {
        d = node->Plane->DistFast( InPosition );

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

SVisArea * ALevel::FindArea( Float3 const & InPosition ) {
    if ( !Nodes.IsEmpty() ) {
        int leaf = FindLeaf( InPosition );
        if ( leaf < 0 ) {
            // solid
            return &OutdoorArea;
        }
        return Leafs[leaf].Area;
    }

    // Bruteforce TODO: remove this!
    for ( int i = 0 ; i < Areas.Size() ; i++ ) {
        if (    InPosition.X >= Areas[i].Bounds.Mins.X
             && InPosition.Y >= Areas[i].Bounds.Mins.Y
             && InPosition.Z >= Areas[i].Bounds.Mins.Z
             && InPosition.X <  Areas[i].Bounds.Maxs.X
             && InPosition.Y <  Areas[i].Bounds.Maxs.Y
             && InPosition.Z <  Areas[i].Bounds.Maxs.Z ) {
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

        // Clamp zeros count if invalid. This can be moved to preprocess stage.
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

byte const * ALevel::LeafPVS( SBinarySpaceLeaf const * InLeaf ) {
    if ( bCompressedVisData ) {
        return InLeaf->Visdata ? DecompressVisdata( InLeaf->Visdata ) : nullptr;
    } else {
        return InLeaf->Visdata;
    }
}

int ALevel::MarkLeafs( int InViewLeaf ) {
    if ( InViewLeaf < 0 ) {
        return ViewMark;
    }

    SBinarySpaceLeaf * pLeaf = &Leafs[ InViewLeaf ];

    if ( ViewCluster == pLeaf->PVSCluster ) {
        return ViewMark;
    }

    ViewMark++;
    ViewCluster = pLeaf->PVSCluster;

    byte const * pVisibility = LeafPVS( pLeaf );
    if ( pVisibility )
    {
        int cluster;
        for ( SBinarySpaceLeaf & leaf : Leafs ) {

            cluster = leaf.PVSCluster;

            if ( cluster < 0 || cluster >= PVSClustersCount
                 || !( pVisibility[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) {
                continue;
            }

            // TODO: check doors here

            SNodeBase * parent = &leaf;
            do {
                if ( parent->ViewMark == ViewMark ) {
                    break;
                }
                parent->ViewMark = ViewMark;
                parent = parent->Parent;
            } while ( parent );
        }
    }
    else
    {
        // Mark all
        int cluster;
        for ( SBinarySpaceLeaf & leaf : Leafs ) {
            cluster = leaf.PVSCluster;
            if ( cluster < 0 || cluster >= PVSClustersCount ) {
                continue;
            }

            SNodeBase * parent = &leaf;
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

AN_FORCEINLINE float HalfToFloat( const unsigned short _Half ) {
    float f;
    *reinterpret_cast< uint32_t * >( &f ) = Math::HalfToFloat( _Half );
    return f;
}

Float3 ALevel::SampleLight( int InLightmapBlock, Float2 const & InLighmapTexcoord ) const {
    if ( !LightData ) {
        return Float3( 1.0f );
    }

    AN_ASSERT( InLightmapBlock >= 0 && InLightmapBlock < Lightmaps.Size() );

    int numChannels = ( LightmapFormat == LIGHTMAP_GRAYSCALED_HALF ) ? 1 : 3;
    int blockSize = LightmapBlockWidth * LightmapBlockHeight * numChannels;

    const unsigned short * src = (const unsigned short *)LightData + InLightmapBlock * blockSize;

    const float sx = Math::Clamp( InLighmapTexcoord.X, 0.0f, 1.0f ) * (LightmapBlockWidth-1);
    const float sy = Math::Clamp( InLighmapTexcoord.Y, 0.0f, 1.0f ) * (LightmapBlockHeight-1);
    const Float2 lerp( Math::Fract( sx ), Math::Fract( sy ) );

    const int x0 = Math::ToIntFast( sx );
    const int y0 = Math::ToIntFast( sy );
    int x1 = x0 + 1;
    if ( x1 >= LightmapBlockWidth )
        x1 = LightmapBlockWidth - 1;
    int y1 = y0 + 1;
    if ( y1 >= LightmapBlockHeight )
        y1 = LightmapBlockHeight - 1;

    const int offset00 = ( y0 * LightmapBlockWidth + x0 ) * numChannels;
    const int offset10 = ( y0 * LightmapBlockWidth + x1 ) * numChannels;
    const int offset01 = ( y1 * LightmapBlockWidth + x0 ) * numChannels;
    const int offset11 = ( y1 * LightmapBlockWidth + x1 ) * numChannels;

    const unsigned short * src00 = src + offset00;
    const unsigned short * src10 = src + offset10;
    const unsigned short * src01 = src + offset01;
    const unsigned short * src11 = src + offset11;

    Float3 light(0);

    switch ( LightmapFormat ) {
    case LIGHTMAP_GRAYSCALED_HALF:
    {
        //light[0] = light[1] = light[2] = HalfToFloat(src00[0]);
        light[0] = light[1] = light[2] = lerp.Bilerp( HalfToFloat( src00[0] ), HalfToFloat( src10[0] ), HalfToFloat( src01[0] ), HalfToFloat( src11[0] ) );
        break;
    }
    case LIGHTMAP_BGR_HALF:
    {
        for ( int i = 0 ; i < 3 ; i++ ) {
            //light[2-i] = HalfToFloat(src00[i]);
            light[2-i] = lerp.Bilerp( HalfToFloat(src00[i]), HalfToFloat(src10[i]), HalfToFloat(src01[i]), HalfToFloat(src11[i]) );
        }
        break;
    }
    default:
        GLogger.Printf( "ALevel::SampleLight: Unknown lightmap format\n" );
        break;
    }

    return light;
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

void ALevel::QueryOverplapAreas_r( int InNodeIndex, BvAxisAlignedBox const & InBounds, TPodArray< SVisArea * > & OutAreas ) {
    do {
        if ( InNodeIndex < 0 ) {
            // leaf
            SVisArea * area = Leafs[ -1 - InNodeIndex ].Area;
            if ( !OutAreas.IsExist( area ) ) {
                OutAreas.Append( area );
            }
            return;
        }

        SBinarySpaceNode * node = &Nodes[ InNodeIndex ];

        // TODO: precalc signbits
        int sideMask = BvBoxOverlapPlaneSideMask( InBounds, *node->Plane, node->Plane->Type, node->Plane->SignBits() );

        if ( sideMask == 1 ) {
            InNodeIndex = node->ChildrenIdx[0];
        } else if ( sideMask == 2 ) {
            InNodeIndex = node->ChildrenIdx[1];
        } else {
            if ( node->ChildrenIdx[1] != 0 ) {
                QueryOverplapAreas_r( node->ChildrenIdx[1], InBounds, OutAreas );
            }
            InNodeIndex = node->ChildrenIdx[0];
        }
    } while ( InNodeIndex != 0 );
}

void ALevel::QueryOverplapAreas_r( int InNodeIndex, BvSphere const & InBounds, TPodArray< SVisArea * > & OutAreas ) {
    do {
        if ( InNodeIndex < 0 ) {
            // leaf
            SVisArea * area = Leafs[ -1 - InNodeIndex ].Area;
            if ( !OutAreas.IsExist( area ) ) {
                OutAreas.Append( area );
            }
            return;
        }

        SBinarySpaceNode * node = &Nodes[ InNodeIndex ];

        float d = node->Plane->DistFast( InBounds.Center );

        if ( d > InBounds.Radius ) {
            InNodeIndex = node->ChildrenIdx[0];
        } else if ( d < -InBounds.Radius ) {
            InNodeIndex = node->ChildrenIdx[1];
        } else {
            if ( node->ChildrenIdx[1] != 0 ) {
                QueryOverplapAreas_r( node->ChildrenIdx[1], InBounds, OutAreas );
            }
            InNodeIndex = node->ChildrenIdx[0];
        }
    } while ( InNodeIndex != 0 );
}

void ALevel::QueryOverplapAreas( BvAxisAlignedBox const & InBounds, TPodArray< SVisArea * > & OutAreas ) {
    OutAreas.Clear();

    if ( Nodes.IsEmpty() ) {
        return;
    }

    QueryOverplapAreas_r( 0, InBounds, OutAreas );
}

void ALevel::QueryOverplapAreas( BvSphere const & InBounds, TPodArray< SVisArea * > & OutAreas ) {
    OutAreas.Clear();

    if ( Nodes.IsEmpty() ) {
        return;
    }

    QueryOverplapAreas_r( 0, InBounds, OutAreas );
}

static SPrimitiveLink ** LastLink;

static AN_FORCEINLINE bool IsPrimitiveInArea( SPrimitiveDef const * InPrimitive, SVisArea const * InArea ) {
    for ( SPrimitiveLink const * link = InPrimitive->Links ; link ; link = link->Next ) {
        if ( link->Area == InArea ) {
            return true;
        }
    }
    return false;
}

static void AddPrimitiveToArea( SVisArea * Area, SPrimitiveDef * Primitive ) {
    if ( IsPrimitiveInArea( Primitive, Area ) ) {
        return;
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

    // Create the area links
    link->Area = Area;
    link->NextInArea = Area->Links;
    Area->Links = link;
}

void ALevel::AddBoxRecursive( int InNodeIndex, SPrimitiveDef * InPrimitive ) {
    do {
        if ( InNodeIndex < 0 ) {
            // leaf
            AddPrimitiveToArea( Leafs[ -1 - InNodeIndex ].Area, InPrimitive );
            return;
        }

        SBinarySpaceNode * node = &Nodes[ InNodeIndex ];

        // TODO: precalc signbits
        int sideMask = BvBoxOverlapPlaneSideMask( InPrimitive->Box, *node->Plane, node->Plane->Type, node->Plane->SignBits() );

        if ( sideMask == 1 ) {
            InNodeIndex = node->ChildrenIdx[0];
        } else if ( sideMask == 2 ) {
            InNodeIndex = node->ChildrenIdx[1];
        } else {
            if ( node->ChildrenIdx[1] != 0 ) {
                AddBoxRecursive( node->ChildrenIdx[1], InPrimitive );
            }
            InNodeIndex = node->ChildrenIdx[0];
        }
    } while ( InNodeIndex != 0 );
}

void ALevel::AddSphereRecursive( int InNodeIndex, SPrimitiveDef * InPrimitive ) {
    do {
        if ( InNodeIndex < 0 ) {
            // leaf
            AddPrimitiveToArea( Leafs[ -1 - InNodeIndex ].Area, InPrimitive );
            return;
        }

        SBinarySpaceNode * node = &Nodes[ InNodeIndex ];

        float d = node->Plane->DistFast( InPrimitive->Sphere.Center );

        if ( d > InPrimitive->Sphere.Radius ) {
            InNodeIndex = node->ChildrenIdx[0];
        } else if ( d < -InPrimitive->Sphere.Radius ) {
            InNodeIndex = node->ChildrenIdx[1];
        } else {
            if ( node->ChildrenIdx[1] != 0 ) {
                AddSphereRecursive( node->ChildrenIdx[1], InPrimitive );
            }
            InNodeIndex = node->ChildrenIdx[0];
        }
    } while ( InNodeIndex != 0 );
}

void ALevel::AddPrimitive( AWorld * InWorld, SPrimitiveDef * InPrimitive ) {

    LastLink = &InPrimitive->Links;

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
}

void ALevel::AddPrimitiveToLevel( SPrimitiveDef * InPrimitive ) {

    bool bHaveBinaryTree = Nodes.Size() > 0;

    if ( bHaveBinaryTree ) {
        switch ( InPrimitive->Type ) {
            case VSD_PRIMITIVE_BOX:
                AddBoxRecursive( 0, InPrimitive );
                break;
            case VSD_PRIMITIVE_SPHERE:
                AddSphereRecursive( 0, InPrimitive );
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
                *prev = link->NextInArea;
                break;
            }

            prev = &walk->NextInArea;
        }

        SPrimitiveLink * free = link;
        link = link->Next;

        GPrimitiveLinkPool.FreeLink( free );
    }

    InPrimitive->Links = nullptr;
}


void ALevel::RemovePrimitiveFromLevel( SPrimitiveDef * InPrimitive ) {
#if 0
    SPrimitiveLink * prev = nullptr;
    SPrimitiveLink * next;
    for ( SPrimitiveLink * link = InPrimitive->Links ; link ; link = next ) {
        next = link->Next;

        SVisArea * area = link->Area;
        if ( area->Level != this ) {
            prev = link;
            continue;
        }

        // Перебираем все ссылки на примитивы в area, чтобы найти удаляемый примитив
        SPrimitiveLink ** last = &area->Links;
        while ( 1 ) {
            SPrimitiveLink * walk = *last;

            if ( !walk ) {
                break;
            }

            if ( walk == link ) {
                // remove this link
                *last = link->NextInArea;
                break;
            }

            last = &walk->NextInArea;
        }

        if ( link == InPrimitive->Links ) {
            InPrimitive->Links = next;
        }

        if ( prev ) {
            prev->Next = next;
        } else {
            InPrimitive->Links = next;
        }

        GPrimitiveLinkPool.FreeLink( link );
    }
#endif
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

    ambient.SourceType = AUDIO_SOURCE_BACKGROUND;
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














#if 0
template< typename T, int MAX_BLOCK_SIZE = 1024 >
class ADynamicPool {
    struct Entry : T {
        Entry * Next;
    };

    struct SBlock {
        Entry Pool[MAX_BLOCK_SIZE];
        Entry * FreeList;
        int Allocated;
        SBlock * Next;
    };
    SBlock * Blocks;
    int TotalAllocated;
    int TotalBlocks;

public:
    ADynamicPool() {
        Blocks = nullptr;
        TotalAllocated = 0;
        TotalBlocks = 0;
    }

    void Free() {
        SBlock * next;
        for ( SBlock * block = Blocks ; block ; block = next ) {
            next = block->Next;

            GHeapMemory.HeapFree( block );
        }
        Blocks = nullptr;
        TotalAllocated = 0;
        TotalBlocks = 0;
    }

    void CleanupEmptyBlocks() {
        SBlock * prev = nullptr;
        SBlock * next;
        for ( SBlock * block = Blocks ; block ; block = next ) {
            next = block->Next;

            if ( block->Allocated == 0 && TotalBlocks > 1 ) { // keep at least one block allocated
                if ( prev ) {
                    prev->Next = next;
                } else {
                    Blocks = next;
                }
                GHeapMemory.HeapFree( block );
                TotalBlocks--;
            } else {
                prev = block;
            }
        }
    }

    SBlock * AllocateBlock() {
        int i;
        SBlock * block = ( SBlock * )GHeapMemory.HeapAlloc( sizeof( SBlock ), 1 );
        //memset( block->Pool, 0, sizeof( block->Pool ) );
        block->FreeList = block->Pool;
        for ( i = 0 ; i < MAX_BLOCK_SIZE - 1 ; i++ ) {
            block->FreeList[i].Next = &block->FreeList[ i + 1 ];
        }
        block->FreeList[i].Next = NULL;
        block->Allocated = 0;
        block->Next = Blocks;
        Blocks = block;
        TotalBlocks++;
        GLogger.Printf( "ADynamicPool::AllocateBlock: allocated a new block\n" );
        return block;
    }

    T * Allocate() {
        Entry * p;
        SBlock * freeBlock = nullptr;

        for ( SBlock * block = Blocks ; block ; block = block->Next ) {
            if ( block->FreeList ) {
                freeBlock = block;
                break;
            }
        }

        if ( !freeBlock ) {
            freeBlock = AllocateBlock();
        }

        p = freeBlock->FreeList;
        freeBlock->FreeList = freeBlock->FreeList->Next;
        ++freeBlock->Allocated;
        ++TotalAllocated;
        GLogger.Printf( "ADynamicPool::Allocate: total blocks %d total %d\n", TotalBlocks, TotalAllocated );
        return p;
    }

    void Free( T * InPtr ) {
        Entry * p = static_cast< Entry * >( InPtr );
        for ( SBlock * block = Blocks ; block ; block = block->Next ) {
            // Find block
            if ( p >= &block->Pool[0] && p < &block->Pool[MAX_BLOCK_SIZE] ) {
                // free
                p->Next = block->FreeList;
                block->FreeList = p;
                --block->Allocated;
                --TotalAllocated;
                return;
            }
        }
    }
};

template< typename T, int MAX_BLOCK_SIZE = 1024 >
class ADynamicPool {
    struct SBlock {
        T Pool[MAX_BLOCK_SIZE];
        T * NextFree[MAX_BLOCK_SIZE];
        T * FreeList;
        int Allocated;
        SBlock * Next;
    };
    SBlock * Blocks;
    int TotalAllocated;
    int TotalBlocks;

public:
    ADynamicPool() {
        Blocks = nullptr;
        TotalAllocated = 0;
        TotalBlocks = 0;
    }

    void Free() {
        SBlock * next;
        for ( SBlock * block = Blocks ; block ; block = next ) {
            next = block->Next;

            GHeapMemory.HeapFree( block );
        }
        Blocks = nullptr;
        TotalAllocated = 0;
        TotalBlocks = 0;
    }

    void CleanupEmptyBlocks() {
        SBlock * prev = nullptr;
        SBlock * next;
        for ( SBlock * block = Blocks ; block ; block = next ) {
            next = block->Next;

            if ( block->Allocated == 0 && TotalBlocks > 1 ) { // keep at least one block allocated
                if ( prev ) {
                    prev->Next = next;
                } else {
                    Blocks = next;
                }
                GHeapMemory.HeapFree( block );
                TotalBlocks--;
            } else {
                prev = block;
            }
        }
    }

    SBlock * AllocateBlock() {
        int i;
        SBlock * block = ( SBlock * )GHeapMemory.HeapAlloc( sizeof( SBlock ), 1 );
        //memset( block->Pool, 0, sizeof( block->Pool ) );
        block->FreeList = block->Pool;
        for ( i = 0 ; i < MAX_BLOCK_SIZE - 1 ; i++ ) {
            block->NextFree[i] = &block->FreeList[ i + 1 ];
        }
        block->NextFree[i] = NULL;
        block->Allocated = 0;
        block->Next = Blocks;
        Blocks = block;
        TotalBlocks++;
        GLogger.Printf( "ADynamicPool::AllocateBlock: allocated a new block\n" );
        return block;
    }

    T * Allocate() {
        T * p;
        SBlock * freeBlock = nullptr;

        for ( SBlock * block = Blocks ; block ; block = block->Next ) {
            if ( block->FreeList ) {
                freeBlock = block;
                break;
            }
        }

        if ( !freeBlock ) {
            freeBlock = AllocateBlock();
        }

        p = freeBlock->FreeList;
        freeBlock->FreeList = freeBlock->FreeList->Next;
        ++freeBlock->Allocated;
        ++TotalAllocated;
        GLogger.Printf( "ADynamicPool::Allocate: total blocks %d total %d\n", TotalBlocks, TotalAllocated );
        return p;
    }

    void Free( T * InPtr ) {
        for ( SBlock * block = Blocks ; block ; block = block->Next ) {
            // Find block
            if ( InPtr >= &block->Pool[0] && InPtr < &block->Pool[MAX_BLOCK_SIZE] ) {
                // free
                int i = InPtr - &block->Pool[0];
                block->NextFree[i] = block->FreeList;
                block->FreeList = InPtr;
                --block->Allocated;
                --TotalAllocated;
                return;
            }
        }
    }
};
#endif
