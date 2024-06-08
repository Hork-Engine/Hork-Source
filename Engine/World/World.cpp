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

#include "World.h"
#include "Component.h"

#include <Engine/World/DebugRenderer.h>

/*

Object life cycle:

GameObject представляет собой игровой объект, имеющий позицию, поворот, масштаб. К нему цепляются
компоненты, которые определяют как объект будет выглядеть, как себя вести и т.п.

Объект создается методом:
World::CreateObject

Удаление объекта:
World::DestroyObject и World::DestroyObjectNow

World::DestroyObject помещает объект в очередь на удаление, тогда как World::DestroyObjectNow
удаляет объект сразу. Как только объект удаляется у него выставляеся флаг
IsDestroyed = true. При удалении объекта, удаляются все его дочерние объекты и компоненты.

Создание компоненты осуществляется методом CreateComponent. Создать компоненту отдельно без game-object
нельзя. Компонента создается сразу, но ее инициализация происходит отложено. Во время инициализации
для компоненты вызывается метод BeginPlay() и устанавливается флаг IsInitialized = true.
Компненты удаляются методом DestroyComponent. При удалении сразу происходит деинициализация компоненты:
вызывается метод EndPlay() и компоненте устанавливается флаг IsInitialized = false.

Важно: даже после удаления объекта или компоненты по ним все еще можно итерироваться. Реальное удаление объектов
из хранилища и вызов их деструкторов происходит в одной точке кадра.

*/

HK_NAMESPACE_BEGIN

World::World()
{
    m_ComponentManagers.Resize(ComponentTypeRegistry::GetComponentTypesCount());
    m_Interfaces.Resize(InterfaceTypeRegistry::GetInterfaceTypesCount());
    m_EventHolders.Resize(WorldEvent::GetTypesCount());
}

World::~World()
{
    for (auto it = m_ObjectStorage.GetObjects() ; it.IsValid() ; ++it)
        DestroyObjectNow(it);
    DestroyObjectsAndComponents();

    for (auto& worldInterface : m_Interfaces)
    {
        if (worldInterface)
            worldInterface->Deinitialize();
    }

    for (ComponentManagerBase* componentManager : m_ComponentManagers)
        delete componentManager;

    for (WorldInterfaceBase* worldInterface : m_Interfaces)
        delete worldInterface;

    for (EventHolderBase* eventHolder : m_EventHolders)
        delete eventHolder;
}

void World::InitializeInterface(InterfaceTypeID id)
{
    m_Interfaces[id]->m_World = this;
    m_Interfaces[id]->m_InterfaceTypeID = id;
    m_Interfaces[id]->Initialize();
}

void World::Purge()
{
    for (auto it = m_ObjectStorage.GetObjects() ; it.IsValid() ; ++it)
        DestroyObjectNow(it);
    DestroyObjectsAndComponents();

    HK_ASSERT(m_ObjectStorage.Size() == 0);

    for (auto& worldInterface : m_Interfaces)
    {
        if (worldInterface)
            worldInterface->Purge();
    }

    for (EventHolderBase* eventHolder : m_EventHolders)
    {
        if (eventHolder)
            eventHolder->Clear();
    }

    m_TransformHierarchy[0].Clear();
    m_TransformHierarchy[1].Clear();

    m_ComponentsToInitialize.Clear();
}

GameObjectHandle World::CreateObject(GameObjectDesc const& desc)
{
    GameObject* object;
    return CreateObject(desc, object);
}

GameObjectHandle World::CreateObject(GameObjectDesc const& desc, GameObject*& gameObject)
{
    auto handle = m_ObjectStorage.CreateObject(gameObject);

    HK_ASSERT(desc.Parent != handle);

    uint32_t hierarchyLevel = 0;
    bool dynamic = desc.IsDynamic;

    if (auto* parent = GetObject(desc.Parent))
    {
        //parent->UpdateWorldTransform(); TODO?
        hierarchyLevel = parent->m_HierarchyLevel + 1;

        if (parent->m_Flags.IsDynamic)
            dynamic = true;
    }

    gameObject->m_Handle = handle;
    gameObject->m_World = this;
    gameObject->m_Flags.IsDynamic = dynamic;
    gameObject->m_Parent = desc.Parent;
    gameObject->m_HierarchyLevel = hierarchyLevel;
    gameObject->m_TransformData = AllocateTransformData(dynamic ? HierarchyType::Dynamic : HierarchyType::Static, hierarchyLevel);
    gameObject->m_TransformData->Owner = gameObject;
    gameObject->m_TransformData->LockWorldPositionAndRotation = false;
    gameObject->m_TransformData->AbsolutePosition = desc.AbsolutePosition;
    gameObject->m_TransformData->AbsoluteRotation = desc.AbsoluteRotation;
    gameObject->m_TransformData->AbsoluteScale    = desc.AbsoluteScale;
    gameObject->m_TransformData->Position = desc.Position;
    gameObject->m_TransformData->Rotation = desc.Rotation;
    gameObject->m_TransformData->Scale    = desc.Scale;

    gameObject->LinkToParent();
    gameObject->m_TransformData->UpdateWorldTransform();

    gameObject->m_Name = desc.Name;

    return handle;
}

void World::SetParent(GameObject* object, GameObject* parent, GameObject::TransformRule transformRule)
{
    if (object == parent)
        return;

    if (object->IsStatic() && (parent && parent->IsDynamic()))
    {
        LOG("WARNING: Attaching static object to dynamic is not allowed\n");
        return;
    }

    object->UnlinkFromParent();

    object->m_NextSibling = 0;
    object->m_PrevSibling = 0;

    if (parent)
    {
        parent->UpdateWorldTransform();

        object->m_Parent = parent->m_Handle;
        object->LinkToParent();
    }

    UpdateHierarchy(object, transformRule);
}

void World::UpdateHierarchy(GameObject* object, GameObject::TransformRule transformRule)
{
    GameObject* parent = object->GetParent();

    UpdateHierarchyData(object, object->IsDynamic());

    object->m_TransformData->Parent = parent ? parent->m_TransformData : nullptr;

    switch (transformRule)
    {
    case GameObject::TransformRule::KeepRelative:
        object->m_TransformData->UpdateWorldTransform();
        break;
    case GameObject::TransformRule::KeepWorld:
        object->SetWorldTransform(object->m_TransformData->WorldPosition,
                                  object->m_TransformData->WorldRotation,
                                  object->m_TransformData->WorldScale);
        break;
    }

    for (auto it = object->GetChildren(); it.IsValid(); ++it)
    {
        UpdateHierarchy(it, transformRule);
    }
}

void World::UpdateHierarchyData(GameObject* object, bool wasDynamic)
{
    GameObject* parent = object->GetParent();

    uint32_t newLevel = parent ? parent->m_HierarchyLevel + 1 : 0;
    uint32_t oldLevel = object->m_HierarchyLevel;

    bool isDynamic = object->IsDynamic();

    if (newLevel != oldLevel || isDynamic != wasDynamic)
    {
        GameObject::TransformData* oldData = object->m_TransformData;
        GameObject::TransformData* newData = AllocateTransformData(isDynamic ? HierarchyType::Dynamic : HierarchyType::Static, newLevel);

        *newData = std::move(*oldData);

        object->m_HierarchyLevel = newLevel;
        object->m_TransformData = newData;

        for (auto it = object->GetChildren(); it.IsValid(); ++it)
        {
            GameObject::TransformData* transformData = it->m_TransformData;
            transformData->Parent = newData;
        }

        FreeTransformData(wasDynamic ? HierarchyType::Dynamic : HierarchyType::Static, oldLevel, oldData);
    }
}

GameObject* World::GetObjectUnsafe(GameObjectHandle handle)
{
    return handle ? m_ObjectStorage.GetObject(handle) : nullptr;
}

GameObject const* World::GetObjectUnsafe(GameObjectHandle handle) const
{
    return const_cast<World*>(this)->GetObjectUnsafe(handle);
}

GameObject* World::GetObject(GameObjectHandle handle)
{
    return IsHandleValid(handle) ? m_ObjectStorage.GetObject(handle) : nullptr;
}

GameObject const* World::GetObject(GameObjectHandle handle) const
{
    return const_cast<World*>(this)->GetObject(handle);
}

bool World::IsHandleValid(GameObjectHandle handle) const
{
    if (!handle)
        return false;

    auto id = handle.GetID();

    auto& objects = m_ObjectStorage.GetRandomAccessTable();

    // Check garbage
    if (id >= objects.Size())
        return false;

    // Check is freed
    if (objects[id] == nullptr)
        return false;

    // Check version / id
    return objects[id]->GetHandle().ToUInt32() == handle.ToUInt32();
}

void World::DestroyObject(GameObjectHandle handle)
{
    m_QueueToDestroy.Add(handle);
}

void World::DestroyObject(GameObject* gameObject)
{
    if (!gameObject)
        return;

    // TODO: Check if gameObject->GetWorld() == this

    m_QueueToDestroy.Add(gameObject->GetHandle());
}

void World::DestroyObjectNow(GameObject* gameObject)
{
    HK_IF_NOT_ASSERT(gameObject) { return; }
    HK_IF_NOT_ASSERT(gameObject->GetWorld() == this) { return; }

    if (gameObject->m_Flags.IsDestroyed)
        return;

    gameObject->m_Flags.IsDestroyed = true;

    for (auto it = gameObject->GetChildren(); it.IsValid(); ++it)
        DestroyObjectNow(it);

    while (!gameObject->m_Components.IsEmpty())
    {
        Component* component = gameObject->m_Components.First();
        component->GetManager()->DestroyComponent(component);
    }

    gameObject->UnlinkFromParent();

    // TODO: Where to free transform data?

    auto hierarchyType = gameObject->IsDynamic() ? HierarchyType::Dynamic : HierarchyType::Static;
    FreeTransformData(hierarchyType, gameObject->m_HierarchyLevel, gameObject->m_TransformData);

    m_ObjectsToDelete.Add(gameObject->GetHandle());
}

void World::DestroyObjectsAndComponents()
{
    for (auto handle : m_QueueToDestroy)
    {
        if (GameObject* gameObject = GetObject(handle))
            DestroyObjectNow(gameObject);
    }
    m_QueueToDestroy.Clear();

    while (!m_ComponentsToDelete.IsEmpty())
    {
        Component* component = m_ComponentsToDelete.First();

        Component* movedComponentOldPtr;
        component->GetManager()->DestructComponent(component->GetHandle(), movedComponentOldPtr);

        if (movedComponentOldPtr)
        {
            // Указатель component теперь ссылается на перемещенный компонент
            GameObject* owner = component->GetOwner();

            // Если компонент не удален из владельца, то пропатчим его указатель внутри game object
            if (owner)
                owner->PatchComponentPointer(movedComponentOldPtr, component);

            // Если перемещенный компонент тоже в списке, то удаляем его старый указатель из списка,
            // т.к. его реальным указателем теперь является component.
            auto index = m_ComponentsToDelete.IndexOf(movedComponentOldPtr);
            if (index != Core::NPOS)
            {
                m_ComponentsToDelete.RemoveUnsorted(index);
                continue;
            }
        }

        m_ComponentsToDelete.RemoveUnsorted(0);
    }

    struct HandleFetcher
    {
        HK_FORCEINLINE static GameObjectHandle FetchHandle(GameObject* object)
        {
            return object->GetHandle();
        }
    };
    for (auto handle : m_ObjectsToDelete)
        m_ObjectStorage.DestroyObject<HandleFetcher>(handle);
    m_ObjectsToDelete.Clear();
}

void World::RegisterTickFunction(TickFunction const& f)
{
    m_FunctionsToRegister.Add(f);
}

void World::RegisterDebugDrawFunction(Delegate<void(DebugRenderer&)> const& function)
{
    m_DebugDrawFunctions.Add(function);
}

void World::RegisterTickFunctions()
{
    for (auto& f : m_FunctionsToRegister)
    {
        TickingGroup* tickingGroup = nullptr;
        switch (f.Group)
        {
            case TickGroup::Update:
                tickingGroup = &m_Update;
                break;
            case TickGroup::FixedUpdate:
                tickingGroup = &m_FixedUpdate;
                break;
            case TickGroup::PhysicsUpdate:
                tickingGroup = &m_PhysicsUpdate;
                break;
            case TickGroup::PostTransform:
                tickingGroup = &m_PostTransform;
                break;
            case TickGroup::LateUpdate:
                tickingGroup = &m_LateUpdate;
                break;
        }

        HK_ASSERT(tickingGroup);

        tickingGroup->AddFunction(f);
    }
    m_FunctionsToRegister.Clear();
}

void World::InitializeComponents()
{
    for (ComponentExtendedHandle extandedHandle : m_ComponentsToInitialize)
    {
        ComponentManagerBase* componentManager = m_ComponentManagers[extandedHandle.TypeID];
        Component* component = componentManager->GetComponent(extandedHandle.Handle);
        HK_ASSERT(component);

        componentManager->InitializeComponent(component);
    }

    m_ComponentsToInitialize.Clear();
}

void World::Tick(float timeStep)
{
    ProcessCommands();

    const float fixedTimeStep = 1.0f / 60.0f;

    m_Tick.FrameTimeStep = timeStep;
    m_Tick.FixedTimeStep = fixedTimeStep;

    m_Update.Dispatch(m_Tick);

    m_TimeAccumulator += timeStep;

    while (m_TimeAccumulator >= fixedTimeStep)
    {
        m_TimeAccumulator -= fixedTimeStep;

        m_Tick.PrevStateIndex = m_Tick.StateIndex;
        m_Tick.StateIndex = (m_Tick.StateIndex + 1) & 1;

        RegisterTickFunctions();

        InitializeComponents();

        m_FixedUpdate.Dispatch(m_Tick);
        m_PhysicsUpdate.Dispatch(m_Tick);

        DestroyObjectsAndComponents();

        UpdateWorldTransforms();

        m_PostTransform.Dispatch(m_Tick);

        //m_LightingSystem->UpdateBoundingBoxes(m_Tick);
        //m_RenderSystem->UpdateBoundingBoxes(m_Tick);

        //m_GameEvents.SwapReadWrite();
        //if (m_EventHandler)
        //    m_EventHandler->ProcessEvents(m_GameEvents.GetEventsUnlocked());

        if (!m_Tick.IsPaused)
        {
            m_Tick.FixedFrameNum++;
            m_Tick.FixedTime = m_Tick.FixedFrameNum * fixedTimeStep;
        }
    }

    m_Tick.Interpolate = m_TimeAccumulator / fixedTimeStep;

    //m_TransformHistorySystem->Update(m_Tick);

    //if (com_InterpolateTransform)
    //{
    //    m_TransformSystem->InterpolateTransformState(m_Tick);
    //    m_SceneGraph.InterpolateTransformState(m_Tick.PrevStateIndex, m_Tick.StateIndex, m_Tick.Interpolate);
    //}
    //else
    //{
    //    m_TransformSystem->CopyTransformState(m_Tick);
    //    m_SceneGraph.CopyTransformState(m_Tick.PrevStateIndex, m_Tick.StateIndex);
    //}

    //m_LightingSystem->Update(m_Tick);

    //m_SkinningSystem->InterpolatePoses(m_Tick);
    //m_SkinningSystem->UpdateSkins();

    m_LateUpdate.Dispatch(m_Tick);

    if (!m_Tick.IsPaused)
        m_Tick.FrameTime += timeStep;
    m_Tick.FrameNum++;

    m_Tick.RunningTime += timeStep;
}

void World::SetPaused(bool paused)
{
    m_CommandBuffer.Add(paused ? Command::Pause : Command::Unpause);
}

void World::ProcessCommands()
{
    for (Command command : m_CommandBuffer)
    {
        switch (command)
        {
        case Command::Pause:
            m_Tick.IsPaused = true;
            break;
        case Command::Unpause:
            m_Tick.IsPaused = false;
            break;
        }
    }
    m_CommandBuffer.Clear();
}

void World::DrawDebug(DebugRenderer& renderer)
{
    for (auto& function : m_DebugDrawFunctions)
        function.Invoke(renderer);
}

TVector<PageStorage<GameObject::TransformData>>& World::GetTransformHierarchy(HierarchyType hierarchyType)
{
    return m_TransformHierarchy[static_cast<int>(hierarchyType)];
}

GameObject::TransformData* World::AllocateTransformData(HierarchyType hierarchyType, uint32_t hierarchyLevel)
{
    auto& transformHierarchy = GetTransformHierarchy(hierarchyType);

    while (transformHierarchy.Size() <= hierarchyLevel)
        transformHierarchy.Add();

    return transformHierarchy[hierarchyLevel].EmplaceBack();
}

void World::FreeTransformData(HierarchyType hierarchyType, uint32_t hierarchyLevel, GameObject::TransformData* transformData)
{
    auto& transformHierarchy = GetTransformHierarchy(hierarchyType);
    auto& storage = transformHierarchy[hierarchyLevel];

    auto* last = &storage[storage.Size() - 1];

    if (transformData != last)
    {
        *transformData = std::move(*last);
        transformData->Owner->m_TransformData = transformData;

        for (auto childIt = transformData->Owner->GetChildren(); childIt.IsValid(); ++childIt)
        {
            childIt->m_TransformData->Parent = transformData;
        }
    }

    storage.PopBack();
    storage.ShrinkToFit();
}

template <typename Visitor>
HK_FORCEINLINE void World::UpdateTransformLevel(PageStorage<GameObject::TransformData>& transforms)
{
#if 0
    for (uint32_t i = 0; i < transforms.Size(); ++i)
    {
        Visitor::Visit(transforms[i]);
    }
#else
    uint32_t processed = 0;
    for (uint32_t pageIndex = 0; pageIndex < transforms.GetPageCount(); ++pageIndex)
    {
        GameObject::TransformData* pageData = transforms.GetPageData(pageIndex);

        uint32_t remaining = transforms.Size() - processed;
        if (remaining < transforms.GetPageSize())
        {
            for (uint32_t i = 0; i < remaining; ++i)
            {
                Visitor::Visit(pageData[i]);
            }

            processed += remaining;
        }
        else
        {
            for (size_t i = 0; i < transforms.GetPageSize(); ++i)
            {
                Visitor::Visit(pageData[i]);
            }

            processed += transforms.GetPageSize();
        }
    }
#endif
}

void World::UpdateWorldTransforms()
{
    DestroyObjectsAndComponents();

    struct UpdateRoot
    {
        HK_FORCEINLINE void Visit(GameObject::TransformData& transform)
        {
            transform.WorldPosition = transform.Position;
            transform.WorldRotation = transform.Rotation;
            transform.WorldScale    = transform.Scale;

            transform.UpdateWorldTransformMatrix();
        }
    };

    struct UpdateWithParent
    {
        HK_FORCEINLINE void Visit(GameObject::TransformData& transform)
        {
            if (transform.LockWorldPositionAndRotation)
            {
                // Пересчитать локальную позицию и поворот относительно родителя так, чтобы мировая позиция
                // оставалась неизменной.
                transform.Position = transform.Parent->WorldTransform.Inversed() * transform.WorldPosition;
                transform.Rotation = transform.Parent->WorldRotation.Inversed() * transform.WorldRotation;
            }
            else
            {
                transform.WorldPosition = transform.AbsolutePosition ? transform.Position : transform.Parent->WorldTransform * transform.Position;
                transform.WorldRotation = transform.AbsoluteRotation ? transform.Rotation : transform.Parent->WorldRotation * transform.Rotation;
            }

            transform.WorldScale = transform.AbsoluteScale    ? transform.Scale    : transform.Parent->WorldScale * transform.Scale;

            transform.UpdateWorldTransformMatrix();
        }
    };

    auto& transformHierarchy = GetTransformHierarchy(HierarchyType::Dynamic);

    if (!transformHierarchy.IsEmpty())
    {
        UpdateRoot updateRoot;
        UpdateWithParent updateWithParent;

        transformHierarchy[0].Iterate(updateRoot);
        for (uint32_t hierarchyLevel = 1; hierarchyLevel < transformHierarchy.Size(); ++hierarchyLevel)
        {
            transformHierarchy[hierarchyLevel].Iterate(updateWithParent);
        }
    }
}

HK_NAMESPACE_END
