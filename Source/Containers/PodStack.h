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

#include "PodVector.h"

/**

TPodStack

Stack for POD types

*/
template <typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator>
class TPodStack final
{
public:
    TPodStack() {}
    TPodStack(TPodStack const& _Stack) :
        Array(_Stack.ToPtr(), _Stack.Size()) {}
    ~TPodStack() {}

    void Clear()
    {
        Array.Clear();
    }

    void Free()
    {
        Array.Free();
    }

    void ShrinkToFit()
    {
        Array.ShrinkToFit();
    }

    void SetCapacity(int _DesiredCapacity)
    {
        Array.Reserve(_DesiredCapacity);
    }

    void Memset(const int _Value)
    {
        Array.Memset(_Value);
    }

    void ZeroMem()
    {
        Array.ZeroMem();
    }

    void Flip()
    {
        Array.Reverse();
    }

    bool IsEmpty() const
    {
        return Array.IsEmpty();
    }

    void Push(T const& _Val)
    {
        Array.Append(_Val);
    }

    bool Pop(T* _Val)
    {
        if (IsEmpty())
        {
            return false;
        }
        *_Val = Array.Last();
        Array.RemoveLast();
        return true;
    }

    bool Pop()
    {
        if (IsEmpty())
        {
            return false;
        }
        Array.RemoveLast();
        return true;
    }

    bool Top(T* _Val) const
    {
        if (IsEmpty())
        {
            return false;
        }
        *_Val = Array.Last();
        return true;
    }

    bool Bottom(T* _Val) const
    {
        if (IsEmpty())
        {
            return false;
        }
        *_Val = *ToPtr();
        return true;
    }

    T* ToPtr()
    {
        return Array.ToPtr();
    }

    T const* ToPtr() const
    {
        return Array.ToPtr();
    }

    int Size() const
    {
        return Array.Size();
    }

    int Capacity() const
    {
        return Array.Capacity();
    }

    int StackPoint() const
    {
        return Size() - 1;
    }

    TPodStack& operator=(TPodStack const& _Stack)
    {
        Set(_Stack.ToPtr(), _Stack.Size());
        return *this;
    }

    void Set(T const* _Elements, int _NumElements)
    {
        Array.Set(_Elements, _NumElements);
    }

private:
    TPodVector<T, BASE_CAPACITY, GRANULARITY, Allocator> Array;
};

template <typename T, typename Allocator = AZoneAllocator>
using TPodStackLite = TPodStack<T, 1, 32, Allocator>;
