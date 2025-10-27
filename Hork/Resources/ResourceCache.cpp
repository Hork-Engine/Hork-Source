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

#include "ResourceCache.h"
#include "Resource.h"
#include "ResourceManager.h"

#include <Hork/Core/Logger.h>

#include <mutex>

HK_NAMESPACE_BEGIN

ResourceCacheBase::~ResourceCacheBase()
{
    for (auto& pair : m_Resources)
    {
        Resource* resource = pair.second;
        if (resource->UseCount() > 0) // Кто-то все еще держит ресурс (возможно глобальные или статические переменные)
        {
            // Очищаем ресурс. Важно сделать это сейчас, а не когда движок будет деинициализирован.
            resource->Purge();

            // Отсоединяем от кеша, чтобы при удалении (на RemoveRef) объект не обращался в кеш, а просто удалялся через delete.
            resource->m_Cache = nullptr;
        }
        else
        {
            // Удаляем ресурс. Purge() не обязателен, т.к. все данные ресурса будут удалены в деструкторе.
            delete pair.second;
        }
    }
}

void ResourceCacheBase::SetFileOpenCallback(std::function<File(StringView)> callback)
{
    m_FileOpenCallback = std::move(callback);
}

ResourceRef ResourceCacheBase::Load(StringView name, ResourceLoad load)
{
    auto resource = Acquire(name);

    if (resource->IsPurged() || load == ResourceLoad::ForceReload)
    {
        // load from file
        if (m_FileOpenCallback)
        {
            auto file = m_FileOpenCallback(name);
            if (file)
            {
                resource->Load(file);
            }
        }
    }

    return resource;
}

ResourceRef ResourceCacheBase::Find(StringView name)
{
    auto it = m_Resources.Find(name);
    if (it == m_Resources.End())
        return nullptr;
    return ResourceRef(it->second);
}

ResourceRef ResourceCacheBase::Acquire(StringView name)
{
    auto resource = Find(name);
    if (!resource)
    {
        resource.Reset(AllocateResource()); 

        auto result = m_Resources.Insert(name, resource.RawPtr());
        resource->m_Cache = this;
        resource->m_Name = result.first.get_node()->mValue.first.Begin();
    }
    return resource;
}

void ResourceCacheBase::PurgeUnusedResources()
{
    std::lock_guard lock(m_GarbageMutex);

    while (m_GarbageHead)
    {
        Resource* next = m_GarbageHead->m_NextGarbage;

        // Race condition, but it's okay for our case
        if (m_GarbageHead->UseCount() == 0)
        {
            m_GarbageHead->Purge();
        }
        else
        {
            // Объект воскрешен
            //LOG("Resource {} is resurrected\n", m_GarbageHead->GetName());
        }

        m_GarbageHead->m_InGarbageList = false;
        m_GarbageHead->m_NextGarbage = nullptr;

        m_GarbageHead = next;
    }
}

void ResourceCacheBase::AddToPurgeQueue(Resource* resource)
{
    std::lock_guard lock(m_GarbageMutex);

    if (!resource->m_InGarbageList)
    {
        //LOG("AddToPurgeQueue {}\n", resource->GetName());

        resource->m_InGarbageList = true;
        resource->m_NextGarbage = m_GarbageHead;
        m_GarbageHead = resource;
    }
}

HK_NAMESPACE_END
