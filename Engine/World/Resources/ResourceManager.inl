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

HK_NAMESPACE_BEGIN

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
ResourceHandle<T> ResourceManager::CreateResourceWithData(StringView name, UniqueRef<T> resourceData)
{
    ResourceHandle<T> resource = GetResource<T>(name);
    if (!resource)
        return {};

    auto& proxy = GetProxy(resource);

    if (proxy.m_State == RESOURCE_STATE_LOAD)
    {
        LOG("ResourceManager::CreateResourceWithData: A resource that is in loading state cannot be created {}\n", name);
        return {};
    }

    proxy.m_Resource = std::move(resourceData);
    proxy.m_State = RESOURCE_STATE_READY;
    proxy.m_Flags = RESOURCE_FLAG_PROCEDURAL;

    if (proxy.m_UseCount == 0)
    {
        // Increment usage counter only on first creation.
        proxy.m_UseCount++;
        IncrementAreas(proxy);
    }

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
    if (auto file = OpenFile(path))
    {
        UniqueRef<T> resource = T::sLoad(file);
        if (resource)
            return CreateResourceWithData<T>(path, std::move(resource));
    }
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

HK_NAMESPACE_END
