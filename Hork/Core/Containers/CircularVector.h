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

#include <Hork/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

template <typename T, uint32_t MaxCapacity = 128>
class CircularVector final
{
public:
    using Reference      = T&;
    using ConstReference = T const&;
    using SizeType       = uint32_t;
    using ThisType       = CircularVector<T, MaxCapacity>;

    CircularVector() :
        m_Head(0), m_Size(0)
    {
        static_assert(IsPowerOfTwo(MaxCapacity), "Circular vector size must be power of two");
    }

    CircularVector(std::initializer_list<T> list) :
        m_Head(0), m_Size(0)
    {
        for (T const& value : list)
            Add(value);
    }

    CircularVector(ThisType const& rhs) :
        m_Head(rhs.Size() & (MaxCapacity - 1)), m_Size(rhs.Size())
    {
        for (SizeType i = 0; i < m_Size; ++i)
            Construct(i, *rhs.InternalGet((rhs.m_Head + i) & (MaxCapacity - 1)));
    }

    CircularVector(ThisType&& rhs) noexcept :
        m_Head(rhs.Size() & (MaxCapacity - 1)), m_Size(rhs.Size())
    {
        for (SizeType i = 0; i < m_Size; ++i)
            Construct(i, std::move(*rhs.InternalGet((rhs.m_Head + i) & (MaxCapacity - 1))));

        rhs.m_Head = 0;
        rhs.m_Size = 0;
    }

    ThisType& operator=(ThisType&& rhs) noexcept
    {
        Resize(0);

        m_Head = rhs.m_Size & (MaxCapacity - 1);
        m_Size = rhs.m_Size;

        for (SizeType i = 0; i < m_Size; ++i)
            Construct(i, *rhs.InternalGet((rhs.m_Head + i) & (MaxCapacity - 1)));
    }

    ~CircularVector()
    {
        for (SizeType i = 0; i < m_Size; ++i)
        {
            SizeType offset = (m_Head + i) & (MaxCapacity - 1);
            Destruct(offset);
        }
    }

    bool IsEmpty() const
    {
        return m_Size == 0;
    }

    bool IsFull() const
    {
        return m_Size == MaxCapacity;
    }

    SizeType Size() const
    {
        return m_Size;
    }

    SizeType Capacity() const
    {
        return MaxCapacity;
    }

    Reference operator[](SizeType index)
    {
        HK_ASSERT(index < Size());

        SizeType offset = (m_Head + index) & (MaxCapacity - 1);
        return *InternalGet(offset);
    }

    ConstReference operator[](SizeType index) const
    {
        HK_ASSERT(index < Size());

        SizeType offset = (m_Head + index) & (MaxCapacity - 1);
        return *InternalGet(offset);
    }

    void Add(ConstReference value)
    {
        Construct(Allocate(), value);
    }

    void Add(T&& value)
    {
        Construct(Allocate(), std::forward<T>(value));
    }

    template <typename... Args>
    Reference EmplaceBack(Args&&... args)
    {
        SizeType offset = Allocate();
        Construct(offset, std::forward<Args>(args)...);
        return *InternalGet(offset);
    }

    void Clear()
    {
        Resize(0);
        m_Head = 0;
    }

    void Resize(SizeType size)
    {
        HK_ASSERT(size >= 0 && size <= MaxCapacity);

        if (size < m_Size)
        {
            for (SizeType i = size; i < m_Size; ++i)
            {
                SizeType offset = (m_Head + i) & (MaxCapacity - 1);
                Destruct(offset);
            }
        }
        else
        {
            for (SizeType i = m_Size; i < size; ++i)
            {
                SizeType offset = (m_Head + i) & (MaxCapacity - 1);
                Construct(offset);
            }
        }

        m_Size = size;
    }

    Reference First()
    {
        HK_ASSERT(!IsEmpty());

        SizeType offset = m_Head & (MaxCapacity - 1);
        return *InternalGet(offset);
    }

    ConstReference First() const
    {
        HK_ASSERT(!IsEmpty());

        SizeType offset = m_Head & (MaxCapacity - 1);
        return *InternalGet(offset);
    }

    Reference Last()
    {
        HK_ASSERT(!IsEmpty());

        SizeType offset = (m_Head + m_Size - 1) & (MaxCapacity - 1);
        return *InternalGet(offset);
    }

    ConstReference Last() const
    {
        HK_ASSERT(!IsEmpty());

        SizeType offset = (m_Head + m_Size - 1) & (MaxCapacity - 1);
        return *InternalGet(offset);
    }

    void PopBack()
    {
        if (IsEmpty())
            return;

        SizeType offset = (m_Head + m_Size - 1) & (MaxCapacity - 1);
        Destruct(offset);
        --m_Size;
    }

    void PopFront()
    {
        if (IsEmpty())
            return;

        SizeType offset = m_Head & (MaxCapacity - 1);
        Destruct(offset);

        m_Head = (m_Head + 1) & (MaxCapacity - 1);
        --m_Size;
    }

    void Remove(SizeType index)
    {
        HK_ASSERT(index < Size());

        SizeType offset = (m_Head + index) & (MaxCapacity - 1);

        SizeType count = m_Size - index - 1;
        for (SizeType i = 0; i < count; ++i)
        {
            offset = (m_Head + index + i) & (MaxCapacity - 1);

            *InternalGet(offset) = std::move(*InternalGet((offset + 1) & (MaxCapacity - 1)));
        }
        --m_Size;
    }

private:
    alignas(alignof(T)) uint8_t m_MemoryBuffer[MaxCapacity][sizeof(T)];
    SizeType m_Head;
    SizeType m_Size;

    SizeType Allocate()
    {
        SizeType offset = (m_Head + m_Size) & (MaxCapacity - 1);
        if (m_Size == MaxCapacity)
        {
            // buffer is full
            m_Head = (m_Head + 1) & (MaxCapacity - 1);
            Destruct(offset);
        }
        else
        {
            ++m_Size;
        }
        return offset;
    }

    T* InternalGet(SizeType offset)
    {
        return reinterpret_cast<T*>(&m_MemoryBuffer[offset][0]);
    }

    T const* InternalGet(SizeType offset) const
    {
        return reinterpret_cast<T const*>(&m_MemoryBuffer[offset][0]);
    }

    // Constructor
    template <typename... Args>
    void Construct(SizeType offset, Args&&... args)
    {
        void* ptr = m_MemoryBuffer[offset];
        new (ptr) T(std::forward<Args>(args)...);
    }

    // Destructor
    void Destruct(SizeType offset)
    {
        InternalGet(offset)->~T();
    }
};

HK_NAMESPACE_END
