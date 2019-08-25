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

#pragma once

#include <Engine/World/Public/Actors/Pawn.h>
#include <Engine/World/Public/Components/MeshComponent.h>
#include <Engine/World/Public/Components/CameraComponent.h>

struct trace_t {
    int surfaceFlags;
    PlaneF Plane;

    float fraction;
    Float3 endpos;
    bool allsolid;
    
    FActor * Actor;
};

class FPlayer : public FPawn {
    AN_ACTOR( FPlayer, FPawn )

public:
    FPhysicalBody * PhysBody;
    FCameraComponent * Camera;
    FMeshComponent * unitBoxComponent;
    FActor * Object;

protected:

    FPlayer();
    void BeginPlay() override;
    void EndPlay() override;
    void SetupPlayerInputComponent( FInputComponent * _Input ) override;
    void Tick( float _TimeStep ) override;
    void TickPrePhysics( float _TimeStep ) override;
    void DrawDebug( FDebugDraw * _DebugDraw );

private:
    void MoveForward( float _Value );
    void MoveRight( float _Value );
    void MoveUp( float _Value );
    void MoveDown( float _Value );
    void TurnRight( float _Value );
    void TurnUp( float _Value );
    void SpeedPress();
    void SpeedRelease();
    //void AttackPress();
    //void AttackRelease();
    //void Shoot();
    void SpawnRandomShape();
    void SpawnSoftBody();
    void SpawnComposedActor();

    Angl Angles;
    bool bSpeed;

    void ApplyFriction();
    bool CheckJump();
    void AirMove();
    void WalkMove();
    float ScaleMove();
    void Accelerate( Float3 const & wishdir, float wishspeed, float accel );
    void StepSlideMove( bool gravity );
    bool SlideMove( bool gravity );
    void GroundTrace();
    void GroundTraceMissed();
    void TraceWorld( trace_t * trace, const Float3 & start, const Float3 & end, const char * debug );

    Float3 Velocity;
    float TimeStep;

    float forwardmove;
    float rightmove;
    float upmove;
    Float3 ForwardVec, RightVec;// , UpVec;
    int pm_flags;
    bool bGroundPlane;
    bool bWalking;
    float impactSpeed;
    Float3 Origin;
    trace_t groundTrace;
    Float3 PMins, PMaxs;
    int clientNum;
    int tracemask;
    FActor * groundEntityNum;
};
