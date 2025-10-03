/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

HK_NAMESPACE_BEGIN

template <typename ComponentType>
HK_INLINE ComponentManager<ComponentType>& World::GetComponentManager()
{
    ComponentTypeID id = ComponentRTTR::TypeID<ComponentType>;

    if (!m_ComponentManagers[id])
        m_ComponentManagers[id] = new ComponentManager<ComponentType>(this);

    return *static_cast<ComponentManager<ComponentType>*>(m_ComponentManagers[id]);
}

HK_FORCEINLINE ComponentManagerBase* World::TryGetComponentManager(ComponentTypeID typeID)
{
    HK_ASSERT(typeID < ComponentRTTR::GetTypesCount());
    return m_ComponentManagers[typeID];
}

template <typename ComponentType>
HK_FORCEINLINE ComponentType* World::GetComponent(Handle32<ComponentType> componentHandle)
{
    return GetComponentManager<ComponentType>().GetComponent(componentHandle);
}

template <typename ComponentType>
HK_FORCEINLINE ComponentType* World::GetComponent(ComponentExtendedHandle componentHandle)
{
    if (ComponentRTTR::TypeID<ComponentType> != componentHandle.TypeID)
        return nullptr;
    return GetComponent(Handle32<ComponentType>(componentHandle.Handle));
}

template <typename Interface>
HK_INLINE Interface& World::GetInterface()
{
    InterfaceTypeID id = InterfaceRTTR::TypeID<Interface>;

    if (!m_Interfaces[id])
    {
        m_Interfaces[id] = new Interface;
        InitializeInterface(id);
    }

    return *static_cast<Interface*>(m_Interfaces[id]);
}

template <typename Event>
HK_INLINE void World::sSubscribeEvent(GameObject* eventSender, Component* receiver, typename Event::Holder::DelegateType delegate)
{
    typename Event::Holder* eventHolder = eventSender->GetWorld()->GetEventHolder<Event>();
    eventHolder->Add(eventSender, receiver, delegate);
}

template <typename Event>
HK_INLINE void World::sUnsubscribeEvent(GameObject* eventSender, Component* receiver)
{
    typename Event::Holder* eventHolder = eventSender->GetWorld()->GetEventHolder<Event>();
    eventHolder->Remove(eventSender, receiver);
}

template <typename Event, typename... Args>
HK_INLINE void World::sDispatchEvent(GameObject* eventSender, Args... args)
{
    typename Event::Holder* eventHolder = eventSender->GetWorld()->GetEventHolder<Event>();
    eventHolder->Dispatch(eventSender, args...);
}

template <typename Event>
HK_INLINE typename Event::Holder* World::GetEventHolder()
{
    auto typeID = WorldEventRTTR::TypeID<Event>;
    if (!m_EventHolders[typeID])
        m_EventHolders[typeID] = new typename Event::Holder;
    return static_cast<typename Event::Holder*>(m_EventHolders[typeID]);
}

template <typename ComponentType>
HK_FORCEINLINE Handle32<ComponentType> GameObject::CreateComponent()
{
    return m_World->GetComponentManager<ComponentType>().CreateComponent(this);
}

template <typename ComponentType>
HK_FORCEINLINE Handle32<ComponentType> GameObject::CreateComponent(ComponentType*& component)
{
    return m_World->GetComponentManager<ComponentType>().CreateComponent(this, component);
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::SubscribeEvents(Component* component)
{
    if constexpr (HK_HAS_METHOD(ComponentType, OnBeginOverlap))
    {
        World::sSubscribeEvent<Event_OnBeginOverlap>(component->GetOwner(), component, {this, &ComponentManager<ComponentType>::OnBeginOverlap});
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnEndOverlap))
    {
        World::sSubscribeEvent<Event_OnEndOverlap>(component->GetOwner(), component, {this, &ComponentManager<ComponentType>::OnEndOverlap});
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnBeginContact))
    {
        World::sSubscribeEvent<Event_OnBeginContact>(component->GetOwner(), component, {this, &ComponentManager<ComponentType>::OnBeginContact});
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnUpdateContact))
    {
        World::sSubscribeEvent<Event_OnUpdateContact>(component->GetOwner(), component, {this, &ComponentManager<ComponentType>::OnUpdateContact});
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnEndContact))
    {
        World::sSubscribeEvent<Event_OnEndContact>(component->GetOwner(), component, {this, &ComponentManager<ComponentType>::OnEndContact});
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::UnsubscribeEvents(Component* component)
{
    if constexpr (HK_HAS_METHOD(ComponentType, OnBeginOverlap))
    {
        World::sUnsubscribeEvent<Event_OnBeginOverlap>(component->GetOwner(), component);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnEndOverlap))
    {
        World::sUnsubscribeEvent<Event_OnEndOverlap>(component->GetOwner(), component);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnBeginContact))
    {
        World::sUnsubscribeEvent<Event_OnBeginContact>(component->GetOwner(), component);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnUpdateContact))
    {
        World::sUnsubscribeEvent<Event_OnUpdateContact>(component->GetOwner(), component);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnEndContact))
    {
        World::sUnsubscribeEvent<Event_OnEndContact>(component->GetOwner(), component);
    }
}

HK_NAMESPACE_END
