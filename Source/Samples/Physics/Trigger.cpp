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

#include "Trigger.h"

#include <Engine/World/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FBoxTrigger )

FBoxTrigger::FBoxTrigger() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "Trigger" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
    MeshComponent->bUseDefaultBodyComposition = true;
    MeshComponent->bTrigger = true;
    MeshComponent->bDispatchOverlapEvents = true;
    //MeshComponent->Mass = 1.0f;
    MeshComponent->bSimulatePhysics = true;
    MeshComponent->CollisionMask = CM_ALL;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeBoxMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

void FBoxTrigger::BeginPlay() {
    Super::BeginPlay();

    E_OnBeginOverlap.Subscribe( this, &FBoxTrigger::OnBeginOverlap );
    E_OnEndOverlap.Subscribe( this, &FBoxTrigger::OnEndOverlap );
    E_OnUpdateOverlap.Subscribe( this, &FBoxTrigger::OnUpdateOverlap );
}

void FBoxTrigger::EndPlay() {
    Super::EndPlay();
}

void FBoxTrigger::OnBeginOverlap( FOverlapEvent const & _Event ) {
    GLogger.Printf( "OnBeginOverlap: self %s other %s\n",
                    _Event.SelfBody->GetName().ToConstChar(),
                    _Event.OtherBody->GetName().ToConstChar() );
}

void FBoxTrigger::OnEndOverlap( FOverlapEvent const & _Event ) {
    GLogger.Printf( "OnEndOverlap: self %s other %s\n",
                    _Event.SelfBody->GetName().ToConstChar(),
                    _Event.OtherBody->GetName().ToConstChar() );
}

void FBoxTrigger::OnUpdateOverlap( FOverlapEvent const & _Event ) {
    GLogger.Printf( "OnUpdateOverlap: self %s other %s\n",
                    _Event.SelfBody->GetName().ToConstChar(),
                    _Event.OtherBody->GetName().ToConstChar() );
}
