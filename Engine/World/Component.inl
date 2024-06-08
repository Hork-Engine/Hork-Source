HK_NAMESPACE_BEGIN

HK_FORCEINLINE ComponentHandle Component::GetHandle() const
{
    return m_Handle;
}

HK_FORCEINLINE GameObject* Component::GetOwner()
{
    return m_Owner;
}

HK_FORCEINLINE GameObject const* Component::GetOwner() const
{
    return m_Owner;
}

HK_FORCEINLINE ComponentManagerBase* Component::GetManager()
{
    return m_Manager;
}

HK_FORCEINLINE bool Component::IsDynamic() const
{
    return m_Flags.IsDynamic;
}

HK_FORCEINLINE bool Component::IsInitialized() const
{
    return m_Flags.IsInitialized;
}

template <typename ComponentType>
HK_FORCEINLINE ComponentType* Component::Upcast(Component* component)
{
    if (component->GetManager()->GetComponentTypeID() == ComponentTypeRegistry::GetComponentTypeID<ComponentType>())
        return static_cast<ComponentType*>(component);
    return nullptr;
}

HK_NAMESPACE_END
