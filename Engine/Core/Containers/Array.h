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

#include "Vector.h"

HK_NAMESPACE_BEGIN

template <typename T, size_t ArraySize>
struct TArray
{
public:
    using ThisType             = TArray<T, ArraySize>;
    using ValueType            = T;
    using Reference            = ValueType&;
    using ConstReference       = const ValueType&;
    using Iterator             = ValueType*;
    using ConstIterator        = const ValueType*;
    using ReverseIterator      = ::eastl::reverse_iterator<Iterator>;
    using ConstReverseIterator = ::eastl::reverse_iterator<ConstIterator>;
    using SizeType             = eastl_size_t;

public:
    ValueType m_Data[ArraySize ? ArraySize : 1];

public:
    HK_FORCEINLINE void Fill(ValueType const& value)
    {
        eastl::fill_n(&m_Data[0], ArraySize, value);
    }

    HK_FORCEINLINE void Swap(ThisType& x)
    {
        eastl::swap_ranges(&m_Data[0], &m_Data[ArraySize], &x.m_Data[0]);
    }

    constexpr HK_FORCEINLINE bool IsEmpty() const
    {
        return (ArraySize == 0);
    }

    constexpr HK_FORCEINLINE SizeType Size() const
    {
        return (SizeType)ArraySize;
    }

    constexpr HK_FORCEINLINE SizeType MaxSize() const
    {
        return (SizeType)ArraySize;
    }

    constexpr HK_FORCEINLINE T* ToPtr()
    {
        return m_Data;
    }

    constexpr HK_FORCEINLINE const T* ToPtr() const
    {
        return m_Data;
    }

    constexpr HK_FORCEINLINE Reference operator[](SizeType i)
    {
        HK_ASSERT_(i < ArraySize, "Undefined behavior accessing out of bounds");
        return m_Data[i];
    }

    constexpr HK_FORCEINLINE ConstReference operator[](SizeType i) const
    {
        HK_ASSERT_(i < ArraySize, "Undefined behavior accessing out of bounds");
        return m_Data[i];
    }

    constexpr HK_FORCEINLINE ConstReference At(SizeType i) const
    {
        HK_ASSERT_(i < ArraySize, "Undefined behavior accessing out of bounds");
        return static_cast<ConstReference>(m_Data[i]);
    }

    constexpr HK_FORCEINLINE Reference At(SizeType i)
    {
        HK_ASSERT_(i < ArraySize, "Undefined behavior accessing out of bounds");
        return static_cast<Reference>(m_Data[i]);
    }

    constexpr HK_FORCEINLINE Reference First()
    {
        return m_Data[0];
    }

    constexpr HK_FORCEINLINE ConstReference First() const
    {
        return m_Data[0];
    }

    constexpr HK_FORCEINLINE Reference Last()
    {
        return m_Data[ArraySize - 1];
    }

    constexpr HK_FORCEINLINE ConstReference Last() const
    {
        return m_Data[ArraySize - 1];
    }

    constexpr HK_FORCEINLINE Iterator begin()
    {
        return &m_Data[0];
    }
    constexpr HK_FORCEINLINE ConstIterator begin() const
    {
        return &m_Data[0];
    }
    constexpr HK_FORCEINLINE ConstIterator cbegin() const
    {
        return &m_Data[0];
    }
    constexpr HK_FORCEINLINE Iterator end()
    {
        return &m_Data[ArraySize];
    }
    constexpr HK_FORCEINLINE ConstIterator end() const
    {
        return &m_Data[ArraySize];
    }
    constexpr HK_FORCEINLINE ConstIterator cend() const
    {
        return &m_Data[ArraySize];
    }
    constexpr HK_FORCEINLINE ReverseIterator rbegin()
    {
        return ReverseIterator(&m_Data[ArraySize]);
    }
    constexpr HK_FORCEINLINE ConstReverseIterator rbegin() const
    {
        return ConstReverseIterator(&m_Data[ArraySize]);
    }
    constexpr HK_FORCEINLINE ConstReverseIterator crbegin() const
    {
        return ConstReverseIterator(&m_Data[ArraySize]);
    }
    constexpr HK_FORCEINLINE ReverseIterator rend()
    {
        return ReverseIterator(&m_Data[0]);
    }
    constexpr HK_FORCEINLINE ConstReverseIterator rend() const
    {
        return ConstReverseIterator(static_cast<ConstIterator>(&m_Data[0]));
    }
    constexpr HK_FORCEINLINE ConstReverseIterator crend() const
    {
        return ConstReverseIterator(static_cast<ConstIterator>(&m_Data[0]));
    }
    constexpr HK_FORCEINLINE Iterator Begin()
    {
        return begin();
    }
    constexpr HK_FORCEINLINE ConstIterator Begin() const
    {
        return begin();
    }
    constexpr HK_FORCEINLINE ConstIterator CBegin() const
    {
        return cbegin();
    }
    constexpr HK_FORCEINLINE Iterator End()
    {
        return end();
    }
    constexpr HK_FORCEINLINE ConstIterator End() const
    {
        return end();
    }
    constexpr HK_FORCEINLINE ConstIterator CEnd() const
    {
        return cend();
    }
    constexpr HK_FORCEINLINE ReverseIterator RBegin()
    {
        return rbegin();
    }
    constexpr HK_FORCEINLINE ConstReverseIterator RBegin() const
    {
        return rbegin();
    }
    constexpr HK_FORCEINLINE ConstReverseIterator CRBegin() const
    {
        return crbegin();
    }
    constexpr HK_FORCEINLINE ReverseIterator REnd()
    {
        return rend();
    }
    constexpr HK_FORCEINLINE ConstReverseIterator REnd() const
    {
        return rend();
    }
    constexpr HK_FORCEINLINE ConstReverseIterator CREnd() const
    {
        return crend();
    }

    HK_FORCEINLINE void ZeroMem()
    {
        Core::ZeroMem(ToPtr(), sizeof(ValueType) * Size());
    }

    HK_FORCEINLINE ConstIterator Find(ValueType const& value) const
    {
        return eastl::find(begin(), end(), value);
    }

    HK_FORCEINLINE bool Contains(ValueType const& value) const
    {
        return eastl::find(begin(), end(), value) != end();
    }

    template <typename Predicate>
    HK_FORCEINLINE bool Contains(ValueType const& value, Predicate predicate) const
    {
        return eastl::find(begin(), end(), value, predicate) != end();
    }

    HK_FORCEINLINE SizeType IndexOf(ValueType const& value) const
    {
        auto it = eastl::find(begin(), end(), value);
        return it == end() ? Core::NPOS : (SizeType)(it - begin());
    }

    HK_FORCEINLINE void Reverse()
    {
        eastl::reverse(begin(), end());
    }

    HK_FORCEINLINE void ReverseRange(SizeType index, SizeType count)
    {
        eastl::reverse(begin() + index, begin() + (index + count));
    }
};


template <typename T, size_t ArraySize>
constexpr inline bool operator==(TArray<T, ArraySize> const& a, TArray<T, ArraySize> const& b)
{
    return eastl::equal(&a.m_Data[0], &a.m_Data[ArraySize], &b.m_Data[0]);
}

template <typename T, size_t ArraySize>
constexpr inline bool operator<(TArray<T, ArraySize> const& a, TArray<T, ArraySize> const& b)
{
    return eastl::lexicographical_compare(&a.m_Data[0], &a.m_Data[ArraySize], &b.m_Data[0], &b.m_Data[ArraySize]);
}

template <typename T, size_t ArraySize>
constexpr inline bool operator!=(TArray<T, ArraySize> const& a, TArray<T, ArraySize> const& b)
{
    return !eastl::equal(&a.m_Data[0], &a.m_Data[ArraySize], &b.m_Data[0]);
}

template <typename T, size_t ArraySize>
constexpr inline bool operator>(TArray<T, ArraySize> const& a, TArray<T, ArraySize> const& b)
{
    return eastl::lexicographical_compare(&b.m_Data[0], &b.m_Data[ArraySize], &a.m_Data[0], &a.m_Data[ArraySize]);
}

template <typename T, size_t ArraySize>
constexpr inline bool operator<=(TArray<T, ArraySize> const& a, TArray<T, ArraySize> const& b)
{
    return !eastl::lexicographical_compare(&b.m_Data[0], &b.m_Data[ArraySize], &a.m_Data[0], &a.m_Data[ArraySize]);
}

template <typename T, size_t ArraySize>
constexpr inline bool operator>=(TArray<T, ArraySize> const& a, TArray<T, ArraySize> const& b)
{
    return !eastl::lexicographical_compare(&a.m_Data[0], &a.m_Data[ArraySize], &b.m_Data[0], &b.m_Data[ArraySize]);
}

HK_NAMESPACE_END

namespace eastl
{
template <typename T, size_t ArraySize>
inline void swap(Hk::TArray<T, ArraySize>& a, Hk::TArray<T, ArraySize>& b)
{
    eastl::swap_ranges(&a.m_Data[0], &a.m_Data[ArraySize], &b.m_Data[0]);
}
} // namespace eastl
