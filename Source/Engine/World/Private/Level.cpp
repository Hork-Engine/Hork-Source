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
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Runtime/Public/Runtime.h>

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

    //OutdoorArea->Tree = NewObject< AOctree >();
    //OutdoorArea->Tree->Owner = OutdoorArea;
    //OutdoorArea->Tree->Build();

    LastVisitedArea = -1;
}

ALevel::~ALevel() {
    ClearLightmaps();

    HugeFree( LightData );

    DestroyActors();

    DestroyPortalTree();
}

void ALevel::AddStaticMesh( AMeshComponent * InStaticMesh ) {
    //INTRUSIVE_ADD_UNIQUE( InStaticMesh, NextStaticMesh, PrevStaticMesh, StaticMeshList, StaticMeshListTail );
}

void ALevel::RemoveStaticMesh( AMeshComponent * InStaticMesh ) {
    //INTRUSIVE_REMOVE( InStaticMesh, NextStaticMesh, PrevStaticMesh, StaticMeshList, StaticMeshListTail );
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

    //area->Tree = NewObject< AOctree >();
    //area->Tree->Owner = area;
    //area->Tree->Build();

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

//        AN_ASSERT( InArea.Level == this );
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

void ALevel::DrawDebug( ADebugRenderer * InRenderer ) {

    if ( RVDrawLevelAreaBounds ) {

#if 0
        InRenderer->SetDepthTest( true );
        int i = 0;
        for ( ALevelArea * area : Areas ) {
            //InRenderer->DrawAABB( area->Bounds );

            i++;

            float f = (float)( (i*12345) & 255 ) / 255.0f;

            InRenderer->SetColor( AColor4( f,f,f,1 ) );

            InRenderer->DrawBoxFilled( area->Bounds.Center(), area->Bounds.HalfSize(), true );
        }
#endif
        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( AColor4( 0,1,0,0.5f) );
        for ( ALevelArea * area : Areas ) {
            InRenderer->DrawAABB( area->Bounds );
        }

    }

    if ( RVDrawLevelPortals ) {
//        InRenderer->SetDepthTest( false );
//        InRenderer->SetColor(1,0,0,1);
//        for ( ALevelPortal * portal : Portals ) {
//            InRenderer->DrawLine( portal->Hull->Points, portal->Hull->NumPoints, true );
//        }

        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( AColor4( 0,0,1,0.4f ) );

        if ( LastVisitedArea >= 0 && LastVisitedArea < Areas.Size() ) {
            ALevelArea * area = Areas[ LastVisitedArea ];
            SAreaPortal * portals = area->PortalList;

            for ( SAreaPortal * p = portals; p; p = p->Next ) {
                InRenderer->DrawConvexPoly( p->Hull->Points, p->Hull->NumPoints, true );
            }
        } else {
            for ( ALevelPortal * portal : Portals ) {
                InRenderer->DrawConvexPoly( portal->Hull->Points, portal->Hull->NumPoints, true );
            }
        }
    }

    if ( RVDrawLevelIndoorBounds ) {
        InRenderer->SetDepthTest( false );
        InRenderer->DrawAABB( IndoorBounds );
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
    //OutdoorArea->Tree->Update();
    //for ( ALevelArea * area : Areas ) {
    //    area->Tree->Update();
    //}
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
