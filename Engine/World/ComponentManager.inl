#pragma once

HK_NAMESPACE_BEGIN

HK_FORCEINLINE ComponentManagerBase::ComponentManagerBase(World* world, ComponentTypeID componentTypeID) :
    m_World(world), m_ComponentTypeID(componentTypeID)
{}

HK_FORCEINLINE ComponentTypeID ComponentManagerBase::GetComponentTypeID() const
{
    return m_ComponentTypeID;
}

template <typename ComponentType>
HK_INLINE Handle32<ComponentType> ComponentManagerBase::CreateComponent(GameObject* gameObject, ComponentType*& component)
{
    HK_IF_NOT_ASSERT(m_ComponentTypeID == ComponentTypeRegistry::GetComponentTypeID<ComponentType>())
    {
        component = nullptr;
        return {};
    }

    component = static_cast<ComponentType*>(CreateComponentInternal(gameObject, ComponentType::Mode));
    return static_cast<Handle32<ComponentType>>(component->GetHandle());
}

HK_FORCEINLINE World* ComponentManagerBase::GetWorld()
{
    return m_World;
}

HK_FORCEINLINE World const* ComponentManagerBase::GetWorld() const
{
    return m_World;
}

HK_FORCEINLINE void ComponentManagerBase::InvokeBeginPlay(Component* component)
{
    m_OnBeginPlay.Invoke(component);
}

HK_FORCEINLINE void ComponentManagerBase::InvokeEndPlay(Component* component)
{
    m_OnEndPlay.Invoke(component);
}

HK_FIND_METHOD(BeginPlay)
HK_FIND_METHOD(EndPlay)
HK_FIND_METHOD(Update)
HK_FIND_METHOD(FixedUpdate)
HK_FIND_METHOD(PostTransform)
HK_FIND_METHOD(LateUpdate)
HK_FIND_METHOD(DrawDebug)
HK_FIND_METHOD(OnBeginOverlap)
HK_FIND_METHOD(OnEndOverlap)

template <typename ComponentType>
HK_FORCEINLINE ComponentManager<ComponentType>::ComponentManager(World* world) :
    ComponentManagerBase(world, ComponentTypeRegistry::GetComponentTypeID<ComponentType>())
{
    if constexpr (HK_HAS_METHOD(ComponentType, BeginPlay))
    {
        m_OnBeginPlay.Bind(this, &ComponentManager<ComponentType>::BeginPlay);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, EndPlay))
    {
        m_OnEndPlay.Bind(this, &ComponentManager<ComponentType>::EndPlay);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, Update))
    {
        TickFunction f;
        TickGroup_Update::InitializeTickFunction<ComponentType>(f.Desc);
        f.Group = TickGroup::Update;
        f.Delegate.Bind(this, &ComponentManager<ComponentType>::Update);
        f.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(f);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, FixedUpdate))
    {
        TickFunction f;
        TickGroup_FixedUpdate::InitializeTickFunction<ComponentType>(f.Desc);
        f.Group = TickGroup::FixedUpdate;
        f.Delegate.Bind(this, &ComponentManager<ComponentType>::FixedUpdate);
        f.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(f);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, PostTransform))
    {
        TickFunction f;
        TickGroup_PostTransform::InitializeTickFunction<ComponentType>(f.Desc);
        f.Group = TickGroup::PostTransform;
        f.Delegate.Bind(this, &ComponentManager<ComponentType>::PostTransform);
        f.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(f);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, LateUpdate))
    {
        TickFunction f;
        TickGroup_LateUpdate::InitializeTickFunction<ComponentType>(f.Desc);
        f.Group = TickGroup::LateUpdate;
        f.Delegate.Bind(this, &ComponentManager<ComponentType>::LateUpdate);
        f.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(f);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, DrawDebug))
    {
        Delegate<void(DebugRenderer&)> function;
        function.Bind(this, &ComponentManager<ComponentType>::DrawDebug);

        RegisterDebugDrawFunction(function);
    }
}

template <typename ComponentType>
HK_FORCEINLINE Handle32<ComponentType> ComponentManager<ComponentType>::CreateComponent(GameObject* gameObject)
{
    ComponentType* component;
    return CreateComponent(gameObject, component);
}

template <typename ComponentType>
HK_FORCEINLINE Handle32<ComponentType> ComponentManager<ComponentType>::CreateComponent(GameObject* gameObject, ComponentType*& component)
{
    return ComponentManagerBase::CreateComponent<ComponentType>(gameObject, component);
}

template <typename ComponentType>
HK_FORCEINLINE void ComponentManager<ComponentType>::DestroyComponent(Handle32<ComponentType> handle)
{
    if (auto component = GetComponent(handle))
        ComponentManagerBase::DestroyComponent(component);
}

template <typename ComponentType>
HK_FORCEINLINE bool ComponentManager<ComponentType>::IsHandleValid(Handle32<ComponentType> handle)
{
    // TODO: Move to ObjectStorage?

    auto id = handle.GetID();

    auto& components = m_ComponentStorage.GetRandomAccessTable();

    // Check garbage
    if (id >= components.Size())
        return false;

    // Check is freed
    if (components[id] == nullptr)
        return false;

    // Check version / id
    return components[id]->GetHandle().ToUInt32() == handle.ToUInt32();
}

template <typename ComponentType>
HK_FORCEINLINE Component* ComponentManager<ComponentType>::GetComponent(ComponentHandle handle)
{
    return GetComponent(Handle32<ComponentType>(handle));
}

template <typename ComponentType>
HK_FORCEINLINE Component* ComponentManager<ComponentType>::GetComponentUnsafe(ComponentHandle handle)
{
    return GetComponentUnsafe(Handle32<ComponentType>(handle));
}

template <typename ComponentType>
HK_FORCEINLINE ComponentType* ComponentManager<ComponentType>::GetComponent(Handle32<ComponentType> handle)
{
    return IsHandleValid(handle) ? m_ComponentStorage.GetObject(handle) : nullptr;
}

template <typename ComponentType>
ComponentType* ComponentManager<ComponentType>::GetComponentUnsafe(Handle32<ComponentType> handle)
{
    return m_ComponentStorage.GetObject(handle);
}

template <typename ComponentType>
HK_FORCEINLINE uint32_t ComponentManager<ComponentType>::GetComponentCount() const
{
    return m_ComponentStorage.Size();
}

template <typename ComponentType>
HK_FORCEINLINE typename ComponentManager<ComponentType>::Iterator ComponentManager<ComponentType>::GetComponents()
{
    return m_ComponentStorage.GetObjects();
}

template <typename ComponentType>
HK_FORCEINLINE typename ComponentManager<ComponentType>::ConstIterator ComponentManager<ComponentType>::GetComponents() const
{
    return m_ComponentStorage.GetObjects();
}

template <typename ComponentType>
template <typename Visitor>
HK_FORCEINLINE void ComponentManager<ComponentType>::IterateComponents(Visitor& visitor)
{
    m_ComponentStorage.Iterate(visitor);
}

template <typename ComponentType>
template <typename Visitor>
HK_FORCEINLINE void ComponentManager<ComponentType>::IterateComponentBatches(Visitor& visitor)
{
    m_ComponentStorage.IterateBatches(visitor);
}

template <typename ComponentType>
HK_FORCEINLINE ComponentHandle ComponentManager<ComponentType>::ConstructComponent(Component*& component)
{
    ComponentType* _component;
    auto handle = m_ComponentStorage.CreateObject(_component);
    component = _component;
    return static_cast<ComponentHandle>(handle);
}

template <typename ComponentType>
HK_FORCEINLINE void ComponentManager<ComponentType>::DestructComponent(ComponentHandle handle, Component*& movedComponent)
{
    struct HandleFetcher
    {
        HK_FORCEINLINE static ComponentHandle FetchHandle(ComponentType* component)
        {
            return component->GetHandle();
        }
    };
    ComponentType* _movedComponent;
    m_ComponentStorage.DestroyObject<HandleFetcher>(static_cast<Handle32<ComponentType>>(handle), _movedComponent);
    movedComponent = _movedComponent;
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::BeginPlay(Component* component)
{
    if constexpr (HK_HAS_METHOD(ComponentType, BeginPlay))
    {
        static_cast<ComponentType*>(component)->BeginPlay();
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::EndPlay(Component* component)
{
    if constexpr (HK_HAS_METHOD(ComponentType, EndPlay))
    {
        static_cast<ComponentType*>(component)->EndPlay();
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::Update()
{
    if constexpr (HK_HAS_METHOD(ComponentType, Update))
    {
        struct Visitor
        {
            HK_FORCEINLINE void Visit(ComponentType& component)
            {
                if (component.IsInitialized())
                    component.Update();
            }
        };

        Visitor visitor;
        m_ComponentStorage.Iterate(visitor);
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::FixedUpdate()
{
    if constexpr (HK_HAS_METHOD(ComponentType, FixedUpdate))
    {
        struct Visitor
        {
            HK_FORCEINLINE void Visit(ComponentType& component)
            {
                if (component.IsInitialized())
                    component.FixedUpdate();
            }
        };

        Visitor visitor;
        m_ComponentStorage.Iterate(visitor);
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::PostTransform()
{
    if constexpr (HK_HAS_METHOD(ComponentType, PostTransform))
    {
        struct Visitor
        {
            HK_FORCEINLINE void Visit(ComponentType& component)
            {
                if (component.IsInitialized())
                    component.PostTransform();
            }
        };

        Visitor visitor;
        m_ComponentStorage.Iterate(visitor);
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::LateUpdate()
{
    if constexpr (HK_HAS_METHOD(ComponentType, LateUpdate))
    {
        struct Visitor
        {
            HK_FORCEINLINE void Visit(ComponentType& component)
            {
                if (component.IsInitialized())
                    component.LateUpdate();
            }
        };

        Visitor visitor;
        m_ComponentStorage.Iterate(visitor);
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::DrawDebug(DebugRenderer& renderer)
{
    if constexpr (HK_HAS_METHOD(ComponentType, DrawDebug))
    {
        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;

            Visitor(DebugRenderer& renderer) :
                m_DebugRenderer(renderer)
            {}

            HK_FORCEINLINE void Visit(ComponentType& component)
            {
                if (component.IsInitialized())
                    component.DrawDebug(m_DebugRenderer);
            }
        };

        Visitor visitor(renderer);
        m_ComponentStorage.Iterate(visitor);
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::OnBeginOverlap(ComponentHandle handle, class BodyComponent* body)
{
    if (auto component = GetComponent(Handle32<ComponentType>(handle)))
        component->OnBeginOverlap(body);
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::OnEndOverlap(ComponentHandle handle, class BodyComponent* body)
{
    if (auto component = GetComponent(Handle32<ComponentType>(handle)))
        component->OnEndOverlap(body);
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::SubscribeEvents(Component* component)
{
    if constexpr (HK_HAS_METHOD(ComponentType, OnBeginOverlap))
    {
        World::SubscribeEvent<Event_OnBeginOverlap>(component->GetOwner(), component, {this, &ComponentManager<ComponentType>::OnBeginOverlap});
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnEndOverlap))
    {
        World::SubscribeEvent<Event_OnEndOverlap>(component->GetOwner(), component, {this, &ComponentManager<ComponentType>::OnEndOverlap});
    }
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::UnsubscribeEvents(Component* component)
{
    if constexpr (HK_HAS_METHOD(ComponentType, OnBeginOverlap))
    {
        World::UnsubscribeEvent<Event_OnBeginOverlap>(component->GetOwner(), component);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, OnEndOverlap))
    {
        World::UnsubscribeEvent<Event_OnEndOverlap>(component->GetOwner(), component);
    }
}

HK_NAMESPACE_END
