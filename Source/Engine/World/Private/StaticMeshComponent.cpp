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

#include <Engine/World/Public/StaticMeshComponent.h>
#include <Engine/World/Public/Actor.h>
#include <Engine/World/Public/World.h>
#include <Engine/Core/Public/Logger.h>

AN_CLASS_META_NO_ATTRIBS( FStaticMeshComponent )

FStaticMeshComponent::FStaticMeshComponent() {
    SurfaceType = SURF_TRISOUP;
    LightmapOffset.Z = LightmapOffset.W = 1;
    bCanEverTick = true;
}

void FStaticMeshComponent::InitializeComponent() {
    Super::InitializeComponent();

    FWorld * world = GetParentActor()->GetWorld();

    world->RegisterStaticMesh( this );

    //GLogger.Printf( "FStaticMeshComponent::InitializeComponent()\n" );
}

void FStaticMeshComponent::BeginPlay() {
    Super::BeginPlay();
}

void FStaticMeshComponent::EndPlay() {
    Super::EndPlay();

    FWorld * world = GetParentActor()->GetWorld();

    world->UnregisterStaticMesh( this );
}

void FStaticMeshComponent::TickComponent( float _TimeStep ) {
    Super::TickComponent( _TimeStep );

//    GetWorld()->DebugDraw.SetColor( 0xffffffff );
//    GetWorld()->DebugDraw.SetDepthTest( true );
//    GetWorld()->DebugDraw.DrawAABB( GetWorldBounds() );

//    FDebugDraw & dd = *GetWorld()->pDebugDraw;
//    Float3 worldPosition = GetWorldPosition();
//    dd.SetColor( Float4( 1,0,0,1 ) );
//    dd.AddLine( worldPosition, worldPosition + GetRightVector()*100.0f );
//    dd.SetColor( Float4( 0,1,0,1 ) );
//    dd.AddLine( worldPosition, worldPosition + GetUpVector()*100.0f );
//    dd.SetColor( Float4( 0,0,1,1 ) );
//    dd.AddLine( worldPosition, worldPosition + GetBackVector()*100.0f );

    //GLogger.Printf( "world pos: %s\n", worldPosition.ToString().ToConstChar() );
}

void FStaticMeshComponent::SetMesh( FIndexedMesh * _Mesh ) {
    if ( _Mesh ) {
        Mesh = _Mesh;
        Subpart = _Mesh->GetPersistentSubpart();
    } else {
        Mesh = nullptr;
        Subpart = nullptr;
    }
}

void FStaticMeshComponent::SetMeshSubpart( FIndexedMeshSubpart * _Subpart ) {
    if ( _Subpart ) {
        Mesh = _Subpart->GetParent();
        Subpart = _Subpart;
    } else {
        Mesh = nullptr;
        Subpart = nullptr;
    }
}

void FStaticMeshComponent::OnUpdateWorldBounds() {
    WorldBounds = Bounds.Transform( GetWorldTransformMatrix() );
}


//AN_SCENE_COMPONENT_BEGIN_DECL( FStaticMeshComponent, CCF_DEFAULT )

//AN_ATTRIBUTE_CONST( "Mesh", FProperty( byte(0) ), "StaticMesh\0\0Static mesh resource", AF_RESOURCE )
//AN_ATTRIBUTE( "Mins", FProperty( Float3(-1) ), SetBoundsMins, GetBoundsMins, "Mesh cull box mins", AF_DEFAULT )
//AN_ATTRIBUTE( "Maxs", FProperty( Float3(1) ), SetBoundsMaxs, GetBoundsMaxs, "Mesh cull box maxs", AF_DEFAULT )
//AN_ATTRIBUTE( "Use Custom Bounds", FProperty( false ), SetUseCustomBounds, IsCustomBounds, "Override mesh resource bounds", AF_DEFAULT )

//AN_SCENE_COMPONENT_END_DECL


//void FStaticMeshComponent::SetMesh( const char * _ResourceName ) {
//    if ( _ResourceName && *_ResourceName ) {
//        FStaticMeshResource * ResourceMesh = GResourceManager->GetResource< FStaticMeshResource >( _ResourceName );
//        SetMesh( ResourceMesh );
//    } else {
//        SetMesh( ( FStaticMeshResource *)0 );
//    }
//}

//void FStaticMeshComponent::SetMesh( FStaticMeshResource * _Mesh ) {
//    if ( Mesh == _Mesh ) {
//        return;
//    }

//    Mesh = _Mesh;

//    if ( Mesh ) {
//        Mesh->Load();
//    }

//    if ( Mesh ) {
//        Bounds = UseCustomBounds ? CustomBounds : Mesh->GetBounds();

//        MarkBoundsDirty();
//    } else {
//        //BvAxisAlignedBox BoundingBox;
//        //BoundingBox.Clear();
//        //SetBounds( BoundingBox );
//        Bounds = CustomBounds;

//        MarkBoundsDirty();
//    }
//}

//void FStaticMeshComponent::SetResource( FResource * _Resource ) {
//    if ( !_Resource ) {
//        return;
//    }
//    if ( _Resource->GetFinalClassId() == FStaticMeshResource::GetClassId() ) {
//        SetMesh( static_cast< FStaticMeshResource * >( _Resource ) );
//    } else {
//        Super::SetResource( _Resource );
//    }
//}

//FResource * FStaticMeshComponent::GetResource( const char * _AttributeName ) {
//    if ( !FString::CmpCase( _AttributeName, "Mesh" ) ) {
//        return Mesh;
//    }
//    return Super::GetResource( _AttributeName );
//}


//void FStaticMeshComponent::DrawDebug( class FCameraComponent * _Camera, EDebugDrawFlags::Type _DebugDrawFlags, class FPrimitiveBatchComponent * _DebugDraw ) {
//    if ( /*Drawable && */( _DebugDrawFlags & EDebugDrawFlags::DRAW_STATIC_BOUNDS ) ) {
//        _DebugDraw->SetZOrder( 0xffff );
//        _DebugDraw->SetZTest( false );
//        _DebugDraw->SetLineWidth( 1 );
//        _DebugDraw->SetColor( 1,1,0,1 );

//        _DebugDraw->DrawAABB( GetWorldBounds() );
//    }
//}
