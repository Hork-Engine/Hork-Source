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

#include <Engine/Resource/Public/MaterialAssembly.h>
#include <Engine/World/Public/Components/InputComponent.h>

AN_BEGIN_CLASS_META( FPlayer )
AN_END_CLASS_META()

FPlayer::FPlayer() {
    Camera = CreateComponent< FCameraComponent >( "Camera" );
    RootComponent = Camera;

    bCanEverTick = true;

    FMaterialProject * proj = NewObject< FMaterialProject >();

    //
    // gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
    //
    FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * inViewPosition = proj->NewBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->NewBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
    positionMinusViewPosition->ValueB->Connect( inViewPosition, "Value" );
    materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
    FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
    NSV_Dir->Connect( /*positionMinusViewPosition*/inPositionBlock, "Value" );

    FMaterialAtmosphereBlock * atmo = proj->NewBlock< FMaterialAtmosphereBlock >();
    atmo->Dir->Connect( materialVertexStage, "Dir" );

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( atmo, "Result" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->MaterialFacing = MATERIAL_FACE_BACK;
    FMaterial * Material = builder->Build();

    // Create unit box
    FMaterialInstance * minst = NewObject< FMaterialInstance >();
    minst->Material = Material;

    FIndexedMesh * unitBox = NewObject< FIndexedMesh >();
    unitBox->InitializeInternalMesh( "*box*" );

    unitBoxComponent = CreateComponent< FMeshComponent >( "sky_box" );
    unitBoxComponent->SetMesh( unitBox );
    unitBoxComponent->SetMaterialInstance( minst );
    unitBoxComponent->SetScale(4000);
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

    unitBoxComponent->SetPosition(RootComponent->GetPosition());
}

void FPlayer::MoveForward( float _Value ) {
    MoveVector += RootComponent->GetForwardVector() * Float(_Value).Sign();
}

void FPlayer::MoveRight( float _Value ) {
    MoveVector += RootComponent->GetRightVector() * Float(_Value).Sign();
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
    Angles.Pitch = Angles.Pitch.Clamp( -90.0f, 90.0f );
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

class FBoxActor : public FActor {
    AN_ACTOR( FBoxActor, FActor )

protected:
    FBoxActor();

private:
    FMeshComponent * MeshComponent;
};

AN_CLASS_META_NO_ATTRIBS( FBoxActor )

FBoxActor::FBoxActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();;
    matInst->Material = GModule->Material;
    matInst->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );
    matInst->UniformVectors[0] = Float4( FMath::Rand(), FMath::Rand(), FMath::Rand(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< FMeshComponent >( "StaticMesh" );
    RootComponent = MeshComponent;
    MeshComponent->PhysicsBehavior = PB_DYNAMIC;
    MeshComponent->bUseDefaultBodyComposition = true;
    MeshComponent->Mass = 1.0f;

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeSphereMesh" ) );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

void FPlayer::AttackPress() {
    FActor * actor;

    FTransform transform;

    transform.Position = Camera->GetWorldPosition();// + Camera->GetWorldForwardVector() * 1.5f;
    transform.Rotation = Angl( 45.0f, 45.0f, 45.0f ).ToQuat();
    //transform.SetScale( 0.3f );

    actor = GetWorld()->SpawnActor< FBoxActor >( transform );

    FMeshComponent * mesh = actor->GetComponent< FMeshComponent >();
    if ( mesh ) {
        mesh->ApplyCentralImpulse( Camera->GetWorldForwardVector() * 20.0f );
    }
}
