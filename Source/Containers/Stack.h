/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

/**

TStack

*/
template <typename T, size_t BaseCapacity = 32, typename OverflowAllocator = Allocators::HeapMemoryAllocator<HEAP_VECTOR>>
class TStack
{
public:
    using Container      = TSmallVector<T, BaseCapacity, OverflowAllocator>;
    using SizeType       = typename Container::SizeType;
    using Reference      = typename Container::Reference;
    using ConstReference = typename Container::ConstReference;
    using Pointer        = typename Container::Pointer;
    using ConstPointer   = typename Container::ConstPointer;

    Container Array;

    TStack()              = default;
    TStack(TStack const&) = default;
    TStack(TStack&&)      = default;

    HK_FORCEINLINE void Clear()
    {
        Array.Clear();
    }

    HK_FORCEINLINE void Free()
    {
        Array.Free();
    }

    HK_FORCEINLINE void ShrinkToFit()
    {
        Array.ShrinkToFit();
    }

    HK_FORCEINLINE void Reserve(SizeType n)
    {
        Array.Reserve(n);
    }

    HK_FORCEINLINE void Flip()
    {
        Array.Reverse();
    }

    HK_FORCEINLINE bool IsEmpty() const
    {
        return Array.IsEmpty();
    }

    HK_FORCEINLINE void Push(T const& _Val)
    {
        Array.Add(_Val);
    }

    HK_FORCEINLINE void Push(T&& _Val)
    {
        Array.Add(std::move(_Val));
    }

    template <class... Args>
    HK_FORCEINLINE void EmplacePush(Args&&... args)
    {
        Array.EmplaceBack(std::forward<Args>(args)...);
    }

    HK_FORCEINLINE T& Push()
    {
        return Array.Add();
    }

    HK_FORCEINLINE bool Pop(Reference _Val)
    {
        if (IsEmpty())
            return false;
        _Val = std::move(Array.Last());
        Array.RemoveLast();
        return true;
    }

    HK_FORCEINLINE bool Pop()
    {
        if (IsEmpty())
            return false;
        Array.RemoveLast();
        return true;
    }

    HK_FORCEINLINE Reference Top()
    {
        return Array.Last();
    }

    HK_FORCEINLINE ConstReference Top() const
    {
        return Array.Last();
    }

    HK_FORCEINLINE Reference Bottom()
    {
        return Array.Front();
    }

    HK_FORCEINLINE ConstReference Bottom() const
    {
        return Array.Front();
    }

    HK_FORCEINLINE Pointer ToPtr()
    {
        return Array.ToPtr();
    }

    HK_FORCEINLINE ConstPointer ToPtr() const
    {
        return Array.ToPtr();
    }

    HK_FORCEINLINE SizeType Size() const
    {
        return Array.Size();
    }

    HK_FORCEINLINE SizeType Capacity() const
    {
        return Array.Capacity();
    }

    HK_FORCEINLINE int StackPoint() const
    {
        return (int)Size() - 1;
    }

    HK_FORCEINLINE void Swap(TStack& x)
    {
        Swap(Array, x.Array);
    }

    TStack& operator=(TStack const&) = default;
    TStack& operator=(TStack&&) = default;
};

template <typename T, size_t BaseCapacity, typename Container>
inline bool operator==(TStack<T, BaseCapacity, Container> const& lhs, TStack<T, BaseCapacity, Container> const& rhs)
{
    return (lhs.Array == rhs.Array);
}

template <typename T, size_t BaseCapacity, typename Container>
inline bool operator!=(TStack<T, BaseCapacity, Container> const& lhs, TStack<T, BaseCapacity, Container> const& rhs)
{
    return !(lhs.Array == rhs.Array);
}

template <typename T, size_t BaseCapacity, typename Container>
inline bool operator<(TStack<T, BaseCapacity, Container> const& lhs, TStack<T, BaseCapacity, Container> const& rhs)
{
    return (lhs.Array < rhs.Array);
}

template <typename T, size_t BaseCapacity, typename Container>
inline bool operator>(TStack<T, BaseCapacity, Container> const& lhs, TStack<T, BaseCapacity, Container> const& rhs)
{
    return (rhs.Array < lhs.Array);
}

template <typename T, size_t BaseCapacity, typename Container>
inline bool operator<=(TStack<T, BaseCapacity, Container> const& lhs, TStack<T, BaseCapacity, Container> const& rhs)
{
    return !(rhs.Array < lhs.Array);
}

template <typename T, size_t BaseCapacity, typename Container>
inline bool operator>=(TStack<T, BaseCapacity, Container> const& lhs, TStack<T, BaseCapacity, Container> const& rhs)
{
    return !(lhs.Array < rhs.Array);
}

namespace eastl
{
template <typename T, size_t BaseCapacity, typename Container>
inline void swap(TStack<T, BaseCapacity, Container>& lhs, TStack<T, BaseCapacity, Container>& rhs)
{
    lhs.Swap(rhs);
}
} // namespace eastl
