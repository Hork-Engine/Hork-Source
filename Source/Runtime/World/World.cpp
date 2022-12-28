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

#include "World.h"
#include "Timer.h"
#include "SkinnedComponent.h"
#include <Runtime/Engine.h>
#include <Runtime/EnvironmentMap.h>

#include <Platform/Logger.h>
#include <Platform/Profiler.h>
#include <Core/IntrusiveLinkedListMacro.h>

#include <angelscript.h>

HK_NAMESPACE_BEGIN

World* World::m_PendingKillWorlds = nullptr;
TVector<World*> World::m_Worlds;
TVector<World*> World::m_TickingWorlds;

World::World()
{
    m_PersistentLevel = NewObj<Level>();
    m_PersistentLevel->AddRef();
    m_PersistentLevel->m_OwnerWorld = this;
    m_PersistentLevel->m_bIsPersistent = true;
    m_PersistentLevel->OnAddLevelToWorld();

    VisibilitySystemCreateInfo ci{};
    m_PersistentLevel->Visibility = MakeRef<VisibilityLevel>(ci);
    m_ArrayOfLevels.Add(m_PersistentLevel);

    VisibilitySystem.RegisterLevel(m_PersistentLevel->Visibility);

    PhysicsSystem.PrePhysicsCallback.Set(this, &World::HandlePrePhysics);
    PhysicsSystem.PostPhysicsCallback.Set(this, &World::HandlePostPhysics);
}

World::~World()
{
    for (Level* level : m_ArrayOfLevels)
    {
        level->OnRemoveLevelFromWorld();

        m_ArrayOfLevels.Remove(m_ArrayOfLevels.IndexOf(level));

        VisibilitySystem.UnregisterLevel(level->Visibility);

        level->m_OwnerWorld = nullptr;
        level->RemoveRef();
    }

    //VisibilitySystem.UnregisterLevel(PersistentLevel->Visibility);
}

void World::SetPaused(bool _Paused)
{
    m_bPauseRequest = _Paused;
    m_bUnpauseRequest = !_Paused;
}

bool World::IsPaused() const
{
    return m_bPaused;
}

void World::ResetGameplayTimer()
{
    m_bResetGameplayTimer = true;
}

void World::SetPhysicsHertz(int _Hertz)
{
    PhysicsSystem.PhysicsHertz = _Hertz;
}

void World::SetContactSolverSplitImpulse(bool _SplitImpulse)
{
    PhysicsSystem.bContactSolverSplitImpulse = _SplitImpulse;
}

void World::SetContactSolverIterations(int _InterationsCount)
{
    PhysicsSystem.NumContactSolverIterations = _InterationsCount;
}

void World::SetGravityVector(Float3 const& _Gravity)
{
    PhysicsSystem.GravityVector = _Gravity;
    PhysicsSystem.bGravityDirty = true;
}

Float3 const& World::GetGravityVector() const
{
    return PhysicsSystem.GravityVector;
}

void World::Destroy()
{
    if (m_bPendingKill)
    {
        return;
    }

    // Mark world to remove it from the game
    m_bPendingKill = true;
    m_NextPendingKillWorld = m_PendingKillWorlds;
    m_PendingKillWorlds = this;

    DestroyActors();
}

void World::DestroyActors()
{
    for (Actor* actor : m_Actors)
    {
        DestroyActor(actor);
    }

    // Destroy actors from spawn queue
    Actor* actor = m_PendingSpawnActors;
    m_PendingSpawnActors = nullptr;
    while (actor)
    {
        Actor* nextActor = actor->m_NextSpawnActor;
        DestroyActor(actor);
        actor = nextActor;
    }
}

void World::DestroyActor(Actor* actor)
{
    if (actor->m_bPendingKill)
    {
        return;
    }

    World* world = actor->m_World;

    HK_ASSERT(world);

    // Mark actor to remove it from the world
    actor->m_bPendingKill = true;
    actor->m_NextPendingKillActor = world->m_PendingKillActors;
    world->m_PendingKillActors = actor;

    for (ActorComponent* component : actor->m_Components)
    {
        DestroyComponent(component);
    }

    if (!actor->m_bSpawning)
    {
        Level* level = actor->m_Level;

        level->m_Actors[actor->m_IndexInLevelArrayOfActors] = level->m_Actors[level->m_Actors.Size() - 1];
        level->m_Actors[actor->m_IndexInLevelArrayOfActors]->m_IndexInLevelArrayOfActors = actor->m_IndexInLevelArrayOfActors;
        level->m_Actors.RemoveLast();
        actor->m_IndexInLevelArrayOfActors = -1;
    }
    else
    {
        LOG("Destroyed before spawn\n");
    }
}

void World::DestroyComponent(ActorComponent* component)
{
    if (component->m_bPendingKill)
    {
        return;
    }

    World* world = component->GetWorld();

    HK_ASSERT(world);

    // Mark component pending kill
    component->m_bPendingKill = true;

    // Add component to pending kill list
    component->m_NextPendingKillComponent = world->m_PendingKillComponents;
    world->m_PendingKillComponents = component;
}

void World::BuildNavigation(AINavigationConfig const& navigationConfig)
{
    NavigationMesh.Initialize(navigationConfig);
    NavigationMesh.Build();
}

ActorSpawnInfo::ActorSpawnInfo(uint64_t _ActorClassId) :
    ActorSpawnInfo(Actor::Factory().LookupClass(_ActorClassId))
{
}

ActorSpawnInfo::ActorSpawnInfo(const char* _ActorClassName) :
    ActorSpawnInfo(Actor::Factory().LookupClass(_ActorClassName))
{
}

void ActorSpawnInfo::SetTemplate(Actor const* _Template)
{
    HK_ASSERT(&_Template->FinalClassMeta() == m_ActorTypeClassMeta);
    m_Template = _Template;
}

asIScriptObject* World::CreateScriptModule(String const& Module, Actor* Actor)
{
    if (!m_ScriptEngine)
        m_ScriptEngine = std::make_unique<ScriptEngine>(this);

    asIScriptObject* scriptModule = m_ScriptEngine->CreateScriptInstance(Module, Actor);

    if (scriptModule)
    {
        ActorScript* script = ActorScript::GetScript(scriptModule);

        asUINT numProps = scriptModule->GetPropertyCount();
        for (asUINT i = 0; i < numProps; i++)
        {
            if (!Platform::Strcmp(scriptModule->GetPropertyName(i), "bTickEvenWhenPaused"))
            {
                if (scriptModule->GetPropertyTypeId(i) != asTYPEID_BOOL)
                {
                    LOG("WARNING: Expected type id 'bool' for bTickEvenWhenPaused\n");
                    break;
                }

                Actor->m_bTickEvenWhenPaused = *(const bool*)scriptModule->GetAddressOfProperty(i);
                break;
            }
        }

        Actor->m_bCanEverTick = Actor->m_bCanEverTick || script->M_Tick != nullptr;
        Actor->m_bTickPrePhysics = Actor->m_bTickPrePhysics || script->M_TickPrePhysics != nullptr;
        Actor->m_bTickPostPhysics = Actor->m_bTickPostPhysics || script->M_TickPostPhysics != nullptr;
        Actor->m_bLateUpdate = Actor->m_bLateUpdate || script->M_LateUpdate != nullptr;
    }

    return scriptModule;
}

Actor* World::_SpawnActor2(ActorSpawnPrivate& SpawnInfo, Transform const& SpawnTransform)
{
    if (m_bPendingKill)
    {
        LOG("World::SpawnActor: Attempting to spawn an actor from a destroyed world\n");
        return nullptr;
    }

    ClassMeta const* actorClass = SpawnInfo.ActorClass;
    HK_ASSERT(actorClass);

    ActorDefinition* pActorDef = SpawnInfo.ActorDef;

    // Override actor class
    if (pActorDef && pActorDef->GetActorClass())
    {
        actorClass = pActorDef->GetActorClass();

        if (actorClass->Factory() != &Actor::Factory())
        {
            LOG("World::SpawnActor: wrong C++ actor class specified\n");
            actorClass = &Actor::GetClassMeta();
        }
    }

    Actor* actor = static_cast<Actor*>(actorClass->CreateInstance());
    actor->AddRef();
    actor->m_bInEditor = SpawnInfo.bInEditor;
    actor->m_pActorDef = pActorDef;

    // Create components from actor definition
    if (pActorDef)
    {
        TPodVector<ActorComponent*> components;

        // Create components and set properties
        int componentIndex = 0;
        for (auto& componentDef : pActorDef->GetComponents())
        {
            auto* component = actor->CreateComponent(componentDef.ClassMeta, componentDef.Name);
            if (component)
            {
                component->SetProperties(componentDef.PropertyHash);

                if (pActorDef->GetRootIndex() == componentIndex)
                {
                    HK_ASSERT(component->FinalClassMeta().IsSubclassOf<SceneComponent>());
                    actor->m_RootComponent = static_cast<SceneComponent*>(component);
                }
            }
            componentIndex++;
            components.Add(component);
        }

        // Attach components
        componentIndex = 0;
        for (auto& componentDef : pActorDef->GetComponents())
        {
            if (componentDef.ParentIndex != -1)
            {
                if (components[componentIndex] && components[componentDef.ParentIndex])
                {
                    HK_ASSERT(components[componentIndex]->FinalClassMeta().IsSubclassOf<SceneComponent>());
                    HK_ASSERT(components[componentDef.ParentIndex]->FinalClassMeta().IsSubclassOf<SceneComponent>());

                    SceneComponent* sceneComponent = static_cast<SceneComponent*>(components[componentIndex]);
                    SceneComponent* parentComponent = static_cast<SceneComponent*>(components[componentDef.ParentIndex]);

                    // TODO: Socket!!!
                    sceneComponent->AttachTo(parentComponent);
                }
            }

            componentIndex++;
        }
    }

    // Initialize actor
    ActorInitializer initializer;
    actor->Initialize(initializer);
    actor->m_bCanEverTick = initializer.bCanEverTick;
    actor->m_bTickEvenWhenPaused = initializer.bTickEvenWhenPaused;
    actor->m_bTickPrePhysics = initializer.bTickPrePhysics;
    actor->m_bTickPostPhysics = initializer.bTickPostPhysics;
    actor->m_bLateUpdate = initializer.bLateUpdate;

    // Set properties for the actor
    if (pActorDef)
        actor->SetProperties(pActorDef->GetActorPropertyHash());

    // Create script
    String const& scriptModule = pActorDef ? pActorDef->GetScriptModule() : SpawnInfo.ScriptModule;
    if (!scriptModule.IsEmpty())
    {
        actor->m_ScriptModule = CreateScriptModule(scriptModule, actor);
        if (actor->m_ScriptModule)
        {
            if (pActorDef)
            {
                ActorScript::SetProperties(actor->m_ScriptModule, pActorDef->GetScriptPropertyHash());
            }
        }
        else
        {
            LOG("WARNING: Unknown script module '{}'\n", scriptModule);
        }
    }

    if (SpawnInfo.Template)
    {
        actor->LifeSpan = SpawnInfo.Template->LifeSpan;

        auto FindTemplateComponent = [](Actor const* Template, ActorComponent* Component) -> ActorComponent*
        {
            auto classId = Component->FinalClassId();
            auto localId = Component->m_LocalId;
            for (ActorComponent* component : Template->GetComponents())
            {
                if (component->FinalClassId() == classId && component->m_LocalId == localId)
                    return component;
            }
            return {};
        };

        // Clone component properties
        //for (ActorComponent* component : actor->GetComponents())
        //{
        //    ActorComponent* templateComponent = FindTemplateComponent(SpawnInfo.Template, component);
        //    if (templateComponent)
        //        ClassMeta::CloneProperties(templateComponent, component);
        //}
        for (ActorComponent* component : SpawnInfo.Template->GetComponents())
        {
            ActorComponent* dst = FindTemplateComponent(actor, component);
            if (!dst)
                dst = actor->CreateComponent(&component->FinalClassMeta(), component->GetObjectName());
            ClassMeta::CloneProperties(component, dst);
        }

        if (actor->m_ScriptModule && SpawnInfo.Template->m_ScriptModule)
        {
            ActorScript::CloneProperties(SpawnInfo.Template->m_ScriptModule, actor->m_ScriptModule);
            // TODO: Clone script properties
        }

        ClassMeta::CloneProperties(SpawnInfo.Template, actor);
    }

    // All components created at spawn time are default.
    for (ActorComponent* component : actor->GetComponents())
        component->m_bIsDefault = true;

    if (SpawnInfo.Instigator)
    {
        actor->m_Instigator = SpawnInfo.Instigator;
        actor->m_Instigator->AddRef();
    }

    actor->m_World = this;
    actor->m_Level = SpawnInfo.Level ? SpawnInfo.Level : m_PersistentLevel;

    if (actor->m_RootComponent)
    {
        actor->m_RootComponent->SetTransform(SpawnTransform);
    }

    actor->m_NextSpawnActor = m_PendingSpawnActors;
    m_PendingSpawnActors = actor;

    return actor;
}

Actor* World::SpawnActor(ActorSpawnInfo const& _SpawnInfo)
{
    ActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass = _SpawnInfo.ActorClassMeta();
    spawnInfo.Template = _SpawnInfo.GetTemplate();
    spawnInfo.Instigator = _SpawnInfo.Instigator;
    spawnInfo.Level = _SpawnInfo.Level;
    spawnInfo.bInEditor = _SpawnInfo.bInEditor;

    if (!spawnInfo.ActorClass)
    {
        LOG("World::SpawnActor: invalid actor class\n");
        return nullptr;
    }

    if (spawnInfo.ActorClass->Factory() != &Actor::Factory())
    {
        LOG("World::SpawnActor: not an actor class\n");
        return nullptr;
    }

    if (spawnInfo.Template && spawnInfo.ActorClass != &spawnInfo.Template->FinalClassMeta())
    {
        LOG("World::SpawnActor: ActorSpawnInfo::Template class doesn't match meta data\n");
        return nullptr;
    }

    return _SpawnActor2(spawnInfo, _SpawnInfo.SpawnTransform);
}

Actor* World::SpawnActor2(Transform const& SpawnTransform, Actor* Instigator, Level* Level, bool bInEditor)
{
    ActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass = &Actor::GetClassMeta();
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level = Level;
    spawnInfo.bInEditor = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

Actor* World::SpawnActor2(ActorDefinition* pActorDef, Transform const& SpawnTransform, Actor* Instigator, Level* Level, bool bInEditor)
{
    if (!pActorDef)
    {
        LOG("World::SpawnActor: invalid actor definition\n");
    }

    ActorSpawnPrivate spawnInfo;
    spawnInfo.ActorDef = pActorDef;
    spawnInfo.ActorClass = &Actor::GetClassMeta();
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level = Level;
    spawnInfo.bInEditor = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

Actor* World::SpawnActor2(String const& ScriptModule, Transform const& SpawnTransform, Actor* Instigator, Level* Level, bool bInEditor)
{
    if (ScriptModule.IsEmpty())
    {
        LOG("World::SpawnActor: invalid script module\n");
    }

    ActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass = &Actor::GetClassMeta();
    spawnInfo.ScriptModule = ScriptModule;
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level = Level;
    spawnInfo.bInEditor = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

Actor* World::SpawnActor2(ClassMeta const* ActorClass, Transform const& SpawnTransform, Actor* Instigator, Level* Level, bool bInEditor)
{
    if (!ActorClass)
    {
        LOG("World::SpawnActor: invalid C++ module class\n");
        ActorClass = &Actor::GetClassMeta();
    }

    ActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass = ActorClass;
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level = Level;
    spawnInfo.bInEditor = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

Actor* World::SpawnActor2(Actor const* Template, Transform const& SpawnTransform, Actor* Instigator, Level* Level, bool bInEditor)
{
    ActorSpawnPrivate spawnInfo;

    if (Template)
    {
        if (Template->m_pActorDef)
        {
            spawnInfo.ActorDef = Template->m_pActorDef;
        }
        else if (Template->m_ScriptModule)
        {
            ActorScript* pScript = ActorScript::GetScript(Template->m_ScriptModule);
            spawnInfo.ScriptModule = pScript->GetModule();
        }
    }
    else
    {
        LOG("World::SpawnActor: invalid template\n");
    }

    spawnInfo.ActorClass = Template ? &Template->FinalClassMeta() : &Actor::GetClassMeta();
    spawnInfo.Template = Template;
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level = Level;
    spawnInfo.bInEditor = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

void World::InitializeAndPlay(Actor* Actor)
{
    if (Actor->m_bCanEverTick)
    {
        m_TickingActors.Add(Actor);
    }
    if (Actor->m_bTickPrePhysics)
    {
        m_PrePhysicsTickActors.Add(Actor);
    }
    if (Actor->m_bTickPostPhysics)
    {
        m_PostPhysicsTickActors.Add(Actor);
    }
    if (Actor->m_bLateUpdate)
    {
        m_LateUpdateActors.Add(Actor);
    }

    for (WorldTimer* timer = Actor->m_TimerList; timer; timer = timer->m_NextInActor)
    {
        RegisterTimer(timer);
    }

    Actor->PreInitializeComponents();

    for (ActorComponent* component : Actor->m_Components)
    {
        HK_ASSERT(!component->m_bInitialized);

        component->InitializeComponent();
        component->m_bInitialized = true;

        if (component->m_bCanEverTick)
        {
            m_TickingComponents.Add(component);
            component->m_bTicking = true;
        }
    }

    Actor->PostInitializeComponents();

    for (ActorComponent* component : Actor->m_Components)
    {
        HK_ASSERT(!component->IsPendingKill());
        component->BeginPlay();
    }

    Actor->CallBeginPlay();
}

void World::CleanupActor(Actor* Actor)
{
    E_OnActorSpawned.Remove(Actor);
    E_OnPrepareRenderFrontend.Remove(Actor);

    Actor->RemoveAllTimers();

    Actor->m_Level = nullptr;
    Actor->m_World = nullptr;

    if (Actor->m_Instigator)
    {
        Actor->m_Instigator->RemoveRef();
        Actor->m_Instigator = nullptr;
    }

    if (Actor->m_pWeakRefFlag)
    {
        // Tell the ones that hold weak references that the object is destroyed
        Actor->m_pWeakRefFlag->Set(true);
        Actor->m_pWeakRefFlag->Release();
        Actor->m_pWeakRefFlag = nullptr;
    }

    if (Actor->m_ScriptModule)
    {
        Actor->m_ScriptModule->Release();
        Actor->m_ScriptModule = nullptr;
    }
}

void World::BroadcastActorSpawned(Actor* _SpawnedActor)
{
    E_OnActorSpawned.Dispatch(_SpawnedActor);
}

void World::UpdatePauseStatus()
{
    if (m_bPauseRequest)
    {
        m_bPauseRequest = false;
        m_bPaused = true;
        LOG("Game paused\n");
    }
    else if (m_bUnpauseRequest)
    {
        m_bUnpauseRequest = false;
        m_bPaused = false;
        LOG("Game unpaused\n");
    }
}

void World::UpdateTimers(float TimeStep)
{
    for (WorldTimer* timer = m_TimerList; timer;)
    {
        // The timer can be unregistered during the Tick function, so we keep a pointer to the next timer.
        m_pNextTickingTimer = timer->m_NextInWorld;

        timer->Tick(this, TimeStep);

        timer = m_pNextTickingTimer;
    }
}

void World::SpawnActors()
{
    Actor* actor = m_PendingSpawnActors;

    m_PendingSpawnActors = nullptr;

    while (actor)
    {
        Actor* nextActor = actor->m_NextSpawnActor;

        if (!actor->IsPendingKill())
        {
            actor->m_bSpawning = false;

            // Add actor to world
            m_Actors.Add(actor);
            actor->m_IndexInWorldArrayOfActors = m_Actors.Size() - 1;
            // Add actor to level
            actor->m_Level->m_Actors.Add(actor);
            actor->m_IndexInLevelArrayOfActors = actor->m_Level->m_Actors.Size() - 1;
            InitializeAndPlay(actor);

            BroadcastActorSpawned(actor);
        }

        actor = nextActor;
    }
}

void World::KillActors(bool bClearSpawnQueue)
{
    do
    {
        // Remove components
        ActorComponent* component = m_PendingKillComponents;

        m_PendingKillComponents = nullptr;

        while (component)
        {
            ActorComponent* nextComponent = component->m_NextPendingKillComponent;

            if (component->m_bInitialized)
            {
                component->DeinitializeComponent();
                component->m_bInitialized = false;
            }

            // Remove component from actor
            Actor* owner = component->m_OwnerActor;
            if (owner)
            {
                owner->m_Components[component->m_ComponentIndex] = owner->m_Components[owner->m_Components.Size() - 1];
                owner->m_Components[component->m_ComponentIndex]->m_ComponentIndex = component->m_ComponentIndex;
                owner->m_Components.RemoveLast();
            }
            component->m_ComponentIndex = -1;
            component->m_OwnerActor = nullptr;

            if (component->m_bTicking)
                m_TickingComponents.Remove(m_TickingComponents.IndexOf(component)); // TODO: Optimize!

            component->RemoveRef();

            component = nextComponent;
        }

        // Remove actors
        Actor* actor = m_PendingKillActors;

        m_PendingKillActors = nullptr;

        while (actor)
        {
            Actor* nextActor = actor->m_NextPendingKillActor;

            // Actor is not in spawn queue
            if (!actor->m_bSpawning)
            {
                // Remove actor from level
                //Level* level                                                              = actor->Level;
                //level->Actors[actor->m_IndexInLevelArrayOfActors]                            = level->Actors[level->Actors.Size() - 1];
                //level->Actors[actor->m_IndexInLevelArrayOfActors]->m_IndexInLevelArrayOfActors = actor->m_IndexInLevelArrayOfActors;
                //level->Actors.RemoveLast();
                //actor->m_IndexInLevelArrayOfActors = -1;

                // Remove actor from world
                m_Actors[actor->m_IndexInWorldArrayOfActors] = m_Actors[m_Actors.Size() - 1];
                m_Actors[actor->m_IndexInWorldArrayOfActors]->m_IndexInWorldArrayOfActors = actor->m_IndexInWorldArrayOfActors;
                m_Actors.RemoveLast();
                actor->m_IndexInWorldArrayOfActors = -1;

                if (actor->m_bCanEverTick)
                    m_TickingActors.Remove(m_TickingActors.IndexOf(actor)); // TODO: Optimize!
                if (actor->m_bTickPrePhysics)
                    m_PrePhysicsTickActors.Remove(m_PrePhysicsTickActors.IndexOf(actor)); // TODO: Optimize!
                if (actor->m_bTickPostPhysics)
                    m_PostPhysicsTickActors.Remove(m_PostPhysicsTickActors.IndexOf(actor)); // TODO: Optimize!
                if (actor->m_bLateUpdate)
                    m_LateUpdateActors.Remove(m_LateUpdateActors.IndexOf(actor)); // TODO: Optimize!
            }

            CleanupActor(actor);

            actor->RemoveRef();

            actor = nextActor;
        }

        // Continue to destroy the actors, if any.
    } while (m_PendingKillActors || m_PendingKillComponents);

    if (bClearSpawnQueue)
    {
        // Kill the actors from the spawn queue
        Actor* actor = m_PendingSpawnActors;
        m_PendingSpawnActors = nullptr;
        while (actor)
        {
            Actor* nextActor = actor->m_NextSpawnActor;
            actor->m_bSpawning = false;
            CleanupActor(actor);
            actor->RemoveRef();
            actor = nextActor;
        }
    }
}

void World::UpdateActors(float TimeStep)
{
    for (ActorComponent* component : m_TickingComponents)
    {
        Actor* actor = component->GetOwnerActor();

        if (actor->IsPendingKill() || component->IsPendingKill())
        {
            continue;
        }

        if (m_bPaused && !actor->m_bTickEvenWhenPaused)
        {
            continue;
        }

        component->TickComponent(TimeStep);
    }

    for (Actor* actor : m_TickingActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        if (m_bPaused && !actor->m_bTickEvenWhenPaused)
        {
            continue;
        }

        actor->CallTick(TimeStep);
    }
}

void World::UpdateActorsPrePhysics(float TimeStep)
{
    // TickComponentsPrePhysics - TODO?

    for (Actor* actor : m_PrePhysicsTickActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        actor->CallTickPrePhysics(TimeStep);
    }
}

void World::UpdateActorsPostPhysics(float TimeStep)
{
    // TickComponentsPostPhysics - TODO?

    for (Actor* actor : m_PostPhysicsTickActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        actor->CallTickPostPhysics(TimeStep);
    }

    for (Actor* actor : m_TickingActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        // Update actor life span
        actor->m_LifeTime += TimeStep;

        if (actor->LifeSpan != LIFESPAN_ALIVE)
        {
            actor->LifeSpan -= TimeStep;

            if (actor->LifeSpan <= LIFESPAN_ALIVE)
            {
                actor->Destroy();
            }
        }
    }
}

void World::UpdateLevels(float TimeStep)
{
    VisibilitySystem.UpdatePrimitiveLinks();
}

void World::HandlePrePhysics(float TimeStep)
{
    m_GameplayTimeMicro = m_GameplayTimeMicroAfterTick;

    // Tick actors
    UpdateActorsPrePhysics(TimeStep);
}

void World::HandlePostPhysics(float TimeStep)
{
    UpdateActorsPostPhysics(TimeStep);

    if (m_bResetGameplayTimer)
    {
        m_bResetGameplayTimer = false;
        m_GameplayTimeMicroAfterTick = 0;
    }
    else
    {
        m_GameplayTimeMicroAfterTick += (double)TimeStep * 1000000.0;
    }
}

void World::UpdatePhysics(float TimeStep)
{
    if (m_bPaused)
    {
        return;
    }

    PhysicsSystem.Simulate(TimeStep);

    E_OnPostPhysicsUpdate.Dispatch(TimeStep);
}

void World::LateUpdate(float TimeStep)
{
    for (Actor* actor : m_LateUpdateActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        if (m_bPaused && !actor->m_bTickEvenWhenPaused)
        {
            continue;
        }

        actor->CallLateUpdate(TimeStep);
    }
}

void World::Tick(float TimeStep)
{
    m_GameRunningTimeMicro = m_GameRunningTimeMicroAfterTick;
    m_GameplayTimeMicro = m_GameplayTimeMicroAfterTick;

    UpdatePauseStatus();

    // Tick timers. FIXME: move to PrePhysicsTick?
    UpdateTimers(TimeStep);

    SpawnActors();

    // Tick actors
    UpdateActors(TimeStep);

    // Tick physics
    UpdatePhysics(TimeStep);

    // Tick navigation
    NavigationMesh.Update(TimeStep);

    LateUpdate(TimeStep);

    // Tick skinning
    SkinningSystem.Update();

    KillActors();

    // Tick levels
    // NOTE: Update level after KillActors() to relink primitives. Ugly. Fix this.
    UpdateLevels(TimeStep);

    uint64_t frameDuration = (double)TimeStep * 1000000;
    m_GameRunningTimeMicroAfterTick += frameDuration;
}

bool World::Raycast(WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastTriangles(Result, RayStart, RayEnd, Filter);
}

bool World::RaycastBounds(TPodVector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastBounds(Result, RayStart, RayEnd, Filter);
}

bool World::RaycastClosest(WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastClosest(Result, RayStart, RayEnd, Filter);
}

bool World::RaycastClosestBounds(BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastClosestBounds(Result, RayStart, RayEnd, Filter);
}

void World::QueryVisiblePrimitives(TPodVector<PrimitiveDef*>& VisPrimitives, TPodVector<SurfaceDef*>& VisSurfs, int* VisPass, VisibilityQuery const& Query)
{
    VisibilitySystem.QueryVisiblePrimitives(VisPrimitives, VisSurfs, VisPass, Query);
}

void World::QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<VisArea*>& Areas)
{
    VisibilitySystem.QueryOverplapAreas(Bounds, Areas);
}

void World::QueryOverplapAreas(BvSphere const& Bounds, TPodVector<VisArea*>& Areas)
{
    VisibilitySystem.QueryOverplapAreas(Bounds, Areas);
}

void World::ApplyRadialDamage(float _DamageAmount, Float3 const& _Position, float _Radius, CollisionQueryFilter const* _QueryFilter)
{
    TPodVector<Actor*> damagedActors;
    ActorDamage damage;

    QueryActors(damagedActors, _Position, _Radius, _QueryFilter);

    damage.Amount = _DamageAmount;
    damage.Position = _Position;
    damage.Radius = _Radius;
    damage.DamageCauser = nullptr;

    for (Actor* damagedActor : damagedActors)
    {
        damagedActor->ApplyDamage(damage);
    }
}

void World::AddLevel(Level* _Level)
{
    if (_Level->IsPersistentLevel())
    {
        LOG("World::AddLevel: Can't add persistent level\n");
        return;
    }

    if (_Level->m_OwnerWorld == this)
    {
        // Already in world
        return;
    }

    if (_Level->m_OwnerWorld)
    {
        _Level->m_OwnerWorld->RemoveLevel(_Level);
    }

    _Level->m_OwnerWorld = this;
    _Level->AddRef();
    _Level->OnAddLevelToWorld();
    m_ArrayOfLevels.Add(_Level);

    VisibilitySystem.RegisterLevel(_Level->Visibility);
}

void World::RemoveLevel(Level* _Level)
{
    if (!_Level)
    {
        return;
    }

    if (_Level->IsPersistentLevel())
    {
        LOG("World::AddLevel: Can't remove persistent level\n");
        return;
    }

    if (_Level->m_OwnerWorld != this)
    {
        LOG("World::AddLevel: level is not in world\n");
        return;
    }

    _Level->OnRemoveLevelFromWorld();

    m_ArrayOfLevels.Remove(m_ArrayOfLevels.IndexOf(_Level));

    VisibilitySystem.UnregisterLevel(_Level->Visibility);

    _Level->m_OwnerWorld = nullptr;
    _Level->RemoveRef();
}

void World::RegisterTimer(WorldTimer* timer)
{
    if (INTRUSIVE_EXISTS(timer, m_NextInWorld, m_PrevInWorld, m_TimerList, m_TimerListTail))
    {
        // Already in the world
        return;
    }

    timer->AddRef();
    INTRUSIVE_ADD(timer, m_NextInWorld, m_PrevInWorld, m_TimerList, m_TimerListTail);
}

void World::UnregisterTimer(WorldTimer* timer)
{
    if (!INTRUSIVE_EXISTS(timer, m_NextInWorld, m_PrevInWorld, m_TimerList, m_TimerListTail))
    {
        return;
    }

    if (m_pNextTickingTimer && m_pNextTickingTimer == timer)
    {
        m_pNextTickingTimer = timer->m_NextInWorld;
    }
    INTRUSIVE_REMOVE(timer, m_NextInWorld, m_PrevInWorld, m_TimerList, m_TimerListTail);

    timer->RemoveRef();
}

void World::DrawDebug(DebugRenderer* InRenderer)
{
    VisibilitySystem.DrawDebug(InRenderer);

    for (Level* level : m_ArrayOfLevels)
    {
        level->DrawDebug(InRenderer);
    }

    for (Actor* actor : m_Actors)
    {
        actor->CallDrawDebug(InRenderer);
    }

    PhysicsSystem.DrawDebug(InRenderer);

    NavigationMesh.DrawDebug(InRenderer);
}

World* World::CreateWorld()
{
    World* world = new World;
    world->AddRef();

    // Add world to the game
    m_Worlds.Add(world);

    return world;
}

void World::DestroyWorlds()
{
    for (World* world : m_Worlds)
    {
        world->Destroy();
    }
}

void World::KillWorlds()
{
    while (m_PendingKillWorlds)
    {
        World* world = m_PendingKillWorlds;
        World* nextWorld;

        m_PendingKillWorlds = nullptr;

        while (world)
        {
            nextWorld = world->m_NextPendingKillWorld;

            const bool bClearSpawnQueue = true;
            world->KillActors(bClearSpawnQueue);

            // Remove all levels from the world including persistent level
            //for (Level* level : world->ArrayOfLevels)
            //{
            //    level->OnRemoveLevelFromWorld();
            //    level->OwnerWorld = nullptr;
            //    level->RemoveRef();
            //}
            //world->ArrayOfLevels.Clear();

            // Remove the world from the game
            m_Worlds.Remove(m_Worlds.IndexOf(world));
            if (world->m_bTicking)
            {
                m_TickingWorlds.Remove(m_TickingWorlds.IndexOf(world));
                world->m_bTicking = false;
            }
            world->RemoveRef();

            world = nextWorld;
        }
    }

    // Static things must be manually freed
    if (m_Worlds.IsEmpty())
        m_Worlds.Free();
    if (m_TickingWorlds.IsEmpty())
        m_TickingWorlds.Free();
}

void World::UpdateWorlds(float TimeStep)
{
    HK_PROFILER_EVENT("Update worlds");

    for (World* world : m_Worlds)
    {
        if (!world->m_bTicking)
        {
            world->m_bTicking = true;
            m_TickingWorlds.Add(world);
        }
    }

    for (World* world : m_TickingWorlds)
    {
        if (world->IsPendingKill())
        {
            continue;
        }
        world->Tick(TimeStep);
    }

    KillWorlds();

    VisibilitySystem::PrimitivePool.CleanupEmptyBlocks();
    VisibilitySystem::PrimitiveLinkPool.CleanupEmptyBlocks();
}

void World::SetGlobalEnvironmentMap(EnvironmentMap* EnvironmentMap)
{
    m_GlobalEnvironmentMap = EnvironmentMap;
}

HK_NAMESPACE_END
