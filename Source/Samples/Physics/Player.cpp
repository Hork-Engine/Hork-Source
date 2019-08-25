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
#include "StaticMesh.h"
#include "SoftMeshActor.h"
#include "ComposedActor.h"

#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/Resource/Public/MaterialAssembly.h>
#include <Engine/Resource/Public/ResourceManager.h>

// movement parameters
float pm_stopspeed = 100.0f;
float pm_duckScale = 0.25f;
float pm_swimScale = 0.50f;
float pm_wadeScale = 0.70f;

float pm_accelerate = 10.0f;
float pm_airaccelerate = 1.0f;
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 8.0f;

float pm_friction = 6.0f;
float pm_waterfriction = 1.0f;
float pm_flightfriction = 3.0f;
float pm_spectatorfriction = 5.0f;

int  c_pmove = 0;

#define OVERCLIP 1.001f
#define JUMP_VELOCITY 270
#define STEPSIZE  (18*UNIT_SCALE)
#define GRAVITY 800
#define SURF_SLICK 1
#define MIN_WALK_NORMAL 0.7f  // can't walk on very steep slopes
#define ENTITYNUM_NONE 0
#define UNIT_SCALE (1.0f/32.0f)

AN_BEGIN_CLASS_META( FPlayer )
AN_END_CLASS_META()

FPlayer::FPlayer() {
    Camera = AddComponent< FCameraComponent >( "Camera" );

    bCanEverTick = true;
    bTickPrePhysics = true;

    FCollisionCapsule * capsule = NewObject< FCollisionCapsule >();
    capsule->Radius = 0.6f;
    capsule->Height = 0.7f;
    capsule->Position.Y = capsule->Height * 0.5f + capsule->Radius;

    PhysBody = AddComponent< FPhysicalBody >( "PlayerCapsule" );
    PhysBody->BodyComposition.AddCollisionBody( capsule );
    PhysBody->PhysicsBehavior = PB_KINEMATIC;
    PhysBody->bDisableGravity = true;
    PhysBody->CollisionGroup = CM_PAWN;
    PhysBody->CollisionMask = CM_WORLD | CM_PAWN | CM_PROJECTILE;
    //PhysBody->Mass = 100;

    RootComponent = PhysBody;

    FMaterialInstance * minst = NewObject< FMaterialInstance >();
    minst->Material = GetResource< FMaterial >( "SkyboxMaterial" );

    unitBoxComponent = AddComponent< FMeshComponent >( "sky_box" );
    unitBoxComponent->SetMesh( GetResource< FIndexedMesh >( "ShapeBoxMesh" ) );
    unitBoxComponent->SetMaterialInstance( minst );
    unitBoxComponent->SetScale( 4000 );
    unitBoxComponent->AttachTo( RootComponent );

    

    Camera->SetPosition( 0, 26 * UNIT_SCALE, 0.0f );
    //Camera->SetPosition( 0, 0, 2.0f );
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
        projected.NormalizeSelf();
        Angles.Yaw = FMath::Degrees( atan2( projected.X, projected.Y ) ) + 90;
    } else {
        projected.NormalizeSelf();
        Angles.Yaw = FMath::Degrees( atan2( projected.X, projected.Y ) );
    }

    Angles.Pitch = Angles.Roll = 0;

    RootComponent->SetAngles( 0, 0, 0 );
    Camera->SetAngles( Angles );
    PhysBody->SetAngularFactor( Float3( 0.0f ) );
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
    //_Input->BindAxis( "MoveObjectForward", this, &FPlayer::MoveObjectForward );
    //_Input->BindAxis( "MoveObjectRight", this, &FPlayer::MoveObjectRight );
    _Input->BindAction( "SpawnRandomShape", IE_Press, this, &FPlayer::SpawnRandomShape );
    _Input->BindAction( "SpawnSoftBody", IE_Press, this, &FPlayer::SpawnSoftBody );
    _Input->BindAction( "SpawnComposedActor", IE_Press, this, &FPlayer::SpawnComposedActor );
}

void FPlayer::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );
}

void FPlayer::SpeedPress() {
    bSpeed = true;
}

void FPlayer::SpeedRelease() {
    bSpeed = false;
}

void FPlayer::TraceWorld( trace_t * trace, const Float3 & start, const Float3 & end, const char * debug ) {
    FTraceResult traceResult;

    if ( start.CompareEps( end, 0.0001f ) ) {
        trace->Actor = nullptr;
        trace->endpos = start;
        trace->fraction = 1;
        trace->allsolid = false;
        trace->Plane.Normal.Clear();
        trace->Plane.D = 0;
        trace->surfaceFlags = 0;
        return;
    }

    FCollisionQueryFilter ignoreLists;
    FActor * actors[ 1 ] = { this };

    ignoreLists.IgnoreActors = actors;
    ignoreLists.ActorsCount = 1;
    ignoreLists.CollisionMask = CM_WORLD | CM_PAWN;


    //if ( GetWorld()->TraceCapsule( traceResult, PMins, PMaxs, start, end, 1 ) ) {
    //if ( GetWorld()->TraceCylinder( traceResult, PMins, PMaxs, start, end, 1 ) ) {
    //if ( GetWorld()->TraceSphere( traceResult, 0.9f, start, end, 1 ) ) {
    if ( GetWorld()->TraceBox( traceResult, PMins, PMaxs, start, end, &ignoreLists ) ) {
        if ( traceResult.Body->GetParentActor() == this ) {
            GLogger.Printf( "This actor\n" );
        }
        trace->Actor = traceResult.Body->GetParentActor();
        trace->endpos = start + traceResult.Fraction*( end - start );
        trace->fraction = traceResult.Fraction;

    } else {
        trace->Actor = nullptr;
        trace->endpos = end;
        trace->fraction = 1;
    }

    trace->allsolid = false;
    trace->Plane.Normal = traceResult.Normal;
    trace->Plane.D = traceResult.Distance;
    trace->surfaceFlags = 0;

    //GLogger.Printf( "%s Normal %s\n",debug,traceResult.Normal.ToString().ToConstChar());

#if 0
    moveclip_t clip;
    int   i;

    if ( !mins ) {
        mins = vec3_origin;
    }
    if ( !maxs ) {
        maxs = vec3_origin;
    }

    Com_Memset ( &clip, 0, sizeof ( moveclip_t ) );

    // clip to world
    CM_BoxTrace( &clip.trace, start, end, mins, maxs, 0, contentmask, capsule );
    clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
    if ( clip.trace.fraction == 0 ) {
        *results = clip.trace;
        return;  // blocked immediately by the world
    }

    clip.contentmask = contentmask;
    clip.start = start;
    // VectorCopy( clip.trace.endpos, clip.end );
    clip.end = end;
    clip.mins = mins;
    clip.maxs = maxs;
    clip.passEntityNum = passEntityNum;
    clip.capsule = capsule;

    // create the bounding box of the entire move
    // we can limit it to the part of the move not
    // already clipped off by the world, which can be
    // a significant savings for line of sight and shot traces
    for ( i = 0; i<3; i++ ) {
        if ( end[ i ] > start[ i ] ) {
            clip.boxmins[ i ] = clip.start[ i ] + clip.mins[ i ] - 1;
            clip.boxmaxs[ i ] = clip.end[ i ] + clip.maxs[ i ] + 1;
        } else {
            clip.boxmins[ i ] = clip.end[ i ] + clip.mins[ i ] - 1;
            clip.boxmaxs[ i ] = clip.start[ i ] + clip.maxs[ i ] + 1;
        }
    }

    // clip to other solid entities
    SV_ClipMoveToEntities ( &clip );

    *results = clip.trace;
#endif
}

void FPlayer::ApplyFriction() {
    float control;

    Float3 vec = Velocity;
    if ( bWalking ) {
        vec[ 1 ] = 0; // ignore slope movement
    }

    float speed = vec.Length();
    if ( speed < 1 ) {
        Velocity[ 0 ] = 0;
        Velocity[ 2 ] = 0;  // allow sinking underwater
                            // FIXME: still have z friction underwater?
        return;
    }

    float drop = 0;

    // apply ground friction
    //if ( pm->waterlevel <= 1 ) {
    if ( bWalking && !( groundTrace.surfaceFlags & SURF_SLICK ) ) {
        // if getting knocked back, no friction
        //        if ( !( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
        control = speed < pm_stopspeed ? pm_stopspeed : speed;
        drop += control*pm_friction*TimeStep;
        //        }
        //    }
    }

    // apply water friction even if just wading
    //if ( pm->waterlevel ) {
    //    drop += speed*pm_waterfriction*pm->waterlevel*TimeStep;
    //}

    // apply flying friction
    //if ( pm->ps->powerups[ PW_FLIGHT ] ) {
    //    drop += speed*pm_flightfriction*TimeStep;
    //}

    //if ( pm->ps->pm_type == PM_SPECTATOR ) {
    //    drop += speed*pm_spectatorfriction*TimeStep;
    //}

    // scale the velocity
    float newspeed = speed - drop;
    if ( newspeed < 0 ) {
        newspeed = 0;
    }
    newspeed /= speed;

    Velocity = Velocity * newspeed;
}

float FPlayer::ScaleMove() {
    float max = abs( forwardmove );
    if ( abs( rightmove ) > max ) {
        max = abs( rightmove );
    }
    if ( abs( upmove ) > max ) {
        max = abs( upmove );
    }
    if ( !max ) {
        return 0;
    }

    float total = sqrt( forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );

    return ( float )/*pm->ps->speed*/255.0f * max / ( 127.0 * total );
}

void ClipVelocity( Float3 const & in, Float3 const & normal, Float3 & out, float overbounce ) {
    float backoff = in.Dot( normal );
    if ( backoff < 0 ) {
        backoff *= overbounce;
    } else {
        backoff /= overbounce;
    }
    out = in - normal * backoff;
}

void FPlayer::Accelerate( Float3 const & wishdir, float wishspeed, float accel ) {
    float  addspeed, accelspeed, currentspeed;

    currentspeed = Velocity.Dot( wishdir );
    addspeed = wishspeed - currentspeed;
    if ( addspeed <= 0 ) {
        return;
    }
    accelspeed = accel*TimeStep*wishspeed;
    if ( accelspeed > addspeed ) {
        accelspeed = addspeed;
    }

    Velocity += accelspeed * wishdir;
}

#define MAX_CLIP_PLANES 5
bool FPlayer::SlideMove( bool gravity ) {
    int   bumpcount, numbumps;
    Float3  dir;
    float  d;
    int   numplanes;
    Float3  planes[ MAX_CLIP_PLANES ];
    Float3  primal_velocity;
    Float3  clipVelocity;
    int   i, j, k;
    trace_t     trace;
    Float3  end;
    float  time_left;
    float  into;
    Float3  endVelocity;
    Float3  endClipVelocity;

    numbumps = 4;

    primal_velocity = Velocity;

    if ( gravity ) {
        endVelocity = Velocity;
        endVelocity[ 1 ] -= GRAVITY * TimeStep;
        Velocity[ 1 ] = ( Velocity[ 1 ] + endVelocity[ 1 ] ) * 0.5f;
        primal_velocity[ 1 ] = endVelocity[ 1 ];
        if ( bGroundPlane ) {
            // slide along the ground plane
            ClipVelocity ( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );
        }
    }

    time_left = TimeStep;

    // never turn against the ground plane
    if ( bGroundPlane ) {
        numplanes = 1;
        planes[ 0 ] = groundTrace.Plane.Normal;
    } else {
        numplanes = 0;
    }

    // never turn against original velocity
    float velLenght = Velocity.Length();
    if ( velLenght ) {
        planes[ numplanes ] = Velocity / velLenght;
    } else {
        planes[ numplanes ].Clear();
    }
    numplanes++;

    for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

        // calculate position we are trying to move to
        end = Origin + Velocity * time_left * UNIT_SCALE;

        //GLogger.Printf( "end %s\n", end.ToString().ToConstChar() );

        // see if we can make it there
        TraceWorld ( &trace, Origin, end, "SlideMove" );

        if ( trace.allsolid ) {
            // entity is completely trapped in another solid
            Velocity[ 1 ] = 0; // don't build up falling damage, but allow sideways acceleration
            return true;
        }

        //GLogger.Printf( "fraction %f\n", trace.fraction );

        if ( trace.fraction > 0 ) {
            // actually covered some distance
            Origin = trace.endpos;
            //GLogger.Printf( "Origin = trace.endpos\n" );
        }

        if ( trace.fraction == 1 ) {
            //GLogger.Printf( "fraction == 1\n" );
            break;  // moved the entire distance
        }

        // save entity for contact
        //PM_AddTouchEnt( trace.entityNum );

        time_left -= time_left * trace.fraction;

        if ( numplanes >= MAX_CLIP_PLANES ) {
            // this shouldn't really happen
            Velocity.Clear();
            return true;
        }

        //
        // if this is the same plane we hit before, nudge velocity
        // out along it, which fixes some epsilon issues with
        // non-axial planes
        //
        for ( i = 0; i < numplanes; i++ ) {
            if ( trace.Plane.Normal.Dot( planes[ i ] ) > 0.99 ) {
                Velocity += trace.Plane.Normal;
                break;
            }
        }
        if ( i < numplanes ) {
            continue;
        }
        planes[ numplanes ] = trace.Plane.Normal;
        numplanes++;
#if 1
        //
        // modify velocity so it parallels all of the clip planes
        //

        // find a plane that it enters
        for ( i = 0; i < numplanes; i++ ) {
            into = Velocity.Dot( planes[ i ] );//*UNIT_SCALE;
            if ( into >= 0.1 ) {
                continue;  // move doesn't interact with the plane
            }

            // see how hard we are hitting things
            if ( -into > impactSpeed ) {
                impactSpeed = -into;
            }

            // slide along the plane
            ClipVelocity ( Velocity, planes[ i ], clipVelocity, OVERCLIP );

            // slide along the plane
            ClipVelocity ( endVelocity, planes[ i ], endClipVelocity, OVERCLIP );

            // see if there is a second plane that the new move enters
            for ( j = 0; j < numplanes; j++ ) {
                if ( j == i ) {
                    continue;
                }
                if ( clipVelocity.Dot( planes[ j ] ) >= 0.1 ) {
                    continue;  // move doesn't interact with the plane
                }

                // try clipping the move to the plane
                ClipVelocity( clipVelocity, planes[ j ], clipVelocity, OVERCLIP );
                ClipVelocity( endClipVelocity, planes[ j ], endClipVelocity, OVERCLIP );

                // see if it goes back into the first clip plane
                if ( clipVelocity.Dot( planes[ i ] ) >= 0 ) {
                    continue;
                }

                // slide the original velocity along the crease
                dir = planes[ i ].Cross( planes[ j ] );
                dir.NormalizeSelf();
                d = dir.Dot( Velocity );
                clipVelocity = dir * d;

                dir = planes[ i ].Cross( planes[ j ] );
                dir.NormalizeSelf();
                d = dir.Dot( endVelocity );
                endClipVelocity = dir * d;

                // see if there is a third plane the the new move enters
                for ( k = 0; k < numplanes; k++ ) {
                    if ( k == i || k == j ) {
                        continue;
                    }
                    if ( clipVelocity.Dot( planes[ k ] ) >= 0.1 ) {
                        continue;  // move doesn't interact with the plane
                    }

                    // stop dead at a tripple plane interaction
                    Velocity.Clear();
                    return true;
                }
            }

            // if we have fixed all interactions, try another move
            Velocity = clipVelocity;
            endVelocity = endClipVelocity;
            break;
        }
#endif
    }

    if ( gravity ) {
        Velocity = endVelocity;
    }

    // don't change velocity if in a timer (FIXME: is this correct?)
    //if ( pm->ps->pm_time ) {
    //    Velocity = primal_velocity;
    //}

    return ( bumpcount != 0 );
}
//#pragma warning(disable:4702)
void FPlayer::StepSlideMove( bool gravity ) {
    Float3 start_o = Origin;
    Float3 start_v = Velocity;

    if ( !SlideMove( gravity ) ) {
        return;  // we got exactly where we wanted to go first try 
    }
//return;!!!
    trace_t  trace;
    Float3 down = start_o;
    down[ 1 ] -= STEPSIZE;
    TraceWorld ( &trace, start_o, down, "StepSlideMove1" );
    Float3 up = Float3( 0, 1, 0 );
    // never step up when you still have up velocity
    if ( Velocity[ 1 ] > 0 && ( trace.fraction == 1.0 || trace.Plane.Normal.Dot( up ) < 0.7 ) ) {
        return;
    }

    //    Float3 down_o = Origin;
    //    Float3 down_v = Velocity;

    up = start_o;
    up[ 1 ] += STEPSIZE;

    // test the player position if they were a stepheight higher
    TraceWorld ( &trace, start_o, up, "StepSlideMove2" );
    if ( trace.allsolid ) {
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:bend can't step\n", c_pmove );
        //}
        return;  // can't step up
    }

    float stepSize = trace.endpos[ 1 ] - start_o[ 1 ];
    //if (stepSize<=0.0f)
    //    return;

    //GLogger.Printf("stepsize %f\n",stepSize);
    // try slidemove from this position
    Origin = trace.endpos;
    Velocity = start_v;

    //Float3 o = Origin, v = Velocity;

    SlideMove( gravity );
#if 1
    // push down the final amount
    down = Origin;
    down[ 1 ] -= stepSize;
    TraceWorld ( &trace, Origin, down, "StepSlideMove3" );

    if ( !trace.allsolid ) {
        Origin = trace.endpos;
    }

    if ( trace.fraction < 1.0 ) {
        ClipVelocity( Velocity, trace.Plane.Normal, Velocity, OVERCLIP );
    }
#endif
#if 0
    // if the down trace can trace back to the original position directly, don't step
    TraceWorld( &trace, Origin, start_o );
    if ( trace.fraction == 1.0 ) {
        // use the original move
        Origin = down_o;
        Velocity = down_v; a
            //if ( pm->debugLevel ) {
            //    Com_Printf( "%i:bend\n", c_pmove );
            //}
    } else
#endif
    {
        //    // use the step move
        //    float delta;

        //    delta = Origin[ 1 ] - start_o[ 1 ];
        //    if ( delta > 2 ) {
        //        if ( delta < 7 ) {
        //            PM_AddEvent( EV_STEP_4 );
        //        } else if ( delta < 11 ) {
        //            PM_AddEvent( EV_STEP_8 );
        //        } else if ( delta < 15 ) {
        //            PM_AddEvent( EV_STEP_12 );
        //        } else {
        //            PM_AddEvent( EV_STEP_16 );
        //        }
        //    }
        //    if ( pm->debugLevel ) {
        //        Com_Printf( "%i:stepped\n", c_pmove );
        //    }
    }
}

#define PMF_JUMP_HELD 1

bool FPlayer::CheckJump() {
    //if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
    //    return qfalse;  // don't allow jump until all buttons are up
    //}

    if ( upmove < 10 ) {
        // not holding jump
        return false;
    }

    // must wait for jump to be released
    if ( pm_flags & PMF_JUMP_HELD ) {
        // clear upmove so cmdscale doesn't lower running speed
        upmove = 0;
        return false;
    }

    bGroundPlane = false;  // jumping away
    bWalking = false;
    pm_flags |= PMF_JUMP_HELD;

    groundEntityNum = ENTITYNUM_NONE;
    Velocity[ 1 ] = JUMP_VELOCITY;
    //PM_AddEvent( EV_JUMP );

    //if ( forwardmove >= 0 ) {
    //    PM_ForceLegsAnim( LEGS_JUMP );
    //    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
    //} else {
    //    PM_ForceLegsAnim( LEGS_JUMPB );
    //    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
    //}

    GLogger.Printf( "Jump\n" );

    return true;
}

void FPlayer::AirMove() {
    Float3  wishvel;
    float  fmove, smove;
    Float3  wishdir;
    float  wishspeed;
    float  scale;

    ApplyFriction();

    fmove = forwardmove;
    smove = rightmove;

    scale = ScaleMove();

    // set the movementDir so clients can rotate the legs for strafing
    //PM_SetMovementDir();

    // project moves down to flat plane
    FMath::DegSinCos( Angles.Yaw, ForwardVec[ 0 ], RightVec[ 0 ] );
    ForwardVec[ 0 ] = RightVec[ 2 ] = -ForwardVec[ 0 ];
    ForwardVec[ 2 ] = -RightVec[ 0 ];
    ForwardVec[ 1 ] = RightVec[ 1 ] = 0;

    wishvel = ForwardVec * fmove + RightVec * smove;
    wishvel[ 1 ] = 0;

    wishdir = wishvel;
    wishspeed = wishdir.NormalizeSelf();
    wishspeed *= scale;

    // not on ground, so little effect on velocity
    Accelerate ( wishdir, wishspeed, pm_airaccelerate );

    // we may have a ground plane that is very steep, even
    // though we don't have a groundentity
    // slide along the steep plane
    if ( bGroundPlane ) {
        ClipVelocity ( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );
    }

#if 0
    //ZOID:  If we are on the grapple, try stair-stepping
    //this allows a player to use the grapple to pull himself
    //over a ledge
    if ( pm->ps->pm_flags & PMF_GRAPPLE_PULL )
        StepSlideMove ( qtrue );
    else
        SlideMove ( qtrue );
#endif

    //GLogger.Printf( "Airmove vel %s\n", Velocity.ToString().ToConstChar() );
    StepSlideMove ( true );
}

void FPlayer::WalkMove() {

    if ( CheckJump () ) {
        // jumped away
        //if ( pm->waterlevel > 1 ) {
        //    PM_WaterMove();
        //} else {
        AirMove();
        //}
        return;
    }

    ApplyFriction();

    float fmove = forwardmove;
    float smove = rightmove;

    float scale = ScaleMove();

    // set the movementDir so clients can rotate the legs for strafing
    //PM_SetMovementDir();

    // project moves down to flat plane
    FMath::DegSinCos( Angles.Yaw, ForwardVec[ 0 ], RightVec[ 0 ] );
    ForwardVec[ 0 ] = RightVec[ 2 ] = -ForwardVec[ 0 ];
    ForwardVec[ 2 ] = -RightVec[ 0 ];
    ForwardVec[ 1 ] = RightVec[ 1 ] = 0;

    // project the forward and right directions onto the ground plane
    ClipVelocity ( ForwardVec, groundTrace.Plane.Normal, ForwardVec, OVERCLIP );
    ClipVelocity ( RightVec, groundTrace.Plane.Normal, RightVec, OVERCLIP );

    ForwardVec.NormalizeSelf();
    RightVec.NormalizeSelf();

    Float3 wishvel = ForwardVec * fmove + RightVec * smove;
    // when going up or down slopes the wish velocity should Not be zero
    // wishvel[1] = 0;

    Float3 wishdir = wishvel;
    float wishspeed = wishdir.NormalizeSelf();
    //wishspeed = VectorNormalize( wishdir );
    wishspeed *= scale;

    // clamp the speed lower if ducking
    //if ( pm->ps->pm_flags & PMF_DUCKED ) {
    //    if ( wishspeed > pm->ps->speed * pm_duckScale ) {
    //        wishspeed = pm->ps->speed * pm_duckScale;
    //    }
    //}

    // clamp the speed lower if wading or walking on the bottom
    //if ( pm->waterlevel ) {
    //    float waterScale;

    //    waterScale = pm->waterlevel / 3.0;
    //    waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
    //    if ( wishspeed > pm->ps->speed * waterScale ) {
    //        wishspeed = pm->ps->speed * waterScale;
    //    }
    //}

    float accelerate;

    // when a player gets hit, they temporarily lose
    // full control, which allows them to be moved a bit
    //if ( ( groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
    //    accelerate = pm_airaccelerate;
    //} else {
    accelerate = pm_accelerate;
    //}

    Accelerate ( wishdir, wishspeed, accelerate );

    //if ( ( groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
    //    Velocity[ 1 ] -= GRAVITY * TimeStep;
    //} else {
    //    // don't reset the z velocity for slopes
    //    //  Velocity[1] = 0;
    //}

    float vel = Velocity.Length();

    // slide along the ground plane
    ClipVelocity( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );

    // don't decrease velocity when going up or down a slope
    Velocity.NormalizeSelf();
    Velocity *= vel;

    // don't do anything if standing still
    if ( !Velocity[ 0 ] && !Velocity[ 2 ] ) {
        //GLogger.Printf( "Standing\n" );
        return;
    }

    GLogger.Printf( "Walkmove vel %s\n", Velocity.ToString().ToConstChar() );
    StepSlideMove( false );
}

void FPlayer::GroundTraceMissed() {
    //trace_t  trace;
    //Float3  point;

    if ( groundEntityNum != ENTITYNUM_NONE ) {
        // we just transitioned into freefall
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:lift\n", c_pmove );
        //}

        // if they aren't in a jumping animation and the ground is a ways away, force into it
        // if we didn't do the trace, the player would be backflipping down staircases
        //point = Origin;
        //point[ 1 ] -= 64;

        //pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
        //if ( trace.fraction == 1.0 ) {
        //    if ( pm->cmd.forwardmove >= 0 ) {
        //        PM_ForceLegsAnim( LEGS_JUMP );
        //        pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
        //    } else {
        //        PM_ForceLegsAnim( LEGS_JUMPB );
        //        pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
        //    }
        //}
    }

    groundEntityNum = ENTITYNUM_NONE;
    bGroundPlane = false;
    bWalking = false;
}

void FPlayer::GroundTrace() {
    Float3  point;
    trace_t  trace;

#if 1
    point[ 0 ] = Origin[ 0 ];
    point[ 1 ] = Origin[ 1 ] - 0.25f*UNIT_SCALE;
    point[ 2 ] = Origin[ 2 ];

    TraceWorld( &trace, Origin, point, "GroundTrace" );
#else
    point[ 0 ] = Origin[ 0 ];
    point[ 1 ] = Origin[ 1 ] - 2.0f;
    point[ 2 ] = Origin[ 2 ];

    FTraceResult traceResult;
    if ( GetWorld()->TraceClosest( traceResult, Origin, point, 1 ) ) {
        trace.Actor = traceResult.Body->GetParentActor();
        trace.endpos = Origin + traceResult.HitFraction*( point - Origin );
        trace.fraction = traceResult.HitFraction;
    } else {
        trace.Actor = nullptr;
        trace.endpos = point;
        trace.fraction = 1;
    }
    trace.allsolid = false;
    trace.Plane.Normal = traceResult.Normal;
    trace.Plane.D = traceResult.Distance;
    trace.surfaceFlags = 0;
#endif

    groundTrace = trace;
    //GLogger.Printf( "normal (%s)\n", trace.Plane.Normal.ToString().ToConstChar() );
    // do something corrective if the trace starts in a solid...
    if ( trace.allsolid ) {
        //if ( !PM_CorrectAllSolid( &trace ) )
        //    return;
    }

    // if the trace didn't hit anything, we are in free fall
    if ( trace.fraction == 1.0 ) {
        GroundTraceMissed();
        bGroundPlane = false;
        bWalking = false;
        GLogger.Printf( "Fall\n" );
        return;
    }

    // check if getting thrown off the ground
    if ( Velocity[ 1 ] > 0 && Velocity.Dot( trace.Plane.Normal ) > 10 ) {
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:kickoff\n", c_pmove );
        //}
        // go into jump animation
        //if ( pm->cmd.forwardmove >= 0 ) {
        //    PM_ForceLegsAnim( LEGS_JUMP );
        //    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
        //} else {
        //    PM_ForceLegsAnim( LEGS_JUMPB );
        //    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
        //}

        groundEntityNum = ENTITYNUM_NONE;
        bGroundPlane = false;
        bWalking = false;
        GLogger.Printf( "Throwoff\n" );
        return;
    }

    // slopes that are too steep will not be considered onground
    if ( trace.Plane.Normal[ 1 ] < MIN_WALK_NORMAL ) {
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:steep\n", c_pmove );
        //}
        // FIXME: if they can't slide down the slope, let them
        // walk (sharp crevices)
        groundEntityNum = ENTITYNUM_NONE;
        bGroundPlane = true;
        bWalking = false;
        GLogger.Printf( "trace.Plane.Normal[ 1 ] < MIN_WALK_NORMAL (%s)\n", trace.Plane.Normal.ToString().ToConstChar() );
        return;
    } else {
        //GLogger.Printf( "normal (%s)\n", trace.Plane.Normal.ToString().ToConstChar() );
    }

    bGroundPlane = true;
    bWalking = true;

    // hitting solid ground will end a waterjump
    //if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
    //{
    //    pm->ps->pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
    //    pm->ps->pm_time = 0;
    //}

    //if ( groundEntityNum == ENTITYNUM_NONE ) {
    //    // just hit the ground
    //    //if ( pm->debugLevel ) {
    //    //    Com_Printf( "%i:Land\n", c_pmove );
    //    //}

    //    PM_CrashLand();

    //    // don't do landing time if we were just going down a slope
    //    if ( pml.previous_velocity[ 1 ] < -200 ) {
    //        // don't allow another jump for a little while
    //        pm->ps->pm_flags |= PMF_TIME_LAND;
    //        pm->ps->pm_time = 250;
    //    }
    //}

    groundEntityNum = trace.Actor;

    // don't reset the Y velocity for slopes
    // Velocity[1] = 0;

    //PM_AddTouchEnt( trace.entityNum );
}

void FPlayer::TickPrePhysics( float _TimeStep ) {
    Super::TickPrePhysics( _TimeStep );

    TimeStep = _TimeStep;
    //ForwardVec = Camera->GetForwardVector();
    //RightVec = Camera->GetRightVector();
    //UpVec = Camera->GetUpVector();
    Origin = RootComponent->GetPosition();
    //bWalking = false;
    bGroundPlane = false;
    impactSpeed = 0;
    //previous_origin.Clear();
    //previous_velocity.Clear()
    //previous_waterlevel = 0;

    PMins[ 0 ] = -15 * UNIT_SCALE;
    PMins[ 2 ] = -15 * UNIT_SCALE;

    PMaxs[ 0 ] = 15 * UNIT_SCALE;
    PMaxs[ 2 ] = 15 * UNIT_SCALE;

#define MINS_Z  (-24*UNIT_SCALE)

    PMins[ 1 ] = MINS_Z;
    PMaxs[ 1 ] = 32 * UNIT_SCALE;

    if ( upmove < 10 ) {
        // not holding jump
        pm_flags &= ~PMF_JUMP_HELD;
    }

    GroundTrace();

    if ( bWalking ) {
        WalkMove();
    } else {
        AirMove();
    }

    RootComponent->SetPosition( Origin );
}

void FPlayer::MoveForward( float _Value ) {
    forwardmove = 127 * FMath::Sign( _Value );
}

void FPlayer::MoveRight( float _Value ) {
    rightmove = 127 * FMath::Sign( _Value );
}

void FPlayer::MoveUp( float _Value ) {
    if ( _Value > 0 ) {
        upmove = 127;
    } else {
        upmove = 0;
    }
}

void FPlayer::MoveDown( float _Value ) {
}

void FPlayer::TurnRight( float _Value ) {
    if ( _Value ) {
        Angles.Yaw -= _Value;
        Angles.Yaw = Angl::Normalize180( Angles.Yaw );
        Camera->SetAngles( Angles );
    }
}

void FPlayer::TurnUp( float _Value ) {
    if ( _Value ) {
        Angles.Pitch += _Value;
        Angles.Pitch = FMath::Clamp( Angles.Pitch, -90.0f, 90.0f );
        Camera->SetAngles( Angles );
    }
}


#if 0
// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_duckScale = 0.25f;
float	pm_swimScale = 0.50f;
float	pm_wadeScale = 0.70f;

float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 3.0f;
float	pm_spectatorfriction = 5.0f;

int		c_pmove = 0;

#define OVERCLIP 1.001f
#define	JUMP_VELOCITY	270
#define	STEPSIZE		(18*UNIT_SCALE)
#define GRAVITY 800
#define SURF_SLICK 1
#define	MIN_WALK_NORMAL	0.7f		// can't walk on very steep slopes
#define ENTITYNUM_NONE 0
#define UNIT_SCALE (1.0f/32.0f)

AN_BEGIN_CLASS_META( FPlayer )
AN_END_CLASS_META()

FPlayer::FPlayer() {
    Camera = AddComponent< FCameraComponent >( "Camera" );

    bCanEverTick = true;
    bTickPrePhysics = true;

    // Create unit box
    



    FCollisionCapsule * capsule = NewObject< FCollisionCapsule >();
    capsule->Radius = 0.6f;
    capsule->Height = 0.7f;
    PhysBody = AddComponent< FPhysicalBody >( "PlayerCapsule" );
    PhysBody->BodyComposition.AddCollisionBody( capsule );
#if 0
    PhysBody->Mass = 70.0f;
    PhysBody->PhysicsSimulation = PS_DYNAMIC;
    PhysBody->bDisableGravity = false;
#else
    PhysBody->PhysicsBehavior = PB_KINEMATIC;
#endif
    PhysBody->CollisionGroup = CM_PAWN;

    RootComponent = PhysBody;

    Camera->SetPosition( 0, 26*UNIT_SCALE, 0.0f );
    Camera->AttachTo( RootComponent );
}


void FPlayer::TraceWorld(
    trace_t *results,
    const Float3 & start,
    Float3 & mins,
    Float3 & maxs,
    const Float3 & end,
    int passEntityNum,
    int contentmask,
    int capsule )
{
    FTraceResult traceResult;

    FCollisionIgnoreLists ignoreLists;
    FActor * actors[ 1 ] = { this };

    ignoreLists.Actors = actors;
    ignoreLists.ActorsCount = 1;
    ignoreLists.CollisionMask = CM_WORLD | CM_PAWN;

    if ( GetWorld()->TraceBox( traceResult, PMins, PMaxs, start, end, &ignoreLists ) ) {
    //if ( GetWorld()->TraceBox( traceResult, /*( maxs - mins )*0.5f*/Float3(0.5f), start, end, 1 ) ) {
        if ( traceResult.Body->GetParentActor() == this ) {
            GLogger.Printf( "This actor\n" );
        }
        GLogger.Printf( "track ok\n" );

        results->Actor = traceResult.Body->GetParentActor();
    } else {
        traceResult.HitFraction = 1;

        results->Actor = nullptr;
    }

    results->allsolid = false;
    results->endpos = start + traceResult.HitFraction*( end - start );
    results->Plane.Normal = traceResult.Normal;
    results->Plane.D = traceResult.Distance;
    results->fraction = traceResult.HitFraction;
    results->surfaceFlags = 0;


#if 0
    moveclip_t	clip;
    int			i;

    if ( !mins ) {
        mins = vec3_origin;
    }
    if ( !maxs ) {
        maxs = vec3_origin;
    }

    Com_Memset ( &clip, 0, sizeof ( moveclip_t ) );

    // clip to world
    CM_BoxTrace( &clip.trace, start, end, mins, maxs, 0, contentmask, capsule );
    clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
    if ( clip.trace.fraction == 0 ) {
        *results = clip.trace;
        return;		// blocked immediately by the world
    }

    clip.contentmask = contentmask;
    clip.start = start;
    //	VectorCopy( clip.trace.endpos, clip.end );
    clip.end = end;
    clip.mins = mins;
    clip.maxs = maxs;
    clip.passEntityNum = passEntityNum;
    clip.capsule = capsule;

    // create the bounding box of the entire move
    // we can limit it to the part of the move not
    // already clipped off by the world, which can be
    // a significant savings for line of sight and shot traces
    for ( i = 0; i<3; i++ ) {
        if ( end[ i ] > start[ i ] ) {
            clip.boxmins[ i ] = clip.start[ i ] + clip.mins[ i ] - 1;
            clip.boxmaxs[ i ] = clip.end[ i ] + clip.maxs[ i ] + 1;
        } else {
            clip.boxmins[ i ] = clip.end[ i ] + clip.mins[ i ] - 1;
            clip.boxmaxs[ i ] = clip.start[ i ] + clip.maxs[ i ] + 1;
        }
    }

    // clip to other solid entities
    SV_ClipMoveToEntities ( &clip );

    *results = clip.trace;
#endif
}

void FPlayer::ApplyFriction() {
    float	control;

    Float3 vec = Velocity;
    if ( bWalking ) {
        vec[ 1 ] = 0;	// ignore slope movement
    }

    float speed = vec.Length();
    if ( speed < 1 ) {
        Velocity[ 0 ] = 0;
        Velocity[ 2 ] = 0;		// allow sinking underwater
                            // FIXME: still have z friction underwater?
        return;
    }

    float drop = 0;

    // apply ground friction
    //if ( pm->waterlevel <= 1 ) {
        if ( bWalking && !( groundTrace.surfaceFlags & SURF_SLICK ) ) {
            // if getting knocked back, no friction
    //        if ( !( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
                control = speed < pm_stopspeed ? pm_stopspeed : speed;
                drop += control*pm_friction*TimeStep;
    //        }
    //    }
    }

    // apply water friction even if just wading
    //if ( pm->waterlevel ) {
    //    drop += speed*pm_waterfriction*pm->waterlevel*TimeStep;
    //}

    // apply flying friction
    //if ( pm->ps->powerups[ PW_FLIGHT ] ) {
    //    drop += speed*pm_flightfriction*TimeStep;
    //}

    //if ( pm->ps->pm_type == PM_SPECTATOR ) {
    //    drop += speed*pm_spectatorfriction*TimeStep;
    //}

    // scale the velocity
    float newspeed = speed - drop;
    if ( newspeed < 0 ) {
        newspeed = 0;
    }
    newspeed /= speed;

    Velocity = Velocity * newspeed;
}

float FPlayer::ScaleMove() {
    float max = abs( forwardmove );
    if ( abs( rightmove ) > max ) {
        max = abs( rightmove );
    }
    if ( abs( upmove ) > max ) {
        max = abs( upmove );
    }
    if ( !max ) {
        return 0;
    }

    float total = sqrt( forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );

    return ( float )/*pm->ps->speed*/255.0f * max / ( 127.0 * total );
}

void ClipVelocity( Float3 const & in, Float3 const & normal, Float3 & out, float overbounce ) {
    float backoff = in.Dot( normal );
    if ( backoff < 0 ) {
        backoff *= overbounce;
    } else {
        backoff /= overbounce;
    }
    out = in - normal * backoff;
}

void FPlayer::Accelerate( Float3 const & wishdir, float wishspeed, float accel ) {
    float		addspeed, accelspeed, currentspeed;

    currentspeed = Velocity.Dot( wishdir );
    addspeed = wishspeed - currentspeed;
    if ( addspeed <= 0 ) {
        return;
    }
    accelspeed = accel*TimeStep*wishspeed;
    if ( accelspeed > addspeed ) {
        accelspeed = addspeed;
    }

    Velocity += accelspeed * wishdir;
}

#define	MAX_CLIP_PLANES	5
bool FPlayer::SlideMove( bool gravity ) {
    int			bumpcount, numbumps;
    Float3		dir;
    float		d;
    int			numplanes;
    Float3		planes[ MAX_CLIP_PLANES ];
    Float3		primal_velocity;
    Float3		clipVelocity;
    int			i, j, k;
    trace_t	    trace;
    Float3		end;
    float		time_left;
    float		into;
    Float3		endVelocity;
    Float3		endClipVelocity;

    numbumps = 4;

    primal_velocity = Velocity;

    if ( gravity ) {
        endVelocity = Velocity;
        endVelocity[ 1 ] -= GRAVITY * TimeStep;
        Velocity[ 1 ] = ( Velocity[ 1 ] + endVelocity[ 1 ] ) * 0.5f;
        primal_velocity[ 1 ] = endVelocity[ 1 ];
        if ( bGroundPlane ) {
            // slide along the ground plane
            ClipVelocity ( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );
        }
    }

    time_left = TimeStep;

    // never turn against the ground plane
    if ( bGroundPlane ) {
        numplanes = 1;
        planes[ 0 ] = groundTrace.Plane.Normal;
    } else {
        numplanes = 0;
    }

    // never turn against original velocity
    float velLenght = Velocity.Length();
    if ( velLenght ) {
        planes[ numplanes ] = Velocity / velLenght;
    } else {
        planes[ numplanes ].Clear();
    }
    numplanes++;

    for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

        // calculate position we are trying to move to
        end = Origin + Velocity * time_left * UNIT_SCALE;

        //GLogger.Printf( "end %s\n", end.ToString().ToConstChar() );

        // see if we can make it there
        TraceWorld ( &trace, Origin, PMins, PMaxs, end, clientNum, tracemask, false );

        if ( trace.allsolid ) {
            // entity is completely trapped in another solid
            Velocity[ 1 ] = 0;	// don't build up falling damage, but allow sideways acceleration
            return true;
        }

        //GLogger.Printf( "fraction %f\n", trace.fraction );

        if ( trace.fraction > 0 ) {
            // actually covered some distance
            Origin = trace.endpos;
            //GLogger.Printf( "Origin = trace.endpos\n" );
        }

        if ( trace.fraction == 1 ) {
            //GLogger.Printf( "fraction == 1\n" );
            break;		// moved the entire distance
        }

        // save entity for contact
        //PM_AddTouchEnt( trace.entityNum );

        time_left -= time_left * trace.fraction;

        if ( numplanes >= MAX_CLIP_PLANES ) {
            // this shouldn't really happen
            Velocity.Clear();
            return true;
        }

        //
        // if this is the same plane we hit before, nudge velocity
        // out along it, which fixes some epsilon issues with
        // non-axial planes
        //
        for ( i = 0; i < numplanes; i++ ) {
            if ( trace.Plane.Normal.Dot( planes[ i ] ) > 0.99 ) {
                Velocity += trace.Plane.Normal;
                break;
            }
        }
        if ( i < numplanes ) {
            continue;
        }
        planes[ numplanes ] = trace.Plane.Normal;
        numplanes++;

        //
        // modify velocity so it parallels all of the clip planes
        //

        // find a plane that it enters
        for ( i = 0; i < numplanes; i++ ) {
            into = Velocity.Dot( planes[ i ] );
            if ( into >= 0.1 ) {
                continue;		// move doesn't interact with the plane
            }

            // see how hard we are hitting things
            if ( -into > impactSpeed ) {
                impactSpeed = -into;
            }

            // slide along the plane
            ClipVelocity ( Velocity, planes[ i ], clipVelocity, OVERCLIP );

            // slide along the plane
            ClipVelocity ( endVelocity, planes[ i ], endClipVelocity, OVERCLIP );

            // see if there is a second plane that the new move enters
            for ( j = 0; j < numplanes; j++ ) {
                if ( j == i ) {
                    continue;
                }
                if ( clipVelocity.Dot( planes[ j ] ) >= 0.1 ) {
                    continue;		// move doesn't interact with the plane
                }

                // try clipping the move to the plane
                ClipVelocity( clipVelocity, planes[ j ], clipVelocity, OVERCLIP );
                ClipVelocity( endClipVelocity, planes[ j ], endClipVelocity, OVERCLIP );

                // see if it goes back into the first clip plane
                if ( clipVelocity.Dot( planes[ i ] ) >= 0 ) {
                    continue;
                }

                // slide the original velocity along the crease
                dir = planes[ i ].Cross( planes[ j ] );
                dir.NormalizeSelf();
                d = dir.Dot( Velocity );
                clipVelocity = dir * d;

                dir = planes[ i ].Cross( planes[ j ] );
                dir.NormalizeSelf();
                d = dir.Dot( endVelocity );
                endClipVelocity = dir * d;

                // see if there is a third plane the the new move enters
                for ( k = 0; k < numplanes; k++ ) {
                    if ( k == i || k == j ) {
                        continue;
                    }
                    if ( clipVelocity.Dot( planes[ k ] ) >= 0.1 ) {
                        continue;		// move doesn't interact with the plane
                    }

                    // stop dead at a tripple plane interaction
                    Velocity.Clear();
                    return true;
                }
            }

            // if we have fixed all interactions, try another move
            Velocity = clipVelocity;
            endVelocity = endClipVelocity;
            break;
        }
    }

    if ( gravity ) {
        Velocity = endVelocity;
    }

    // don't change velocity if in a timer (FIXME: is this correct?)
    //if ( pm->ps->pm_time ) {
    //    Velocity = primal_velocity;
    //}

    return ( bumpcount != 0 );
}

void FPlayer::StepSlideMove( bool gravity ) {
    Float3		start_o, start_v;
    Float3		down_o, down_v;
    trace_t		trace;
    //	float		down_dist, up_dist;
    //	Float3		delta, delta2;
    Float3		up, down;
    float		stepSize;

    start_o = Origin;
    start_v = Velocity;

    if ( SlideMove( gravity ) == 0 ) {
        GLogger.Printf( "!!!!!!!\n" );
        return;		// we got exactly where we wanted to go first try	
    }
    GLogger.Printf( "2222222222\n" );
    down = start_o;
    down[ 1 ] -= STEPSIZE;
    TraceWorld ( &trace, start_o, PMins, PMaxs, down, clientNum, tracemask, false );
    up = Float3( 0, 1, 0 );
    // never step up when you still have up velocity
    if ( Velocity[ 1 ] > 0 && ( trace.fraction == 1.0 || trace.Plane.Normal.Dot( up ) < 0.7 ) ) {
        return;
    }

    down_o = Origin;
    down_v = Velocity;

    up = start_o;
    up[ 1 ] += STEPSIZE;

    // test the player position if they were a stepheight higher
    TraceWorld ( &trace, start_o, PMins, PMaxs, up, clientNum, tracemask, false );
    if ( trace.allsolid ) {
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:bend can't step\n", c_pmove );
        //}
        return;		// can't step up
    }

    stepSize = trace.endpos[ 1 ] - start_o[ 1 ];
    // try slidemove from this position
    Origin = trace.endpos;
    Velocity = start_v;

    SlideMove( gravity );

    // push down the final amount
    down = Origin;
    down[ 1 ] -= stepSize;
    TraceWorld ( &trace, Origin, PMins, PMaxs, down, clientNum, tracemask, false );
    if ( !trace.allsolid ) {
        Origin = trace.endpos;
    }
    if ( trace.fraction < 1.0 ) {
        ClipVelocity( Velocity, trace.Plane.Normal, Velocity, OVERCLIP );
    }

#if 0
    // if the down trace can trace back to the original position directly, don't step
    TraceWorld( &trace, Origin, PMins, PMaxs, start_o, clientNum, tracemask, false );
    if ( trace.fraction == 1.0 ) {
        // use the original move
        VectorCopy ( down_o, Origin );
        VectorCopy ( down_v, Velocity );
        if ( pm->debugLevel ) {
            Com_Printf( "%i:bend\n", c_pmove );
        }
    } else
#endif
    //{
    //    // use the step move
    //    float	delta;

    //    delta = Origin[ 1 ] - start_o[ 1 ];
    //    if ( delta > 2 ) {
    //        if ( delta < 7 ) {
    //            PM_AddEvent( EV_STEP_4 );
    //        } else if ( delta < 11 ) {
    //            PM_AddEvent( EV_STEP_8 );
    //        } else if ( delta < 15 ) {
    //            PM_AddEvent( EV_STEP_12 );
    //        } else {
    //            PM_AddEvent( EV_STEP_16 );
    //        }
    //    }
    //    if ( pm->debugLevel ) {
    //        Com_Printf( "%i:stepped\n", c_pmove );
    //    }
    //}
}

#define PMF_JUMP_HELD 1

bool FPlayer::CheckJump() {
    //if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
    //    return qfalse;		// don't allow jump until all buttons are up
    //}

    if ( upmove < 10 ) {
        // not holding jump
        return false;
    }

    // must wait for jump to be released
    if ( pm_flags & PMF_JUMP_HELD ) {
        // clear upmove so cmdscale doesn't lower running speed
        upmove = 0;
        return false;
    }

    bGroundPlane = false;		// jumping away
    bWalking = false;
    pm_flags |= PMF_JUMP_HELD;

    groundEntityNum = ENTITYNUM_NONE;
    Velocity[ 1 ] = JUMP_VELOCITY;
    //PM_AddEvent( EV_JUMP );

    //if ( forwardmove >= 0 ) {
    //    PM_ForceLegsAnim( LEGS_JUMP );
    //    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
    //} else {
    //    PM_ForceLegsAnim( LEGS_JUMPB );
    //    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
    //}

    GLogger.Printf( "Jump\n" );

    return true;
}

void FPlayer::AirMove() {
    Float3		wishvel;
    float		fmove, smove;
    Float3		wishdir;
    float		wishspeed;
    float		scale;

    ApplyFriction();

    fmove = forwardmove;
    smove = rightmove;

    scale = ScaleMove();

    // set the movementDir so clients can rotate the legs for strafing
    //PM_SetMovementDir();

    // project moves down to flat plane
    ForwardVec[ 1 ] = 0;
    RightVec[ 1 ] = 0;
    ForwardVec.NormalizeSelf();
    RightVec.NormalizeSelf();

    wishvel = ForwardVec * fmove + RightVec * smove;
    wishvel[ 1 ] = 0;

    wishdir = wishvel;
    wishspeed = wishdir.NormalizeSelf();
    wishspeed *= scale;

    // not on ground, so little effect on velocity
    Accelerate ( wishdir, wishspeed, pm_airaccelerate );

    // we may have a ground plane that is very steep, even
    // though we don't have a groundentity
    // slide along the steep plane
    if ( bGroundPlane ) {
        ClipVelocity ( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );
    }

#if 0
    //ZOID:  If we are on the grapple, try stair-stepping
    //this allows a player to use the grapple to pull himself
    //over a ledge
    if ( pm->ps->pm_flags & PMF_GRAPPLE_PULL )
        StepSlideMove ( qtrue );
    else
        SlideMove ( qtrue );
#endif

    GLogger.Printf( "Airmove vel %s\n", Velocity.ToString().ToConstChar() );
    StepSlideMove ( true );
}

void FPlayer::WalkMove() {

    if ( CheckJump () ) {
        // jumped away
        //if ( pm->waterlevel > 1 ) {
        //    PM_WaterMove();
        //} else {
            AirMove();
        //}
        return;
    }

    ApplyFriction();

    float fmove = forwardmove;
    float smove = rightmove;

    float scale = ScaleMove();

    // set the movementDir so clients can rotate the legs for strafing
    //PM_SetMovementDir();

    // project moves down to flat plane
    ForwardVec[ 1 ] = 0;
    RightVec[ 1 ] = 0;

    // project the forward and right directions onto the ground plane
    ClipVelocity ( ForwardVec, groundTrace.Plane.Normal, ForwardVec, OVERCLIP );
    ClipVelocity ( RightVec, groundTrace.Plane.Normal, RightVec, OVERCLIP );

    ForwardVec.NormalizeSelf();
    RightVec.NormalizeSelf();

    Float3 wishvel = ForwardVec * fmove + RightVec * smove;
    // when going up or down slopes the wish velocity should Not be zero
    //	wishvel[1] = 0;

    Float3 wishdir = wishvel;
    float wishspeed = wishdir.NormalizeSelf();
    //wishspeed = VectorNormalize( wishdir );
    wishspeed *= scale;

    // clamp the speed lower if ducking
    //if ( pm->ps->pm_flags & PMF_DUCKED ) {
    //    if ( wishspeed > pm->ps->speed * pm_duckScale ) {
    //        wishspeed = pm->ps->speed * pm_duckScale;
    //    }
    //}

    // clamp the speed lower if wading or walking on the bottom
    //if ( pm->waterlevel ) {
    //    float	waterScale;

    //    waterScale = pm->waterlevel / 3.0;
    //    waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
    //    if ( wishspeed > pm->ps->speed * waterScale ) {
    //        wishspeed = pm->ps->speed * waterScale;
    //    }
    //}

    float accelerate;

    // when a player gets hit, they temporarily lose
    // full control, which allows them to be moved a bit
    //if ( ( groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
    //    accelerate = pm_airaccelerate;
    //} else {
        accelerate = pm_accelerate;
    //}

    Accelerate ( wishdir, wishspeed, accelerate );

    //if ( ( groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
    //    Velocity[ 1 ] -= GRAVITY * TimeStep;
    //} else {
    //    // don't reset the z velocity for slopes
    //    //		Velocity[1] = 0;
    //}

    float vel = Velocity.Length();

    // slide along the ground plane
    ClipVelocity( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );

    // don't decrease velocity when going up or down a slope
    Velocity.NormalizeSelf();
    Velocity *= vel;

    // don't do anything if standing still
    if ( !Velocity[ 0 ] && !Velocity[ 2 ] ) {
        GLogger.Printf( "Standing\n" );
        return;
    }

    StepSlideMove( false );
}

void FPlayer::GroundTraceMissed() {
    //trace_t		trace;
    Float3		point;

    if ( groundEntityNum != ENTITYNUM_NONE ) {
        // we just transitioned into freefall
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:lift\n", c_pmove );
        //}

        // if they aren't in a jumping animation and the ground is a ways away, force into it
        // if we didn't do the trace, the player would be backflipping down staircases
        point = Origin;
        point[ 1 ] -= 64;

        //pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
        //if ( trace.fraction == 1.0 ) {
        //    if ( pm->cmd.forwardmove >= 0 ) {
        //        PM_ForceLegsAnim( LEGS_JUMP );
        //        pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
        //    } else {
        //        PM_ForceLegsAnim( LEGS_JUMPB );
        //        pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
        //    }
        //}
    }

    groundEntityNum = ENTITYNUM_NONE;
    bGroundPlane = false;
    bWalking = false;
}

void FPlayer::GroundTrace() {
    Float3		point;
    trace_t		trace;

    point[ 0 ] = Origin[ 0 ];
    point[ 1 ] = Origin[ 1 ] - 0.25f*UNIT_SCALE;
    point[ 2 ] = Origin[ 2 ];

    TraceWorld( &trace, Origin, PMins, PMaxs, point, clientNum, tracemask, false );
    groundTrace = trace;

    // do something corrective if the trace starts in a solid...
    if ( trace.allsolid ) {
        //if ( !PM_CorrectAllSolid( &trace ) )
        //    return;
    }

    // if the trace didn't hit anything, we are in free fall
    if ( trace.fraction == 1.0 ) {
//        PM_GroundTraceMissed();
        bGroundPlane = false;
        bWalking = false;
        return;
    }

    // check if getting thrown off the ground
    if ( Velocity[ 1 ] > 0 && Velocity.Dot( trace.Plane.Normal ) > 10 ) {
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:kickoff\n", c_pmove );
        //}
        // go into jump animation
        //if ( pm->cmd.forwardmove >= 0 ) {
        //    PM_ForceLegsAnim( LEGS_JUMP );
        //    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
        //} else {
        //    PM_ForceLegsAnim( LEGS_JUMPB );
        //    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
        //}

        groundEntityNum = ENTITYNUM_NONE;
        bGroundPlane = false;
        bWalking = false;
        return;
    }

    // slopes that are too steep will not be considered onground
    if ( trace.Plane.Normal[ 1 ] < MIN_WALK_NORMAL ) {
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:steep\n", c_pmove );
        //}
        // FIXME: if they can't slide down the slope, let them
        // walk (sharp crevices)
        groundEntityNum = ENTITYNUM_NONE;
        bGroundPlane = true;
        bWalking = false;
        return;
    }

    bGroundPlane = true;
    bWalking = true;

    // hitting solid ground will end a waterjump
    //if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
    //{
    //    pm->ps->pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
    //    pm->ps->pm_time = 0;
    //}

    //if ( groundEntityNum == ENTITYNUM_NONE ) {
    //    // just hit the ground
    //    //if ( pm->debugLevel ) {
    //    //    Com_Printf( "%i:Land\n", c_pmove );
    //    //}

    //    PM_CrashLand();

    //    // don't do landing time if we were just going down a slope
    //    if ( pml.previous_velocity[ 1 ] < -200 ) {
    //        // don't allow another jump for a little while
    //        pm->ps->pm_flags |= PMF_TIME_LAND;
    //        pm->ps->pm_time = 250;
    //    }
    //}

    groundEntityNum = trace.Actor;

    // don't reset the Y velocity for slopes
    //	Velocity[1] = 0;

    //PM_AddTouchEnt( trace.entityNum );
}

void FPlayer::TickPrePhysics( float _TimeStep ) {
    Super::TickPrePhysics( _TimeStep );

    TimeStep = _TimeStep;
    ForwardVec = Camera->GetForwardVector();
    RightVec = Camera->GetRightVector();
    UpVec = Camera->GetUpVector();
    Origin = RootComponent->GetPosition();
    //bWalking = false;
    bGroundPlane = false;
    impactSpeed = 0;
    //previous_origin.Clear();
    //previous_velocity.Clear()
    //previous_waterlevel = 0;
    //PMins = Float3( -0.5f );
    //PMaxs = Float3( 0.5f );

    PMins[ 0 ] = -15 * UNIT_SCALE;
    PMins[ 2 ] = -15 * UNIT_SCALE;

    PMaxs[ 0 ] = 15 * UNIT_SCALE;
    PMaxs[ 2 ] = 15 * UNIT_SCALE;

    #define	MINS_Z	 (-24*UNIT_SCALE)

    PMins[ 1 ] = MINS_Z;
    PMaxs[ 1 ] = 32*UNIT_SCALE;

    if ( upmove < 10 ) {
        // not holding jump
        pm_flags &= ~PMF_JUMP_HELD;
    }

    GroundTrace();

    if ( bWalking ) {
        WalkMove();
    } else {
        AirMove();
    }

    //PhysBody->SetLinearVelocity( Velocity );
    //PhysBody->SetPosition( PhysBody->GetPosition() + Velocity*0.001f );
    RootComponent->SetPosition( Origin );

    forwardmove = rightmove = upmove = 0;

    unitBoxComponent->SetPosition( RootComponent->GetPosition() );
}

void FPlayer::MoveForward( float _Value ) {
    forwardmove = 127 * FMath::Sign( _Value );
}

void FPlayer::MoveRight( float _Value ) {
    rightmove = 127 * FMath::Sign( _Value );
}

void FPlayer::MoveUp( float _Value ) {
    if ( _Value > 0 ) {
        upmove = 127;
    } else {
        upmove = 0;
    }
}

void FPlayer::MoveDown( float _Value ) {
}

void FPlayer::TurnRight( float _Value ) {
    if ( _Value ) {
        Angles.Yaw -= _Value;
        Angles.Yaw = Angl::Normalize180( Angles.Yaw );
        Camera->SetAngles( Angles );
    }
}

void FPlayer::TurnUp( float _Value ) {
    if ( _Value ) {
        Angles.Pitch += _Value;
        Angles.Pitch = FMath::Clamp( Angles.Pitch, -90.0f, 90.0f );
        Camera->SetAngles( Angles );
    }
}

void FPlayer::MoveObjectForward( float _Value ) {

    if ( Object ) {
        Object->RootComponent->StepForward( _Value * 10 );
    }
}

void FPlayer::MoveObjectRight( float _Value ) {
    if ( Object ) {
        Object->RootComponent->StepRight( _Value * 10 );
    }
}

void FPlayer::DrawDebug( FDebugDraw * _DebugDraw ) {

    //Super::DrawDebug( _DebugDraw );





    //_DebugDraw->DrawPlane( Float3(0,1,0),0.5f );
}

#endif


void FPlayer::SpawnRandomShape() {
    FActor * actor;

    FTransform transform;

    transform.Position = Camera->GetWorldPosition() + Camera->GetWorldForwardVector() * 1.5f;
    transform.Rotation = Angl( 45.0f, 45.0f, 45.0f ).ToQuat();
    transform.SetScale( 0.6f );

    int i = FMath::Rand() * 4;
    switch ( i ) {
    case 0:
        transform.SetScale(5.0f);
        actor = GetWorld()->SpawnActor< FBoxActor >( transform );
        break;
    case 1:
        actor = GetWorld()->SpawnActor< FSphereActor >( transform );
        break;
    case 2:
        transform.Scale.X = 2;
        transform.Scale.Z = 2;
        actor = GetWorld()->SpawnActor< FSphereActor >( transform );
        break;
    default:
        actor = GetWorld()->SpawnActor< FCylinderActor >( transform );
        break;
    }

    FMeshComponent * mesh = actor->GetComponent< FMeshComponent >();
    if ( mesh ) {
        mesh->ApplyCentralImpulse( Camera->GetWorldForwardVector() * 1.0f );
    }
}

void FPlayer::SpawnSoftBody() {
    FTransform transform;

    transform.Position = Camera->GetWorldPosition() + Camera->GetWorldForwardVector() * 1.5f;
    transform.Rotation = Camera->GetWorldRotation();

    Object = GetWorld()->SpawnActor< FSoftMeshActor >( transform );
}

void FPlayer::SpawnComposedActor() {
    FTransform transform;

    transform.Position = Camera->GetWorldPosition() + Camera->GetWorldForwardVector() * 1.5f;

    FComposedActor * actor = GetWorld()->SpawnActor< FComposedActor >( transform );
    FMeshComponent * mesh = actor->GetComponent< FMeshComponent >();
    if ( mesh ) {
        mesh->ApplyCentralImpulse( Camera->GetWorldForwardVector() * 2.0f );
    }
}

void FPlayer::DrawDebug( FDebugDraw * _DebugDraw ) {
//    TPodArray<Float3> verts;
//    TPodArray<unsigned int > ind;
//    PhysBody->CreateCollisionModel( verts, ind );

//    _DebugDraw->SetDepthTest(false);
//    _DebugDraw->SetColor(0,0,1,1);
//    _DebugDraw->DrawTriangleSoupWireframe( verts.ToPtr(), sizeof(Float3), ind.ToPtr(), ind.Length() );
}
