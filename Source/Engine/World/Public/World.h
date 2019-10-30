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

#include "Level.h"
#include "WorldRaycastQuery.h"
#include "WorldCollisionQuery.h"

class FActor;
class FPawn;
class FActorComponent;
class FMeshComponent;
class FSkinnedComponent;
class FDirectionalLightComponent;
class FPointLightComponent;
class FSpotLightComponent;
class FTimer;
class FDebugDraw;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btSoftRigidDynamicsWorld;
struct btSoftBodyWorldInfo;
class IGameModule;

/** Actor spawn parameters */
struct FActorSpawnInfo {

    /** Initial actor transform */
    FTransform SpawnTransform;

    /** Level for actor spawn */
    FLevel * Level;

    /** Who spawn the actor */
    FPawn * Instigator;

    FActorSpawnInfo() = delete;

    FActorSpawnInfo( FClassMeta const * _ActorTypeClassMeta )
        : Level( nullptr )
        , Instigator( nullptr )
        , Template( nullptr )
        , ActorTypeClassMeta( _ActorTypeClassMeta )
    {
    }

    FActorSpawnInfo( uint64_t _ActorClassId );

    FActorSpawnInfo( const char * _ActorClassName );

    /** Set actor template */
    void SetTemplate( FActor const * _Template );

    /** Get actor template */
    FActor const * GetTemplate() const { return Template; }

    /** Get actor meta class */
    FClassMeta const * ActorClassMeta() const { return ActorTypeClassMeta; }

protected:

    /** NOTE: template type meta must match ActorTypeClassMeta */
    FActor const * Template;

    /** Actor type */
    FClassMeta const * ActorTypeClassMeta;
};

/** Template helper for actor spawn parameters */
template< typename ActorType >
struct TActorSpawnInfo : FActorSpawnInfo
{
    TActorSpawnInfo()
        : FActorSpawnInfo( &ActorType::ClassMeta() )
    {
    }
};

/** Collision contact */
struct FCollisionContact {
    class btPersistentManifold * Manifold;

    FActor * ActorA;
    FActor * ActorB;
    FPhysicalBody * ComponentA;
    FPhysicalBody * ComponentB;

    bool bActorADispatchContactEvents;
    bool bActorBDispatchContactEvents;
    bool bActorADispatchOverlapEvents;
    bool bActorBDispatchOverlapEvents;

    bool bComponentADispatchContactEvents;
    bool bComponentBDispatchContactEvents;
    bool bComponentADispatchOverlapEvents;
    bool bComponentBDispatchOverlapEvents;

    int Hash() const {
        return ( size_t )ComponentA + ( size_t )ComponentB;
    }
};

/** World. Defines a game map or editor/tool scene */
class ANGIE_API FWorld : public FBaseObject
{
    AN_CLASS( FWorld, FBaseObject )

    friend class FActor;
    friend class FActorComponent;
    friend struct FWorldCollisionQuery;

public:
    /** Physics refresh rate */
    int PhysicsHertz = 60;

    /** Enable interpolation during physics simulation */
    bool bEnablePhysicsInterpolation = true;

    /** Contact solver split impulse. Disabled by default for performance */
    bool bContactSolverSplitImpulse = false;

    /** Contact solver iterations count */
    int NumContactSolverIterations = 10;

    /** Scale audio volume in the entire world */
    float AudioVolume = 1.0f;

    /** Delegate to notify when any actor spawned */
    using FOnActorSpawned = TEvent< FActor * >;
    FOnActorSpawned E_OnActorSpawned;

    /** Create a new world */
    static FWorld * CreateWorld();

    /** Destroy all existing worlds */
    static void DestroyWorlds();

    /** Get array of worlds */
    static TPodArray< FWorld * > const & GetWorlds() { return Worlds; }

    /** Tick worlds */
    static void UpdateWorlds( IGameModule * _GameModule, float _TimeStep );

    /** Remove worlds, marked pending kill */
    static void KickoffPendingKillWorlds();

    /** Spawn a new actor */
    FActor * SpawnActor( FActorSpawnInfo const & _SpawnParameters );

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( TActorSpawnInfo< ActorType > const & _SpawnParameters ) {
        FActorSpawnInfo const & spawnParameters = _SpawnParameters;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( FLevel * _Level = nullptr ) {
        TActorSpawnInfo< ActorType > spawnParameters;
        spawnParameters.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( FTransform const & _SpawnTransform, FLevel * _Level = nullptr ) {
        TActorSpawnInfo< ActorType > spawnParameters;
        spawnParameters.SpawnTransform = _SpawnTransform;
        spawnParameters.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( Float3 const & _Position, Quat const & _Rotation, FLevel * _Level = nullptr ) {
        TActorSpawnInfo< ActorType > spawnParameters;
        spawnParameters.SpawnTransform.Position = _Position;
        spawnParameters.SpawnTransform.Rotation = _Rotation;
        spawnParameters.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    /** Load actor from the document */
    FActor * LoadActor( FDocument const & _Document, int _FieldsHead, FLevel * _Level = nullptr );

    /** Find actor by name */
    FActor * FindActor( const char * _UniqueName );

    /** Get all actors in world */
    TPodArray< FActor * > const & GetActors() const { return Actors; }

    /** Serialize world to the document */
    int Serialize( FDocument & _Doc ) override;

    /** Destroy this world */
    void Destroy();

    /** Destroy all actors in the world */
    void DestroyActors();

    /** Add level to the world */
    void AddLevel( FLevel * _Level );

    /** Remove level from the world */
    void RemoveLevel( FLevel * _Level );

    /** Get world's persistent level */
    FLevel * GetPersistentLevel() { return PersistentLevel; }

    /** Get all levels in the world */
    TPodArray< FLevel * > const & GetArrayOfLevels() const { return ArrayOfLevels; }

    /** Unique name generator for actors */
    FString GenerateActorUniqueName( const char * _Name );

    /** Pause the game. Freezes world and actor ticking since the next game tick. */
    void SetPaused( bool _Paused );

    /** Returns current pause state */
    bool IsPaused() const;

    /** Game virtual time based on variable frame step */
    int64_t GetRunningTimeMicro() const { return GameRunningTimeMicro; }

    /** Gameplay virtual time based on fixed frame step, running when unpaused */
    int64_t GetGameplayTimeMicro() const { return GameplayTimeMicro; }

    /** Reset gameplay timer to zero. This is delayed operation. */
    void ResetGameplayTimer();

    /** Set world gravity vector */
    void SetGravityVector( Float3 const & _Gravity );

    /** Get world gravity vector */
    Float3 const & GetGravityVector() const;

    /** Is in physics update now */
    bool IsDuringPhysicsUpdate() const { return bDuringPhysicsUpdate; }

    /** Is world destroyed, but not removed yet. */
    bool IsPendingKill() const { return bPendingKill; }

    /** Per-triangle raycast */
    bool Raycast( FWorldRaycastResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter = nullptr ) const {
        return FWorldRaycastQuery::Raycast( this, _Result, _RayStart, _RayEnd, _Filter );
    }

    /** Per-AABB raycast */
    bool RaycastAABB( TPodArray< FBoxHitResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter = nullptr ) const {
        return FWorldRaycastQuery::RaycastAABB( this, _Result, _RayStart, _RayEnd, _Filter );
    }

    /** Per-triangle raycast */
    bool RaycastClosest( FWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter = nullptr ) const {
        return FWorldRaycastQuery::RaycastClosest( this, _Result, _RayStart, _RayEnd, _Filter );
    }

    /** Per-AABB raycast */
    bool RaycastClosestAABB( FBoxHitResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter = nullptr ) const {
        return FWorldRaycastQuery::RaycastClosestAABB( this, _Result, _RayStart, _RayEnd, _Filter );
    }

    /** Trace collision bodies */
    bool Trace( TPodArray< FCollisionTraceResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        return FWorldCollisionQuery::Trace( this, _Result, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceClosest( FCollisionTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        return FWorldCollisionQuery::TraceClosest( this, _Result, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceSphere( FCollisionTraceResult & _Result, float _Radius, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        return FWorldCollisionQuery::TraceSphere( this, _Result, _Radius, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceBox( FCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        return FWorldCollisionQuery::TraceBox( this, _Result, _Mins, _Maxs, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceCylinder( FCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        return FWorldCollisionQuery::TraceCylinder( this, _Result, _Mins, _Maxs, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceCapsule( FCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        return FWorldCollisionQuery::TraceCapsule( this, _Result, _Mins, _Maxs, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceConvex( FCollisionTraceResult & _Result, FConvexSweepTest const & _SweepTest ) const {
        return FWorldCollisionQuery::TraceConvex( this, _Result, _SweepTest );
    }

    /** Query objects in sphere */
    void QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        FWorldCollisionQuery::QueryPhysicalBodies( this, _Result, _Position, _Radius, _QueryFilter );
    }

    /** Query objects in box */
    void QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        FWorldCollisionQuery::QueryPhysicalBodies( this, _Result, _Position, _HalfExtents, _QueryFilter );
    }

    /** Query objects in AABB */
    void QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, BvAxisAlignedBox const & _BoundingBox, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        FWorldCollisionQuery::QueryPhysicalBodies( this, _Result, _BoundingBox, _QueryFilter );
    }

    /** Query objects in sphere */
    void QueryActors( TPodArray< FActor * > & _Result, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        FWorldCollisionQuery::QueryActors( this, _Result, _Position, _Radius, _QueryFilter );
    }

    /** Query objects in box */
    void QueryActors( TPodArray< FActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        FWorldCollisionQuery::QueryActors( this, _Result, _Position, _HalfExtents, _QueryFilter );
    }

    /** Query objects in AABB */
    void QueryActors( TPodArray< FActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, FCollisionQueryFilter const * _QueryFilter = nullptr ) const {
        FWorldCollisionQuery::QueryActors( this, _Result, _BoundingBox, _QueryFilter );
    }

    /** Apply amount of damage in specified radius */
    void ApplyRadialDamage( float _DamageAmount, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Get static and skinned meshes in the world */
    FMeshComponent * GetMeshes() { return MeshList; }
    FMeshComponent * GetMeshes() const { return MeshList; }

    /** Get skinned meshes in the world */
    FSkinnedComponent * GetSkinnedMeshes() { return SkinnedMeshList; }

    /** Get directional lights in the world */
    FDirectionalLightComponent * GetDirectionalLights() { return DirectionalLightList; }

    /** Get point lights in the world */
    FPointLightComponent * GetPointLights() { return PointLightList; }

    /** Get spot lights in the world */
    FSpotLightComponent * GetSpotLights() { return SpotLightList; }

    //
    // Internal
    //

    void AddMesh( FMeshComponent * _Mesh );
    void RemoveMesh( FMeshComponent * _Mesh );

    void AddSkinnedMesh( FSkinnedComponent * _Skeleton );
    void RemoveSkinnedMesh( FSkinnedComponent * _Skeleton );

    void AddDirectionalLight( FDirectionalLightComponent * _Light );
    void RemoveDirectionalLight( FDirectionalLightComponent * _Light );

    void AddPointLight( FPointLightComponent * _Light );
    void RemovePointLight( FPointLightComponent * _Light );

    void AddSpotLight( FSpotLightComponent * _Light );
    void RemoveSpotLight( FSpotLightComponent * _Light );

    void AddPhysicalBody( FPhysicalBody * _PhysicalBody );
    void RemovePhysicalBody( FPhysicalBody * _PhysicalBody );

    btSoftRigidDynamicsWorld * GetPhysicsWorld() { return PhysicsWorld; }

    btSoftBodyWorldInfo * GetSoftBodyWorldInfo() { return SoftBodyWorldInfo; }

    void RenderFrontend_AddInstances( FRenderFrontendDef * _Def );
    void RenderFrontend_AddDirectionalShadowmapInstances( FRenderFrontendDef * _Def );

    void DrawDebug( FDebugDraw * _DebugDraw, int _FrameNumber );
    int GetFirstDebugDrawCommand() const { return FirstDebugDrawCommand; }
    int GetDebugDrawCommandCount() const { return DebugDrawCommandCount; }

protected:
    void BeginPlay();

    void EndPlay();

    void Tick( float _TimeStep );

    FWorld();

private:
    void SimulatePhysics( float _TimeStep );

    void OnPrePhysics( float _TimeStep );
    void OnPostPhysics( float _TimeStep );

    static void OnPrePhysics( class btDynamicsWorld * _World, float _TimeStep );
    static void OnPostPhysics( class btDynamicsWorld * _World, float _TimeStep );

    void GenerateContactPoints( int _ContactIndex, FCollisionContact & _Contact );
    void DispatchContactAndOverlapEvents();

    void BroadcastActorSpawned( FActor * _SpawnedActor );

    void RegisterTimer( FTimer * _Timer );      // friend FActor
    void UnregisterTimer( FTimer * _Timer );    // friend FActor

    void KickoffPendingKillObjects();

    TPodArray< FActor * > Actors;

    FMeshComponent * MeshList;
    FMeshComponent * MeshListTail;
    FSkinnedComponent * SkinnedMeshList;
    FSkinnedComponent * SkinnedMeshListTail;
    FDirectionalLightComponent * DirectionalLightList;
    FDirectionalLightComponent * DirectionalLightListTail;
    FPointLightComponent * PointLightList;
    FPointLightComponent * PointLightListTail;
    FSpotLightComponent * SpotLightList;
    FSpotLightComponent * SpotLightListTail;

    bool bPauseRequest;
    bool bUnpauseRequest;
    bool bPaused;
    bool bResetGameplayTimer;

    // Game virtual time based on variable frame step
    int64_t GameRunningTimeMicro;
    int64_t GameRunningTimeMicroAfterTick;

    // Gameplay virtual time based on fixed frame step, running when unpaused
    int64_t GameplayTimeMicro;
    int64_t GameplayTimeMicroAfterTick;

    FTimer * TimerList;
    FTimer * TimerListTail;

    int VisFrame;
    int FirstDebugDrawCommand;
    int DebugDrawCommandCount;
    int DebugDrawFrame;

    int IndexInGameArrayOfWorlds = -1;

    bool bPendingKill;

    FActor * PendingKillActors; // friend FActor
    FActorComponent * PendingKillComponents; // friend FActorComponent

    FWorld * NextPendingKillWorld;
    static FWorld * PendingKillWorlds;

    // All existing worlds
    static TPodArray< FWorld * > Worlds;

    TRef< FLevel > PersistentLevel;
    TPodArray< FLevel * > ArrayOfLevels;
//public:
    btBroadphaseInterface * PhysicsBroadphase;
    btDefaultCollisionConfiguration * CollisionConfiguration;
    btCollisionDispatcher * CollisionDispatcher;
    btSequentialImpulseConstraintSolver * ConstraintSolver;
    btSoftBodyWorldInfo * SoftBodyWorldInfo;
    btSoftRigidDynamicsWorld * PhysicsWorld;
    TPodArray< FCollisionContact > CollisionContacts[ 2 ];
    THash<> ContactHash[ 2 ];
    TPodArray< FContactPoint > ContactPoints;
    FPhysicalBody * PendingAddToWorldHead;
    FPhysicalBody * PendingAddToWorldTail;

    Float3 GravityVector;
    bool bGravityDirty;
    bool bDuringPhysicsUpdate;
    float TimeAccumulation;
    int FixedTickNumber;
};


#include "Actors/Actor.h"
#include "Components/ActorComponent.h"

/*

TActorIterator

Iterate world actors

Example:
for ( TActorIterator< FMyActor > it( GetWorld() ) ; it ; ++it ) {
    FMyActor * actor = *it;
    ...
}

*/
template< typename T >
struct TActorIterator {
    explicit TActorIterator( FWorld * _World )
        : Actors( _World->GetActors() ), i(0)
    {
        Next();
    }

    operator bool() const {
        return Actor != nullptr;
    }

    T * operator++() {
        Next();
        return Actor;
    }

    T * operator++(int) {
        T * a = Actor;
        Next();
        return a;
    }

    T * operator*() const {
        return Actor;
    }

    T * operator->() const {
        return Actor;
    }

    void Next() {
        FActor * a;
        while ( i < Actors.Size() ) {
            a = Actors[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                Actor = static_cast< T * >( a );
                return;
            }
        }
        Actor = nullptr;
    }

private:
    TPodArray< FActor * > const & Actors;
    T * Actor;
    int i;
};

/*

TActorIterator2

Iterate world actors

Example:

TActorIterator2< FMyActor > it( this );
for ( FMyActor * actor = it.First() ; actor ; actor = it.Next() ) {
    ...
}

*/
template< typename T >
struct TActorIterator2 {
    explicit TActorIterator2( FWorld * _World )
        : Actors( _World->GetActors() ), i( 0 )
    {
    }

    T * First() {
        i = 0;
        return Next();
    }

    T * Next() {
        FActor * a;
        while ( i < Actors.Size() ) {
            a = Actors[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                return static_cast< T * >( a );
            }
        }
        return nullptr;
    }

private:
    TPodArray< FActor * > const & Actors;
    int i;
};

/*

TComponentIterator

Iterate actor components

Example:
for ( TComponentIterator< FMyComponent > it( actor ) ; it ; ++it ) {
    FMyComponent * component = *it;
    ...
}

*/
template< typename T >
struct TComponentIterator {
    explicit TComponentIterator( FActor * _Actor )
        : Components( _Actor->GetComponents() ), i(0)
    {
        Next();
    }

    operator bool() const {
        return Component != nullptr;
    }

    T * operator++() {
        Next();
        return Component;
    }

    T * operator++(int) {
        T * a = Component;
        Next();
        return a;
    }

    T * operator*() const {
        return Component;
    }

    T * operator->() const {
        return Component;
    }

    void Next() {
        FActorComponent * a;
        while ( i < Components.Size() ) {
            a = Components[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                Component = static_cast< T * >( a );
                return;
            }
        }
        Component = nullptr;
    }

private:
    TPodArray< FActorComponent * > const & Components;
    T * Component;
    int i;
};

/*

TComponentIterator2

Iterate actor components

Example:

TComponentIterator2< FMyComponent > it( this );
for ( FMyComponent * component = it.First() ; component ; component = it.Next() ) {
    ...
}

*/
template< typename T >
struct TComponentIterator2 {
    explicit TComponentIterator2( FActor * _Actor )
        : Components( _Actor->GetComponents() ), i( 0 )
    {
    }

    T * First() {
        i = 0;
        return Next();
    }

    T * Next() {
        FActorComponent * a;
        while ( i < Components.Size() ) {
            a = Components[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                return static_cast< T * >( a );
            }
        }
        return nullptr;
    }

private:
    TPodArray< FActorComponent * > const & Components;
    int i;
};
