#pragma once

#include <Engine/Core/Allocators/LinearAllocator.h>
#include <Engine/Core/Containers/Hash.h>
#include <Engine/Core/Containers/ArrayView.h>

#include "TypeList.h"

HK_NAMESPACE_BEGIN

namespace ECS
{

using ComponentTypeId = uint32_t;
using ArchetypeId = TVector<ComponentTypeId>;

class World;

class EntityHandle
{
    uint64_t Handle{};

public:
    EntityHandle() = default;
    EntityHandle(EntityHandle const& rhs) = default;
    EntityHandle(EntityHandle&& rhs) = default;

    EntityHandle(uint32_t id, uint32_t version) :
        Handle(uint64_t(id) | (uint64_t(version) << 32))
    {}

    EntityHandle(std::nullptr_t) :
        Handle(0)
    {}

    explicit EntityHandle(uint64_t handle) :
        Handle(handle)
    {}

    EntityHandle& operator=(EntityHandle const& rhs) = default;

    bool operator==(EntityHandle const& rhs) const
    {
        return Handle == rhs.Handle;
    }

    bool operator!=(EntityHandle const& rhs) const
    {
        return Handle != rhs.Handle;
    }

    operator bool() const
    {
        return Handle != 0;
    }

    operator uint64_t() const
    {
        return Handle;
    }

    uint16_t GetVersion() const
    {
        return Handle >> 32;
    }

    uint32_t GetID() const
    {
        return Handle & 0xffffffff;
    }

    uint64_t ToUInt64() const
    {
        return Handle;
    }

    uint32_t Hash() const
    {
        return HashTraits::Hash(Handle);
    }
};

namespace Internal
{

template <typename T>
struct Event
{
public:
    static const size_t Id;
};

HK_INLINE size_t GenerateEventId()
{
    static size_t IdGen{};
    return ++IdGen;
}

template <typename T>
const size_t Event<T>::Id = GenerateEventId();

struct EventFunction
{
    void (*Execute)(World*, void*, void*);
    void* Handler;
};

} // namespace Internal


namespace Event
{
struct OnEntitySpawned
{
    EntityHandle Handle;

    OnEntitySpawned(EntityHandle entityHandle) :
        Handle(entityHandle)
    {}

    operator EntityHandle() const
    {
        return Handle;
    }
};

struct OnEntityDestroyed
{
    EntityHandle m_EntityHandle;

    OnEntityDestroyed(EntityHandle entityHandle) :
        m_EntityHandle(entityHandle)
    {}

    EntityHandle GetEntity() const
    {
        return m_EntityHandle;
    }
};

template <typename T>
struct OnComponentAdded : Noncopyable
{
    EntityHandle m_EntityHandle;
    T& m_Component;

    OnComponentAdded(EntityHandle entityHandle, T& component) :
        m_EntityHandle(entityHandle),
        m_Component(component)
    {}

    EntityHandle GetEntity() const
    {
        return m_EntityHandle;
    }

    T& Component() const
    {
        return m_Component;
    }
};

template <typename T>
struct OnComponentRemoved : Noncopyable
{
    EntityHandle m_EntityHandle;
    T& m_Component;

    OnComponentRemoved(EntityHandle entityHandle, T& component) :
        m_EntityHandle(entityHandle),
        m_Component(component)
    {}

    EntityHandle GetEntity() const
    {
        return m_EntityHandle;
    }

    T& Component() const
    {
        return m_Component;
    }
};
} // namespace Event

struct ComponentTypeInfo
{
    void (*OnComponentAdded)(World* world, EntityHandle entityHandle, uint8_t* data);
    void (*OnComponentRemoved)(World* world, EntityHandle entityHandle, uint8_t* data);
    void (*Destruct)(uint8_t* data);
    void (*Move)(uint8_t* src, uint8_t* dst);
    size_t Size{};
};

namespace Internal
{

struct ComponentFactory
{
    static ComponentTypeInfo* Registry;
    static size_t RegistrySize;

    template <typename T>
    static ComponentTypeId GenerateTypeId()
    {
        using ComponentType = typename std::remove_const_t<T>;
        return _GenerateTypeId<ComponentType>();
    }

    static size_t GetComponentTypesCount()
    {
        return m_IdGen;
    }

private:
    template <typename T>
    static ComponentTypeId _GenerateTypeId();

private:
    static ComponentTypeId m_IdGen;
};

} // namespace Internal


HK_INLINE void Shutdown()
{
    if (Internal::ComponentFactory::Registry)
    {
        free(Internal::ComponentFactory::Registry);
        Internal::ComponentFactory::Registry = nullptr;
    }
}

HK_INLINE TArrayView<ComponentTypeInfo> ComponentRegistry()
{
    return TArrayView<ComponentTypeInfo>(Internal::ComponentFactory::Registry, Internal::ComponentFactory::RegistrySize);
};

template <typename T>
struct Component {
public:
    // NOTE: Id is used for runtime. For static time use Internal::ComponentFactory::GenerateTypeId<T>
    static const ComponentTypeId Id;
};

template<typename T>
const ComponentTypeId Component<T>::Id = Internal::ComponentFactory::GenerateTypeId<T>();

struct ComponentData
{
    static constexpr size_t PageSize = 64; // TODO: choose value

    struct Page
    {
        uint8_t* Data;
    };

    TVector<Page> Pages;
    size_t ComponentSize;

    ~ComponentData()
    {
        for (Page& page : Pages)
        {
            Core::GetHeapAllocator<HEAP_MISC>().Free(page.Data);
        }
    }

    void Grow(size_t count)
    {
        HK_ASSERT(count > 0);

        size_t pageCount = count / PageSize + 1;
        size_t curPageCount = Pages.Size();

        const size_t alignment = 16;
        const size_t pageSizeInBytes = Align(PageSize * ComponentSize, alignment);

        if (pageCount > curPageCount)
        {
            Pages.Resize(pageCount);
            for (size_t i = curPageCount ; i < pageCount ; ++i)
                Pages[i].Data = (uint8_t*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(pageSizeInBytes, alignment);
        }
    }

    uint8_t* GetPage(size_t pageIndex) const
    {
        return Pages[pageIndex].Data;
    }

    uint8_t* At(size_t index) const
    {
        size_t pageIndex = index / PageSize;
        size_t pageOffset = index - pageIndex * PageSize;

        return Pages[pageIndex].Data + pageOffset * ComponentSize;
    }
};

//#define HK_ECS_ARCHETYPE_LOOKUP_INDEX

struct Archetype
{
    ArchetypeId Type;
    TVector<ComponentData> Components;
    TVector<EntityHandle> EntityIds;
#ifdef HK_ECS_ARCHETYPE_LOOKUP_INDEX
    std::unordered_map<ComponentTypeId, uint32_t> LookupIndex;
#endif

    size_t GetComponentIndex(ComponentTypeId componentTID) const
    {
#if 1
        auto it = std::find(Type.begin(), Type.end(), componentTID);
        if (it == Type.end())
            return (size_t)-1;

        return std::distance(Type.begin(), it);
#else
        // TODO: Check what is faster

#ifdef HK_ECS_ARCHETYPE_LOOKUP_INDEX
        HK_ASSERT(LookupIndex.count(componentTID) != 0);
        return LookupIndex.find(componentTID)->second;
#else
        auto it = std::lower_bound(Type.begin(), Type.end(), componentTID);
        if (it == Type.end() || *it != componentTID)
            return -1;

        HK_ASSERT(std::distance(Type.begin(), std::find(Type.begin(), Type.end(), componentTID))
                  == std::distance(Type.begin(), it));

        return std::distance(Type.begin(), it);
#endif
#endif
    }

    bool HasComponent(ComponentTypeId componentTID) const
    {
        return std::find(Type.begin(), Type.end(), componentTID) != Type.end();

        //return std::binary_search(Type.begin(), Type.end(), componentTID);
        //auto it = std::lower_bound(Type.begin(), Type.end(), componentTID);
        //return it != Type.end() && *it == componentTID;
    }
};

struct Entity
{
    // Entity archetype
    Archetype* pArchetype;

    // Entity index in archetype
    uint32_t Index;

    // Entity generation
    uint32_t Version;
};

// 32bits Version
// id 26 bit for id inside pool + 4bits PoolID

class EntityAllocator
{
public:
    EntityAllocator();
    ~EntityAllocator();

    EntityHandle EntityAlloc();

    void EntityFreeUnlocked(EntityHandle handle);

    Entity& GetEntityRef(EntityHandle handle);

private:
    static constexpr size_t MAX_POOLS = 16;

    void AllocatePool(size_t poolNum);

    static uint32_t MakeId(uint32_t poolNum, uint32_t index);

    struct Pool
    {
        Entity* Entities;
        uint32_t Total;
    };

    Pool m_Pools[MAX_POOLS];
    int m_NumPools{};
    SpinLock m_Mutex;
    TVector<uint32_t> m_FreeList;
};

class EntityView
{
public:
    EntityView(EntityHandle handle, EntityAllocator& allocator);

    EntityHandle GetHandle() const
    {
        return m_Handle;
    }

    bool IsValid() const
    {
        return m_EntityRef.Version == m_Handle.GetVersion();
    }

    // Check is entity has component of specified type.
    template <typename T>
    bool HasComponent() const;

    // Returns a pointer to the entity component of the specified type.
    // If the entity does not have this component, returns nullptr.
    // NOTE: You must be sure that the component is not currently writing
    // from another thread.
    template <typename T>
    auto GetComponent() const -> T*;

    TArrayView<ComponentTypeId> GetComponentIDs() const;

    auto GetComponentByID(ComponentTypeId componentTID) const -> uint8_t*;

private:
    EntityHandle m_Handle;
    Entity& m_EntityRef;
};

class CommandBuffer;

struct EntityConstruct
{
private:
    CommandBuffer& m_CommandBuffer;
    EntityHandle m_Handle;

public:
    EntityConstruct(CommandBuffer& commandBuffer, EntityHandle handle) :
        m_CommandBuffer(commandBuffer),
        m_Handle(handle)
    {}

    template <typename T, typename... Args>
    EntityConstruct& AddComponent(Args&&... args);

    operator EntityHandle() const
    {
        return m_Handle;
    }
};

class CommandBuffer
{
    friend class World;

    enum COMMAND
    {
        CMD_SPAWN_ENTITY,
        CMD_DESTROY_ENTITY,
        CMD_ADD_COMPONENT,
        CMD_REMOVE_COMPONENT,
        CMD_DESTROY_ENTITIES
    };

    struct Command
    {
        COMMAND         Name;
        EntityHandle    Entity;
        ComponentTypeId ComponentId;
        void*           Component;
    };

    EntityAllocator*   m_EntityAllocator;
    TVector<Command>   m_CommandBuffer;
    TLinearAllocator<> m_Allocator;

    TVector<Command> const& GetCommands() const
    {
        return m_CommandBuffer;
    }

    void Clear()
    {
        m_CommandBuffer.Clear();
        m_Allocator.Reset();
    }

    CommandBuffer() = default;

public:
    EntityConstruct SpawnEntity()
    {
        Command cmd;
        cmd.Name = CMD_SPAWN_ENTITY;
        cmd.Entity = EntityHandle(m_EntityAllocator->EntityAlloc());
        cmd.ComponentId = 0;
        cmd.Component = nullptr;
        m_CommandBuffer.Add(cmd);

        return EntityConstruct(*this, cmd.Entity);
    }

    void DestroyEntity(EntityHandle handle)
    {
        Command cmd;
        cmd.Name = CMD_DESTROY_ENTITY;
        cmd.Entity = handle;
        cmd.ComponentId = 0;
        cmd.Component = nullptr;
        m_CommandBuffer.Add(cmd);
    }

    void DestroyEntities()
    {
        Command cmd;
        cmd.Name = CMD_DESTROY_ENTITIES;
        cmd.Entity = nullptr;
        cmd.ComponentId = 0;
        cmd.Component = nullptr;
        m_CommandBuffer.Add(cmd);        
    }

    template <typename T, typename... Args>
    auto AddComponent(EntityHandle handle, Args&&... args) -> std::remove_const_t<T>&
    {
        using Type = std::remove_const_t<T>;

        Type* component = m_Allocator.New<Type>(std::forward<Args>(args)...);

        Command cmd;
        cmd.Name = CMD_ADD_COMPONENT;
        cmd.Entity = handle;
        cmd.ComponentId = Component<Type>::Id;
        cmd.Component = component;
        m_CommandBuffer.Add(cmd);

        return *component;
    }

    template <typename T>
    void RemoveComponent(EntityHandle handle)
    {
        using Type = std::remove_const_t<T>;

        Command cmd;
        cmd.Name = CMD_REMOVE_COMPONENT;
        cmd.Entity = handle;
        cmd.ComponentId = Component<Type>::Id;
        cmd.Component = nullptr;
        m_CommandBuffer.Add(cmd);
    }
};

template <typename T, typename... Args>
EntityConstruct& EntityConstruct::AddComponent(Args&&... args)
{
    m_CommandBuffer.AddComponent<T>(m_Handle, std::forward<Args>(args)...);
    return *this;
}

struct QueryCache
{
    TVector<Archetype*> Archetypes;
};

struct WorldCreateInfo
{
    uint32_t NumThreads{1};
};

class World
{
public:
    World(WorldCreateInfo const& createInfo);
    virtual ~World();

    // Get command buffer for specified thread.
    CommandBuffer& GetCommandBuffer(uint32_t threadIndex);

    // Execute command should be called once per frame. It's not thread safe.
    // Do not call it from systems.
    void ExecuteCommands();

    // Subscribe to event. Can be used only from main thread.
    template <typename T, typename HandlerClass>
    void AddEventHandler(HandlerClass* handler);

    // Unsubscribe from event. Can be used only from main thread.
    template <typename T, typename HandlerClass>
    void RemoveEventHandler(HandlerClass* handler);

    // Unsubscribe handler from all events. Can be used only from main thread.
    void RemoveHandler(void* handler);

    // Send event to subscribers. Can be used only from main thread.
    // TODO: Add SendEvent to command buffer to allow event sending from any thread.
    template <typename T>
    void SendEvent(T const& event);

    // Returns all archetypes in the world.
    auto GetArchetypes() const -> TVector<Archetype*> const& { return m_Archetypes; }

    // Returns query cache. Used by queries to speed up archetype searching.
    auto GetQueryCache(uint32_t queryId) -> QueryCache const&;

    EntityView GetEntityView(EntityHandle handle);

private:
    auto GetEntity(EntityHandle handle) -> Entity*;

    auto GetEntity(EntityHandle handle) const -> Entity const*;

    void DoSpawnEntity(EntityHandle handle, TVector<std::pair<uint32_t, uint8_t*>> const& components);

    void DoDestroyEntity(EntityHandle handle);

    void DoDestroyEntities();

    void DoAddComponent(EntityHandle handle, ComponentTypeId componentTID, uint8_t* data);

    void DoRemoveComponent(EntityHandle handle, ComponentTypeId componentTID);

    void DoConstructEntity();

    void AddEventHandler(size_t eventId, Internal::EventFunction handler);

    void RemoveEventHandler(size_t eventId, void* handler);

    auto GetArchetype(ArchetypeId const& id) -> Archetype*;

    uint32_t m_NumThreads;

    CommandBuffer* m_CommandBuffers;
    EntityAllocator m_EntityAllocator;

    // Temp data to generate archetype ids
    ArchetypeId m_TempArchetypeId;

    TVector<Archetype*> m_Archetypes;
    TVector<QueryCache> m_QueryCaches;
    THashMap<size_t, TVector<Internal::EventFunction>> m_EventHandlers;

    struct Constructable
    {
        EntityHandle Handle{};
        TVector<std::pair<uint32_t, uint8_t*>> Components;
    };
    Constructable m_Constructable;
};

template <typename T, typename HandlerClass>
void World::AddEventHandler(HandlerClass* handler)
{
    using EventType = std::remove_const_t<T>;

    Internal::EventFunction func;
    func.Execute = [](World* world, void* handler, void* event)
    {
        ((HandlerClass*)handler)->HandleEvent(world, *(EventType*)event);
    };
    func.Handler = handler;

    AddEventHandler(Internal::Event<EventType>::Id, func);
}

template <typename T, typename HandlerClass>
void World::RemoveEventHandler(HandlerClass* handler)
{
    using EventType = std::remove_const_t<T>;
    RemoveEventHandler(Internal::Event<EventType>::Id, handler);
}

template <typename T>
void World::SendEvent(T const& event)
{
    using EventType = std::remove_const_t<T>;
    auto it = m_EventHandlers.find(Internal::Event<EventType>::Id);
    if (it != m_EventHandlers.end())
    {
        for (auto& handler : it->second)
        {
            handler.Execute(this, handler.Handler, (EventType*)&event);
        }
    }
}

template <typename T>
bool EntityView::HasComponent() const
{
    if (!IsValid())
        return false;

    Archetype const* archetype = m_EntityRef.pArchetype;

    return archetype && archetype->HasComponent(Component<T>::Id);
}

template <typename T>
auto EntityView::GetComponent() const -> T*
{
    using Type = std::remove_const_t<T>;
    return reinterpret_cast<T*>(GetComponentByID(Component<Type>::Id));
}

#if 0
struct ComponentIterator
{
    explicit ComponentIterator(World& world, EntityHandle handle);

    operator bool() const;

    uint8_t* operator++();

    uint8_t* operator++(int);

    uint8_t* GetData() const;

    ComponentTypeId GetTypeId() const;

    void Next();

private:
    Archetype const* m_Archetype{};
    int m_Index{};
    int m_i{};
    ComponentTypeId m_ComponentTID;
};

HK_INLINE ComponentIterator::ComponentIterator(World& world, EntityHandle handle)
{
    if (handle.IsValid())
    {
        auto& entity = *(Entity*)handle.GetId();
        m_Archetype = entity.pArchetype;
        m_Index = entity.Index;
    }

    Next();
}

HK_INLINE ComponentIterator::operator bool() const
{
    return m_Archetype && m_i < m_Archetype->Type.Size();
}

HK_INLINE uint8_t* ComponentIterator::operator++()
{
    Next();

    return m_Archetype->Components[m_i].At(m_Index);
}

HK_INLINE uint8_t* ComponentIterator::operator++(int)
{
    uint8_t* data = m_Archetype->Components[m_i].At(m_Index);
    Next();
    return data;
}

HK_INLINE uint8_t* ComponentIterator::GetData() const
{
    return m_Archetype->Components[m_i].At(m_Index);
}

HK_INLINE ComponentTypeId ComponentIterator::GetTypeId() const
{
    return m_ComponentTID;
}

HK_INLINE void ComponentIterator::Next()
{
    while (m_Archetype && m_i < m_Archetype->Type.Size())
    {
        m_ComponentTID = m_Archetype->Type[m_i++];
        return;
    }
}
#endif
using QueryId = TVector<ComponentTypeId>;

HK_INLINE TVector<QueryId>& GetQueryList()
{
    static TVector<QueryId> queries;
    return queries;
}

template <typename T>
class QueryTypeInfo
{
    template<typename U>
    struct MakeId
    {
        void operator()(QueryId& id) const
        {
            id.Add(Internal::ComponentFactory::GenerateTypeId<U>());
        }
    };

public:
    static uint32_t RegisterQuery();
};

template <typename T>
uint32_t QueryTypeInfo<T>::RegisterQuery()
{
    struct QueryIdGenerator
    {
        uint32_t m_Id;

        QueryIdGenerator()
        {
            QueryId id;

            ForEach<MakeId, QueryId, typename T::QueryComponentList>()(id);
            std::sort(id.begin(), id.end());

            auto& queries = GetQueryList();
            auto it = std::find(queries.begin(), queries.end(), id);
            if (it == queries.end())
            {
                queries.Add(std::move(id));

                m_Id = queries.Size() - 1;
            }
            else
                m_Id = std::distance(queries.begin(), it);
        }
    };

    static QueryIdGenerator generator;
    return generator.m_Id;
}

// The Required and ReadOnly components are mixed together so there is no need to specify them twice.
//
template<typename TL_READ = TypeList<>, typename TL_WRITE = TypeList<>>
struct Query {
    template<typename T, typename = std::enable_if_t<!std::is_const_v<T>, void>>
    using ReadOnly = Query<typename Append<const T, TL_READ>::type, TL_WRITE>;

    template<typename T, typename = std::enable_if_t<!std::is_const_v<T>, void>>
    using Required = Query<TL_READ, typename Append<const T, TL_WRITE>::type>;

    // TODO:
    //using Exclude = ...

    // Make unique list
    using QueryComponentList = typename RemoveDuplicates<typename Append<TL_READ, TL_WRITE>::type>::type;

    // Read only comonents
    using QueryComponentReadOnlyList = typename RemoveDuplicates<TL_READ>::type;

    // Read/Write components
    using QueryComponentRequiredList = typename RemoveDuplicates<TL_WRITE>::type;

    static uint32_t Id;

    struct Iterator
    {
        explicit Iterator(World& world);

        operator bool() const;

        auto operator++() -> Archetype*;

        auto operator++(int) -> Archetype*;

        auto operator*() const -> Archetype*;

        auto operator->() const -> Archetype*;

        auto GetEntity(int index) const -> EntityHandle;

        auto Count() const -> int;

        // Returns a batch of components. The component must be specified in the query description.
        // All checks are done at compile time. Never returns nullptr.
        template <typename T>
        auto Get();

        // Tries to get a batch of components. The component may not be specified in the query description.
        // Returns nullptr if the component does not exist.
        template <typename T>
        auto TryGet();

        void Next();

        template <typename T>
        bool HasComponent();

    private:
        template <typename T>
        auto GetComponentData() -> T*;

        template <typename T>
        auto TryGetComponentData() -> T*;

        TVector<Archetype*> const& m_Archetypes;
        Archetype* m_Archetype;
        int m_i;
        int m_Remains;
        int m_BatchSize;
        int m_BatchPageIndex;
    };
};

template <typename TL_READ, typename TL_WRITE>
uint32_t Query<TL_READ, TL_WRITE>::Id = QueryTypeInfo<Query<TL_READ, TL_WRITE>>::RegisterQuery();


template <typename TL_READ, typename TL_WRITE>
Query<TL_READ, TL_WRITE>::Iterator::Iterator(World& world) :
    m_Archetypes(world.GetQueryCache(Id).Archetypes), m_i(0), m_Remains(0), m_BatchSize(0), m_BatchPageIndex(0)
{
    Next();
}

template <typename TL_READ, typename TL_WRITE>
Query<TL_READ, TL_WRITE>::Iterator::operator bool() const
{
    return m_Archetype != nullptr;
}

template <typename TL_READ, typename TL_WRITE>
auto Query<TL_READ, TL_WRITE>::Iterator::operator++() -> Archetype*
{
    Next();
    return m_Archetype;
}

template <typename TL_READ, typename TL_WRITE>
auto Query<TL_READ, TL_WRITE>::Iterator::operator++(int) -> Archetype*
{
    Archetype* a = m_Archetype;
    Next();
    return a;
}

template <typename TL_READ, typename TL_WRITE>
auto Query<TL_READ, TL_WRITE>::Iterator::operator*() const -> Archetype*
{
    return m_Archetype;
}

template <typename TL_READ, typename TL_WRITE>
auto Query<TL_READ, TL_WRITE>::Iterator::operator->() const -> Archetype*
{
    return m_Archetype;
}

template <typename TL_READ, typename TL_WRITE>
auto Query<TL_READ, TL_WRITE>::Iterator::GetEntity(int index) const -> EntityHandle
{
    return m_Archetype->EntityIds[m_BatchPageIndex * ComponentData::PageSize + index];
}

template <typename TL_READ, typename TL_WRITE>
auto Query<TL_READ, TL_WRITE>::Iterator::Count() const -> int
{
    return m_BatchSize;
}

template <typename TL_READ, typename TL_WRITE>
template <typename T>
auto Query<TL_READ, TL_WRITE>::Iterator::Get()
{
    using Type = std::remove_const_t<T>;

    if constexpr (Contains<const Type, QueryComponentReadOnlyList>::value)
        return const_cast<const Type*>(GetComponentData<Type>());

    if constexpr (Contains<const Type, QueryComponentRequiredList>::value)
        return GetComponentData<Type>();

    static_assert(Contains<const Type, QueryComponentList>::value, "Unexpected component type");
}

template <typename TL_READ, typename TL_WRITE>
template <typename T>
auto Query<TL_READ, TL_WRITE>::Iterator::TryGet()
{
    using Type = std::remove_const_t<T>;

    if constexpr (Contains<const Type, QueryComponentReadOnlyList>::value)
        return const_cast<const Type*>(GetComponentData<Type>());

    if constexpr (Contains<const Type, QueryComponentRequiredList>::value)
        return GetComponentData<Type>();

    return TryGetComponentData<Type>();
}

template <typename TL_READ, typename TL_WRITE>
void Query<TL_READ, TL_WRITE>::Iterator::Next()
{
    if (m_Remains > 0)
    {
        m_BatchSize = std::min<int>(m_Remains, ComponentData::PageSize);
        m_Remains -= m_BatchSize;
        ++m_BatchPageIndex;
        return;
    }

    while (m_i < m_Archetypes.Size())
    {
        auto archetype = m_Archetypes[m_i++];

        if (archetype->EntityIds.IsEmpty())
            continue;

        //if (HK_UNLIKELY(archetype->EntityIds.IsEmpty()))
        //{
        //    // All empty archetypes at the end of the list
        //    break;
        //}

        m_Archetype = archetype;
        m_Remains = archetype->EntityIds.Size();
        m_BatchSize = std::min<int>(m_Remains, ComponentData::PageSize);
        m_BatchPageIndex = 0;
        m_Remains -= m_BatchSize;
        return;
    }
    m_Archetype = nullptr;
}

template <typename TL_READ, typename TL_WRITE>
template <typename T>
bool Query<TL_READ, TL_WRITE>::Iterator::HasComponent()
{
    return m_Archetype->HasComponent(Component<T>::Id);
}

template <typename TL_READ, typename TL_WRITE>
template <typename T>
auto Query<TL_READ, TL_WRITE>::Iterator::GetComponentData() -> T*
{
    size_t index = m_Archetype->GetComponentIndex(Component<T>::Id);
    HK_ASSERT(index != -1);

    return reinterpret_cast<T*>(m_Archetype->Components[index].GetPage(m_BatchPageIndex));
}

template <typename TL_READ, typename TL_WRITE>
template <typename T>
auto Query<TL_READ, TL_WRITE>::Iterator::TryGetComponentData() -> T*
{
    size_t index = m_Archetype->GetComponentIndex(Component<T>::Id);
    if (index == -1)
        return nullptr;

    return reinterpret_cast<T*>(m_Archetype->Components[index].GetPage(m_BatchPageIndex));
}

namespace Internal
{

template <typename T>
ComponentTypeId ComponentFactory::_GenerateTypeId()
{
    static_assert(!std::is_const<T>::value, "The type must not be constant");

    struct IdGen
    {
        IdGen() : Id(m_IdGen++)
        {
            if (Id >= RegistrySize)
            {
                auto oldSize = RegistrySize;
                RegistrySize = std::max<size_t>(256, oldSize * 2);
                Registry = (ComponentTypeInfo*)realloc(Registry, RegistrySize * sizeof(ComponentTypeInfo));
                Core::ZeroMem((uint8_t*)Registry + oldSize * sizeof(ComponentTypeInfo), (RegistrySize - oldSize) * sizeof(ComponentTypeInfo));
            }
            Registry[Id].Size = sizeof(T);
            Registry[Id].OnComponentAdded = [](World* world, EntityHandle entityHandle, uint8_t* data)
            {
                world->SendEvent(Hk::ECS::Event::OnComponentAdded<T>(entityHandle, *reinterpret_cast<T*>(data)));
            };
            Registry[Id].OnComponentRemoved = [](World* world, EntityHandle entityHandle, uint8_t* data)
            {
                world->SendEvent(Hk::ECS::Event::OnComponentRemoved<T>(entityHandle, *reinterpret_cast<T*>(data)));
            };
            Registry[Id].Destruct = [](uint8_t* data)
            {
                T* component = std::launder(reinterpret_cast<T*>(data));
                component->~T();
            };
            Registry[Id].Move = [](uint8_t* src, uint8_t* dst)
            {
                new (&dst[0]) T(std::move(*reinterpret_cast<T*>(src)));

                T* component = std::launder(reinterpret_cast<T*>(src));
                component->~T();
            };
        }

        const ComponentTypeId Id;
    };

    static IdGen generator;
    return generator.Id;
}

}

}

HK_NAMESPACE_END

HK_FORMAT_DEF_(Hk::ECS::EntityHandle, "[{}:{}]", v.GetID(), v.GetVersion());
