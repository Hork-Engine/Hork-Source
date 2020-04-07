/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <Runtime/Public/Runtime.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/World.h>

AN_BEGIN_CLASS_META( APlayer )
AN_END_CLASS_META()

APlayer::APlayer() {
    static TStaticResourceFinder< AIndexedMesh > CheckerMesh( _CTS( "CheckerMesh" ) );

    Camera = CreateComponent< ACameraComponent >( "Camera" );
    RootComponent = Camera;

    PawnCamera = Camera;

//    Box = CreateComponent< AMeshComponent >( "checker" );
//    Box->SetMesh( CheckerMesh.GetObject() );
//    Box->CopyMaterialsFromMeshResource();
//    Box->SetPosition( 0, 0, -0.5f );
//    Box->SetScale( 0.1f );
//    Box->AttachTo( Camera );

    bCanEverTick = true;

    //static TStaticResourceFinder< AIndexedMesh > UnitBox( _CTS( "/Default/Meshes/Box" ) );
    //static TStaticResourceFinder< AMaterialInstance > SkyboxMaterialInst( _CTS( "SkyboxMaterialInstance" ) );
    //Skybox = CreateComponent< AMeshComponent >( "Skybox" );
    //Skybox->SetMesh( UnitBox.GetObject() );
    //Skybox->SetMaterialInstance( SkyboxMaterialInst.GetObject() );
    //Skybox->ForceOutdoor( true );
    //Skybox->AttachTo( Camera );
    //Skybox->SetAbsoluteRotation( true );
    //Skybox->RenderingOrder = RENDER_ORDER_SKYBOX;
}

void APlayer::BeginPlay() {
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
            Angles.Yaw = Math::Degrees( atan2( projected.X, projected.Y ) ) + 90;
//        }
    } else {
        projected.NormalizeSelf();
        Angles.Yaw = Math::Degrees( atan2( projected.X, projected.Y ) );
    }

    Angles.Pitch = Angles.Roll = 0;

    RootComponent->SetAngles( Angles );
}

void APlayer::EndPlay() {
    Super::EndPlay();
}

void APlayer::SetupPlayerInputComponent( AInputComponent * _Input ) {
    _Input->BindAxis( "MoveForward", this, &APlayer::MoveForward );
    _Input->BindAxis( "MoveRight", this, &APlayer::MoveRight );
    _Input->BindAxis( "MoveUp", this, &APlayer::MoveUp );
    _Input->BindAxis( "MoveDown", this, &APlayer::MoveDown );
    _Input->BindAxis( "TurnRight", this, &APlayer::TurnRight );
    _Input->BindAxis( "TurnUp", this, &APlayer::TurnUp );
    _Input->BindAction( "Speed", IA_PRESS, this, &APlayer::SpeedPress );
    _Input->BindAction( "Speed", IA_RELEASE, this, &APlayer::SpeedRelease );
}

void APlayer::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    constexpr float PLAYER_MOVE_SPEED = 1.5f; // Meters per second
    constexpr float PLAYER_MOVE_HIGH_SPEED = 3.0f;

    const float MoveSpeed = _TimeStep * ( bSpeed ? PLAYER_MOVE_HIGH_SPEED : PLAYER_MOVE_SPEED );
    float lenSqr = MoveVector.LengthSqr();
    if ( lenSqr > 0 ) {
        RootComponent->Step( MoveVector.Normalized() * MoveSpeed );
        MoveVector.Clear();
    }

//    Box->TurnLeftFPS( _TimeStep );

    SWorldRaycastFilter filter;

    filter.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;

    SWorldRaycastClosestResult result;
    if ( GetWorld()->RaycastClosest( result, RootComponent->GetPosition(), RootComponent->GetPosition() + RootComponent->GetForwardVector()*99999.0f, &filter ) ) {
        HitPos = result.TriangleHit.Location;
        HitNormal = result.TriangleHit.Normal;
        Triangle[0] = result.Vertices[0];
        Triangle[1] = result.Vertices[1];
        Triangle[2] = result.Vertices[2];
    }
}

void APlayer::MoveForward( float _Value ) {
    MoveVector += RootComponent->GetForwardVector() * Math::Sign(_Value);
}

void APlayer::MoveRight( float _Value ) {
    MoveVector += RootComponent->GetRightVector() * Math::Sign(_Value);
}

void APlayer::MoveUp( float _Value ) {
    MoveVector.Y += 1;
}

void APlayer::MoveDown( float _Value ) {
    MoveVector.Y -= 1;
}

void APlayer::TurnRight( float _Value ) {
    Angles.Yaw -= _Value;
    Angles.Yaw = Angl::Normalize180( Angles.Yaw );
    RootComponent->SetAngles( Angles );
}

void APlayer::TurnUp( float _Value ) {
    Angles.Pitch += _Value;
    Angles.Pitch = Math::Clamp( Angles.Pitch, -90.0f, 90.0f );
    RootComponent->SetAngles( Angles );
}

void APlayer::SpeedPress() {
    bSpeed = true;
}

void APlayer::SpeedRelease() {
    bSpeed = false;
}

void APlayer::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    InRenderer->SetDepthTest( false );
    InRenderer->SetColor( AColor4( 1, 0, 0, 0.5f ) );
    InRenderer->DrawTriangles( Triangle, 1, sizeof( Float3 ), true );
    InRenderer->SetColor( AColor4( 0, 1, 0, 1 ) );
    InRenderer->DrawLine( HitPos, HitPos + HitNormal * 10 );
}
