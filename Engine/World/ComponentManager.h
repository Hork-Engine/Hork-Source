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

#pragma once

#include "Component.h"
#include "TickFunction.h"

#include <Engine/Core/Delegate.h>
#include <Engine/Core/Containers/ObjectStorage.h>

HK_NAMESPACE_BEGIN

class World;
class GameObject;
class DebugRenderer;

class ComponentManagerBase : public Noncopyable
{
    friend class World;

public:
    ComponentTypeID         GetComponentTypeID() const;

    template <typename ComponentType>
    Handle32<ComponentType> CreateComponent(GameObject* gameObject, ComponentType*& component);

    void                    DestroyComponent(Component* component);

    virtual Component*      GetComponent(ComponentHandle handle) = 0;
    virtual Component*      GetComponentUnsafe(ComponentHandle handle) = 0;

    World*                  GetWorld();
    World const*            GetWorld() const;

protected:
                            ComponentManagerBase(World* world, ComponentTypeID componentTypeID);
    virtual                 ~ComponentManagerBase() {}

    Component*              CreateComponentInternal(GameObject* gameObject, ComponentMode componentMode);

    void                    RegisterTickFunction(TickFunction const& tickFunc);
    void                    RegisterDebugDrawFunction(Delegate<void(DebugRenderer& renderer)> const& function);

    virtual ComponentHandle ConstructComponent(Component*& component) = 0;
    virtual void            DestructComponent(ComponentHandle handle, Component*& movedComponent) = 0;

    void                    InitializeComponent(Component* component);
    void                    DeinitializeComponent(Component* component);

    virtual void            SubscribeEvents(Component* component) = 0;
    virtual void            UnsubscribeEvents(Component* component) = 0;

    void                    InvokeBeginPlay(Component* component);
    void                    InvokeEndPlay(Component* component);

    World*                  m_World;
    ComponentTypeID         m_ComponentTypeID;
    Delegate<void(Component*)> m_OnBeginPlay;
    Delegate<void(Component*)> m_OnEndPlay;
};

template <typename ComponentType>
class ComponentManager final : public ComponentManagerBase
{
    friend class World;

public:
    static constexpr ObjectStorageType StorageType = ComponentMeta::ComponentStorageType<ComponentType>();

    using ComponentStorage = ObjectStorage<ComponentType, 64, StorageType, HEAP_WORLD_OBJECTS>;

    Handle32<ComponentType> CreateComponent(GameObject* gameObject);

    Handle32<ComponentType> CreateComponent(GameObject* gameObject, ComponentType*& component);

    void                    DestroyComponent(Handle32<ComponentType> handle);

    bool                    IsHandleValid(Handle32<ComponentType> handle);

    virtual Component*      GetComponent(ComponentHandle handle) override;
    virtual Component*      GetComponentUnsafe(ComponentHandle handle) override;

    ComponentType*          GetComponent(Handle32<ComponentType> handle);

    ComponentType*          GetComponentUnsafe(Handle32<ComponentType> handle);

    uint32_t                GetComponentCount() const;

    using Iterator = typename ComponentStorage::Iterator;
    using ConstIterator = typename ComponentStorage::ConstIterator;

    Iterator                GetComponents();
    ConstIterator           GetComponents() const;

    template <typename Visitor>
    void                    IterateComponents(Visitor& visitor);

    template <typename Visitor>
    void                    IterateComponentBatches(Visitor& visitor);

private:
    explicit                ComponentManager(World* world);

    virtual ComponentHandle ConstructComponent(Component*& component) override;
    virtual void            DestructComponent(ComponentHandle handle, Component*& movedComponent) override;

    virtual void            SubscribeEvents(Component* component) override;
    virtual void            UnsubscribeEvents(Component* component) override;

    void                    BeginPlay(Component* component);
    void                    EndPlay(Component* component);

    void                    Update();
    void                    FixedUpdate();
    void                    PhysicsUpdate();
    void                    PostTransform();
    void                    LateUpdate();
    void                    DrawDebug(DebugRenderer& renderer);

    void                    OnBeginOverlap(ComponentHandle handle, class BodyComponent* body);
    void                    OnEndOverlap(ComponentHandle handle, class BodyComponent* body);

    void                    OnBeginContact(ComponentHandle handle, struct Collision& collision);
    void                    OnUpdateContact(ComponentHandle handle, struct Collision& collision);
    void                    OnEndContact(ComponentHandle handle, class BodyComponent* body);

    ComponentStorage        m_ComponentStorage;
};

HK_NAMESPACE_END

#include "ComponentManager.inl"
