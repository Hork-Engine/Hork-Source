HK_NAMESPACE_BEGIN

template <typename ComponentType>
HK_INLINE ComponentManager<ComponentType>& World::GetComponentManager()
{
    ComponentTypeID id = ComponentTypeRegistry::GetComponentTypeID<ComponentType>();

    if (!m_ComponentManagers[id])
        m_ComponentManagers[id] = new ComponentManager<ComponentType>(this);

    return *static_cast<ComponentManager<ComponentType>*>(m_ComponentManagers[id]);
}

HK_FORCEINLINE ComponentManagerBase* World::TryGetComponentManager(ComponentTypeID typeID)
{
    HK_ASSERT(typeID < ComponentTypeRegistry::GetComponentTypesCount());
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
    if (ComponentTypeRegistry::GetComponentTypeID<ComponentType>() != componentHandle.TypeID)
        return nullptr;
    return GetComponent(Handle32<ComponentType>(componentHandle.Handle));
}

template <typename Interface>
HK_INLINE Interface& World::GetInterface()
{
    InterfaceTypeID id = InterfaceTypeRegistry::GetInterfaceTypeID<Interface>();

    if (!m_Interfaces[id])
    {
        m_Interfaces[id] = new Interface;
        InitializeInterface(id);
    }

    return *static_cast<Interface*>(m_Interfaces[id]);
}

template <typename Event>
HK_INLINE void World::SubscribeEvent(GameObject* eventSender, Component* receiver, typename Event::Holder::DelegateType delegate)
{
    Event::Holder* eventHolder = eventSender->GetWorld()->GetEventHolder<Event>();
    eventHolder->Add(eventSender, receiver, delegate);
}

template <typename Event>
HK_INLINE void World::UnsubscribeEvent(GameObject* eventSender, Component* receiver)
{
    Event::Holder* eventHolder = eventSender->GetWorld()->GetEventHolder<Event>();
    eventHolder->Remove(eventSender, receiver);
}

template <typename Event, typename... Args>
HK_INLINE void World::DispatchEvent(GameObject* eventSender, Args... args)
{
    typename Event::Holder* eventHolder = eventSender->GetWorld()->GetEventHolder<Event>();
    eventHolder->Dispatch(eventSender, args...);
}

template <typename Event>
HK_INLINE typename Event::Holder* World::GetEventHolder()
{
    auto typeID = WorldEvent::TypeID<Event>;
    if (!m_EventHolders[typeID])
        m_EventHolders[typeID] = new Event::Holder;
    return static_cast<Event::Holder*>(m_EventHolders[typeID]);
}

HK_NAMESPACE_END
