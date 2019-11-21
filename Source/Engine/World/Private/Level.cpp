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


#include <World/Public/Level.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/World.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Components/CameraComponent.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Resource/Texture.h>
#include <Core/Public/BV/BvIntersect.h>
#include <Runtime/Public/Runtime.h>

#include "ShadowCascade.h"

ARuntimeVariable RVDrawLevelAreaBounds( _CTS( "DrawLevelAreaBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawLevelIndoorBounds( _CTS( "DrawLevelIndoorBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawLevelPortals( _CTS( "DrawLevelPortals" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( ALevel )
AN_CLASS_META( ALevelArea )
AN_CLASS_META( ALevelPortal )

ALevel::ALevel() {
    IndoorBounds.Clear();

    OutdoorArea = NewObject< ALevelArea >();
    OutdoorArea->Extents = Float3( CONVEX_HULL_MAX_BOUNDS * 2 );
    OutdoorArea->ParentLevel = this;
    OutdoorArea->Bounds.Mins = -OutdoorArea->Extents * 0.5f;
    OutdoorArea->Bounds.Maxs = OutdoorArea->Extents * 0.5f;

    OutdoorArea->Tree = NewObject< AOctree >();
    OutdoorArea->Tree->Owner = OutdoorArea;
    OutdoorArea->Tree->Build();

    LastVisitedArea = -1;
}

ALevel::~ALevel() {
    ClearLightmaps();

    HugeFree( LightData );

    DestroyActors();

    DestroyPortalTree();
}

void ALevel::SetLightData( const byte * _Data, int _Size ) {
    HugeFree( LightData );
    LightData = (byte *)HugeAlloc( _Size );
    memcpy( LightData, _Data, _Size );
}

void ALevel::ClearLightmaps() {
    for ( ATexture * lightmap : Lightmaps ) {
        lightmap->RemoveRef();
    }
    Lightmaps.Free();
}

void ALevel::DestroyActors() {
    for ( AActor * actor : Actors ) {
        actor->Destroy();
    }
}

void ALevel::OnAddLevelToWorld() {
    RemoveDrawables();
    AddDrawables();
}

void ALevel::OnRemoveLevelFromWorld() {
    RemoveDrawables();
}

ALevelArea * ALevel::AddArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint ) {

    ALevelArea * area = NewObject< ALevelArea >();
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

    area->Tree = NewObject< AOctree >();
    area->Tree->Owner = area;
    area->Tree->Build();

    Areas.Append( area );

    return area;
}

ALevelPortal * ALevel::AddPortal( Float3 const * _HullPoints, int _NumHullPoints, ALevelArea * _Area1, ALevelArea * _Area2 ) {
    if ( _Area1 == _Area2 ) {
        return nullptr;
    }

    ALevelPortal * portal = NewObject< ALevelPortal >();
    portal->AddRef();
    portal->Hull = AConvexHull::CreateFromPoints( _HullPoints, _NumHullPoints );
    portal->Plane = portal->Hull->CalcPlane();
    portal->Area1 = _Area1 ? _Area1 : OutdoorArea;
    portal->Area2 = _Area2 ? _Area2 : OutdoorArea;
    portal->ParentLevel = this;
    Portals.Append( portal );
    return portal;
}

void ALevel::DestroyPortalTree() {
    PurgePortals();

    for ( ALevelArea * area : Areas ) {
        area->RemoveRef();
    }

    Areas.Clear();

    for ( ALevelPortal * portal : Portals ) {
        portal->RemoveRef();
    }

    Portals.Clear();

    IndoorBounds.Clear();
}

void ALevel::AddDrawables() {
    AWorld * pWorld = GetOwnerWorld();
    ARenderWorld & RenderWorld = pWorld->GetRenderWorld();

    for ( ADrawable * drawable = RenderWorld.GetDrawables() ; drawable ; drawable = drawable->GetNextDrawable() ) {
        AddDrawable( drawable );
    }
}

void ALevel::RemoveDrawables() {
    for ( ALevelArea * area : Areas ) {
        while ( !area->Drawables.IsEmpty() ) {
            RemoveDrawable( area->Drawables[0] );
        }
    }

    while ( !OutdoorArea->Drawables.IsEmpty() ) {
        RemoveDrawable( OutdoorArea->Drawables[0] );
    }
}

void ALevel::PurgePortals() {
    RemoveDrawables();

    for ( SAreaPortal & areaPortal : AreaPortals ) {
        AConvexHull::Destroy( areaPortal.Hull );
    }

    AreaPortals.Clear();
}

void ALevel::BuildPortals() {

    PurgePortals();

    IndoorBounds.Clear();

//    Float3 halfExtents;
    for ( ALevelArea * area : Areas ) {
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

    AreaPortals.ResizeInvalidate( Portals.Size() << 1 );

    int areaPortalId = 0;

    for ( ALevelPortal * portal : Portals ) {
        ALevelArea * a1 = portal->Area1;
        ALevelArea * a2 = portal->Area2;

        if ( a1 == OutdoorArea ) {
            StdSwap( a1, a2 );
        }

        // Check area position relative to portal plane
        EPlaneSide offset = portal->Plane.SideOffset( a1->ReferencePoint, 0.0f );

        // If area position is on back side of plane, then reverse hull vertices and plane
        int id = offset == EPlaneSide::Back ? 1 : 0;

        SAreaPortal * areaPortal;

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

    AddDrawables();
}

void ALevel::AddDrawableToArea( int _AreaNum, ADrawable * _Drawable ) {
    ALevelArea * area = _AreaNum >= 0 ? Areas[_AreaNum] : OutdoorArea;

    area->Drawables.Append( _Drawable );
    SAreaLink & areaLink = _Drawable->InArea.Append();
    areaLink.AreaNum = _AreaNum;
    areaLink.Index = area->Drawables.Size() - 1;
    areaLink.Level = this;
}

void ALevel::AddDrawable( ADrawable * _Drawable ) {
    BvAxisAlignedBox const & bounds = _Drawable->GetWorldBounds();
    int numAreas = Areas.Size();
    ALevelArea * area;

    if ( _Drawable->IsOutdoor() ) {
        // add to outdoor
        AddDrawableToArea( -1, _Drawable );
        return;
    }

    bool bHaveIntersection = false;
    if ( BvBoxOverlapBox( IndoorBounds, bounds ) ) {
        // TODO: optimize it!
        for ( int i = 0 ; i < numAreas ; i++ ) {
            area = Areas[i];

            if ( BvBoxOverlapBox( area->Bounds, bounds ) ) {
                AddDrawableToArea( i, _Drawable );

                bHaveIntersection = true;
            }
        }
    }

    if ( !bHaveIntersection ) {
        AddDrawableToArea( -1, _Drawable );
    }
}

void ALevel::RemoveDrawable( ADrawable * _Drawable ) {
    ALevelArea * area;

    // Remove renderables from any areas
    for ( int i = 0 ; i < _Drawable->InArea.Size() ; ) {
        SAreaLink & InArea = _Drawable->InArea[ i ];

        if ( InArea.Level != this ) {
            i++;
            continue;
        }

        AN_ASSERT( InArea.AreaNum < InArea.Level->Areas.Size() );
        area = InArea.AreaNum >= 0 ? InArea.Level->Areas[ InArea.AreaNum ] : OutdoorArea;

        AN_ASSERT( area->Drawables[ InArea.Index ] == _Drawable );

        // Swap with last array element
        area->Drawables.RemoveSwap( InArea.Index );

        // Update swapped movable index
        if ( InArea.Index < area->Drawables.Size() ) {
            ADrawable * drawable = area->Drawables[ InArea.Index ];
            for ( int j = 0 ; j < drawable->InArea.Size() ; j++ ) {
                if ( drawable->InArea[ j ].Level == this && drawable->InArea[ j ].AreaNum == InArea.AreaNum ) {
                    drawable->InArea[ j ].Index = InArea.Index;

                    AN_ASSERT( area->Drawables[ drawable->InArea[ j ].Index ] == drawable );
                    break;
                }
            }
        }

        _Drawable->InArea.RemoveSwap(i);
    }
}

void ALevel::DrawDebug( ADebugDraw * _DebugDraw ) {

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

    if ( RVDrawLevelAreaBounds ) {

#if 0
        _DebugDraw->SetDepthTest( true );
        int i = 0;
        for ( ALevelArea * area : Areas ) {
            //_DebugDraw->DrawAABB( area->Bounds );

            i++;

            float f = (float)( (i*12345) & 255 ) / 255.0f;

            _DebugDraw->SetColor( AColor4( f,f,f,1 ) );

            _DebugDraw->DrawBoxFilled( area->Bounds.Center(), area->Bounds.HalfSize(), true );
        }
#endif
        _DebugDraw->SetDepthTest( false );
        _DebugDraw->SetColor( AColor4( 0,1,0,0.5f) );
        for ( ALevelArea * area : Areas ) {
            _DebugDraw->DrawAABB( area->Bounds );
        }

    }

    if ( RVDrawLevelPortals ) {
//        _DebugDraw->SetDepthTest( false );
//        _DebugDraw->SetColor(1,0,0,1);
//        for ( ALevelPortal * portal : Portals ) {
//            _DebugDraw->DrawLine( portal->Hull->Points, portal->Hull->NumPoints, true );
//        }

        _DebugDraw->SetDepthTest( false );
        _DebugDraw->SetColor( AColor4( 0,0,1,0.4f ) );

        if ( LastVisitedArea >= 0 && LastVisitedArea < Areas.Size() ) {
            ALevelArea * area = Areas[ LastVisitedArea ];
            SAreaPortal * portals = area->PortalList;

            for ( SAreaPortal * p = portals; p; p = p->Next ) {
                _DebugDraw->DrawConvexPoly( p->Hull->Points, p->Hull->NumPoints, true );
            }
        } else {
            for ( ALevelPortal * portal : Portals ) {
                _DebugDraw->DrawConvexPoly( portal->Hull->Points, portal->Hull->NumPoints, true );
            }
        }
    }

    if ( RVDrawLevelIndoorBounds ) {
        _DebugDraw->SetDepthTest( false );
        _DebugDraw->DrawAABB( IndoorBounds );
    }
}

int ALevel::FindArea( Float3 const & _Position ) {
    // TODO: ... binary tree?

    LastVisitedArea = -1;

    if ( Areas.Size() == 0 ) {
        return -1;
    }

    for ( int i = 0 ; i < Areas.Size() ; i++ ) {
        if (    _Position.X >= Areas[i]->Bounds.Mins.X
             && _Position.Y >= Areas[i]->Bounds.Mins.Y
             && _Position.Z >= Areas[i]->Bounds.Mins.Z
             && _Position.X <  Areas[i]->Bounds.Maxs.X
             && _Position.Y <  Areas[i]->Bounds.Maxs.Y
             && _Position.Z <  Areas[i]->Bounds.Maxs.Z ) {
            LastVisitedArea = i;
            return i;
        }
    }

    return -1;
}

void ALevel::Tick( float _TimeStep ) {
    OutdoorArea->Tree->Update();
    for ( ALevelArea * area : Areas ) {
        area->Tree->Update();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Portals traversing variables
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Portal stack
//

#define MAX_PORTAL_STACK 64

struct SPortalScissor {
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;
};

struct SPortalStack {
    PlaneF AreaFrustum[ 4 ];
    int PlanesCount;
    SAreaPortal const * Portal;
    SPortalScissor Scissor;
};

static SPortalStack PortalStack[ MAX_PORTAL_STACK ];
static int PortalStackPos;

//
// Viewer variables
//

static Float3 RightVec;
static Float3 UpVec;
static PlaneF ViewPlane;
static float  ViewZNear;
static Float3 ViewCenter;

//
// Hull clipping variables
//

#define MAX_HULL_POINTS 128
static float ClipDistances[ MAX_HULL_POINTS ];
static EPlaneSide ClipSides[ MAX_HULL_POINTS ];

struct SPortalHull {
    int NumPoints;
    Float3 Points[ MAX_HULL_POINTS ];
};
static SPortalHull PortalHull[ 2 ];


//
// Portal scissors debug
//

//#define DEBUG_PORTAL_SCISSORS
#ifdef DEBUG_PORTAL_SCISSORS
static TPodArray< SPortalScissor > DebugScissors;
#endif

//
// Debugging counters
//

//#define DEBUG_TRAVERSING_COUNTERS

#ifdef DEBUG_TRAVERSING_COUNTERS
static int Dbg_SkippedByVisFrame;
static int Dbg_SkippedByPlaneOffset;
static int Dbg_CulledByDrawableBounds;
static int Dbg_CulledSubpartsCount;
static int Dbg_CulledByDotProduct;
static int Dbg_CulledByLightBounds;
static int Dbg_CulledByEnvCaptureBounds;
static int Dbg_ClippedPortals;
static int Dbg_PassedPortals;
static int Dbg_StackDeep;
#endif

//
// AABB culling
//
AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, Float3 const & _Mins, Float3 const & _Maxs ) {
    bool inside = true;
    for ( PlaneF const * p = _Planes; p < _Planes + _Count; p++ ) {
        inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X )
            + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y )
            + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z )
            + p->D ) > 0;
    }
    return !inside;
}

AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, BvAxisAlignedBox const & _AABB ) {
    return Cull( _Planes, _Count, _AABB.Mins, _AABB.Maxs );
}

//
// Sphere culling
//
AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, BvSphereSSE const & _Sphere ) {
    bool cull = false;
    for ( const PlaneF * p = _Planes; p < _Planes + _Count; p++ ) {
        if ( Math::Dot( p->Normal, _Sphere.Center ) + p->D <= -_Sphere.Radius ) {
            cull = true;
        }
    }
    return cull;
}

//
// Fast polygon clipping. Without memory allocations.
//
static bool ClipPolygonFast( Float3 const * _InPoints, int _InNumPoints, SPortalHull * _Out, PlaneF const & _Plane, const float _Epsilon ) {
    int Front = 0;
    int Back = 0;
    int i;
    float Dist;

    AN_ASSERT( _InNumPoints + 4 <= MAX_HULL_POINTS );

    // Определить с какой стороны находится каждая точка исходного полигона
    for ( i = 0; i < _InNumPoints; i++ ) {
        Dist = _InPoints[ i ].Dot( _Plane.Normal ) + _Plane.D;

        ClipDistances[ i ] = Dist;

        if ( Dist > _Epsilon ) {
            ClipSides[ i ] = EPlaneSide::Front;
            Front++;
        } else if ( Dist < -_Epsilon ) {
            ClipSides[ i ] = EPlaneSide::Back;
            Back++;
        } else {
            ClipSides[ i ] = EPlaneSide::On;
        }
    }

    if ( !Front ) {
        // Все точки находятся по заднюю сторону плоскости
        _Out->NumPoints = 0;
        return true;
    }

    if ( !Back ) {
        // Все точки находятся по фронтальную сторону плоскости
        return false;
    }

    _Out->NumPoints = 0;

    ClipSides[ i ] = ClipSides[ 0 ];
    ClipDistances[ i ] = ClipDistances[ 0 ];

    for ( i = 0; i < _InNumPoints; i++ ) {
        Float3 const & v = _InPoints[ i ];

        if ( ClipSides[ i ] == EPlaneSide::On ) {
            _Out->Points[ _Out->NumPoints++ ] = v;
            continue;
        }

        if ( ClipSides[ i ] == EPlaneSide::Front ) {
            _Out->Points[ _Out->NumPoints++ ] = v;
        }

        EPlaneSide NextSide = ClipSides[ i + 1 ];

        if ( NextSide == EPlaneSide::On || NextSide == ClipSides[ i ] ) {
            continue;
        }

        Float3 & NewVertex = _Out->Points[ _Out->NumPoints++ ];

        NewVertex = _InPoints[ ( i + 1 ) % _InNumPoints ];

        Dist = ClipDistances[ i ] / ( ClipDistances[ i ] - ClipDistances[ i + 1 ] );
        //for ( int j = 0 ; j < 3 ; j++ ) {
        //	if ( _Plane.Normal[ j ] == 1 ) {
        //		NewVertex[ j ] = -_Plane.D;
        //	} else if ( _Plane.Normal[ j ] == -1 ) {
        //		NewVertex[ j ] = _Plane.D;
        //	} else {
        //		NewVertex[ j ] = v[ j ] + Dist * ( NewVertex[j] - v[j] );
        //	}
        //}
        NewVertex = v + Dist * ( NewVertex - v );
    }

    return true;
}

void ALevel::RenderFrontend_AddInstances( SRenderFrontendDef * _Def ) {
    // Update view area
    FindArea( _Def->View->ViewPosition );

    // Cull invisible objects
    CullInstances( _Def );
}

void ALevel::CullInstances( SRenderFrontendDef * _Def ) {
    AN_ASSERT( LastVisitedArea < Areas.Size() );

    #ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_SkippedByVisFrame = 0;
    Dbg_SkippedByPlaneOffset = 0;
    Dbg_CulledByDrawableBounds = 0;
    Dbg_CulledSubpartsCount = 0;
    Dbg_CulledByDotProduct = 0;
    Dbg_CulledByLightBounds = 0;
    Dbg_CulledByEnvCaptureBounds = 0;
    Dbg_ClippedPortals = 0;
    Dbg_PassedPortals = 0;
    Dbg_StackDeep = 0;
    #endif

    #ifdef DEBUG_PORTAL_SCISSORS
    DebugScissors.Clear();
    #endif

    BvFrustum const & frustum = *_Def->Frustum;

    RightVec = _Def->View->ViewRightVec;
    UpVec = _Def->View->ViewUpVec;
    ViewPlane = frustum[ FPL_NEAR ];
    ViewZNear = ViewPlane.Dist( _Def->View->ViewPosition );//Camera->GetZNear();
    ViewCenter = ViewPlane.Normal * ViewZNear;

    // Get corner at left-bottom of frustum
    Float3 corner = Math::Cross( frustum[ FPL_BOTTOM ].Normal, frustum[ FPL_LEFT ].Normal );

    // Project left-bottom corner to near plane
    corner = corner * ( ViewZNear / Math::Dot( ViewPlane.Normal, corner ) );

    float x = Math::Dot( RightVec, corner );
    float y = Math::Dot( UpVec, corner );

    // w = tan( half_fov_x_rad ) * znear * 2;
    // h = tan( half_fov_y_rad ) * znear * 2;

    PortalStackPos = 0;
    PortalStack[ 0 ].AreaFrustum[ 0 ] = frustum[ 0 ];
    PortalStack[ 0 ].AreaFrustum[ 1 ] = frustum[ 1 ];
    PortalStack[ 0 ].AreaFrustum[ 2 ] = frustum[ 2 ];
    PortalStack[ 0 ].AreaFrustum[ 3 ] = frustum[ 3 ];
    PortalStack[ 0 ].PlanesCount = 4;
    PortalStack[ 0 ].Portal = NULL;
    PortalStack[ 0 ].Scissor.MinX = x;
    PortalStack[ 0 ].Scissor.MinY = y;
    PortalStack[ 0 ].Scissor.MaxX = -x;
    PortalStack[ 0 ].Scissor.MaxY = -y;

    FlowThroughPortals_r( _Def, LastVisitedArea >= 0 ? Areas[ LastVisitedArea ] : OutdoorArea );

    #ifdef DEBUG_TRAVERSING_COUNTERS
    GLogger.Printf( "VSD: VisFrame %d\n", Dbg_SkippedByVisFrame );
    GLogger.Printf( "VSD: PlaneOfs %d\n", Dbg_SkippedByPlaneOffset );
    GLogger.Printf( "VSD: FaceCull %d\n", Dbg_CulledByDotProduct );
    GLogger.Printf( "VSD: AABBCull %d\n", Dbg_CulledByDrawableBounds );
    GLogger.Printf( "VSD: AABBCull (subparts) %d\n", Dbg_CulledSubpartsCount );
    GLogger.Printf( "VSD: LightCull %d\n", Dbg_CulledByLightBounds );
    GLogger.Printf( "VSD: EnvCaptureCull %d\n", Dbg_CulledByEnvCaptureBounds );
    GLogger.Printf( "VSD: Clipped %d\n", Dbg_ClippedPortals );
    GLogger.Printf( "VSD: PassedPortals %d\n", Dbg_PassedPortals );
    GLogger.Printf( "VSD: StackDeep %d\n", Dbg_StackDeep );
    #endif
}

void ALevel::RenderDrawables( SRenderFrontendDef * _Def, TPodArray< ADrawable * > const & _Drawables, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    for ( ADrawable * drawable : _Drawables ) {
        if ( !drawable->bLightPass ) {
            continue;
        }

        if ( drawable->RenderMark == _Def->VisMarker ) {
            continue;
        }

        if ( ( drawable->RenderingGroup & _Def->RenderingMask ) == 0 ) {
            drawable->RenderMark = _Def->VisMarker;
            continue;
        }

        if ( drawable->VSDPasses & VSD_PASS_FACE_CULL ) {
            // TODO: bTwoSided and bFrontSided must came from the drawable
            const bool bTwoSided = false;
            const bool bFrontSided = true;
            const float EPS = 0.25f;

            if ( !bTwoSided ) {
                PlaneF const & plane = drawable->FacePlane;
                float d = _Def->View->ViewPosition.Dot( plane.Normal );

                bool bFaceCull = false;

                if ( bFrontSided ) {
                    if ( d < -plane.D - EPS ) {
                        bFaceCull = true;
                    }
                } else {
                    if ( d > -plane.D + EPS ) {
                        bFaceCull = true;
                    }
                }

                if ( bFaceCull ) {
                    drawable->RenderMark = _Def->VisMarker;
                    #ifdef DEBUG_TRAVERSING_COUNTERS
                    Dbg_CulledByDotProduct++;
                    #endif
                    continue;
                }
            }
        }

        if ( drawable->VSDPasses & VSD_PASS_BOUNDS ) {

            // TODO: use SSE cull
            BvAxisAlignedBox const & bounds = drawable->GetWorldBounds();

            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_CulledByDrawableBounds++;
                #endif
                continue;
            }
        }

        drawable->RenderMark = _Def->VisMarker;

        if ( drawable->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

            bool bVisible = false;
            drawable->RenderFrontend_CustomVisibleStep( _Def, bVisible );

            if ( !bVisible ) {
                continue;
            }
        }

        if ( drawable->VSDPasses & VSD_PASS_VIS_MARKER ) {
            bool bVisible = drawable->VisMarker == _Def->VisMarker;
            if ( !bVisible ) {
                continue;
            }
        }

        AMeshComponent * mesh = Upcast< AMeshComponent >( drawable );

        if ( mesh ) {
            RenderMesh( _Def, mesh, _CullPlanes, _CullPlanesCount );
        } else {
            GLogger.Printf( "Not a mesh\n" );

            // TODO: Add other drawables (Sprite, ...)
        }
    }
}

void ALevel::RenderArea( SRenderFrontendDef * _Def, ALevelArea * _Area, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    RenderDrawables( _Def, _Area->GetDrawables(), _CullPlanes, _CullPlanesCount );
#ifdef FUTURE
    RenderPointLights( _Def, _Area->GetPointLights(), _CullPlanes, _CullPlanesCount );
    RenderSpotLights( _Def, _Area->GetSpotLights(), _CullPlanes, _CullPlanesCount );
    RenderEnvProbes( _Def, _Area->GetEnvProbes(), _CullPlanes, _CullPlanesCount );
#endif
}

void ALevel::FlowThroughPortals_r( SRenderFrontendDef * _Def, ALevelArea * _Area ) {
    SPortalStack * prevStack = &PortalStack[ PortalStackPos ];
    SPortalStack * stack = prevStack + 1;

    RenderArea( _Def, _Area, prevStack->AreaFrustum, prevStack->PlanesCount );

    if ( PortalStackPos == ( MAX_PORTAL_STACK - 1 ) ) {
        GLogger.Printf( "MAX_PORTAL_STACK hit\n" );
        return;
    }

    ++PortalStackPos;

    #ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_StackDeep = Math::Max( Dbg_StackDeep, PortalStackPos );
    #endif

    static float x, y, d;
    static Float3 vec;
    static Float3 p;
    static Float3 rightMin;
    static Float3 rightMax;
    static Float3 upMin;
    static Float3 upMax;
    static Float3 corners[ 4 ];
    static int flip = 0;

    for ( SAreaPortal const * portal = _Area->GetPortals(); portal; portal = portal->Next ) {

        //if ( portal->DoublePortal->VisFrame == _Def->VisMarker ) {
        //    #ifdef DEBUG_TRAVERSING_COUNTERS
        //    Dbg_SkippedByVisFrame++;
        //    #endif
        //    continue;
        //}

        d = portal->Plane.Dist( _Def->View->ViewPosition );
        if ( d <= 0.0f ) {
            #ifdef DEBUG_TRAVERSING_COUNTERS
            Dbg_SkippedByPlaneOffset++;
            #endif
            continue;
        }

        if ( d > 0.0f && d <= ViewZNear ) {
            // View intersecting the portal

            for ( int i = 0; i < prevStack->PlanesCount; i++ ) {
                stack->AreaFrustum[ i ] = prevStack->AreaFrustum[ i ];
            }
            stack->PlanesCount = prevStack->PlanesCount;
            stack->Scissor = prevStack->Scissor;

        } else {

            //for ( int i = 0 ; i < PortalStackPos ; i++ ) {
            //    if ( PortalStack[ i ].Portal == portal ) {
            //        GLogger.Printf( "Recursive!\n" );
            //    }
            //}

            // Clip portal winding by view plane
            //static PolygonF Winding;
            //PolygonF * portalWinding = &portal->Hull;
            //if ( ClipPolygonFast( portal->Hull, Winding, ViewPlane, 0.0f ) ) {
            //    if ( Winding.IsEmpty() ) {
            //        #ifdef DEBUG_TRAVERSING_COUNTERS
            //        Dbg_ClippedPortals++;
            //        #endif
            //        continue; // Culled
            //    }
            //    portalWinding = &Winding;
            //}

            if ( !ClipPolygonFast( portal->Hull->Points, portal->Hull->NumPoints, &PortalHull[ flip ], ViewPlane, 0.0f ) ) {

                AN_ASSERT( portal->Hull->NumPoints <= MAX_HULL_POINTS );

                memcpy( PortalHull[ flip ].Points, portal->Hull->Points, portal->Hull->NumPoints * sizeof( Float3 ) );
                PortalHull[ flip ].NumPoints = portal->Hull->NumPoints;
            }

            if ( PortalHull[ flip ].NumPoints >= 3 ) {
                for ( int i = 0; i < prevStack->PlanesCount; i++ ) {
                    if ( ClipPolygonFast( PortalHull[ flip ].Points, PortalHull[ flip ].NumPoints, &PortalHull[ ( flip + 1 ) & 1 ], prevStack->AreaFrustum[ i ], 0.0f ) ) {
                        flip = ( flip + 1 ) & 1;

                        if ( PortalHull[ flip ].NumPoints < 3 ) {
                            break;
                        }
                    }
                }
            }

            SPortalHull * portalWinding = &PortalHull[ flip ];

            if ( portalWinding->NumPoints < 3 ) {
                // Invisible
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_ClippedPortals++;
                #endif
                continue;
            }

            float & minX = stack->Scissor.MinX;
            float & minY = stack->Scissor.MinY;
            float & maxX = stack->Scissor.MaxX;
            float & maxY = stack->Scissor.MaxY;

            minX = 99999999.0f;
            minY = 99999999.0f;
            maxX = -99999999.0f;
            maxY = -99999999.0f;

            for ( int i = 0; i < portalWinding->NumPoints; i++ ) {

                // Project portal vertex to view plane
                vec = portalWinding->Points[ i ] - _Def->View->ViewPosition;

                d = Math::Dot( ViewPlane.Normal, vec );

                //if ( d < ViewZNear ) {
                //    AN_ASSERT(0);
                //}

                p = d < ViewZNear ? vec : vec * ( ViewZNear / d );

                // Compute relative coordinates
                x = Math::Dot( RightVec, p );
                y = Math::Dot( UpVec, p );

                // Compute bounds
                minX = Math::Min( x, minX );
                minY = Math::Min( y, minY );

                maxX = Math::Max( x, maxX );
                maxY = Math::Max( y, maxY );
            }

            // Clip bounds by current scissor bounds
            minX = Math::Max( prevStack->Scissor.MinX, minX );
            minY = Math::Max( prevStack->Scissor.MinY, minY );
            maxX = Math::Min( prevStack->Scissor.MaxX, maxX );
            maxY = Math::Min( prevStack->Scissor.MaxY, maxY );

            if ( minX >= maxX || minY >= maxY ) {
                // invisible
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_ClippedPortals++;
                #endif
                continue; // go to next portal
            }

            // Compute 3D frustum to cull objects inside vis area
            if ( portalWinding->NumPoints <= 4 ) {
                stack->PlanesCount = portalWinding->NumPoints;

                // Compute based on portal winding
                for ( int i = 0; i < stack->PlanesCount; i++ ) {
                    stack->AreaFrustum[ i ].FromPoints( _Def->View->ViewPosition, portalWinding->Points[ ( i + 1 ) % portalWinding->NumPoints ], portalWinding->Points[ i ] );
                }
            } else {
                // Compute based on portal scissor
                rightMin = RightVec * minX + ViewCenter;
                rightMax = RightVec * maxX + ViewCenter;
                upMin = UpVec * minY;
                upMax = UpVec * maxY;
                corners[ 0 ] = rightMin + upMin;
                corners[ 1 ] = rightMax + upMin;
                corners[ 2 ] = rightMax + upMax;
                corners[ 3 ] = rightMin + upMax;

                // bottom
                p = Math::Cross( corners[ 1 ], corners[ 0 ] );
                stack->AreaFrustum[ 0 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 0 ].D = -Math::Dot( stack->AreaFrustum[ 0 ].Normal, _Def->View->ViewPosition );

                // right
                p = Math::Cross( corners[ 2 ], corners[ 1 ] );
                stack->AreaFrustum[ 1 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 1 ].D = -Math::Dot( stack->AreaFrustum[ 1 ].Normal, _Def->View->ViewPosition );

                // top
                p = Math::Cross( corners[ 3 ], corners[ 2 ] );
                stack->AreaFrustum[ 2 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 2 ].D = -Math::Dot( stack->AreaFrustum[ 2 ].Normal, _Def->View->ViewPosition );

                // left
                p = Math::Cross( corners[ 0 ], corners[ 3 ] );
                stack->AreaFrustum[ 3 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 3 ].D = -Math::Dot( stack->AreaFrustum[ 3 ].Normal, _Def->View->ViewPosition );

                stack->PlanesCount = 4;
            }
        }

        #ifdef DEBUG_PORTAL_SCISSORS
        DebugScissors.Append( stack->Scissor );
        #endif

        #ifdef DEBUG_TRAVERSING_COUNTERS
        Dbg_PassedPortals++;
        #endif

        stack->Portal = portal;

        portal->Owner->VisMark = _Def->VisMarker;
        FlowThroughPortals_r( _Def, portal->ToArea );
    }

    --PortalStackPos;
}

void ALevel::RenderMesh( SRenderFrontendDef * _Def, AMeshComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    Float4x4 tmpMatrix;
    Float4x4 * instanceMatrix;

    AIndexedMesh * mesh = component->GetMesh();

    size_t skeletonOffset = 0;
    size_t skeletonSize = 0;
    if ( mesh->IsSkinned() && component->IsSkinnedMesh() ) {
        ASkinnedComponent * skeleton = static_cast< ASkinnedComponent * >( component );
        skeleton->UpdateJointTransforms( skeletonOffset, skeletonSize, GRuntime.GetFrameData()->FrameNumber );
    }

    Float3x4 const & componentWorldTransform = component->GetWorldTransformMatrix();

    if ( component->bNoTransform ) {
        instanceMatrix = &_Def->View->ModelviewProjection;
    } else {
        tmpMatrix = _Def->View->ModelviewProjection * componentWorldTransform; // TODO: optimize: parallel, sse, check if transformable
        instanceMatrix = &tmpMatrix;
    }

    AActor * actor = component->GetParentActor();
    ALevel * level = actor->GetLevel();

    AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

        AIndexedMeshSubpart * subpart = subparts[ subpartIndex ];

        if ( !mesh->IsSkinned() && subparts.Size() > 1 && ( component->VSDPasses & VSD_PASS_BOUNDS ) ) {

            // TODO: use SSE cull

            if ( Cull( _CullPlanes, _CullPlanesCount, subpart->GetBoundingBox().Transform( componentWorldTransform ) ) ) {
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_CulledSubpartsCount++;
                #endif
                continue;
            }
        }

        AMaterialInstance * materialInstance = component->GetMaterialInstance( subpartIndex );
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        SMaterialFrameData * materialInstanceFrameData = materialInstance->RenderFrontend_Update( _Def->VisMarker );

        // Add render instance
        SRenderInstance * instance = ( SRenderInstance * )GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
        if ( !instance ) {
            return;
        }

        GRuntime.GetFrameData()->Instances.Append( instance );

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;
        instance->VertexBuffer = mesh->GetVertexBufferGPU();
        instance->IndexBuffer = mesh->GetIndexBufferGPU();
        instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

        if ( component->LightmapUVChannel && component->LightmapBlock >= 0 && component->LightmapBlock < level->Lightmaps.Size() ) {
            instance->LightmapUVChannel = component->LightmapUVChannel->GetGPUResource();
            instance->LightmapOffset = component->LightmapOffset;
            instance->Lightmap = level->Lightmaps[ component->LightmapBlock ]->GetGPUResource();
        } else {
            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
        }

        if ( component->VertexLightChannel ) {
            instance->VertexLightChannel = component->VertexLightChannel->GetGPUResource();
        } else {
            instance->VertexLightChannel = nullptr;
        }

        if ( component->bUseDynamicRange ) {
            instance->IndexCount = component->DynamicRangeIndexCount;
            instance->StartIndexLocation = component->DynamicRangeStartIndexLocation;
            instance->BaseVertexLocation = component->DynamicRangeBaseVertexLocation;
        } else {
            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex() + component->SubpartBaseVertexOffset;
        }

        instance->SkeletonOffset = skeletonOffset;
        instance->SkeletonSize = skeletonSize;
        instance->Matrix = *instanceMatrix;

        if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
            instance->ModelNormalToViewSpace = _Def->View->NormalToViewMatrix * component->GetWorldRotation().ToMatrix();
        }

        instance->RenderingOrder = component->RenderingOrder;

        _Def->View->InstanceCount++;

        _Def->PolyCount += instance->IndexCount / 3;

        if ( component->bUseDynamicRange ) {
            // If component uses dynamic range, mesh has actually one subpart
            break;
        }
    }
}


#ifdef FUTURE
void AddLight( FLightComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    if ( component->RenderMark == VisMarker ) {
        return;
    }

    if ( ( component->RenderingGroup & RP->RenderingLayers ) == 0 ) {
        component->RenderMark = VisMarker;
        return;
    }

    if ( component->VSDPasses & VSD_PASS_BOUNDS ) {

        // TODO: use SSE cull

        switch ( Light->GetType() ) {
        case FLightComponent::T_Point:
        {
            BvSphereSSE const & bounds = Light->GetSphereWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByLightBounds++;
                return;
            }
            break;
        }
        case FLightComponent::T_Spot:
        {
            BvSphereSSE const & bounds = Light->GetSphereWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByLightBounds++;
                return;
            }
            break;
        }
        case FLightComponent::T_Direction:
        {
            break;
        }
        }
    }

    component->RenderMark = VisMarker;

    if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

        bool bVisible;
        component->OnCustomVisibleStep( Camera, bVisible );

        if ( !bVisible ) {
            return;
        }
    }

    if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {
        bool bVisible = component->VisMarker == VisMarker;
        if ( !bVisible ) {
            return;
        }
    }

    // TODO: add to render view

    RV->LightCount++;
}

void AddEnvCapture( FEnvCaptureComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    if ( component->RenderMark == VisMarker ) {
        return;
    }

    if ( ( component->RenderingGroup & RP->RenderingLayers ) == 0 ) {
        component->RenderMark = VisMarker;
        return;
    }

    if ( component->VSDPasses & VSD_PASS_BOUNDS ) {

        // TODO: use SSE cull

        switch ( component->GetShapeType() ) {
        case FEnvCaptureComponent::SHAPE_BOX:
        {
            // Check OBB ?
            BvAxisAlignedBox const & bounds = component->GetAABBWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByEnvCaptureBounds++;
                return;
            }
            break;
        }
        case FEnvCaptureComponent::SHAPE_SPHERE:
        {
            BvSphereSSE const & bounds = component->GetSphereWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByEnvCaptureBounds++;
                return;
            }
            break;
        }
        case FEnvCaptureComponent::SHAPE_GLOBAL:
        {
            break;
        }
        }
    }

    component->RenderMark = VisMarker;

    if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

        bool bVisible;
        component->OnCustomVisibleStep( Camera, bVisible );

        if ( !bVisible ) {
            return;
        }
    }

    if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {
        bool bVisible = component->VisMarker == VisMarker;
        if ( !bVisible ) {
            return;
        }
    }

    // TODO: add to render view

    RV->EnvCaptureCount++;
}
#endif
