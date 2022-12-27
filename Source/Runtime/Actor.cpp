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

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawRootComponentAxis("com_DrawRootComponentAxis"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(Actor)

static uint32_t UniqueName = 0;

Actor::Actor()
{
    SetObjectName(HK_FORMAT("Actor{}", UniqueName));
    UniqueName++;
}

void Actor::Destroy()
{
    World::DestroyActor(this);
}

void Actor::SetRootComponent(SceneComponent* rootComponent)
{
    m_RootComponent = rootComponent;
}

ActorComponent* Actor::CreateComponent(uint64_t _ClassId, StringView InName)
{
    ActorComponent* component = static_cast<ActorComponent*>(ActorComponent::Factory().CreateInstance(_ClassId));
    AddComponent(component, InName);
    return component;
}

ActorComponent* Actor::CreateComponent(const char* _ClassName, StringView InName)
{
    ActorComponent* component = static_cast<ActorComponent*>(ActorComponent::Factory().CreateInstance(_ClassName));
    AddComponent(component, InName);
    return component;
}

ActorComponent* Actor::CreateComponent(ClassMeta const* _ClassMeta, StringView InName)
{
    HK_ASSERT(_ClassMeta->Factory() == &ActorComponent::Factory());
    ActorComponent* component = static_cast<ActorComponent*>(_ClassMeta->CreateInstance());
    AddComponent(component, InName);
    return component;
}

void Actor::AddComponent(ActorComponent* Component, StringView InName)
{
    if (!Component)
        return;

    Component->AddRef();
    Component->SetObjectName(InName);
    Component->m_ComponentIndex = m_Components.Size();
    Component->m_OwnerActor     = this;
    Component->m_LocalId        = ++m_ComponentLocalIdGen;

    m_Components.Add(Component);
}

ActorComponent* Actor::GetComponent(uint64_t _ClassId)
{
    for (ActorComponent* component : m_Components)
    {
        if (component->FinalClassId() == _ClassId)
        {
            return component;
        }
    }
    return nullptr;
}

ActorComponent* Actor::GetComponent(const char* _ClassName)
{
    for (ActorComponent* component : m_Components)
    {
        if (!Platform::Strcmp(component->FinalClassName(), _ClassName))
        {
            return component;
        }
    }
    return nullptr;
}

ActorComponent* Actor::GetComponent(ClassMeta const* _ClassMeta)
{
    HK_ASSERT(_ClassMeta->Factory() == &ActorComponent::Factory());
    for (ActorComponent* component : m_Components)
    {
        if (&component->FinalClassMeta() == _ClassMeta)
        {
            return component;
        }
    }
    return nullptr;
}

#define CALL_SCRIPT(Function)                                                \
    {                                                                        \
        if (m_ScriptModule)                                                  \
        {                                                                    \
            ActorScript* pScript = ActorScript::GetScript(m_ScriptModule); \
            pScript->Function(m_ScriptModule);                               \
        }                                                                    \
    }

#define CALL_SCRIPT_ARG(Function, ...)                                       \
    {                                                                        \
        if (m_ScriptModule)                                                  \
        {                                                                    \
            ActorScript* pScript = ActorScript::GetScript(m_ScriptModule); \
            pScript->Function(m_ScriptModule, __VA_ARGS__);                  \
        }                                                                    \
    }

void Actor::CallBeginPlay()
{
    BeginPlay();

    CALL_SCRIPT(BeginPlay);
}

void Actor::CallTick(float TimeStep)
{
    Tick(TimeStep);

    CALL_SCRIPT_ARG(Tick, TimeStep);
}

void Actor::CallTickPrePhysics(float TimeStep)
{
    TickPrePhysics(TimeStep);

    CALL_SCRIPT_ARG(TickPrePhysics, TimeStep);
}

void Actor::CallTickPostPhysics(float TimeStep)
{
    TickPostPhysics(TimeStep);

    CALL_SCRIPT_ARG(TickPostPhysics, TimeStep);
}

void Actor::CallLateUpdate(float TimeStep)
{
    LateUpdate(TimeStep);

    CALL_SCRIPT_ARG(LateUpdate, TimeStep);
}

void Actor::CallDrawDebug(DebugRenderer* Renderer)
{
    for (ActorComponent* component : m_Components)
    {
        component->DrawDebug(Renderer);
    }

    if (com_DrawRootComponentAxis)
    {
        if (m_RootComponent)
        {
            Renderer->SetDepthTest(false);
            Renderer->DrawAxis(m_RootComponent->GetWorldTransformMatrix(), false);
        }
    }

    DrawDebug(Renderer);

    CALL_SCRIPT_ARG(DrawDebug, Renderer);
}

void Actor::ApplyDamage(ActorDamage const& Damage)
{
    OnApplyDamage(Damage);

    CALL_SCRIPT_ARG(OnApplyDamage, Damage);
}

asILockableSharedBool* Actor::ScriptGetWeakRefFlag()
{
    if (!m_pWeakRefFlag)
        m_pWeakRefFlag = asCreateLockableSharedBool();

    return m_pWeakRefFlag;
}

bool Actor::SetPublicProperty(StringView PublicName, StringView Value)
{
    if (!m_pActorDef)
        return false;

    auto FindComponent = [this](ClassMeta const* ClassMeta, int ComponentIndex) -> ActorComponent*
    {
        for (ActorComponent* component : GetComponents())
        {
            if (component->FinalClassId() == ClassMeta->ClassId && component->m_LocalId == ComponentIndex)
                return component;
        }
        return {};
    };

    for (ActorDefinition::PublicProperty const& prop : m_pActorDef->GetPublicProperties())
    {
        if (PublicName == prop.PublicName)
        {
            if (prop.ComponentIndex != -1)
            {
                // NOTE: component->LocalId should match ComponentIndex
                ActorComponent* component = FindComponent(m_pActorDef->GetComponents()[prop.ComponentIndex].ClassMeta, prop.ComponentIndex);
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

    if (m_ScriptModule)
    {
        for (ActorDefinition::ScriptPublicProperty const& prop : m_pActorDef->GetScriptPublicProperties())
        {
            if (prop.PublicName == PublicName)
            {
                return ActorScript::SetProperty(m_ScriptModule, prop.PropertyName, Value);
            }
        }
    }

    return false;
}

WorldTimer* Actor::AddTimer(TCallback<void()> const& Callback)
{
    if (m_bPendingKill)
    {
        LOG("Actor::AddTimer: Attempting to add a timer to a destroyed actor\n");
        return {};
    }

    WorldTimer* timer = NewObj<WorldTimer>();
    timer->AddRef();
    timer->Callback = Callback;

    // If an actor is queued to spawn, the timer will be registered after spawning
    if (!m_bSpawning)
    {
        m_World->RegisterTimer(timer);
    }

    INTRUSIVE_ADD(timer, m_NextInActor, m_PrevInActor, m_TimerList, m_TimerListTail);

    return timer;
}

void Actor::RemoveTimer(WorldTimer* Timer)
{
    if (!Timer)
    {
        LOG("Timer is null\n");
        return;
    }

    if (!INTRUSIVE_EXISTS(Timer, m_NextInActor, m_PrevInActor, m_TimerList, m_TimerListTail))
    {
        LOG("Timer is not exists\n");
        return;
    }

    if (m_World)
    {
        m_World->UnregisterTimer(Timer);
    }

    INTRUSIVE_REMOVE(Timer, m_NextInActor, m_PrevInActor, m_TimerList, m_TimerListTail);
    Timer->RemoveRef();
}

void Actor::RemoveAllTimers()
{
    for (WorldTimer* timer = m_TimerList; timer; timer = timer->m_NextInActor)
    {
        if (m_World)
        {
            m_World->UnregisterTimer(timer);
        }
        timer->RemoveRef();
    }
    m_TimerList     = {};
    m_TimerListTail = {};
}

HK_NAMESPACE_END
