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
#include <Runtime/Public/RuntimeVariable.h>

ARuntimeVariable com_DrawLevelAreaBounds( _CTS( "com_DrawLevelAreaBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_DrawLevelIndoorBounds( _CTS( "com_DrawLevelIndoorBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_DrawLevelPortals( _CTS( "com_DrawLevelPortals" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( ALevel )

ALevel::APrimitiveLinkPool ALevel::PrimitiveLinkPool;

ALevel::ALevel() {
    ViewCluster = -1;

    Float3 extents( CONVEX_HULL_MAX_BOUNDS * 2 );

    Core::ZeroMem( &OutdoorArea, sizeof( OutdoorArea ) );

    OutdoorArea.Bounds.Mins = -extents * 0.5f;
    OutdoorArea.Bounds.Maxs = extents * 0.5f;

    IndoorBounds.Clear();
}

ALevel::~ALevel() {
    Purge();
}

void ALevel::OnAddLevelToWorld() {
}

void ALevel::OnRemoveLevelFromWorld() {
    DestroyActors();
}

void ALevel::Initialize() {
    // Calc indoor bounding box
    IndoorBounds.Clear();
    for ( SVisArea & area : Areas ) {
        IndoorBounds.AddAABB( area.Bounds );
    }

    ViewMark = 0;
    ViewCluster = -1;

    if ( bCompressedVisData && Visdata && PVSClustersCount > 0 )
    {
        // Allocate decompressed vis data
        DecompressedVisData = (byte *)GHeapMemory.Alloc( ( PVSClustersCount + 7 ) >> 3 );
    }

    // FIXME: Use AVertexMemoryGPU?

    RenderCore::SBufferDesc bufferCI = {};
    bufferCI.MutableClientAccess = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    bufferCI.MutableUsage = RenderCore::MUTABLE_STORAGE_STATIC;

    bufferCI.SizeInBytes = ShadowCasterVerts.Size() * sizeof( Float3 );
    GRuntime->GetRenderDevice()->CreateBuffer( bufferCI, ShadowCasterVerts.ToPtr(), &ShadowCasterVB );
    ShadowCasterVB->SetDebugName( "ShadowCasterVB" );

    bufferCI.SizeInBytes = ShadowCasterIndices.Size() * sizeof( unsigned int );
    GRuntime->GetRenderDevice()->CreateBuffer( bufferCI, ShadowCasterIndices.ToPtr(), &ShadowCasterIB );
    ShadowCasterIB->SetDebugName( "ShadowCasterIB" );

    bufferCI.SizeInBytes = LightPortalVertexBuffer.Size() * sizeof( Float3 );
    GRuntime->GetRenderDevice()->CreateBuffer( bufferCI, LightPortalVertexBuffer.ToPtr(), &LightPortalsVB );
    ShadowCasterIB->SetDebugName( "LightPortalVertexBuffer" );

    bufferCI.SizeInBytes = LightPortalIndexBuffer.Size() * sizeof( unsigned int );
    GRuntime->GetRenderDevice()->CreateBuffer( bufferCI, LightPortalIndexBuffer.ToPtr(), &LightPortalsIB );
    ShadowCasterIB->SetDebugName( "LightPortalIndexBuffer" );
}

void ALevel::Purge() {
    DestroyActors();

    RemovePrimitives();

    RemoveLightmapUVChannels();
    RemoveVertexLightChannels();

    Areas.Free();

    PurgePortals();

    Nodes.Free();

    Leafs.Free();

    SplitPlanes.Free();

    IndoorBounds.Clear();

    PVSClustersCount = 0;

    GHeapMemory.Free( Visdata );
    Visdata = nullptr;

    GHeapMemory.Free( DecompressedVisData );
    DecompressedVisData = nullptr;

    GHeapMemory.Free( LightData );
    LightData = nullptr;

    AreaSurfaces.Free();

    Model.Reset();

    AudioAreas.Free();
    AmbientSounds.Free();

    Lightmaps.Free();

    ShadowCasterVerts.Free();
    ShadowCasterIndices.Free();

    LightPortalVertexBuffer.Free();
    LightPortalIndexBuffer.Free();

    // FIXME: free GPU buffers?
}

void ALevel::DestroyActors() {
    while ( !Actors.IsEmpty() ) {
        AActor * actor = Actors.Last();
        actor->Destroy();
    }
}

void ALevel::PurgePortals() {
    for ( SPortalLink & portalLink : AreaLinks ) {
        portalLink.Hull->Destroy();
    }

    // Clear area portals
    for ( SVisArea & area : Areas ) {
        area.PortalList = nullptr;
    }

    AreaLinks.Free();
    Portals.Free();
}

void ALevel::CreatePortals( SPortalDef const * InPortals, int InPortalsCount, Float3 const * InHullVertices ) {
    AConvexHull * hull;
    PlaneF hullPlane;
    SPortalLink * portalLink;
    int portalLinkNum;

    PurgePortals();

    Portals.ResizeInvalidate( InPortalsCount );
    AreaLinks.ResizeInvalidate( Portals.Size() << 1 );

    portalLinkNum = 0;

    for ( int i = 0 ; i < InPortalsCount ; i++ ) {
        SPortalDef const * def = InPortals + i;
        SVisPortal & portal = Portals[i];

        SVisArea * a1 = def->Areas[0] >= 0 ? &Areas[def->Areas[0]] : &OutdoorArea;
        SVisArea * a2 = def->Areas[1] >= 0 ? &Areas[def->Areas[1]] : &OutdoorArea;
#if 0
        if ( a1 == &OutdoorArea ) {
            StdSwap( a1, a2 );
        }

        // Check area position relative to portal plane
        float d = portal.Plane.Dist( a1->ReferencePoint );

        // If area position is on back side of plane, then reverse hull vertices and plane
        int id = d < 0.0f;
#else
        int id = 0;
#endif

        hull = AConvexHull::CreateFromPoints( InHullVertices + def->FirstVert, def->NumVerts );
        hullPlane = hull->CalcPlane();

        portalLink = &AreaLinks[ portalLinkNum++ ];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a2;
        if ( id & 1 ) {
            portalLink->Hull = hull;
            portalLink->Plane = hullPlane;
        } else {
            portalLink->Hull = hull->Reversed();
            portalLink->Plane = -hullPlane;
        }
        portalLink->Next = a1->PortalList;
        portalLink->Portal = &portal;
        a1->PortalList = portalLink;

        id = ( id + 1 ) & 1;

        portalLink = &AreaLinks[ portalLinkNum++ ];
        portal.Portals[id] = portalLink;
        portalLink->ToArea = a1;
        if ( id & 1 ) {
            portalLink->Hull = hull;
            portalLink->Plane = hullPlane;
        } else {
            portalLink->Hull = hull->Reversed();
            portalLink->Plane = -hullPlane;
        }
        portalLink->Next = a2->PortalList;
        portalLink->Portal = &portal;
        a2->PortalList = portalLink;

        portal.bBlocked = false;
    }
}

void ALevel::CreateLightPortals( SLightPortalDef const * InPortals, int InPortalsCount, Float3 const * InMeshVertices, int InVertexCount, unsigned int const * InMeshIndices, int InIndexCount ) {
    LightPortals.Resize( InPortalsCount );
    Core::Memcpy( LightPortals.ToPtr(), InPortals, InPortalsCount * sizeof( LightPortals[0] ) );

    LightPortalVertexBuffer.Resize( InVertexCount );
    Core::Memcpy( LightPortalVertexBuffer.ToPtr(), InMeshVertices, InVertexCount * sizeof( LightPortalVertexBuffer[0] ) );

    LightPortalIndexBuffer.Resize( InIndexCount );
    Core::Memcpy( LightPortalIndexBuffer.ToPtr(), InMeshIndices, InIndexCount * sizeof( LightPortalIndexBuffer[0] ) );
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
        nodeIndex = node->ChildrenIdx[ d <= 0 ];

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
    if ( VisibilityMethod != LEVEL_VISIBILITY_PVS ) {
        GLogger.Printf( "ALevel::MarkLeafs: expect LEVEL_VISIBILITY_PVS\n" );
        return ViewMark;
    }

    if ( InViewLeaf < 0 ) {
        return ViewMark;
    }

    SBinarySpaceLeaf * pLeaf = &Leafs[InViewLeaf];

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
        //int cluster;
        for ( SBinarySpaceLeaf & leaf : Leafs ) {
            //cluster = leaf.PVSCluster;
            //if ( cluster < 0 || cluster >= PVSClustersCount ) {
            //    continue;
            //}

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
    UpdatePrimitiveLinks();
}

Float3 ALevel::SampleLight( int InLightmapBlock, Float2 const & InLighmapTexcoord ) const {
    if ( !LightData ) {
        return Float3( 1.0f );
    }

    AN_ASSERT( InLightmapBlock >= 0 && InLightmapBlock < Lightmaps.Size() );

    int numChannels = ( LightmapFormat == LIGHTMAP_GRAYSCALED_HALF ) ? 1 : 4;
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
        light[0] = light[1] = light[2] = Math::Bilerp( Math::HalfToFloat( src00[0] ), Math::HalfToFloat( src10[0] ), Math::HalfToFloat( src01[0] ), Math::HalfToFloat( src11[0] ), lerp );
        break;
    }
    case LIGHTMAP_BGR_HALF:
    {
        for ( int i = 0 ; i < 3 ; i++ ) {
            //light[2-i] = Math::HalfToFloat(src00[i]);
            light[2-i] = Math::Bilerp( Math::HalfToFloat(src00[i]), Math::HalfToFloat(src10[i]), Math::HalfToFloat(src01[i]), Math::HalfToFloat(src11[i]), lerp );
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

    //hull->Destroy();

#if 0
    TPodVector< BvAxisAlignedBox > clusters;
    clusters.Resize( PVSClustersCount );
    for ( BvAxisAlignedBox & box : clusters ) {
        box.Clear();
    }
    for ( SBinarySpaceLeaf const & leaf : Leafs ) {
        if ( leaf.PVSCluster >= 0 && leaf.PVSCluster < PVSClustersCount ) {
            clusters[leaf.PVSCluster].AddAABB( leaf.Bounds );
        }
    }

    for ( BvAxisAlignedBox const & box : clusters ) {
        InRenderer->DrawAABB( box );
    }
    //GLogger.Printf( "leafs %d clusters %d\n", Leafs.Size(), clusters.Size() );
#endif

    // Draw light portals
#if 0
    InRenderer->DrawTriangleSoup( LightPortalVertexBuffer.ToPtr(), LightPortalVertexBuffer.Size(),
                                  sizeof(Float3), LightPortalIndexBuffer.ToPtr(), LightPortalIndexBuffer.Size() );
#endif

    if ( com_DrawLevelAreaBounds ) {
        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( AColor4( 0,1,0,0.5f) );
        for ( SVisArea & area : Areas ) {
            InRenderer->DrawAABB( area.Bounds );
        }
    }

    if ( com_DrawLevelPortals ) {
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

    if ( com_DrawLevelIndoorBounds ) {
        InRenderer->SetDepthTest( false );
        InRenderer->DrawAABB( IndoorBounds );
    }
}

void ALevel::QueryOverplapAreas_r( int InNodeIndex, BvAxisAlignedBox const & InBounds, TPodVector< SVisArea * > & OutAreas ) {
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

void ALevel::QueryOverplapAreas_r( int InNodeIndex, BvSphere const & InBounds, TPodVector< SVisArea * > & OutAreas ) {
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

void ALevel::QueryOverplapAreas( BvAxisAlignedBox const & InBounds, TPodVector< SVisArea * > & OutAreas ) {
    OutAreas.Clear();

    if ( Nodes.IsEmpty() ) {
        return;
    }

    QueryOverplapAreas_r( 0, InBounds, OutAreas );
}

void ALevel::QueryOverplapAreas( BvSphere const & InBounds, TPodVector< SVisArea * > & OutAreas ) {
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

void ALevel::AddPrimitiveToArea( SVisArea * Area, SPrimitiveDef * Primitive ) {
    if ( IsPrimitiveInArea( Primitive, Area ) ) {
        return;
    }

    SPrimitiveLink * link = PrimitiveLinkPool.Allocate();
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

void ALevel::LinkPrimitive( SPrimitiveDef * InPrimitive ) {

    LastLink = &InPrimitive->Links;

    if ( InPrimitive->bPendingRemove ) {
        return;
    }

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

void ALevel::UnlinkPrimitive( SPrimitiveDef * InPrimitive ) {
    SPrimitiveLink * link = InPrimitive->Links;

    while ( link ) {
        AN_ASSERT( link->Area );

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

        PrimitiveLinkPool.Deallocate( free );
    }

    InPrimitive->Links = nullptr;
}

ALightmapUV * ALevel::CreateLightmapUVChannel( AIndexedMesh * InSourceMesh ) {
    ALightmapUV * lightmapUV = CreateInstanceOf< ALightmapUV >();
    lightmapUV->AddRef();
    lightmapUV->Initialize( InSourceMesh, this );
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
    AVertexLight * vertexLight = CreateInstanceOf< AVertexLight >();
    vertexLight->AddRef();
    vertexLight->Initialize( InSourceMesh, this );
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

void ALevel::UpdatePrimitiveLinks() {
    SPrimitiveDef * next;

    // First Pass: remove primitives from the areas
    for ( SPrimitiveDef * primitive = PrimitiveUpdateList ; primitive ; primitive = primitive->NextUpd )
    {
        UnlinkPrimitive( primitive );
    }

    // Second Pass: add primitive to the areas
    for ( SPrimitiveDef * primitive = PrimitiveUpdateList ; primitive ; primitive = next )
    {
        LinkPrimitive( primitive );

        next = primitive->NextUpd;
        primitive->PrevUpd = primitive->NextUpd = nullptr;
    }

    PrimitiveUpdateList = PrimitiveUpdateListTail = nullptr;
}

void ALevel::MarkPrimitives() {
    for ( SPrimitiveDef * primitive = PrimitiveList ; primitive ; primitive = primitive->Next ) {
        MarkPrimitive( primitive );
    }
}

void ALevel::UnmarkPrimitives() {
    SPrimitiveDef * next;
    for ( SPrimitiveDef * primitive = PrimitiveUpdateList ; primitive ; primitive = next )
    {
        next = primitive->NextUpd;
        primitive->PrevUpd = primitive->NextUpd = nullptr;
    }
    PrimitiveUpdateList = PrimitiveUpdateListTail = nullptr;
}

void ALevel::RemovePrimitives() {
#if 1
    SPrimitiveDef * next;

    for ( SPrimitiveDef * primitive = PrimitiveUpdateList ; primitive ; primitive = next )
    {
        UnlinkPrimitive( primitive );

        next = primitive->NextUpd;
        primitive->PrevUpd = primitive->NextUpd = nullptr;
    }

    PrimitiveUpdateList = PrimitiveUpdateListTail = nullptr;
#else
    UnmarkPrimitives();

    for ( SPrimitiveDef * primitive = PrimitiveList ; primitive ; primitive = primitive->Next ) {
        UnlinkPrimitive( primitive );
    }
#endif
}

void ALevel::AddPrimitive( SPrimitiveDef * InPrimitive ) {
    INTRUSIVE_ADD_UNIQUE( InPrimitive, Next, Prev, PrimitiveList, PrimitiveListTail );

    InPrimitive->bPendingRemove = false;

    MarkPrimitive( InPrimitive );
}

void ALevel::RemovePrimitive( SPrimitiveDef * InPrimitive ) {
    INTRUSIVE_REMOVE( InPrimitive, Next, Prev, PrimitiveList, PrimitiveListTail );

    InPrimitive->bPendingRemove = true;

    MarkPrimitive( InPrimitive );
}

void ALevel::MarkPrimitive( SPrimitiveDef * InPrimitive ) {
    INTRUSIVE_ADD_UNIQUE( InPrimitive, NextUpd, PrevUpd, PrimitiveUpdateList, PrimitiveUpdateListTail );
}

AN_CLASS_META( ABrushModel )

void ABrushModel::Purge() {
    Surfaces.Free();

    Vertices.Free();

    LightmapVerts.Free();

    VertexLight.Free();

    Indices.Free();

    SurfaceMaterials.Free();

    //CollisionModel.Reset();
}
