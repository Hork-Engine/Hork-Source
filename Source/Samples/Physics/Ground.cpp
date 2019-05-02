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

#include "Ground.h"
#include "Module.h"

#include <Engine/World/Public/MeshComponent.h>
#include <Engine/World/Public/MaterialAssembly.h>
#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/InputComponent.h>
#include <Engine/World/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FGround )

FGround::FGround() {
    // Get mesh resource (plane)
    FIndexedMesh * mesh = GResourceManager.GetResource< FIndexedMesh >( "DefaultShapePlane256x256x256" );

    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();
    matInst->Material = GModule->Material;
    matInst->SetTexture( 0, GResourceManager.GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "StaticMesh" );
    RootComponent = MeshComponent;

    // Create collision body for mesh component
#if 0
    MeshComponent->bUseDefaultBodyComposition = true;
#else
    FCollisionPlane * collisionBody = NewObject< FCollisionPlane >();
    MeshComponent->BodyComposition.AddCollisionBody( collisionBody );
#endif

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( mesh );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

void FGround::BeginPlay() {
    MeshComponent->SetFriction(2);
}
