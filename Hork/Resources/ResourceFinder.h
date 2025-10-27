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

#include <Hork/Resources/ResourceManager.h>

HK_NAMESPACE_BEGIN

class ResourceFinderBase
{
public:
    static void         SetResourceManager(ResourceManager* resourceMngr);

protected:
    static ResourceManager* s_ResourceManager;
};

template <typename T>
class ResourceFinder final : private ResourceFinderBase
{
public:
    explicit            ResourceFinder(StringView name);

    /// Возвращает указатель на ресурс, не гарантирует, что ресурс загружен
    IntrusiveRef<T>     Acquire();

    /// Возвращает указатель на ресурс, загружает если ресурс еще не загружен
    IntrusiveRef<T>     Load();

    /// Возвращает указатель на ресурс, загружает асинхронно если ресурс еще не загружен
    IntrusiveRef<T>     LoadAsync(uint32_t batchId = BATCH_DEFAULT, TaskPriority priority = TaskPriority::Normal);

private:
    T*                  m_Resource;
};

HK_NAMESPACE_END

#include "ResourceFinder.inl"
