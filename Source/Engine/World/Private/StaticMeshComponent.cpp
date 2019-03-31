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

AN_CLASS_META_NO_ATTRIBS( FStaticMeshComponent )

FStaticMeshComponent::FStaticMeshComponent() {
    SurfaceType = SURF_TRISOUP;
    LightmapOffset.Z = LightmapOffset.W = 1;
}

void FStaticMeshComponent::InitializeComponent() {
    Super::InitializeComponent();

    FWorld * world = GetParentActor()->GetWorld();

    world->RegisterStaticMesh( this );
}

void FStaticMeshComponent::BeginPlay() {
    Super::BeginPlay();
}

void FStaticMeshComponent::EndPlay() {
    Super::EndPlay();

    FWorld * world = GetParentActor()->GetWorld();

    world->UnregisterStaticMesh( this );
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
