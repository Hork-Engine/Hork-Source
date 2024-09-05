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

#include <Hork/Core/IO.h>
#include <Hork/Core/Containers/ArrayView.h>
#include <Hork/Core/Containers/PagedVector.h>
#include <Hork/Core/Containers/Hash.h>

#include "ResourceHandle.h"
#include "ResourceProxy.h"

#include "ThreadSafeQueue.h"

HK_NAMESPACE_BEGIN

using ResourceAreaID = uint32_t;

struct ResourceArea;

class ResourceManager final : public Noncopyable
{
public:
                            ResourceManager();
                            ~ResourceManager();

    /// Adds resource pack. Not thread safe.
    void                    AddResourcePack(StringView fileName);

    Vector<Archive> const&  GetResourcePacks() const { return m_ResourcePacks; }

    ResourceAreaID          CreateResourceArea(ArrayView<ResourceID> resourceList);
    void                    DestroyResourceArea(ResourceAreaID area);

    void                    LoadArea(ResourceAreaID area);
    void                    UnloadArea(ResourceAreaID area);
    void                    ReloadArea(ResourceAreaID area);

    bool                    LoadResource(ResourceID resource);
    bool                    UnloadResource(ResourceID resource);
    bool                    ReloadResource(ResourceID resource);

    /// Enques a resource to unload, increases the usage counter
    template <typename T>
    ResourceHandle<T>       LoadResource(StringView name);

    /// Enques a resource to unload, decreases the usage counter, unloads if the usage counter == 0
    template <typename T>
    void                    UnloadResource(StringView name);

    /// Creates a resource in place, replacing resource data, does not increase the usage counter.
    template <typename T>
    ResourceHandle<T>       CreateResourceWithData(StringView name, UniqueRef<T> resourceData);

    /// Creates a resource in place, replacing resource data, does not increase the usage counter.
    template <typename T, typename... Args>
    ResourceHandle<T>       CreateResource(StringView name, Args&&... args);

    /// Creates a resource in place, loads and replacing resource data, does not increase the usage counter.
    template <typename T>
    ResourceHandle<T>       CreateResourceFromFile(StringView path);

    /// Just frees the resource data, does not change the state of the resource.
    void                    PurgeResourceData(ResourceID resource);

    bool                    IsAreaReady(ResourceAreaID area);

    /// Wait for the resources to load. Can be called only from main thread.
    void                    MainThread_WaitResourceArea(ResourceAreaID area);

    /// Wait for the resources to load. Can be called only from main thread.
    void                    MainThread_WaitResource(ResourceID resource);

    template <typename T>
    ResourceHandle<T>       GetResource(StringView resourcePath);

    ResourceProxy*          FindResource(StringView resourcePath);

    template <typename T>
    T*                      TryGet(ResourceID resource);

    template <typename T>
    T*                      TryGet(ResourceHandle<T> handle);

    ResourceProxy&          GetProxy(ResourceID resource);

    StringView              GetResourceName(ResourceID resource);

    bool                    IsResourceReady(ResourceID resource);

    // Called per frame
    void                    MainThread_Update(float timeBudget);

    File                    OpenFile(StringView path);

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
        TYPE        Type;
        uint32_t    ResourceOrAreaID;
    };

    void                    UpdateAsync();

    UniqueRef<ResourceBase> LoadResourceAsync(RESOURCE_TYPE type, StringView name);

    /// Find file in resource packs
    bool                    FindFile(StringView fileName, int* pResourcePackIndex, FileHandle* pFileHandle) const;

    ResourceArea*           AllocateArea();
    void                    FreeArea(ResourceAreaID areaID);
    ResourceArea*           FetchArea(ResourceAreaID areaID);

    void                    AddCommand(Command const& command);

    void                    ExecuteCommands();

    void                    ReleaseResource(ResourceID resource);

    void                    IncrementAreas(ResourceProxy& proxy);
    void                    DecrementAreas(ResourceProxy& proxy);

    using ResourceList = PagedVector<ResourceProxy, 1024, 1024>;
    ResourceList            m_ResourceList;
    StringHashMap<ResourceID> m_ResourceHash;
    Mutex                   m_ResourceHashMutex;

    Vector<ResourceID>      m_DelayedRelease;

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

    ResourceStreamQueue     m_StreamQueue;
    ThreadSafeQueue<ResourceID> m_ProcessingQueue;
    SyncEvent               m_StreamQueueEvent;
    SyncEvent               m_ProcessingQueueEvent;

    Vector<ResourceArea*>   m_ResourceAreas;
    Vector<uint32_t>        m_ResourceAreaFreeList;
    Mutex                   m_ResourceAreaAllocMutex;

    Vector<Command>         m_CommandBuffer;
    Mutex                   m_CommandBufferMutex;    

    HashMap<ResourceID, int> m_Refs;
    HashSet<ResourceID>     m_ReloadResources;

    Thread                  m_Thread;
    AtomicBool              m_RunAsync;

    Vector<Archive>         m_ResourcePacks;
};

HK_NAMESPACE_END

#include "ResourceManager.inl"
