#pragma once

#include <Engine/Core/IO.h>
#include <Engine/Core/Containers/ArrayView.h>
#include <Engine/Core/Containers/PagedVector.h>
#include <Engine/Core/Containers/Hash.h>

#include "ResourceHandle.h"
#include "ResourceProxy.h"

#include "ThreadSafeQueue.h"

HK_NAMESPACE_BEGIN

class ResourceStreamQueue
{
public:
    void Enqueue(ResourceID resource)
    {
        m_Queue.Push(resource);
    }

    ResourceID Dequeue()
    {
        ResourceID resource;
        m_Queue.TryPop(resource);
        return resource;
    }

    ThreadSafeQueue<ResourceID> m_Queue;
};

using ResourceAreaID = uint32_t;

struct ResourceArea;

/*
All public methods are thread safe except AddResourcePack.
Methods prefixed with MainThread_ can only be called from the main thread.
*/
class ResourceManager final
{
public:
    ResourceManager();
    ~ResourceManager();

    // AddResourcePack is not thread safe.
    void AddResourcePack(StringView fileName);

    TVector<Archive> const& GetResourcePacks() const { return m_ResourcePacks; }

    ResourceAreaID CreateResourceArea(TArrayView<ResourceID> resourceList);
    void DestroyResourceArea(ResourceAreaID area);

    void LoadArea(ResourceAreaID area);
    void UnloadArea(ResourceAreaID area);
    void ReloadArea(ResourceAreaID area);

    bool LoadResource(ResourceID resource);
    bool UnloadResource(ResourceID resource);
    bool ReloadResource(ResourceID resource);

    template <typename T>
    ResourceHandle<T> LoadResource(StringView name);

    template <typename T>
    void UnloadResource(StringView name);

    template <typename T>
    ResourceHandle<T> CreateResourceWithData(StringView name, TUniqueRef<T> resourceData);

    template <typename T, typename... Args>
    ResourceHandle<T> CreateResource(StringView name, Args&&... args);

    template <typename T>
    ResourceHandle<T> CreateResourceFromFile(StringView path);

    File OpenResource(StringView path);

    bool IsAreaReady(ResourceAreaID area);

    // Can be called only from main thread.
    void MainThread_WaitResourceArea(ResourceAreaID area);

    // Can be called only from main thread.
    void MainThread_WaitResource(ResourceID resource);

    template <typename T>
    ResourceHandle<T> GetResource(StringView resourcePath);

    ResourceProxy* FindResource(StringView resourcePath);

    template <typename T>
    T* TryGet(ResourceID resource);

    template <typename T>
    T* TryGet(ResourceHandle<T> handle);

    ResourceProxy& GetProxy(ResourceID resource);

    StringView GetResourceName(ResourceID resource);

    bool IsResourceReady(ResourceID resource);

    // Called per frame
    void MainThread_Update(float timeBudget);

private:
    struct Command
    {
        enum TYPE : uint8_t
        {
            CREATE_AREA,
            DESTROY_AREA,
            LOAD_RESOURCE,
            LOAD_AREA,
            UNLOAD_RESOURCE,
            UNLOAD_AREA,
            RELOAD_RESOURCE,
            RELOAD_AREA,
        };
        TYPE Type;
        uint32_t ResourceOrAreaID;
    };

    void UpdateAsync();

    TUniqueRef<ResourceBase> LoadResourceAsync(RESOURCE_TYPE type, StringView name);

    /** Find file in resource packs */
    bool FindFile(StringView fileName, int* pResourcePackIndex, FileHandle* pFileHandle) const;

    ResourceArea* AllocateArea();
    void FreeArea(ResourceAreaID areaID);
    ResourceArea* FetchArea(ResourceAreaID areaID);

    void AddCommand(Command const& command);

    void ExecuteCommands();

    void ReleaseResource(ResourceID resource);

    void IncrementAreas(ResourceProxy& proxy);
    void DecrementAreas(ResourceProxy& proxy);

    using ResourceList = TPagedVector<ResourceProxy, 1024, 1024>;
    ResourceList m_ResourceList;
    TStringHashMap<ResourceID> m_ResourceHash;
    Mutex m_ResourceHashMutex;

    TVector<ResourceID> m_DelayedRelease;

    ResourceStreamQueue m_StreamQueue;
    ThreadSafeQueue<ResourceID> m_ProcessingQueue;
    SyncEvent m_StreamQueueEvent;
    SyncEvent m_ProcessingQueueEvent;

    TVector<ResourceArea*> m_ResourceAreas;
    TVector<uint32_t> m_ResourceAreaFreeList;
    Mutex m_ResourceAreaAllocMutex;

    TVector<Command> m_CommandBuffer;
    Mutex m_CommandBufferMutex;    

    THashMap<ResourceID, int> m_Refs;
    THashSet<ResourceID> m_ReloadResources;

    Thread m_Thread;
    AtomicBool m_RunAsync;

    TVector<Archive> m_ResourcePacks;
};

template <typename T>
ResourceHandle<T> ResourceManager::LoadResource(StringView name)
{
    ResourceHandle<T> resource = GetResource<T>(name);
    LoadResource(resource);
    return resource;
}

template <typename T>
void ResourceManager::UnloadResource(StringView name)
{
    UnloadResource(GetResource<T>(name));
}

template <typename T>
ResourceHandle<T> ResourceManager::CreateResourceWithData(StringView name, TUniqueRef<T> resourceData)
{
    ResourceHandle<T> resource = GetResource<T>(name);
    if (!resource)
        return {};

    auto& proxy = GetProxy(resource);

    proxy.m_Resource = std::move(resourceData);
    proxy.m_UseCount++; // = 1;
    proxy.m_State = RESOURCE_STATE_READY;
    proxy.m_Flags = RESOURCE_FLAG_PROCEDURAL;

    IncrementAreas(proxy);

    return resource;
}

template <typename T, typename... Args>
ResourceHandle<T> ResourceManager::CreateResource(StringView name, Args&&... args)
{
    return CreateResourceWithData<T>(name, MakeUnique<T>(std::forward<Args>(args)...));
}

template <typename T>
ResourceHandle<T> ResourceManager::CreateResourceFromFile(StringView path)
{
    File file = OpenResource(path);
    if (file)
        return CreateResource<T>(path, file, this);
    return CreateResource<T>(path);
}

template <typename T>
ResourceHandle<T> ResourceManager::GetResource(StringView resourcePath)
{
    HK_ASSERT(!resourcePath.IsEmpty());
    if (resourcePath.IsEmpty())
        return {};

    MutexGuard lock(m_ResourceHashMutex);

    auto it = m_ResourceHash.Find(resourcePath);
    if (it == m_ResourceHash.End())
    {
        ResourceID resource(T::Type, m_ResourceList.Add());

        auto result = m_ResourceHash.Insert(resourcePath, resource);

        GetProxy(resource).m_Name = result.first.get_node()->mValue.first;
        //LOG("RESOURCES SIZE: sizeof {} num blocks {} block sizeof {} proxy size {}\n", sizeof(m_Resources), m_Resources.m_ResourceList.GetNumBlocks(), sizeof(ResourceList<ResourceProxy, ResourceContainer::BlockSize>::Block), sizeof(ResourceProxy));
        return ResourceHandle<T>(resource);
    }
    // Check is resource already registered with different type.
    HK_ASSERT(it->second.Is<T>());
    if (!it->second.Is<T>())
        return {};
    return ResourceHandle<T>(it->second);
}

template <typename T>
T* ResourceManager::TryGet(ResourceID resource)
{
    HK_ASSERT(!resource || resource.Is<T>());
    if (!resource.Is<T>())
    {
        return nullptr;
    }

    auto& proxy = GetProxy(resource);
    if (!proxy.IsReady())
        return nullptr;

    return static_cast<T*>(proxy.m_Resource.RawPtr());
}

template <typename T>
HK_FORCEINLINE T* ResourceManager::TryGet(ResourceHandle<T> handle)
{
    return TryGet<T>(handle.ID);
}

HK_FORCEINLINE ResourceProxy& ResourceManager::GetProxy(ResourceID resource)
{
    return m_ResourceList.Get(resource.GetIndex());
}

HK_FORCEINLINE StringView ResourceManager::GetResourceName(ResourceID resource)
{
    return GetProxy(resource).GetName();
}

HK_FORCEINLINE bool ResourceManager::IsResourceReady(ResourceID resource)
{
    return GetProxy(resource).IsReady();
}

/*
template <typename T>
bool ResourceManager::LoadResource(ResourceHandle<T> handle)
{
    ResourceID resource = handle.ID;

    // Type check
    if (resource.GetType() != T::Type)
        return false;

    return LoadResource(resource);
}
*/
/*
template <typename T>
bool ResourceManager::UnloadResource(ResourceHandle<T> handle)
{
    ResourceID resource = handle.ID;

    if (resource.GetType() != T::Type)
        return false;

    return UnloadResource(resource);
}
*/

#if 0 // todo
/**

Static resource finder

Usage:
static TStaticResourceFinder<MeshResource> FindMesh("/Root/Meshes/MyMesh.mesh"s);
MeshResource* mesh = FindMesh.TryGet();
MeshHandle handle = FindMesh.GetHandle();

or simple:

static auto handle = GameApplication::GetResourceManager()->GetResource<MeshResource>("/Root/Meshes/MyMesh.mesh"s);

*/
template <typename T>
struct TStaticResourceFinder
{
    TStaticResourceFinder(GlobalStringView path) :
        m_Handle(Global::TheResourceManager->GetResource<T>(path))
    {}

    ResourceHandle<T> GetHandle() const
    {
        return m_Handle;
    }

    T* TryGet()
    {
        return Global::TheResourceManager->TryGet(m_Handle);
    }

private:
    ResourceHandle<T> m_Handle;
};
#endif

HK_NAMESPACE_END
