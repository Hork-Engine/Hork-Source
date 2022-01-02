/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Core/Public/Ref.h>

template <typename T, int MAX_BUFFER_SIZE = 128>
class TPodCircularBuffer
{
public:
    T   Data[MAX_BUFFER_SIZE];
    int Head;
    int Sz;

    TPodCircularBuffer() :
        Head(0), Sz(0)
    {
        static_assert(IsPowerOfTwo(MAX_BUFFER_SIZE), "Circular buffer size must be power of two");

        Platform::ZeroMem(Data, sizeof(Data));
    }

    bool IsEmpty() const
    {
        return Sz == 0;
    }

    bool IsFull() const
    {
        return Sz == MAX_BUFFER_SIZE;
    }

    int Size() const
    {
        return Sz;
    }

    int Capacity() const
    {
        return MAX_BUFFER_SIZE;
    }

    T& operator[](int Index)
    {
        AN_ASSERT(Index >= 0 && Index < Size());

        int offset = (Head + Index) & (MAX_BUFFER_SIZE - 1);

        return Data[offset];
    }

    T const& operator[](int Index) const
    {
        AN_ASSERT(Index >= 0 && Index < Size());

        int offset = (Head + Index) & (MAX_BUFFER_SIZE - 1);

        return Data[offset];
    }

    void Append(T const& Element)
    {
        int offset = (Head + Sz) & (MAX_BUFFER_SIZE - 1);

        if (Sz == MAX_BUFFER_SIZE)
        {
            // buffer is full
            Head = (Head + 1) & (MAX_BUFFER_SIZE - 1);
        }
        else
        {
            Sz++;
        }

        Data[offset] = Element;
    }

    void Clear()
    {
        Resize(0);

        Head = 0;
    }

    void Resize(int NewSize)
    {
        AN_ASSERT(NewSize >= 0 && NewSize <= MAX_BUFFER_SIZE);

        if (NewSize < Sz)
        {
            for (int i = NewSize; i < Sz; i++)
            {
                int offset = (Head + i) & (MAX_BUFFER_SIZE - 1);

                Platform::ZeroMem(Data[offset], sizeof(T));
            }
        }

        Sz = NewSize;
    }

    void PopBack()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = (Head + Sz - 1) & (MAX_BUFFER_SIZE - 1);
        Platform::ZeroMem(Data[offset], sizeof(T));

        Sz--;
    }

    void PopFront()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = Head & (MAX_BUFFER_SIZE - 1);
        Platform::ZeroMem(Data[offset], sizeof(T));

        Head = (Head + 1) & (MAX_BUFFER_SIZE - 1);
        Sz--;
    }

    void Remove(int Index)
    {
        AN_ASSERT(Index >= 0 && Index < Size());

        int offset = (Head + Index) & (MAX_BUFFER_SIZE - 1);

        int count = Sz - Index - 1;
        for (int i = 0; i < count; i++)
        {
            offset = (Head + Index + i) & (MAX_BUFFER_SIZE - 1);

            Data[offset] = Data[(offset + 1) & (MAX_BUFFER_SIZE - 1)];
        }

        offset = (Head + Sz - 1) & (MAX_BUFFER_SIZE - 1);
        Platform::ZeroMem(Data[offset], sizeof(T));

        Sz--;
    }
};


template <typename T, int MAX_BUFFER_SIZE = 128>
class TCircularRefBuffer
{
public:
    TRef<T> Data[MAX_BUFFER_SIZE];
    int     Head;
    int     Sz;

    TCircularRefBuffer() :
        Head(0), Sz(0)
    {
        static_assert(IsPowerOfTwo(MAX_BUFFER_SIZE), "Circular buffer size must be power of two");
    }

    bool IsEmpty() const
    {
        return Sz == 0;
    }

    bool IsFull() const
    {
        return Sz == MAX_BUFFER_SIZE;
    }

    int Size() const
    {
        return Sz;
    }

    int Capacity() const
    {
        return MAX_BUFFER_SIZE;
    }

    TRef<T>& operator[](int Index)
    {
        AN_ASSERT(Index >= 0 && Index < Size());

        int offset = (Head + Index) & (MAX_BUFFER_SIZE - 1);

        return Data[offset];
    }

    TRef<T> const& operator[](int Index) const
    {
        AN_ASSERT(Index >= 0 && Index < Size());

        int offset = (Head + Index) & (MAX_BUFFER_SIZE - 1);

        return Data[offset];
    }

    void Append(T* Element)
    {
        int offset = (Head + Sz) & (MAX_BUFFER_SIZE - 1);

        if (Sz == MAX_BUFFER_SIZE)
        {
            // buffer is full
            Head = (Head + 1) & (MAX_BUFFER_SIZE - 1);
        }
        else
        {
            Sz++;
        }

        Data[offset] = Element;
    }

    void Clear()
    {
        Resize(0);

        Head = 0;
    }

    void Resize(int NewSize)
    {
        AN_ASSERT(NewSize >= 0 && NewSize <= MAX_BUFFER_SIZE);

        if (NewSize < Sz)
        {
            for (int i = NewSize; i < Sz; i++)
            {
                int offset = (Head + i) & (MAX_BUFFER_SIZE - 1);

                Data[offset].Reset();
            }
        }

        Sz = NewSize;
    }

    void PopBack()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = (Head + Sz - 1) & (MAX_BUFFER_SIZE - 1);
        Data[offset].Reset();

        Sz--;
    }

    void PopFront()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = Head & (MAX_BUFFER_SIZE - 1);
        Data[offset].Reset();

        Head = (Head + 1) & (MAX_BUFFER_SIZE - 1);
        Sz--;
    }

    void Remove(int Index)
    {
        AN_ASSERT(Index >= 0 && Index < Size());

        int offset = (Head + Index) & (MAX_BUFFER_SIZE - 1);

        Data[offset].Reset();

        int count = Sz - Index - 1;
        for (int i = 0; i < count; i++)
        {
            offset = (Head + Index + i) & (MAX_BUFFER_SIZE - 1);

            Data[offset] = Data[(offset + 1) & (MAX_BUFFER_SIZE - 1)];
        }

        offset = (Head + Sz - 1) & (MAX_BUFFER_SIZE - 1);
        Data[offset].Reset();

        Sz--;
    }
};
