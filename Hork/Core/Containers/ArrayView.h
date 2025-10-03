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

    constexpr HK_FORCEINLINE ArrayView(ConstPointer data, SizeType size) :
        m_Data(data), m_Size(size)
    {}

    constexpr HK_FORCEINLINE ArrayView(ConstPointer begin, ConstPointer end) :
        m_Data(begin), m_Size(static_cast<SizeType>(end - begin))
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE ArrayView(T const (&array)[N]) :
        m_Data(array), m_Size(N)
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE ArrayView(Array<ValueType, N> const& array) :
        m_Data(array.ToPtr()), m_Size(array.Size())
    {}

    template <typename Allocator>
    constexpr HK_FORCEINLINE ArrayView(Vector<ValueType, Allocator> const& vector) :
        m_Data(vector.ToPtr()), m_Size(vector.Size())
    {}

    template <size_t BaseCapacity, bool bEnableOverflow, typename OverflowAllocator>
    constexpr HK_FORCEINLINE ArrayView(FixedVector<ValueType, BaseCapacity, bEnableOverflow, OverflowAllocator> const& vector) :
        m_Data(vector.ToPtr()), m_Size(vector.Size())
    {}

    constexpr ArrayView(ArrayView const& rhs) = default;
    constexpr ArrayView& operator=(ArrayView const& rhs) = default;

    constexpr HK_FORCEINLINE bool operator==(ArrayView rhs) const
    {
        return (m_Size == rhs.m_Size) && eastl::equal(begin(), end(), rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator!=(ArrayView rhs) const
    {
        return (m_Size != rhs.m_Size) || !eastl::equal(begin(), end(), rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator<(ArrayView rhs) const
    {
        return eastl::lexicographical_compare(begin(), end(), rhs.begin(), rhs.end());
    }

    constexpr HK_FORCEINLINE bool operator<=(ArrayView rhs) const { return !(rhs < *this); }

    constexpr HK_FORCEINLINE bool operator>(ArrayView rhs) const { return rhs < *this; }

    constexpr HK_FORCEINLINE bool operator>=(ArrayView rhs) const { return !(*this < rhs); }

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

    constexpr HK_FORCEINLINE MutableArrayView(Pointer data, SizeType size) :
        m_Data(data), m_Size(size)
    {}

    constexpr HK_FORCEINLINE MutableArrayView(Pointer begin, Pointer end) :
        m_Data(begin), m_Size(static_cast<SizeType>(end - begin))
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE MutableArrayView(T (&array)[N]) :
        m_Data(array), m_Size(N)
    {}

    template <size_t N>
    constexpr HK_FORCEINLINE MutableArrayView(Array<ValueType, N>& array) :
        m_Data(array.ToPtr()), m_Size(array.Size())
    {}

    template <typename Allocator>
    constexpr HK_FORCEINLINE MutableArrayView(Vector<ValueType, Allocator>& vector) :
        m_Data(vector.ToPtr()), m_Size(vector.Size())
    {}

    template <size_t BaseCapacity, bool bEnableOverflow, typename OverflowAllocator>
    constexpr HK_FORCEINLINE MutableArrayView(FixedVector<ValueType, BaseCapacity, bEnableOverflow, OverflowAllocator>& vector) :
        m_Data(vector.ToPtr()), m_Size(vector.Size())
    {}

    constexpr MutableArrayView(MutableArrayView const& rhs) = default;
    constexpr MutableArrayView& operator=(MutableArrayView const& rhs) = default;

    constexpr HK_FORCEINLINE bool operator==(MutableArrayView rhs) const
    {
        return (m_Size == rhs.m_Size) && eastl::equal(begin(), end(), rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator!=(MutableArrayView rhs) const
    {
        return (m_Size != rhs.m_Size) || !eastl::equal(begin(), end(), rhs.begin());
    }

    constexpr HK_FORCEINLINE bool operator<(MutableArrayView rhs) const
    {
        return eastl::lexicographical_compare(begin(), end(), rhs.begin(), rhs.end());
    }

    constexpr HK_FORCEINLINE bool operator<=(MutableArrayView rhs) const { return !(rhs < *this); }

    constexpr HK_FORCEINLINE bool operator>(MutableArrayView rhs) const { return rhs < *this; }

    constexpr HK_FORCEINLINE bool operator>=(MutableArrayView rhs) const { return !(*this < rhs); }

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
