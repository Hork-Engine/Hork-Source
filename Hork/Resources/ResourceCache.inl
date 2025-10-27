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

HK_NAMESPACE_BEGIN

template <typename ResourceType>
IntrusiveRef<ResourceType> ResourceCache<ResourceType>::Acquire(StringView name)
{
    return static_pointer_cast<ResourceType>(ResourceCacheBase::Acquire(name));
}

template <typename ResourceType>
IntrusiveRef<ResourceType> ResourceCache<ResourceType>::Load(StringView name, ResourceLoad load)
{
    return static_pointer_cast<ResourceType>(ResourceCacheBase::Load(name, load));
}

template <typename ResourceType>
IntrusiveRef<ResourceType> ResourceCache<ResourceType>::Find(StringView name)
{
    return static_pointer_cast<ResourceType>(ResourceCacheBase::Find(name));
}

template <typename ResourceType>
std::unique_ptr<void, void(*)(void*)> ResourceCache<ResourceType>::BeginAsyncLoad(IBinaryStreamReadInterface& stream)
{
    UniqueRef<ResourceType::DataType> data = ResourceType::BeginAsyncLoad(stream);
    std::unique_ptr<void, void(*)(void*)> p(data.Detach(),
        [](void* ptr) {
            delete static_cast<ResourceType::DataType*>(ptr);
        });
    return p;
}

template <typename ResourceType>
void ResourceCache<ResourceType>::InitFromData(ResourceRef const& resource, std::unique_ptr<void, void(*)(void*)> data)
{
    UniqueRef<ResourceType::DataType> specialized(static_cast<ResourceType::DataType*>(data.release()));

    static_pointer_cast<ResourceType>(resource)->InitFromData(std::move(specialized));
}

template <typename ResourceType>
Resource* ResourceCache<ResourceType>::AllocateResource()
{
    // TODO: Use allocator (pool?)
    return new ResourceType;
}

HK_NAMESPACE_END
