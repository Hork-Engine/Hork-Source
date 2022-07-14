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

HK_CLASS_META(AWorld)

AWorld*             AWorld::PendingKillWorlds = nullptr;
TPodVector<AWorld*> AWorld::Worlds;
TPodVector<AWorld*> AWorld::TickingWorlds;

AWorld::AWorld()
{
    PersistentLevel = CreateInstanceOf<ALevel>();
    PersistentLevel->AddRef();
    PersistentLevel->OwnerWorld    = this;
    PersistentLevel->bIsPersistent = true;
    PersistentLevel->OnAddLevelToWorld();

    SVisibilitySystemCreateInfo ci{};
    PersistentLevel->Visibility = MakeRef<AVisibilityLevel>(ci);
    ArrayOfLevels.Add(PersistentLevel);

    VisibilitySystem.RegisterLevel(PersistentLevel->Visibility);

    PhysicsSystem.PrePhysicsCallback.Set(this, &AWorld::HandlePrePhysics);
    PhysicsSystem.PostPhysicsCallback.Set(this, &AWorld::HandlePostPhysics);
}

AWorld::~AWorld()
{
    for (ALevel* level : ArrayOfLevels)
    {
        level->OnRemoveLevelFromWorld();

        ArrayOfLevels.Remove(ArrayOfLevels.IndexOf(level));

        VisibilitySystem.UnregisterLevel(level->Visibility);

        level->OwnerWorld = nullptr;
        level->RemoveRef();
    }

    //VisibilitySystem.UnregisterLevel(PersistentLevel->Visibility);
}

void AWorld::SetPaused(bool _Paused)
{
    bPauseRequest   = _Paused;
    bUnpauseRequest = !_Paused;
}

bool AWorld::IsPaused() const
{
    return bPaused;
}

void AWorld::ResetGameplayTimer()
{
    bResetGameplayTimer = true;
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
    if (bPendingKill)
    {
        return;
    }

    // Mark world to remove it from the game
    bPendingKill         = true;
    NextPendingKillWorld = PendingKillWorlds;
    PendingKillWorlds    = this;

    DestroyActors();
}

void AWorld::DestroyActors()
{
    for (AActor* actor : Actors)
    {
        DestroyActor(actor);
    }

    // Destroy actors from spawn queue
    AActor* actor = PendingSpawnActors;
    PendingSpawnActors = nullptr;
    while (actor)
    {
        AActor* nextActor = actor->NextSpawnActor;
        DestroyActor(actor);
        actor = nextActor;
    }
}

void AWorld::DestroyActor(AActor* Actor)
{
    if (Actor->bPendingKill)
    {
        return;
    }

    AWorld* world = Actor->World;

    HK_ASSERT(world);

    // Mark actor to remove it from the world
    Actor->bPendingKill         = true;
    Actor->NextPendingKillActor = world->PendingKillActors;
    world->PendingKillActors    = Actor;

    for (AActorComponent* component : Actor->Components)
    {
        DestroyComponent(component);
    }
}

void AWorld::DestroyComponent(AActorComponent* Component)
{
    if (Component->bPendingKill)
    {
        return;
    }

    AWorld* world = Component->GetWorld();

    HK_ASSERT(world);

    // Mark component pending kill
    Component->bPendingKill = true;

    // Add component to pending kill list
    Component->NextPendingKillComponent = world->PendingKillComponents;
    world->PendingKillComponents        = Component;
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
    if (!ScriptEngine)
        ScriptEngine = std::make_unique<AScriptEngine>(this);

    asIScriptObject* scriptModule = ScriptEngine->CreateScriptInstance(Module, Actor);

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

                Actor->bTickEvenWhenPaused = *(const bool*)scriptModule->GetAddressOfProperty(i);
                break;
            }
        }

        Actor->bCanEverTick     = Actor->bCanEverTick || script->M_Tick != nullptr;
        Actor->bTickPrePhysics  = Actor->bTickPrePhysics || script->M_TickPrePhysics != nullptr;
        Actor->bTickPostPhysics = Actor->bTickPostPhysics || script->M_TickPostPhysics != nullptr;
        Actor->bLateUpdate      = Actor->bLateUpdate || script->M_LateUpdate != nullptr;
    }

    return scriptModule;
}

AActor* AWorld::_SpawnActor2(SActorSpawnPrivate& SpawnInfo, STransform const& SpawnTransform)
{
    if (bPendingKill)
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
    actor->bInEditor = SpawnInfo.bInEditor;
    actor->pActorDef = pActorDef;

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
                    actor->RootComponent = static_cast<ASceneComponent*>(component);
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
    actor->bCanEverTick        = initializer.bCanEverTick;
    actor->bTickEvenWhenPaused = initializer.bTickEvenWhenPaused;
    actor->bTickPrePhysics     = initializer.bTickPrePhysics;
    actor->bTickPostPhysics    = initializer.bTickPostPhysics;
    actor->bLateUpdate         = initializer.bLateUpdate;

    // Set properties for the actor
    if (pActorDef)
        SetProperties(pActorDef->GetActorPropertyHash());

    // Create script
    AString const& scriptModule = pActorDef ? pActorDef->GetScriptModule() : SpawnInfo.ScriptModule;
    if (!scriptModule.IsEmpty())
    {
        actor->ScriptModule = CreateScriptModule(scriptModule, actor);
        if (actor->ScriptModule)
        {
            if (pActorDef)
            {
                AActorScript::SetProperties(actor->ScriptModule, pActorDef->GetScriptPropertyHash());
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
            auto localId = Component->LocalId;
            for (AActorComponent* component : Template->GetComponents())
            {
                if (component->FinalClassId() == classId && component->LocalId == localId)
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

        if (actor->ScriptModule && SpawnInfo.Template->ScriptModule)
        {
            AActorScript::CloneProperties(SpawnInfo.Template->ScriptModule, actor->ScriptModule);
            // TODO: Clone script properties
        }

        AClassMeta::CloneProperties(SpawnInfo.Template, actor);
    }

    if (SpawnInfo.Instigator)
    {
        actor->Instigator = SpawnInfo.Instigator;
        actor->Instigator->AddRef();
    }

    actor->World = this;
    actor->Level = SpawnInfo.Level ? SpawnInfo.Level : PersistentLevel;

    if (actor->bInEditor)
    {
        // FIXME: Specify avatar in ActorDef?
        AArrayOfActorComponents tempArray = actor->Components;
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

    if (actor->RootComponent)
    {
        actor->RootComponent->SetTransform(SpawnTransform);
    }

    actor->NextSpawnActor = PendingSpawnActors;
    PendingSpawnActors    = actor;

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
        if (Template->pActorDef)
        {
            spawnInfo.ActorDef = Template->pActorDef;
        }
        else if (Template->ScriptModule)
        {
            AActorScript* pScript  = AActorScript::GetScript(Template->ScriptModule);
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
    if (Actor->bCanEverTick)
    {
        TickingActors.Add(Actor);
    }
    if (Actor->bTickPrePhysics)
    {
        PrePhysicsTickActors.Add(Actor);
    }
    if (Actor->bTickPostPhysics)
    {
        PostPhysicsTickActors.Add(Actor);
    }
    if (Actor->bLateUpdate)
    {
        LateUpdateActors.Add(Actor);
    }

    for (ATimer* timer = Actor->TimerList; timer; timer = timer->NextInActor)
    {
        RegisterTimer(timer);
    }

    Actor->PreInitializeComponents();

    for (AActorComponent* component : Actor->Components)
    {
        HK_ASSERT(!component->bInitialized);

        component->InitializeComponent();
        component->bInitialized = true;

        if (component->bCanEverTick)
        {
            TickingComponents.Add(component);
            component->bTicking = true;
        }
    }

    Actor->PostInitializeComponents();

    for (AActorComponent* component : Actor->Components)
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

    Actor->Level = nullptr;
    Actor->World = nullptr;

    if (Actor->Instigator)
    {
        Actor->Instigator->RemoveRef();
        Actor->Instigator = nullptr;
    }

    if (Actor->pWeakRefFlag)
    {
        // Tell the ones that hold weak references that the object is destroyed
        Actor->pWeakRefFlag->Set(true);
        Actor->pWeakRefFlag->Release();
        Actor->pWeakRefFlag = nullptr;
    }

    if (Actor->ScriptModule)
    {
        Actor->ScriptModule->Release();
        Actor->ScriptModule = nullptr;
    }
}

void AWorld::BroadcastActorSpawned(AActor* _SpawnedActor)
{
    E_OnActorSpawned.Dispatch(_SpawnedActor);
}

void AWorld::UpdatePauseStatus()
{
    if (bPauseRequest)
    {
        bPauseRequest = false;
        bPaused       = true;
        LOG("Game paused\n");
    }
    else if (bUnpauseRequest)
    {
        bUnpauseRequest = false;
        bPaused         = false;
        LOG("Game unpaused\n");
    }
}

void AWorld::UpdateTimers(float TimeStep)
{
    for (ATimer* timer = TimerList; timer;)
    {
        // The timer can be unregistered during the Tick function, so we keep a pointer to the next timer.
        pNextTickingTimer = timer->NextInWorld;

        timer->Tick(this, TimeStep);

        timer = pNextTickingTimer;
    }
}

void AWorld::SpawnActors()
{
    AActor* actor = PendingSpawnActors;

    PendingSpawnActors = nullptr;

    while (actor)
    {
        AActor* nextActor = actor->NextSpawnActor;

        if (!actor->IsPendingKill())
        {
            actor->bSpawning = false;

            // Add actor to world
            Actors.Add(actor);
            actor->IndexInWorldArrayOfActors = Actors.Size() - 1;
            // Add actor to level
            actor->Level->Actors.Add(actor);
            actor->IndexInLevelArrayOfActors = actor->Level->Actors.Size() - 1;
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
        AActorComponent* component = PendingKillComponents;

        PendingKillComponents = nullptr;

        while (component)
        {
            AActorComponent* nextComponent = component->NextPendingKillComponent;

            if (component->bInitialized)
            {
                component->DeinitializeComponent();
                component->bInitialized = false;
            }

            // Remove component from actor
            AActor* owner = component->OwnerActor;
            if (owner)
            {
                owner->Components[component->ComponentIndex]                 = owner->Components[owner->Components.Size() - 1];
                owner->Components[component->ComponentIndex]->ComponentIndex = component->ComponentIndex;
                owner->Components.RemoveLast();
            }
            component->ComponentIndex = -1;
            component->OwnerActor     = nullptr;

            if (component->bTicking)
                TickingComponents.Remove(TickingComponents.IndexOf(component)); // TODO: Optimize!

            component->RemoveRef();

            component = nextComponent;
        }

        // Remove actors
        AActor* actor = PendingKillActors;

        PendingKillActors = nullptr;

        while (actor)
        {
            AActor* nextActor = actor->NextPendingKillActor;

            // Actor is not in spawn queue
            if (!actor->bSpawning)
            {
                // Remove actor from level
                ALevel* level                                                              = actor->Level;
                level->Actors[actor->IndexInLevelArrayOfActors]                            = level->Actors[level->Actors.Size() - 1];
                level->Actors[actor->IndexInLevelArrayOfActors]->IndexInLevelArrayOfActors = actor->IndexInLevelArrayOfActors;
                level->Actors.RemoveLast();
                actor->IndexInLevelArrayOfActors = -1;

                // Remove actor from world
                Actors[actor->IndexInWorldArrayOfActors]                            = Actors[Actors.Size() - 1];
                Actors[actor->IndexInWorldArrayOfActors]->IndexInWorldArrayOfActors = actor->IndexInWorldArrayOfActors;
                Actors.RemoveLast();
                actor->IndexInWorldArrayOfActors = -1;

                if (actor->bCanEverTick)
                    TickingActors.Remove(TickingActors.IndexOf(actor)); // TODO: Optimize!
                if (actor->bTickPrePhysics)
                    PrePhysicsTickActors.Remove(PrePhysicsTickActors.IndexOf(actor)); // TODO: Optimize!
                if (actor->bTickPostPhysics)
                    PostPhysicsTickActors.Remove(PostPhysicsTickActors.IndexOf(actor)); // TODO: Optimize!
                if (actor->bLateUpdate)
                    LateUpdateActors.Remove(LateUpdateActors.IndexOf(actor)); // TODO: Optimize!
            }

            CleanupActor(actor);

            actor->RemoveRef();

            actor = nextActor;
        }

        // Continue to destroy the actors, if any.
    } while (PendingKillActors || PendingKillComponents);

    if (bClearSpawnQueue)
    {
        // Kill the actors from the spawn queue
        AActor* actor      = PendingSpawnActors;
        PendingSpawnActors = nullptr;
        while (actor)
        {
            AActor* nextActor = actor->NextSpawnActor;
            actor->bSpawning  = false;
            CleanupActor(actor);
            actor->RemoveRef();
            actor = nextActor;
        }
    }
}

void AWorld::UpdateActors(float TimeStep)
{
    for (AActorComponent* component : TickingComponents)
    {
        AActor* actor = component->GetOwnerActor();

        if (actor->IsPendingKill() || component->IsPendingKill())
        {
            continue;
        }

        if (bPaused && !actor->bTickEvenWhenPaused)
        {
            continue;
        }

        component->TickComponent(TimeStep);
    }

    for (AActor* actor : TickingActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        if (bPaused && !actor->bTickEvenWhenPaused)
        {
            continue;
        }

        actor->CallTick(TimeStep);
    }
}

void AWorld::UpdateActorsPrePhysics(float TimeStep)
{
    // TickComponentsPrePhysics - TODO?

    for (AActor* actor : PrePhysicsTickActors)
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

    for (AActor* actor : PostPhysicsTickActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        actor->CallTickPostPhysics(TimeStep);
    }

    for (AActor* actor : TickingActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        // Update actor life span
        actor->LifeTime += TimeStep;

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
    GameplayTimeMicro = GameplayTimeMicroAfterTick;

    // Tick actors
    UpdateActorsPrePhysics(TimeStep);
}

void AWorld::HandlePostPhysics(float TimeStep)
{
    UpdateActorsPostPhysics(TimeStep);

    if (bResetGameplayTimer)
    {
        bResetGameplayTimer        = false;
        GameplayTimeMicroAfterTick = 0;
    }
    else
    {
        GameplayTimeMicroAfterTick += (double)TimeStep * 1000000.0;
    }
}

void AWorld::UpdatePhysics(float TimeStep)
{
    if (bPaused)
    {
        return;
    }

    PhysicsSystem.Simulate(TimeStep);

    E_OnPostPhysicsUpdate.Dispatch(TimeStep);
}

void AWorld::LateUpdate(float TimeStep)
{
    for (AActor* actor : LateUpdateActors)
    {
        if (actor->IsPendingKill())
        {
            continue;
        }

        if (bPaused && !actor->bTickEvenWhenPaused)
        {
            continue;
        }

        actor->CallLateUpdate(TimeStep);
    }
}

void AWorld::Tick(float TimeStep)
{
    GameRunningTimeMicro = GameRunningTimeMicroAfterTick;
    GameplayTimeMicro    = GameplayTimeMicroAfterTick;

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
    GameRunningTimeMicroAfterTick += frameDuration;
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

    if (_Level->OwnerWorld == this)
    {
        // Already in world
        return;
    }

    if (_Level->OwnerWorld)
    {
        _Level->OwnerWorld->RemoveLevel(_Level);
    }

    _Level->OwnerWorld = this;
    _Level->AddRef();
    _Level->OnAddLevelToWorld();
    ArrayOfLevels.Add(_Level);

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

    if (_Level->OwnerWorld != this)
    {
        LOG("AWorld::AddLevel: level is not in world\n");
        return;
    }

    _Level->OnRemoveLevelFromWorld();

    ArrayOfLevels.Remove(ArrayOfLevels.IndexOf(_Level));

    VisibilitySystem.UnregisterLevel(_Level->Visibility);

    _Level->OwnerWorld = nullptr;
    _Level->RemoveRef();
}

void AWorld::RegisterTimer(ATimer* Timer)
{
    if (INTRUSIVE_EXISTS(Timer, NextInWorld, PrevInWorld, TimerList, TimerListTail))
    {
        // Already in the world
        return;
    }

    Timer->AddRef();
    INTRUSIVE_ADD(Timer, NextInWorld, PrevInWorld, TimerList, TimerListTail);
}

void AWorld::UnregisterTimer(ATimer* Timer)
{
    if (!INTRUSIVE_EXISTS(Timer, NextInWorld, PrevInWorld, TimerList, TimerListTail))
    {
        return;
    }

    if (pNextTickingTimer && pNextTickingTimer == Timer)
    {
        pNextTickingTimer = Timer->NextInWorld;
    }
    INTRUSIVE_REMOVE(Timer, NextInWorld, PrevInWorld, TimerList, TimerListTail);

    Timer->RemoveRef();
}

void AWorld::DrawDebug(ADebugRenderer* InRenderer)
{
    VisibilitySystem.DrawDebug(InRenderer);

    for (ALevel* level : ArrayOfLevels)
    {
        level->DrawDebug(InRenderer);
    }

    for (AActor* actor : Actors)
    {
        actor->CallDrawDebug(InRenderer);
    }

    PhysicsSystem.DrawDebug(InRenderer);

    NavigationMesh.DrawDebug(InRenderer);
}

AWorld* AWorld::CreateWorld()
{
    AWorld* world = CreateInstanceOf<AWorld>();

    world->AddRef();

    // Add world to the game
    Worlds.Add(world);

    return world;
}

void AWorld::DestroyWorlds()
{
    for (AWorld* world : Worlds)
    {
        world->Destroy();
    }
}

void AWorld::KillWorlds()
{
    while (PendingKillWorlds)
    {
        AWorld* world = PendingKillWorlds;
        AWorld* nextWorld;

        PendingKillWorlds = nullptr;

        while (world)
        {
            nextWorld = world->NextPendingKillWorld;

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
            Worlds.Remove(Worlds.IndexOf(world));
            if (world->bTicking)
            {
                TickingWorlds.Remove(TickingWorlds.IndexOf(world));
                world->bTicking = false;
            }
            world->RemoveRef();

            world = nextWorld;
        }
    }

    // Static things must be manually freed
    if (Worlds.IsEmpty())
        Worlds.Free();
    if (TickingWorlds.IsEmpty())
        TickingWorlds.Free();
}

void AWorld::UpdateWorlds(float TimeStep)
{
    for (AWorld* world : Worlds)
    {
        if (!world->bTicking)
        {
            world->bTicking = true;
            TickingWorlds.Add(world);
        }
    }

    for (AWorld* world : TickingWorlds)
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
    GlobalEnvironmentMap = EnvironmentMap;
}
