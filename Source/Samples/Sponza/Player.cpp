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

#include "Player.h"

#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/World/Public/Components/InputComponent.h>

AN_BEGIN_CLASS_META( FPlayer )
AN_END_CLASS_META()

FPlayer::FPlayer() {
    Camera = AddComponent< FCameraComponent >( "Camera" );
    RootComponent = Camera;

    bCanEverTick = true;

    // Create skybox
    static TStaticInternalResourceFinder< FIndexedMesh > UnitBox( _CTS( "FIndexedMesh.Box" ) );
    static TStaticResourceFinder< FMaterialInstance > SkyboxMaterialInst( _CTS( "SkyboxMaterialInstance" ) );
    SkyboxComponent = AddComponent< FMeshComponent >( "Skybox" );
    SkyboxComponent->SetMesh( UnitBox.GetObject() );
    SkyboxComponent->SetMaterialInstance( SkyboxMaterialInst.GetObject() );
    SkyboxComponent->AttachTo( Camera );
    SkyboxComponent->SetAbsoluteRotation( true );
    SkyboxComponent->RenderingOrder = RENDER_ORDER_SKYBOX;
}

void FPlayer::BeginPlay() {
    Super::BeginPlay();

    Float3 vec = RootComponent->GetBackVector();
    Float2 projected( vec.X, vec.Z );
    float lenSqr = projected.LengthSqr();
    if ( lenSqr < 0.0001 ) {
        vec = RootComponent->GetRightVector();
        projected.X = vec.X;
        projected.Y = vec.Z;
//        float lenSqr = projected.LengthSqr();
//        if ( lenSqr < 0.0001 ) {

//        } else {
            projected.NormalizeSelf();
            Angles.Yaw = FMath::Degrees( atan2( projected.X, projected.Y ) ) + 90;
//        }
    } else {
        projected.NormalizeSelf();
        Angles.Yaw = FMath::Degrees( atan2( projected.X, projected.Y ) );
    }

    Angles.Pitch = Angles.Roll = 0;

    RootComponent->SetAngles( Angles );
}

void FPlayer::EndPlay() {
    Super::EndPlay();
}

void FPlayer::SetupPlayerInputComponent( FInputComponent * _Input ) {
    _Input->BindAxis( "MoveForward", this, &FPlayer::MoveForward );
    _Input->BindAxis( "MoveRight", this, &FPlayer::MoveRight );
    _Input->BindAxis( "MoveUp", this, &FPlayer::MoveUp );
    _Input->BindAxis( "MoveDown", this, &FPlayer::MoveDown );
    _Input->BindAxis( "TurnRight", this, &FPlayer::TurnRight );
    _Input->BindAxis( "TurnUp", this, &FPlayer::TurnUp );
    _Input->BindAction( "Speed", IE_Press, this, &FPlayer::SpeedPress );
    _Input->BindAction( "Speed", IE_Release, this, &FPlayer::SpeedRelease );
    _Input->BindAction( "Attack", IE_Press, this, &FPlayer::AttackPress );
}

void FPlayer::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    constexpr float PLAYER_MOVE_SPEED = 4; // Meters per second
    constexpr float PLAYER_MOVE_HIGH_SPEED = 8;

    const float MoveSpeed = _TimeStep * ( bSpeed ? PLAYER_MOVE_HIGH_SPEED : PLAYER_MOVE_SPEED );
    float lenSqr = MoveVector.LengthSqr();
    if ( lenSqr > 0 ) {

        //if ( lenSqr > 1 ) {
            MoveVector.NormalizeSelf();
        //}

        Float3 dir = MoveVector * MoveSpeed;

        RootComponent->Step( dir );

        MoveVector.Clear();
    }

    //unitBoxComponent->SetPosition(RootComponent->GetPosition());
}

void FPlayer::MoveForward( float _Value ) {
    MoveVector += RootComponent->GetForwardVector() * FMath::Sign(_Value);
}

void FPlayer::MoveRight( float _Value ) {
    MoveVector += RootComponent->GetRightVector() * FMath::Sign(_Value);
}

void FPlayer::MoveUp( float _Value ) {
    MoveVector.Y += 1;//_Value;
}

void FPlayer::MoveDown( float _Value ) {
    MoveVector.Y -= 1;//_Value;
}

void FPlayer::TurnRight( float _Value ) {
    Angles.Yaw -= _Value;
    Angles.Yaw = Angl::Normalize180( Angles.Yaw );
    RootComponent->SetAngles( Angles );
}

void FPlayer::TurnUp( float _Value ) {
    Angles.Pitch += _Value;
    Angles.Pitch = FMath::Clamp( Angles.Pitch, -90.0f, 90.0f );
    RootComponent->SetAngles( Angles );
}

void FPlayer::SpeedPress() {
    bSpeed = true;
}

void FPlayer::SpeedRelease() {
    bSpeed = false;
}




#include"SponzaModel.h"
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/World/Public/World.h>

class FSphereActor : public FActor {
    AN_ACTOR( FSphereActor, FActor )

protected:
    FSphereActor();

private:
    FMeshComponent * MeshComponent;
};

#include <Engine/MaterialGraph/Public/MaterialGraph.h>
AN_CLASS_META( FSphereActor )

static FMaterial * GetOrCreateSphereMaterial() {
    static FMaterial * material = nullptr;
    if ( !material )
    {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();
        MGInTexCoord * inTexCoordBlock = graph->AddNode< MGInTexCoord >();
        MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();
        MGNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );
        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
        MGSampler * diffuseSampler = graph->AddNode< MGSampler >();
        diffuseSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );
        MGUniformAddress * uniformAddress = graph->AddNode< MGUniformAddress >();
        uniformAddress->Address = 0;
        uniformAddress->Type = AT_Float4;
        MGMulNode * mul = graph->AddNode< MGMulNode >();
        mul->ValueA->Connect( diffuseSampler, "RGBA" );
        mul->ValueB->Connect( uniformAddress, "Value" );
        MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
        materialFragmentStage->Color->Connect( mul, "Result" );

        //MGFloat3Node * normal = graph->AddNode< MGFloat3Node >();
        //normal->Value = Float3(0,0,1);
        MGFloatNode * metallic = graph->AddNode< MGFloatNode >();
        metallic->Value = 0.0f;
        MGFloatNode * roughness = graph->AddNode< MGFloatNode >();
        roughness->Value = 0.1f;

        //materialFragmentStage->Normal->Connect( normal, "Value" );
        materialFragmentStage->Metallic->Connect( metallic, "Value" );
        materialFragmentStage->Roughness->Connect( roughness, "Value" );

        //materialFragmentStage->Ambient->Connect( ambientSampler, "R" );
        //materialFragmentStage->Emissive->Connect( emissiveSampler, "RGBA" );

        FMaterialBuilder * builder = CreateInstanceOf< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_PBR;
        builder->RegisterTextureSlot( diffuseTexture );
        material = builder->Build();
        material->SetName( "SphereMaterial" );
        RegisterResource( material );
    }
    return material;
}

FSphereActor::FSphereActor() {
    static TStaticInternalResourceFinder< FIndexedMesh > MeshResource( _CTS( "FIndexedMesh.Sphere" ) );
    static TStaticResourceFinder< FTexture2D > TextureResource( _CTS( "mipmapchecker.png" ) );

    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();
    matInst->SetMaterial( GetOrCreateSphereMaterial() );
    matInst->SetTexture( 0, TextureResource.GetObject() );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = AddComponent< FMeshComponent >( "StaticMesh" );
    RootComponent = MeshComponent;
    MeshComponent->PhysicsBehavior = PB_DYNAMIC;
    MeshComponent->bUseDefaultBodyComposition = true;
    MeshComponent->Mass = 1.0f;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( MeshResource.GetObject() );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

void FPlayer::AttackPress() {
    FActor * actor;

    FTransform transform;

    transform.Position = Camera->GetWorldPosition();// + Camera->GetWorldForwardVector() * 1.5f;
    transform.Rotation = Angl( 45.0f, 45.0f, 45.0f ).ToQuat();
    //transform.SetScale( 0.3f );

    actor = GetWorld()->SpawnActor< FSphereActor >( transform );

    FMeshComponent * mesh = actor->GetComponent< FMeshComponent >();
    if ( mesh ) {
        mesh->ApplyCentralImpulse( Camera->GetWorldForwardVector() * 20.0f );
    }
}
