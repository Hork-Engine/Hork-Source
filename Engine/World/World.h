#pragma once

#include "ComponentManager.h"
#include "WorldInterface.h"
#include "WorldEvent.h"
#include "WorldTick.h"
#include "TickingGroup.h"
#include "GameObject.h"

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

    TVector<PageStorage<GameObject::TransformData>>& GetTransformHierarchy(HierarchyType hierarchyType);

    enum Command
    {
        Pause,
        Unpause
    };

    ObjectStorage<GameObject>   m_ObjectStorage;
    TVector<ComponentManagerBase*> m_ComponentManagers;
    TVector<WorldInterfaceBase*>m_Interfaces;
    TVector<EventHolderBase*>   m_EventHolders;
    TVector<ComponentExtendedHandle> m_ComponentsToInitialize;
    TVector<GameObjectHandle>   m_ObjectsToDelete;
    TVector<Component*>         m_ComponentsToDelete;
    TVector<GameObjectHandle>   m_QueueToDestroy;
    TVector<TickFunction>       m_FunctionsToRegister;
    TVector<Delegate<void(DebugRenderer&)>> m_DebugDrawFunctions;
    TickingGroup                m_Update;
    TickingGroup                m_FixedUpdate;
    TickingGroup                m_PhysicsUpdate;
    TickingGroup                m_PostTransform;
    TickingGroup                m_LateUpdate;
    TVector<Command>            m_CommandBuffer;
    WorldTick                   m_Tick;
    float                       m_TimeAccumulator = 0.0f;
    TVector<PageStorage<GameObject::TransformData>> m_TransformHierarchy[2];
};

HK_NAMESPACE_END

#include "World.inl"
