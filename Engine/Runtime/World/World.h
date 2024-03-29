/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Runtime/PhysicsSystem.h>
#include <Engine/Runtime/AINavigationMesh.h>
#include <Engine/Runtime/SkinningSystem.h>
#include <Engine/Runtime/LightingSystem.h>
#include <Engine/Runtime/Level.h>
#include <Engine/Runtime/ScriptEngine.h>

#include <Engine/Core/Platform/Platform.h>

HK_NAMESPACE_BEGIN

class Actor;
class ActorComponent;
class CameraComponent;
class WorldTimer;
class DebugRenderer;
class GameModule;
class EnvironmentMap;

/** Actor spawn parameters */
struct ActorSpawnInfo
{
    /** Initial actor transform */
    Transform SpawnTransform;

    /** Level for actor spawn */
    Level* pLevel;

    /** Who spawn the actor */
    Actor* Instigator;

    /** Actor spawned for editing */
    bool bInEditor;

    ActorSpawnInfo() = delete;

    ActorSpawnInfo(ClassMeta const* _ActorTypeClassMeta) :
        pLevel(nullptr), Instigator(nullptr), bInEditor(false), m_Template(nullptr), m_ActorTypeClassMeta(_ActorTypeClassMeta)
    {
    }

    ActorSpawnInfo(uint64_t _ActorClassId);

    ActorSpawnInfo(const char* _ActorClassName);

    /** Set actor template */
    void SetTemplate(Actor const* _Template);

    /** Get actor template */
    Actor const* GetTemplate() const { return m_Template; }

    /** Get actor meta class */
    ClassMeta const* ActorClassMeta() const { return m_ActorTypeClassMeta; }

protected:
    /** NOTE: template type meta must match ActorTypeClassMeta */
    Actor const* m_Template;

    /** Actor type */
    ClassMeta const* m_ActorTypeClassMeta;
};

/** Template helper for actor spawn parameters */
template <typename ActorType>
struct TActorSpawnInfo : ActorSpawnInfo
{
    TActorSpawnInfo() :
        ActorSpawnInfo(&ActorType::GetClassMeta())
    {
    }
};

/** World. Defines a game map or editor/tool scene */
class World : public GCObject
{
public:
    /** Delegate to notify when any actor spawned */
    using OnActorSpawned = TEvent<Actor*>;
    OnActorSpawned E_OnActorSpawned;

    /** Called on each tick after physics simulation */
    using OnPostPhysicsUpdate = TEvent<float>;
    OnPostPhysicsUpdate E_OnPostPhysicsUpdate;

    /** Delegate to prepare for rendering */
    using OnPrepareRenderFrontend = TEvent<CameraComponent*, int>;
    OnPrepareRenderFrontend E_OnPrepareRenderFrontend;

    Hk::VisibilitySystem VisibilitySystem;

    Hk::PhysicsSystem PhysicsSystem;

    Hk::AINavigationMesh NavigationMesh;

    Hk::SkinningSystem SkinningSystem;

    Hk::LightingSystem LightingSystem;

    /** Create a new world */
    static World* CreateWorld();

    /** Destroy all existing worlds */
    static void DestroyWorlds();

    /** Get array of worlds */
    static TVector<World*> const& GetWorlds() { return m_Worlds; }

    /** Tick the worlds */
    static void UpdateWorlds(float TimeStep);

    /** Remove worlds, marked pending kill */
    static void KillWorlds();

    /** Bake AI nav mesh */
    void BuildNavigation(AINavigationConfig const& navigationConfig);

    /** DEPRECATED!!!! Spawn a new actor */
    Actor* SpawnActor(ActorSpawnInfo const& spawnInfo);

    /** DEPRECATED!!!! Spawn a new actor */
    template <typename ActorType>
    ActorType* SpawnActor(TActorSpawnInfo<ActorType> const& spawnInfo)
    {
        return static_cast<ActorType*>(SpawnActor(static_cast<ActorSpawnInfo const&>(spawnInfo)));
    }

    /** Spawn empty actor */
    Actor* SpawnActor(Transform const& SpawnTransform = {}, Actor* Instigator = {}, Level* Level = {}, bool bInEditor = {});

    /** Spawn actor with definition */
    Actor* SpawnActor(class ActorDefinition* ActorDef, Transform const& SpawnTransform = {}, Actor* Instigator = {}, Level* Level = {}, bool bInEditor = {});

    /** Spawn actor with script module */
    Actor* SpawnActor(String const& ScriptModule, Transform const& SpawnTransform = {}, Actor* Instigator = {}, Level* Level = {}, bool bInEditor = {});

    /** Spawn actor with C++ module */
    Actor* SpawnActor(ClassMeta const* ActorClass, Transform const& SpawnTransform = {}, Actor* Instigator = {}, Level* Level = {}, bool bInEditor = {});

    /** Spawn actor with C++ module */
    template <typename T>
    T* SpawnActor(Transform const& SpawnTransform = {}, Actor* Instigator = {}, Level* Level = {}, bool bInEditor = {})
    {
        if (T::GetClassMeta().Factory() != &Actor::Factory())
            CriticalError("World::SpawnActor: not an actor class\n");

        return static_cast<T*>(SpawnActor(&T::GetClassMeta(), SpawnTransform, Instigator, Level, bInEditor));
    }

    /** Clone actor */
    Actor* SpawnActor(Actor const* Template, Transform const& SpawnTransform = {}, Actor* Instigator = {}, Level* Level = {}, bool bInEditor = {});

    /** Get all actors in the world */
    TVector<Actor*> const& GetActors() const { return m_Actors; }

    /** Destroy this world */
    void Destroy();

    /** Destroy all actors in the world */
    void DestroyActors();

    /** Add level to the world */
    void AddLevel(Level* level);

    /** Remove level from the world */
    void RemoveLevel(Level* level);

    /** Get world's persistent level */
    Level* GetPersistentLevel() { return m_PersistentLevel; }

    /** Get all levels in the world */
    TVector<Level*> const& GetArrayOfLevels() const { return m_ArrayOfLevels; }

    /** Pause the game. Freezes world and actor ticking since the next game tick. */
    void SetPaused(bool _Paused);

    /** Returns current pause state */
    bool IsPaused() const;

    /** Game virtual time based on variable frame step */
    int64_t GetRunningTimeMicro() const { return m_GameRunningTimeMicro; }

    /** Gameplay virtual time based on fixed frame step, running when unpaused */
    int64_t GetGameplayTimeMicro() const { return m_GameplayTimeMicro; }

    /** Reset gameplay timer to zero. This is delayed operation. */
    void ResetGameplayTimer();

    /** Physics simulation refresh rate */
    void SetPhysicsHertz(int Hertz);

    /** Contact solver split impulse. Disabled by default for performance */
    void SetContactSolverSplitImpulse(bool SplitImpulse);

    /** Contact solver iterations count */
    void SetContactSolverIterations(int InterationsCount);

    /** Set world gravity vector */
    void SetGravityVector(Float3 const& Gravity);

    /** Get world gravity vector */
    Float3 const& GetGravityVector() const;

    /** Is in physics update now */
    bool IsDuringPhysicsUpdate() const { return PhysicsSystem.bDuringPhysicsUpdate; }

    /** Is world destroyed, but not removed yet. */
    bool IsPendingKill() const { return m_bPendingKill; }

    /** Scale audio volume in the entire world */
    void SetAudioVolume(float Volume)
    {
        m_AudioVolume = Math::Saturate(Volume);
    }

    /** Scale audio volume in the entire world */
    float GetAudioVolume() const { return m_AudioVolume; }

    void SetGlobalEnvironmentMap(EnvironmentMap* EnvironmentMap);
    EnvironmentMap* GetGlobalEnvironmentMap() const { return m_GlobalEnvironmentMap; }

    /** Per-triangle raycast */
    bool Raycast(WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter = nullptr) const;

    /** Per-bounds raycast */
    bool RaycastBounds(TVector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter = nullptr) const;

    /** Per-triangle raycast */
    bool RaycastClosest(WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter = nullptr) const;

    /** Per-bounds raycast */
    bool RaycastClosestBounds(BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter = nullptr) const;

    /** Trace collision bodies */
    bool Trace(TVector<CollisionTraceResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.Trace(Result, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceClosest(CollisionTraceResult& Result, Float3 const& RayStart, Float3 const& RayEnd, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceClosest(Result, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceSphere(CollisionTraceResult& Result, float Radius, Float3 const& RayStart, Float3 const& RayEnd, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceSphere(Result, Radius, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceBox(CollisionTraceResult& Result, Float3 const& Mins, Float3 const& Maxs, Float3 const& RayStart, Float3 const& RayEnd, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceBox(Result, Mins, Maxs, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceBox2(TVector<CollisionTraceResult>& Result, Float3 const& Mins, Float3 const& Maxs, Float3 const& RayStart, Float3 const& RayEnd, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceBox2(Result, Mins, Maxs, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceCylinder(CollisionTraceResult& Result, Float3 const& Mins, Float3 const& Maxs, Float3 const& RayStart, Float3 const& RayEnd, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceCylinder(Result, Mins, Maxs, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceCapsule(CollisionTraceResult& Result, float CapsuleHeight, float CapsuleRadius, Float3 const& RayStart, Float3 const& RayEnd, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        return PhysicsSystem.TraceCapsule(Result, CapsuleHeight, CapsuleRadius, RayStart, RayEnd, QueryFilter);
    }

    /** Trace collision bodies */
    bool TraceConvex(CollisionTraceResult& Result, ConvexSweepTest const& SweepTest) const
    {
        return PhysicsSystem.TraceConvex(Result, SweepTest);
    }

    /** Query objects in sphere */
    void QueryHitProxies(TVector<HitProxy*>& Result, Float3 const& Position, float Radius, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryHitProxies_Sphere(Result, Position, Radius, QueryFilter);
    }

    /** Query objects in box */
    void QueryHitProxies(TVector<HitProxy*>& Result, Float3 const& Position, Float3 const& HalfExtents, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryHitProxies_Box(Result, Position, HalfExtents, QueryFilter);
    }

    /** Query objects in AABB */
    void QueryHitProxies(TVector<HitProxy*>& Result, BvAxisAlignedBox const& BoundingBox, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryHitProxies(Result, BoundingBox, QueryFilter);
    }

    /** Query actors in sphere */
    void QueryActors(TVector<Actor*>& Result, Float3 const& Position, float Radius, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryActors_Sphere(Result, Position, Radius, QueryFilter);
    }

    /** Query actors in box */
    void QueryActors(TVector<Actor*>& Result, Float3 const& Position, Float3 const& HalfExtents, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryActors_Box(Result, Position, HalfExtents, QueryFilter);
    }

    /** Query actors in AABB */
    void QueryActors(TVector<Actor*>& Result, BvAxisAlignedBox const& BoundingBox, CollisionQueryFilter const* QueryFilter = nullptr) const
    {
        PhysicsSystem.QueryActors(Result, BoundingBox, QueryFilter);
    }

    /** Query collisions with sphere */
    void QueryCollision_Sphere(TVector<CollisionQueryResult>& Result, Float3 const& Position, float Radius, CollisionQueryFilter const* QueryFilter) const
    {
        PhysicsSystem.QueryCollision_Sphere(Result, Position, Radius, QueryFilter);
    }

    /** Query collisions with box */
    void QueryCollision_Box(TVector<CollisionQueryResult>& Result, Float3 const& Position, Float3 const& HalfExtents, CollisionQueryFilter const* QueryFilter) const
    {
        PhysicsSystem.QueryCollision_Box(Result, Position, HalfExtents, QueryFilter);
    }

    /** Query collisions with AABB */
    void QueryCollision(TVector<CollisionQueryResult>& Result, BvAxisAlignedBox const& BoundingBox, CollisionQueryFilter const* QueryFilter) const
    {
        PhysicsSystem.QueryCollision(Result, BoundingBox, QueryFilter);
    }

    /** Query visible primitives */
    void QueryVisiblePrimitives(TVector<PrimitiveDef*>& VisPrimitives, TVector<SurfaceDef*>& VisSurfs, int* VisPass, VisibilityQuery const& InQuery);

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TVector<VisArea*>& Areas);

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& Bounds, TVector<VisArea*>& Areas);

    /** Apply amount of damage in specified radius */
    void ApplyRadialDamage(float DamageAmount, Float3 const& Position, float Radius, CollisionQueryFilter const* QueryFilter = nullptr);

    //
    // Internal
    //

    ScriptEngine* GetScriptEngine() { return m_ScriptEngine.get(); }

    void DrawDebug(DebugRenderer* renderer);

    /** Same as Actor->Destroy() */
    static void DestroyActor(Actor* actor);

    /** Same as Component->Destroy() */
    static void DestroyComponent(ActorComponent* component);

    void RegisterTimer(WorldTimer* timer);

    void UnregisterTimer(WorldTimer* timer);

protected:
    void Tick(float TimeStep);

private:
    Actor* m_PendingSpawnActors{};
    Actor* m_PendingKillActors{};
    ActorComponent* m_PendingKillComponents{};

private:
    struct ActorSpawnPrivate
    {
        ClassMeta const* ActorClass{};
        ActorDefinition* ActorDef{};
        String ScriptModule;
        Actor const* Template{};
        Actor* Instigator{};
        Level* pLevel{};
        bool bInEditor{};
    };

    World();
    ~World();

    Actor* _SpawnActor(ActorSpawnPrivate& SpawnInfo, Transform const& SpawnTransform = {});

    void BroadcastActorSpawned(Actor* SpawnedActor);

    asIScriptObject* CreateScriptModule(String const& Module, Actor* Actor);

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

    void InitializeAndPlay(Actor* Actor);
    void CleanupActor(Actor* Actor);

    void KillActors(bool bClearSpawnQueue = false);

    TVector<Actor*> m_Actors;
    TVector<Actor*> m_TickingActors;
    TVector<Actor*> m_PrePhysicsTickActors;
    TVector<Actor*> m_PostPhysicsTickActors;
    TVector<Actor*> m_LateUpdateActors;
    TVector<ActorComponent*> m_TickingComponents;

    // Just a buffer to no reallocate the vector
    TVector<Actor*> m_DamagedActors;

    bool m_bPauseRequest = false;
    bool m_bUnpauseRequest = false;
    bool m_bPaused = false;
    bool m_bResetGameplayTimer = false;

    // Game virtual time based on variable frame step
    int64_t m_GameRunningTimeMicro = 0;
    int64_t m_GameRunningTimeMicroAfterTick = 0;

    // Gameplay virtual time based on fixed frame step, running when unpaused
    int64_t m_GameplayTimeMicro = 0;
    int64_t m_GameplayTimeMicroAfterTick = 0;

    WorldTimer* m_TimerList = nullptr;
    WorldTimer* m_TimerListTail = nullptr;
    WorldTimer* m_pNextTickingTimer = nullptr;

    bool m_bPendingKill = false;
    bool m_bTicking = false;

    World* m_NextPendingKillWorld = nullptr;
    static World* m_PendingKillWorlds;

    // All existing worlds
    static TVector<World*> m_Worlds;
    // The worlds that are ticking
    static TVector<World*> m_TickingWorlds;

    TRef<Level> m_PersistentLevel;
    TVector<Level*> m_ArrayOfLevels;

    /** Scale audio volume in the entire world */
    float m_AudioVolume = 1.0f;

    TRef<EnvironmentMap> m_GlobalEnvironmentMap;

    // Script
    std::unique_ptr<ScriptEngine> m_ScriptEngine;
};

HK_NAMESPACE_END

#include "Actor.h"
#include "ActorComponent.h"

HK_NAMESPACE_BEGIN

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
    explicit TActorIterator(World* world) :
        m_Actors(world->GetActors()), m_i(0)
    {
        Next();
    }

    operator bool() const
    {
        return m_Actor != nullptr;
    }

    T* operator++()
    {
        Next();
        return m_Actor;
    }

    T* operator++(int)
    {
        T* a = m_Actor;
        Next();
        return a;
    }

    T* operator*() const
    {
        return m_Actor;
    }

    T* operator->() const
    {
        return m_Actor;
    }

    void Next()
    {
        Actor* a;
        while (m_i < m_Actors.Size())
        {
            a = m_Actors[m_i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::GetClassMeta())
            {
                m_Actor = static_cast<T*>(a);
                return;
            }
        }
        m_Actor = nullptr;
    }

private:
    TVector<Actor*> const& m_Actors;
    T* m_Actor;
    int m_i;
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
    explicit TActorIterator2(World* world) :
        m_Actors(world->GetActors()), m_i(0)
    {}

    T* First()
    {
        m_i = 0;
        return Next();
    }

    T* Next()
    {
        Actor* a;
        while (m_i < m_Actors.Size())
        {
            a = m_Actors[m_i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::GetClassMeta())
            {
                return static_cast<T*>(a);
            }
        }
        return nullptr;
    }

private:
    TVector<Actor*> const& m_Actors;
    int m_i;
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
    explicit TComponentIterator(Actor* actor) :
        m_Components(actor->GetComponents()), m_i(0)
    {
        Next();
    }

    operator bool() const
    {
        return m_Component != nullptr;
    }

    T* operator++()
    {
        Next();
        return m_Component;
    }

    T* operator++(int)
    {
        T* a = m_Component;
        Next();
        return a;
    }

    T* operator*() const
    {
        return m_Component;
    }

    T* operator->() const
    {
        return m_Component;
    }

    void Next()
    {
        ActorComponent* a;
        while (m_i < m_Components.Size())
        {
            a = m_Components[m_i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::GetClassMeta())
            {
                m_Component = static_cast<T*>(a);
                return;
            }
        }
        m_Component = nullptr;
    }

private:
    ActorComponents const& m_Components;
    T* m_Component;
    int m_i;
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
    explicit TComponentIterator2(Actor* actor) :
        m_Components(actor->GetComponents()), m_i(0)
    {}

    T* First()
    {
        m_i = 0;
        return Next();
    }

    T* Next()
    {
        ActorComponent* a;
        while (m_i < m_Components.Size())
        {
            a = m_Components[m_i++];
            if (a->IsPendingKill())
            {
                continue;
            }
            if (&a->FinalClassMeta() == &T::GetClassMeta())
            {
                return static_cast<T*>(a);
            }
        }
        return nullptr;
    }

private:
    ActorComponents const& m_Components;
    int m_i;
};

HK_NAMESPACE_END
