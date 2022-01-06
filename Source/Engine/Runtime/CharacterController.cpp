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


// Interaction between ACharacterController and dynamic rigid bodies needs to be explicity implemented by the user.

#include "CharacterController.h"
#include "PhysicalBody.h"
#include "World.h"
#include "RuntimeVariable.h"

#include <float.h>

ARuntimeVariable com_TraceGroundOffset( _CTS( "com_TraceGroundOffset" ), _CTS( "0.01" ) );
ARuntimeVariable com_TraceGroundCylinder( _CTS( "com_TraceGroundCylinder" ), _CTS( "1" ) );

ARuntimeVariable WaterMoveScale( _CTS( "WaterMoveScale" ), _CTS( "0.5" ) );

static const Float3 UpVector( 0, 1, 0 );

AN_CLASS_META( ACharacterController )

ACharacterController::ACharacterController()
{
    LinearVelocity.Clear();
    LandingVelocity = 0;

    MoveForward = 0;
    MoveSide = 0;
    MoveUp = 0;

    WaterLevel = CHARACTER_WATER_LEVEL_NONE;

    TotalForce.Clear();
}

void ACharacterController::BeginPlay()
{
    Super::BeginPlay();

    GetWorld()->E_OnPostPhysicsUpdate.Add( this, &ACharacterController::HandlePostPhysicsUpdate );
}

void ACharacterController::EndPlay()
{
    GetWorld()->E_OnPostPhysicsUpdate.Remove( this );

    Super::EndPlay();
}

void ACharacterController::HandlePostPhysicsUpdate( float TimeStep )
{
    ClearForces();
}

void ACharacterController::DrawDebug( ADebugRenderer * InRenderer )
{
    Super::DrawDebug( InRenderer );

    if ( bTouchGround ) {
        InRenderer->SetColor( Color4(1,0,0,1) );
        InRenderer->DrawLine( GroundPoint, GroundPoint + GroundNormal );
    }
}

void ACharacterController::SetControlMovement( float Forward, float Side, float Up )
{
    MoveForward = Forward;
    MoveSide = Side;
    MoveUp = Up;
}

void ACharacterController::ClearForces()
{
    TotalForce.Clear();
}

void ACharacterController::ApplyCentralForce( Float3 const & _Force )
{
    TotalForce += _Force;
}

void ACharacterController::ApplyCentralImpulse( Float3 const & _Impulse )
{
    LinearVelocity += _Impulse / Mass;
}

bool ACharacterController::ApplyJumpVelocity()
{
    if ( !bTryJump ) {
        return false;
    }

    if ( MoveType != CHARACTER_MOVETYPE_WALK ) {
        return false;
    }

    // Too deep to jump
    if ( WaterLevel > CHARACTER_WATER_LEVEL_WAIST ) {
        return false;
    }

    // Check water jump
    if ( WaterLevel == CHARACTER_WATER_LEVEL_WAIST ) {
        if ( bWaterJump ) {
            return false;
        }

        if ( bTouchGround ) {
            LinearVelocity[1] = WaterJumpVelocity * 0.5f;
            bWaterJump = true;
            bTouchGround = false;
            return true;
        }

        Float3 forward = GetWorldForwardVector();
        forward[1] = 0;
        forward.NormalizeSelf();

        const float sampleRadius = 0.01f;
        Float3 samplePos;

        samplePos = CurrentPosition + forward * ( GetCapsuleRadius() + 0.01f ); // Add little margin to character radius
        samplePos[1] = CurrentPosition[1] + GetCharacterHeight() * 0.4f; //GetEyeHeight();

        SCollisionQueryFilter filter;
        filter.bSortByDistance = false;

        // Try to hit any solid at the front
        filter.CollisionMask = CM_WORLD;
        TPodVector< AHitProxy * > hitProxes;
        GetWorld()->QueryHitProxies( hitProxes, samplePos, sampleRadius, &filter );
        if ( hitProxes.IsEmpty() ) {
            // No solid at the front
            return false;
        }

        // Check empty space
        filter.CollisionMask = CM_ALL & ~CM_TRIGGER;
        samplePos[1] = CurrentPosition[1] + GetCharacterHeight() * 0.8f;
        GetWorld()->QueryHitProxies( hitProxes, samplePos, sampleRadius, &filter );
        if ( !hitProxes.IsEmpty() ) {
            return false;
        }

        // Water jump
        //LinearVelocity = forward * 1.5f;
        LinearVelocity[1] = WaterJumpVelocity;

        bWaterJump = true;

        return true;
    }

    if ( !bTouchGround ) {
        // Can't jump if in air
        return false;
    }

    // Jump
    LinearVelocity[1] = JumpVelocity;
    bTouchGround = false;

    return true;
}

void ACharacterController::ApplyFriction()
{
    Float3 vec = LinearVelocity;

    if ( bTouchGround && MoveType == CHARACTER_MOVETYPE_WALK ) {

        if ( WaterLevel == CHARACTER_WATER_LEVEL_NONE ) {
            vec[1] = 0;
        }
    }

    float speed = vec.Length();
    if ( speed < 0.03f ) {
        LinearVelocity[0] = 0;
        LinearVelocity[2] = 0;
        return;
    }

    float drop = 0;

    if ( MoveType == CHARACTER_MOVETYPE_NOCLIP || MoveType == CHARACTER_MOVETYPE_FLY ) {
        drop += Math::Max( speed, StopSpeed ) * Friction * StepTimeDelta;
    }
    else {
        if ( bTouchGround && WaterLevel <= CHARACTER_WATER_LEVEL_FEET ) {
        //if ( bTouchGround && WaterLevel <= CHARACTER_WATER_LEVEL_WAIST ) {
            drop += Math::Max( speed, StopSpeed ) * Friction * StepTimeDelta;
        }

        if ( WaterLevel > CHARACTER_WATER_LEVEL_NONE ) {
            //drop += Math::Max( speed, /*StopSpeed*/2.0f ) * WaterFriction * WaterLevel * StepTimeDelta;
            drop += speed * WaterFriction * WaterLevel * StepTimeDelta;
        }
    }

    if ( drop > 0.0f ) {
        LinearVelocity = LinearVelocity * Math::Max( 0.0f, 1.0f - drop / speed );
    }
}

float ACharacterController::CalcMoveSpeed() const
{
    float forwardAbs = Math::Abs( MoveForward );
    float sideAbs = Math::Abs( MoveSide );
    float upAbs = Math::Abs( MoveUp );

    float maxSpeed = Math::Max3( forwardAbs, sideAbs, upAbs );
    if ( maxSpeed < FLT_EPSILON ) {
        return 0;
    }

    float moveLength = std::sqrt( MoveForward * MoveForward + MoveSide * MoveSide + MoveUp * MoveUp );

    return maxSpeed / moveLength;
}

Float3 ACharacterController::CalcWishVelocity()
{
    Float3 moveDir;
    float moveSpeed;

    // Calc move direction

    if ( MoveType == CHARACTER_MOVETYPE_NOCLIP || MoveType == CHARACTER_MOVETYPE_FLY ) {
        Float3 forwardVec = GetWorldForwardVector();
        Float3 rightVec = GetWorldRightVector();

        moveDir = forwardVec * MoveForward + rightVec * MoveSide;
        moveDir[1] += MoveUp;
    }
    else {
        if ( WaterLevel == CHARACTER_WATER_LEVEL_NONE ) {
            Float3 forwardVec, rightVec;

            Math::DegSinCos( GetCharacterYaw(), forwardVec[0], rightVec[0] );
            forwardVec[0] = rightVec[2] = -forwardVec[0];
            forwardVec[2] = -rightVec[0];

            // Project to flat plane
            forwardVec[1] = rightVec[1] = 0;
            forwardVec.NormalizeSelf();
            rightVec.NormalizeSelf();

            //if ( bTouchGround )
            //{
            //    forwardVec = Math::ProjectVector( forwardVec, GroundNormal );
            //    rightVec = Math::ProjectVector( rightVec, GroundNormal );
            //    forwardVec.NormalizeSelf();
            //    rightVec.NormalizeSelf();
            //}

            moveDir = forwardVec * MoveForward + rightVec * MoveSide;
        } else {
            Float3 forwardVec = GetWorldForwardVector();
            Float3 rightVec = GetWorldRightVector();

            moveDir = forwardVec * MoveForward + rightVec * MoveSide;

            // Calc move direction
            if ( WaterLevel > CHARACTER_WATER_LEVEL_FEET ) {
                moveDir[1] += MoveUp;
            }
        }
    }

    moveSpeed = CalcMoveSpeed() * ControlSpeed;

    Float3 wishVel = moveDir * moveSpeed;

    // Apply water drift
    if ( WaterLevel >= CHARACTER_WATER_LEVEL_WAIST ) {
        if ( wishVel.LengthSqr() < 0.001f ) {
            wishVel[1] = -WaterDrift;
        }
    }

    // Recalc direction & speed
    moveSpeed = wishVel.NormalizeSelf();

    // Apply water move scale
    if ( WaterLevel > CHARACTER_WATER_LEVEL_NONE ) {
        const float MAX_WATER_LEVEL = (float)CHARACTER_WATER_LEVEL_EYE;
        float scale = 1.0f - (float)WaterLevel / MAX_WATER_LEVEL * (1.0f - WaterMoveScale.GetFloat());
        float maxPlayerSpeed = ControlSpeed * scale;
        if ( moveSpeed > maxPlayerSpeed ) {
            moveSpeed = maxPlayerSpeed;
        }
    }

    wishVel = wishVel * moveSpeed;

    if ( !bTouchGround && MoveType == CHARACTER_MOVETYPE_WALK ) {
        if ( WaterLevel == CHARACTER_WATER_LEVEL_NONE ) {
            wishVel[1] = 0;
        }
    }

    return wishVel;
}

void ACharacterController::ApplyAcceleration( Float3 const & WishVelocity )
{
    Float3 wishDir = WishVelocity;
    float wishSpeed = wishDir.NormalizeSelf();
    float currentSpeed = Math::Dot( LinearVelocity, wishDir );
    float maxAccelSpeed = wishSpeed - currentSpeed;
    if ( maxAccelSpeed > 0.0f ) {
        float acceleration = 0;

        if ( MoveType == CHARACTER_MOVETYPE_WALK ) {
            if ( WaterLevel > CHARACTER_WATER_LEVEL_FEET ) {
                acceleration = SwimAcceleration;
            }
            else {
                acceleration = bTouchGround ? WalkAcceleration : FlyAcceleration;
            }
        }
        else if ( MoveType == CHARACTER_MOVETYPE_FLY ) {
            if ( WaterLevel > CHARACTER_WATER_LEVEL_FEET ) {
                acceleration = SwimAcceleration;
            }
            else {
                acceleration = WalkAcceleration;//FlyAcceleration;
            }
        }
        else if ( MoveType == CHARACTER_MOVETYPE_NOCLIP ) {
            acceleration = WalkAcceleration;
        }

        float accelSpeed = Math::Min( acceleration * wishSpeed * StepTimeDelta, maxAccelSpeed );

        LinearVelocity += wishDir * accelSpeed;
    }
}

void ACharacterController::Update( float TimeDelta )
{
    StepTimeDelta = TimeDelta;

    CurrentPosition = GetWorldPosition();

    //SetCapsuleWorldPosition( CurrentPosition );

    // Integrate velocity
    LinearVelocity += TotalForce * (StepTimeDelta / Mass);

    // Adjust min/max velocity
    for ( int i = 0 ; i < 3 ; i++ ) {
        LinearVelocity[i] = Math::Clamp( LinearVelocity[i], -MaxVelocity, MaxVelocity );
    }

    Float3 startPosition = CurrentPosition;
    Float3 startVelocity = LinearVelocity;

    bool bWasTouchGround = bTouchGround;

    TraceGround();
    UpdateWaterLevel();

    //GLogger.Printf( "Water level %d\n", WaterLevel );

    bLanded = bTouchGround && !bWasTouchGround;

    bool bJustJump = ApplyJumpVelocity();

    UpdateAttach();

    if ( !bWaterJump )
    {
        Float3 wishVelocity = CalcWishVelocity();

        ApplyFriction();
        ApplyAcceleration( wishVelocity );

        if ( bTouchGround && MoveType == CHARACTER_MOVETYPE_WALK )
        {
            if ( ( bLanded && GroundNormal[1] > 0.7f && GroundNormal[1] < 0.99f )
                 || ( WaterLevel >= CHARACTER_WATER_LEVEL_WAIST && Math::Dot( LinearVelocity, GroundNormal ) < 0.0f ) )
                //|| ( WaterLevel > CHARACTER_WATER_LEVEL_WAIST && Math::Dot( LinearVelocity, GroundNormal ) < 0.0f ) )
            {
                if ( WaterLevel == CHARACTER_WATER_LEVEL_NONE || WaterLevel > CHARACTER_WATER_LEVEL_WAIST ) {
                    float vel = LinearVelocity.Length();

                    ClipVelocity( LinearVelocity, GroundNormal, LinearVelocity );

                    if ( LinearVelocity.LengthSqr() > 0.01f ) {
                        LinearVelocity.NormalizeSelf();
                        //LinearVelocity *= vel * 0.99f; // TODO: test
                        LinearVelocity *= vel;
                    }
                    else {
                        LinearVelocity.Clear();
                    }
                }
                else {
                    //    float backoff = Math::Dot( InVelocity, InNormal ) * Overbounce;

                    //    OutVelocity = InVelocity - InNormal * backoff;
                    ClipVelocity( LinearVelocity, GroundNormal, LinearVelocity, 1 );
                }
            }
        }
    }

    if ( MoveType == CHARACTER_MOVETYPE_NOCLIP ) {
        CurrentPosition += LinearVelocity * StepTimeDelta;
    }
    else {
        bool throwOff = !bTouchGround && bWasTouchGround;

        float fallVelocity = 0;

        if ( MoveType == CHARACTER_MOVETYPE_WALK ) {
            fallVelocity = Gravity * StepTimeDelta;
        }

        // Don't apply fall velocity if character is in the water
        if ( WaterLevel >= CHARACTER_WATER_LEVEL_WAIST ) {
            fallVelocity = 0;
        }

        // Don't apply fall velocity if the character has just thrown off the ground
        if ( throwOff ) {
            fallVelocity = 0;
        }
        bool stepdown = true;
        if ( bTouchGround && !LinearVelocity[0] && !LinearVelocity[2] && LinearVelocity[1] <= 0.0f /*&& MoveType == CHARACTER_MOVETYPE_WALK*/ )
        {
            //if ( WaterLevel < CHARACTER_WATER_LEVEL_WAIST ) {
            LinearVelocity[1] = 0;
            //}

            // Standing

            // No step up
            CurrentStepOffset = 0.0f;
        }
        else
        {
            // Don't apply gravity before slide move if character is on ground
            if ( !bTouchGround ) {
                LinearVelocity[1] -= fallVelocity;
            }

            // Try slide move
            Float3 targetPosition;
            Float3 targetVelocity;
            bool clipped;
            SlideMove( CurrentPosition, LinearVelocity, StepTimeDelta, targetPosition, targetVelocity, &clipped );
            if ( !clipped /*|| !bTouchGround*/ /*|| MoveType == CHARACTER_MOVETYPE_FLY*/ ) {
                // Ok
                CurrentPosition = targetPosition;
                LinearVelocity = targetVelocity;

                // No step up
                CurrentStepOffset = 0.0f;
            }
            else {
                // Try step up & slide move

                StepUp();
                SlideMove( CurrentPosition, LinearVelocity, StepTimeDelta, CurrentPosition, LinearVelocity );

                //if ( CurrentStepOffset > 0.0f ) {
                //    SCharacterControllerTrace trace2;
                //    TraceSelf( CurrentPosition, CurrentPosition - Float3( 0, 0.01f, 0 ), UpVector, MaxSlopeCosine, trace2, true );
                //    //TraceSelf( CurrentPosition, CurrentPosition - Float3( 0, CurrentStepOffset, 0 ), trace2, true );
                //    if ( trace2.HasHit() )
                //    {
                //        //if ( trace2.Normal[1] < MaxSlopeCosine ) {
                //            //bTouchGround = false;
                //            //ClipVelocity( LinearVelocity, trace2.Normal, LinearVelocity );

                //            CurrentPosition = targetPosition;
                //            LinearVelocity = targetVelocity;

                //            // No step up
                //            CurrentStepOffset = 0.0f;

                //            stepdown = false;
                //        //}
                //    }
                //}
            }
        }

        // Apply gravity after slide move if the character is on ground
        if ( bTouchGround ) {
            LinearVelocity[1] -= fallVelocity;
        }

        // Try step down
        if( stepdown )
            StepDown();
    }

    if ( CurrentPosition != startPosition ) {
        // Update capsule world position
        SetCapsuleWorldPosition( CurrentPosition );

        // Update scene component world position
        SetWorldPosition( CurrentPosition );
    }

    if ( MoveType != CHARACTER_MOVETYPE_NOCLIP ) {
        RecoverFromPenetration( MaxPenetrationDepth, 4 );
    }

    // Clear insignificant velocities
    for ( int i = 0 ; i < 3 ; i++ ) {
        if ( Math::Abs( LinearVelocity[i] ) < 0.0001f ) {
            LinearVelocity[i] = 0;
        }
    }

    if ( LinearVelocity[1] <= 0.0f )
    {
        bWaterJump = false;
    }

    // Update character state flags
    bJumped = bJustJump && !bTouchGround;
    bLanded = bLanded || (bTouchGround && !bWasTouchGround);
    bThrownOff = !bTouchGround && bWasTouchGround;
    LandingVelocity = bLanded ? startVelocity[1] : 0.0f;

    // Reset try jump
    bTryJump = false;
}

void ACharacterController::UpdateAttach()
{
    if ( MoveType != CHARACTER_MOVETYPE_WALK ) {
        Detach( true );
        return;
    }

    // Try to attach to ground actor
    if ( bTouchGround && GroundNode ) {
        if ( GroundNode->IsPendingKill() ) {
            GroundNode = nullptr;
            Detach( true );
        } else {
            APhysicalBody * physicalBody = Upcast< APhysicalBody >( GroundNode );
            if ( physicalBody && physicalBody->GetMotionBehavior() == MB_KINEMATIC ) {
                AttachTo( GroundNode, nullptr, true );
            }
            else {
                Detach( true );
            }
        }
    }
    else {
        Detach( true );
    }
}

void ACharacterController::TraceGround()
{
    if ( MoveType == CHARACTER_MOVETYPE_NOCLIP ) {
        GroundNode = nullptr;
        bTouchGround = false;
        return;
    }

//    if ( MoveType == CHARACTER_MOVETYPE_FLY ) {
//        GroundNode = nullptr;
//        bTouchGround = false;
//        return;
//    }

    SCharacterControllerTrace trace;

    Float3 targetPosition = CurrentPosition;
    targetPosition[1] -= com_TraceGroundOffset.GetFloat();

    // Use cylinder for tracing the ground to get correct contact normal
    TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace, com_TraceGroundCylinder );
    //Float3 sweepDirNegative( CurrentPosition - targetPosition );
    //TraceSelf( CurrentPosition, targetPosition, sweepDirNegative, 0, trace, com_TraceGroundCylinder );
    //TraceSelf( CurrentPosition, targetPosition, trace, com_TraceGroundCylinder );

    GroundNode = nullptr;

    bTouchGround = trace.HasHit();
    if ( !bTouchGround ) {

        return;
        //Float3 finalPos;
        //SlideMove( CurrentPosition, targetPosition, StepTimeDelta, finalPos );

        //if ( CurrentPosition[1] != finalPos[1] ) {
        //    return;
        //}

        //bTouchGround = true;
    }

    GroundPoint = trace.Position;
    GroundNormal = trace.Normal;

    for ( int i = 0 ; i < 3 ; i++ ) {
        if ( Math::Abs( GroundNormal[i] ) < 0.0001f ) {
            GroundNormal[i] = 0;
        }

        if ( Math::Abs( GroundNormal[i] ) > 0.9999f ) {
            GroundNormal.Clear();
            GroundNormal[i] = 1.0f;
            break;
        }
    }
    GroundNormal.NormalizeSelf();

    GroundNode = trace.HitProxy ? trace.HitProxy->GetOwnerComponent() : nullptr;    

    // check if getting thrown off the ground
    if ( LinearVelocity[1] > 0 && Math::Dot( LinearVelocity, GroundNormal ) > 0.3f ) {
        bTouchGround = false;
        GroundNode = nullptr;
        return;
    }

    // slopes that are too steep will not be considered onground
    //if ( GroundNormal[1] < MaxSlopeCosine ) {
    //    bTouchGround = false;
    //    GroundNode = nullptr;
    //    return;
    //}
}

void ACharacterController::UpdateWaterLevel()
{
    if ( MoveType == CHARACTER_MOVETYPE_NOCLIP ) {
        WaterLevel = CHARACTER_WATER_LEVEL_NONE;
        return;
    }

    SCollisionQueryFilter filter;
    filter.CollisionMask = CM_WATER;
    filter.bSortByDistance = false;

    const float sampleRadius = 0.01f;
    Float3 samplePos = CurrentPosition;

    float y = samplePos[1];

    samplePos[1] = y + EyeHeight;

    TPodVector< AHitProxy * > hitProxes;
    GetWorld()->QueryHitProxies( hitProxes, samplePos, sampleRadius, &filter );
    if ( !hitProxes.IsEmpty() ) {
        WaterLevel = CHARACTER_WATER_LEVEL_EYE;
        return;
    }

    samplePos[1] = y + EyeHeight*0.625f;

    GetWorld()->QueryHitProxies( hitProxes, samplePos, sampleRadius, &filter );
    if ( !hitProxes.IsEmpty() ) {
        WaterLevel = CHARACTER_WATER_LEVEL_WAIST;
        return;
    }

    samplePos[1] = y + 0.03f;

    GetWorld()->QueryHitProxies( hitProxes, samplePos, sampleRadius, &filter );
    if ( !hitProxes.IsEmpty() ) {
        WaterLevel = CHARACTER_WATER_LEVEL_FEET;
        return;
    }

    WaterLevel = CHARACTER_WATER_LEVEL_NONE;
}

void ACharacterController::StepUp()
{
    CurrentStepOffset = 0.0f;

    if ( LinearVelocity[1] > 0.0f ) {
        return;
    }

    if ( StepHeight < FLT_EPSILON ) {
        return;
    }

#if 0
    SCharacterControllerTrace downTrace;
    TraceSelf( CurrentPosition, CurrentPosition - UpVector * StepHeight, UpVector, MaxSlopeCosine, downTrace );

    if ( downTrace.HasHit() && Math::Dot( downTrace.Normal, UpVector ) < MaxSlopeCosine ) {
        GLogger.Printf( "What\n" );
    }

    if ( !downTrace.HasHit() ) {
        return;
    }
#endif

    Float3 targetPosition = CurrentPosition;
    targetPosition[1] += StepHeight;

    SCharacterControllerTrace trace;

    TraceSelf( CurrentPosition, targetPosition, -UpVector, MaxSlopeCosine, trace );
    //TraceSelf( CurrentPosition, targetPosition, trace );

    if ( trace.HasHit() ) {
        // Move up only a fraction of the step height
        CurrentStepOffset = StepHeight * trace.Fraction;
        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );
    }
    else {
        // Full step up
        CurrentStepOffset = StepHeight;
        CurrentPosition[1] = targetPosition[1];
    }
}
#if 0
void ACharacterController::StepDown()
{
    if ( WaterLevel > CHARACTER_WATER_LEVEL_WAIST ) {
        if ( CurrentStepOffset < FLT_EPSILON ) {
            return;
        }

        Float3 targetPosition = CurrentPosition;
        targetPosition[1] -= CurrentStepOffset;

        SCharacterControllerTrace trace;
        //TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
        TraceSelf( CurrentPosition, targetPosition, UpVector, 0.1f, trace );
        //TraceSelf( CurrentPosition, targetPosition, trace );
        if ( !trace.HasHit() ) {
            CurrentPosition[1] = targetPosition[1];
            return;
        }

        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );

        //float vel = LinearVelocity.Length();
        ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
        //LinearVelocity.NormalizeSelf();
        //LinearVelocity *= vel;
//        } else {
//            LinearVelocity.Clear();
//        }

        return;
    }

    float verticalSpeed = 0;// -LinearVelocity[1]; // FIXME: Is it correct to apply vertical velocity here?

    float stepDown = CurrentStepOffset + StepHeight + verticalSpeed * StepTimeDelta;

    if ( Math::Abs( stepDown ) < FLT_EPSILON ) {
        return;
    }

    Float3 targetPosition = CurrentPosition;
    targetPosition[1] -= stepDown;

    SCharacterControllerTrace trace;

    // Try a full step down
    //TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
    TraceSelf( CurrentPosition, targetPosition, UpVector, 0.01f, trace );

    if ( !trace.HasHit() ) {
        // No hit, player falls
        CurrentPosition[1] -= CurrentStepOffset;

        //targetPosition = CurrentPosition;
        //targetPosition[1] -= CurrentStepOffset;
        //SlideMove( CurrentPosition, targetPosition, StepTimeDelta, CurrentPosition );
        return;
    }

    if ( bTouchGround ) {
        // Can step down
        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );
        //LinearVelocity[1] = 0;

        if ( trace.Normal[1] < MaxSlopeCosine ) {
            //bTouchGround = false;
            ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
        }
        else {
            LinearVelocity[1] = 0;
        }

        //ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );

        //if ( trace.Normal[1] < MaxSlopeCosine ) {
        //    ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
        //}
        //else {
        //    LinearVelocity[1] = 0;
        //}
        return;
    }

    if ( CurrentStepOffset <= 0.0f ) {
        // No step up
        return;
    }

    // Try step down with little offset
    stepDown = CurrentStepOffset + 0.01;
    targetPosition[1] = CurrentPosition[1] - stepDown;

    //TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
    TraceSelf( CurrentPosition, targetPosition, UpVector, 0.01f, trace );
    //TraceSelf( CurrentPosition, targetPosition, trace );
    if ( trace.HasHit() ) {
        // Landed
        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );

        if ( trace.Normal[1] < MaxSlopeCosine ) {
            //bTouchGround = false;
            ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
        }

        //LinearVelocity[1] = 0;
        //ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
    }
    else {
        // Still falling
        CurrentPosition[1] -= CurrentStepOffset;
    }
}

#endif




void ACharacterController::StepDown()
{
    if ( WaterLevel > CHARACTER_WATER_LEVEL_WAIST ) {
        if ( CurrentStepOffset < FLT_EPSILON ) {
            return;
        }

        Float3 targetPosition = CurrentPosition;
        targetPosition[1] -= CurrentStepOffset;

        SCharacterControllerTrace trace;
        //TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
        TraceSelf( CurrentPosition, targetPosition, UpVector, 0.1f, trace );
        //TraceSelf( CurrentPosition, targetPosition, trace );
        if ( !trace.HasHit() ) {
            CurrentPosition[1] = targetPosition[1];
            return;
        }

        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );

        //float vel = LinearVelocity.Length();
        ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
        //LinearVelocity.NormalizeSelf();
        //LinearVelocity *= vel;
//        } else {
//            LinearVelocity.Clear();
//        }

        return;
    }

    float stepDown = CurrentStepOffset + StepHeight;

    if ( Math::Abs( stepDown ) < FLT_EPSILON ) {
        return;
    }

    Float3 targetPosition = CurrentPosition;
    targetPosition[1] -= stepDown;

    SCharacterControllerTrace trace;

    const float stepDownMaxSlope = MaxSlopeCosine;

    // Try a full step down
    TraceSelf( CurrentPosition, targetPosition, UpVector, stepDownMaxSlope, trace );
    //TraceSelf( CurrentPosition, targetPosition, trace );

    if ( !trace.HasHit() ) {
        // No hit, player falls
        CurrentPosition[1] -= CurrentStepOffset;
        return;
    }

    if ( bTouchGround && MoveType == CHARACTER_MOVETYPE_WALK ) {
        // Can step down

        // Calc actual step down
        //TraceSelf( CurrentPosition, targetPosition, trace );

        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );
        LinearVelocity[1] = 0;
        //ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
        return;
    }

    if ( CurrentStepOffset <= 0.0f ) {
        // No step up
        return;
    }

    // Try step down with little offset
    stepDown = CurrentStepOffset + 0.01;
    targetPosition[1] = CurrentPosition[1] - stepDown;

    TraceSelf( CurrentPosition, targetPosition, UpVector, stepDownMaxSlope, trace );
    //TraceSelf( CurrentPosition, targetPosition, trace );
    if ( trace.HasHit() ) {
        // Landed
        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );
    }
    else {
        // Still falling
        CurrentPosition[1] -= CurrentStepOffset;
    }
}


















#if 0


static Float3 computeReflectionDirection( const Float3& direction, const Float3& normal )
{
    return direction - (2.0f * Math::Dot( direction, normal )) * normal;
}

// Returns the portion of 'direction' that is parallel to 'normal'
static Float3 parallelComponent( const Float3& direction, const Float3& normal )
{
    float magnitude = Math::Dot( direction, normal );
    return normal * magnitude;
}

// Returns the portion of 'direction' that is perpindicular to 'normal'
static Float3 perpindicularComponent( const Float3& direction, const Float3& normal )
{
    return direction - parallelComponent( direction, normal );
}

static void updateTargetPositionBasedOnCollision( Float3 const & currentPosition, Float3 & targetPosition, Float3 const & hitNormal, float tangentMag, float normalMag )
{
    Float3 movementDirection = targetPosition - currentPosition;
    float movementLength = movementDirection.NormalizeSelf();

    if ( movementLength > FLT_EPSILON )
    {
        Float3 reflectDir = computeReflectionDirection( movementDirection, hitNormal );
        reflectDir.NormalizeSelf();

        //Float3 parallelDir = parallelComponent( reflectDir, hitNormal );
        Float3 perpindicularDir = perpindicularComponent( reflectDir, hitNormal );

        targetPosition = currentPosition;
        //if ( tangentMag != 0.0)
        //{
        //    Float3 parComponent = parallelDir * ( tangentMag * movementLength );
        //    //			printf("parComponent=%f,%f,%f\n",parComponent[0],parComponent[1],parComponent[2]);
        //    targetPosition += parComponent;
        //}

        if ( normalMag != 0.0 )
        {
            Float3 perpComponent = perpindicularDir * ( normalMag * movementLength );
            //			printf("perpComponent=%f,%f,%f\n",perpComponent[0],perpComponent[1],perpComponent[2]);
            targetPosition += perpComponent;
        }
    }
}

void ACharacterController::Slide()
{
    Float3 walkMove = m_linearVelocity * StepTimeDelta;
    Float3 normalizedDirection = walkMove.Normalized();    
    Float3 targetPosition = m_currentPosition + walkMove;

    float fraction = 1;

    int maxIter = 10;
    int numIter = 0;

    while ( fraction > 0.01f && maxIter-- > 0 )
    {
        numIter++;

        Float3 startPos = m_currentPosition;

        SCharacterControllerTrace trace;
        trace.Fraction = 1;

        if ( !(startPos == targetPosition) )
        {
            Float3 sweepDirNegative( m_currentPosition - targetPosition );
            TraceSelf( startPos, targetPosition, sweepDirNegative, 0, trace );
        }

        fraction -= trace.Fraction;

        if ( trace.HasHit() )
        {
            // we moved only a fraction
            //float hitDistance;
            //hitDistance = (trace.Position - m_currentPosition).length();

            //m_currentPosition.setInterpolate3 (m_currentPosition, targetPosition, trace.Fraction);

            updateTargetPositionBasedOnCollision( m_currentPosition, targetPosition, trace.Normal, 0, 1 );

            clipVelocity( startPos, targetPosition );
#if 1
            Float3 currentDir = targetPosition - m_currentPosition;
            float distance2 = currentDir.LengthSqr();
            if ( distance2 > FLT_EPSILON )
            {
                currentDir.NormalizeSelf();
                /* See Quake2: "If velocity is against original velocity, stop ead to avoid tiny oscilations in sloping corners." */
                if ( Math::Dot( currentDir,normalizedDirection ) <= 0 )
                {
                    //GLogger.Printf( "currentDir.dot( normalizedDirection ) <= 0\n" );

                    targetPosition = m_currentPosition;
                    m_linearVelocity.Clear();
                    break;
                }
            }
            else
            {
                targetPosition = m_currentPosition;

                m_linearVelocity.Clear();
                break;
            }
#endif
        }
        else
        {
            m_currentPosition = targetPosition;

            //clipVelocity( start.getOrigin(), targetPosition );

            break;
        }
    }

    if ( numIter > 1 ) {
        GLogger.Printf("Num iterations %d\n", numIter );
    }

    m_currentPosition = targetPosition;
}

void ACharacterController::clipVelocity( Float3 const & cur, Float3 const & target )
{
    m_linearVelocity = (target - cur)/StepTimeDelta;
}

#endif

#if 0
class ACharacterControllerTraceRayCallback : public btCollisionWorld::RayResultCallback
{
    using Super = btCollisionWorld::RayResultCallback;

public:
    ACharacterControllerTraceRayCallback( btCollisionObject * self, Float3 const & rayFromWorld, Float3 const & rayToWorld )
        : m_rayFromWorld( btVectorToFloat3( rayFromWorld ) )
        , m_rayToWorld( btVectorToFloat3( rayToWorld ) )
        , m_self( self )

    {
        m_collisionFilterGroup = self->getBroadphaseHandle()->m_collisionFilterGroup;
        m_collisionFilterMask = self->getBroadphaseHandle()->m_collisionFilterMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        if ( !Super::needsCollision( proxy0 ) ) {
            return false;
        }

        AHitProxy const * hitProxy0 = static_cast< AHitProxy const * >(m_self->getUserPointer());
        AHitProxy const * hitProxy1 = static_cast< AHitProxy const * >(static_cast< btCollisionObject const * >(proxy0->m_clientObject)->getUserPointer());

        if ( !hitProxy0 || !hitProxy1 ) {
            return true;
        }

        AActor * actor0 = hitProxy0->GetOwnerActor();
        AActor * actor1 = hitProxy1->GetOwnerActor();

        if ( hitProxy0->GetCollisionIgnoreActors().Find( actor1 ) != hitProxy0->GetCollisionIgnoreActors().End() ) {
            return false;
        }

        if ( hitProxy1->GetCollisionIgnoreActors().Find( actor0 ) != hitProxy1->GetCollisionIgnoreActors().End() ) {
            return false;
        }

        return true;
    }

    btScalar addSingleResult( btCollisionWorld::LocalRayResult & RayResult, bool bNormalInWorldSpace ) override
    {
        if ( RayResult.m_collisionObject == m_self )
        {
            return 1;
        }

        if ( !RayResult.m_collisionObject->hasContactResponse() )
        {
            return 1;
        }

        // caller already does the filter on the m_closestHitFraction
        btAssert( RayResult.m_hitFraction <= m_closestHitFraction );

        m_closestHitFraction = RayResult.m_hitFraction;
        m_collisionObject = RayResult.m_collisionObject;
        m_hitNormalWorld = bNormalInWorldSpace
            ? RayResult.m_hitNormalLocal
            : m_collisionObject->getWorldTransform().getBasis()*RayResult.m_hitNormalLocal;
        //m_hitPointWorld.setInterpolate3( m_rayFromWorld, m_rayToWorld, RayResult.m_hitFraction );
        return RayResult.m_hitFraction;
    }

    btVector3 m_rayFromWorld;
    btVector3 m_rayToWorld;
    //btVector3 m_hitPointWorld;
    btVector3 m_hitNormalWorld;
    btCollisionObject * m_self;
};
#endif

#if 0
void ACharacterController::StepDown()
{
    bool runonce = false;

    if ( LinearVelocity[1] > 0.0f )
    {
        return;
    }

    //if ( CurrentStepOffset <= 0.0f )
    //{
    //    return;
    //}

    float verticalSpeed;
    if ( bTouchGround )
    {
        verticalSpeed = LinearVelocity[1] < 0.0f ? -LinearVelocity[1] : 0.0f;
    } else // in air
    {
        verticalSpeed = 0.0f;
    }

    float stepDown = CurrentStepOffset + verticalSpeed * StepTimeDelta;

    if ( Math::Abs( stepDown ) < FLT_EPSILON )
    {
        return;
    }

    Float3 step_drop = UpVector * stepDown;
    Float3 targetPosition = CurrentPosition - step_drop;

    SCharacterControllerTrace trace, trace2;
    trace.Fraction = 1;
    trace2.Fraction = 1;

    while ( 1 )
    {
        TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
        if ( !trace.HasHit() )
        {
            //set double test for 2x the step drop, to check for a large drop vs small drop
            Float3 target2 = targetPosition - step_drop;

            TraceSelf( CurrentPosition, target2, UpVector, MaxSlopeCosine, trace2 );
        }

        float downOffset = verticalSpeed * StepTimeDelta;
        bool has_hit = trace2.HasHit();

        float stepHeight = 0.0f;
        if ( LinearVelocity[1] < 0.0 )
            stepHeight = StepHeight;

        if ( downOffset > 0.0f && downOffset < stepHeight && has_hit && !runonce && bTouchGround )
        {
            //redo the velocity calculation when falling a small amount, for fast stairs motion
            //for larger falls, use the smoother/slower interpolated movement by not touching the target position

            stepDown = CurrentStepOffset + stepHeight;
            if ( Math::Abs( stepDown ) < FLT_EPSILON )
            {
                return;
            }

            step_drop = UpVector * stepDown;
            targetPosition = CurrentPosition - step_drop;
            runonce = true;

            continue;  //re-run previous tests
        }

        break;
    }


    if ( trace.HasHit() || runonce )
    {
        // slopes that are too steep will not be considered onground
        //if ( m_hasGroundNormal && m_groundNormal[1] < 0.7f/*MIN_WALK_NORMAL*/ ) {
        //}
        //else {
        CurrentPosition = Math::Lerp( CurrentPosition, targetPosition, trace.Fraction );
        LinearVelocity[1] = 0;
        bTouchGround = true;



#if 0
        m_hasGroundNormal = false;
        Float3 groundPoint = btVectorToFloat3( callback.m_hitPointWorld );
        Float3 targetPos = groundPoint - UpVector * com_TraceGroundOffset.GetFloat();

        ACharacterControllerTraceRayCallback hitResult( GhostObject, groundPoint, targetPos );

        World->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );
        if ( hitResult.hasHit() ) {
            m_groundNormal = btVectorToFloat3( hitResult.m_hitNormalWorld );
            m_hasGroundNormal = true;
        }

        // check if getting thrown off the ground
        if ( m_hasGroundNormal && m_linearVelocity[1] > 0 && Math::Dot( m_linearVelocity * 32, m_groundNormal ) > 10 ) {
            m_touchGround = false;
            //return;
        }

        // slopes that are too steep will not be considered onground
        if ( m_hasGroundNormal && m_groundNormal[1] < 0.7f/*MIN_WALK_NORMAL*/ ) {
            m_touchGround = false;
            //return;
        }

        if ( m_hasGroundNormal ) {
            float vel = m_linearVelocity.Length();

            PM_ClipVelocity( m_linearVelocity, m_groundNormal, m_linearVelocity, 1.0003f );

            if ( m_linearVelocity.LengthSqr() > 0.01f ) {
                m_linearVelocity.NormalizeSelf();
                m_linearVelocity *= vel;
            } else {
                m_linearVelocity.Clear();
            }
        }
#endif
        //}
    } else
    {
        CurrentPosition = targetPosition;
    }
}
#endif

// Landing velocity from q3
#if 0
    float dist = CurrentPosition[ 1 ] - startPosition[ 1 ];
    float vel = startVelocity[ 1 ];
    float acc = -Gravity;
    float a = acc / 2;
    float b = vel;
    float c = -dist;
    float den = b * b - 4 * a * c;
    if ( den < 0 ) {
        LandingVelocity = 0;
    }
    else {
        float t = ( -b - sqrt( den ) ) / ( 2 * a );
        LandingVelocity = vel + t * acc;
        //LandingVelocity = LandingVelocity*LandingVelocity*0.0001;
    }
#endif







#if 0
void ACharacterController::StepDown()
{
    if ( WaterLevel > CHARACTER_WATER_LEVEL_WAIST ) {
        if ( CurrentStepOffset < FLT_EPSILON ) {
            return;
        }

        Float3 targetPosition = CurrentPosition;
        targetPosition[1] -= CurrentStepOffset;

        SCharacterControllerTrace trace;
        //TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
        TraceSelf( CurrentPosition, targetPosition, UpVector, 0.1f, trace );
        //TraceSelf( CurrentPosition, targetPosition, trace );
        if ( !trace.HasHit() ) {
            CurrentPosition[1] = targetPosition[1];
            return;
        }

        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );

        //float vel = LinearVelocity.Length();
        ClipVelocity( LinearVelocity, trace.Normal, LinearVelocity );
        //LinearVelocity.NormalizeSelf();
        //LinearVelocity *= vel;
//        } else {
//            LinearVelocity.Clear();
//        }

        return;
    }

    float verticalSpeed = 0;// -LinearVelocity[1]; // FIXME: Is it correct to apply vertical velocity here?

    float stepDown = CurrentStepOffset + StepHeight + verticalSpeed * StepTimeDelta;

    if ( Math::Abs( stepDown ) < FLT_EPSILON ) {
        return;
    }

    Float3 targetPosition = CurrentPosition;
    targetPosition[1] -= stepDown;

    SCharacterControllerTrace trace;

    // Try a full step down
    TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
    //TraceSelf( CurrentPosition, targetPosition, trace );

    if ( !trace.HasHit() ) {
        // No hit, player falls
        CurrentPosition[1] -= CurrentStepOffset;
        return;
    }

    if ( bTouchGround ) {
        // Can step down
        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );
        LinearVelocity[1] = 0;
        return;
    }

    if ( CurrentStepOffset <= 0.0f ) {
        // No step up
        return;
    }

    // Try step down with little offset
    stepDown = CurrentStepOffset + 0.01;
    targetPosition[1] = CurrentPosition[1] - stepDown;

    TraceSelf( CurrentPosition, targetPosition, UpVector, MaxSlopeCosine, trace );
    //TraceSelf( CurrentPosition, targetPosition, trace );
    if ( trace.HasHit() ) {
        // Landed
        CurrentPosition[1] = Math::Lerp( CurrentPosition[1], targetPosition[1], trace.Fraction );
    }
    else {
        // Still falling
        CurrentPosition[1] -= CurrentStepOffset;
    }
}
#endif
