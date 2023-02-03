/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Core/Platform/Memory/Memory.h>
#include <Engine/Core/Platform/Platform.h>

#include <malloc.h>
#include <memory.h>
#include <xmmintrin.h>
#include <immintrin.h>
#include <mimalloc/mimalloc.h>

#define USE_MIMALLOC

HK_NAMESPACE_BEGIN

namespace Platform
{

MemoryHeap MemoryHeaps[HEAP_MAX];

void _MemcpySSE(byte* _Dst, const byte* _Src, size_t _SizeInBytes)
{
    HK_ASSERT(IsSSEAligned((size_t)_Dst));
    HK_ASSERT(IsSSEAligned((size_t)_Src));

    int n = 0;

    while (n + 256 <= _SizeInBytes)
    {
        __m128i d0 = _mm_load_si128((__m128i*)&_Src[n + 0 * 16]);
        __m128i d1 = _mm_load_si128((__m128i*)&_Src[n + 1 * 16]);
        __m128i d2 = _mm_load_si128((__m128i*)&_Src[n + 2 * 16]);
        __m128i d3 = _mm_load_si128((__m128i*)&_Src[n + 3 * 16]);
        __m128i d4 = _mm_load_si128((__m128i*)&_Src[n + 4 * 16]);
        __m128i d5 = _mm_load_si128((__m128i*)&_Src[n + 5 * 16]);
        __m128i d6 = _mm_load_si128((__m128i*)&_Src[n + 6 * 16]);
        __m128i d7 = _mm_load_si128((__m128i*)&_Src[n + 7 * 16]);
        _mm_stream_si128((__m128i*)&_Dst[n + 0 * 16], d0);
        _mm_stream_si128((__m128i*)&_Dst[n + 1 * 16], d1);
        _mm_stream_si128((__m128i*)&_Dst[n + 2 * 16], d2);
        _mm_stream_si128((__m128i*)&_Dst[n + 3 * 16], d3);
        _mm_stream_si128((__m128i*)&_Dst[n + 4 * 16], d4);
        _mm_stream_si128((__m128i*)&_Dst[n + 5 * 16], d5);
        _mm_stream_si128((__m128i*)&_Dst[n + 6 * 16], d6);
        _mm_stream_si128((__m128i*)&_Dst[n + 7 * 16], d7);
        d0 = _mm_load_si128((__m128i*)&_Src[n + 8 * 16]);
        d1 = _mm_load_si128((__m128i*)&_Src[n + 9 * 16]);
        d2 = _mm_load_si128((__m128i*)&_Src[n + 10 * 16]);
        d3 = _mm_load_si128((__m128i*)&_Src[n + 11 * 16]);
        d4 = _mm_load_si128((__m128i*)&_Src[n + 12 * 16]);
        d5 = _mm_load_si128((__m128i*)&_Src[n + 13 * 16]);
        d6 = _mm_load_si128((__m128i*)&_Src[n + 14 * 16]);
        d7 = _mm_load_si128((__m128i*)&_Src[n + 15 * 16]);
        _mm_stream_si128((__m128i*)&_Dst[n + 8 * 16], d0);
        _mm_stream_si128((__m128i*)&_Dst[n + 9 * 16], d1);
        _mm_stream_si128((__m128i*)&_Dst[n + 10 * 16], d2);
        _mm_stream_si128((__m128i*)&_Dst[n + 11 * 16], d3);
        _mm_stream_si128((__m128i*)&_Dst[n + 12 * 16], d4);
        _mm_stream_si128((__m128i*)&_Dst[n + 13 * 16], d5);
        _mm_stream_si128((__m128i*)&_Dst[n + 14 * 16], d6);
        _mm_stream_si128((__m128i*)&_Dst[n + 15 * 16], d7);
        n += 256;
    }

    while (n + 128 <= _SizeInBytes)
    {
        __m128i d0 = _mm_load_si128((__m128i*)&_Src[n + 0 * 16]);
        __m128i d1 = _mm_load_si128((__m128i*)&_Src[n + 1 * 16]);
        __m128i d2 = _mm_load_si128((__m128i*)&_Src[n + 2 * 16]);
        __m128i d3 = _mm_load_si128((__m128i*)&_Src[n + 3 * 16]);
        __m128i d4 = _mm_load_si128((__m128i*)&_Src[n + 4 * 16]);
        __m128i d5 = _mm_load_si128((__m128i*)&_Src[n + 5 * 16]);
        __m128i d6 = _mm_load_si128((__m128i*)&_Src[n + 6 * 16]);
        __m128i d7 = _mm_load_si128((__m128i*)&_Src[n + 7 * 16]);
        _mm_stream_si128((__m128i*)&_Dst[n + 0 * 16], d0);
        _mm_stream_si128((__m128i*)&_Dst[n + 1 * 16], d1);
        _mm_stream_si128((__m128i*)&_Dst[n + 2 * 16], d2);
        _mm_stream_si128((__m128i*)&_Dst[n + 3 * 16], d3);
        _mm_stream_si128((__m128i*)&_Dst[n + 4 * 16], d4);
        _mm_stream_si128((__m128i*)&_Dst[n + 5 * 16], d5);
        _mm_stream_si128((__m128i*)&_Dst[n + 6 * 16], d6);
        _mm_stream_si128((__m128i*)&_Dst[n + 7 * 16], d7);
        n += 128;
    }

    while (n + 16 <= _SizeInBytes)
    {
        __m128i d0 = _mm_load_si128((__m128i*)&_Src[n]);
        _mm_stream_si128((__m128i*)&_Dst[n], d0);
        n += 16;
    }

    while (n + 4 <= _SizeInBytes)
    {
        *(uint32_t*)&_Dst[n] = *(const uint32_t*)&_Src[n];
        n += 4;
    }

    while (n < _SizeInBytes)
    {
        _Dst[n] = _Src[n];
        n++;
    }

    _mm_sfence();
}

void _ZeroMemSSE(byte* _Dst, size_t _SizeInBytes)
{
    HK_ASSERT(IsSSEAligned((size_t)_Dst));

    int n = 0;

    __m128i zero = _mm_setzero_si128();

    while (n + 256 <= _SizeInBytes)
    {
        _mm_stream_si128((__m128i*)&_Dst[n + 0 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 1 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 2 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 3 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 4 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 5 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 6 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 7 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 8 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 9 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 10 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 11 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 12 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 13 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 14 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 15 * 16], zero);
        n += 256;
    }

    while (n + 128 <= _SizeInBytes)
    {
        _mm_stream_si128((__m128i*)&_Dst[n + 0 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 1 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 2 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 3 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 4 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 5 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 6 * 16], zero);
        _mm_stream_si128((__m128i*)&_Dst[n + 7 * 16], zero);
        n += 128;
    }

    while (n + 16 <= _SizeInBytes)
    {
        _mm_stream_si128((__m128i*)&_Dst[n], zero);
        n += 16;
    }

    while (n + 4 <= _SizeInBytes)
    {
        *(uint32_t*)&_Dst[n] = 0;
        n += 4;
    }

    while (n < _SizeInBytes)
    {
        _Dst[n] = 0;
        n++;
    }

    _mm_sfence();
}

void _MemsetSSE(byte* _Dst, int _Val, size_t _SizeInBytes)
{
    HK_ASSERT(IsSSEAligned((size_t)_Dst));

    alignas(16) int32_t data[4];

    std::memset(data, _Val, sizeof(data));

    __m128i val = _mm_load_si128((__m128i*)&data[0]);

    int n = 0;

    while (n + 256 <= _SizeInBytes)
    {
        _mm_stream_si128((__m128i*)&_Dst[n + 0 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 1 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 2 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 3 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 4 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 5 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 6 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 7 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 8 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 9 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 10 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 11 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 12 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 13 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 14 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 15 * 16], val);
        n += 256;
    }

    while (n + 128 <= _SizeInBytes)
    {
        _mm_stream_si128((__m128i*)&_Dst[n + 0 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 1 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 2 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 3 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 4 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 5 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 6 * 16], val);
        _mm_stream_si128((__m128i*)&_Dst[n + 7 * 16], val);
        n += 128;
    }

    while (n + 16 <= _SizeInBytes)
    {
        _mm_stream_si128((__m128i*)&_Dst[n], val);
        n += 16;
    }

    while (n + 4 <= _SizeInBytes)
    {
        *(uint32_t*)&_Dst[n] = data[0];
        n += 4;
    }

    while (n < _SizeInBytes)
    {
        _Dst[n] = data[0];
        n++;
    }

    _mm_sfence();
}

} // namespace Platform

#ifndef USE_MIMALLOC

struct HeapChunk
{
    uint32_t Size;
    uint32_t Offset;
};

void* MemoryHeap::_Alloc(size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags)
{
    HK_VERIFY(SizeInBytes <= std::numeric_limits<uint32_t>::max(), "MemoryAlloc: Too large allocation\n");

    constexpr size_t DefaultAlignment =
#    if defined(__GNUC__)
        __BIGGEST_ALIGNMENT__;
#    else
        16;
#    endif

    if (Alignment == 0)
    {
        Alignment = DefaultAlignment;
    }
    else
    {
        HK_VERIFY(IsPowerOfTwo(Alignment), "MemoryAlloc: Alignment must be power of two\n");
    }

    size_t size = SizeInBytes + sizeof(HeapChunk) + (Alignment - 1);

    byte* p = (byte*)malloc(size);
    if (HK_UNLIKELY(!p))
        return nullptr;

    void* aligned = AlignPtr(p + sizeof(HeapChunk), Alignment);

    size_t offset = ((byte*)aligned - p);
    HK_VERIFY(offset <= std::numeric_limits<uint32_t>::max(), "MemoryAlloc: Too big alignment\n");

    (((HeapChunk*)aligned) - 1)->Offset = (uint32_t)offset;
    (((HeapChunk*)aligned) - 1)->Size   = SizeInBytes;

    if (Flags & MALLOC_ZERO)
        Platform::ZeroMem(aligned, SizeInBytes);

    m_PeakAllocated.Store(std::max(m_PeakAllocated.Load(), m_MemoryAllocated.Add(SizeInBytes)));
    m_MemoryAllocs.Increment();
    m_PerFrameAllocs.Increment();

    return aligned;
}

void MemoryHeap::Free(void* Ptr)
{
    if (!Ptr)
        return;

    m_MemoryAllocated.Sub((((HeapChunk*)Ptr) - 1)->Size);
    m_MemoryAllocs.Decrement();
    m_PerFrameFrees.Increment();

    free((byte*)Ptr - (((HeapChunk*)Ptr) - 1)->Offset);
}

size_t MemoryHeap::GetSize(void* Ptr)
{
    return Ptr ? (((HeapChunk*)Ptr) - 1)->Size : 0;
}

void* MemoryHeap::_Realloc(void* Ptr, size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags)
{
    if (!Ptr)
        return _Alloc(SizeInBytes, Alignment, Flags);

    size_t OldSize = (((HeapChunk*)Ptr) - 1)->Size;
    if (OldSize >= SizeInBytes && IsAlignedPtr(Ptr, Alignment))
        return Ptr;

    void* NewPtr = _Alloc(SizeInBytes, Alignment, Flags);
    if (HK_LIKELY(NewPtr))
    {
        if (!(Flags & MALLOC_DISCARD))
            Platform::Memcpy(NewPtr, Ptr, OldSize);
    }

    m_MemoryAllocated.Sub((((HeapChunk*)Ptr) - 1)->Size);

    free((byte*)Ptr - (((HeapChunk*)Ptr) - 1)->Offset);
    m_PerFrameFrees.Increment();
    m_MemoryAllocs.Decrement();

    return NewPtr;
}

#else

void* MemoryHeap::_Alloc(size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags)
{
    void* Ptr;

    if (Alignment == 0)
    {
        Ptr = (Flags & MALLOC_ZERO) ? mi_zalloc(SizeInBytes) : mi_malloc(SizeInBytes);
    }
    else
    {
        HK_VERIFY(IsPowerOfTwo(Alignment), "MemoryAlloc: Alignment must be power of two\n");
        HK_VERIFY(Alignment <= MI_ALIGNMENT_MAX, "MemoryAlloc: Too big alignment\n");

        Ptr = (Flags & MALLOC_ZERO) ? mi_zalloc_aligned(SizeInBytes, Alignment) : mi_malloc_aligned(SizeInBytes, Alignment);
    }

    if (HK_UNLIKELY(!Ptr))
        return nullptr;
    HK_ASSERT(SizeInBytes <= mi_malloc_size(Ptr));
    SizeInBytes = mi_malloc_size(Ptr);
    m_PeakAllocated.Store(std::max(m_PeakAllocated.Load(), m_MemoryAllocated.Add(SizeInBytes)));
    m_MemoryAllocs.Increment();
    m_PerFrameAllocs.Increment();

    return Ptr;
}

void MemoryHeap::Free(void* Ptr)
{
    if (!Ptr)
        return;

    m_MemoryAllocated.Sub(mi_malloc_size(Ptr));
    m_MemoryAllocs.Decrement();
    m_PerFrameFrees.Increment();
    mi_free(Ptr);
}

size_t MemoryHeap::GetSize(void* Ptr)
{
    return Ptr ? mi_malloc_size(Ptr) : 0;
}

void* MemoryHeap::_Realloc(void* Ptr, size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags)
{
    if (!Ptr)
        return _Alloc(SizeInBytes, Alignment, Flags);

    if (Flags & MALLOC_DISCARD)
    {
        Free(Ptr);
        return _Alloc(SizeInBytes, Alignment, Flags);
    }

    m_MemoryAllocated.Sub(mi_malloc_size(Ptr));

    Ptr = Alignment == 0 ? mi_realloc(Ptr, SizeInBytes) : mi_realloc_aligned(Ptr, SizeInBytes, Alignment);
    if (HK_LIKELY(Ptr))
    {
        HK_ASSERT(SizeInBytes <= mi_malloc_size(Ptr));
        SizeInBytes = mi_malloc_size(Ptr);
        m_PeakAllocated.Store(std::max(m_PeakAllocated.Load(), m_MemoryAllocated.Add(SizeInBytes)));

        m_PerFrameAllocs.Increment();
        m_MemoryAllocs.Increment();
    }

    m_PerFrameFrees.Increment();
    m_MemoryAllocs.Decrement();

    return Ptr;
}

#endif

void* MemoryHeap::Alloc(size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags)
{
    HK_VERIFY(SizeInBytes != 0, "MemoryAlloc: Invalid bytes count\n");

    void* Ptr = _Alloc(SizeInBytes, Alignment, Flags);
    if (HK_UNLIKELY(!Ptr))
        CriticalError("Failed on allocation of {} bytes\n", SizeInBytes);
    return Ptr;
}

void* MemoryHeap::Realloc(void* Ptr, size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags)
{
    HK_VERIFY(SizeInBytes != 0, "MemoryAlloc: Invalid bytes count\n");

    Ptr = _Realloc(Ptr, SizeInBytes, Alignment, Flags);
    if (HK_UNLIKELY(!Ptr))
        CriticalError("Failed on allocation of {} bytes\n", SizeInBytes);
    return Ptr;
}

MemoryStat MemoryHeap::MemoryGetStat()
{
    MemoryStat stat = {};
    for (int n = 0; n < HEAP_MAX; n++)
    {
        MemoryStat heapStat = Platform::MemoryHeaps[n].GetStat();
        stat.FrameAllocs += heapStat.FrameAllocs;
        stat.FrameFrees += heapStat.FrameFrees;
        stat.MemoryAllocated += heapStat.MemoryAllocated;
        stat.MemoryAllocs += heapStat.MemoryAllocs;
        stat.MemoryPeakAlloc += heapStat.MemoryPeakAlloc;
    }
    return stat;
}

MemoryStat MemoryHeap::GetStat()
{
    MemoryStat stat;

    stat.FrameAllocs     = m_PerFrameAllocs.Load();
    stat.FrameFrees      = m_PerFrameFrees.Load();
    stat.MemoryAllocated = m_MemoryAllocated.Load();
    stat.MemoryAllocs    = m_MemoryAllocs.Load();
    stat.MemoryPeakAlloc = m_PeakAllocated.Load();
    return stat;
}

void MemoryHeap::MemoryNewFrame()
{
    using namespace Platform;
    for (int n = 0; n < HEAP_MAX; n++)
    {
        MemoryHeaps[n].m_PerFrameAllocs.Store(0);
        MemoryHeaps[n].m_PerFrameFrees.Store(0);
    }
}

void MemoryHeap::MemoryCleanup()
{
}

HK_NAMESPACE_END
