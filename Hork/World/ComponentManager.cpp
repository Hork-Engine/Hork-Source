/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "ComponentManager.h"
#include "World.h"
#include "Component.h"

HK_NAMESPACE_BEGIN

ComponentManagerBase::ComponentManagerBase(World* world, ComponentTypeID componentTypeID) :
    m_World(world), m_ComponentTypeID(componentTypeID)
{}

void ComponentManagerBase::RegisterTickFunction(TickFunction const& tickFunc)
{
    m_World->RegisterTickFunction(tickFunc);
}

void ComponentManagerBase::RegisterDebugDrawFunction(Delegate<void(DebugRenderer& renderer)> const& function)
{
    m_World->RegisterDebugDrawFunction(function);
}

void ComponentManagerBase::InitializeComponent(Component* component)
{
    HK_ASSERT(!component->m_Flags.IsInitialized);

    InvokeBeginPlay(component);
    SubscribeEvents(component);

    component->m_Flags.IsInitialized = true;
}

void ComponentManagerBase::DeinitializeComponent(Component* component)
{
    if (!component->IsInitialized())
        return;

    InvokeEndPlay(component);
    UnsubscribeEvents(component);

    component->m_Flags.IsInitialized = false;
}

Component* ComponentManagerBase::CreateComponentInternal(GameObject* gameObject, ComponentMode componentMode)
{
    HK_IF_NOT_ASSERT(gameObject && !gameObject->m_Flags.IsDestroyed) { return nullptr; }
    HK_IF_NOT_ASSERT(gameObject->GetWorld() == m_World) { return nullptr; }

    if (gameObject->IsStatic() && componentMode != ComponentMode::Static)
        gameObject->SetDynamic(true);

    Component* component;

    auto handle = ConstructComponent(component);

    component->m_Handle = handle;
    component->m_Manager = this;

    if (componentMode == ComponentMode::Dynamic)
        component->m_Flags.IsDynamic = true;

    gameObject->AddComponent(component);

    m_World->m_ComponentsToInitialize.EmplaceBack(handle, m_ComponentTypeID);

    return component;
}

void ComponentManagerBase::DestroyComponent(Component* component)
{
    HK_IF_NOT_ASSERT(component) { return; }
    HK_IF_NOT_ASSERT(component->GetManager() == this) { return; }

    DeinitializeComponent(component);

    if (GameObject* owner = component->GetOwner())
    {
        owner->RemoveComponent(component);
    }

    // TODO: Когда будет гарантировано ранее, что компонент уже в списке, будем использовать метод Add,
    // а пока вызываем SortedInsert, чтобы в списке компоненты не повторялись.
    m_World->m_ComponentsToDelete.SortedInsert(component);
}


HK_NAMESPACE_END
