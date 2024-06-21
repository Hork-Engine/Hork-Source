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

#include <Engine/Core/CoreApplication.h>

HK_NAMESPACE_BEGIN

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>::ObjectStorage() :
    m_Data(sizeof(T))
{}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>::ObjectStorage(ObjectStorage&& rhs) noexcept :
    m_Data(std::move(rhs.m_Data)),
    m_Size(rhs.m_Size),
    m_RandomAccess(std::move(rhs.m_RandomAccess)),
    m_FreeListHead(rhs.m_FreeListHead)
{
    rhs.m_Size = 0;
    rhs.m_FreeListHead = 0;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>::~ObjectStorage()
{
    Clear();
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>& ObjectStorage<T, PageSize, StorageType>::operator=(ObjectStorage&& rhs) noexcept
{
    m_Data = std::move(rhs.m_Data);
    m_Size = rhs.m_Size;
    m_RandomAccess = std::move(rhs.m_RandomAccess);
    m_FreeListHead = rhs.m_FreeListHead;
    rhs.m_Size = 0;
    rhs.m_FreeListHead = 0;
    return *this;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_INLINE Handle32<T> ObjectStorage<T, PageSize, StorageType>::CreateObject(T*& newObject)
{
    static_assert(sizeof(T) >= sizeof(m_FreeListHead), "The size of the object must be greater than or equal to sizeof(uint32_t)");

    if (m_FreeListHead)
    {
        auto freeHandle = Handle32<T>(m_FreeListHead);
        auto freeHandleID = freeHandle.GetID();

        auto version = freeHandle.GetVersion() + 1;
        if (version >= Handle32<T>::MAX_VERSION)
            version = 1;

        void* address;

        if constexpr (StorageType == ObjectStorageType::Compact)
        {
            address = m_Data.GetAddress(m_Size);
        }
        else
        {
            address = m_Data.GetAddress(freeHandleID);
        }

        m_FreeListHead = *reinterpret_cast<uint32_t*>(address);

        ++m_Size;

        newObject = new (address) T();

        HK_ASSERT(m_RandomAccess[freeHandleID] == nullptr);
        m_RandomAccess[freeHandleID] = newObject;

        return Handle32<T>(freeHandleID, version);
    }

    uint32_t handleID = m_Size;
    if (handleID >= Handle32<T>::MAX_ID)
        CoreApplication::TerminateWithError("ObjectStorage::Create: Too many objects allocated\n");

    m_Data.Grow(m_Size + 1);
    newObject = new (m_Data.GetAddress(m_Size++)) T();

    m_RandomAccess.Add(newObject);

    return Handle32<T>(handleID, 1);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename HandleFetcher>
HK_INLINE void ObjectStorage<T, PageSize, StorageType>::DestroyObject(Handle32<T> handle)
{
    T* movedObject;
    DestroyObject<HandleFetcher>(handle, movedObject);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename HandleFetcher>
HK_INLINE void ObjectStorage<T, PageSize, StorageType>::DestroyObject(Handle32<T> handle, T*& movedObject)
{
    uint32_t handleID = handle.GetID();
    HK_ASSERT(m_RandomAccess[handleID]);

    if constexpr (StorageType == ObjectStorageType::Compact)
    {
        movedObject = std::launder(reinterpret_cast<T*>(m_Data.GetAddress(--m_Size)));

        uint32_t movedObjectID = HandleFetcher::FetchHandle(movedObject).GetID();

        if constexpr (!std::is_trivial_v<T>)
        {
            m_RandomAccess[handleID]->~T();

            if (m_RandomAccess[handleID] != movedObject)
            {
                m_RandomAccess[movedObjectID] = new (m_RandomAccess[handleID]) T(std::move(*movedObject));
                movedObject->~T();
            }
            else
                movedObject = nullptr;
        }
        else
        {
            if (m_RandomAccess[handleID] != movedObject)
            {
                Core::Memcpy(m_RandomAccess[handleID], movedObject, sizeof(T));
                m_RandomAccess[movedObjectID] = m_RandomAccess[handleID];
            }
            else
                movedObject = nullptr;
        }

        m_RandomAccess[handleID] = nullptr;

        *reinterpret_cast<uint32_t*>(m_Data.GetAddress(m_Size)) = m_FreeListHead;
        m_FreeListHead = handle.ToUInt32();
    }
    else
    {
        if constexpr (!std::is_trivial_v<T>)
        {
            m_RandomAccess[handleID]->~T();
        }

        m_RandomAccess[handleID] = nullptr;

        movedObject = nullptr;

        *reinterpret_cast<uint32_t*>(m_Data.GetAddress(handleID)) = m_FreeListHead;
        m_FreeListHead = handle.ToUInt32();

        --m_Size;
    }
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE T* ObjectStorage<T, PageSize, StorageType>::GetObject(Handle32<T> handle)
{
    return m_RandomAccess[handle.GetID()];
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE T const* ObjectStorage<T, PageSize, StorageType>::GetObject(Handle32<T> handle) const
{
    return m_RandomAccess[handle.GetID()];
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::Clear()
{
    if constexpr (std::is_trivially_destructible_v<T>)
    {
        m_Size = 0;
    }
    else
    {
        if constexpr (StorageType == ObjectStorageType::Compact)
        {
            uint32_t i{};
            while (m_Size)
            {
                T* object = std::launder(reinterpret_cast<T*>(m_Data.GetAddress(i++)));
                object->~T();
                --m_Size;
            }
        }
        else
        {
            for (T* object : m_RandomAccess)
            {
                if (object)
                {
                    object->~T();
                    if (--m_Size == 0)
                        break;
                }
            }
        }
    }

    m_Data.Shrink(0);
    m_RandomAccess.Clear();
    m_FreeListHead = 0;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE bool ObjectStorage<T, PageSize, StorageType>::IsEmpty() const
{
    return m_Size == 0;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE uint32_t ObjectStorage<T, PageSize, StorageType>::Size() const
{
    return m_Size;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE uint32_t ObjectStorage<T, PageSize, StorageType>::Capacity() const
{
    return m_Data.GetPageCount() * PageSize;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE uint32_t ObjectStorage<T, PageSize, StorageType>::GetPageCount() const
{
    return m_Data.GetPageCount();
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename Visitor, bool IsConst>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::_Iterate(Visitor& visitor)
{
    if constexpr (StorageType == ObjectStorageType::Compact)
    {
        uint32_t processed = 0;
        uint32_t pageIndex = 0;
        while (processed < m_Size)
        {
            T* pageData = reinterpret_cast<T*>(m_Data.GetPageAddress(pageIndex++));

            uint32_t remaining = m_Size - processed;
            if (remaining < PageSize)
            {
                for (uint32_t i = 0; i < remaining; ++i)
                    visitor.Visit(pageData[i]);
                processed += remaining;
            }
            else
            {
                for (uint32_t i = 0; i < PageSize; ++i)
                {
                    if constexpr (IsConst)
                        visitor.Visit(const_cast<T const&>(pageData[i]));
                    else
                        visitor.Visit(pageData[i]);
                }
                processed += PageSize;
            }
        }
    }
    else
    {
        uint32_t processed = 0;
        uint32_t index = 0;
        while (processed < m_Size)
        {
            T* object = m_RandomAccess[index++];
            if (object)
            {
                if constexpr (IsConst)
                    visitor.Visit(const_cast<T const&>(*object));
                else
                    visitor.Visit(*object);
                ++processed;
            }
        }
    }
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename Visitor, bool IsConst>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::_IterateBatches(Visitor& visitor)
{
    if constexpr (StorageType == ObjectStorageType::Compact)
    {
        uint32_t processed = 0;
        uint32_t pageIndex = 0;
        while (processed < m_Size)
        {
            T* pageData = reinterpret_cast<T*>(m_Data.GetPageAddress(pageIndex++));

            uint32_t remaining = m_Size - processed;
            if (remaining < PageSize)
            {
                if constexpr (IsConst)
                    visitor.Visit(const_cast<T const*>(pageData), remaining);
                else
                    visitor.Visit(pageData, remaining);
                processed += remaining;
            }
            else
            {
                if constexpr (IsConst)
                    visitor.Visit(const_cast<T const*>(pageData), PageSize);
                else
                    visitor.Visit(pageData, PageSize);
                processed += PageSize;
            }
        }
    }
    else
    {
        uint32_t processed = 0;
        uint32_t index = 0;
        uint32_t total = m_RandomAccess.Size();
        while (processed < m_Size)
        {
            HK_ASSERT((index % PageSize) == 0);

            uint32_t end = index + PageSize;
            if (end > total)
                end = total;

            while (index != end && processed < m_Size)
            {
                uint32_t firstIndex = index;
                while (index != end && m_RandomAccess[index])
                {
                    ++index;
                }
                uint32_t batchSize = index - firstIndex;
                if (batchSize > 0)
                {
                    if constexpr (IsConst)
                        visitor.Visit(const_cast<T const*>(m_RandomAccess[firstIndex]), batchSize);
                    else
                        visitor.Visit(m_RandomAccess[firstIndex], batchSize);
                    processed += batchSize;
                }
                else
                {
                    ++index;
                }
            }
        }
    }
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename Visitor>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::Iterate(Visitor& visitor)
{
    _Iterate<Visitor, false>(visitor);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename Visitor>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::Iterate(Visitor& visitor) const
{
    const_cast<ObjectStorage<T, PageSize, StorageType>*>(this)->_Iterate<Visitor, true>(visitor);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename Visitor>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::IterateBatches(Visitor& visitor)
{
    _IterateBatches<Visitor, false>(visitor);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
template <typename Visitor>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::IterateBatches(Visitor& visitor) const
{
    const_cast<ObjectStorage<T, PageSize, StorageType>*>(this)->_IterateBatches<Visitor, true>(visitor);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE typename ObjectStorage<T, PageSize, StorageType>::Iterator ObjectStorage<T, PageSize, StorageType>::GetObjects()
{
    if constexpr (StorageType == ObjectStorageType::Compact)
        return Iterator(0, m_Size, *this);
    else
        return Iterator(0, m_RandomAccess.Size(), *this);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE typename ObjectStorage<T, PageSize, StorageType>::ConstIterator ObjectStorage<T, PageSize, StorageType>::GetObjects() const
{
    if constexpr (StorageType == ObjectStorageType::Compact)
        return ConstIterator(0, m_Size, *this);
    else
        return ConstIterator(0, m_RandomAccess.Size(), *this);
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>::Iterator::Iterator(uint32_t startIndex, uint32_t endIndex, ObjectStorage<T, PageSize, StorageType>& storage) :
    m_Index(startIndex), m_EndIndex(endIndex), m_Storage(storage)
{}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE T* ObjectStorage<T, PageSize, StorageType>::Iterator::operator->()
{
    if constexpr (StorageType == ObjectStorageType::Compact)
        return reinterpret_cast<T*>(m_Storage.m_Data.GetAddress(m_Index));
    else
        return m_Storage.m_RandomAccess[m_Index];
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>::Iterator::operator T*()
{
    if constexpr (StorageType == ObjectStorageType::Compact)
        return reinterpret_cast<T*>(m_Storage.m_Data.GetAddress(m_Index));
    else
        return m_Storage.m_RandomAccess[m_Index];
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE bool ObjectStorage<T, PageSize, StorageType>::Iterator::IsValid() const
{
    return m_Index < m_EndIndex;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE uint32_t ObjectStorage<T, PageSize, StorageType>::Iterator::GetIndex() const
{
    return m_Index;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::Iterator::operator++()
{
    ++m_Index;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>::ConstIterator::ConstIterator(uint32_t startIndex, uint32_t endIndex, ObjectStorage<T, PageSize, StorageType> const& storage) :
    m_Index(startIndex), m_EndIndex(endIndex), m_Storage(storage)
{}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE T const* ObjectStorage<T, PageSize, StorageType>::ConstIterator::operator->() const
{
    if constexpr (StorageType == ObjectStorageType::Compact)
        return reinterpret_cast<T*>(m_Storage.m_Data.GetAddress(m_Index));
    else
        return m_Storage.m_RandomAccess[m_Index];
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE ObjectStorage<T, PageSize, StorageType>::ConstIterator::operator T const*() const
{
    if constexpr (StorageType == ObjectStorageType::Compact)
        return reinterpret_cast<T*>(m_Storage.m_Data.GetAddress(m_Index));
    else
        return m_Storage.m_RandomAccess[m_Index];
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE bool ObjectStorage<T, PageSize, StorageType>::ConstIterator::IsValid() const
{
    return m_Index < m_EndIndex;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE uint32_t ObjectStorage<T, PageSize, StorageType>::ConstIterator::GetIndex() const
{
    return m_Index;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE void ObjectStorage<T, PageSize, StorageType>::ConstIterator::operator++()
{
    ++m_Index;
}

template <typename T, uint32_t PageSize, ObjectStorageType StorageType>
HK_FORCEINLINE Vector<T*> const& ObjectStorage<T, PageSize, StorageType>::GetRandomAccessTable() const
{
    return m_RandomAccess;
}

HK_NAMESPACE_END