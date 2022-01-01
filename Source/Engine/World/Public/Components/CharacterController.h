/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "CharacterControllerBase.h"

#include <Geometry/Public/Shuffle.h>

enum ECharacterWaterLevel
{
    CHARACTER_WATER_LEVEL_NONE  = 0,
    CHARACTER_WATER_LEVEL_FEET  = 1,
    CHARACTER_WATER_LEVEL_WAIST = 2,
    CHARACTER_WATER_LEVEL_EYE   = 3,
};

enum ECharacterMoveType
{
    CHARACTER_MOVETYPE_WALK,
    CHARACTER_MOVETYPE_FLY,
    CHARACTER_MOVETYPE_NOCLIP,
};

class ACharacterController : public ACharacterControllerBase
{
    AN_COMPONENT( ACharacterController, ACharacterControllerBase )

public:
    void SetMoveType( ECharacterMoveType _MoveType )
    {
        MoveType = _MoveType;
    }

    ECharacterMoveType GetMoveType() const
    {
        return MoveType;
    }

    void SetMass( float _Mass )
    {
        Mass = Math::Max( _Mass, 0.01f );
    }

    float GetMass() const
    {
        return Mass;
    }

    void SetStepHeight( float _StepHeight )
    {
        StepHeight = Math::Max( _StepHeight, 0.0f );
    }

    float GetStepHeight() const
    {
        return StepHeight;
    }

    void SetEyeHeight( float _EyeHeight )
    {
        EyeHeight = _EyeHeight;
    }

    float GetEyeHeight() const
    {
        return EyeHeight;
    }

    void SetGravity( float _Gravity )
    {
        Gravity = Math::Max( _Gravity, 0.0f );
    }

    float GetGravity() const
    {
        return Gravity;
    }

    void SetWaterDrift( float _WaterDrift )
    {
        WaterDrift = _WaterDrift;
    }

    float GetWaterDrift() const
    {
        return WaterDrift;
    }

    /** The max slope determines the maximum angle that the controller can walk up.
    The slope angle is measured in degrees. */
    void SetMaxSlope( float _SlopeDegrees )
    {
        MaxSlopeDegrees = _SlopeDegrees;
        MaxSlopeCosine = Math::Cos( Math::Radians( _SlopeDegrees ) );
    }

    float GetMaxSlope() const
    {
        return MaxSlopeDegrees;
    }

    void SetMaxPenetrationDepth( float _Depth )
    {
        MaxPenetrationDepth = _Depth;
    }

    float GetMaxPenetrationDepth() const
    {
        return MaxPenetrationDepth;
    }

    void SetMaxVelocity( float _MaxVelocity )
    {
        MaxVelocity = Math::Max( _MaxVelocity, 0.0f );
    }

    float GetMaxVelocity() const
    {
        return MaxVelocity;
    }

    void SetJumpVelocity( float _JumpVelocity )
    {
        JumpVelocity = Math::Max( _JumpVelocity, 0.0f );
    }

    float GetJumpVelocity() const
    {
        return JumpVelocity;
    }

    void SetWaterJumpVelocity( float _WaterJumpVelocity )
    {
        WaterJumpVelocity = Math::Max( _WaterJumpVelocity, 0.0f );
    }

    float GetWaterJumpVelocity() const
    {
        return WaterJumpVelocity;
    }

    void SetStopSpeed( float _StopSpeed )
    {
        StopSpeed = Math::Max( _StopSpeed, 0.0f );
    }

    float GetStopSpeed() const
    {
        return StopSpeed;
    }

    void SetFriction( float _Friction )
    {
        Friction = Math::Max( _Friction, 0.0f );
    }

    float GetFriction() const
    {
        return Friction;
    }

    void SetWaterFriction( float _Friction )
    {
        WaterFriction = Math::Max( _Friction, 0.0f );
    }

    float GetWaterFriction() const
    {
        return WaterFriction;
    }

    void SetWalkAcceleration( float _WalkAcceleration )
    {
        WalkAcceleration = Math::Max( _WalkAcceleration, 0.0f );
    }

    float GetWalkAcceleration() const
    {
        return WalkAcceleration;
    }

    void SetSwimAcceleration( float _SwimAcceleration )
    {
        SwimAcceleration = Math::Max( _SwimAcceleration, 0.0f );
    }

    float GetSwimAcceleration() const
    {
        return SwimAcceleration;
    }

    void SetFlyAcceleration( float _FlyAcceleration )
    {
        FlyAcceleration = Math::Max( _FlyAcceleration, 0.0f );
    }

    float GetFlyAcceleration() const
    {
        return FlyAcceleration;
    }

    bool IsOnGround() const
    {
        return bTouchGround;
    }

    bool IsJumped() const
    {
        return bJumped;
    }

    bool IsWaterJump() const
    {
        return bWaterJump;
    }

    bool IsLanded() const
    {
        return bLanded;
    }

    bool IsThrownOff() const
    {
        return bThrownOff;
    }

    void AddLinearVelocity( Float3 const & _Velocity )
    {
        LinearVelocity += _Velocity;
    }

    void SetLinearVelocity( Float3 const & _Velocity )
    {
        LinearVelocity = _Velocity;
    }

    Float3 const & GetLinearVelocity() const
    {
        return LinearVelocity;
    }

    void SetVerticalVelocity( float _Velocity )
    {
        LinearVelocity[1] = _Velocity;
    }

    float GetVerticalVelocity() const
    {
        return LinearVelocity[1];
    }

    float GetLandingVelocity() const
    {
        return LandingVelocity;
    }

    float GetMoveSpeed() const
    {
        return LinearVelocity.Length();
    }

    float GetWalkSpeed() const
    {
        return LinearVelocity.Shuffle2< sXZ >().Length();
    }

    ECharacterWaterLevel GetWaterLevel() const
    {
        return (ECharacterWaterLevel)WaterLevel;
    }

    void SetControlMovement( float Forward, float Side, float Up );

    void SetControlSpeed( float _ControlSpeed )
    {
        ControlSpeed = _ControlSpeed;
    }

    float GetControlSpeed() const
    {
        return ControlSpeed;
    }

    void TryJump()
    {
        bTryJump = true;
    }

    void ClearForces();

    void ApplyCentralForce( Float3 const & _Force );

    void ApplyCentralImpulse( Float3 const & _Impulse );

protected:
    ACharacterController();

    void BeginPlay() override;
    void EndPlay() override;
    void DrawDebug( ADebugRenderer * InRenderer ) override;

    void Update( float TimeDelta ) override;
    void HandlePostPhysicsUpdate( float TimeStep );
    void StepUp();
    void StepDown();
    void TraceGround();
    void UpdateWaterLevel();
    void UpdateAttach();
    bool ApplyJumpVelocity();
    void ApplyFriction();
    void ApplyAcceleration( Float3 const & WishVelocity );
    float CalcMoveSpeed() const;
    Float3 CalcWishVelocity();

    float StepTimeDelta;
    
    float MoveForward;
    float MoveSide;
    float MoveUp;
    int WaterLevel;
    float CurrentStepOffset;
    float LandingVelocity;
    Float3 CurrentPosition;
    Float3 GroundNormal;
    Float3 GroundPoint;
    TRef< ASceneComponent > GroundNode;
    Float3 TotalForce;

    // Attributes
    ECharacterMoveType MoveType = CHARACTER_MOVETYPE_WALK;
    float MaxPenetrationDepth = 0.2f;
    float MaxSlopeDegrees = 45.0f;    // Slope angle that is set (used for returning the exact value)
    float MaxSlopeCosine = 0.7071f;   // Cosine equivalent of MaxSlopeDegrees (calculated once when set, for optimization)
    float Mass = 80.0f;
    float Gravity = 25;
    float WaterDrift = 1.87f;
    float MaxVelocity = 100;
    float StepHeight = 0.5f;
    float EyeHeight = 0.0f;
    float JumpVelocity = 8.5f;
    float WaterJumpVelocity = 10.93f;
    float StopSpeed = 3.125f;
    float Friction = 6.0f;
    float WaterFriction = 1.0f;
    float WalkAcceleration = 10.0f;
    float SwimAcceleration = 4.0f;
    float FlyAcceleration = 1.0f;
    Float3 LinearVelocity;
    float ControlSpeed = 10.0f;

    bool bTouchGround = false;
    bool bJumped = false;
    bool bLanded = false;
    bool bThrownOff = false;
    bool bTryJump = false;
    bool bWaterJump = false;
};
