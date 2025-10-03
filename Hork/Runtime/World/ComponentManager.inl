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

#pragma once

HK_NAMESPACE_BEGIN

HK_FORCEINLINE ComponentTypeID ComponentManagerBase::GetComponentTypeID() const
{
    return m_ComponentTypeID;
}

template <typename ComponentType>
HK_INLINE Handle32<ComponentType> ComponentManagerBase::CreateComponent(GameObject* gameObject, ComponentType*& component)
{
    HK_IF_NOT_ASSERT(m_ComponentTypeID == ComponentRTTR::TypeID<ComponentType>)
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
HK_FIND_METHOD(PhysicsUpdate)
HK_FIND_METHOD(PostTransform)
HK_FIND_METHOD(LateUpdate)
HK_FIND_METHOD(DrawDebug)
HK_FIND_METHOD(OnBeginOverlap)
HK_FIND_METHOD(OnEndOverlap)
HK_FIND_METHOD(OnBeginContact)
HK_FIND_METHOD(OnUpdateContact)
HK_FIND_METHOD(OnEndContact)

template <typename ComponentType>
HK_FORCEINLINE ComponentManager<ComponentType>::ComponentManager(World* world) :
    ComponentManagerBase(world, ComponentRTTR::TypeID<ComponentType>)
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
        TickFunction tickFunc;
        TickGroup_Update::InitializeTickFunction<ComponentType>(tickFunc.Desc);
        tickFunc.Group = TickGroup::Update;
        tickFunc.Delegate.Bind(this, &ComponentManager<ComponentType>::Update);
        tickFunc.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(tickFunc);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, FixedUpdate))
    {
        TickFunction tickFunc;
        TickGroup_FixedUpdate::InitializeTickFunction<ComponentType>(tickFunc.Desc);
        tickFunc.Group = TickGroup::FixedUpdate;
        tickFunc.Delegate.Bind(this, &ComponentManager<ComponentType>::FixedUpdate);
        tickFunc.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(tickFunc);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, PhysicsUpdate))
    {
        TickFunction tickFunc;
        TickGroup_FixedUpdate::InitializeTickFunction<ComponentType>(tickFunc.Desc);
        tickFunc.Group = TickGroup::PhysicsUpdate;
        tickFunc.Delegate.Bind(this, &ComponentManager<ComponentType>::PhysicsUpdate);
        tickFunc.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(tickFunc);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, PostTransform))
    {
        TickFunction tickFunc;
        TickGroup_PostTransform::InitializeTickFunction<ComponentType>(tickFunc.Desc);
        tickFunc.Group = TickGroup::PostTransform;
        tickFunc.Delegate.Bind(this, &ComponentManager<ComponentType>::PostTransform);
        tickFunc.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(tickFunc);
    }

    if constexpr (HK_HAS_METHOD(ComponentType, LateUpdate))
    {
        TickFunction tickFunc;
        TickGroup_LateUpdate::InitializeTickFunction<ComponentType>(tickFunc.Desc);
        tickFunc.Group = TickGroup::LateUpdate;
        tickFunc.Delegate.Bind(this, &ComponentManager<ComponentType>::LateUpdate);
        tickFunc.OwnerTypeID = GetComponentTypeID();

        RegisterTickFunction(tickFunc);
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
HK_FORCEINLINE bool ComponentManager<ComponentType>::IsHandleValid(Handle32<ComponentType> handle) const
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
HK_FORCEINLINE bool ComponentManager<ComponentType>::IsHandleValid(ComponentHandle handle) const
{
    return IsHandleValid(Handle32<ComponentType>(handle));
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
        HK_FORCEINLINE ComponentHandle operator()(ComponentType const* component) const
        {
            return component->GetHandle();
        }
    };
    ComponentType* _movedComponent;
    m_ComponentStorage.DestroyObject(HandleFetcher{}, static_cast<Handle32<ComponentType>>(handle), _movedComponent);
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
HK_INLINE void ComponentManager<ComponentType>::PhysicsUpdate()
{
    if constexpr (HK_HAS_METHOD(ComponentType, PhysicsUpdate))
    {
        struct Visitor
        {
            HK_FORCEINLINE void Visit(ComponentType& component)
            {
                if (component.IsInitialized())
                    component.PhysicsUpdate();
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
HK_INLINE void ComponentManager<ComponentType>::OnBeginContact(ComponentHandle handle, Collision& collision)
{
    if (auto component = GetComponent(Handle32<ComponentType>(handle)))
        component->OnBeginContact(collision);
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::OnUpdateContact(ComponentHandle handle, Collision& collision)
{
    if (auto component = GetComponent(Handle32<ComponentType>(handle)))
        component->OnUpdateContact(collision);
}

template <typename ComponentType>
HK_INLINE void ComponentManager<ComponentType>::OnEndContact(ComponentHandle handle, class BodyComponent* body)
{
    if (auto component = GetComponent(Handle32<ComponentType>(handle)))
        component->OnEndContact(body);
}

template <typename ComponentType>
HK_FORCEINLINE ComponentType* Component::sUpcast(Component* component)
{
    if (component->GetManager()->GetComponentTypeID() == ComponentRTTR::TypeID<ComponentType>)
        return static_cast<ComponentType*>(component);
    return nullptr;
}

HK_NAMESPACE_END
