/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "WorldPhysics.h"
#include "AINavigationMesh.h"
#include "Level.h"
#include "Render/RenderWorld.h"

class AActor;
class APawn;
class AActorComponent;
class ACameraComponent;
class ATimer;
class ADebugRenderer;
class IGameModule;

/** Actor spawn parameters */
struct SActorSpawnInfo
{
    /** Initial actor transform */
    STransform SpawnTransform;

    /** Level for actor spawn */
    ALevel * Level;

    /** Who spawn the actor */
    APawn * Instigator;

    /** Actor spawned for editing */
    bool bInEditor;

    SActorSpawnInfo() = delete;

    SActorSpawnInfo( AClassMeta const * _ActorTypeClassMeta )
        : SpawnTransform( Float3::Zero(), Quat::Identity() )
        , Level( nullptr )
        , Instigator( nullptr )
        , bInEditor( false )
        , Template( nullptr )
        , ActorTypeClassMeta( _ActorTypeClassMeta )
    {
    }

    SActorSpawnInfo( uint64_t _ActorClassId );

    SActorSpawnInfo( const char * _ActorClassName );

    /** Set actor template */
    void SetTemplate( AActor const * _Template );

    /** Get actor template */
    AActor const * GetTemplate() const { return Template; }

    /** Get actor meta class */
    AClassMeta const * ActorClassMeta() const { return ActorTypeClassMeta; }

    template< typename AttributeType >
    void SetAttribute( AString const & AttributeName, AttributeType const & AttributeValue )
    {
        AString s;

        SetAttributeToString( AttributeValue, s );
        _SetAttribute( AttributeName, s );
    }

    THash<> const & GetAttributeHash() const { return AttributeHash; }
    TStdVector< std::pair< AString, AString > > const & GetAttributes() const { return Attributes; }

private:

    void _SetAttribute( AString const & AttributeName, AString const & AttributeValue );

protected:

    /** NOTE: template type meta must match ActorTypeClassMeta */
    AActor const * Template;

    /** Actor type */
    AClassMeta const * ActorTypeClassMeta;

    /** Spawn attributes (experimental) */
    THash<> AttributeHash;
    TStdVector< std::pair< AString, AString > > Attributes;
};

/** Template helper for actor spawn parameters */
template< typename ActorType >
struct TActorSpawnInfo : SActorSpawnInfo
{
    TActorSpawnInfo()
        : SActorSpawnInfo( &ActorType::ClassMeta() )
    {
    }
};

struct SVisibilityQuery
{
    /** View frustum planes */
    PlaneF const * FrustumPlanes[6];

    /** View origin */
    Float3 ViewPosition;

    /** View right vector */
    Float3 ViewRightVec;

    /** View up vector */
    Float3 ViewUpVec;

    /** Result filter */
    int VisibilityMask;

    /** Result filter */
    int QueryMask;

    // FIXME: add bool bQueryPrimitives, bool bQuerySurfaces?
};

/** Box hit result */
struct SBoxHitResult
{
    /** Box owner. Null for the surfaces. */
    ASceneComponent * Object;

    Float3 LocationMin;
    Float3 LocationMax;
    float DistanceMin;
    float DistanceMax;
    //float FractionMin;
    //float FractionMax;

    void Clear()
    {
        Core::ZeroMem( this, sizeof( *this ) );
    }
};

/** Raycast primitive */
struct SWorldRaycastPrimitive
{
    /** Primitive owner. Null for surfaces. */
    ASceneComponent * Object;

    /** First hit in array of hits */
    int FirstHit;

    /** Hits count */
    int NumHits;

    /** Closest hit num */
    int ClosestHit;
};

/** Raycast result */
struct SWorldRaycastResult
{
    /** Array of hits */
    TPodArray< STriangleHitResult > Hits;

    /** Array of primitives and surfaces */
    TPodArray< SWorldRaycastPrimitive > Primitives;

    /** Sort raycast result by hit distance */
    void Sort()
    {
        struct ASortPrimitives
        {
            TPodArray< STriangleHitResult > const & Hits;

            ASortPrimitives( TPodArray< STriangleHitResult > const & _Hits ) : Hits(_Hits) {}

            bool operator() ( SWorldRaycastPrimitive const & _A, SWorldRaycastPrimitive const & _B ) {
                const float hitDistanceA = Hits[_A.ClosestHit].Distance;
                const float hitDistanceB = Hits[_B.ClosestHit].Distance;

                return ( hitDistanceA < hitDistanceB );
            }
        } SortPrimitives( Hits );

        // Sort by primitives distance
        StdSort( Primitives.ToPtr(), Primitives.ToPtr() + Primitives.Size(), SortPrimitives );

        struct ASortHit
        {
            bool operator() ( STriangleHitResult const & _A, STriangleHitResult const & _B ) {
                return ( _A.Distance < _B.Distance );
            }
        } SortHit;

        // Sort by hit distance
        for ( SWorldRaycastPrimitive & primitive : Primitives ) {
            StdSort( Hits.ToPtr() + primitive.FirstHit, Hits.ToPtr() + (primitive.FirstHit + primitive.NumHits), SortHit );
            primitive.ClosestHit = primitive.FirstHit;
        }
    }

    /** Clear raycast result */
    void Clear()
    {
        Hits.Clear();
        Primitives.Clear();
    }
};

/** Closest hit result */
struct SWorldRaycastClosestResult
{
    /** Primitive owner. Null for surfaces. */
    ASceneComponent * Object;

    /** Hit */
    STriangleHitResult TriangleHit;

    /** Hit fraction */
    float Fraction;

    /** Triangle vertices in world coordinates */
    Float3 Vertices[3];

    /** Triangle texture coordinate for the hit */
    Float2 Texcoord;

    Float3 LightmapSample_Experimental;

    /** Clear raycast result */
    void Clear()
    {
        Core::ZeroMem( this, sizeof( *this ) );
    }
};

/** World raycast filter */
struct SWorldRaycastFilter
{
    /** Filter objects by mask */
    int VisibilityMask;
    /** VSD query mask */
    int QueryMask;
    /** Sort result by the distance */
    bool bSortByDistance;

    SWorldRaycastFilter()
    {
        VisibilityMask = ~0;
        QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        bSortByDistance = true;
    }
};

/** AWorld. Defines a game map or editor/tool scene */
class AWorld : public ABaseObject
{
    AN_CLASS( AWorld, ABaseObject )

public:
    /** Delegate to notify when any actor spawned */
    using AOnActorSpawned = TEvent< AActor * >;
    AOnActorSpawned E_OnActorSpawned;

    /** Called on each tick after physics simulation */
    using AOnPostPhysicsUpdate = TEvent< float >;
    AOnPostPhysicsUpdate E_OnPostPhysicsUpdate;

    /** Delegate to prepare for rendering */
    using AOnPrepareRenderFrontend = TEvent< ACameraComponent *, int >;
    AOnPrepareRenderFrontend E_OnPrepareRenderFrontend;

    /** Create a new world */
    static AWorld * CreateWorld();

    /** Destroy all existing worlds */
    static void DestroyWorlds();

    /** Get array of worlds */
    static TPodArray< AWorld * > const & GetWorlds() { return Worlds; }

    /** Tick the worlds */
    static void UpdateWorlds( float _TimeStep );

    /** Remove worlds, marked pending kill */
    static void KickoffPendingKillWorlds();

    /** Bake AI nav mesh */
    void BuildNavigation( SAINavigationConfig const & _NavigationConfig );

    /** Spawn a new actor */
    AActor * SpawnActor( SActorSpawnInfo const & _SpawnInfo );

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( TActorSpawnInfo< ActorType > const & _SpawnInfo )
    {
        SActorSpawnInfo const & spawnInfo = _SpawnInfo;
        return static_cast< ActorType * >( SpawnActor( spawnInfo ) );
    }

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( ALevel * _Level = nullptr )
    {
        TActorSpawnInfo< ActorType > spawnInfo;
        spawnInfo.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnInfo ) );
    }

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( STransform const & _SpawnTransform, ALevel * _Level = nullptr )
    {
        TActorSpawnInfo< ActorType > spawnInfo;
        spawnInfo.SpawnTransform = _SpawnTransform;
        spawnInfo.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnInfo ) );
    }

    /** Spawn a new actor */
    template< typename ActorType >
    ActorType * SpawnActor( Float3 const & _Position, Quat const & _Rotation, ALevel * _Level = nullptr )
    {
        TActorSpawnInfo< ActorType > spawnInfo;
        spawnInfo.SpawnTransform.Position = _Position;
        spawnInfo.SpawnTransform.Rotation = _Rotation;
        spawnInfo.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnInfo ) );
    }

    /** Load actor from the document */
    AActor * LoadActor( ADocValue const * pObject, ALevel * _Level = nullptr, bool bInEditor = false );

    /** Get all actors in the world */
    TPodArray< AActor * > const & GetActors() const { return Actors; }

    /** Serialize world to the document */
    TRef< ADocObject > Serialize() override;

    /** Destroy this world */
    void Destroy();

    /** Destroy all actors in the world */
    void DestroyActors();

    /** Add level to the world */
    void AddLevel( ALevel * _Level );

    /** Remove level from the world */
    void RemoveLevel( ALevel * _Level );

    /** Get world's persistent level */
    ALevel * GetPersistentLevel() { return PersistentLevel; }

    /** Get all levels in the world */
    TPodArray< ALevel * > const & GetArrayOfLevels() const { return ArrayOfLevels; }

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

    /** Physics simulation refresh rate */
    void SetPhysicsHertz( int _Hertz );

    /** Contact solver split impulse. Disabled by default for performance */
    void SetContactSolverSplitImpulse( bool _SplitImpulse );

    /** Contact solver iterations count */
    void SetContactSolverIterations( int _InterationsCount );

    /** Set world gravity vector */
    void SetGravityVector( Float3 const & _Gravity );

    /** Get world gravity vector */
    Float3 const & GetGravityVector() const;

    /** Is in physics update now */
    bool IsDuringPhysicsUpdate() const { return WorldPhysics.bDuringPhysicsUpdate; }

    /** Is world destroyed, but not removed yet. */
    bool IsPendingKill() const { return bPendingKill; }

    /** Scale audio volume in the entire world */
    void SetAudioVolume( float Volume )
    {
        AudioVolume = Math::Saturate( Volume );
    }

    /** Scale audio volume in the entire world */
    float GetAudioVolume() const { return AudioVolume; }

    void SetGlobalIrradianceMap( int Index );
    int GetGlobalIrradianceMap() const { return GlobalIrradianceMap; }

    void SetGlobalReflectionMap( int Index );
    int GetGlobalReflectionMap() const { return GlobalReflectionMap; }

    /** Per-triangle raycast */
    bool Raycast( SWorldRaycastResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr ) const;

    /** Per-bounds raycast */
    bool RaycastBounds( TPodArray< SBoxHitResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr ) const;

    /** Per-triangle raycast */
    bool RaycastClosest( SWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr ) const;

    /** Per-bounds raycast */
    bool RaycastClosestBounds( SBoxHitResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr ) const;

    /** Trace collision bodies */
    bool Trace( TPodArray< SCollisionTraceResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        return WorldPhysics.Trace( _Result, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceClosest( SCollisionTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        return WorldPhysics.TraceClosest( _Result, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceSphere( SCollisionTraceResult & _Result, float _Radius, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        return WorldPhysics.TraceSphere( _Result, _Radius, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceBox( SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        return WorldPhysics.TraceBox( _Result, _Mins, _Maxs, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceBox2( TPodArray< SCollisionTraceResult > & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        return WorldPhysics.TraceBox2( _Result, _Mins, _Maxs, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceCylinder( SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        return WorldPhysics.TraceCylinder( _Result, _Mins, _Maxs, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceCapsule( SCollisionTraceResult & _Result, float _CapsuleHeight, float CapsuleRadius, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        return WorldPhysics.TraceCapsule( _Result, _CapsuleHeight, CapsuleRadius, _RayStart, _RayEnd, _QueryFilter );
    }

    /** Trace collision bodies */
    bool TraceConvex( SCollisionTraceResult & _Result, SConvexSweepTest const & _SweepTest ) const
    {
        return WorldPhysics.TraceConvex( _Result, _SweepTest );
    }

    /** Query objects in sphere */
    void QueryHitProxies( TPodArray< AHitProxy * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        WorldPhysics.QueryHitProxies_Sphere( _Result, _Position, _Radius, _QueryFilter );
    }

    /** Query objects in box */
    void QueryHitProxies( TPodArray< AHitProxy * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        WorldPhysics.QueryHitProxies_Box( _Result, _Position, _HalfExtents, _QueryFilter );
    }

    /** Query objects in AABB */
    void QueryHitProxies( TPodArray< AHitProxy * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        WorldPhysics.QueryHitProxies( _Result, _BoundingBox, _QueryFilter );
    }

    /** Query actors in sphere */
    void QueryActors( TPodArray< AActor * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        WorldPhysics.QueryActors_Sphere( _Result, _Position, _Radius, _QueryFilter );
    }

    /** Query actors in box */
    void QueryActors( TPodArray< AActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        WorldPhysics.QueryActors_Box( _Result, _Position, _HalfExtents, _QueryFilter );
    }

    /** Query actors in AABB */
    void QueryActors( TPodArray< AActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter = nullptr ) const
    {
        WorldPhysics.QueryActors( _Result, _BoundingBox, _QueryFilter );
    }

    /** Query collisions with sphere */
    void QueryCollision_Sphere( TPodArray< SCollisionQueryResult > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter ) const
    {
        WorldPhysics.QueryCollision_Sphere( _Result, _Position, _Radius, _QueryFilter );
    }

    /** Query collisions with box */
    void QueryCollision_Box( TPodArray< SCollisionQueryResult > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter ) const
    {
        WorldPhysics.QueryCollision_Box( _Result, _Position, _HalfExtents, _QueryFilter );
    }

    /** Query collisions with AABB */
    void QueryCollision( TPodArray< SCollisionQueryResult > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter ) const
    {
        WorldPhysics.QueryCollision( _Result, _BoundingBox, _QueryFilter );
    }

    /** Query visible primitives */
    void QueryVisiblePrimitives( TPodArray< SPrimitiveDef * > & VisPrimitives, TPodArray< SSurfaceDef * > & VisSurfs, int * VisPass, SVisibilityQuery const & InQuery );

    /** Apply amount of damage in specified radius */
    void ApplyRadialDamage( float _DamageAmount, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter = nullptr );

    //
    // Internal
    //

    // World physics. Used by a hit proxy.
    AWorldPhysics & GetPhysics()
    {
        return WorldPhysics;
    }

    ARenderWorld & GetRender()
    {
        return WorldRender;
    }

    AAINavigationMesh & GetNavigationMesh()
    {
        return NavigationMesh;
    }

    void DrawDebug( ADebugRenderer * InRenderer );

protected:
    void BeginPlay();

    void EndPlay();

    void Tick( float _TimeStep );

    AWorld();
    ~AWorld();

private:
    // Allow timer to register itself in the world
    friend class ATimer;
    void AddTimer( ATimer * _Timer );
    void RemoveTimer( ATimer * _Timer );

private:
    // Allow actor to add self to pendingkill list
    friend class AActor;
    AActor * PendingKillActors = nullptr;

private:
    // Allow actor component to add self to pendingkill list
    friend class AActorComponent;
    AActorComponent * PendingKillComponents = nullptr;

private:
    void BroadcastActorSpawned( AActor * _SpawnedActor );

    void KickoffPendingKillObjects();

    void UpdatePauseStatus();
    void UpdateTimers( float _TimeStep );
    void UpdateActors( float _TimeStep );
    void UpdateActorsPrePhysics( float _TimeStep );
    void UpdateActorsPostPhysics( float _TimeStep );
    void UpdateLevels( float _TimeStep );
    void UpdatePhysics( float _TimeStep );
    void UpdateSkinning();

    void HandlePrePhysics( float _TimeStep );
    void HandlePostPhysics( float _TimeStep );

    TPodArray< AActor * > Actors;

    bool bPauseRequest = false;
    bool bUnpauseRequest = false;
    bool bPaused = false;
    bool bResetGameplayTimer = false;

    // Game virtual time based on variable frame step
    int64_t GameRunningTimeMicro = 0;
    int64_t GameRunningTimeMicroAfterTick = 0;

    // Gameplay virtual time based on fixed frame step, running when unpaused
    int64_t GameplayTimeMicro = 0;
    int64_t GameplayTimeMicroAfterTick = 0;

    struct STimerCmd
    {
        enum { ADD, REMOVE } Command;
        ATimer * TimerCb;
    };

    TPodArray< STimerCmd > TimerCmd;

    ATimer * TimerList = nullptr;
    ATimer * TimerListTail = nullptr;

    bool bDuringTimerTick = false;

    int IndexInGameArrayOfWorlds = -1;

    bool bPendingKill = false;

    AWorld * NextPendingKillWorld = nullptr;
    static AWorld * PendingKillWorlds;

    // All existing worlds
    static TPodArray< AWorld * > Worlds;

    TRef< ALevel > PersistentLevel;
    TPodArray< ALevel * > ArrayOfLevels;

    /** Scale audio volume in the entire world */
    float AudioVolume = 1.0f;

    int GlobalIrradianceMap = 0;
    int GlobalReflectionMap = 0;

    // Physics extension
    AWorldPhysics WorldPhysics;
    // Renderer extension
    ARenderWorld WorldRender;
    // Path finding extension
    AAINavigationMesh NavigationMesh;
};

#include "Actors/Actor.h"
#include "Components/ActorComponent.h"

/*

TActorIterator

Iterate world actors

Example:
for ( TActorIterator< AMyActor > it( GetWorld() ) ; it ; ++it ) {
    AMyActor * actor = *it;
    ...
}

*/
template< typename T >
struct TActorIterator
{
    explicit TActorIterator( AWorld * _World )
        : Actors( _World->GetActors() ), i(0)
    {
        Next();
    }

    operator bool() const
    {
        return Actor != nullptr;
    }

    T * operator++()
    {
        Next();
        return Actor;
    }

    T * operator++(int)
    {
        T * a = Actor;
        Next();
        return a;
    }

    T * operator*() const
    {
        return Actor;
    }

    T * operator->() const
    {
        return Actor;
    }

    void Next()
    {
        AActor * a;
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
    TPodArray< AActor * > const & Actors;
    T * Actor;
    int i;
};

/*

TActorIterator2

Iterate world actors

Example:

TActorIterator2< AMyActor > it( GetWorld() );
for ( AMyActor * actor = it.First() ; actor ; actor = it.Next() ) {
    ...
}

*/
template< typename T >
struct TActorIterator2 {
    explicit TActorIterator2( AWorld * _World )
        : Actors( _World->GetActors() ), i( 0 )
    {
    }

    T * First()
    {
        i = 0;
        return Next();
    }

    T * Next()
    {
        AActor * a;
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
    TPodArray< AActor * > const & Actors;
    int i;
};

/*

TComponentIterator

Iterate actor components

Example:
for ( TComponentIterator< AMyComponent > it( actor ) ; it ; ++it ) {
    AMyComponent * component = *it;
    ...
}

*/
template< typename T >
struct TComponentIterator {
    explicit TComponentIterator( AActor * _Actor )
        : Components( _Actor->GetComponents() ), i(0)
    {
        Next();
    }

    operator bool() const
    {
        return Component != nullptr;
    }

    T * operator++()
    {
        Next();
        return Component;
    }

    T * operator++(int)
    {
        T * a = Component;
        Next();
        return a;
    }

    T * operator*() const
    {
        return Component;
    }

    T * operator->() const
    {
        return Component;
    }

    void Next()
    {
        AActorComponent * a;
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
    AArrayOfActorComponents const & Components;
    T * Component;
    int i;
};

/*

TComponentIterator2

Iterate actor components

Example:

TComponentIterator2< AMyComponent > it( this );
for ( AMyComponent * component = it.First() ; component ; component = it.Next() ) {
    ...
}

*/
template< typename T >
struct TComponentIterator2
{
    explicit TComponentIterator2( AActor * _Actor )
        : Components( _Actor->GetComponents() ), i( 0 )
    {
    }

    T * First()
    {
        i = 0;
        return Next();
    }

    T * Next()
    {
        AActorComponent * a;
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
    TPodArray< AActorComponent * > const & Components;
    int i;
};
