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

#include "Array.h"
#include "Vector.h"

HK_NAMESPACE_BEGIN

template <typename T>
class ArrayView
{
public:
    using ValueType            = std::remove_cv_t<T>;
    using Pointer              = T*;
    using ConstPointer         = const T*;
    using Reference            = T&;
    using ConstReference       = const T&;
    using Iterator             = T*;
    using ConstIterator        = const T*;
    using ReverseIterator      = eastl::reverse_iterator<Iterator>;
    using ConstReverseIterator = eastl::reverse_iterator<ConstIterator>;
    using SizeType             = size_t;

    constexpr HK_FORCEINLINE ArrayView() :
        m_Data(nullptr), m_Size(0)
    {}

    constexpr HK_FORCEINLINE ArrayView(ConstPointer _Data, SizeType _Size) :
        m_Data(_Data), m_Size(_Size)
    {}

    constexpr HK_FORCEINLINE ArrayView(ConstPointer pBegin, ConstPointer pEnd) :
        m_Data(pBegin), m_Size(static_cast<SizeType>(pEnd - pBegin))
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE ArrayView(T const (&Array)[N]) :
        m_Data(Array), m_Size(N)
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE ArrayView(Array<ValueType, N> const& Array) :
        m_Data(Array.ToPtr()), m_Size(Array.Size())
    {}

    template <typename Allocator>
    constexpr HK_FORCEINLINE ArrayView(Vector<ValueType, Allocator> const& Vector) :
        m_Data(Vector.ToPtr()), m_Size(Vector.Size())
    {}

    template <size_t BaseCapacity, bool bEnableOverflow, typename OverflowAllocator>
    constexpr HK_FORCEINLINE ArrayView(FixedVector<ValueType, BaseCapacity, bEnableOverflow, OverflowAllocator> const& Vector) :
        m_Data(Vector.ToPtr()), m_Size(Vector.Size())
    {}

    constexpr ArrayView(ArrayView const& Rhs) = default;
    constexpr ArrayView& operator=(ArrayView const& Rhs) = default;

    constexpr HK_FORCEINLINE bool operator==(ArrayView Rhs) const
    {
        return (m_Size == Rhs.m_Size) && eastl::equal(begin(), end(), Rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator!=(ArrayView Rhs) const
    {
        return (m_Size != Rhs.m_Size) || !eastl::equal(begin(), end(), Rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator<(ArrayView Rhs) const
    {
        return eastl::lexicographical_compare(begin(), end(), Rhs.begin(), Rhs.end());
    }

    constexpr HK_FORCEINLINE bool operator<=(ArrayView Rhs) const { return !(Rhs < *this); }

    constexpr HK_FORCEINLINE bool operator>(ArrayView Rhs) const { return Rhs < *this; }

    constexpr HK_FORCEINLINE bool operator>=(ArrayView Rhs) const { return !(*this < Rhs); }

    constexpr HK_FORCEINLINE ConstPointer ToPtr() const
    {
        return m_Data;
    }

    constexpr HK_FORCEINLINE SizeType Size() const
    {
        return m_Size;
    }

    constexpr HK_FORCEINLINE bool IsEmpty() const
    {
        return m_Size == 0;
    }

    constexpr HK_FORCEINLINE ConstReference operator[](SizeType i) const
    {
        HK_ASSERT_(i < m_Size, "Undefined behavior accessing out of bounds");
        return m_Data[i];
    }

    constexpr HK_FORCEINLINE ConstReference First() const
    {
        HK_ASSERT_(m_Size > 0, "Undefined behavior accessing an empty ArrayView");
        return m_Data[0];
    }

    constexpr HK_FORCEINLINE ConstReference Last() const
    {
        HK_ASSERT_(m_Size > 0, "Undefined behavior accessing an empty ArrayView");
        return m_Data[m_Size - 1];
    }

    constexpr HK_FORCEINLINE ConstIterator begin() const
    {
        return m_Data;
    }
    
    constexpr HK_FORCEINLINE ConstIterator end() const
    {
        return m_Data + m_Size;
    }
    
    constexpr HK_FORCEINLINE ConstIterator cbegin() const
    {
        return m_Data;
    }
    
    constexpr HK_FORCEINLINE ConstIterator cend() const
    {
        return m_Data + m_Size;
    }
    
    constexpr HK_FORCEINLINE ConstReverseIterator rbegin() const
    {
        return ConstReverseIterator(m_Data + m_Size);
    }
    
    constexpr HK_FORCEINLINE ConstReverseIterator rend() const
    {
        return ConstReverseIterator(m_Data);
    }
    
    constexpr HK_FORCEINLINE ConstReverseIterator crbegin() const
    {
        return ConstReverseIterator(m_Data + m_Size);
    }
    
    constexpr HK_FORCEINLINE ConstReverseIterator crend() const
    {
        return ConstReverseIterator(m_Data);
    }

    constexpr HK_FORCEINLINE ConstIterator Begin() const
    {
        return begin();
    }

    constexpr HK_FORCEINLINE ConstIterator End() const
    {
        return end();
    }

    constexpr HK_FORCEINLINE ConstIterator CBegin() const
    {
        return cbegin();
    }

    constexpr HK_FORCEINLINE ConstIterator CEnd() const
    {
        return cend();
    }

    constexpr HK_FORCEINLINE ConstReverseIterator RBegin() const
    {
        return rbegin();
    }

    constexpr HK_FORCEINLINE ConstReverseIterator REnd() const
    {
        return rend();
    }

    constexpr HK_FORCEINLINE ConstReverseIterator CRBegin() const
    {
        return crbegin();
    }

    constexpr HK_FORCEINLINE ConstReverseIterator CREnd() const
    {
        return crend();
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

    HK_FORCEINLINE ArrayView<T> GetSubView(SizeType first, SizeType count) const
    {
        HK_ASSERT_(first + count <= m_Size, "Undefined behavior accessing out of bounds");
        return ArrayView<T>(m_Data + first, count);
    }

    HK_FORCEINLINE ArrayView<T> GetSubView(SizeType first) const
    {
        HK_ASSERT_(first <= m_Size, "Undefined behavior accessing out of bounds");
        return ArrayView<T>(m_Data + first, m_Size - first);
    }

private:
    ConstPointer m_Data;
    SizeType m_Size;
};


template <typename T>
class MutableArrayView
{
public:
    using ValueType            = std::remove_cv_t<T>;
    using Pointer              = T*;
    using ConstPointer         = const T*;
    using Reference            = T&;
    using ConstReference       = const T&;
    using Iterator             = T*;
    using ConstIterator        = const T*;
    using ReverseIterator      = eastl::reverse_iterator<Iterator>;
    using ConstReverseIterator = eastl::reverse_iterator<ConstIterator>;
    using SizeType             = size_t;

    constexpr HK_FORCEINLINE MutableArrayView() :
        m_Data(nullptr), m_Size(0)
    {}

    constexpr HK_FORCEINLINE MutableArrayView(Pointer _Data, SizeType _Size) :
        m_Data(_Data), m_Size(_Size)
    {}

    constexpr HK_FORCEINLINE MutableArrayView(Pointer pBegin, Pointer pEnd) :
        m_Data(pBegin), m_Size(static_cast<SizeType>(pEnd - pBegin))
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE MutableArrayView(T (&Array)[N]) :
        m_Data(Array), m_Size(N)
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE MutableArrayView(Array<ValueType, N>& Array) :
        m_Data(Array.ToPtr()), m_Size(Array.Size())
    {}

    template <typename Allocator>
    constexpr HK_FORCEINLINE MutableArrayView(Vector<ValueType, Allocator>& Vector) :
        m_Data(Vector.ToPtr()), m_Size(Vector.Size())
    {}

    template <size_t BaseCapacity, bool bEnableOverflow, typename OverflowAllocator>
    constexpr HK_FORCEINLINE MutableArrayView(FixedVector<ValueType, BaseCapacity, bEnableOverflow, OverflowAllocator>& Vector) :
        m_Data(Vector.ToPtr()), m_Size(Vector.Size())
    {}

    constexpr MutableArrayView(MutableArrayView const& Rhs) = default;
    constexpr MutableArrayView& operator=(MutableArrayView const& Rhs) = default;

    constexpr HK_FORCEINLINE bool operator==(MutableArrayView Rhs) const
    {
        return (m_Size == Rhs.m_Size) && eastl::equal(begin(), end(), Rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator!=(MutableArrayView Rhs) const
    {
        return (m_Size != Rhs.m_Size) || !eastl::equal(begin(), end(), Rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator<(MutableArrayView Rhs) const
    {
        return eastl::lexicographical_compare(begin(), end(), Rhs.begin(), Rhs.end());
    }

    constexpr HK_FORCEINLINE bool operator<=(MutableArrayView Rhs) const { return !(Rhs < *this); }

    constexpr HK_FORCEINLINE bool operator>(MutableArrayView Rhs) const { return Rhs < *this; }

    constexpr HK_FORCEINLINE bool operator>=(MutableArrayView Rhs) const { return !(*this < Rhs); }

    constexpr HK_FORCEINLINE Pointer ToPtr()
    {
        return m_Data;
    }

    constexpr HK_FORCEINLINE ConstPointer ToPtr() const
    {
        return m_Data;
    }

    constexpr HK_FORCEINLINE SizeType Size() const
    {
        return m_Size;
    }

    constexpr HK_FORCEINLINE bool IsEmpty() const
    {
        return m_Size == 0;
    }

    constexpr HK_FORCEINLINE Reference operator[](SizeType i) const
    {
        HK_ASSERT_(i < m_Size, "Undefined behavior accessing out of bounds");
        return m_Data[i];
    }

    constexpr HK_FORCEINLINE Reference First()
    {
        HK_ASSERT_(m_Size > 0, "Undefined behavior accessing an empty MutableArrayView");
        return m_Data[0];
    }

    constexpr HK_FORCEINLINE Reference First() const
    {
        HK_ASSERT_(m_Size > 0, "Undefined behavior accessing an empty MutableArrayView");
        return m_Data[0];
    }

    constexpr HK_FORCEINLINE ConstReference Last()
    {
        HK_ASSERT_(m_Size > 0, "Undefined behavior accessing an empty MutableArrayView");
        return m_Data[m_Size - 1];
    }

    constexpr HK_FORCEINLINE ConstReference Last() const
    {
        HK_ASSERT_(m_Size > 0, "Undefined behavior accessing an empty MutableArrayView");
        return m_Data[m_Size - 1];
    }

    constexpr HK_FORCEINLINE Iterator begin() const
    {
        return m_Data;
    }

    constexpr HK_FORCEINLINE Iterator end() const
    {
        return m_Data + m_Size;
    }

    constexpr HK_FORCEINLINE ConstIterator cbegin() const
    {
        return m_Data;
    }

    constexpr HK_FORCEINLINE ConstIterator cend() const
    {
        return m_Data + m_Size;
    }

    constexpr HK_FORCEINLINE ReverseIterator rbegin() const
    {
        return ReverseIterator(m_Data + m_Size);
    }

    constexpr HK_FORCEINLINE ReverseIterator rend() const
    {
        return ReverseIterator(m_Data);
    }

    constexpr HK_FORCEINLINE ConstReverseIterator crbegin() const
    {
        return ConstReverseIterator(m_Data + m_Size);
    }

    constexpr HK_FORCEINLINE ConstReverseIterator crend() const
    {
        return ConstReverseIterator(m_Data);
    }

    constexpr HK_FORCEINLINE Iterator Begin() const
    {
        return begin();
    }

    constexpr HK_FORCEINLINE Iterator End() const
    {
        return end();
    }

    constexpr HK_FORCEINLINE ConstIterator CBegin() const
    {
        return cbegin();
    }

    constexpr HK_FORCEINLINE ConstIterator CEnd() const
    {
        return cend();
    }

    constexpr HK_FORCEINLINE ReverseIterator RBegin() const
    {
        return rbegin();
    }

    constexpr HK_FORCEINLINE ReverseIterator REnd() const
    {
        return rend();
    }

    constexpr HK_FORCEINLINE ConstReverseIterator CRBegin() const
    {
        return crbegin();
    }

    constexpr HK_FORCEINLINE ConstReverseIterator CREnd() const
    {
        return crend();
    }

    HK_FORCEINLINE Iterator Find(ValueType const& value) const
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

    HK_FORCEINLINE MutableArrayView<T> GetSubView(SizeType first, SizeType count) const
    {
        HK_ASSERT_(first + count <= m_Size, "Undefined behavior accessing out of bounds");
        return MutableArrayView<T>(m_Data + first, count);
    }

    HK_FORCEINLINE MutableArrayView<T> GetSubView(SizeType first) const
    {
        HK_ASSERT_(first <= m_Size, "Undefined behavior accessing out of bounds");
        return MutableArrayView<T>(m_Data + first, m_Size - first);
    }

private:
    Pointer m_Data;
    SizeType m_Size;
};

HK_NAMESPACE_END
