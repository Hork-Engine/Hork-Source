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

    void                    RegisterTickFunction(TickFunction const& f);
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
    Handle32<ComponentType> CreateComponent(GameObject* gameObject);

    Handle32<ComponentType> CreateComponent(GameObject* gameObject, ComponentType*& component);

    void                    DestroyComponent(Handle32<ComponentType> handle);

    bool                    IsHandleValid(Handle32<ComponentType> handle);

    virtual Component*      GetComponent(ComponentHandle handle) override;
    virtual Component*      GetComponentUnsafe(ComponentHandle handle) override;

    ComponentType*          GetComponent(Handle32<ComponentType> handle);

    ComponentType*          GetComponentUnsafe(Handle32<ComponentType> handle);

    uint32_t                GetComponentCount() const;

    using Iterator = typename ObjectStorage<ComponentType>::Iterator;
    using ConstIterator = typename ObjectStorage<ComponentType>::ConstIterator;

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
    void                    PostTransform();
    void                    LateUpdate();
    void                    DrawDebug(DebugRenderer& renderer);

    void                    OnBeginOverlap(ComponentHandle handle, class BodyComponent* body);
    void                    OnEndOverlap(ComponentHandle handle, class BodyComponent* body);
    
    ObjectStorage<ComponentType> m_ComponentStorage;
};

HK_NAMESPACE_END

#include "ComponentManager.inl"
