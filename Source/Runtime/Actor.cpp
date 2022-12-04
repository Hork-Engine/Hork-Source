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

#include "Actor.h"
#include "SceneComponent.h"
#include "World.h"
#include "Timer.h"

#include <Core/ConsoleVar.h>
#include <Core/IntrusiveLinkedListMacro.h>
#include <Platform/Logger.h>

#include <angelscript.h>

AConsoleVar com_DrawRootComponentAxis("com_DrawRootComponentAxis"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(AActor)

static uint32_t UniqueName = 0;

AActor::AActor()
{
    SetObjectName(HK_FORMAT("Actor{}", UniqueName));
    UniqueName++;
}

void AActor::Destroy()
{
    AWorld::DestroyActor(this);
}

AActorComponent* AActor::CreateComponent(uint64_t _ClassId, AStringView InName)
{
    AActorComponent* component = static_cast<AActorComponent*>(AActorComponent::Factory().CreateInstance(_ClassId));
    AddComponent(component, InName);
    return component;
}

AActorComponent* AActor::CreateComponent(const char* _ClassName, AStringView InName)
{
    AActorComponent* component = static_cast<AActorComponent*>(AActorComponent::Factory().CreateInstance(_ClassName));
    AddComponent(component, InName);
    return component;
}

AActorComponent* AActor::CreateComponent(AClassMeta const* _ClassMeta, AStringView InName)
{
    HK_ASSERT(_ClassMeta->Factory() == &AActorComponent::Factory());
    AActorComponent* component = static_cast<AActorComponent*>(_ClassMeta->CreateInstance());
    AddComponent(component, InName);
    return component;
}

void AActor::AddComponent(AActorComponent* Component, AStringView InName)
{
    if (!Component)
        return;

    Component->AddRef();
    Component->SetObjectName(InName);
    Component->ComponentIndex = Components.Size();
    Component->OwnerActor     = this;
    Component->LocalId        = ++ComponentLocalIdGen;

    Components.Add(Component);
}

AActorComponent* AActor::GetComponent(uint64_t _ClassId)
{
    for (AActorComponent* component : Components)
    {
        if (component->FinalClassId() == _ClassId)
        {
            return component;
        }
    }
    return nullptr;
}

AActorComponent* AActor::GetComponent(const char* _ClassName)
{
    for (AActorComponent* component : Components)
    {
        if (!Platform::Strcmp(component->FinalClassName(), _ClassName))
        {
            return component;
        }
    }
    return nullptr;
}

AActorComponent* AActor::GetComponent(AClassMeta const* _ClassMeta)
{
    HK_ASSERT(_ClassMeta->Factory() == &AActorComponent::Factory());
    for (AActorComponent* component : Components)
    {
        if (&component->FinalClassMeta() == _ClassMeta)
        {
            return component;
        }
    }
    return nullptr;
}

#define CALL_SCRIPT(Function)                                              \
    {                                                                      \
        if (ScriptModule)                                                  \
        {                                                                  \
            AActorScript* pScript = AActorScript::GetScript(ScriptModule); \
            pScript->Function(ScriptModule);                               \
        }                                                                  \
    }

#define CALL_SCRIPT_ARG(Function, ...)                                     \
    {                                                                      \
        if (ScriptModule)                                                  \
        {                                                                  \
            AActorScript* pScript = AActorScript::GetScript(ScriptModule); \
            pScript->Function(ScriptModule, __VA_ARGS__);                  \
        }                                                                  \
    }

void AActor::CallBeginPlay()
{
    BeginPlay();

    CALL_SCRIPT(BeginPlay);
}

void AActor::CallTick(float TimeStep)
{
    Tick(TimeStep);

    CALL_SCRIPT_ARG(Tick, TimeStep);
}

void AActor::CallTickPrePhysics(float TimeStep)
{
    TickPrePhysics(TimeStep);

    CALL_SCRIPT_ARG(TickPrePhysics, TimeStep);
}

void AActor::CallTickPostPhysics(float TimeStep)
{
    TickPostPhysics(TimeStep);

    CALL_SCRIPT_ARG(TickPostPhysics, TimeStep);
}

void AActor::CallLateUpdate(float TimeStep)
{
    LateUpdate(TimeStep);

    CALL_SCRIPT_ARG(LateUpdate, TimeStep);
}

void AActor::CallDrawDebug(ADebugRenderer* Renderer)
{
    for (AActorComponent* component : Components)
    {
        component->DrawDebug(Renderer);
    }

    if (com_DrawRootComponentAxis)
    {
        if (RootComponent)
        {
            Renderer->SetDepthTest(false);
            Renderer->DrawAxis(RootComponent->GetWorldTransformMatrix(), false);
        }
    }

    DrawDebug(Renderer);

    CALL_SCRIPT_ARG(DrawDebug, Renderer);
}

void AActor::ApplyDamage(SActorDamage const& Damage)
{
    OnApplyDamage(Damage);

    CALL_SCRIPT_ARG(OnApplyDamage, Damage);
}

asILockableSharedBool* AActor::ScriptGetWeakRefFlag()
{
    if (!pWeakRefFlag)
        pWeakRefFlag = asCreateLockableSharedBool();

    return pWeakRefFlag;
}

bool AActor::SetPublicProperty(AStringView PublicName, AStringView Value)
{
    if (!pActorDef)
        return false;

    auto FindComponent = [this](AClassMeta const* ClassMeta, int ComponentIndex) -> AActorComponent*
    {
        for (AActorComponent* component : GetComponents())
        {
            if (component->FinalClassId() == ClassMeta->ClassId && component->LocalId == ComponentIndex)
                return component;
        }
        return {};
    };

    for (AActorDefinition::SPublicProperty const& prop : pActorDef->GetPublicProperties())
    {
        if (PublicName == prop.PublicName)
        {
            if (prop.ComponentIndex != -1)
            {
                // NOTE: component->LocalId should match ComponentIndex
                AActorComponent* component = FindComponent(pActorDef->GetComponents()[prop.ComponentIndex].ClassMeta, prop.ComponentIndex);
                if (component)
                {
                    return component->SetProperty(prop.PropertyName, Value);
                }
            }
            else
            {
                return SetProperty(prop.PropertyName, Value);
            }
        }
    }

    if (ScriptModule)
    {
        for (AActorDefinition::SScriptPublicProperty const& prop : pActorDef->GetScriptPublicProperties())
        {
            if (prop.PublicName == PublicName)
            {
                return AActorScript::SetProperty(ScriptModule, prop.PropertyName, Value);
            }
        }
    }

    return false;
}

ATimer* AActor::AddTimer(TCallback<void()> const& Callback)
{
    if (bPendingKill)
    {
        LOG("AActor::AddTimer: Attempting to add a timer to a destroyed actor\n");
        return {};
    }

    ATimer* timer = NewObj<ATimer>();
    timer->AddRef();
    timer->Callback = Callback;

    // If an actor is queued to spawn, the timer will be registered after spawning
    if (!bSpawning)
    {
        World->RegisterTimer(timer);
    }

    INTRUSIVE_ADD(timer, NextInActor, PrevInActor, TimerList, TimerListTail);

    return timer;
}

void AActor::RemoveTimer(ATimer* Timer)
{
    if (!Timer)
    {
        LOG("Timer is null\n");
        return;
    }

    if (!INTRUSIVE_EXISTS(Timer, NextInActor, PrevInActor, TimerList, TimerListTail))
    {
        LOG("Timer is not exists\n");
        return;
    }

    if (World)
    {
        World->UnregisterTimer(Timer);
    }

    INTRUSIVE_REMOVE(Timer, NextInActor, PrevInActor, TimerList, TimerListTail);
    Timer->RemoveRef();
}

void AActor::RemoveAllTimers()
{
    for (ATimer* timer = TimerList; timer; timer = timer->NextInActor)
    {
        if (World)
        {
            World->UnregisterTimer(timer);
        }
        timer->RemoveRef();
    }
    TimerList     = {};
    TimerListTail = {};
}
