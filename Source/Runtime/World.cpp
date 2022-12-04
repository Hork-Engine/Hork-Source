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
#include "PointLightComponent.h"
#include "Engine.h"
#include "EnvironmentMap.h"

#include <Platform/Logger.h>
#include <Core/IntrusiveLinkedListMacro.h>

#include <angelscript.h>

AWorld*          AWorld::m_PendingKillWorlds = nullptr;
TVector<AWorld*> AWorld::m_Worlds;
TVector<AWorld*> AWorld::m_TickingWorlds;

AWorld::AWorld()
{
    m_PersistentLevel = NewObj<ALevel>();
    m_PersistentLevel->AddRef();
    m_PersistentLevel->m_OwnerWorld    = this;
    m_PersistentLevel->m_bIsPersistent = true;
    m_PersistentLevel->OnAddLevelToWorld();

    SVisibilitySystemCreateInfo ci{};
    m_PersistentLevel->Visibility = MakeRef<AVisibilityLevel>(ci);
    m_ArrayOfLevels.Add(m_PersistentLevel);

    VisibilitySystem.RegisterLevel(m_PersistentLevel->Visibility);

    PhysicsSystem.PrePhysicsCallback.Set(this, &AWorld::HandlePrePhysics);
    PhysicsSystem.PostPhysicsCallback.Set(this, &AWorld::HandlePostPhysics);
}

AWorld::~AWorld()
{
    for (ALevel* level : m_ArrayOfLevels)
    {
        level->OnRemoveLevelFromWorld();

        m_ArrayOfLevels.Remove(m_ArrayOfLevels.IndexOf(level));

        VisibilitySystem.UnregisterLevel(level->Visibility);

        level->m_OwnerWorld = nullptr;
        level->RemoveRef();
    }

    //VisibilitySystem.UnregisterLevel(PersistentLevel->Visibility);
}

void AWorld::SetPaused(bool _Paused)
{
    m_bPauseRequest   = _Paused;
    m_bUnpauseRequest = !_Paused;
}

bool AWorld::IsPaused() const
{
    return m_bPaused;
}

void AWorld::ResetGameplayTimer()
{
    m_bResetGameplayTimer = true;
}

void AWorld::SetPhysicsHertz(int _Hertz)
{
    PhysicsSystem.PhysicsHertz = _Hertz;
}

void AWorld::SetContactSolverSplitImpulse(bool _SplitImpulse)
{
    PhysicsSystem.bContactSolverSplitImpulse = _SplitImpulse;
}

void AWorld::SetContactSolverIterations(int _InterationsCount)
{
    PhysicsSystem.NumContactSolverIterations = _InterationsCount;
}

void AWorld::SetGravityVector(Float3 const& _Gravity)
{
    PhysicsSystem.GravityVector = _Gravity;
    PhysicsSystem.bGravityDirty = true;
}

Float3 const& AWorld::GetGravityVector() const
{
    return PhysicsSystem.GravityVector;
}

void AWorld::Destroy()
{
    if (m_bPendingKill)
    {
        return;
    }

    // Mark world to remove it from the game
    m_bPendingKill         = true;
    m_NextPendingKillWorld = m_PendingKillWorlds;
    m_PendingKillWorlds    = this;

    DestroyActors();
}

void AWorld::DestroyActors()
{
    for (AActor* actor : m_Actors)
    {
        DestroyActor(actor);
    }

    // Destroy actors from spawn queue
    AActor* actor = m_PendingSpawnActors;
    m_PendingSpawnActors = nullptr;
    while (actor)
    {
        AActor* nextActor = actor->m_NextSpawnActor;
        DestroyActor(actor);
        actor = nextActor;
    }
}

void AWorld::DestroyActor(AActor* Actor)
{
    if (Actor->m_bPendingKill)
    {
        return;
    }

    AWorld* world = Actor->m_World;

    HK_ASSERT(world);

    // Mark actor to remove it from the world
    Actor->m_bPendingKill         = true;
    Actor->m_NextPendingKillActor = world->m_PendingKillActors;
    world->m_PendingKillActors    = Actor;

    for (AActorComponent* component : Actor->m_Components)
    {
        DestroyComponent(component);
    }

    ALevel* level = Actor->m_Level;

    level->m_Actors[Actor->m_IndexInLevelArrayOfActors]                              = level->m_Actors[level->m_Actors.Size() - 1];
    level->m_Actors[Actor->m_IndexInLevelArrayOfActors]->m_IndexInLevelArrayOfActors = Actor->m_IndexInLevelArrayOfActors;
    level->m_Actors.RemoveLast();
    Actor->m_IndexInLevelArrayOfActors = -1;
}

void AWorld::DestroyComponent(AActorComponent* Component)
{
    if (Component->m_bPendingKill)
    {
        return;
    }

    AWorld* world = Component->GetWorld();

    HK_ASSERT(world);

    // Mark component pending kill
    Component->m_bPendingKill = true;

    // Add component to pending kill list
    Component->NextPendingKillComponent = world->m_PendingKillComponents;
    world->m_PendingKillComponents      = Component;
}

void AWorld::BuildNavigation(SAINavigationConfig const& _NavigationConfig)
{
    NavigationMesh.Initialize(_NavigationConfig);
    NavigationMesh.Build();
}

SActorSpawnInfo::SActorSpawnInfo(uint64_t _ActorClassId) :
    SActorSpawnInfo(AActor::Factory().LookupClass(_ActorClassId))
{
}

SActorSpawnInfo::SActorSpawnInfo(const char* _ActorClassName) :
    SActorSpawnInfo(AActor::Factory().LookupClass(_ActorClassName))
{
}

void SActorSpawnInfo::SetTemplate(AActor const* _Template)
{
    HK_ASSERT(&_Template->FinalClassMeta() == ActorTypeClassMeta);
    Template = _Template;
}

asIScriptObject* AWorld::CreateScriptModule(AString const& Module, AActor* Actor)
{
    if (!m_ScriptEngine)
        m_ScriptEngine = std::make_unique<AScriptEngine>(this);

    asIScriptObject* scriptModule = m_ScriptEngine->CreateScriptInstance(Module, Actor);

    if (scriptModule)
    {
        AActorScript* script = AActorScript::GetScript(scriptModule);

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

        Actor->m_bCanEverTick     = Actor->m_bCanEverTick || script->M_Tick != nullptr;
        Actor->m_bTickPrePhysics  = Actor->m_bTickPrePhysics || script->M_TickPrePhysics != nullptr;
        Actor->m_bTickPostPhysics = Actor->m_bTickPostPhysics || script->M_TickPostPhysics != nullptr;
        Actor->m_bLateUpdate      = Actor->m_bLateUpdate || script->M_LateUpdate != nullptr;
    }

    return scriptModule;
}

AActor* AWorld::_SpawnActor2(SActorSpawnPrivate& SpawnInfo, STransform const& SpawnTransform)
{
    if (m_bPendingKill)
    {
        LOG("AWorld::SpawnActor: Attempting to spawn an actor from a destroyed world\n");
        return nullptr;
    }

    AClassMeta const* actorClass = SpawnInfo.ActorClass;
    HK_ASSERT(actorClass);

    AActorDefinition* pActorDef = SpawnInfo.ActorDef;

    // Override actor class
    if (pActorDef && pActorDef->GetActorClass())
    {
        actorClass = pActorDef->GetActorClass();

        if (actorClass->Factory() != &AActor::Factory())
        {
            LOG("AWorld::SpawnActor: wrong C++ actor class specified\n");
            actorClass = &AActor::ClassMeta();
        }
    }

    AActor* actor = static_cast<AActor*>(actorClass->CreateInstance());
    actor->AddRef();
    actor->m_bInEditor = SpawnInfo.bInEditor;
    actor->m_pActorDef = pActorDef;

    // Create components from actor definition
    if (pActorDef)
    {
        TPodVector<AActorComponent*> components;

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
                    HK_ASSERT(component->FinalClassMeta().IsSubclassOf<ASceneComponent>());
                    actor->m_RootComponent = static_cast<ASceneComponent*>(component);
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
                    HK_ASSERT(components[componentIndex]->FinalClassMeta().IsSubclassOf<ASceneComponent>());
                    HK_ASSERT(components[componentDef.ParentIndex]->FinalClassMeta().IsSubclassOf<ASceneComponent>());

                    ASceneComponent* sceneComponent  = static_cast<ASceneComponent*>(components[componentIndex]);
                    ASceneComponent* parentComponent = static_cast<ASceneComponent*>(components[componentDef.ParentIndex]);

                    // TODO: Socket!!!
                    sceneComponent->AttachTo(parentComponent);
                }
            }

            componentIndex++;
        }
    }

    // Initialize actor
    SActorInitializer initializer;
    actor->Initialize(initializer);
    actor->m_bCanEverTick        = initializer.bCanEverTick;
    actor->m_bTickEvenWhenPaused = initializer.bTickEvenWhenPaused;
    actor->m_bTickPrePhysics     = initializer.bTickPrePhysics;
    actor->m_bTickPostPhysics    = initializer.bTickPostPhysics;
    actor->m_bLateUpdate         = initializer.bLateUpdate;

    // Set properties for the actor
    if (pActorDef)
        actor->SetProperties(pActorDef->GetActorPropertyHash());

    // Create script
    AString const& scriptModule = pActorDef ? pActorDef->GetScriptModule() : SpawnInfo.ScriptModule;
    if (!scriptModule.IsEmpty())
    {
        actor->m_ScriptModule = CreateScriptModule(scriptModule, actor);
        if (actor->m_ScriptModule)
        {
            if (pActorDef)
            {
                AActorScript::SetProperties(actor->m_ScriptModule, pActorDef->GetScriptPropertyHash());
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

        auto FindTemplateComponent = [](AActor const* Template, AActorComponent* Component) -> AActorComponent*
        {
            auto classId = Component->FinalClassId();
            auto localId = Component->m_LocalId;
            for (AActorComponent* component : Template->GetComponents())
            {
                if (component->FinalClassId() == classId && component->m_LocalId == localId)
                    return component;
            }
            return {};
        };

        // Clone component properties
        for (AActorComponent* component : actor->GetComponents())
        {
            AActorComponent* templateComponent = FindTemplateComponent(SpawnInfo.Template, component);
            if (templateComponent)
                AClassMeta::CloneProperties(templateComponent, component);
        }

        if (actor->m_ScriptModule && SpawnInfo.Template->m_ScriptModule)
        {
            AActorScript::CloneProperties(SpawnInfo.Template->m_ScriptModule, actor->m_ScriptModule);
            // TODO: Clone script properties
        }

        AClassMeta::CloneProperties(SpawnInfo.Template, actor);
    }

    if (SpawnInfo.Instigator)
    {
        actor->m_Instigator = SpawnInfo.Instigator;
        actor->m_Instigator->AddRef();
    }

    actor->m_World = this;
    actor->m_Level = SpawnInfo.Level ? SpawnInfo.Level : m_PersistentLevel;

    if (actor->m_bInEditor)
    {
        // FIXME: Specify avatar in ActorDef?
        ActorComponents tempArray = actor->m_Components;
        for (AActorComponent* component : tempArray)
        {
            component->OnCreateAvatar();
        }

        // FIXME: Or better create in actor module? Like this:
        // Actor->CreateAvatar();
        // void AActor::CreateAvatar()
        // {
        //      CALL_SCRIPT(CreateAvatar);
        // }
    }

    if (actor->m_RootComponent)
    {
        actor->m_RootComponent->SetTransform(SpawnTransform);
    }

    actor->m_NextSpawnActor = m_PendingSpawnActors;
    m_PendingSpawnActors    = actor;

    return actor;
}

AActor* AWorld::SpawnActor(SActorSpawnInfo const& _SpawnInfo)
{
    SActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass = _SpawnInfo.ActorClassMeta();
    spawnInfo.Template   = _SpawnInfo.GetTemplate();
    spawnInfo.Instigator = _SpawnInfo.Instigator;
    spawnInfo.Level      = _SpawnInfo.Level;
    spawnInfo.bInEditor  = _SpawnInfo.bInEditor;

    if (!spawnInfo.ActorClass)
    {
        LOG("AWorld::SpawnActor: invalid actor class\n");
        return nullptr;
    }

    if (spawnInfo.ActorClass->Factory() != &AActor::Factory())
    {
        LOG("AWorld::SpawnActor: not an actor class\n");
        return nullptr;
    }

    if (spawnInfo.Template && spawnInfo.ActorClass != &spawnInfo.Template->FinalClassMeta())
    {
        LOG("AWorld::SpawnActor: SActorSpawnInfo::Template class doesn't match meta data\n");
        return nullptr;
    }

    return _SpawnActor2(spawnInfo, _SpawnInfo.SpawnTransform);
}

AActor* AWorld::SpawnActor2(STransform const& SpawnTransform, AActor* Instigator, ALevel* Level, bool bInEditor)
{
    SActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass = &AActor::ClassMeta();
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level      = Level;
    spawnInfo.bInEditor  = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

AActor* AWorld::SpawnActor2(AActorDefinition* pActorDef, STransform const& SpawnTransform, AActor* Instigator, ALevel* Level, bool bInEditor)
{
    if (!pActorDef)
    {
        LOG("AWorld::SpawnActor: invalid actor definition\n");
    }

    SActorSpawnPrivate spawnInfo;
    spawnInfo.ActorDef   = pActorDef;
    spawnInfo.ActorClass = &AActor::ClassMeta();
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level      = Level;
    spawnInfo.bInEditor  = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

AActor* AWorld::SpawnActor2(AString const& ScriptModule, STransform const& SpawnTransform, AActor* Instigator, ALevel* Level, bool bInEditor)
{
    if (ScriptModule.IsEmpty())
    {
        LOG("AWorld::SpawnActor: invalid script module\n");
    }

    SActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass   = &AActor::ClassMeta();
    spawnInfo.ScriptModule = ScriptModule;
    spawnInfo.Instigator   = Instigator;
    spawnInfo.Level        = Level;
    spawnInfo.bInEditor    = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

AActor* AWorld::SpawnActor2(AClassMeta const* ActorClass, STransform const& SpawnTransform, AActor* Instigator, ALevel* Level, bool bInEditor)
{
    if (!ActorClass)
    {
        LOG("AWorld::SpawnActor: invalid C++ module class\n");
        ActorClass = &AActor::ClassMeta();
    }

    SActorSpawnPrivate spawnInfo;
    spawnInfo.ActorClass = ActorClass;
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level      = Level;
    spawnInfo.bInEditor  = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

AActor* AWorld::SpawnActor2(AActor const* Template, STransform const& SpawnTransform, AActor* Instigator, ALevel* Level, bool bInEditor)
{
    SActorSpawnPrivate spawnInfo;

    if (Template)
    {
        if (Template->m_pActorDef)
        {
            spawnInfo.ActorDef = Template->m_pActorDef;
        }
        else if (Template->m_ScriptModule)
        {
            AActorScript* pScript  = AActorScript::GetScript(Template->m_ScriptModule);
            spawnInfo.ScriptModule = pScript->GetModule();
        }
    }
    else
    {
        LOG("AWorld::SpawnActor: invalid template\n");
    }

    spawnInfo.ActorClass = Template ? &Template->FinalClassMeta() : &AActor::ClassMeta();
    spawnInfo.Template   = Template;
    spawnInfo.Instigator = Instigator;
    spawnInfo.Level      = Level;
    spawnInfo.bInEditor  = bInEditor;
    return _SpawnActor2(spawnInfo, SpawnTransform);
}

void AWorld::InitializeAndPlay(AActor* Actor)
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

    for (ATimer* timer = Actor->m_TimerList; timer; timer = timer->NextInActor)
    {
        RegisterTimer(timer);
    }

    Actor->PreInitializeComponents();

    for (AActorComponent* component : Actor->m_Components)
    {
        HK_ASSERT(!component->m_bInitialized);

        component->InitializeComponent();
        component->m_bInitialized = true;

        if (component->bCanEverTick)
        {
            m_TickingComponents.Add(component);
            component->m_bTicking = true;
        }
    }

    Actor->PostInitializeComponents();

    for (AActorComponent* component : Actor->m_Components)
    {
        HK_ASSERT(!component->IsPendingKill());
        component->BeginPlay();
    }

    Actor->CallBeginPlay();
}

void AWorld::CleanupActor(AActor* Actor)
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

void AWorld::BroadcastActorSpawned(AActor* _SpawnedActor)
{
    E_OnActorSpawned.Dispatch(_SpawnedActor);
}

void AWorld::UpdatePauseStatus()
{
    if (m_bPauseRequest)
    {
        m_bPauseRequest = false;
        m_bPaused       = true;
        LOG("Game paused\n");
    }
    else if (m_bUnpauseRequest)
    {
        m_bUnpauseRequest = false;
        m_bPaused         = false;
        LOG("Game unpaused\n");
    }
}

void AWorld::UpdateTimers(float TimeStep)
{
    for (ATimer* timer = m_TimerList; timer;)
    {
        // The timer can be unregistered during the Tick function, so we keep a pointer to the next timer.
        m_pNextTickingTimer = timer->NextInWorld;

        timer->Tick(this, TimeStep);

        timer = m_pNextTickingTimer;
    }
}

void AWorld::SpawnActors()
{
    AActor* actor = m_PendingSpawnActors;

    m_PendingSpawnActors = nullptr;

    while (actor)
    {
        AActor* nextActor = actor->m_NextSpawnActor;

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

void AWorld::KillActors(bool bClearSpawnQueue)
{
    do
    {
        // Remove components
        AActorComponent* component = m_PendingKillComponents;

        m_PendingKillComponents = nullptr;

        while (component)
        {
            AActorComponent* nextComponent = component->NextPendingKillComponent;

            if (component->m_bInitialized)
            {
                component->DeinitializeComponent();
                component->m_bInitialized = false;
            }

            // Remove component from actor
            AActor* owner = component->m_OwnerActor;
            if (owner)
            {
                owner->m_Components[component->m_ComponentIndex]               = owner->m_Components[owner->m_Components.Size() - 1];
                owner->m_Components[component->m_ComponentIndex]->m_ComponentIndex = component->m_ComponentIndex;
                owner->m_Components.RemoveLast();
            }
            component->m_ComponentIndex = -1;
            component->m_OwnerActor     = nullptr;

            if (component->m_bTicking)
                m_TickingComponents.Remove(m_TickingComponents.IndexOf(component)); // TODO: Optimize!

            component->RemoveRef();

            component = nextComponent;
        }

        // Remove actors
        AActor* actor = m_PendingKillActors;

        m_PendingKillActors = nullptr;

        while (actor)
        {
            AActor* nextActor = actor->m_NextPendingKillActor;

            // Actor is not in spawn queue
            if (!actor->m_bSpawning)
            {
                // Remove actor from level
                //ALevel* level                                                              = actor->Level;
                //level->Actors[actor->m_IndexInLevelArrayOfActors]                            = level->Actors[level->Actors.Size() - 1];
                //level->Actors[actor->m_IndexInLevelArrayOfActors]->m_IndexInLevelArrayOfActors = actor->m_IndexInLevelArrayOfActors;
                //level->Actors.RemoveLast();
                //actor->m_IndexInLevelArrayOfActors = -1;

                // Remove actor from world
                m_Actors[actor->m_IndexInWorldArrayOfActors]                              = m_Actors[m_Actors.Size() - 1];
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
        AActor* actor        = m_PendingSpawnActors;
        m_PendingSpawnActors = nullptr;
        while (actor)
        {
            AActor* nextActor = actor->m_NextSpawnActor;
            actor->m_bSpawning = false;
            CleanupActor(actor);
            actor->RemoveRef();
            actor = nextActor;
        }
    }
}

void AWorld::UpdateActors(float TimeStep)
{
    for (AActorComponent* component : m_TickingComponents)
    {
        AActor* actor = component->GetOwnerActor();

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

    for (AActor* actor : m_TickingActors)
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

void AWorld::UpdateActorsPrePhysics(float TimeStep)
{
    // TickComponentsPrePhysics - TODO?

    for (AActor* actor : m_PrePhysicsTickActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        actor->CallTickPrePhysics(TimeStep);
    }
}

void AWorld::UpdateActorsPostPhysics(float TimeStep)
{
    // TickComponentsPostPhysics - TODO?

    for (AActor* actor : m_PostPhysicsTickActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        actor->CallTickPostPhysics(TimeStep);
    }

    for (AActor* actor : m_TickingActors)
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

void AWorld::UpdateLevels(float TimeStep)
{
    VisibilitySystem.UpdatePrimitiveLinks();
}

void AWorld::HandlePrePhysics(float TimeStep)
{
    m_GameplayTimeMicro = m_GameplayTimeMicroAfterTick;

    // Tick actors
    UpdateActorsPrePhysics(TimeStep);
}

void AWorld::HandlePostPhysics(float TimeStep)
{
    UpdateActorsPostPhysics(TimeStep);

    if (m_bResetGameplayTimer)
    {
        m_bResetGameplayTimer      = false;
        m_GameplayTimeMicroAfterTick = 0;
    }
    else
    {
        m_GameplayTimeMicroAfterTick += (double)TimeStep * 1000000.0;
    }
}

void AWorld::UpdatePhysics(float TimeStep)
{
    if (m_bPaused)
    {
        return;
    }

    PhysicsSystem.Simulate(TimeStep);

    E_OnPostPhysicsUpdate.Dispatch(TimeStep);
}

void AWorld::LateUpdate(float TimeStep)
{
    for (AActor* actor : m_LateUpdateActors)
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

void AWorld::Tick(float TimeStep)
{
    m_GameRunningTimeMicro = m_GameRunningTimeMicroAfterTick;
    m_GameplayTimeMicro    = m_GameplayTimeMicroAfterTick;

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

bool AWorld::Raycast(SWorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastTriangles(Result, RayStart, RayEnd, Filter);
}

bool AWorld::RaycastBounds(TPodVector<SBoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastBounds(Result, RayStart, RayEnd, Filter);
}

bool AWorld::RaycastClosest(SWorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastClosest(Result, RayStart, RayEnd, Filter);
}

bool AWorld::RaycastClosestBounds(SBoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, SWorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastClosestBounds(Result, RayStart, RayEnd, Filter);
}

void AWorld::QueryVisiblePrimitives(TPodVector<SPrimitiveDef*>& VisPrimitives, TPodVector<SSurfaceDef*>& VisSurfs, int* VisPass, SVisibilityQuery const& Query)
{
    VisibilitySystem.QueryVisiblePrimitives(VisPrimitives, VisSurfs, VisPass, Query);
}

void AWorld::QueryOverplapAreas(BvAxisAlignedBox const& Bounds, TPodVector<SVisArea*>& Areas)
{
    VisibilitySystem.QueryOverplapAreas(Bounds, Areas);
}

void AWorld::QueryOverplapAreas(BvSphere const& Bounds, TPodVector<SVisArea*>& Areas)
{
    VisibilitySystem.QueryOverplapAreas(Bounds, Areas);
}

void AWorld::ApplyRadialDamage(float _DamageAmount, Float3 const& _Position, float _Radius, SCollisionQueryFilter const* _QueryFilter)
{
    TPodVector<AActor*> damagedActors;
    SActorDamage        damage;

    QueryActors(damagedActors, _Position, _Radius, _QueryFilter);

    damage.Amount       = _DamageAmount;
    damage.Position     = _Position;
    damage.Radius       = _Radius;
    damage.DamageCauser = nullptr;

    for (AActor* damagedActor : damagedActors)
    {
        damagedActor->ApplyDamage(damage);
    }
}

void AWorld::AddLevel(ALevel* _Level)
{
    if (_Level->IsPersistentLevel())
    {
        LOG("AWorld::AddLevel: Can't add persistent level\n");
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

void AWorld::RemoveLevel(ALevel* _Level)
{
    if (!_Level)
    {
        return;
    }

    if (_Level->IsPersistentLevel())
    {
        LOG("AWorld::AddLevel: Can't remove persistent level\n");
        return;
    }

    if (_Level->m_OwnerWorld != this)
    {
        LOG("AWorld::AddLevel: level is not in world\n");
        return;
    }

    _Level->OnRemoveLevelFromWorld();

    m_ArrayOfLevels.Remove(m_ArrayOfLevels.IndexOf(_Level));

    VisibilitySystem.UnregisterLevel(_Level->Visibility);

    _Level->m_OwnerWorld = nullptr;
    _Level->RemoveRef();
}

void AWorld::RegisterTimer(ATimer* Timer)
{
    if (INTRUSIVE_EXISTS(Timer, NextInWorld, PrevInWorld, m_TimerList, m_TimerListTail))
    {
        // Already in the world
        return;
    }

    Timer->AddRef();
    INTRUSIVE_ADD(Timer, NextInWorld, PrevInWorld, m_TimerList, m_TimerListTail);
}

void AWorld::UnregisterTimer(ATimer* Timer)
{
    if (!INTRUSIVE_EXISTS(Timer, NextInWorld, PrevInWorld, m_TimerList, m_TimerListTail))
    {
        return;
    }

    if (m_pNextTickingTimer && m_pNextTickingTimer == Timer)
    {
        m_pNextTickingTimer = Timer->NextInWorld;
    }
    INTRUSIVE_REMOVE(Timer, NextInWorld, PrevInWorld, m_TimerList, m_TimerListTail);

    Timer->RemoveRef();
}

void AWorld::DrawDebug(ADebugRenderer* InRenderer)
{
    VisibilitySystem.DrawDebug(InRenderer);

    for (ALevel* level : m_ArrayOfLevels)
    {
        level->DrawDebug(InRenderer);
    }

    for (AActor* actor : m_Actors)
    {
        actor->CallDrawDebug(InRenderer);
    }

    PhysicsSystem.DrawDebug(InRenderer);

    NavigationMesh.DrawDebug(InRenderer);
}

AWorld* AWorld::CreateWorld()
{
    AWorld* world = new AWorld;
    world->AddRef();

    // Add world to the game
    m_Worlds.Add(world);

    return world;
}

void AWorld::DestroyWorlds()
{
    for (AWorld* world : m_Worlds)
    {
        world->Destroy();
    }
}

void AWorld::KillWorlds()
{
    while (m_PendingKillWorlds)
    {
        AWorld* world = m_PendingKillWorlds;
        AWorld* nextWorld;

        m_PendingKillWorlds = nullptr;

        while (world)
        {
            nextWorld = world->m_NextPendingKillWorld;

            const bool bClearSpawnQueue = true;
            world->KillActors(bClearSpawnQueue);

            // Remove all levels from the world including persistent level
            //for (ALevel* level : world->ArrayOfLevels)
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

void AWorld::UpdateWorlds(float TimeStep)
{
    for (AWorld* world : m_Worlds)
    {
        if (!world->m_bTicking)
        {
            world->m_bTicking = true;
            m_TickingWorlds.Add(world);
        }
    }

    for (AWorld* world : m_TickingWorlds)
    {
        if (world->IsPendingKill())
        {
            continue;
        }
        world->Tick(TimeStep);
    }

    KillWorlds();

    AVisibilitySystem::PrimitivePool.CleanupEmptyBlocks();
    AVisibilitySystem::PrimitiveLinkPool.CleanupEmptyBlocks();
}

void AWorld::SetGlobalEnvironmentMap(AEnvironmentMap* EnvironmentMap)
{
    m_GlobalEnvironmentMap = EnvironmentMap;
}
