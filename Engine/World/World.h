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

#include "ComponentManager.h"
#include "WorldInterface.h"
#include "WorldEvent.h"
#include "WorldTick.h"
#include "TickingGroup.h"
#include "GameObject.h"

#include <Engine/Core/Containers/PageStorage.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

class World final : public Noncopyable
{
    friend class WorldInterfaceBase;
    friend class ComponentManagerBase;
    friend class GameObject;

public:
                        World();
                        ~World();

    template <typename ComponentType>
    ComponentManager<ComponentType>& GetComponentManager();

    ComponentManagerBase* TryGetComponentManager(ComponentTypeID typeID);

    template <typename ComponentType>
    ComponentType*      GetComponent(Handle32<ComponentType> componentHandle);

    template <typename ComponentType>
    ComponentType*      GetComponent(ComponentExtendedHandle componentHandle);

    template <typename Interface>
    Interface&          GetInterface();

    void                Purge();

    GameObjectHandle    CreateObject(GameObjectDesc const& desc = {});
    GameObjectHandle    CreateObject(GameObjectDesc const& desc, GameObject*& gameObject);

    GameObject*         GetObjectUnsafe(GameObjectHandle handle);
    GameObject const*   GetObjectUnsafe(GameObjectHandle handle) const;

    GameObject*         GetObject(GameObjectHandle handle);
    GameObject const*   GetObject(GameObjectHandle handle) const;

    bool                IsHandleValid(GameObjectHandle handle) const;

    void                DestroyObject(GameObjectHandle handle);
    void                DestroyObject(GameObject* gameObject);
    void                DestroyObjectNow(GameObject* gameObject);

    void                Tick(float timeStep);

    WorldTick const&    GetTick() const { return m_Tick; }

    void                SetPaused(bool paused);

    void                DrawDebug(DebugRenderer& renderer);

    template <typename Event>
    static void         SubscribeEvent(GameObject* eventSender, Component* receiver, typename Event::Holder::DelegateType delegate);

    template <typename Event>
    static void         UnsubscribeEvent(GameObject* eventSender, Component* receiver);

    template <typename Event, typename... Args>
    static void         DispatchEvent(GameObject* eventSender, Args... args);

private:
    template <typename Event>
    typename Event::Holder* GetEventHolder();

    void                SetParent(GameObject* object, GameObject* parent, GameObject::TransformRule transformRule);

    void                RegisterTickFunction(TickFunction const& f);
    void                RegisterDebugDrawFunction(Delegate<void(DebugRenderer&)> const& function);

    // TODO: add
    //void UnregisterTickFunction(...);
    //void UnregisterDebugDrawFunction(...);

    void                DestroyObjectsAndComponents();
    void                InitializeInterface(InterfaceTypeID id);
    void                ProcessCommands();
    void                RegisterTickFunctions();
    void                InitializeComponents();

    enum class HierarchyType
    {
        Static,
        Dynamic
    };

    GameObject::TransformData*  AllocateTransformData(HierarchyType hierarchyType, uint32_t hierarchyLevel);
    void                        FreeTransformData(HierarchyType hierarchyType, uint32_t hierarchyLevel, GameObject::TransformData* transformData);


    template <typename Visitor>
    void                UpdateTransformLevel(PageStorage<GameObject::TransformData>& transforms);
    void                UpdateWorldTransforms();

    void                UpdateHierarchy(GameObject* object, GameObject::TransformRule transformRule);
    void                UpdateHierarchyData(GameObject* object, bool wasDynamic);

    Vector<PageStorage<GameObject::TransformData>>& GetTransformHierarchy(HierarchyType hierarchyType);

    enum Command
    {
        Pause,
        Unpause
    };

    using GameObjectStorage = ObjectStorage<GameObject, 64, ObjectStorageType::Compact>;

    GameObjectStorage           m_ObjectStorage;
    Vector<ComponentManagerBase*> m_ComponentManagers;
    Vector<WorldInterfaceBase*> m_Interfaces;
    Vector<EventHolderBase*>    m_EventHolders;
    Vector<ComponentExtendedHandle> m_ComponentsToInitialize;
    Vector<GameObjectHandle>    m_ObjectsToDelete;
    Vector<Component*>          m_ComponentsToDelete;
    Vector<GameObjectHandle>    m_QueueToDestroy;
    Vector<TickFunction>        m_FunctionsToRegister;
    Vector<Delegate<void(DebugRenderer&)>> m_DebugDrawFunctions;
    TickingGroup                m_Update;
    TickingGroup                m_FixedUpdate;
    TickingGroup                m_PhysicsUpdate;
    TickingGroup                m_PostTransform;
    TickingGroup                m_LateUpdate;
    Vector<Command>             m_CommandBuffer;
    WorldTick                   m_Tick;
    float                       m_TimeAccumulator = 0.0f;
    Vector<PageStorage<GameObject::TransformData>> m_TransformHierarchy[2];
};

HK_NAMESPACE_END

#include "World.inl"
