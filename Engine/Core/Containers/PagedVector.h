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

#include "../Memory.h"

HK_NAMESPACE_BEGIN

constexpr uint32_t ConstexprLog2(uint32_t v)
{
    uint32_t r = (v > 0xffff) << 4;
    v >>= r;
    uint32_t shift = (v > 0xff) << 3;
    v >>= shift;
    r |= shift;
    shift = (v > 0xf) << 2;
    v >>= shift;
    r |= shift;
    shift = (v > 0x3) << 1;
    v >>= shift;
    r |= shift;
    r |= (v >> 1);
    return r;
}

//constexpr size_t constexpr_log2(size_t x)
//{
//# ifdef __GNUC__
//    return __builtin_ffs( x ) - 1; // GCC
//#endif
//#ifdef _MSC_VER
//    return CHAR_BIT * sizeof(x) - __lzcnt( x ); // Visual studio
//#endif
//}

template <typename T, size_t PageSize, size_t MaxPages>
class PagedVector : Noncopyable
{
public:
    static constexpr size_t ElementSize = sizeof(T);
    static constexpr size_t PageSizeLog2 = ConstexprLog2(PageSize);

    PagedVector()
    {
        static_assert(IsPowerOfTwo(PageSize), "Page size must be power of two");
        Core::ZeroMem(m_Pages, sizeof(m_Pages));
    }

    ~PagedVector()
    {
        for (size_t i = 0; i < m_Size; i++)
        {
            uint32_t pageNum = i >> PageSizeLog2;
            uint32_t localIndex = i & (PageSize - 1);

            reinterpret_cast<T*>(m_Pages[pageNum]->Data[localIndex])->~T();
        }

        for (size_t i = 0; i < MaxPages && m_Pages[i]; i++)
        {
            delete m_Pages[i];
        }
    }

    // Returns num elements in the vector.
    // Must be protected by mutex.
    size_t Size() const
    {
        return m_Size;
    }

    // Returns num pages in the vector.
    // Must be protected by mutex.
    uint32_t GetPageCount() const
    {
        return (m_Size >> PageSizeLog2) + 1;
    }

    // Emplace back new object. Returns the index within a vector.
    // Must be protected by mutex.
    template <typename... Args>
    uint32_t Add(Args&&... args)
    {
        uint32_t i = m_Size++;

        uint32_t pageNum = i >> PageSizeLog2;
        uint32_t localIndex = i & (PageSize - 1);

        HK_ASSERT(pageNum < MaxPages);
        if (pageNum >= MaxPages)
        {
            // TODO: Critical error
            std::terminate();
        }

        if (localIndex == 0)
        {
            m_Pages[pageNum] = new Page;
        }

        new (m_Pages[pageNum]->Data[localIndex]) T(std::forward<Args>(args)...);

        return i;
    }

    // Lock-free
    T& Get(uint32_t index)
    {
        uint32_t pageNum = index >> PageSizeLog2;
        uint32_t localIndex = index & (PageSize - 1);

        return *reinterpret_cast<T*>(m_Pages[pageNum]->Data[localIndex]);
    }

private:
    struct Page
    {
        uint8_t Data[PageSize][ElementSize];
    };

    Page* m_Pages[MaxPages];
    size_t m_Size{};
};

HK_NAMESPACE_END
