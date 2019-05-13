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

#include "Explosion.h"

#include <Engine/World/Public/IndexedMesh.h>
#include <Engine/World/Public/MeshComponent.h>
#include <Engine/World/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FExplosionActor )

FExplosionActor::FExplosionActor() {
    bCanEverTick = true;

    MeshComponent = CreateComponent< FMeshComponent >( "Explosion" );
    RootComponent = MeshComponent;

    FIndexedMesh * mesh = GetResource< FIndexedMesh >( "UnitSphere" );
    MeshComponent->SetMesh( mesh );
    MeshComponent->SetMaterialInstance( 0, GetResource< FMaterialInstance >( "ExplosionMaterialInstance" ) );
}

void FExplosionActor::BeginPlay() {
    Super::BeginPlay();

    ExplosionRadius = 1;
}

void FExplosionActor::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    ExplosionRadius += _TimeStep * 40;

    constexpr float MAX_VISUAL_RADIUS = 10.0f;
    constexpr float MAX_RADIUS = 15;

    if ( ExplosionRadius > MAX_RADIUS ) {
        Destroy();
    } else {
        RootComponent->SetScale( FMath::Min( ExplosionRadius, MAX_VISUAL_RADIUS ) );
    }
}
