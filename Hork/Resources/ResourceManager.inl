/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

HK_NAMESPACE_BEGIN

template <typename ResourceType>
ResourceTaskID ResourceManager::LoadAsync(uint32_t batchId, IntrusiveRef<ResourceType> resource, StringView name, TaskPriority priority)
{
    RegisterResourceType<ResourceType>();
    return ResourceTaskID(LoadAsyncImpl(batchId, ResourceRTTR::TypeID<ResourceType>, resource, name, priority));
}

template <typename ResourceType>
ResourceTaskID ResourceManager::LoadAsync(uint32_t batchId, IntrusiveRef<ResourceType> resource, TaskPriority priority)
{
    return LoadAsync(batchId, resource, resource->GetName(), priority);
}

template <typename ResourceType>
IntrusiveRef<ResourceType> ResourceManager::LoadAsync(uint32_t batchId, StringView name, TaskPriority priority)
{
    auto resource = Acquire<ResourceType>(name);
    if (resource->IsPurged())
    {
        LoadAsync(batchId, resource, name, priority);
    }
    return resource;
}

template <typename ResourceType>
void ResourceManager::RegisterResourceType()
{
    ResourceTypeID typeID = ResourceRTTR::TypeID<ResourceType>;

    if (!m_Caches[typeID])
    {
        m_Caches[typeID].Attach(new ResourceCache<ResourceType>);
        m_Caches[typeID]->SetFileOpenCallback([this](StringView name) { return OpenFile(name); });
    }
}

template <typename ResourceType>
HK_INLINE ResourceCache<ResourceType>& ResourceManager::GetCache()
{
    ResourceTypeID typeID = ResourceRTTR::TypeID<ResourceType>;

    RegisterResourceType<ResourceType>();
    return *static_cast<ResourceCache<ResourceType>*>(m_Caches[typeID].RawPtr());
}

HK_NAMESPACE_END
