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

#include <Engine/Resource/Public/ResourceManager.h>

AN_CLASS_META( FBoxActor )
AN_CLASS_META( FStaticBoxActor )
AN_CLASS_META( FSphereActor )
AN_CLASS_META( FCylinderActor )

FBoxActor::FBoxActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = AddComponent< FMeshComponent >( "DynamicBox" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
    MeshComponent->bUseDefaultBodyComposition = true;

    // Example how to create custom collision body
    //FCollisionBox * collisionBody = NewObject< FCollisionBox >();
    //collisionBody->HalfExtents = Float3(0.5f);
    //MeshComponent->BodyComposition.AddCollisionBody( collisionBody );
    //MeshComponent->bUseDefaultBodyComposition = false;

    // Setup physics
    MeshComponent->Mass = 1.0f;
    MeshComponent->PhysicsBehavior = PB_DYNAMIC;
    MeshComponent->CollisionGroup = CM_WORLD_DYNAMIC;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeBoxMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

FStaticBoxActor::FStaticBoxActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( 0.5f );

    // Create mesh component and set it as root component
    MeshComponent = AddComponent< FMeshComponent >( "StaticBox" );
    RootComponent = MeshComponent;

    // Setup physics
    MeshComponent->bUseDefaultBodyComposition = true;
    MeshComponent->PhysicsBehavior = PB_STATIC;

    MeshComponent->bAINavigation = true;

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
    MeshComponent = AddComponent< FMeshComponent >( "DynamicSphere" );
    RootComponent = MeshComponent;

    // Setup physics
    MeshComponent->bUseDefaultBodyComposition = false;
    FCollisionSphere * collisionSphere = MeshComponent->BodyComposition.AddCollisionBody< FCollisionSphere >();
    collisionSphere->bProportionalScale = false;
    MeshComponent->Mass = 1.0f;
    MeshComponent->PhysicsBehavior = PB_DYNAMIC;

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
    MeshComponent = AddComponent< FMeshComponent >( "DynamicCylinder" );
    RootComponent = MeshComponent;

    // Setup physics
    MeshComponent->bUseDefaultBodyComposition = true;
    MeshComponent->Mass = 1.0f;
    MeshComponent->PhysicsBehavior = PB_DYNAMIC;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeCylinderMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}
