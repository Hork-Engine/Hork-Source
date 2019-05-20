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

#include "StaticMesh.h"

#include <Engine/World/Public/MaterialAssembly.h>
#include <Engine/World/Public/MeshComponent.h>
#include <Engine/World/Public/SoftMeshComponent.h>
#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/InputComponent.h>
#include <Engine/World/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FBoxActor )
AN_CLASS_META_NO_ATTRIBS( FStaticBoxActor )
AN_CLASS_META_NO_ATTRIBS( FSphereActor )
AN_CLASS_META_NO_ATTRIBS( FCylinderActor )
AN_CLASS_META_NO_ATTRIBS( FComposedActor )
AN_CLASS_META_NO_ATTRIBS( FBoxTrigger )

FBoxActor::FBoxActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "DynamicBox" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
#if 1
    MeshComponent->bUseDefaultBodyComposition = true;
#else
    FCollisionBox * collisionBody = NewObject< FCollisionBox >();
    collisionBody->HalfExtents = Float3(0.5f);
    MeshComponent->BodyComposition.AddCollisionBody( collisionBody );
#endif

    MeshComponent->Mass = 1.0f;
    MeshComponent->bSimulatePhysics = true;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeBoxMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );


//    FPhysicalBody * Dummy;
//    Dummy = CreateComponent< FPhysicalBody >( "Dummy" );
//    Dummy->Mass = 1.0f;
//    Dummy->AttachTo( RootComponent );

//    // Create collision body for mesh component
//    collisionBody = NewObject< FCollisionBox >();
//    collisionBody->HalfExtents = Float3(0.5f);
//    collisionBody->Position = Float3(-2.0f, 0.5f, 0.0f );
//    Dummy->BodyComposition.AddCollisionBody( collisionBody );
}

FStaticBoxActor::FStaticBoxActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( 0.5f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "StaticBox" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
#if 1
    MeshComponent->bUseDefaultBodyComposition = true;
#else
    FCollisionBox * collisionBody = NewObject< FCollisionBox >();
    collisionBody->HalfExtents = Float3(0.5f);
    MeshComponent->BodyComposition.AddCollisionBody( collisionBody );
#endif
    MeshComponent->bSimulatePhysics = true;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeBoxMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

FSphereActor::FSphereActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FSoftMeshComponent >( "DynamicSphere" );
    RootComponent = MeshComponent;

#if 0
    // Create collision body for mesh component
#if 0
    MeshComponent->bUseDefaultBodyComposition = true;
#else
    FCollisionSphere * collisionBody = NewObject< FCollisionSphere >();
    collisionBody->Radius = 0.5f;
    collisionBody->bProportionalScale = false;
    MeshComponent->BodyComposition.AddCollisionBody( collisionBody );
#endif
#endif

    MeshComponent->Mass = 1.0f;
    MeshComponent->bSimulatePhysics = true;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeSphereMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

FCylinderActor::FCylinderActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "DynamicCylinder" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
#if 1
    MeshComponent->bUseDefaultBodyComposition = true;
#else
    FCollisionCylinder * collisionBody = NewObject< FCollisionCylinder >();
    collisionBody->HalfExtents = Float3(0.5f);
    MeshComponent->BodyComposition.AddCollisionBody( collisionBody );
#endif
    MeshComponent->Mass = 1.0f;
    MeshComponent->bSimulatePhysics = true;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeCylinderMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}





FComposedActor::FComposedActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    {
        // Create mesh component and set it as root component
        Cylinder = CreateComponent< FMeshComponent >( "DynamicComposed" );
        RootComponent = Cylinder;

        FCollisionCylinder * cylinderBody = NewObject< FCollisionCylinder >();
        cylinderBody->HalfExtents = Float3(0.5f);
        Cylinder->BodyComposition.AddCollisionBody( cylinderBody );

        FCollisionBox * boxBody = NewObject< FCollisionBox >();
        boxBody->Position = Float3(0,4,0);
        boxBody->Rotation.FromAngles( FMath::Radians( 45.0f ), 0.f, 0.f );
        Cylinder->BodyComposition.AddCollisionBody( boxBody );

        Cylinder->BodyComposition.ComputeCenterOfMass();

        Cylinder->Mass = 1.0f;
        Cylinder->bSimulatePhysics = true;

        // Set mesh and material resources for mesh component
        Cylinder->SetMesh( GetResource< FIndexedMesh >( "ShapeCylinderMesh" ) );
        Cylinder->SetMaterialInstance( 0, matInst );
    }

#if 1
    {
        Box = CreateComponent< FMeshComponent >( "Box" );
        Box->AttachTo( Cylinder );
        Box->SetPosition( Float3(0,4.0f,0) );
        Box->SetAngles( 45, 0, 0 );

        //Box->SetScale( 4,1,4 );

        // Set mesh and material resources for mesh component
        Box->SetMesh( GetResource< FIndexedMesh >( "ShapeBoxMesh" ) );
        Box->SetMaterialInstance( 0, matInst );

        Box->Mass = 0.0f;
        Box->bSimulatePhysics = false;
        Box->bKinematicBody = true;
        //Box->bDisableGravity = true;

        FCollisionBox * body = NewObject< FCollisionBox >();
        //body->Radius = Float3(1);
        //body->Position = Float3(0,0,0);
        Box->BodyComposition.AddCollisionBody( body );
    }
#endif

//    {
//        FMeshComponent * SphereComponent;
//        SphereComponent = CreateComponent< FMeshComponent >( "Sphere" );
//        SphereComponent->AttachTo( MeshComponent );
//        SphereComponent->SetPosition( Float3(0,1.0f,0) );

//        // No physics, just visual
//        SphereComponent->bSimulatePhysics = false;

//        SphereComponent->SetScale( 4,1,4 );

//        // Set mesh and material resources for mesh component
//        SphereComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeSphereMesh" ) );
//        SphereComponent->SetMaterialInstance( 0, matInst );
//    }
}

void FComposedActor::BeginPlay() {
    Cylinder->SetScale( 1, 4, 1 );
}



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
