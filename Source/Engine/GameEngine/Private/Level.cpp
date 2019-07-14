/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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


#include <Engine/GameEngine/Public/Level.h>
#include <Engine/GameEngine/Public/Actor.h>
#include <Engine/GameEngine/Public/Texture.h>
#include <Engine/GameEngine/Public/World.h>
#include <Engine/GameEngine/Public/MeshComponent.h>
#include <Engine/Core/Public/BV/BvIntersect.h>

AN_CLASS_META_NO_ATTRIBS( FLevel )
AN_CLASS_META_NO_ATTRIBS( FLevelArea )
AN_CLASS_META_NO_ATTRIBS( FLevelPortal )

FLevel::FLevel() {
    IndoorBounds.Clear();

    OutdoorArea = NewObject< FLevelArea >();
    OutdoorArea->Extents = Float3( CONVEX_HULL_MAX_BOUNDS * 2 );
    OutdoorArea->ParentLevel = this;
    OutdoorArea->Bounds.Mins = -OutdoorArea->Extents * 0.5f;
    OutdoorArea->Bounds.Maxs = OutdoorArea->Extents * 0.5f;

    OutdoorArea->Tree = NewObject< FOctree >();
    OutdoorArea->Tree->Owner = OutdoorArea;
    OutdoorArea->Tree->Build();

    NavigationBoundingBox.Mins = Float3(-512);
    NavigationBoundingBox.Maxs = Float3(512);
}

FLevel::~FLevel() {
    ClearLightmaps();

    DeallocateBufferData( LightData );

    DestroyActors();

    DestroyPortalTree();

//    BSP.Planes.Free();
//    BSP.Nodes.Free();
//    BSP.Leafs.Free();
//    BSP.Surfaces.Free();
//    BSP.Marksurfaces.Free();
//    BSP.Vertices.Free();
//    BSP.Indices.Free();
}

void FLevel::SetLightData( const byte * _Data, int _Size ) {
    DeallocateBufferData( LightData );
    LightData = (byte *)AllocateBufferData( _Size );
    memcpy( LightData, _Data, _Size );
}

void FLevel::ClearLightmaps() {
    for ( FTexture * lightmap : Lightmaps ) {
        lightmap->RemoveRef();
    }
    Lightmaps.Free();
}

void FLevel::DestroyActors() {
    for ( FActor * actor : Actors ) {
        actor->Destroy();
    }
}

void FLevel::OnAddLevelToWorld() {
    RemoveSurfaces();
    AddSurfaces();
}

void FLevel::OnRemoveLevelFromWorld() {
    RemoveSurfaces();
}

FLevelArea * FLevel::CreateArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint ) {

    FLevelArea * area = NewObject< FLevelArea >();
    area->AddRef();
    area->Position = _Position;
    area->Extents = _Extents;
    area->ReferencePoint = _ReferencePoint;
    area->ParentLevel = this;

    Float3 halfExtents = area->Extents * 0.5f;
    for ( int i = 0 ; i < 3 ; i++ ) {
        area->Bounds.Mins[i] = area->Position[i] - halfExtents[i];
        area->Bounds.Maxs[i] = area->Position[i] + halfExtents[i];
    }

    area->Tree = NewObject< FOctree >();
    area->Tree->Owner = area;
    area->Tree->Build();

    Areas.Append( area );

    return area;
}

FLevelPortal * FLevel::CreatePortal( Float3 const * _HullPoints, int _NumHullPoints, FLevelArea * _Area1, FLevelArea * _Area2 ) {
    if ( _Area1 == _Area2 ) {
        return nullptr;
    }

    FLevelPortal * portal = NewObject< FLevelPortal >();
    portal->AddRef();
    portal->Hull = FConvexHull::CreateFromPoints( _HullPoints, _NumHullPoints );
    portal->Plane = portal->Hull->CalcPlane();
    portal->Area1 = _Area1 ? _Area1 : OutdoorArea;
    portal->Area2 = _Area2 ? _Area2 : OutdoorArea;
    portal->ParentLevel = this;
    Portals.Append( portal );
    return portal;
}

void FLevel::DestroyPortalTree() {
    PurgePortals();

    for ( FLevelArea * area : Areas ) {
        area->RemoveRef();
    }

    Areas.Clear();

    for ( FLevelPortal * portal : Portals ) {
        portal->RemoveRef();
    }

    Portals.Clear();

    IndoorBounds.Clear();
}

void FLevel::AddSurfaces() {
    FWorld * world = GetOwnerWorld();

    for ( FMeshComponent * mesh = world->GetMeshList() ; mesh ; mesh = mesh->GetNextMesh() ) {

        // TODO: if ( !mesh->IsMarkedDirtyArea() )
        AddSurfaceAreas( mesh );
    }
}

void FLevel::RemoveSurfaces() {
    for ( FLevelArea * area : Areas ) {
        while ( !area->Movables.IsEmpty() ) {
            RemoveSurfaceAreas( area->Movables[0] );
        }
    }

    while ( !OutdoorArea->Movables.IsEmpty() ) {
        RemoveSurfaceAreas( OutdoorArea->Movables[0] );
    }
}

void FLevel::PurgePortals() {
    RemoveSurfaces();

    for ( FAreaPortal & areaPortal : AreaPortals ) {
        FConvexHull::Destroy( areaPortal.Hull );
    }

    AreaPortals.Clear();
}

void FLevel::BuildPortals() {

    PurgePortals();

    IndoorBounds.Clear();

//    Float3 halfExtents;
    for ( FLevelArea * area : Areas ) {
//        // Update area bounds
//        halfExtents = area->Extents * 0.5f;
//        for ( int i = 0 ; i < 3 ; i++ ) {
//            area->Bounds.Mins[i] = area->Position[i] - halfExtents[i];
//            area->Bounds.Maxs[i] = area->Position[i] + halfExtents[i];
//        }

        IndoorBounds.AddAABB( area->Bounds );

        // Clear area portals
        area->PortalList = NULL;
    }

    AreaPortals.ResizeInvalidate( Portals.Length() << 1 );

    int areaPortalId = 0;

    for ( FLevelPortal * portal : Portals ) {
        FLevelArea * a1 = portal->Area1;
        FLevelArea * a2 = portal->Area2;

        if ( a1 == OutdoorArea ) {
            std::swap( a1, a2 );
        }

        // Check area position relative to portal plane
        EPlaneSide offset = portal->Plane.SideOffset( a1->ReferencePoint, 0.0f );

        // If area position is on back side of plane, then reverse hull vertices and plane
        int id = offset == EPlaneSide::Back ? 1 : 0;

        FAreaPortal * areaPortal;

        areaPortal = &AreaPortals[ areaPortalId++ ];
        portal->Portals[id] = areaPortal;
        areaPortal->ToArea = a2;
        if ( id & 1 ) {
            areaPortal->Hull = portal->Hull->Reversed();
            areaPortal->Plane = -portal->Plane;
        } else {
            areaPortal->Hull = portal->Hull->Duplicate();
            areaPortal->Plane = portal->Plane;
        }
        areaPortal->Next = a1->PortalList;
        areaPortal->Owner = portal;
        a1->PortalList = areaPortal;

        id = ( id + 1 ) & 1;

        areaPortal = &AreaPortals[ areaPortalId++ ];
        portal->Portals[id] = areaPortal;
        areaPortal->ToArea = a1;
        if ( id & 1 ) {
            areaPortal->Hull = portal->Hull->Reversed();
            areaPortal->Plane = -portal->Plane;
        } else {
            areaPortal->Hull = portal->Hull->Duplicate();
            areaPortal->Plane = portal->Plane;
        }
        areaPortal->Next = a2->PortalList;
        areaPortal->Owner = portal;
        a2->PortalList = areaPortal;
    }

    AddSurfaces();
}

void FLevel::AddSurfaceToArea( int _AreaNum, FSpatialObject * _Surf ) {
    FLevelArea * area = _AreaNum >= 0 ? Areas[_AreaNum] : OutdoorArea;

    area->Movables.Append( _Surf );
    FAreaLink & areaLink = _Surf->InArea.Append();
    areaLink.AreaNum = _AreaNum;
    areaLink.Index = area->Movables.Length() - 1;
    areaLink.Level = this;
}

void FLevel::AddSurfaceAreas( FSpatialObject * _Surf ) {
    BvAxisAlignedBox const & bounds = _Surf->GetWorldBounds();
    int numAreas = Areas.Length();
    FLevelArea * area;

    if ( _Surf->IsOutdoor() ) {
        // add to outdoor
        AddSurfaceToArea( -1, _Surf );
        return;
    }

    bool bHaveIntersection = false;
    if ( FMath::Intersects( IndoorBounds, bounds ) ) {
        // TODO: optimize it!
        for ( int i = 0 ; i < numAreas ; i++ ) {
            area = Areas[i];

            if ( FMath::Intersects( area->Bounds, bounds ) ) {
                AddSurfaceToArea( i, _Surf );

                bHaveIntersection = true;
            }
        }
    }

    if ( !bHaveIntersection ) {
        AddSurfaceToArea( -1, _Surf );
    }
}

void FLevel::RemoveSurfaceAreas( FSpatialObject * _Surf ) {
    FLevelArea * area;

    // Remove renderables from any areas
    for ( int i = 0 ; i < _Surf->InArea.Length() ; ) {
        FAreaLink & InArea = _Surf->InArea[ i ];

        if ( InArea.Level != this ) {
            i++;
            continue;
        }

        AN_Assert( InArea.AreaNum < InArea.Level->Areas.Length() );
        area = InArea.AreaNum >= 0 ? InArea.Level->Areas[ InArea.AreaNum ] : OutdoorArea;

        AN_Assert( area->Movables[ InArea.Index ] == _Surf );

        // Swap with last array element
        area->Movables.RemoveSwap( InArea.Index );

        // Update swapped movable index
        if ( InArea.Index < area->Movables.Length() ) {
            FSpatialObject * surf = area->Movables[ InArea.Index ];
            for ( int j = 0 ; j < surf->InArea.Length() ; j++ ) {
                if ( surf->InArea[ j ].Level == this && surf->InArea[ j ].AreaNum == InArea.AreaNum ) {
                    surf->InArea[ j ].Index = InArea.Index;

                    AN_Assert( area->Movables[ surf->InArea[ j ].Index ] == surf );
                    break;
                }
            }
        }

        _Surf->InArea.RemoveSwap(i);
    }
}

void FLevel::DrawDebug( FDebugDraw * _DebugDraw ) {

#if 0
    static TPodArray< Float3 > vertices;
    static TPodArray< unsigned int > indices;
    static BvAxisAlignedBox boundingBox;
    static TBitMask<> walkableMask;
    static bool gen=false;
    if (!gen) {
        GenerateSourceNavMesh( vertices, indices, walkableMask, boundingBox, bOverrideNavigationBoundingBox ? &NavigationBoundingBox : nullptr );
        gen=true;
    }

    _DebugDraw->SetDepthTest(true);
    _DebugDraw->DrawTriangleSoup(vertices.ToPtr(),vertices.Length(),sizeof(Float3),indices.ToPtr(),indices.Length(),false);
    GLogger.Printf( "indices %d\n",indices.Length());
#endif

    NavMesh.DrawDebug( _DebugDraw );

    if ( GDebugDrawFlags.bDrawLevelAreaBounds ) {
        _DebugDraw->SetDepthTest( false );

        int i = 0;
        for ( FLevelArea * area : Areas ) {
            //_DebugDraw->DrawAABB( area->Bounds );

            i++;

            float f = (float)( (i*12345) & 255 ) / 255.0f;

            _DebugDraw->SetColor(f,f,f,0.5f);

            _DebugDraw->DrawBoxFilled( area->Bounds.Center(), area->Bounds.HalfSize(), true );
        }

        _DebugDraw->SetDepthTest( false );
        _DebugDraw->SetColor(0,1,0,0.5f);
        for ( FLevelArea * area : Areas ) {
            _DebugDraw->DrawAABB( area->Bounds );
        }

    }

    if ( GDebugDrawFlags.bDrawLevelPortals ) {
//        _DebugDraw->SetDepthTest( false );
//        _DebugDraw->SetColor(1,0,0,1);
//        for ( FLevelPortal * portal : Portals ) {
//            _DebugDraw->DrawLine( portal->Hull->Points, portal->Hull->NumPoints, true );
//        }

        _DebugDraw->SetDepthTest( false );
        _DebugDraw->SetColor(1,0,0,0.4f);
        for ( FLevelPortal * portal : Portals ) {
            _DebugDraw->DrawConvexPoly( portal->Hull->Points, portal->Hull->NumPoints, true );
        }
    }

    if ( GDebugDrawFlags.bDrawLevelIndoorBounds ) {
        _DebugDraw->SetDepthTest( false );
        _DebugDraw->DrawAABB( IndoorBounds );
    }
}

int FLevel::FindArea( Float3 const & _Position ) {
    // TODO: ... binary tree?

    if ( Areas.Length() == 0 ) {
        return -1;
    }

    for ( int i = 0 ; i < Areas.Length() ; i++ ) {
        if (    _Position.X >= Areas[i]->Bounds.Mins.X
             && _Position.Y >= Areas[i]->Bounds.Mins.Y
             && _Position.Z >= Areas[i]->Bounds.Mins.Z
             && _Position.X <  Areas[i]->Bounds.Maxs.X
             && _Position.Y <  Areas[i]->Bounds.Maxs.Y
             && _Position.Z <  Areas[i]->Bounds.Maxs.Z ) {
            return i;
        }
    }

    return -1;
}

bool CalcAABBIntersection( BvAxisAlignedBox const & _A, BvAxisAlignedBox const & _B, BvAxisAlignedBox & _Intersection ) {
    const float x_min = FMath::Max( _A.Mins[0], _B.Mins[0] );
    const float x_max = FMath::Min( _A.Maxs[0], _B.Maxs[0] );
    if ( x_max <= x_min ) {
        return false;
    }

    const float y_min = FMath::Max( _A.Mins[1], _B.Mins[1] );
    const float y_max = FMath::Min( _A.Maxs[1], _B.Maxs[1] );
    if ( y_max <= y_min ) {
        return false;
    }

    const float z_min = FMath::Max( _A.Mins[2], _B.Mins[2] );
    const float z_max = FMath::Min( _A.Maxs[2], _B.Maxs[2] );
    if ( z_max <= z_min ) {
        return false;
    }

    _Intersection.Mins[0] = x_min;
    _Intersection.Mins[1] = y_min;
    _Intersection.Mins[2] = z_min;

    _Intersection.Maxs[0] = x_max;
    _Intersection.Maxs[1] = y_max;
    _Intersection.Maxs[2] = z_max;

    return true;
}

bool IsBoundingBoxOverlapTriangle( BvAxisAlignedBox const & _BoundingBox, Float3 const & v0, Float3 const & v1, Float3 const & v2 ) {

    // Simple fast triangle AABB intersection

    BvAxisAlignedBox triangleBounds;
    triangleBounds.Clear();
    triangleBounds.AddPoint( v0 );
    triangleBounds.AddPoint( v1 );
    triangleBounds.AddPoint( v2 );

    return FMath::Intersects( _BoundingBox, triangleBounds );
}

void FLevel::GenerateSourceNavMesh( TPodArray< Float3 > & _Vertices,
                                    TPodArray< unsigned int > & _Indices,
                                    TBitMask<> & _WalkableTriangles,
                                    BvAxisAlignedBox & _ResultBoundingBox,
                                    BvAxisAlignedBox const * _ClipBoundingBox ) {
    BvAxisAlignedBox clippedBounds;
    TPodArray< Float3 > collisionVertices;
    TPodArray< unsigned int > collisionIndices;
    BvAxisAlignedBox worldBounds;

    _Vertices.Clear();
    _Indices.Clear();

//    if ( _ClipBoundingBox ) {
//        _ResultBoundingBox = *_ClipBoundingBox;
//    } else {
        _ResultBoundingBox.Clear();
//    }

    for ( FActor * actor : Actors ) {

        if ( actor->IsPendingKill() ) {
            continue;
        }

        FArrayOfActorComponents const & components = actor->GetComponents();

        for ( FActorComponent * component : components ) {

            if ( component->IsPendingKill() ) {
                continue;
            }

            FPhysicalBody * physBody = Upcast< FPhysicalBody >( component );
            if ( !physBody ) {
                continue;
            }

            if ( !physBody->bAINavigation ) {
                // Not used for AI navigation
                continue;
            }

            if ( physBody->PhysicsBehavior != PB_STATIC ) {
                // Generate navmesh only for static geometry
                continue;
            }

            physBody->GetCollisionWorldBounds( worldBounds );
            if ( worldBounds.IsEmpty() ) {
                continue;
            }

            if ( _ClipBoundingBox ) {
                if ( !CalcAABBIntersection( worldBounds, *_ClipBoundingBox, clippedBounds ) ) {
                    continue;
                }
                _ResultBoundingBox.AddAABB( clippedBounds );
            } else {
                _ResultBoundingBox.AddAABB( worldBounds );
            }

            collisionVertices.Clear();
            collisionIndices.Clear();

            physBody->CreateCollisionModel( collisionVertices, collisionIndices );

            if ( collisionIndices.IsEmpty() ) {

                // Try to get from mesh
                FMeshComponent * mesh = Upcast< FMeshComponent >( component );

                if ( mesh && !mesh->IsSkinnedMesh() ) {

                    FIndexedMesh * indexedMesh = mesh->GetMesh();

                    if ( indexedMesh && !indexedMesh->IsSkinned() ) {

                        Float3x4 const & worldTransform = mesh->GetWorldTransformMatrix();

                        FMeshVertex const * srcVertices = indexedMesh->GetVertices();
                        unsigned int * srcIndices = indexedMesh->GetIndices();

                        int firstVertex = _Vertices.Length();
                        int firstIndex = _Indices.Length();
                        int firstTriangle = _Indices.Length() / 3;

                        // indexCount may be different from indexedMesh->GetIndexCount()
                        int indexCount = 0;
                        for ( FIndexedMeshSubpart const * subpart : indexedMesh->GetSubparts() ) {
                            indexCount += subpart->IndexCount;
                        }

                        _Vertices.Resize( firstVertex + indexedMesh->GetVertexCount() );
                        _Indices.Resize( firstIndex + indexCount );
                        _WalkableTriangles.Resize( firstTriangle + indexCount / 3 );

                        Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
                        unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

                        for ( int i = 0 ; i < indexedMesh->GetVertexCount() ; i++ ) {
                            pVertices[i] = worldTransform * srcVertices[i].Position;

                            //pVertices[i].Y += 50;
                        }

                        if ( _ClipBoundingBox ) {
                            // Clip triangles
                            unsigned int i0, i1, i2;
                            int triangleNum = 0;
                            for ( FIndexedMeshSubpart const * subpart : indexedMesh->GetSubparts() ) {
                                int numTriangles = subpart->IndexCount / 3;
                                for ( int i = 0 ; i < numTriangles ; i++ ) {
                                    i0 = firstVertex + subpart->BaseVertex + srcIndices[ subpart->FirstIndex + i*3 + 0 ];
                                    i1 = firstVertex + subpart->BaseVertex + srcIndices[ subpart->FirstIndex + i*3 + 1 ];
                                    i2 = firstVertex + subpart->BaseVertex + srcIndices[ subpart->FirstIndex + i*3 + 2 ];

                                    if ( IsBoundingBoxOverlapTriangle( clippedBounds, pVertices[i0], pVertices[i1], pVertices[i2] ) ) {
                                        *pIndices++ = i0;
                                        *pIndices++ = i1;
                                        *pIndices++ = i2;

                                        if ( !physBody->bAINonWalkable ) {
                                            _WalkableTriangles.Mark( firstTriangle + triangleNum );
                                        }
                                        triangleNum++;
                                    }
                                }
                            }

                            _Indices.Resize( firstIndex + triangleNum*3 );
                            _WalkableTriangles.Resize( firstTriangle + triangleNum );

                        } else {
                            int triangleNum = 0;
                            for ( FIndexedMeshSubpart const * subpart : indexedMesh->GetSubparts() ) {
                                int numTriangles = subpart->IndexCount / 3;
                                for ( int i = 0 ; i < numTriangles ; i++ ) {
                                    *pIndices++ = firstVertex + subpart->BaseVertex + srcIndices[ subpart->FirstIndex + i*3 + 0 ];
                                    *pIndices++ = firstVertex + subpart->BaseVertex + srcIndices[ subpart->FirstIndex + i*3 + 1 ];
                                    *pIndices++ = firstVertex + subpart->BaseVertex + srcIndices[ subpart->FirstIndex + i*3 + 2 ];

                                    if ( !physBody->bAINonWalkable ) {
                                        _WalkableTriangles.Mark( firstTriangle + triangleNum );
                                    }
                                    triangleNum++;
                                }
                            }
                        }
                    }
                }
            } else {
                Float3 const * srcVertices = collisionVertices.ToPtr();
                unsigned int * srcIndices = collisionIndices.ToPtr();

                int firstVertex = _Vertices.Length();
                int firstIndex = _Indices.Length();
                int firstTriangle = _Indices.Length() / 3;
                int vertexCount = collisionVertices.Length();
                int indexCount = collisionIndices.Length();

                _Vertices.Resize( firstVertex + vertexCount );
                _Indices.Resize( firstIndex + indexCount );
                _WalkableTriangles.Resize( firstTriangle + indexCount / 3 );

                Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
                unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

                memcpy( pVertices, srcVertices, vertexCount * sizeof( Float3 ) );

                //for ( int i = 0 ; i < vertexCount ; i++ ) {
                //    pVertices[i].Y += 50;
                //}

                if ( _ClipBoundingBox ) {
                    // Clip triangles
                    unsigned int i0, i1, i2;
                    int numTriangles = indexCount / 3;
                    int triangleNum = 0;
                    for ( int i = 0 ; i < numTriangles ; i++ ) {
                        i0 = firstVertex + srcIndices[ i*3 + 0 ];
                        i1 = firstVertex + srcIndices[ i*3 + 1 ];
                        i2 = firstVertex + srcIndices[ i*3 + 2 ];

                        if ( IsBoundingBoxOverlapTriangle( clippedBounds, pVertices[i0], pVertices[i1], pVertices[i2] ) ) {
                            *pIndices++ = i0;
                            *pIndices++ = i1;
                            *pIndices++ = i2;

                            if ( !physBody->bAINonWalkable ) {
                                _WalkableTriangles.Mark( firstTriangle + triangleNum );
                            }
                            triangleNum++;
                        }
                    }

                    _Indices.Resize( firstIndex + triangleNum * 3 );
                    _WalkableTriangles.Resize( firstTriangle + triangleNum );

                } else {
                    int numTriangles = indexCount / 3;
                    for ( int i = 0 ; i < numTriangles ; i++ ) {
                        *pIndices++ = firstVertex + srcIndices[ i*3 + 0 ];
                        *pIndices++ = firstVertex + srcIndices[ i*3 + 1 ];
                        *pIndices++ = firstVertex + srcIndices[ i*3 + 2 ];

                        if ( !physBody->bAINonWalkable ) {
                            _WalkableTriangles.Mark( firstTriangle + i );
                        }
                    }
                }
            }
        }
    }

//    if ( !_ClipBoundingBox && NavigationBoundingBoxPadding > 0 ) {
//        // Apply bounding box padding
//        _ResultBoundingBox.Mins -= NavigationBoundingBoxPadding;
//        _ResultBoundingBox.Maxs += NavigationBoundingBoxPadding;
//    }
}

void FLevel::BuildNavMesh() {
//    TPodArray< Float3 > vertices;
//    TPodArray< unsigned int > indices;
//    BvAxisAlignedBox boundingBox;
//    TBitMask<> walkableMask;
//    GenerateSourceNavMesh( vertices, indices, walkableMask, boundingBox, nullptr );

    FAINavMeshInitial initial;
    initial.BoundingBox = NavigationBoundingBox;
    initial.bDynamicNavMesh = true;
    initial.NavWalkableClimb = 0.9f;
    initial.NavWalkableSlopeAngle = 80;
    //initial.NavTileSize = 128;

    NavMesh.Initialize( this, initial );
    NavMesh.Build();
}

void FLevel::Tick( float _TimeStep ) {
    NavMesh.Tick( _TimeStep );

    OutdoorArea->Tree->Update();
    for ( FLevelArea * area : Areas ) {
        area->Tree->Update();
    }
}
