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

#include <Engine/World/Public/MaterialAssembly.h>
#include <Engine/World/Public/InputComponent.h>
#include <Engine/World/Public/ResourceManager.h>

AN_BEGIN_CLASS_META( FPlayer )
AN_END_CLASS_META()

FPlayer::FPlayer() {
    Camera = CreateComponent< FCameraComponent >( "Camera" );

    bCanEverTick = true;
    bTickPrePhysics = true;

    // Create unit box
    FMaterialInstance * minst = NewObject< FMaterialInstance >();
    minst->Material = GetResource< FMaterial >( "SkyboxMaterial" );

    unitBoxComponent = CreateComponent< FMeshComponent >( "sky_box" );
    unitBoxComponent->SetMesh( GetResource< FIndexedMesh >( "UnitBox" ) );
    unitBoxComponent->SetMaterialInstance( minst );
    unitBoxComponent->SetScale(4000);

    FCollisionCapsule * capsule = NewObject< FCollisionCapsule >();
    capsule->Radius = 0.6f;
    capsule->Height = 0.7f;
    PhysBody = CreateComponent< FPhysicalBody >( "PlayerCapsule" );
    PhysBody->BodyComposition.AddCollisionBody( capsule );
    PhysBody->Mass = 70.0f;
    PhysBody->bKinematicBody = false;
    PhysBody->bDisableGravity = false;
    PhysBody->bSimulatePhysics = true;

    RootComponent = PhysBody;

    Camera->SetPosition( 0, 0.7f, 0.0f );
    Camera->AttachTo( RootComponent );
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

    //Angles.Pitch = Angles.Roll = 0;

    RootComponent->SetAngles( 0, 0, 0 );
    Camera->SetAngles( Angles );

    PhysBody->SetAngularFactor( Float3(0.0f) );
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
    _Input->BindAction( "Attack", IE_Release, this, &FPlayer::AttackRelease );
}

void FPlayer::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );
}

void FPlayer::TickPrePhysics( float _TimeStep ) {
    Super::TickPrePhysics( _TimeStep );

    constexpr float PLAYER_MOVE_SPEED = 6; // Meters per second
    constexpr float PLAYER_MOVE_HIGH_SPEED = 10;

    //Float3 Velocity = PhysBody->GetLinearVelocity();
    //PhysBody->ApplyCentralImpulse( -Velocity * 7 );
//GLogger.Printf( "Velocity %s Camera %s\n", Velocity.ToString().ToConstChar(), Camera->GetWorldPosition().ToString().ToConstChar() );
    const float MoveSpeed = ( bSpeed ? PLAYER_MOVE_HIGH_SPEED : PLAYER_MOVE_SPEED );
    float lenSqr = MoveVector.LengthSqr();
    if ( lenSqr > 0 ) {

        //if ( lenSqr > 1 ) {
            MoveVector.NormalizeSelf();
        //}

        Float3 dir = MoveVector;// * MoveSpeed;

        

        //RootComponent->Step( dir );

        //PhysBody->ApplyCentralImpulse( dir*10 );

        const float accelSpeed = _TimeStep * 30;
        const float maxSpeed = MoveSpeed;

        Float3 vel = Float3(Velocity.X + dir.X * accelSpeed,0,Velocity.Z + dir.Z * accelSpeed);
        float len = vel.Length();
        if ( len > maxSpeed ) {
            vel *= maxSpeed / len;
        }

        Velocity.X = vel.X;
        Velocity.Z = vel.Z;

        MoveVector.Clear();
    } else {
        
        Float3 vel = Float3(Velocity.X,0,Velocity.Z);

        const float stopSpeed = _TimeStep * 10;

        vel *= stopSpeed;

        Velocity -= vel;
    }

    //Velocity.Y -= 9.8f * _TimeStep;

    PhysBody->SetLinearVelocity( Velocity );

    unitBoxComponent->SetPosition(RootComponent->GetPosition());
}

void FPlayer::MoveForward( float _Value ) {
    Float3 vec = Camera->GetForwardVector();
    vec.Y = 0;
    vec.NormalizeSelf();
    MoveVector += vec * Float(_Value).Sign();
}

void FPlayer::MoveRight( float _Value ) {
    Float3 vec = Camera->GetRightVector();
    vec.Y = 0;
    vec.NormalizeSelf();
    MoveVector += vec * Float(_Value).Sign();
}

void FPlayer::MoveUp( float _Value ) {
    MoveVector.Y += 1;
}

void FPlayer::MoveDown( float _Value ) {
    MoveVector.Y -= 1;
}

void FPlayer::TurnRight( float _Value ) {
    Angles.Yaw -= _Value;
    Angles.Yaw = Angl::Normalize180( Angles.Yaw );
    Camera->SetAngles( Angles );
}

void FPlayer::TurnUp( float _Value ) {
    Angles.Pitch += _Value;
    Angles.Pitch = Angles.Pitch.Clamp( -90.0f, 90.0f );
    Camera->SetAngles( Angles );
}

void FPlayer::SpeedPress() {
    bSpeed = true;
}

void FPlayer::SpeedRelease() {
    bSpeed = false;
}

#include "StaticMesh.h"
//#include "Module.h"
#include <Engine/World/Public/World.h>
void FPlayer::AttackPress() {
    FActor * actor;

    FTransform transform;

    transform.Position = Camera->GetWorldPosition() + Camera->GetWorldForwardVector() * 1.5f;
    transform.Rotation = Angl( 45.0f, 45.0f, 45.0f ).ToQuat();
    transform.SetScale( 0.6f );

#if 1
    int i = FMath::Rand()*3;

    if ( i == 1 ) {
        transform.Scale.X = 2;
        transform.Scale.Z = 2;
    }

    switch ( i ) {
        case 0: actor = GetWorld()->SpawnActor< FBoxActor >( transform ); break;
        case 1: actor = GetWorld()->SpawnActor< FSphereActor >( transform ); break;
        default: actor = GetWorld()->SpawnActor< FCylinderActor >( transform ); break;
    }
#else
    actor = GetWorld()->SpawnActor< FComposedActor >( transform );
#endif

    FMeshComponent * mesh = actor->GetComponent< FMeshComponent >();
    if ( mesh ) {
        mesh->ApplyCentralImpulse( Camera->GetWorldForwardVector() * 2.0f );
    }
}

void FPlayer::AttackRelease() {
}
