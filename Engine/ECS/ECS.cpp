#include <Engine/Core/Logger.h>

#include "ECS.h"

HK_NAMESPACE_BEGIN

namespace ECS
{

ComponentTypeId Internal::ComponentFactory::m_IdGen = 0;
ComponentTypeInfo* Internal::ComponentFactory::Registry;
size_t Internal::ComponentFactory::RegistrySize;

World::World(WorldCreateInfo const& createInfo) :
    m_NumThreads(Math::Max(createInfo.NumThreads, 1u))
{
    m_CommandBuffers = new CommandBuffer[m_NumThreads];

    for (uint32_t threadIndex = 0; threadIndex < m_NumThreads; ++threadIndex)
        m_CommandBuffers[threadIndex].m_EntityAllocator = &m_EntityAllocator;

    m_QueryCaches.Resize(GetQueryList().Size());
    LOG("TOTAL QUERIES: {}\n", m_QueryCaches.Size());
}

World::~World()
{
    ExecuteCommands();

    DoDestroyEntities();

    delete[] m_CommandBuffers;
}

auto World::GetArchetype(ArchetypeId const& id) -> Archetype*
{
    for (Archetype* archetype : m_Archetypes)
    {
        if (archetype->Type == id)
            return archetype;
    }

    Archetype* archetype = new Archetype;
    archetype->Type = id;
    m_Archetypes.Add(archetype);

    archetype->Components.Reserve(id.Size());
    for (ArchetypeId::SizeType i = 0; i < id.Size(); ++i)
    {
        archetype->Components.EmplaceBack(Internal::ComponentFactory::Registry[id[i]].Size);

#ifdef HK_ECS_ARCHETYPE_LOOKUP_INDEX
        archetype->LookupIndex[id[i]] = i;
#endif
    }

    LOG("NUM ARCHETYPES: {}\n", m_Archetypes.Size());

    uint32_t n = 0;
    for (QueryId const& query : GetQueryList())
    {
        if (std::includes(archetype->Type.begin(), archetype->Type.end(),
                          query.begin(), query.end()))
        {
            m_QueryCaches[n].Archetypes.Add(archetype);
        }
        ++n;
    }

    return archetype;
}

void World::AddEventHandler(size_t eventId, Internal::EventFunction handler)
{
    auto it = m_EventHandlers.Find(eventId);
    if (it == m_EventHandlers.End())
    {
        TVector<Internal::EventFunction> handlers(1, handler);
        m_EventHandlers[eventId] = std::move(handlers);
    }
    else
    {
        HK_ASSERT(Core::NPOS == it->second.IndexOf(handler.Handler, [](Internal::EventFunction const& func, void const* handler)
                                                   { return func.Handler == handler; }));
        it->second.Add(handler);
    }
}

void World::RemoveEventHandler(size_t eventId, void* handler)
{
    auto it = m_EventHandlers.Find(eventId);
    if (it != m_EventHandlers.End())
    {
        auto index = it->second.IndexOf(handler, [](Internal::EventFunction const& func, void const* handler)
                                        { return func.Handler == handler; });

        if (index != Core::NPOS)
        {
            it->second.Remove(index);
            if (it->second.IsEmpty())
            {
                m_EventHandlers.Erase(it);
            }
        }
    }
}

void World::RemoveHandler(void* handler)
{
    for (auto it = m_EventHandlers.begin(); it != m_EventHandlers.end();)
    {
        auto& pair = *it;
        auto& handlers = pair.second;

        auto index = handlers.IndexOf(handler, [](Internal::EventFunction const& func, void const* handler)
                                      { return func.Handler == handler; });

        if (index != Core::NPOS)
        {
            handlers.Remove(index);
        }

        if (handlers.IsEmpty())
            it = m_EventHandlers.Erase(it);
        else
            it++;
    }
}

auto World::GetQueryCache(uint32_t queryId) -> QueryCache const&
{
    return m_QueryCaches[queryId];
}

EntityView World::GetEntityView(EntityHandle handle)
{
    return EntityView(handle, m_EntityAllocator);
}

static Entity DummyEntity = {};

EntityView::EntityView(EntityHandle handle, EntityAllocator& allocator) :
    m_Handle(handle),
    m_EntityRef(handle ? allocator.GetEntityRef(handle) : DummyEntity)
{}

auto World::GetEntity(EntityHandle handle) -> Entity*
{
    HK_ASSERT(handle);

    if (!handle)
        return nullptr;

    Entity& entity = m_EntityAllocator.GetEntityRef(handle);

    HK_ASSERT(entity.Version == handle.GetVersion());
    if (entity.Version != handle.GetVersion())
        return nullptr;

    return &entity;
}

auto World::GetEntity(EntityHandle handle) const -> Entity const*
{
    return const_cast<World*>(this)->GetEntity(handle);
}

namespace
{

size_t GrowComponentPool(Archetype* archetype, size_t componentPoolIndex)
{
    auto index = archetype->EntityIds.Size();

    archetype->Components[componentPoolIndex].Grow(index + 1);
    return index;
}

}

CommandBuffer& World::GetCommandBuffer(uint32_t threadIndex)
{
    return m_CommandBuffers[threadIndex];
}

void World::ExecuteCommands()
{
    for (uint32_t threadIndex = 0; threadIndex < m_NumThreads; ++threadIndex)
    {
        CommandBuffer& commandBuffer = m_CommandBuffers[threadIndex];

        for (auto& command : commandBuffer.GetCommands())
        {
            switch (command.Name)
            {
                case CommandBuffer::CMD_SPAWN_ENTITY:
                    HK_ASSERT(m_Constructable.Handle != command.Entity);
                    DoConstructEntity();
                    m_Constructable.Handle = command.Entity;
                    break;
                case CommandBuffer::CMD_DESTROY_ENTITY:
                case CommandBuffer::CMD_DESTROY_ENTITIES:
                    if (command.Entity == m_Constructable.Handle)
                    {
                        // The entity is destroyed before it has been built.
                        // Therefore, we must destroy all of its components.
                        for (auto& pair : m_Constructable.Components)
                        {
                            auto componentTID = pair.first;
                            auto componentData = pair.second;
                            auto& componentType = ComponentRegistry()[componentTID];

                            //componentType.OnComponentRemoved(this, command.Entity, componentData);
                            componentType.Destruct(componentData);
                        }

                        m_Constructable.Handle = 0;
                        m_Constructable.Components.Clear();
                    }
                    if (command.Name == CommandBuffer::CMD_DESTROY_ENTITY)
                        DoDestroyEntity(command.Entity);
                    else
                        DoDestroyEntities();
                    break;
                case CommandBuffer::CMD_ADD_COMPONENT:
                    if (command.Entity == m_Constructable.Handle)
                    {
                        m_Constructable.Components.Add(std::make_pair(command.ComponentId, command.Component));
                    }
                    else
                    {
                        DoAddComponent(command.Entity, command.ComponentId, command.Component);
                    }
                    break;
                case CommandBuffer::CMD_REMOVE_COMPONENT:
                    if (command.Entity == m_Constructable.Handle)
                    {
                        DoConstructEntity();
                    }
                    DoRemoveComponent(command.Entity, command.ComponentId);
                    break;
            }
        }
        DoConstructEntity();
        commandBuffer.Clear();
    }

    // Sort archetypes by the number of entities so that empty ones are at the end of the list.
//    if (m_bSortArchetypes)
//    {
//        std::sort(m_Archetypes.begin(), m_Archetypes.end(), [](Archetype* a, Archetype* b)
//                  {
//                      return a->EntityIds.Size() > b->EntityIds.Size();
//                  });
//        m_bSortArchetypes = false;
//    }
}

void World::DoSpawnEntity(EntityHandle handle, TVector<std::pair<uint32_t, void*>> const& components)
{
    auto* entity = GetEntity(handle);
    HK_ASSERT(entity);
    HK_ASSERT(entity->pArchetype == nullptr);

    SendEvent(Event::OnEntitySpawned(handle));

    if (components.IsEmpty())
        return;

    auto registry = ComponentRegistry();

    m_TempArchetypeId.Resize(components.Size());
    for (int i = 0; i < components.Size(); i++)
        m_TempArchetypeId[i] = components[i].first;
    std::sort(m_TempArchetypeId.begin(), m_TempArchetypeId.end());
    auto last = std::unique(m_TempArchetypeId.begin(), m_TempArchetypeId.end());
    m_TempArchetypeId.Erase(last, m_TempArchetypeId.end());

    Archetype* archetype = GetArchetype(m_TempArchetypeId);

    for (auto& pair : components)
    {
        auto componentTID = pair.first;
        auto componentData = pair.second;
        auto& componentType = registry[componentTID];
        auto componentPoolIndex = archetype->GetComponentIndex(componentTID);

        if (m_TempArchetypeId[componentPoolIndex] == ~0u)
        {
            // Component already added
            //componentType.OnComponentRemoved(this, handle, componentData);
            componentType.Destruct(componentData);
            continue;
        }

        // Use m_TempArchetypeId to mark already added components
        m_TempArchetypeId[componentPoolIndex] = ~0u;

        auto index = GrowComponentPool(archetype, componentPoolIndex);

        componentType.Move(componentData, archetype->Components[componentPoolIndex].GetAddress(index));
    }

    archetype->EntityIds.Add(handle);
    entity->Index = archetype->EntityIds.Size() - 1;
    entity->pArchetype = archetype;

    for (int componentIndex = 0 ; componentIndex < archetype->Type.Size() ; componentIndex++)
    {
        auto& componentType = registry[archetype->Type[componentIndex]];
        componentType.OnComponentAdded(this, handle, archetype->Components[componentIndex].GetAddress(entity->Index));
    }

    //m_bSortArchetypes = true;
}

void World::DoDestroyEntity(EntityHandle handle)
{
    auto* entity = GetEntity(handle);
    HK_ASSERT(entity);

    if (!entity)
        return;

    Archetype* archetype = entity->pArchetype;

    if (!archetype)
    {
        SendEvent(Event::OnEntityDestroyed(handle));

        m_EntityAllocator.EntityFreeUnlocked(handle);
        return;
    }

    auto registry = ComponentRegistry();

    if (archetype->EntityIds.Size() > 1)
    {
        auto movedEntityIndex = archetype->EntityIds.Size() - 1;
        auto movedEntityHandle = archetype->EntityIds[movedEntityIndex];

        GetEntity(movedEntityHandle)->Index = entity->Index;

        archetype->EntityIds[entity->Index] = movedEntityHandle;
        archetype->EntityIds.RemoveLast();

        for (std::size_t i = 0; i < archetype->Type.Size(); ++i)
        {
            auto  componentTID  = archetype->Type[i];
            auto& componentType = registry[componentTID];
            auto  componentData = archetype->Components[i].GetAddress(entity->Index);

            componentType.OnComponentRemoved(this, handle, componentData);
            componentType.Destruct(componentData);

            if (movedEntityIndex != entity->Index)
            {
                componentType.Move(archetype->Components[i].GetAddress(movedEntityIndex), archetype->Components[i].GetAddress(entity->Index));
            }
        }
    }
    else
    {
        archetype->EntityIds.RemoveLast();

        for (std::size_t i = 0; i < archetype->Type.Size(); ++i)
        {
            auto  componentTID  = archetype->Type[i];
            auto& componentType = registry[componentTID];
            auto  componentData = archetype->Components[i].GetAddress(entity->Index);

            componentType.OnComponentRemoved(this, handle, componentData);
            componentType.Destruct(componentData);

            HK_ASSERT(entity->Index == 0);
        }

        //m_bSortArchetypes = true;
    }

    SendEvent(Event::OnEntityDestroyed(handle));
    m_EntityAllocator.EntityFreeUnlocked(handle);
}

void World::DoDestroyEntities()
{
    THashSet<EntityHandle> entities;

    auto registry = ComponentRegistry();
    for (Archetype* archetype : m_Archetypes)
    {
        for (ArchetypeId::SizeType i = 0; i < archetype->Type.Size(); ++i)
        {
            auto const& componentType = registry[archetype->Type[i]];

            for (std::size_t index = 0; index < archetype->EntityIds.Size(); ++index)
            {
                auto* data = archetype->Components[i].GetAddress(index);
                componentType.OnComponentRemoved(this, archetype->EntityIds[index], data);
                componentType.Destruct(data);

                entities.Insert(archetype->EntityIds[index]);
            }
        }
        delete archetype;
    }

    m_Archetypes.Clear();

    for (QueryCache& cache : m_QueryCaches)
        cache.Archetypes.Clear();

    for (auto& entity : entities)
    {
        SendEvent(Event::OnEntityDestroyed(entity));

        m_EntityAllocator.EntityFreeUnlocked(entity);
    }
}

namespace
{

ArchetypeId& ArchetypeId_Make(ArchetypeId& dst, uint32_t componentTID)
{
    dst.Resize(1);
    dst[0] = componentTID;
    return dst;
}

ArchetypeId& ArchetypeId_MakeAdd(ArchetypeId const& src, ArchetypeId& dst, uint32_t added)
{
    size_t count = src.Size();

    size_t i = 0;
    size_t j = 0;

    dst.Resize(count + 1);
    while (j < count && src[j] < added)
        dst[i++] = src[j++];
    dst[i++] = added;
    while (j < count)
        dst[i++] = src[j++];

    return dst;
}

ArchetypeId& ArchetypeId_MakeRemove(ArchetypeId const& src, ArchetypeId& dst, uint32_t removed)
{
    size_t count = src.Size();

    size_t i = 0;
    size_t j = 0;

    dst.Resize(count - 1);

    while (j < count)
    {
        if (src[j] != removed)
        {
            dst[i++] = src[j];
        }
        j++;
    }

    return dst;
}

void Cleanup(World* world, EntityHandle handle, ComponentTypeId componentTID, void* data)
{
    auto registry = ComponentRegistry();

    //registry[componentTID].OnComponentRemoved(world, handle, data);
    registry[componentTID].Destruct(data);
}

}

void World::DoAddComponent(EntityHandle handle, ComponentTypeId componentTID, void* data)
{
    auto* entity = GetEntity(handle);
    HK_ASSERT(entity);

    if (!entity)
    {
        Cleanup(this, handle, componentTID, data);
        return;
    }

    auto registry = ComponentRegistry();

    auto& componentType = registry[componentTID];

    Archetype* oldArchetype = entity->pArchetype;
    Archetype* newArchetype = nullptr;

    int componentIndex{};

    if (oldArchetype)
    {
        ArchetypeId const& oldArchetypeId = oldArchetype->Type;

        if (oldArchetype->HasComponent(componentTID))
        {
            // The entity already has this component.
            Cleanup(this, handle, componentTID, data);
            return;
        }

        ArchetypeId const& newArchetypeId = ArchetypeId_MakeAdd(oldArchetypeId, m_TempArchetypeId, componentTID);

        newArchetype = GetArchetype(newArchetypeId);

        std::size_t i = 0;
        for (std::size_t j = 0; j < newArchetypeId.Size(); ++j)
        {
            auto  newComponentTID  = newArchetypeId[j];
            auto& newComponentType = registry[newComponentTID];

            auto index = GrowComponentPool(newArchetype, j);

            // Find component data and move it to new archetype.
            if (i < oldArchetypeId.Size() && oldArchetypeId[i] == newComponentTID)
            {
                newComponentType.Move(oldArchetype->Components[i].GetAddress(entity->Index),
                                      newArchetype->Components[j].GetAddress(index));

                ++i;
            }
            else
            {
                // If the component is not found, create it.
                //newComponent = new (newArchetype->Components[j].GetAddress(index)) ComponentType(std::forward<Args>(args)...);

                componentType.Move(data, newArchetype->Components[j].GetAddress(index));

                componentIndex = j;
            }
        }

        auto movedEntityIndex = oldArchetype->EntityIds.Size() - 1;
        auto movedEntityHandle = oldArchetype->EntityIds[movedEntityIndex];

        GetEntity(movedEntityHandle)->Index = entity->Index;

        oldArchetype->EntityIds[entity->Index] = movedEntityHandle;
        oldArchetype->EntityIds.RemoveLast();

        if (movedEntityIndex != entity->Index)
        {
            for (i = 0; i < oldArchetypeId.Size(); ++i)
            {
                auto  oldComponentTID  = oldArchetypeId[i];
                auto& oldComponentType = registry[oldComponentTID];

                // Move last component to entity->Index position
                oldComponentType.Move(oldArchetype->Components[i].GetAddress(movedEntityIndex), oldArchetype->Components[i].GetAddress(entity->Index));
            }
        }
    }
    else
    {
        ArchetypeId const& newArchetypeId = ArchetypeId_Make(m_TempArchetypeId, componentTID);

        newArchetype = GetArchetype(newArchetypeId);

        auto index = GrowComponentPool(newArchetype, 0);

        //newComponent = new (newArchetype->Components[0].GetAddress(index)) ComponentType(std::forward<Args>(args)...);
        componentType.Move(data, newArchetype->Components[0].GetAddress(index));
    }

    newArchetype->EntityIds.Add(handle);
    entity->Index = newArchetype->EntityIds.Size() - 1;
    entity->pArchetype = newArchetype;

    componentType.OnComponentAdded(this, handle, newArchetype->Components[componentIndex].GetAddress(entity->Index));

    //m_bSortArchetypes = true;
}

void World::DoRemoveComponent(EntityHandle handle, ComponentTypeId componentTID)
{
    auto* entity = GetEntity(handle);
    HK_ASSERT(entity);

    if (!entity)
        return;

    Archetype* oldArchetype = entity->pArchetype;

    if (!oldArchetype || !oldArchetype->HasComponent(componentTID))
    {
        return;
    }

    auto registry = ComponentRegistry();

    ArchetypeId const& oldArchetypeId = oldArchetype->Type;
    ArchetypeId const& newArchetypeId = ArchetypeId_MakeRemove(oldArchetypeId, m_TempArchetypeId, componentTID);

    Archetype* newArchetype = GetArchetype(newArchetypeId);

    std::size_t j = 0;
    for (std::size_t i = 0; i < oldArchetypeId.Size(); ++i)
    {
        if (componentTID == oldArchetypeId[i])
        {
            auto componentData = oldArchetype->Components[i].GetAddress(entity->Index);
            registry[componentTID].OnComponentRemoved(this, handle, componentData);
            registry[componentTID].Destruct(componentData);
            continue;
        }

        auto        newComponentTID  = newArchetypeId[j];
        auto const& newComponentType = registry[newComponentTID];

        HK_ASSERT(newComponentTID == oldArchetypeId[i]);

        // Reallocate component pools if needed
        size_t index = GrowComponentPool(newArchetype, j);

        // Move entity components from old archetype to new
        newComponentType.Move(oldArchetype->Components[i].GetAddress(entity->Index),
                              newArchetype->Components[j].GetAddress(index));

        ++j;
    }

    auto movedEntityIndex = oldArchetype->EntityIds.Size() - 1;
    auto movedEntityHandle = oldArchetype->EntityIds[movedEntityIndex];

    GetEntity(movedEntityHandle)->Index = entity->Index;

    oldArchetype->EntityIds[entity->Index] = movedEntityHandle;
    oldArchetype->EntityIds.RemoveLast();

    if (movedEntityIndex != entity->Index)
    {
        for (std::size_t i = 0; i < oldArchetypeId.Size(); ++i)
        {
            auto  oldComponentTID  = oldArchetypeId[i];
            auto& oldComponentType = registry[oldComponentTID];

            oldComponentType.Move(oldArchetype->Components[i].GetAddress(movedEntityIndex), oldArchetype->Components[i].GetAddress(entity->Index));
        }
    }

    newArchetype->EntityIds.Add(handle);
    entity->Index = newArchetype->EntityIds.Size() - 1;
    entity->pArchetype = newArchetype;

    //m_bSortArchetypes = true;
}

void World::DoConstructEntity()
{
    if (!m_Constructable.Handle)
        return;

    DoSpawnEntity(m_Constructable.Handle, m_Constructable.Components);

    m_Constructable.Handle = 0;
    m_Constructable.Components.Clear();
}

TArrayView<ComponentTypeId> EntityView::GetComponentIDs() const
{
    if (!IsValid())
        return {};

    Archetype const* archetype = m_EntityRef.pArchetype;
    if (!archetype)
        return {};

    return TArrayView<ComponentTypeId>(archetype->Type.Begin(), archetype->Type.End());
}

auto EntityView::GetComponentByID(ComponentTypeId componentTID) const -> void*
{
    if (!IsValid())
        return nullptr;

    Archetype const* archetype = m_EntityRef.pArchetype;
    if (!archetype)
        return nullptr;

    size_t index = archetype->GetComponentIndex(componentTID);
    if (index == -1)
        return nullptr;

    return archetype->Components[index].GetAddress(m_EntityRef.Index);
}

}

HK_NAMESPACE_END
