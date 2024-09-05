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

#include <Engine/Core/BinaryStream.h>

HK_NAMESPACE_BEGIN

/**

BitMask

Variable size bit mask

*/
template <size_t BaseCapacityInBits = 1024, typename OverflowAllocator = Allocators::HeapMemoryAllocator<HEAP_VECTOR>>
class BitMask
{
public:
    using T                                     = uint32_t;
    static constexpr size_t BitCount            = sizeof(T) * 8;
    static constexpr size_t BitWrapMask         = BitCount - 1;
    static constexpr size_t BitExponent         = 5;
    static constexpr size_t BaseCapacityInBytes = (BaseCapacityInBits & BitWrapMask) ? BaseCapacityInBits / BitCount + 1 : BaseCapacityInBits / BitCount;

    BitMask()               = default;
    BitMask(BitMask const&) = default;

    BitMask(BitMask&& rhs) :
        m_Bits(std::move(rhs.m_Bits)),
        m_NumBits(rhs.m_NumBits)
    {
        rhs.m_NumBits = 0;
    }

    HK_FORCEINLINE void Clear()
    {
        m_Bits.Clear();
        m_NumBits = 0;
    }

    HK_FORCEINLINE void Free()
    {
        m_Bits.Free();
        m_NumBits = 0;
    }

    HK_FORCEINLINE void ShrinkToFit()
    {
        m_Bits.ShrinkToFit();
    }

    HK_FORCEINLINE void Reserve(size_t capacity)
    {
        size_t capacityWords = capacity / BitCount;
        if (capacity & BitWrapMask)
            ++capacityWords;
        m_Bits.Reserve(capacityWords);
    }

    HK_FORCEINLINE bool IsEmpty() const
    {
        return m_NumBits == 0;
    }

    HK_FORCEINLINE T*       ToPtr() { return m_Bits.ToPtr(); }
    HK_FORCEINLINE T const* ToPtr() const { return m_Bits.ToPtr(); }

    BitMask& operator=(BitMask const&) = default;

    BitMask& operator=(BitMask&& rhs)
    {
        m_Bits = std::move(rhs.m_Bits);
        m_NumBits = rhs.m_NumBits;
        rhs.m_NumBits = 0;
    }

    void Resize(size_t numBits)
    {
        const size_t oldsize = m_Bits.Size();
        const size_t newsize = numBits / BitCount + ((numBits & BitWrapMask) ? 1 : 0);

        m_Bits.Resize(newsize);

        if (numBits > m_NumBits)
        {
            Core::ZeroMem(m_Bits.ToPtr() + oldsize, (newsize - oldsize) * sizeof(T));

            const size_t clearBitsInWord = oldsize * BitCount;
            for (size_t i = m_NumBits; i < clearBitsInWord; i++)
            {
                m_Bits[i >> BitExponent] &= ~(1 << (i & BitWrapMask));
            }
        }
        m_NumBits = numBits;
    }

    HK_FORCEINLINE void ResizeInvalidate(size_t numBits)
    {
        m_Bits.ResizeInvalidate(numBits / BitCount + ((numBits & BitWrapMask) ? 1 : 0));
        m_NumBits = numBits;
    }

    HK_FORCEINLINE size_t Size() const
    {
        return m_NumBits;
    }

    HK_FORCEINLINE size_t Capacity() const
    {
        return m_Bits.Capacity() * BitCount;
    }

    HK_FORCEINLINE void MarkAll()
    {
        Core::Memset(m_Bits.ToPtr(), 0xff, m_Bits.Size() * sizeof(m_Bits[0]));
    }

    HK_FORCEINLINE void UnmarkAll()
    {
        m_Bits.ZeroMem();
    }

    HK_FORCEINLINE void Mark(size_t bitIndex)
    {
        if (bitIndex >= Size())
        {
            Resize(bitIndex + 1);
        }
        m_Bits[bitIndex >> BitExponent] |= 1 << (bitIndex & BitWrapMask);
    }

    HK_FORCEINLINE void Unmark(size_t bitIndex)
    {
        if (bitIndex < Size())
        {
            m_Bits[bitIndex >> BitExponent] &= ~(1 << (bitIndex & BitWrapMask));
        }
    }

    HK_FORCEINLINE bool IsMarked(size_t bitIndex) const
    {
        return bitIndex < Size() && (m_Bits[bitIndex >> BitExponent] & (1 << (bitIndex & BitWrapMask)));
    }

    HK_FORCEINLINE void Swap(BitMask& x)
    {
        Swap(m_Bits, x.m_Bits);
        std::swap(m_NumBits, x.m_NumBits);
    }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteUInt32(m_NumBits);
        stream.WriteArray(m_Bits);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        m_NumBits = stream.ReadUInt32();
        stream.ReadArray(m_Bits);
    }

private:
    SmallVector<T, BaseCapacityInBytes, OverflowAllocator>  m_Bits;
    size_t                                                  m_NumBits{};
};

HK_NAMESPACE_END

namespace eastl
{
template <size_t BaseCapacityInBits, typename OverflowAllocator>
HK_INLINE void swap(Hk::BitMask<BaseCapacityInBits, OverflowAllocator>& lhs, Hk::BitMask<BaseCapacityInBits, OverflowAllocator>& rhs)
{
    lhs.Swap(rhs);
}
} // namespace eastl
