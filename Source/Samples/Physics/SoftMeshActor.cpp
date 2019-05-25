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

#include "SoftMeshActor.h"

#include <Engine/World/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FSoftMeshActor )

FSoftMeshActor::FSoftMeshActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GetResource< FMaterial >( "DefaultMaterial" );
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    FAnchorComponent * Anchor = CreateComponent< FAnchorComponent >( "Anchor" );
    FAnchorComponent * Anchor2 = CreateComponent< FAnchorComponent >( "Anchor2" );
    FAnchorComponent * Anchor3 = CreateComponent< FAnchorComponent >( "Anchor3" );
    FAnchorComponent * Anchor4 = CreateComponent< FAnchorComponent >( "Anchor4" );

    // Create mesh component and set it as root component
    SoftMesh = CreateComponent< FSoftMeshComponent >( "DynamicSphere" );

    SoftMesh->AttachVertex( 0, Anchor );
    SoftMesh->AttachVertex( 16, Anchor2 );
    //SoftMesh->AttachVertex( 16 * 17, Anchor3 );
    //SoftMesh->AttachVertex( 16 * 17 + 16, Anchor4 );

    RootComponent = Anchor;
    Anchor2->AttachTo( Anchor );
    Anchor3->AttachTo( Anchor );
    Anchor4->AttachTo( Anchor );

    Anchor2->SetPosition( Float3( 8, 0, 0 ) );
    Anchor3->SetPosition( Float3( 0, 0, 8 ) );
    Anchor4->SetPosition( Float3( 8, 0, 8 ) );

    SoftMesh->Mass = 1.0f;
    SoftMesh->bSimulatePhysics = true;
    SoftMesh->SetWindVelocity( Float3( 10, 0, 10 ) );

    SoftMesh->BaseTransform = RootComponent->GetWorldTransformMatrix();

    SoftMesh->SetMesh( GetResource< FIndexedMesh >( "SoftmeshPatch" ) );
    SoftMesh->SetMaterialInstance( 0, matInst );
}
