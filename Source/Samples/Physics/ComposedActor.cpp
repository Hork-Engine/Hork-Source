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

#include "ComposedActor.h"

#include <Engine/World/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FComposedActor )

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

#if 0
        FCollisionBox * boxBody = NewObject< FCollisionBox >();
        boxBody->Position = Float3(0,4,0);
        boxBody->Rotation.FromAngles( FMath::Radians( 45.0f ), 0.f, 0.f );
        Cylinder->BodyComposition.AddCollisionBody( boxBody );
#endif

        Cylinder->BodyComposition.ComputeCenterOfMassAvg();

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

        Box->Mass = 1;
        Box->bSimulatePhysics = true;// false;
        Box->bKinematicBody = false;
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
