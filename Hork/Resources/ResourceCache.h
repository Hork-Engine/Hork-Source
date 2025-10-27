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

#pragma once

#include "ResourceRTTR.h"

#include <Hork/Core/IntrusiveRef.h>
#include <Hork/Core/Containers/Hash.h>

#include <mutex>

HK_NAMESPACE_BEGIN

class File;
class IBinaryStreamReadInterface;

class Resource;
using ResourceRef = IntrusiveRef<Resource>;

enum class ResourceLoad
{
    UseCachedResource,
    ForceReload
};

class ResourceCacheBase : public Noncopyable
{
public:
    virtual                 ~ResourceCacheBase();

    ResourceRef             Acquire(StringView name);

    ResourceRef             Load(StringView name, ResourceLoad load);

    ResourceRef             Find(StringView name);

    void                    SetFileOpenCallback(std::function<File(StringView)> callback);

    virtual std::unique_ptr<void, void(*)(void*)> BeginAsyncLoad(IBinaryStreamReadInterface& stream) = 0;

    virtual void            InitFromData(ResourceRef const& resource, std::unique_ptr<void, void(*)(void*)> data) = 0;

    void                    AddToPurgeQueue(Resource* resource);

    void                    PurgeUnusedResources();

protected:
                            ResourceCacheBase() = default;

    virtual Resource*       AllocateResource() = 0;

private:

    StringHashMap<Resource*>m_Resources;
    std::function<File(StringView)> m_FileOpenCallback;
    std::mutex              m_GarbageMutex;
    Resource*               m_GarbageHead = nullptr;
};

template <typename ResourceType>
class ResourceCache final : public ResourceCacheBase
{
public:
    IntrusiveRef<ResourceType> Acquire(StringView name);

    IntrusiveRef<ResourceType> Load(StringView name, ResourceLoad load);

    IntrusiveRef<ResourceType> Find(StringView name);

    std::unique_ptr<void, void(*)(void*)> BeginAsyncLoad(IBinaryStreamReadInterface& stream) override;

    void                    InitFromData(ResourceRef const& resource, std::unique_ptr<void, void(*)(void*)> data) override;

private:
    Resource*               AllocateResource() override;
};

HK_NAMESPACE_END

#include "ResourceCache.inl"