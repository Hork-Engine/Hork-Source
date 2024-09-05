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

#include "ResourceManager.h"

#include "Resource_Animation.h"
#include "Resource_Mesh.h"
#include "Resource_Material.h"
#include "Resource_Texture.h"
#include "Resource_Font.h"
#include "Resource_Terrain.h"
#include "Resource_Sound.h"

#include <Engine/GameApplication/GameApplication.h>
#include <Engine/Core/Platform.h>
#include <Engine/Core/Profiler.h>

HK_NAMESPACE_BEGIN

struct ResourceArea
{
    ResourceAreaID      m_Id{};
    uint32_t            m_ResourcesLoaded{};
    Vector<ResourceID>  m_ResourceList;
    bool                m_Load{};

    bool IsReady() const
    {
        return m_ResourcesLoaded == m_ResourceList.Size();
    }
};

ResourceManager::ResourceManager()
{
    // Add dummy area. Area with ID == 0 is invalid.
    m_ResourceAreas.Add(nullptr);

    m_RunAsync.Store(true);

    // TODO: thread pool
    m_Thread.Start([this]()
                   { UpdateAsync(); });

    Core::TraverseDirectory(CoreApplication::sGetRootPath(), false,
                            [this](StringView fileName, bool bIsDirectory)
                            {
                                if (bIsDirectory)
                                {
                                    return;
                                }

                                if (PathUtils::sCompareExt(fileName, ".resources"))
                                {
                                    AddResourcePack(fileName);
                                }
                            });
}

ResourceManager::~ResourceManager()
{
    m_RunAsync.Store(false);
    m_StreamQueueEvent.Signal();

    m_Thread.Join();

    // TODO: Purge resources
}

void ResourceManager::AddResourcePack(StringView fileName)
{
    m_ResourcePacks.EmplaceBack<Archive>(Archive::sOpen(fileName, true));
}

bool ResourceManager::FindFile(StringView fileName, int* pResourcePackIndex, FileHandle* pFileHandle) const
{
    *pResourcePackIndex = -1;
    pFileHandle->Reset();

    for (int i = m_ResourcePacks.Size() - 1; i >= 0; i--)
    {
        Archive const& pack = m_ResourcePacks[i];

        FileHandle handle = pack.LocateFile(fileName);
        if (handle.IsValid())
        {
            *pResourcePackIndex = i;
            *pFileHandle = handle;
            return true;
        }
    }
    return false;
}

File ResourceManager::OpenFile(StringView path)
{
    if (!path.IcmpN("/Root/", 6))
    {
        path = path.TruncateHead(6);
        #if 0
        for (auto& factory : m_ResourceFactories)
        {
            File file = factory->OpenFile(path);
            if (file)
                return file;
        }
        #endif
        // try to load from file system
        String fileSystemPath = CoreApplication::sGetRootPath() + path;
        if (Core::IsFileExists(fileSystemPath))
        {
            return File::sOpenRead(fileSystemPath);
        }

        // try to load from resource pack
        int resourcePack;
        FileHandle fileHandle;
        if (FindFile(path, &resourcePack, &fileHandle))
        {
            return File::sOpenRead(fileHandle, m_ResourcePacks[resourcePack]);
        }

        LOG("File not found /Root/{}\n", path);
        return {};
    }
    #if 0
    if (!path.IcmpN("/Common/", 8))
    {
        path = path.TruncateHead(1);

        // try to load from file system
        if (Core::IsFileExists(path))
        {
            return File::OpenRead(path);
        }

        // try to load from resource pack
        return File::OpenRead(path.TruncateHead(7), m_CommonResources);
    }
    #endif
    if (!path.IcmpN("/FS/", 4))
    {
        path = path.TruncateHead(4);

        return File::sOpenRead(path);
    }

    if (!path.IcmpN("/Embedded/", 10))
    {
        path = path.TruncateHead(10);

        return File::sOpenRead(path, GameApplication::sGetEmbeddedArchive());
    }

    LOG("Invalid path \"{}\"\n", path);
    return {};
}

ResourceProxy* ResourceManager::FindResource(StringView resourcePath)
{
    MutexGuard lock(m_ResourceHashMutex);

    auto it = m_ResourceHash.Find(resourcePath);
    if (it == m_ResourceHash.End())
    {
        return nullptr;
    }

    ResourceID resource = it->second;
    return &GetProxy(resource);
}

UniqueRef<ResourceBase> ResourceManager::LoadResourceAsync(RESOURCE_TYPE type, StringView name)
{
    auto n = name.FindCharacter('#');
    if (n != -1)
        name = name.GetSubstring(0, n);

    File f = OpenFile(name);
    if (!f)
        return {};

    switch (type)
    {
        case RESOURCE_MESH:
            return MeshResource::sLoad(f);
        case RESOURCE_ANIMATION:
            return AnimationResource::sLoad(f);
        case RESOURCE_TEXTURE:
            return TextureResource::sLoad(f);
        case RESOURCE_MATERIAL:
            return MaterialResource::sLoad(f);
        case RESOURCE_SOUND:
            return SoundResource::sLoad(f);
        case RESOURCE_FONT:
            return FontResource::sLoad(f);
        case RESOURCE_TERRAIN:
            return TerrainResource::sLoad(f);
        default:
            break;
    }
    HK_ASSERT(0);
    return {};
}

void ResourceManager::UpdateAsync()
{
    while (m_RunAsync.Load())
    {
        ResourceID resource = m_StreamQueue.Dequeue();
        if (resource)
        {
            auto& proxy = GetProxy(resource);
            proxy.m_Resource = LoadResourceAsync(RESOURCE_TYPE(resource.GetType()), proxy.GetName());

            m_ProcessingQueue.Push(resource);
            m_ProcessingQueueEvent.Signal();
        }
        else
        {
            //LOG("Sleep\n");
            m_StreamQueueEvent.Wait();
            //LOG("Awake\n");
        }
    }
}

void ResourceManager::MainThread_Update(float timeBudget)
{
    HK_PROFILER_EVENT("ResourceManager::MainThread_Update");

    uint64_t time = Core::SysMicroseconds();

    ExecuteCommands();

    do
    {
        ResourceID resource;
        if (!m_ProcessingQueue.TryPop(resource))
            break;

        ResourceProxy& proxy = GetProxy(resource);

        if (proxy.HasData())
        {
            proxy.m_State = RESOURCE_STATE_READY;

            // Upload resource to gpu
            proxy.Upload();
        }
        else
        {
            proxy.m_State = RESOURCE_STATE_INVALID;
        }     

        //LOG("Processed {} {} [{}]\n", resource, proxy.GetName(), proxy.m_State == RESOURCE_STATE_READY ? "READY" : "INVALID");

        IncrementAreas(proxy);

        uint64_t curtime = Core::SysMicroseconds();

        uint64_t t = curtime - time;
        time = curtime;

        timeBudget -= t * 0.000001;
    } while (timeBudget > 0);

    for (auto it = m_DelayedRelease.begin(); it != m_DelayedRelease.end() ; )
    {
        ResourceID resource = *it;
        ResourceProxy& proxy = GetProxy(resource);
        if (proxy.m_State != RESOURCE_STATE_LOAD)
        {
            ReleaseResource(resource);
            it = m_DelayedRelease.Erase(it);
        }
        else
            it++;
    }
}

namespace
{

template <typename T>
HK_FORCEINLINE T FetchAdd(T& val, T const& add)
{
    T v = val;
    val += add;
    return v;
}

}

void ResourceManager::ExecuteCommands()
{
    m_Refs.Clear();
    m_ReloadResources.Clear();

    m_CommandBufferMutex.Lock();
    for (Command& command : m_CommandBuffer)
    {
        switch (command.Type)
        {
            case Command::CREATE_AREA:
            {
                ResourceArea* area = FetchArea(command.ResourceOrAreaID);
                for (ResourceID resource : area->m_ResourceList)
                {
                    ResourceProxy& proxy = GetProxy(resource);

                    //HK_ASSERT(!proxy.IsProcedural());

                    proxy.m_Areas.Add(area);
                    if (proxy.m_State == RESOURCE_STATE_READY || proxy.m_State == RESOURCE_STATE_INVALID)
                    {
                        area->m_ResourcesLoaded++;
                    }
                }
                break;
            }
            case Command::DESTROY_AREA:
            {
                ResourceArea* area = FetchArea(command.ResourceOrAreaID);

                // Remove area from resources
                for (ResourceID resource : area->m_ResourceList)
                {
                    ResourceProxy& proxy = GetProxy(resource);

                    //HK_ASSERT(!proxy.IsProcedural());

                    auto i = proxy.m_Areas.IndexOf(area);
                    if (i != Core::NPOS)
                        proxy.m_Areas.Remove(i);
                }
                FreeArea(command.ResourceOrAreaID);
                break;
            }
            case Command::LOAD_RESOURCE:
                m_Refs[ResourceID(command.ResourceOrAreaID)]++;
                break;
            case Command::UNLOAD_RESOURCE:
                m_Refs[ResourceID(command.ResourceOrAreaID)]--;
                break;
            case Command::LOAD_AREA:
            {
                ResourceArea* area = FetchArea(command.ResourceOrAreaID);
                if (!area->m_Load)
                {
                    for (ResourceID resource : area->m_ResourceList)
                        m_Refs[resource]++;

                    area->m_Load = true;
                }
                break;
            }
            case Command::UNLOAD_AREA:
            {
                ResourceArea* area = FetchArea(command.ResourceOrAreaID);
                if (area->m_Load)
                {
                    for (ResourceID resource : area->m_ResourceList)
                        m_Refs[resource]--;

                    area->m_Load = false;
                }
                break;
            }
            case Command::RELOAD_RESOURCE:
            {
                m_ReloadResources.Insert(ResourceID(command.ResourceOrAreaID));
                break;
            }
            case Command::RELOAD_AREA:
            {
                ResourceArea* area = FetchArea(command.ResourceOrAreaID);
                for (ResourceID resource : area->m_ResourceList)
                {
                    m_ReloadResources.Insert(resource);
                }
                break;
            }
        }
    }
    m_CommandBuffer.Clear();
    m_CommandBufferMutex.Unlock();

    bool signal = false;

    for (auto& pair : m_Refs)
    {
        ResourceID resource = pair.first;
        int refCount = pair.second;

        // skip bad request
        if (!resource)
            continue;

        if (refCount > 0)
        {
            ResourceProxy& proxy = GetProxy(resource);

            auto useCount = FetchAdd(proxy.m_UseCount, refCount);

            if (useCount == 0)
            {
                auto i = m_DelayedRelease.IndexOf(resource);
                if (i != Core::NPOS)
                {
                    m_DelayedRelease.RemoveUnsorted(i);
                }
                else
                {
                    if (proxy.m_State != RESOURCE_STATE_LOAD)
                    {
                        m_StreamQueue.Enqueue(resource);
                        signal = true;

                        proxy.m_State = RESOURCE_STATE_LOAD;

                        //LOG("Enqueued {} {}\n", resource, proxy.GetName());
                    }
                }
            }
        }
        else if (refCount < 0)
        {
            ResourceProxy& proxy = GetProxy(resource);

            proxy.m_UseCount += refCount;

            HK_ASSERT(proxy.m_UseCount >= 0);

            if (proxy.m_UseCount == 0)
            {
                // Check if resource was sent to loader thread
                if (proxy.m_State == RESOURCE_STATE_LOAD)
                {
                    m_DelayedRelease.Add(resource);
                }
                else
                {
                    ReleaseResource(resource);
                }
            }            
        }
    }

    for (ResourceID resource : m_ReloadResources)
    {
        ResourceProxy& proxy = GetProxy(resource);

        auto i = m_DelayedRelease.IndexOf(resource);
        if (i != Core::NPOS)
        {
            m_DelayedRelease.RemoveUnsorted(i);
        }

        switch (proxy.m_State)
        {
            case RESOURCE_STATE_LOAD:
                break;

            case RESOURCE_STATE_READY:
            case RESOURCE_STATE_INVALID:
            {
                proxy.Purge();
                proxy.m_State = RESOURCE_STATE_FREE;

                DecrementAreas(proxy);

                [[fallthrough]];
            }
            case RESOURCE_STATE_FREE:
            {
                m_StreamQueue.Enqueue(resource);
                signal = true;

                proxy.m_State = RESOURCE_STATE_LOAD;

                break;
            }
        }
    }

    if (signal)
        m_StreamQueueEvent.Signal();
}

void ResourceManager::ReleaseResource(ResourceID resource)
{
    ResourceProxy& proxy = GetProxy(resource);

    HK_ASSERT(proxy.m_State != RESOURCE_STATE_LOAD);

    proxy.Purge();
    proxy.m_State = RESOURCE_STATE_FREE;

    DecrementAreas(proxy);
}

void ResourceManager::PurgeResourceData(ResourceID resource)
{
    if (!resource)
        return;

    auto& proxy = GetProxy(resource);

    if (proxy.m_State == RESOURCE_STATE_LOAD)
    {
        LOG("ResourceManager::PurgeResourceData: A resource that is in loading state cannot be cleared {}\n", proxy.GetName());
        return;
    }

    proxy.Purge();
}

void ResourceManager::IncrementAreas(ResourceProxy& proxy)
{
    for (auto* area : proxy.m_Areas)
    {
        area->m_ResourcesLoaded++;
    }
}

void ResourceManager::DecrementAreas(ResourceProxy& proxy)
{
    for (auto* area : proxy.m_Areas)
    {
        area->m_ResourcesLoaded--;
    }
}

ResourceArea* ResourceManager::AllocateArea()
{
    ResourceArea* area = new ResourceArea;

    MutexGuard lock(m_ResourceAreaAllocMutex);

    if (!m_ResourceAreaFreeList.IsEmpty())
    {
        ResourceAreaID areaID = m_ResourceAreaFreeList.Last();
        m_ResourceAreaFreeList.RemoveLast();

        m_ResourceAreas[areaID] = area;
        area->m_Id = areaID;
    }
    else
    {
        ResourceAreaID areaID = m_ResourceAreas.Size();
        m_ResourceAreas.Add(area);
        area->m_Id = areaID;
    }

    return area;
}

void ResourceManager::FreeArea(ResourceAreaID areaID)
{
    MutexGuard lock(m_ResourceAreaAllocMutex);

    HK_ASSERT(m_ResourceAreas[areaID]);
    if (!m_ResourceAreas[areaID])
        return;

    m_ResourceAreaFreeList.Add(areaID);
    delete m_ResourceAreas[areaID];

    m_ResourceAreas[areaID] = nullptr;
}

ResourceArea* ResourceManager::FetchArea(ResourceAreaID areaID)
{
    MutexGuard lock(m_ResourceAreaAllocMutex);

    HK_ASSERT(areaID < m_ResourceAreas.Size() && m_ResourceAreas[areaID]);

    return areaID < m_ResourceAreas.Size() ? m_ResourceAreas[areaID] : nullptr;
}

void ResourceManager::AddCommand(Command const& command)
{
    MutexGuard lock(m_CommandBufferMutex);
    m_CommandBuffer.Add(command);
}

namespace
{

Vector<ResourceID> MakeUniqueList(ArrayView<ResourceID> resourceList)
{
    Vector<ResourceID> uniqueList(resourceList.begin(), resourceList.end());

    std::sort(uniqueList.begin(), uniqueList.end(),
        [](ResourceID a, ResourceID b)
        {
            return uint32_t(a) < uint32_t(b);
        });

    auto it = std::unique(uniqueList.begin(), uniqueList.end());
    uniqueList.Resize(std::distance(uniqueList.begin(), it));

    size_t size = uniqueList.Size();
    HK_UNUSED(size);

    return uniqueList;
}

}

ResourceAreaID ResourceManager::CreateResourceArea(ArrayView<ResourceID> resourceList)
{
    ResourceArea* area = AllocateArea();

    area->m_ResourceList = MakeUniqueList(resourceList);

    Command command;
    command.Type = Command::CREATE_AREA;
    command.ResourceOrAreaID = area->m_Id;

    AddCommand(command);
    
    return area->m_Id;
}

void ResourceManager::DestroyResourceArea(ResourceAreaID area)
{
    if (!area)
        return;

    UnloadArea(area);

    Command command;
    command.Type = Command::DESTROY_AREA;
    command.ResourceOrAreaID = area;

    AddCommand(command);
}

void ResourceManager::LoadArea(ResourceAreaID area)
{
    if (!area)
        return;

    Command command;
    command.Type = Command::LOAD_AREA;
    command.ResourceOrAreaID = area;

    AddCommand(command);
}

void ResourceManager::UnloadArea(ResourceAreaID area)
{
    if (!area)
        return;

    Command command;
    command.Type = Command::UNLOAD_AREA;
    command.ResourceOrAreaID = area;

    AddCommand(command);
}

void ResourceManager::ReloadArea(ResourceAreaID area)
{
    if (!area)
        return;

    Command command;
    command.Type = Command::RELOAD_AREA;
    command.ResourceOrAreaID = area;

    AddCommand(command);
}

bool ResourceManager::LoadResource(ResourceID resource)
{
    if (!resource.IsValid())
        return false;
    
    Command command;
    command.Type = Command::LOAD_RESOURCE;
    command.ResourceOrAreaID = resource;

    AddCommand(command);

    return true;
}

bool ResourceManager::UnloadResource(ResourceID resource)
{
    if (!resource.IsValid())
        return false;

    Command command;
    command.Type = Command::UNLOAD_RESOURCE;
    command.ResourceOrAreaID = resource;

    AddCommand(command);

    return true;
}

bool ResourceManager::ReloadResource(ResourceID resource)
{
    if (!resource.IsValid())
        return false;

    Command command;
    command.Type = Command::RELOAD_RESOURCE;
    command.ResourceOrAreaID = resource;

    AddCommand(command);

    return true;
}

bool ResourceManager::IsAreaReady(ResourceAreaID areaID)
{
    ResourceArea* area = FetchArea(areaID);
    return area->IsReady();
}

void ResourceManager::MainThread_WaitResourceArea(ResourceAreaID areaID)
{
    if (!areaID)
        return;

    ResourceArea* area = FetchArea(areaID);
    if (area->IsReady())
        return;
        
    for (;;)
    {
        MainThread_Update(std::numeric_limits<float>::infinity());

        if (area->IsReady())
            break;

        m_ProcessingQueueEvent.Wait();
    }
}

void ResourceManager::MainThread_WaitResource(ResourceID resource)
{
    if (!resource.IsValid())
        return;

    auto& proxy = GetProxy(resource);
    if (proxy.IsReady())
        return;

    for (;;)
    {
        MainThread_Update(std::numeric_limits<float>::infinity());

        if (proxy.IsReady())
            break;

        m_ProcessingQueueEvent.Wait();
    }
}

HK_NAMESPACE_END
