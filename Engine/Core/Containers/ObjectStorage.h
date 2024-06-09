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

HK_NAMESPACE_BEGIN

enum class ObjectStorageType
{
    Compact,
    Sparse
};

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
class ObjectStorage final
{
public:
                    ObjectStorage();
                    ObjectStorage(ObjectStorage const& rhs) = delete;
                    ObjectStorage(ObjectStorage&& rhs) noexcept;
                    ~ObjectStorage();

                    ObjectStorage& operator=(ObjectStorage& rhs) = delete;
                    ObjectStorage& operator=(ObjectStorage&& rhs) noexcept;

    Handle32<T>     CreateObject(T*& newObject);

    template <typename HandleFetcher>
    void            DestroyObject(Handle32<T> handle);

    template <typename HandleFetcher>
    void            DestroyObject(Handle32<T> handle, T*& movedObject);

    T*              GetObject(Handle32<T> handle);

    T const*        GetObject(Handle32<T> handle) const;

    void            Clear();

    bool            IsEmpty() const;

    uint32_t        Size() const;

    uint32_t        Capacity() const;

    uint32_t        GetPageCount() const;

    static constexpr size_t GetPageSize() { return PageSize; }

    template <typename Visitor>
    void            Iterate(Visitor& visitor);
    template <typename Visitor>
    void            Iterate(Visitor& visitor) const;

    template <typename Visitor>
    void            IterateBatches(Visitor& visitor);
    template <typename Visitor>
    void            IterateBatches(Visitor& visitor) const;

    struct Iterator
    {
    private:
        uint32_t    m_Index;
        uint32_t    m_EndIndex;
        ObjectStorage<T, PageSize, StorageType>& m_Storage;

    public:
                    Iterator(uint32_t startIndex, uint32_t endIndex, ObjectStorage<T, PageSize, StorageType>& storage);

        T*          operator->();

                    operator T*();

        bool        IsValid() const;

        uint32_t    GetIndex() const;

        void        operator++();
    };

    struct ConstIterator
    {
    private:
        uint32_t    m_Index;
        uint32_t    m_EndIndex;
        ObjectStorage<T, PageSize, StorageType> const& m_Storage;

    public:
                    ConstIterator(uint32_t startIndex, uint32_t endIndex, ObjectStorage<T, PageSize, StorageType> const& storage);

        T const*    operator->() const;

                    operator T const*() const;

        bool        IsValid() const;

        uint32_t    GetIndex() const;

        void        operator++();
    };

    Iterator        GetObjects();
    ConstIterator   GetObjects() const;

    Vector<T*> const& GetRandomAccessTable() const;

private:
    template <typename Visitor, bool IsConst>
    void            _Iterate(Visitor& visitor);

    template <typename Visitor, bool IsConst>
    void            _IterateBatches(Visitor& visitor);

    PageAllocator<PageSize> m_Data;
    Vector<T*>      m_RandomAccess;
    uint32_t        m_Size = 0;
    uint32_t        m_FreeListHead = 0;
};

HK_NAMESPACE_END

#include "ObjectStorage.inl"
