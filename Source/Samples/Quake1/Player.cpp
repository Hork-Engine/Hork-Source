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
#include "Projectile.h"
#include "Game.h"
#include "MyPlayerController.h"

#include <Engine/Audio/Public/AudioSystem.h>
#include <Engine/Audio/Public/AudioClip.h>

#include <Engine/Resource/Public/ResourceManager.h>

#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Components/CameraComponent.h>

AN_BEGIN_CLASS_META( FPlayer )
AN_END_CLASS_META()

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
#define GRAVITY 800//1400
#define SURF_SLICK 1
#define MIN_WALK_NORMAL 0.7f  // can't walk on very steep slopes
#define UNIT_SCALE (1.0f/32.0f)

#ifdef AN_COMPILER_GCC
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#ifdef AN_COMPILER_MSVC
#pragma warning(disable:4189)
#pragma warning(disable:4702)
#pragma warning(disable:4101)
#endif

FPlayer::FPlayer() {
    Camera = AddComponent< FCameraComponent >( "Camera" );
    Spin = AddComponent< FSceneComponent >( "Spin" );

    // Animation single frame holder
    WeaponModel = AddComponent< FQuakeModelFrame >( "Frame" );

    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_axe.mdl");
    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_shot.mdl");
    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_shot2.mdl");
    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_nail.mdl");
    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_nail2.mdl");
    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_rock.mdl");
    FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_rock2.mdl");
    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/g_rock2.mdl");
    //FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/v_light.mdl");

    WeaponModel->SetModel( model );

    WeaponFramesCount = model ? model->Frames.Size() : 0;

    FMaterialInstance * matInst = NewObject< FMaterialInstance >();
    matInst->Material = GetResource< FMaterial >( "SkinMaterial" );

    WeaponModel->SetMaterialInstance( matInst );

    if ( model && !model->Skins.IsEmpty() ) {
        // Set random skin (just for fun)
        matInst->SetTexture( 0, model->Skins[rand()%model->Skins.Size()].Texture );
    }

    WeaponModel->AttachTo( Spin );
    WeaponModel->SetAngles(0,180,0);
    //WeaponModel->SetPosition( 0.2f, 0.1f, 0.3f );

#if 0
    FIndexedMesh * mesh = NewObject< FIndexedMesh >();
    mesh->InitializeInternalMesh( "*sphere*" );

    Sphere = AddComponent< FMeshComponent >( "Sphere" );
    Sphere->SetMesh( mesh );
    Sphere->SetMaterialInstance( matInst );
#endif
#if 1
    FIndexedMesh * mesh = NewObject< FIndexedMesh >();
    mesh->InitializeInternalMesh( "*box*" );

    Sphere = AddComponent< FMeshComponent >( "Sphere" );
    Sphere->SetMesh( mesh );
    Sphere->SetMaterialInstance( matInst );
#endif

    bCanEverTick = true;
    bTickPrePhysics = true;

#if 1
    FCollisionCapsule * capsule = NewObject< FCollisionCapsule >();
    capsule->Radius = 0.6f;
    capsule->Height = 0.7f;
    //capsule->Margin = 0.2f;
    //capsule->Position.Y = capsule->Height * 0.5f + capsule->Radius;
    capsule->Position.Y = 0.5f;
#else
    FCollisionSphere * capsule = NewObject< FCollisionSphere >();
    capsule->Radius = 1.0f;
#endif

    PhysBody = AddComponent< FPhysicalBody >( "PlayerCapsule" );
    PhysBody->BodyComposition.AddCollisionBody( capsule );
    PhysBody->PhysicsBehavior = PB_KINEMATIC;
    //PhysBody->bDisableGravity = true;
    PhysBody->CollisionGroup = CM_PAWN;
    PhysBody->CollisionMask = CM_WORLD | CM_PAWN | CM_PROJECTILE;
    //PhysBody->SetFriction( 0.0f );
    //PhysBody->SetRollingFriction( 0.0f );
    //PhysBody->SetRestitution( 0.0f );
    //PhysBody->SetCcdRadius( 10 );

#if 0
    PhysBody->Mass = 70.0f;
    PhysBody->PhysicsSimulation = PS_DYNAMIC;
#endif

    RootComponent = PhysBody;

    Spin->SetPosition( 0, 26 * UNIT_SCALE, 0.0f );
    //Spin->SetPosition( 0, 26 * UNIT_SCALE, 4.0f );
    Spin->AttachTo( RootComponent );
    Camera->AttachTo( Spin );


    Obstacle.Shape = AI_NAV_MESH_OBSTACLE_CYLINDER;
    Obstacle.Radius = 1.0f;
    Obstacle.Height = 8.0f;
    Obstacle.ObstacleRef = 0;
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

    RootComponent->SetAngles( 0, 0, 0 );
    Spin->SetAngles( Angles );

    PrevY = RootComponent->GetPosition().Y;

#if 1
    PhysBody->SetAngularFactor( Float3( 0.0f ) );
#endif
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
    _Input->BindAction( "SwitchToSpectator", IE_Press, this, &FPlayer::SwitchToSpectator, true );
}

float cl_bob = 0.02f;
float cl_bobcycle = 0.6f;
float cl_bobup = 0.5f;

float FPlayer::CalcBob ( Float3 const & _Velocity )
{
    float	bob;
    float	cycle;

    float cl_time = GetWorld()->GetGameplayTimeMicro() * 0.000001;
    cycle = cl_time - ( int )( cl_time / cl_bobcycle )*cl_bobcycle;
    cycle /= cl_bobcycle;
    if ( cycle < cl_bobup )
        cycle = FMath::_PI * cycle / cl_bobup;
    else
        cycle = FMath::_PI + FMath::_PI*( cycle - cl_bobup ) / ( 1.0 - cl_bobup );

    bob = sqrt( _Velocity[ 0 ] * _Velocity[ 0 ] + _Velocity[ 2 ] * _Velocity[ 2 ] ) * cl_bob;
    bob = bob*0.3 + bob*0.7*sin( cycle );
    if ( bob > 4 )
        bob = 4;
    else if ( bob < -7 )
        bob = -7;
    return bob;
}

float cl_rollspeed = 200;
float cl_rollangle = 2.0;

float CalcRoll( Float3 const & _RightVec, Float3 const & _Velocity )
{
    float	sign;
    float	side;
    float	value;

    side = _Velocity.Dot( _RightVec );
    sign = side < 0 ? -1 : 1;
    side = fabs( side );

    value = cl_rollangle;
    //	if (cl.inwater)
    //		value *= 6;

    if ( side < cl_rollspeed )
        side = side * value / cl_rollspeed;
    else
        side = value;

    return side*sign;

}

void CalcViewRoll( Angl & _Angles, Float3 const & _RightVec, Float3 const & _Velocity )
{
    float		side;

    side = CalcRoll ( _RightVec, _Velocity );
    _Angles.Roll -= side;

    //if ( v_dmg_time > 0 )
    //{
    //    _Angles.Roll += v_dmg_time / v_kicktime.value*v_dmg_roll;
    //    _Angles.Pitch += v_dmg_time / v_kicktime.value*v_dmg_pitch;
    //    v_dmg_time -= host_frametime;
    //}

    //if ( cl.stats[ STAT_HEALTH ] <= 0 )
    //{
    //    _Angles.Roll = 80;	// dead view angle
    //    return;
    //}

}

void FPlayer::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    //Obstacle.Position = RootComponent->GetPosition();
    //GetLevel()->NavMesh.RemoveObstacle( &Obstacle );
    //GetLevel()->NavMesh.AddObstacle( &Obstacle );


    if ( bAttackStarted ) {
        if ( WeaponFramesCount > 0 ) {
            int keyFrame = FMath::Floor( AttackTime );
            float lerp = FMath::Fract( AttackTime );

            constexpr float ANIMATION_SPEED = 10.0f; // frames per second

            AttackTime += _TimeStep * ANIMATION_SPEED;

            keyFrame = FMath::Min( keyFrame, WeaponFramesCount - 1 );

            WeaponModel->SetFrame( keyFrame, FMath::Min( keyFrame + 1, WeaponFramesCount - 1 ), lerp );

            constexpr int ShootFrameNum = 0;

            if ( keyFrame == ShootFrameNum && !bAttacked ) {
                Shoot();

                bAttacked = true;
            }

            if ( keyFrame == WeaponFramesCount-1 ) {
                AttackTime = 0;
                AttackAngle = Angles;
                bAttacked = false;

                if ( !bAttacking ) {
                    bAttackStarted = false;
                }
            }
        }
    }

#if 0
    RayF ray;
    ray.Start = RootComponent->GetPosition();
    ray.Dir = RootComponent->GetForwardVector();
    FRaycastResult result;
    if ( GetWorld()->RaycastSphere( result, 0.5f, ray ) ) {
        Sphere->SetPosition( result.Position );
    }
#endif
#if 0
    RayF ray;
    ray.Start = RootComponent->GetPosition();
    ray.Dir = RootComponent->GetForwardVector();
    FRaycastResult result;
    if ( GetWorld()->RaycastClosest( result, ray ) ) {
        Sphere->SetPosition( result.Position );
    }
#endif
#if 0
    FTraceResult result;
    Sphere->SetScale( (PMaxs - PMins)*0.5f );
    Float3 start = Spin->GetWorldPosition();
    Float3 end = start+Spin->GetWorldForwardVector()*1000.0f;
    if ( GetWorld()->TraceBox( result, PMins, PMaxs, start, end, 1 ) ) {

        Float3 endpos = start + result.HitFraction*( end - start );

        Sphere->SetPosition( endpos/*result.Position*/ );
    }
#endif
}

void FPlayer::Shoot() {

    TActorSpawnParameters< FProjectileActor > spawnParameters;

    spawnParameters.SpawnTransform.Position = Spin->GetWorldPosition();
    spawnParameters.SpawnTransform.Rotation = Angl( -AttackAngle.Pitch, AttackAngle.Yaw+180.0f, AttackAngle.Roll ).ToQuat();

    spawnParameters.Instigator = this;

    FProjectileActor * projectile = GetWorld()->SpawnActor( spawnParameters );
    projectile->LifeSpan = 10.0f;
    projectile->MeshComponent->SetLinearVelocity( Spin->GetWorldForwardVector() * 30.0f );

    //PhysBody->AddCollisionIgnoreActor( projectile );
}

void FPlayer::SpeedPress() {
    bSpeed = true;
}

void FPlayer::SpeedRelease() {
    bSpeed = false;
}

void FPlayer::AttackPress() {
    bAttacking = true;

    if ( !bAttackStarted ) {
        bAttackStarted = true;
        AttackAngle = Angles;
        AttackTime = 0.0f;
    }
}

void FPlayer::AttackRelease() {
    bAttacking = false;
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
    if ( GetWorld()->TraceCylinder( traceResult, PMins, PMaxs, start, end, &ignoreLists ) ) {
    //if ( GetWorld()->TraceSphere( traceResult, 0.9f, start, end, 1 ) ) {
    //if ( GetWorld()->TraceBox( traceResult, PMins, PMaxs, start, end, &ignoreLists ) ) {
        if ( traceResult.Body->GetParentActor() == this ) {
            GLogger.Printf( "This actor\n" );
        }
        trace->Actor = traceResult.Body->GetParentActor();
        trace->endpos = start + (traceResult.Fraction*0.9999)*( end - start );
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


void FPlayer::TraceWorld2( trace_t * trace, const Float3 & start, const Float3 & end, const char * debug ) {
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
    if ( GetWorld()->TraceBox( traceResult, PMins+Float3(0.1f,0, 0.1f ), PMaxs - Float3( 0.1f,0, 0.1f ), start, end, &ignoreLists ) ) {
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
#if 0
    Float3  endVelocity;
    Float3 primal_velocity = Velocity;
    trace_t     trace;

    if ( gravity ) {
        endVelocity = Velocity;
        endVelocity[ 1 ] -= GRAVITY * TimeStep;
        Velocity[ 1 ] = ( Velocity[ 1 ] + endVelocity[ 1 ] ) * 0.5f;
        //primal_velocity[ 1 ] = endVelocity[ 1 ];
        if ( bGroundPlane ) {
            // slide along the ground plane
            ClipVelocity ( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );
        }
    }

    //float time_left = TimeStep;

    //Float3 end = Origin + Velocity * time_left * UNIT_SCALE;

    //TraceWorld ( &trace, Origin, end, "SlideMove" );

    

    //Origin = trace.endpos;

    

    //if ( trace.fraction < 1.0f ) {
    //ClipVelocity ( Velocity, groundTrace.Plane.Normal, Velocity, OVERCLIP );
    //}

    
    //if ( gravity ) {
    //    Velocity = endVelocity;
    //}

    //if ( gravity ) {
    //    PhysBody->SetLinearVelocity( endVelocity*UNIT_SCALE/*TimeStep*/ );
    //} else {
    //    PhysBody->SetLinearVelocity( Velocity*UNIT_SCALE/*TimeStep*/ );
    //}
    //PhysBody->RigidBody->

        // TODO: Clip velocity and move physbody by hand!



    return false;
#else
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

        //
        // modify velocity so it parallels all of the clip planes
        //

        // find a plane that it enters
        for ( i = 0; i < numplanes; i++ ) {
            into = Velocity.Dot( planes[ i ] );
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
    }

    if ( gravity ) {
        Velocity = endVelocity;
    }

    // don't change velocity if in a timer (FIXME: is this correct?)
    //if ( pm->ps->pm_time ) {
    //    Velocity = primal_velocity;
    //}

    return ( bumpcount != 0 );
#endif
}

void FPlayer::StepSlideMove( bool gravity ) {
    Float3 start_o = Origin;
    Float3 start_v = Velocity;

    if ( !SlideMove( gravity ) ) {
        return;  // we got exactly where we wanted to go first try 
    }
//return;
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

    //GLogger.Printf( "stepSize %f\n", stepSize );
    //if (stepSize<=0.0f)
    //    return;

//    Float3 a = trace.endpos;
//    Float3 b = trace.endpos+start_v;///*start_v*/start_v.Normalized()*0.2f;
//    trace_t  trace2;
//    TraceWorld ( &trace2, a, b, "StepSlideMove3a" );
//    if ( trace2.fraction == 0 ) {
//        return;
//    }

    //GLogger.Printf("stepsize %f\n",stepSize);
    // try slidemove from this position
    Origin.Y += stepSize;//Origin = trace.endpos;
    Velocity = start_v;

    //TraceWorld ( &trace, Origin, Origin+ Velocity.Normalized()*0.2f, "StepSlideMove3" );
    //if ( trace.fraction == 0 ) {
    //    Origin = start_o;
    //    Velocity = start_v;
    //    SlideMove( gravity );
    //    ClipVelocity( Velocity, trace.Plane.Normal, Velocity, OVERCLIP );
    //    return;
    //}
//Velocity.Clear();
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

    GroundActor = nullptr;
    Velocity[ 1 ] = JUMP_VELOCITY;
    //PM_AddEvent( EV_JUMP );

    //if ( forwardmove >= 0 ) {
    //    PM_ForceLegsAnim( LEGS_JUMP );
    //    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
    //} else {
    //    PM_ForceLegsAnim( LEGS_JUMPB );
    //    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
    //}

    FSoundSpawnParameters spawnParameters;
    spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
    GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/plyrjmp8.wav" ), this, &spawnParameters );

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

    GLogger.Printf( "Airmove\n" );
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
    FMath::DegSinCos( Angles.Yaw, ForwardVec[0], RightVec[ 0 ] );
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

    GLogger.Printf( "Walkmove\n" );
    //GLogger.Printf( "Walkmove vel %s\n", Velocity.ToString().ToConstChar() );
    StepSlideMove( false );
}

void FPlayer::GroundTraceMissed() {
    //trace_t  trace;
    //Float3  point;

    if ( GroundActor ) {
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

    GroundActor = nullptr;
    bGroundPlane = false;
    bWalking = false;
}

void FPlayer::CrashLand() {

    //if ( JumpVel < -300 ) {
    //    FSoundSpawnParameters spawnParameters;
    //    spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
    //    GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land.wav" ), this, &spawnParameters );
    //} else if ( JumpVel < -650 ) {
    //    FSoundSpawnParameters spawnParameters;
    //    spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
    //    GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land2.wav" ), this, &spawnParameters );
    //}

#if 0
    float		delta;
    float		dist;
    float		vel, acc;
    float		t;
    float		a, b, c, den;

    // decide which landing animation to use
    //if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
    //    PM_ForceLegsAnim( LEGS_LANDB );
    //} else {
    //    PM_ForceLegsAnim( LEGS_LAND );
    //}

    //pm->ps->legsTimer = TIMER_LAND;

    // calculate the exact velocity on landing
    dist = (Origin[ 1 ] - PrevOrigin[ 1 ])/UNIT_SCALE;
    vel = PrevVelocity[ 1 ];
    acc = -GRAVITY;// pm->ps->gravity;

    a = acc / 2;
    b = vel;
    c = -dist;

    den = b * b - 4 * a * c;
    if ( den < 0 ) {
        return;
    }
    t = ( -b - sqrt( den ) ) / ( 2 * a );

    delta = vel + t * acc;
    delta = delta*delta;// *0.0001;

    // ducking while falling doubles damage
    //if ( pm->ps->pm_flags & PMF_DUCKED ) {
    //    delta *= 2;
    //}

    // never take falling damage if completely underwater
    //if ( pm->waterlevel == 3 ) {
    //    return;
    //}

    //// reduce falling damage if there is standing water
    //if ( pm->waterlevel == 2 ) {
    //    delta *= 0.25;
    //}
    //if ( pm->waterlevel == 1 ) {
    //    delta *= 0.5;
    //}

    GLogger.Printf( "Delta %f Dist %f vel %s\n", delta, dist, PrevVelocity.ToString().ToConstChar() );

    if ( delta < 1 ) {
        return;
    }

    

    // create a local entity event to play the sound

    // SURF_NODAMAGE is used for bounce pads where you don't ever
    // want to take damage or play a crunch sound
    //if ( !( pml.groundTrace.surfaceFlags & SURF_NODAMAGE ) ) {
        if ( delta > 60 ) {
            //PM_AddEvent( EV_FALL_FAR );

            FSoundSpawnParameters spawnParameters;
            spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
            GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land2.wav" ), this, &spawnParameters );

        //} else if ( delta > 40 ) {
            // this is a pain grunt, so don't play it if dead
            //if ( pm->ps->stats[ STAT_HEALTH ] > 0 ) {
            //    PM_AddEvent( EV_FALL_MEDIUM );
            //}
        } else if ( delta > 7 ) {
           // PM_AddEvent( EV_FALL_SHORT );
            FSoundSpawnParameters spawnParameters;
            spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
            GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land.wav" ), this, &spawnParameters );
        } else {
            //PM_AddEvent( PM_FootstepForSurface() );
        }
    //}

    // start footstep cycle over
    //pm->ps->bobCycle = 0;

#else

#endif
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
        //GLogger.Printf("Fall\n");
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

        GroundActor = nullptr;
        bGroundPlane = false;
        bWalking = false;
        GLogger.Printf("Throwoff\n");
        return;
    }

    //GLogger.Printf( "Groundtrace normal %s fract %f\n", groundTrace.Plane.Normal.ToString().ToConstChar(), groundTrace.fraction );

    // slopes that are too steep will not be considered onground
    if ( trace.Plane.Normal[ 1 ] < MIN_WALK_NORMAL ) {
        //if ( pm->debugLevel ) {
        //    Com_Printf( "%i:steep\n", c_pmove );
        //}
        // FIXME: if they can't slide down the slope, let them
        // walk (sharp crevices)
        GroundActor = nullptr;
        bGroundPlane = true;
        bWalking = false;
        //GLogger.Printf("trace.Plane.Normal[ 1 ] < MIN_WALK_NORMAL (%s)\n", trace.Plane.Normal.ToString().ToConstChar() );
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

    if ( !GroundActor ) {
    //    // just hit the ground
    //    //if ( pm->debugLevel ) {
    //    //    Com_Printf( "%i:Land\n", c_pmove );
    //    //}

        CrashLand();

        //if ( JumpVel < -300 ) {
        //    FSoundSpawnParameters spawnParameters;
        //    spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
        //    GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land.wav" ), this, &spawnParameters );
        //} else if ( JumpVel < -650 ) {
        //    FSoundSpawnParameters spawnParameters;
        //    spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
        //    GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land2.wav" ), this, &spawnParameters );
        //}

        //GLogger.Printf( "Hit the ground vel %f\n", JumpVel );

    //    // don't do landing time if we were just going down a slope
    //    if ( pml.previous_velocity[ 1 ] < -200 ) {
    //        // don't allow another jump for a little while
    //        pm->ps->pm_flags |= PMF_TIME_LAND;
    //        pm->ps->pm_time = 250;
    //    }
    } else {
    //    JumpVel = Velocity.Y;
    }

    GroundActor = trace.Actor;

    // don't reset the Y velocity for slopes
    // Velocity[1] = 0;

    //PM_AddTouchEnt( trace.entityNum );
}

void FPlayer::TickPrePhysics( float _TimeStep ) {
    Super::TickPrePhysics( _TimeStep );
//Velocity = PhysBody->GetLinearVelocity()/UNIT_SCALE;
    //GLogger.Printf( "Velocity %f\n", JumpVel );
    TimeStep = _TimeStep;
    //ForwardVec = Spin->GetForwardVector();
    //RightVec = Spin->GetRightVector();
    //UpVec = Spin->GetUpVector();
    Origin = RootComponent->GetPosition();
    PrevOrigin = Origin;
    PrevVelocity = Velocity;
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

    if ( !GroundActor ) {
        JumpVel = Velocity.Y;
    }

    if ( bWalking ) {
        WalkMove();
    } else {
        AirMove();
    }

    if ( GroundActor ) {
        //GLogger.Printf( "GroundActor\n" );
        if ( JumpVel < -650 ) {
            FSoundSpawnParameters spawnParameters;
            spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
            GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land2.wav" ), this, &spawnParameters );
        } else if ( JumpVel < -300 ) {
            FSoundSpawnParameters spawnParameters;
            spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
            GAudioSystem.PlaySound( GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/player/land.wav" ), this, &spawnParameters );
        }
        JumpVel = 0;
    }



    RootComponent->SetPosition( Origin );
//    Origin = RootComponent->GetPosition();

    Angl viewAngles(0,0,0);
    CalcViewRoll( viewAngles, Spin->GetRightVector(), Velocity );

    float bob = CalcBob( Velocity ) * UNIT_SCALE;

    float stepY = 0;
    if ( bWalking && Origin.Y - PrevY > 0.01f )
    {
        PrevY += _TimeStep * 80 * UNIT_SCALE;
        if ( PrevY > Origin.Y )
            PrevY = Origin.Y;
        if ( Origin.Y - PrevY > 12* UNIT_SCALE )
            PrevY = Origin.Y - 12 * UNIT_SCALE;
        stepY = PrevY - Origin.Y;

        GLogger.Printf( "Step up %f\n", stepY );

    } else {
        PrevY = Origin.Y;
    }

    Spin->SetPosition( 0, 26 * UNIT_SCALE + bob + stepY, 0.0f );
    Camera->SetAngles( viewAngles );

    WeaponModel->SetPosition( 0, UNIT_SCALE/* + bob*/, bob*0.4f );
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
        Spin->SetAngles( Angles );
    }
}

void FPlayer::TurnUp( float _Value ) {
    if ( _Value ) {
        Angles.Pitch += _Value;
        Angles.Pitch = FMath::Clamp( Angles.Pitch, -90.0f, 90.0f );
        Spin->SetAngles( Angles );
    }
}

void FPlayer::DrawDebug( FDebugDraw * _DebugDraw ) {
//    TPodArray<Float3> verts;
//    TPodArray<unsigned int > ind;
//    PhysBody->CreateCollisionModel( verts, ind );

//    _DebugDraw->SetDepthTest(false);
//    _DebugDraw->SetColor(0,0,1,1);
//    _DebugDraw->DrawTriangleSoupWireframe( verts.ToPtr(), sizeof(Float3), ind.ToPtr(), ind.Length() );

    BvAxisAlignedBox bb;
    bb.Mins = PMins+RootComponent->GetPosition();
    bb.Maxs = PMaxs+RootComponent->GetPosition();
    _DebugDraw->SetDepthTest(true);
    _DebugDraw->SetColor(FColor4(0,0,1,1));
    _DebugDraw->DrawAABB( bb );
}

void FPlayer::SwitchToSpectator() {
    GGameModule->PlayerController->SetPawn( GGameModule->Spectator );
    GGameModule->PlayerController->SetViewCamera( GGameModule->Spectator->Camera );
}
