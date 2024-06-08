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

#include "GameObject.h"
#include "World.h"

#include <Engine/Core/Logger.h>

HK_NAMESPACE_BEGIN

void GameObject::SetDynamic(bool dynamic)
{
    if (IsDynamic() == dynamic)
        return;

    if (!dynamic)
    {
        GameObject* parent = GetParent();
        if (parent && parent->IsDynamic())
        {
            LOG("Can't make this object static because the parent is dynamic.\n");
            return;
        }

        for (Component* component : m_Components)
        {
            if (component->IsDynamic())
            {
                LOG("Can't make this object static because it has dynamic components.\n");
                return;
            }
        }
    }

    m_Flags.IsDynamic = dynamic;

    m_World->UpdateHierarchyData(this, !dynamic);

    if (dynamic)
    {
        for (auto it = GetChildren(); it.IsValid(); ++it)
        {
            it->SetDynamic(true);
        }
    }
}

void GameObject::SetLockWorldPositionAndRotation(bool lock)
{
    m_TransformData->LockWorldPositionAndRotation = lock;
}

void GameObject::AddComponent(Component* component)
{
    HK_ASSERT(component->m_Owner == nullptr);

    component->m_Owner = this;
    m_Components.Add(component);
}

void GameObject::RemoveComponent(Component* component)
{
    HK_ASSERT(component->m_Owner == this);

    auto index = m_Components.IndexOf(component);
    HK_IF_ASSERT(index != Core::NPOS)
    {
        m_Components.RemoveUnsorted(index);
    }
    component->m_Owner = nullptr;
}

void GameObject::PatchComponentPointer(Component* oldPointer, Component* newPointer)
{
    auto index = m_Components.IndexOf(oldPointer);
    HK_IF_ASSERT(index != Core::NPOS)
    {
        m_Components[index] = newPointer;
    }
}

Component* GameObject::GetComponent(ComponentTypeID id)
{
    for (auto* component : m_Components)
    {
        if (component->GetManager()->GetComponentTypeID() == id)
            return component;
    }
    return nullptr;
}

void GameObject::GetAllComponents(ComponentTypeID id, TVector<Component*>& components)
{
    for (auto* component : m_Components)
    {
        if (component->GetManager()->GetComponentTypeID() == id)
            components.Add(component);
    }
}

void GameObject::SetParent(GameObjectHandle handle, TransformRule transformRule)
{
    m_World->SetParent(this, m_World->GetObject(handle), transformRule);
}

void GameObject::SetParent(GameObject* parent, TransformRule transformRule)
{
    HK_IF_NOT_ASSERT(!parent || (parent && parent->GetWorld() == GetWorld()))
    {
        return;
    }

    m_World->SetParent(this, parent, transformRule);
}

GameObject* GameObject::GetParent()
{
    return m_World->GetObjectUnsafe(m_Parent);
}

void GameObject::LinkToParent()
{
    HK_ASSERT(!m_NextSibling && !m_PrevSibling);

    if (GameObject* parent = GetParent())
    {
        if (parent->m_FirstChild)
        {
            m_PrevSibling = parent->m_LastChild;
            m_World->GetObjectUnsafe(parent->m_LastChild)->m_NextSibling = m_Handle;
        }
        else
        {
            parent->m_FirstChild = m_Handle;
        }

        parent->m_LastChild = m_Handle;
        parent->m_ChildCount++;

        m_TransformData->Parent = parent->m_TransformData;
    }
}

void GameObject::UnlinkFromParent()
{
    if (GameObject* parent = GetParent())
    {
        if (m_Handle == parent->m_FirstChild)
            parent->m_FirstChild = m_NextSibling;

        if (m_Handle == parent->m_LastChild)
            parent->m_LastChild = m_PrevSibling;

        if (GameObject* next = m_World->GetObjectUnsafe(m_NextSibling))
            next->m_PrevSibling = m_PrevSibling;

        if (GameObject* prev = m_World->GetObjectUnsafe(m_PrevSibling))
            prev->m_NextSibling = m_NextSibling;

        parent->m_ChildCount--;
        m_Parent = {};

        m_TransformData->Parent = nullptr;
    }
}

GameObject::ChildIterator GameObject::GetChildren()
{
    World* world = GetWorld();
    return ChildIterator(world->GetObjectUnsafe(m_FirstChild), world);
}

GameObject::ConstChildIterator GameObject::GetChildren() const
{
    World const* world = GetWorld();
    return ConstChildIterator(world->GetObjectUnsafe(m_FirstChild), world);
}

void GameObject::UpdateWorldTransform()
{
    m_TransformData->UpdateWorldTransform_r();
}

void GameObject::ChildIterator::operator++()
{
    m_Object = m_Object->GetWorld()->GetObjectUnsafe(m_Object->m_NextSibling);
}

HK_NAMESPACE_END
