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

#include <Core/Ref.h>

template <typename T, int MAX_BUFFER_SIZE = 128>
class TPodCircularBuffer
{
public:
    T   m_Data[MAX_BUFFER_SIZE];
    int m_Head;
    int m_Size;

    TPodCircularBuffer() :
        m_Head(0), m_Size(0)
    {
        static_assert(IsPowerOfTwo(MAX_BUFFER_SIZE), "Circular buffer size must be power of two");

        Platform::ZeroMem(m_Data, sizeof(m_Data));
    }

    bool IsEmpty() const
    {
        return m_Size == 0;
    }

    bool IsFull() const
    {
        return m_Size == MAX_BUFFER_SIZE;
    }

    int Size() const
    {
        return m_Size;
    }

    int Capacity() const
    {
        return MAX_BUFFER_SIZE;
    }

    T& operator[](int Index)
    {
        HK_ASSERT(Index >= 0 && Index < Size());

        int offset = (m_Head + Index) & (MAX_BUFFER_SIZE - 1);

        return m_Data[offset];
    }

    T const& operator[](int Index) const
    {
        HK_ASSERT(Index >= 0 && Index < Size());

        int offset = (m_Head + Index) & (MAX_BUFFER_SIZE - 1);

        return m_Data[offset];
    }

    void Append(T const& Element)
    {
        int offset = (m_Head + m_Size) & (MAX_BUFFER_SIZE - 1);

        if (m_Size == MAX_BUFFER_SIZE)
        {
            // buffer is full
            m_Head = (m_Head + 1) & (MAX_BUFFER_SIZE - 1);
        }
        else
        {
            m_Size++;
        }

        m_Data[offset] = Element;
    }

    void Clear()
    {
        Resize(0);

        m_Head = 0;
    }

    void Resize(int NewSize)
    {
        HK_ASSERT(NewSize >= 0 && NewSize <= MAX_BUFFER_SIZE);

        if (NewSize < m_Size)
        {
            for (int i = NewSize; i < m_Size; i++)
            {
                int offset = (m_Head + i) & (MAX_BUFFER_SIZE - 1);

                Platform::ZeroMem(m_Data[offset], sizeof(T));
            }
        }

        m_Size = NewSize;
    }

    void PopBack()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = (m_Head + m_Size - 1) & (MAX_BUFFER_SIZE - 1);
        Platform::ZeroMem(m_Data[offset], sizeof(T));

        m_Size--;
    }

    void PopFront()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = m_Head & (MAX_BUFFER_SIZE - 1);
        Platform::ZeroMem(m_Data[offset], sizeof(T));

        m_Head = (m_Head + 1) & (MAX_BUFFER_SIZE - 1);
        m_Size--;
    }

    void Remove(int Index)
    {
        HK_ASSERT(Index >= 0 && Index < Size());

        int offset = (m_Head + Index) & (MAX_BUFFER_SIZE - 1);

        int count = m_Size - Index - 1;
        for (int i = 0; i < count; i++)
        {
            offset = (m_Head + Index + i) & (MAX_BUFFER_SIZE - 1);

            m_Data[offset] = m_Data[(offset + 1) & (MAX_BUFFER_SIZE - 1)];
        }

        offset = (m_Head + m_Size - 1) & (MAX_BUFFER_SIZE - 1);
        Platform::ZeroMem(m_Data[offset], sizeof(T));

        m_Size--;
    }
};


template <typename T, int MAX_BUFFER_SIZE = 128>
class TCircularRefBuffer
{
public:
    TRef<T> m_Data[MAX_BUFFER_SIZE];
    int     m_Head;
    int     m_Size;

    TCircularRefBuffer() :
        m_Head(0), m_Size(0)
    {
        static_assert(IsPowerOfTwo(MAX_BUFFER_SIZE), "Circular buffer size must be power of two");
    }

    bool IsEmpty() const
    {
        return m_Size == 0;
    }

    bool IsFull() const
    {
        return m_Size == MAX_BUFFER_SIZE;
    }

    int Size() const
    {
        return m_Size;
    }

    int Capacity() const
    {
        return MAX_BUFFER_SIZE;
    }

    TRef<T>& operator[](int Index)
    {
        HK_ASSERT(Index >= 0 && Index < Size());

        int offset = (m_Head + Index) & (MAX_BUFFER_SIZE - 1);

        return m_Data[offset];
    }

    TRef<T> const& operator[](int Index) const
    {
        HK_ASSERT(Index >= 0 && Index < Size());

        int offset = (m_Head + Index) & (MAX_BUFFER_SIZE - 1);

        return m_Data[offset];
    }

    void Append(T* Element)
    {
        int offset = (m_Head + m_Size) & (MAX_BUFFER_SIZE - 1);

        if (m_Size == MAX_BUFFER_SIZE)
        {
            // buffer is full
            m_Head = (m_Head + 1) & (MAX_BUFFER_SIZE - 1);
        }
        else
        {
            m_Size++;
        }

        m_Data[offset] = Element;
    }

    void Clear()
    {
        Resize(0);

        m_Head = 0;
    }

    void Resize(int NewSize)
    {
        HK_ASSERT(NewSize >= 0 && NewSize <= MAX_BUFFER_SIZE);

        if (NewSize < m_Size)
        {
            for (int i = NewSize; i < m_Size; i++)
            {
                int offset = (m_Head + i) & (MAX_BUFFER_SIZE - 1);

                m_Data[offset].Reset();
            }
        }

        m_Size = NewSize;
    }

    void PopBack()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = (m_Head + m_Size - 1) & (MAX_BUFFER_SIZE - 1);
        m_Data[offset].Reset();

        m_Size--;
    }

    void PopFront()
    {
        if (IsEmpty())
        {
            return;
        }

        int offset = m_Head & (MAX_BUFFER_SIZE - 1);
        m_Data[offset].Reset();

        m_Head = (m_Head + 1) & (MAX_BUFFER_SIZE - 1);
        m_Size--;
    }

    void Remove(int Index)
    {
        HK_ASSERT(Index >= 0 && Index < Size());

        int offset = (m_Head + Index) & (MAX_BUFFER_SIZE - 1);

        m_Data[offset].Reset();

        int count = m_Size - Index - 1;
        for (int i = 0; i < count; i++)
        {
            offset = (m_Head + Index + i) & (MAX_BUFFER_SIZE - 1);

            m_Data[offset] = m_Data[(offset + 1) & (MAX_BUFFER_SIZE - 1)];
        }

        offset = (m_Head + m_Size - 1) & (MAX_BUFFER_SIZE - 1);
        m_Data[offset].Reset();

        m_Size--;
    }
};
