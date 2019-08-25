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

#include "Spectator.h"
#include "Game.h"
#include "MyPlayerController.h"

#include <Engine/Resource/Public/MaterialAssembly.h>
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/World.h>

AN_BEGIN_CLASS_META( FSpectator )
AN_END_CLASS_META()

FSpectator::FSpectator() {
    Camera = AddComponent< FCameraComponent >( "Camera" );
    RootComponent = Camera;

    bCanEverTick = true;
    bTickEvenWhenPaused = true;
}

void FSpectator::BeginPlay() {
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

void FSpectator::EndPlay() {
    Super::EndPlay();
}

void FSpectator::SetupPlayerInputComponent( FInputComponent * _Input ) {
    bool bExecuteBindingsWhenPaused = bTickEvenWhenPaused;

    _Input->BindAxis( "MoveForward", this, &FSpectator::MoveForward, bExecuteBindingsWhenPaused );
    _Input->BindAxis( "MoveRight", this, &FSpectator::MoveRight, bExecuteBindingsWhenPaused );
    _Input->BindAxis( "MoveUp", this, &FSpectator::MoveUp, bExecuteBindingsWhenPaused );
    _Input->BindAxis( "MoveDown", this, &FSpectator::MoveDown, bExecuteBindingsWhenPaused );
    _Input->BindAxis( "TurnRight", this, &FSpectator::TurnRight, bExecuteBindingsWhenPaused );
    _Input->BindAxis( "TurnUp", this, &FSpectator::TurnUp, bExecuteBindingsWhenPaused );
    _Input->BindAction( "Speed", IE_Press, this, &FSpectator::SpeedPress, bExecuteBindingsWhenPaused );
    _Input->BindAction( "Speed", IE_Release, this, &FSpectator::SpeedRelease, bExecuteBindingsWhenPaused );
    _Input->BindAction( "SwitchToSpectator", IE_Press, this, &FSpectator::SwitchToAircraft, bExecuteBindingsWhenPaused );
}

void FSpectator::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );
//GLogger.Printf( "%f time\n",_TimeStep);
    constexpr float PLAYER_MOVE_SPEED = 40; // Meters per second
    constexpr float PLAYER_MOVE_HIGH_SPEED = 80;

    const float MoveSpeed = _TimeStep * ( bSpeed ? PLAYER_MOVE_HIGH_SPEED : PLAYER_MOVE_SPEED );
    float lenSqr = MoveVector.LengthSqr();
    if ( lenSqr > 0 ) {

        MoveVector.NormalizeSelf();

        Float3 dir = MoveVector * MoveSpeed;

        RootComponent->Step( dir );

        MoveVector.Clear();
    }

    FWorldRaycastClosestResult result;
    if ( GetWorld()->RaycastClosest( result, RootComponent->GetPosition(), RootComponent->GetPosition() + RootComponent->GetForwardVector()*99999.0f ) ) {
        HitPos = result.Position;
        HitNormal = result.Normal;

        FMeshComponent * mesh = Upcast< FMeshComponent >( result.Object );
        if ( mesh ) {
            Float3x4 const & transform = mesh->GetWorldTransformMatrix();
            Triangle[ 0 ] = transform * mesh->GetMesh()->GetVertices()[ result.TriangleIndices[ 0 ] ].Position;
            Triangle[ 1 ] = transform * mesh->GetMesh()->GetVertices()[ result.TriangleIndices[ 1 ] ].Position;
            Triangle[ 2 ] = transform * mesh->GetMesh()->GetVertices()[ result.TriangleIndices[ 2 ] ].Position;
        }

        //GLogger.Printf("Raycast: %s\n", HitPos.ToString().ToConstChar() );
    }
}

void FSpectator::MoveForward( float _Value ) {
    MoveVector += RootComponent->GetForwardVector() * FMath::Sign(_Value);
}

void FSpectator::MoveRight( float _Value ) {
    MoveVector += RootComponent->GetRightVector() * FMath::Sign(_Value);
}

void FSpectator::MoveUp( float _Value ) {
    MoveVector.Y += 1;//_Value;
}

void FSpectator::MoveDown( float _Value ) {
    MoveVector.Y -= 1;//_Value;
}

void FSpectator::TurnRight( float _Value ) {
    Angles.Yaw -= _Value;
    Angles.Yaw = Angl::Normalize180( Angles.Yaw );
    RootComponent->SetAngles( Angles );
}

void FSpectator::TurnUp( float _Value ) {
    Angles.Pitch += _Value;
    Angles.Pitch = FMath::Clamp( Angles.Pitch, -90.0f, 90.0f );
    RootComponent->SetAngles( Angles );
}

void FSpectator::SpeedPress() {
    bSpeed = true;
}

void FSpectator::SpeedRelease() {
    bSpeed = false;
}

void FSpectator::SwitchToAircraft() {
    GGameModule->PlayerController->SetPawn( GGameModule->Player );
    GGameModule->PlayerController->SetViewCamera( GGameModule->Player->Camera );
}

void FSpectator::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    _DebugDraw->SetDepthTest( false );
    //_DebugDraw->SetColor(1,0,0,1);
    //_DebugDraw->DrawBoxFilled( HitPos, Float3(0.2f), true );
    _DebugDraw->SetColor( FColor4( 1, 0, 0, 0.5f ) );
    _DebugDraw->DrawTriangles( Triangle, 1, sizeof( Float3 ), true );

    _DebugDraw->SetColor( FColor4( 0, 1, 0, 1 ) );
    _DebugDraw->DrawLine( HitPos, HitPos + HitNormal * 10 );

    //_DebugDraw->DrawLine( RootComponent->GetPosition(), RootComponent->GetPosition() + RootComponent->GetForwardVector()*99999.0f );
}
