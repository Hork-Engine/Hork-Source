/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

template <size_t _PageSize = 64>
class PageAllocator final : Noncopyable
{
    struct Page
    {
        void* Data;
    };

    TVector<Page>   m_Pages;
    size_t          m_TypeSize;

public:
    static constexpr size_t PageSize = _PageSize;

    explicit        PageAllocator(size_t typeSize);
                    PageAllocator(PageAllocator&& rhs) noexcept;
                    ~PageAllocator();

    PageAllocator&  operator=(PageAllocator&& rhs) noexcept;

    uint32_t        GetPageCount() const;

    void            Grow(uint32_t count);

    void            Shrink(uint32_t count);

    void*           GetPageAddress(uint32_t pageIndex) const;

    void*           GetAddress(uint32_t index) const;
};

template <size_t _PageSize>
HK_FORCEINLINE PageAllocator<_PageSize>::PageAllocator(size_t typeSize) :
    m_TypeSize(typeSize)
{}

template <size_t _PageSize>
HK_FORCEINLINE PageAllocator<_PageSize>::PageAllocator(PageAllocator&& rhs) noexcept :
    m_Pages(std::move(rhs.m_Pages)),
    m_TypeSize(rhs.m_TypeSize)
{}

template <size_t _PageSize>
HK_FORCEINLINE PageAllocator<_PageSize>::~PageAllocator()
{
    for (Page& page : m_Pages)
        Core::GetHeapAllocator<HEAP_MISC>().Free(page.Data);
}

template <size_t _PageSize>
HK_FORCEINLINE PageAllocator<_PageSize>& PageAllocator<_PageSize>::operator=(PageAllocator&& rhs) noexcept
{
    m_Pages = std::move(rhs.m_Pages);
    m_TypeSize = rhs.m_TypeSize;
    return *this;
}

template <size_t _PageSize>
HK_FORCEINLINE uint32_t PageAllocator<_PageSize>::GetPageCount() const
{
    return m_Pages.Size();
}

template <size_t _PageSize>
HK_INLINE void PageAllocator<_PageSize>::Grow(uint32_t count)
{
    HK_ASSERT(count > 0);

    uint32_t pageCount = count / _PageSize;
    if (count % _PageSize)
        ++pageCount;

    uint32_t curPageCount = m_Pages.Size();

    const size_t alignment = 16;
    const size_t pageSizeInBytes = Align(_PageSize * m_TypeSize, alignment);

    if (pageCount > curPageCount)
    {
        m_Pages.Resize(pageCount);
        for (uint32_t i = curPageCount ; i < pageCount ; ++i)
            m_Pages[i].Data = Core::GetHeapAllocator<HEAP_MISC>().Alloc(pageSizeInBytes, alignment);
    }
}

template <size_t _PageSize>
HK_INLINE void PageAllocator<_PageSize>::Shrink(uint32_t count)
{
    count = std::min(size_t(count), m_Pages.Size() * _PageSize);

    uint32_t pageCount = count / _PageSize;
    if (count % _PageSize)
        ++pageCount;

    uint32_t pagesToRemove = m_Pages.Size() - pageCount;
    while (pagesToRemove > 0)
    {
        Core::GetHeapAllocator<HEAP_MISC>().Free(m_Pages.Last().Data);
        m_Pages.RemoveLast();
        --pagesToRemove;
    }
}

template <size_t _PageSize>
HK_FORCEINLINE void* PageAllocator<_PageSize>::GetPageAddress(uint32_t pageIndex) const
{
    return m_Pages[pageIndex].Data;
}

template <size_t _PageSize>
HK_FORCEINLINE void* PageAllocator<_PageSize>::GetAddress(uint32_t index) const
{
    uint32_t pageIndex = index / _PageSize;
    size_t pageOffset = index - pageIndex * _PageSize;

    return reinterpret_cast<uint8_t*>(m_Pages[pageIndex].Data) + pageOffset * m_TypeSize;
}

HK_NAMESPACE_END
