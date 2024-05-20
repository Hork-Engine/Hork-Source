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
        uint8_t* Data;
    };

    TVector<Page> m_Pages;
    size_t m_TypeSize;

public:
    static constexpr size_t PageSize = _PageSize;

    explicit PageAllocator(size_t typeSize) :
        m_TypeSize(typeSize)
    {}

    PageAllocator(PageAllocator&& rhs) noexcept :
        m_Pages(std::move(rhs.m_Pages)),
        m_TypeSize(rhs.m_TypeSize)
    {}

    ~PageAllocator()
    {
        for (Page& page : m_Pages)
            Core::GetHeapAllocator<HEAP_MISC>().Free(page.Data);
    }

    void Grow(uint32_t count)
    {
        HK_ASSERT(count > 0);

        uint32_t pageCount = count / _PageSize + 1;
        uint32_t curPageCount = m_Pages.Size();

        const size_t alignment = 16;
        const size_t pageSizeInBytes = Align(_PageSize * m_TypeSize, alignment);

        if (pageCount > curPageCount)
        {
            m_Pages.Resize(pageCount);
            for (uint32_t i = curPageCount ; i < pageCount ; ++i)
                m_Pages[i].Data = (uint8_t*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(pageSizeInBytes, alignment);
        }
    }

    HK_FORCEINLINE uint8_t* GetPage(uint32_t pageIndex) const
    {
        return m_Pages[pageIndex].Data;
    }

    HK_FORCEINLINE uint8_t* At(uint32_t index) const
    {
        uint32_t pageIndex = index / _PageSize;
        size_t pageOffset = index - pageIndex * _PageSize;

        return m_Pages[pageIndex].Data + pageOffset * m_TypeSize;
    }
};

HK_NAMESPACE_END
