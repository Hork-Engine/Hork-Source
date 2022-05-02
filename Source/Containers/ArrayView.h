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

#include "Array.h"
#include "PodVector.h"

template <typename T>
class TArrayView
{
public:
    typedef const T* ConstIterator;

    HK_FORCEINLINE TArrayView() :
        ArrayData(nullptr), Size(0)
    {
    }

    HK_FORCEINLINE TArrayView(T* _Data, int _Size) :
        ArrayData(_Data), ArraySize(_Size)
    {
        HK_ASSERT(_Size >= 0);
    }

    template <size_t N>
    HK_FORCEINLINE TArrayView(T const (&Array)[N]) :
        ArrayData(Array), ArraySize(N)
    {}

    template <size_t N>
    HK_FORCEINLINE TArrayView(TArray<T, N> const& Array) :
        ArrayData(Array.ToPtr()), ArraySize(Array.Size())
    {}

    template <int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator>
    HK_FORCEINLINE TArrayView(TPodVector<T, BASE_CAPACITY, GRANULARITY, Allocator> const& Vector) :
        ArrayData(Vector.ToPtr()), ArraySize(Vector.Size())
    {}

    TArrayView(TArrayView const& Str) = default;
    TArrayView& operator=(TArrayView const& Rhs) = default;

    template <size_t N>
    TArrayView& operator=(TArray<T, N> const& Rhs)
    {
        ArrayData = Rhs.ToPtr();
        ArraySize = Rhs.Size();
        return *this;
    }

    template <int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator>
    TArrayView& operator=(TPodVector<T, BASE_CAPACITY, GRANULARITY, Allocator> const& Rhs)
    {
        ArrayData = Rhs.ToPtr();
        ArraySize = Rhs.Size();
        return *this;
    }

    bool operator==(TArrayView Rhs) const
    {
        if (ArraySize != Rhs.ArraySize)
            return false;
        for (int i = 0 ; i < ArraySize ; i++)
            if (ArrayData[i] != Rhs.ArrayData[i])
                return false;
        return true;
    }

    bool operator!=(TArrayView Rhs) const
    {
        return !(*this == Rhs);
    }

    T const* ToPtr() const
    {
        return ArrayData;
    }

    int Size() const
    {
        return ArraySize;
    }

    bool IsEmpty() const
    {
        return ArraySize == 0;
    }

    T const& operator[](const int _Index) const
    {
        HK_ASSERT_(_Index >= 0 && _Index < ArraySize, "TArrayView::operator[]");
        return ArrayData[_Index];
    }

    T const& Last() const
    {
        HK_ASSERT_(ArraySize > 0, "TArrayView::Last");
        return ArrayData[ArraySize - 1];
    }

    T const& First() const
    {
        HK_ASSERT_(ArraySize > 0, "TArrayView::First");
        return ArrayData[0];
    }

    ConstIterator Begin() const
    {
        return ArrayData;
    }

    ConstIterator End() const
    {
        return ArrayData + ArraySize;
    }

    /** foreach compatibility */
    ConstIterator begin() const { return Begin(); }

    /** foreach compatibility */
    ConstIterator end() const { return End(); }

    ConstIterator Find(T const& _Element) const
    {
        return Find(Begin(), End(), _Element);
    }

    ConstIterator Find(ConstIterator _Begin, ConstIterator _End, const T& _Element) const
    {
        for (auto It = _Begin; It != _End; It++)
        {
            if (*It == _Element)
            {
                return It;
            }
        }
        return End();
    }

    bool IsExists(T const& _Element) const
    {
        for (int i = 0; i < ArraySize; i++)
        {
            if (ArrayData[i] == _Element)
            {
                return true;
            }
        }
        return false;
    }

    int IndexOf(T const& _Element) const
    {
        for (int i = 0; i < ArraySize; i++)
        {
            if (ArrayData[i] == _Element)
            {
                return i;
            }
        }
        return -1;
    }

private:
    T const* ArrayData;
    int ArraySize;
};
