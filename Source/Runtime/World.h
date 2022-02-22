/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "PhysicsSystem.h"
#include "AINavigationMesh.h"
#include "SkinningSystem.h"
#include "LightingSystem.h"
#include "Level.h"
#include "ScriptEngine.h"

#include <Platform/Platform.h>

class AActor;
class AActorComponent;
class ACameraComponent;
class ATimer;
class ADebugRenderer;
class AGameModule;
class AEnvironmentMap;

/** Actor spawn parameters */
struct SActorSpawnInfo
{
    /** Initial actor transform */
    STransform SpawnTransform;

    /** Level for actor spawn */
    ALevel* Level;

    /** Who spawn the actor */
    AActor* Instigator;

    /** Actor spawned for editing */
    bool bInEditor;

    SActorSpawnInfo() = delete;

    SActorSpawnInfo(AClassMeta const* _ActorTypeClassMeta) :
        SpawnTransform(Float3::Zero(), Quat::Identity()), Level(nullptr), Instigator(nullptr), bInEditor(false), Template(nullptr), ActorTypeClassMeta(_ActorTypeClassMeta)
    {
    }

    SActorSpawnInfo(uint64_t _ActorClassId);

    SActorSpawnInfo(const char* _ActorClassName);

    /** Set actor template */
    void SetTemplate(AActor const* _Template);

    /** Get actor template */
    AActor const* GetTemplate() const { return Template; }

    /** Get actor meta class */
    AClassMeta const* ActorClassMeta() const { return ActorTypeClassMeta; }

protected:
    /** NOTE: template type meta must match ActorTypeClassMeta */
    AActor const* Template;

    /** Actor type */
    AClassMeta const* ActorTypeClassMeta;
};

/** Template helper for actor spawn parameters */
template <typename ActorType>
struct TActorSpawnInfo : SActorSpawnInfo
{
    TActorSpawnInfo() :
        SActorSpawnInfo(&ActorType::ClassMeta())
    {
    }
};

/** AWorld. Defines a game map or editor/tool scene */
class AWorld : public ABaseObject
{
    HK_CLASS(AWorld, ABaseObject)

public:
    /** Delegate to notify when any actor spawned */
    using AOnActorSpawned = TEvent<AActor*>;
    AOnActorSpawned E_OnActorSpawned;

    /** Called on each tick after physics simulation */
    using AOnPostPhysicsUpdate = TEvent<float>;
    AOnPostPhysicsUpdate E_OnPostPhysicsUpdate;

    /** Delegate to prepare for rendering */
    using AOnPrepareRenderFrontend = TEvent<ACameraComponent*, int>;
    AOnPrepareRenderFrontend E_OnPrepareRenderFrontend;

    AVisibilitySystem VisibilitySystem;

    APhysicsSystem PhysicsSystem;

    AAINavigationMesh NavigationMesh;

    ASkinningSystem SkinningSystem;

    ALightingSystem LightingSystem;

    /** Create a new world */
    static AWorld* CreateWorld();

    /** Destroy all existing worlds */
    static void DestroyWorlds();

    /** Get array of worlds */
    static TPodVector<AWorld*> const& GetWorlds() { return Worlds; }

    /** Tick the worlds */
    static void UpdateWorlds(float TimeStep);

    /** Remove worlds, marked pending kill */
    static void KillWorlds();

    /** Bake AI nav mesh */
    void BuildNavigation(SAINavigationConfig const& _NavigationConfig);

    /** DEPRECATED!!!! Spawn a new actor */
    AActor* SpawnActor(SActorSpawnInfo const& _SpawnInfo);

    /** DEPRECATED!!!! Spawn a new actor */
    template <typename ActorType>
    ActorType* SpawnActor(TActorSpawnInfo<ActorType> const& _SpawnInfo)
    {
        SActorSpawnInfo const& spawnInfo = _SpawnInfo;
        return static_cast<ActorType*>(SpawnActor(spawnInfo));
    }

    /** Spawn empty actor */
    AActor* SpawnActor2(STransform const& SpawnTransform = {}, AActor* Instigator = {}, ALevel* Level = {}, bool bInEditor = {});

    /** Spawn actor with definition */
    AActor* SpawnActor2(class AActorDefinition* ActorDef, STransform const& SpawnTransform = {}, AActor* Instigator = {}, ALevel* Level = {}, bool bInEditor = {});

    /** Spawn actor with script module */
    AActor* SpawnActor2(AString const& ScriptModule, STransform const& SpawnTransform = {}, AActor* Instigator = {}, ALevel* Level = {}, bool bInEditor = {});

    /** Spawn actor with C++ module */
    AActor* SpawnActor2(AClassMeta const* ActorClass, STransform const& SpawnTransform = {}, AActor* Instigator = {}, ALevel* Level = {}, bool bInEditor = {});

    /** Spawn actor with C++ module */
    template <typename T>
    T* SpawnActor2(STransform const& SpawnTransform = {}, AActor* Instigator = {}, ALevel* Level = {}, bool bInEditor = {})
    {
        if (T::ClassMeta().Factory() != &AActor::Factory())
            CriticalError("AWorld::SpawnActor: not an actor class\n");

        return static_cast<T*>(SpawnActor2(&T::ClassMeta(), SpawnTransform, Instigator, Level, bInEditor));
    }

    /** Clone actor */
    AActor* SpawnActor2(AActor const* Template, STransform const& SpawnTransform = {}, AActor* Instigator = {}, ALevel* Level = {}, bool bInEditor = {});

    /** Get all actors in the world */
    TPodVector<AActor*> const& GetActors() const { return Actors; }

    /** Destroy this world */
    void Destroy();

    /** Destroy all actors in the world */
    void DestroyActors();

    /** Add level to the world */
    void AddLevel(ALevel* _Level);

    /** Remove level from the world */
    void RemoveLevel(ALevel* _Level);

    /** Get world's persistent level */
    ALevel* GetPersistentLevel() { return PersistentLevel; }

    /** Get all levels in the world */
    TPodVector<ALevel*> const& GetArrayOfLevels() const { return ArrayOfLevels; }

    /** Pause the game. Freezes world and actor ticking since the next game tick. */
    void SetPaused(bool _Paused);

    /** Returns current pause state */
    bool IsPaused() const;

    /** Game virtual time based on variable frame step */
    int64_t GetRunningTimeMicro() const { return GameRunningTimeMicro; }

    /** Gameplay virtual time based on fixed frame step, running when unpaused */
    int64_t GetGameplayTimeMicro() const { return GameplayTimeMicro; }

    /** Reset gameplay timer to zero. This is delayed operation. */
    void ResetGameplayTimer();

    /** Physics simulation refresh rate */
    void SetPhysicsHertz(int _Hertz);

    /** Contact solver split impulse. Disabled by default for performance */
    void SetContactSolverSplitImpulse(bool _SplitImpulse);

    /** Contact solver iterations count */
    void SetContactSolverIterations(int _InterationsCount);

    /** Set world gravity vector */
    void SetGravityVector(Float3 const& _Gravity);

    /** Get world gravity vector */
    Float3 const& GetGravityVector() const;

    /** Is in physics update now */
    bool IsDuringPhysicsUpdate() const { return PhysicsSystem.bDuringPhysicsUpdate; }

    /** Is world destroyed, but not removed yet. */
    bool IsPendingKill() const { return bPendingKill; }

    /** Scale audio volume in the entire world */
    void SetAudioVolume(float Volume)
    {
        AudioVolume = Math::Saturate(Volume);
    }

    /** Scale audio volume in the entire world */
    float GetAudioVolume() const { return AudioVolume; }

    void SetGlobalEnvironmentMap(AEnvironmentMap* EnvironmentMap);
    AEnvironmentMap* GetGlobalEnvironmentMap() const { return GlobalEnvironmentMap; }

    /** Per-triangle raycast */
    bool Raycast(SWorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter = nullptr) const;

    /** Per-bounds raycast */
    bool RaycastBounds(TPodVector<SBoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter = nullptr) const;

    /** Per-triangle raycast */
    bool RaycastClosest(SWorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter = nullptr) const;

    /** Per-bounds raycast */
    bool RaycastClosestBounds(SBoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter = nullptr) const;

    /** Trace collision bodies */
    bool Trace(TPodVector<SCollisionTraceResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.Trace(Result, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceClosest(SCollisionTraceResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceClosest(Result, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceSphere(SCollisionTraceResult& Result, float Radius, Float3 const& RayStart, Float3 const& RayEnd, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceSphere(Result, Radius, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceBox(SCollisionTraceResult& Result, Float3 const& Mins, Float3 const& Maxs, Float3 const& RayStart, Float3 const& RayEnd, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceBox(Result, Mins, Maxs, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceBox2(TPodVector<SCollisionTraceResult>& Result, Float3 const& Mins, Float3 const& Maxs, Float3 const& RayStart, Float3 const& RayEnd, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceBox2(Result, Mins, Maxs, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceCylinder(SCollisionTraceResult& Result, Float3 const& Mins, Float3 const& Maxs, Float3 const& RayStart, Float3 const& RayEnd, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceCylinder(Result, Mins, Maxs, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceCapsule(SCollisionTraceResult& Result, float CapsuleHeight, float CapsuleRadius, Float3 const& RayStart, Float3 const& RayEnd, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceCapsule(Result, CapsuleHeight, CapsuleRadius, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceConvex(SCollisionTraceResult& Result, SConvexSweepTest const& SweepTest) const
    {
        return PhysicsSystem.TraceConvex(Result, SweepTest);
    }

    /** Query objects in sphere */
    void QueryHitProxies(TPodVector<AHitProxy*>& Result, Float3 const& Position, float Radius, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryHitProxies_Sphere(Result, Position, Radius, QueryFilter);
    }

    /** Query objects in box */
    void QueryHitProxies(TPodVector<AHitProxy*>& Result, Float3 const& Position, Float3 const& HalfExtents, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryHitProxies_Box(Result, Position, HalfExtents, QueryFilter);
    }

    /** Query objects in AABB */
    void QueryHitProxies(TPodVector<AHitProxy*>& Result, BvAxisAlignedBox const& BoundingBox, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryHitProxies(Result, BoundingBox, QueryFilter);
    }

    /** Query actors in sphere */
    void QueryActors(TPodVector<AActor*>& Result, Float3 const& Position, float Radius, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryActors_Sphere(Result, Position, Radius, QueryFilter);
    }

    /** Query actors in box */
    void QueryActors(TPodVector<AActor*>& Result, Float3 const& Position, Float3 const& HalfExtents, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryActors_Box(Result, Position, HalfExtents, QueryFilter);
    }

    /** Query actors in AABB */
    void QueryActors(TPodVector<AActor*>& Result, BvAxisAlignedBox const& BoundingBox, SCollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryActors(Result, BoundingBox, QueryFilter);
    }

    /** Query collisions with sphere */
    void QueryCollision_Sphere(TPodVector<SCollisionQueryResult>& Result, Float3 const& Position, float Radius, SCollisionQueryFilter const* QueryFilter) const
    {
        PhysicsSystem.QueryCollision_Sphere(Result, Position, Radius, QueryFilter);
    }

    /** Query collisions with box */
    void QueryCollision_Box(TPodVector<SCollisionQueryResult>& Result, Float3 const& Position, Float3 const& HalfExtents, SCollisionQueryFilter const* QueryFilter) const
    {
        PhysicsSystem.QueryCollision_Box(Result, Position, HalfExtents, QueryFilter);
    }

    /** Query collisions with AABB */
    void QueryCollision(TPodVector<SCollisionQueryResult>& Result, BvAxisAlignedBox const& BoundingBox, SCollisionQueryFilter const* QueryFilter) const
    {
        PhysicsSystem.QueryCollision(Result, BoundingBox, QueryFilter);
    }

    /** Query visible primitives */
    void QueryVisiblePrimitives(TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& InQuery);

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& Areas);

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TPodVector<SVisArea*>& Areas);

    /** Apply amount of damage in specified radius */
    void ApplyRadialDamage(float DamageAmount, Float3 const& Position, float Radius, SCollisionQueryFilter const* QueryFilter = nullptr);

    //
    // Internal
    //

    AScriptEngine* GetScriptEngine() { return ScriptEngine.get(); }

    void DrawDebug(ADebugRenderer* InRenderer);

    AWorld();
    ~AWorld();

    /** Same as Actor->Destroy() */
    static void DestroyActor(AActor* Actor);

    /** Same as Component->Destroy() */
    static void DestroyComponent(AActorComponent* Component);

    void RegisterTimer(ATimer* Timer);

    void UnregisterTimer(ATimer* Timer);

protected:
    void Tick(float TimeStep);

private:
    AActor*          PendingSpawnActors{};
    AActor*          PendingKillActors{};
    AActorComponent* PendingKillComponents{};

private:
    struct SActorSpawnPrivate
    {
        AClassMeta const* ActorClass{};
        AActorDefinition* ActorDef{};
        AString           ScriptModule;
        AActor const*     Template{};
        AActor*           Instigator{};
        ALevel*           Level{};
        bool              bInEditor{};
    };

    AActor* _SpawnActor2(SActorSpawnPrivate& SpawnInfo, STransform const& SpawnTransform = {});

    void BroadcastActorSpawned(AActor* _SpawnedActor);

    asIScriptObject* CreateScriptModule(AString const& Module, AActor* Actor);

    void UpdatePauseStatus();
    void UpdateTimers(float TimeStep);
    void SpawnActors();
    void UpdateActors(float TimeStep);
    void UpdateActorsPrePhysics(float TimeStep);
    void UpdateActorsPostPhysics(float TimeStep);
    void UpdateLevels(float TimeStep);
    void UpdatePhysics(float TimeStep);
    void LateUpdate(float TimeStep);

    void HandlePrePhysics(float TimeStep);
    void HandlePostPhysics(float TimeStep);

    void InitializeAndPlay(AActor* Actor);
    void CleanupActor(AActor* Actor);

    void KillActors(bool bClearSpawnQueue = false);

    TPodVector<AActor*>          Actors;
    TPodVector<AActor*>          TickingActors;
    TPodVector<AActor*>          PrePhysicsTickActors;
    TPodVector<AActor*>          PostPhysicsTickActors;
    TPodVector<AActor*>          LateUpdateActors;
    TPodVector<AActorComponent*> TickingComponents;

    bool bPauseRequest       = false;
    bool bUnpauseRequest     = false;
    bool bPaused             = false;
    bool bResetGameplayTimer = false;

    // Game virtual time based on variable frame step
    int64_t GameRunningTimeMicro          = 0;
    int64_t GameRunningTimeMicroAfterTick = 0;

    // Gameplay virtual time based on fixed frame step, running when unpaused
    int64_t GameplayTimeMicro          = 0;
    int64_t GameplayTimeMicroAfterTick = 0;

    ATimer* TimerList         = nullptr;
    ATimer* TimerListTail     = nullptr;
    ATimer* pNextTickingTimer = nullptr;

    bool bPendingKill = false;
    bool bTicking     = false;

    AWorld*        NextPendingKillWorld = nullptr;
    static AWorld* PendingKillWorlds;

    // All existing worlds
    static TPodVector<AWorld*> Worlds;
    // The worlds that are ticking
    static TPodVector<AWorld*> TickingWorlds;

    TRef<ALevel>        PersistentLevel;
    TPodVector<ALevel*> ArrayOfLevels;

    /** Scale audio volume in the entire world */
    float AudioVolume = 1.0f;

    TRef<AEnvironmentMap> GlobalEnvironmentMap;

    // Script
    std::unique_ptr<AScriptEngine> ScriptEngine;
};

#include "Actor.h"
#include "ActorComponent.h"

/*

TActorIterator

Iterate world actors

Example:
for ( TActorIterator< AMyActor > it( GetWorld() ) ; it ; ++it ) {
    AMyActor * actor = *it;
    ...
}

*/
template <typename T>
struct TActorIterator
{
    explicit TActorIterator(AWorld* _World) :
        Actors(_World->GetActors()), i(0)
    {
        Next();
    }

    operator bool() const
    {
        return Actor != nullptr;
    }

    T* operator++()
    {
        Next();
        return Actor;
    }

    T* operator++(int)
    {
        T* a = Actor;
        Next();
        return a;
    }

    T* operator*() const
    {
        return Actor;
    }

    T* operator->() const
    {
        return Actor;
    }

    void Next()
    {
        AActor* a;
        while (i < Actors.Size())
        {
            a = Actors[i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::ClassMeta())
            {
                Actor = static_cast<T*>(a);
                return;
            }
        }
        Actor = nullptr;
    }

private:
    TPodVector<AActor*> const& Actors;
    T*                         Actor;
    int                        i;
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
template <typename T>
struct TActorIterator2
{
    explicit TActorIterator2(AWorld* _World) :
        Actors(_World->GetActors()), i(0)
    {
    }

    T* First()
    {
        i = 0;
        return Next();
    }

    T* Next()
    {
        AActor* a;
        while (i < Actors.Size())
        {
            a = Actors[i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::ClassMeta())
            {
                return static_cast<T*>(a);
            }
        }
        return nullptr;
    }

private:
    TPodVector<AActor*> const& Actors;
    int                        i;
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
template <typename T>
struct TComponentIterator
{
    explicit TComponentIterator(AActor* _Actor) :
        Components(_Actor->GetComponents()), i(0)
    {
        Next();
    }

    operator bool() const
    {
        return Component != nullptr;
    }

    T* operator++()
    {
        Next();
        return Component;
    }

    T* operator++(int)
    {
        T* a = Component;
        Next();
        return a;
    }

    T* operator*() const
    {
        return Component;
    }

    T* operator->() const
    {
        return Component;
    }

    void Next()
    {
        AActorComponent* a;
        while (i < Components.Size())
        {
            a = Components[i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::ClassMeta())
            {
                Component = static_cast<T*>(a);
                return;
            }
        }
        Component = nullptr;
    }

private:
    AArrayOfActorComponents const& Components;
    T*                             Component;
    int                            i;
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
template <typename T>
struct TComponentIterator2
{
    explicit TComponentIterator2(AActor* _Actor) :
        Components(_Actor->GetComponents()), i(0)
    {
    }

    T* First()
    {
        i = 0;
        return Next();
    }

    T* Next()
    {
        AActorComponent* a;
        while (i < Components.Size())
        {
            a = Components[i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::ClassMeta())
            {
                return static_cast<T*>(a);
            }
        }
        return nullptr;
    }

private:
    TPodVector<AActorComponent*> const& Components;
    int                                 i;
};
