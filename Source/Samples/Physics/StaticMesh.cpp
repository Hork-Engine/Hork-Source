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
#include "Module.h"

#include <Engine/World/Public/MaterialAssembly.h>
#include <Engine/World/Public/MeshComponent.h>
#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/InputComponent.h>
#include <Engine/World/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FBoxActor )
AN_CLASS_META_NO_ATTRIBS( FSphereActor )
AN_CLASS_META_NO_ATTRIBS( FCylinderActor )
AN_CLASS_META_NO_ATTRIBS( FComposedActor )
AN_CLASS_META_NO_ATTRIBS( FBoxTrigger )

FBoxActor::FBoxActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GModule->Material;
    matInst->SetTexture( 0, GResourceManager.GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "StaticMesh" );
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

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GResourceManager.GetResource< FIndexedMesh >( "*box*" ) );
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

FSphereActor::FSphereActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GModule->Material;
    matInst->SetTexture( 0, GResourceManager.GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "StaticMesh" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
#if 0
    MeshComponent->bUseDefaultBodyComposition = true;
#else
    FCollisionSphere * collisionBody = NewObject< FCollisionSphere >();
    collisionBody->Radius = 0.5f;
    collisionBody->bProportionalScale = false;
    MeshComponent->BodyComposition.AddCollisionBody( collisionBody );
#endif
    MeshComponent->Mass = 1.0f;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GResourceManager.GetResource< FIndexedMesh >( "ShapeSphereMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

FCylinderActor::FCylinderActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GModule->Material;
    matInst->SetTexture( 0, GResourceManager.GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "StaticMesh" );
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

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GResourceManager.GetResource< FIndexedMesh >( "ShapeCylinderMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}





FComposedActor::FComposedActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GModule->Material;
    matInst->SetTexture( 0, GResourceManager.GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    {
        // Create mesh component and set it as root component
        MeshComponent = CreateComponent< FMeshComponent >( "Cylinder" );
        RootComponent = MeshComponent;

        FCollisionCylinder * cylinderBody = NewObject< FCollisionCylinder >();
        cylinderBody->HalfExtents = Float3(0.5f);
        MeshComponent->BodyComposition.AddCollisionBody( cylinderBody );

//        FCollisionSphereRadii * sphereBody = NewObject< FCollisionSphereRadii >();
//        sphereBody->Radius = Float3( 2,0.5,2 );
//        sphereBody->Position = Float3(0,1,0);
//        MeshComponent->BodyComposition.AddCollisionBody( sphereBody );

        MeshComponent->Mass = 1.0f;
        // Set mesh and material resources for mesh component
        MeshComponent->SetMesh( GResourceManager.GetResource< FIndexedMesh >( "ShapeCylinderMesh" ) );
        MeshComponent->SetMaterialInstance( 0, matInst );
    }


    {
        FMeshComponent * SphereComponent;
        SphereComponent = CreateComponent< FMeshComponent >( "Sphere" );
        SphereComponent->AttachTo( MeshComponent );
        SphereComponent->SetPosition( Float3(0,1.0f,0) );

        SphereComponent->SetScale( 4,1,4 );

        // Set mesh and material resources for mesh component
        SphereComponent->SetMesh( GResourceManager.GetResource< FIndexedMesh >( "ShapeSphereMesh" ) );
        SphereComponent->SetMaterialInstance( 0, matInst );

//        !!!TODO!!!
        SphereComponent->Mass = 1.0f;

        FCollisionSphere * sphereBody = NewObject< FCollisionSphere >();
        sphereBody->Radius = 1;
        sphereBody->Position = Float3(0,1,0);
        sphereBody->bProportionalScale = false;
        MeshComponent->BodyComposition.AddCollisionBody( sphereBody );


        SphereComponent->AttachTo( MeshComponent );
    }

//    {
//        FMeshComponent * SphereComponent;
//        SphereComponent = CreateComponent< FMeshComponent >( "Sphere" );
//        SphereComponent->AttachTo( MeshComponent );
//        SphereComponent->SetPosition( Float3(0,1.0f,0) );

//        // No physics, just visual
//        SphereComponent->bNoPhysics = true;

//        SphereComponent->SetScale( 4,1,4 );

//        // Set mesh and material resources for mesh component
//        SphereComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeSphereMesh" ) );
//        SphereComponent->SetMaterialInstance( 0, matInst );
//    }
}





FBoxTrigger::FBoxTrigger() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GModule->Material;
    matInst->SetTexture( 0, GResourceManager.GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "Trigger" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
    MeshComponent->bUseDefaultBodyComposition = true;
    MeshComponent->bTrigger = true;
    MeshComponent->bDispatchOverlapEvents = true;
    //MeshComponent->Mass = 1.0f;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GResourceManager.GetResource< FIndexedMesh >( "*box*" ) );
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

void FBoxTrigger::OnBeginOverlap() {
    GLogger.Printf( "OnBeginOverlap\n" );
}

void FBoxTrigger::OnEndOverlap() {
    GLogger.Printf( "OnEndOverlap\n" );
}

void FBoxTrigger::OnUpdateOverlap() {
    GLogger.Printf( "OnUpdateOverlap\n" );
}
