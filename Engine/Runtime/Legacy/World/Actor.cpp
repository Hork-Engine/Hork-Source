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
#if 0
#include "Actor.h"
#include "World.h"

#include <Engine/Core/ConsoleVar.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/Core/Application/Logger.h>

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
    //World::DestroyActor(this);
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
        if (!Core::Strcmp(component->FinalClassName(), _ClassName))
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

void Actor::CallBeginPlay()
{
    BeginPlay();
}

void Actor::CallTick(float TimeStep)
{
    Tick(TimeStep);
}

void Actor::CallTickPrePhysics(float TimeStep)
{
    TickPrePhysics(TimeStep);
}

void Actor::CallTickPostPhysics(float TimeStep)
{
    TickPostPhysics(TimeStep);
}

void Actor::CallLateUpdate(float TimeStep)
{
    LateUpdate(TimeStep);
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
            //Renderer->SetDepthTest(false);
            //Renderer->DrawAxis(m_RootComponent->GetWorldTransformMatrix(), false);
        }
    }

    DrawDebug(Renderer);
}

void Actor::ApplyDamage(ActorDamage const& Damage)
{
    OnApplyDamage(Damage);
}

HK_NAMESPACE_END
#endif
