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

#include <Engine/Core/Allocators/PageAllocator.h>

HK_NAMESPACE_BEGIN

template <typename T, size_t PageSize = 64>
class PageStorage final : public Noncopyable
{
    PageAllocator<PageSize> m_Data;
    uint32_t                m_Size{};

public:
                            PageStorage();
                            PageStorage(PageStorage&& rhs) noexcept;
                            ~PageStorage();

    PageStorage&            operator=(PageStorage&& rhs) noexcept;

    T&                      operator[](uint32_t index);

    T const&                operator[](uint32_t index) const;

    bool                    IsEmpty() const;

    void                    Clear();

    uint32_t                Size() const;

    uint32_t                Capacity() const;

    uint32_t                GetPageCount() const;

    T*                      GetPageData(uint32_t pageIndex) const;

    void*                   GetAddress(uint32_t index);
    void const*             GetAddress(uint32_t index) const;

    void                    Reserve(uint32_t capacity);

    void                    Resize(uint32_t size);

    template <typename... Args>
    T*                      EmplaceBack(Args&&...);

    void                    PushBack(T const& value);

    void                    PopBack();

    void                    ShrinkToFit();

    static constexpr size_t GetPageSize();

    template <typename Visitor>
    void                    Iterate(Visitor& visitor);

    template <typename Visitor>
    void                    IterateBatches(Visitor& visitor);
};


template <typename T, size_t PageSize>
HK_FORCEINLINE PageStorage<T, PageSize>::PageStorage() :
    m_Data(sizeof(T))
{}

template <typename T, size_t PageSize>
HK_FORCEINLINE PageStorage<T, PageSize>::PageStorage(PageStorage&& rhs) noexcept :
    m_Data(std::move(rhs.m_Data)),
    m_Size(rhs.m_Size)
{
    rhs.m_Size = 0;
}

template <typename T, size_t PageSize>
HK_FORCEINLINE PageStorage<T, PageSize>::~PageStorage()
{
    Clear();
}

template <typename T, size_t PageSize>
HK_FORCEINLINE PageStorage<T, PageSize>& PageStorage<T, PageSize>::operator=(PageStorage&& rhs) noexcept
{
    m_Data = std::move(rhs.m_Data);
    m_Size = rhs.m_Size;
    rhs.m_Size = 0;
    return *this;
}

template <typename T, size_t PageSize>
HK_FORCEINLINE T& PageStorage<T, PageSize>::operator[](uint32_t index)
{
    HK_ASSERT(index < m_Size);
    return *std::launder(reinterpret_cast<T*>(m_Data.GetAddress(index)));
}

template <typename T, size_t PageSize>
HK_FORCEINLINE T const& PageStorage<T, PageSize>::operator[](uint32_t index) const
{
    HK_ASSERT(index < m_Size);
    return *std::launder(reinterpret_cast<T*>(m_Data.GetAddress(index)));
}

template <typename T, size_t PageSize>
HK_FORCEINLINE bool PageStorage<T, PageSize>::IsEmpty() const
{
    return m_Size == 0;
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void PageStorage<T, PageSize>::Clear()
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        uint32_t i{};
        while (m_Size)
        {
            T* ptr = std::launder(reinterpret_cast<T*>(m_Data.GetAddress(i++)));
            ptr->~T();
            --m_Size;
        }
    }
    else
        m_Size = 0;
}

template <typename T, size_t PageSize>
HK_FORCEINLINE uint32_t PageStorage<T, PageSize>::Size() const
{
    return m_Size;
}

template <typename T, size_t PageSize>
HK_FORCEINLINE uint32_t PageStorage<T, PageSize>::Capacity() const
{
    return m_Data.GetPageCount() * PageSize;
}

template <typename T, size_t PageSize>
HK_FORCEINLINE uint32_t PageStorage<T, PageSize>::GetPageCount() const
{
    return m_Data.GetPageCount();
}

template <typename T, size_t PageSize>
HK_FORCEINLINE T* PageStorage<T, PageSize>::GetPageData(uint32_t pageIndex) const
{
    return std::launder(reinterpret_cast<T*>(m_Data.GetPageAddress(pageIndex)));
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void* PageStorage<T, PageSize>::GetAddress(uint32_t index)
{
    HK_ASSERT(index < Capacity());
    return m_Data.GetAddress(index);
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void const* PageStorage<T, PageSize>::GetAddress(uint32_t index) const
{
    HK_ASSERT(index < Capacity());
    return m_Data.GetAddress(index);
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void PageStorage<T, PageSize>::Reserve(uint32_t capacity)
{
    m_Data.Grow(capacity);
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void PageStorage<T, PageSize>::Resize(uint32_t size)
{
    Reserve(size);

    if constexpr (!std::is_trivially_constructible_v<T>)
    {
        while (m_Size < size)
            new (m_Data.GetAddress(m_Size++)) T;

        while (m_Size > size)
            PopBack();
    }
    else
        m_Size = size;
}

template <typename T, size_t PageSize>
template <typename... Args>
HK_FORCEINLINE T* PageStorage<T, PageSize>::EmplaceBack(Args&&...)
{
    m_Data.Grow(m_Size + 1);
    return new (m_Data.GetAddress(m_Size++)) T(std::forward<Args>(args)...);
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void PageStorage<T, PageSize>::PushBack(T const& value)
{
    m_Data.Grow(m_Size + 1);
    new (m_Data.GetAddress(m_Size++)) T(value);
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void PageStorage<T, PageSize>::PopBack()
{
    HK_ASSERT(m_Size > 0);

    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        T* ptr = std::launder(reinterpret_cast<T*>(m_Data.GetAddress(--m_Size)));
        ptr->~T();
    }
    else
    {
        --m_Size;
    }
}

template <typename T, size_t PageSize>
HK_FORCEINLINE void PageStorage<T, PageSize>::ShrinkToFit()
{
    m_Data.Shrink(m_Size);
}

template <typename T, size_t PageSize>
constexpr size_t PageStorage<T, PageSize>::GetPageSize()
{
    return PageSize;
}

template <typename T, size_t PageSize>
template <typename Visitor>
HK_FORCEINLINE void PageStorage<T, PageSize>::Iterate(Visitor& visitor)
{
    uint32_t pageCount = GetPageCount();
    uint32_t processed = 0;
    for (uint32_t pageIndex = 0; pageIndex < pageCount; ++pageIndex)
    {
        T* pageData = GetPageData(pageIndex);

        uint32_t remaining = m_Size - processed;
        if (remaining < PageSize)
        {
            for (uint32_t i = 0; i < remaining; ++i)
                visitor.Visit(pageData[i]);
            break;
        }
        for (size_t i = 0; i < PageSize; ++i)
            visitor.Visit(pageData[i]);
        processed += PageSize;
    }
}

template <typename T, size_t PageSize>
template <typename Visitor>
HK_FORCEINLINE void PageStorage<T, PageSize>::IterateBatches(Visitor& visitor)
{
    uint32_t pageCount = GetPageCount();
    uint32_t processed = 0;
    for (uint32_t pageIndex = 0; pageIndex < pageCount; ++pageIndex)
    {
        T* pageData = GetPageData(pageIndex);

        uint32_t remaining = m_Size - processed;
        if (remaining < PageSize)
        {
            visitor.Visit(pageData, remaining);
            break;
        }
        visitor.Visit(pageData, PageSize);
        processed += PageSize;
    }
}

HK_NAMESPACE_END
