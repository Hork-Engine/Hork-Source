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

#include <Engine/Core/Handle.h>
#include <Engine/Core/Allocators/PageAllocator.h>
#include <Engine/Core/Containers/PageStorage.h>
#include <Engine/Core/CoreApplication.h>

HK_NAMESPACE_BEGIN

/*
TODO:

Add:
enum class ObjectStorageType
{
    Compact,
    Sparse
};

*/

template <typename T>
class ObjectStorage final
{
public:
                    ObjectStorage() = default;
                    ObjectStorage(ObjectStorage const& rhs) = delete;
                    ObjectStorage(ObjectStorage&& rhs) noexcept;
                    ~ObjectStorage();

    ObjectStorage&  operator=(ObjectStorage& rhs) = delete;
    ObjectStorage&  operator=(ObjectStorage&& rhs) noexcept;

    Handle32<T>     CreateObject(T*& newObject);

    template <typename HandleFetcher>
    void            DestroyObject(Handle32<T> handle);

    template <typename HandleFetcher>
    void            DestroyObject(Handle32<T> handle, T*& movedObject);

    T*              GetObject(Handle32<T> handle);

    T const*        GetObject(Handle32<T> handle) const;

    uint32_t        Size() const;

    T*              At(uint32_t index);

    T const*        At(uint32_t index) const;

    template <typename Visitor>
    void            Iterate(Visitor& visitor);

    template <typename Visitor>
    void            IterateBatches(Visitor& visitor);

    struct Iterator
    {
    private:
        uint32_t                m_Index;
        uint32_t                m_EndIndex;
        PageStorage<T>&         m_PageStorage;

    public:
                    Iterator(uint32_t startIndex, uint32_t endIndex, PageStorage<T>& pageStorage);

        T&          operator*();

        T*          operator->();

                    operator T*();

        bool        IsValid() const;

        uint32_t    GetIndex() const;

        void        operator++();
    };

    struct ConstIterator
    {
    private:
        uint32_t                m_Index;
        uint32_t                m_EndIndex;
        PageStorage<T> const&   m_PageStorage;

    public:
                    ConstIterator(uint32_t startIndex, uint32_t endIndex, PageStorage<T> const& pageStorage);

        T const&    operator*() const;

        T const*    operator->() const;

                    operator T const*() const;

        bool        IsValid() const;

        uint32_t    GetIndex() const;

        void        operator++();
    };

    Iterator        GetObjects();
    ConstIterator   GetObjects() const;

    Vector<T*> const& GetRandomAccessTable() const
    {
        return m_RandomAccess;
    }

private:
    PageStorage<T>          m_PageStorage;
    Vector<T*>             m_RandomAccess;
    uint32_t                m_FreeListHead = 0;
};

HK_NAMESPACE_END

#include "ObjectStorage.inl"
