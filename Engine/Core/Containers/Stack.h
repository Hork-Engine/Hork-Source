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

/**

Stack

*/
template <typename T, size_t BaseCapacity = 32, typename OverflowAllocator = Allocators::HeapMemoryAllocator<HEAP_VECTOR>>
class Stack
{
public:
    using Container      = SmallVector<T, BaseCapacity, OverflowAllocator>;
    using SizeType       = typename Container::SizeType;
    using Reference      = typename Container::Reference;
    using ConstReference = typename Container::ConstReference;
    using Pointer        = typename Container::Pointer;
    using ConstPointer   = typename Container::ConstPointer;

    Container m_Array;

    Stack()              = default;
    Stack(Stack const&) = default;
    Stack(Stack&&)      = default;

    HK_FORCEINLINE void Clear()
    {
        m_Array.Clear();
    }

    HK_FORCEINLINE void Free()
    {
        m_Array.Free();
    }

    HK_FORCEINLINE void ShrinkToFit()
    {
        m_Array.ShrinkToFit();
    }

    HK_FORCEINLINE void Reserve(SizeType n)
    {
        m_Array.Reserve(n);
    }

    HK_FORCEINLINE void Flip()
    {
        m_Array.Reverse();
    }

    HK_FORCEINLINE bool IsEmpty() const
    {
        return m_Array.IsEmpty();
    }

    HK_FORCEINLINE void Push(T const& _Val)
    {
        m_Array.Add(_Val);
    }

    HK_FORCEINLINE void Push(T&& _Val)
    {
        m_Array.Add(std::move(_Val));
    }

    template <class... Args>
    HK_FORCEINLINE void EmplacePush(Args&&... args)
    {
        m_Array.EmplaceBack(std::forward<Args>(args)...);
    }

    HK_FORCEINLINE T& Push()
    {
        return m_Array.Add();
    }

    HK_FORCEINLINE bool Pop(Reference _Val)
    {
        if (IsEmpty())
            return false;
        _Val = std::move(m_Array.Last());
        m_Array.RemoveLast();
        return true;
    }

    HK_FORCEINLINE bool Pop()
    {
        if (IsEmpty())
            return false;
        m_Array.RemoveLast();
        return true;
    }

    HK_FORCEINLINE Reference Top()
    {
        return m_Array.Last();
    }

    HK_FORCEINLINE ConstReference Top() const
    {
        return m_Array.Last();
    }

    HK_FORCEINLINE Reference Bottom()
    {
        return m_Array.Front();
    }

    HK_FORCEINLINE ConstReference Bottom() const
    {
        return m_Array.Front();
    }

    HK_FORCEINLINE Pointer ToPtr()
    {
        return m_Array.ToPtr();
    }

    HK_FORCEINLINE ConstPointer ToPtr() const
    {
        return m_Array.ToPtr();
    }

    HK_FORCEINLINE SizeType Size() const
    {
        return m_Array.Size();
    }

    HK_FORCEINLINE SizeType Capacity() const
    {
        return m_Array.Capacity();
    }

    HK_FORCEINLINE int StackPoint() const
    {
        return (int)Size() - 1;
    }

    HK_FORCEINLINE void Swap(Stack& x)
    {
        Swap(m_Array, x.m_Array);
    }

    Stack& operator=(Stack const&) = default;
    Stack& operator=(Stack&&) = default;
};

template <typename T, size_t BaseCapacity, typename OverflowAllocator>
inline bool operator==(Stack<T, BaseCapacity, OverflowAllocator> const& lhs, Stack<T, BaseCapacity, OverflowAllocator> const& rhs)
{
    return (lhs.m_Array == rhs.m_Array);
}

template <typename T, size_t BaseCapacity, typename OverflowAllocator>
inline bool operator!=(Stack<T, BaseCapacity, OverflowAllocator> const& lhs, Stack<T, BaseCapacity, OverflowAllocator> const& rhs)
{
    return (lhs.m_Array != rhs.m_Array);
}

template <typename T, size_t BaseCapacity, typename OverflowAllocator>
inline bool operator<(Stack<T, BaseCapacity, OverflowAllocator> const& lhs, Stack<T, BaseCapacity, OverflowAllocator> const& rhs)
{
    return (lhs.m_Array < rhs.m_Array);
}

template <typename T, size_t BaseCapacity, typename OverflowAllocator>
inline bool operator>(Stack<T, BaseCapacity, OverflowAllocator> const& lhs, Stack<T, BaseCapacity, OverflowAllocator> const& rhs)
{
    return (rhs.m_Array > lhs.m_Array);
}

template <typename T, size_t BaseCapacity, typename OverflowAllocator>
inline bool operator<=(Stack<T, BaseCapacity, OverflowAllocator> const& lhs, Stack<T, BaseCapacity, OverflowAllocator> const& rhs)
{
    return (rhs.m_Array <= lhs.m_Array);
}

template <typename T, size_t BaseCapacity, typename OverflowAllocator>
inline bool operator>=(Stack<T, BaseCapacity, OverflowAllocator> const& lhs, Stack<T, BaseCapacity, OverflowAllocator> const& rhs)
{
    return (lhs.m_Array >= rhs.m_Array);
}

HK_NAMESPACE_END

namespace eastl
{
template <typename T, size_t BaseCapacity, typename OverflowAllocator>
inline void swap(Hk::Stack<T, BaseCapacity, OverflowAllocator>& lhs, Hk::Stack<T, BaseCapacity, OverflowAllocator>& rhs)
{
    lhs.Swap(rhs);
}
} // namespace eastl
