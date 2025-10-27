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

template <typename T>
ResourceFinder<T>::ResourceFinder(StringView name)
{
    HK_ASSERT(s_ResourceManager != nullptr);

    // Зарегистрировать ресурс и получить указатель на него
    m_Resource = s_ResourceManager->Acquire<T>(name).RawPtr();
}

template <typename T>
IntrusiveRef<T> ResourceFinder<T>::Acquire()
{
    // Нам не нужно искать указатель в кеше, так как мы сохраняем его в m_Resource
    return IntrusiveRef<T>(m_Resource);
}

template <typename T>
IntrusiveRef<T> ResourceFinder<T>::Load()
{
    if (m_Resource->IsPurged())
    {
        // Ресурс не загружен, загрузить его
        IntrusiveRef<T> resource(m_Resource);
        s_ResourceManager->ForceLoad(resource, resource->GetName());
        return resource;
    }
    else
    {
        // Ресурс уже загружен, возвращаем указатель на ресурс
        return IntrusiveRef<T>(m_Resource);
    }
}

template <typename T>
IntrusiveRef<T> ResourceFinder<T>::LoadAsync(uint32_t batchId, TaskPriority priority)
{
    if (m_Resource->IsPurged())
    {
        // Ресурс не загружен, загрузить его асинхронно
        IntrusiveRef<T> resource(m_Resource);
        s_ResourceManager->LoadAsync(batchId, resource, priority);
        return resource;
    }
    else
    {
        // Ресурс уже загружен, возвращаем указатель на ресурс
        return IntrusiveRef<T>(m_Resource);
    }
}

HK_NAMESPACE_END
