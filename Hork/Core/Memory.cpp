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

#include "Memory.h"
#include "Platform.h"
#include "CoreApplication.h"

#include <malloc.h>
#include <memory.h>
#include <xmmintrin.h>
#include <immintrin.h>
#include <mimalloc/mimalloc.h>

#define USE_MIMALLOC

HK_NAMESPACE_BEGIN

namespace Core
{

MemoryHeap MemoryHeaps[HEAP_MAX];

void _MemcpySSE(byte* dst, const byte* src, size_t sizeInBytes)
{
    HK_ASSERT(IsSSEAligned((size_t)dst));
    HK_ASSERT(IsSSEAligned((size_t)src));

    int n = 0;

    while (n + 256 <= sizeInBytes)
    {
        __m128i d0 = _mm_load_si128((__m128i*)&src[n + 0 * 16]);
        __m128i d1 = _mm_load_si128((__m128i*)&src[n + 1 * 16]);
        __m128i d2 = _mm_load_si128((__m128i*)&src[n + 2 * 16]);
        __m128i d3 = _mm_load_si128((__m128i*)&src[n + 3 * 16]);
        __m128i d4 = _mm_load_si128((__m128i*)&src[n + 4 * 16]);
        __m128i d5 = _mm_load_si128((__m128i*)&src[n + 5 * 16]);
        __m128i d6 = _mm_load_si128((__m128i*)&src[n + 6 * 16]);
        __m128i d7 = _mm_load_si128((__m128i*)&src[n + 7 * 16]);
        _mm_stream_si128((__m128i*)&dst[n + 0 * 16], d0);
        _mm_stream_si128((__m128i*)&dst[n + 1 * 16], d1);
        _mm_stream_si128((__m128i*)&dst[n + 2 * 16], d2);
        _mm_stream_si128((__m128i*)&dst[n + 3 * 16], d3);
        _mm_stream_si128((__m128i*)&dst[n + 4 * 16], d4);
        _mm_stream_si128((__m128i*)&dst[n + 5 * 16], d5);
        _mm_stream_si128((__m128i*)&dst[n + 6 * 16], d6);
        _mm_stream_si128((__m128i*)&dst[n + 7 * 16], d7);
        d0 = _mm_load_si128((__m128i*)&src[n + 8 * 16]);
        d1 = _mm_load_si128((__m128i*)&src[n + 9 * 16]);
        d2 = _mm_load_si128((__m128i*)&src[n + 10 * 16]);
        d3 = _mm_load_si128((__m128i*)&src[n + 11 * 16]);
        d4 = _mm_load_si128((__m128i*)&src[n + 12 * 16]);
        d5 = _mm_load_si128((__m128i*)&src[n + 13 * 16]);
        d6 = _mm_load_si128((__m128i*)&src[n + 14 * 16]);
        d7 = _mm_load_si128((__m128i*)&src[n + 15 * 16]);
        _mm_stream_si128((__m128i*)&dst[n + 8 * 16], d0);
        _mm_stream_si128((__m128i*)&dst[n + 9 * 16], d1);
        _mm_stream_si128((__m128i*)&dst[n + 10 * 16], d2);
        _mm_stream_si128((__m128i*)&dst[n + 11 * 16], d3);
        _mm_stream_si128((__m128i*)&dst[n + 12 * 16], d4);
        _mm_stream_si128((__m128i*)&dst[n + 13 * 16], d5);
        _mm_stream_si128((__m128i*)&dst[n + 14 * 16], d6);
        _mm_stream_si128((__m128i*)&dst[n + 15 * 16], d7);
        n += 256;
    }

    while (n + 128 <= sizeInBytes)
    {
        __m128i d0 = _mm_load_si128((__m128i*)&src[n + 0 * 16]);
        __m128i d1 = _mm_load_si128((__m128i*)&src[n + 1 * 16]);
        __m128i d2 = _mm_load_si128((__m128i*)&src[n + 2 * 16]);
        __m128i d3 = _mm_load_si128((__m128i*)&src[n + 3 * 16]);
        __m128i d4 = _mm_load_si128((__m128i*)&src[n + 4 * 16]);
        __m128i d5 = _mm_load_si128((__m128i*)&src[n + 5 * 16]);
        __m128i d6 = _mm_load_si128((__m128i*)&src[n + 6 * 16]);
        __m128i d7 = _mm_load_si128((__m128i*)&src[n + 7 * 16]);
        _mm_stream_si128((__m128i*)&dst[n + 0 * 16], d0);
        _mm_stream_si128((__m128i*)&dst[n + 1 * 16], d1);
        _mm_stream_si128((__m128i*)&dst[n + 2 * 16], d2);
        _mm_stream_si128((__m128i*)&dst[n + 3 * 16], d3);
        _mm_stream_si128((__m128i*)&dst[n + 4 * 16], d4);
        _mm_stream_si128((__m128i*)&dst[n + 5 * 16], d5);
        _mm_stream_si128((__m128i*)&dst[n + 6 * 16], d6);
        _mm_stream_si128((__m128i*)&dst[n + 7 * 16], d7);
        n += 128;
    }

    while (n + 16 <= sizeInBytes)
    {
        __m128i d0 = _mm_load_si128((__m128i*)&src[n]);
        _mm_stream_si128((__m128i*)&dst[n], d0);
        n += 16;
    }

    while (n + 4 <= sizeInBytes)
    {
        *(uint32_t*)&dst[n] = *(const uint32_t*)&src[n];
        n += 4;
    }

    while (n < sizeInBytes)
    {
        dst[n] = src[n];
        n++;
    }

    _mm_sfence();
}

void _ZeroMemSSE(byte* dst, size_t sizeInBytes)
{
    HK_ASSERT(IsSSEAligned((size_t)dst));

    int n = 0;

    __m128i zero = _mm_setzero_si128();

    while (n + 256 <= sizeInBytes)
    {
        _mm_stream_si128((__m128i*)&dst[n + 0 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 1 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 2 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 3 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 4 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 5 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 6 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 7 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 8 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 9 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 10 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 11 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 12 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 13 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 14 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 15 * 16], zero);
        n += 256;
    }

    while (n + 128 <= sizeInBytes)
    {
        _mm_stream_si128((__m128i*)&dst[n + 0 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 1 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 2 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 3 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 4 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 5 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 6 * 16], zero);
        _mm_stream_si128((__m128i*)&dst[n + 7 * 16], zero);
        n += 128;
    }

    while (n + 16 <= sizeInBytes)
    {
        _mm_stream_si128((__m128i*)&dst[n], zero);
        n += 16;
    }

    while (n + 4 <= sizeInBytes)
    {
        *(uint32_t*)&dst[n] = 0;
        n += 4;
    }

    while (n < sizeInBytes)
    {
        dst[n] = 0;
        n++;
    }

    _mm_sfence();
}

void _MemsetSSE(byte* dst, int _val, size_t sizeInBytes)
{
    HK_ASSERT(IsSSEAligned((size_t)dst));

    alignas(16) int32_t data[4];

    std::memset(data, _val, sizeof(data));

    __m128i val = _mm_load_si128((__m128i*)&data[0]);

    int n = 0;

    while (n + 256 <= sizeInBytes)
    {
        _mm_stream_si128((__m128i*)&dst[n + 0 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 1 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 2 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 3 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 4 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 5 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 6 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 7 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 8 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 9 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 10 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 11 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 12 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 13 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 14 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 15 * 16], val);
        n += 256;
    }

    while (n + 128 <= sizeInBytes)
    {
        _mm_stream_si128((__m128i*)&dst[n + 0 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 1 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 2 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 3 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 4 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 5 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 6 * 16], val);
        _mm_stream_si128((__m128i*)&dst[n + 7 * 16], val);
        n += 128;
    }

    while (n + 16 <= sizeInBytes)
    {
        _mm_stream_si128((__m128i*)&dst[n], val);
        n += 16;
    }

    while (n + 4 <= sizeInBytes)
    {
        *(uint32_t*)&dst[n] = data[0];
        n += 4;
    }

    while (n < sizeInBytes)
    {
        dst[n] = data[0];
        n++;
    }

    _mm_sfence();
}

} // namespace Core

#ifndef USE_MIMALLOC

struct HeapChunk
{
    uint32_t Size;
    uint32_t Offset;
};

void* MemoryHeap::_Alloc(size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags)
{
    HK_VERIFY(sizeInBytes <= std::numeric_limits<uint32_t>::max(), "MemoryAlloc: Too large allocation\n");

    constexpr size_t DefaultAlignment =
#    if defined(__GNUC__)
        __BIGGEST_ALIGNMENT__;
#    else
        16;
#    endif

    if (alignment == 0)
    {
        alignment = DefaultAlignment;
    }
    else
    {
        HK_VERIFY(IsPowerOfTwo(alignment), "MemoryAlloc: Alignment must be power of two\n");
    }

    size_t size = sizeInBytes + sizeof(HeapChunk) + (alignment - 1);

    byte* p = (byte*)malloc(size);
    if (HK_UNLIKELY(!p))
        return nullptr;

    void* aligned = AlignPtr(p + sizeof(HeapChunk), alignment);

    size_t offset = ((byte*)aligned - p);
    HK_VERIFY(offset <= std::numeric_limits<uint32_t>::max(), "MemoryAlloc: Too big alignment\n");

    (((HeapChunk*)aligned) - 1)->Offset = (uint32_t)offset;
    (((HeapChunk*)aligned) - 1)->Size   = sizeInBytes;

    if (flags & MALLOC_ZERO)
        Core::ZeroMem(aligned, sizeInBytes);

    m_PeakAllocated.Store(std::max(m_PeakAllocated.Load(), m_MemoryAllocated.Add(sizeInBytes)));
    m_MemoryAllocs.Increment();
    m_PerFrameAllocs.Increment();

    return aligned;
}

void MemoryHeap::Free(void* ptr)
{
    if (!ptr)
        return;

    m_MemoryAllocated.Sub((((HeapChunk*)ptr) - 1)->Size);
    m_MemoryAllocs.Decrement();
    m_PerFrameFrees.Increment();

    free((byte*)ptr - (((HeapChunk*)ptr) - 1)->Offset);
}

size_t MemoryHeap::GetSize(void* ptr)
{
    return ptr ? (((HeapChunk*)ptr) - 1)->Size : 0;
}

void* MemoryHeap::_Realloc(void* ptr, size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags)
{
    if (!ptr)
        return _Alloc(sizeInBytes, alignment, flags);

    size_t OldSize = (((HeapChunk*)ptr) - 1)->Size;
    if (OldSize >= sizeInBytes && IsAlignedPtr(ptr, alignment))
        return ptr;

    void* NewPtr = _Alloc(sizeInBytes, alignment, flags);
    if (HK_LIKELY(NewPtr))
    {
        if (!(flags & MALLOC_DISCARD))
            Core::Memcpy(NewPtr, ptr, OldSize);
    }

    m_MemoryAllocated.Sub((((HeapChunk*)ptr) - 1)->Size);

    free((byte*)ptr - (((HeapChunk*)ptr) - 1)->Offset);
    m_PerFrameFrees.Increment();
    m_MemoryAllocs.Decrement();

    return NewPtr;
}

#else

void* MemoryHeap::_Alloc(size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags)
{
    void* ptr;

    if (alignment == 0)
    {
        ptr = (flags & MALLOC_ZERO) ? mi_zalloc(sizeInBytes) : mi_malloc(sizeInBytes);
    }
    else
    {
        HK_VERIFY(IsPowerOfTwo(alignment), "MemoryAlloc: Alignment must be power of two\n");
        HK_VERIFY(alignment <= MI_ALIGNMENT_MAX, "MemoryAlloc: Too big alignment\n");

        ptr = (flags & MALLOC_ZERO) ? mi_zalloc_aligned(sizeInBytes, alignment) : mi_malloc_aligned(sizeInBytes, alignment);
    }

    if (HK_UNLIKELY(!ptr))
        return nullptr;
    HK_ASSERT(sizeInBytes <= mi_malloc_size(ptr));
    sizeInBytes = mi_malloc_size(ptr);
    m_PeakAllocated.Store(std::max(m_PeakAllocated.Load(), m_MemoryAllocated.Add(sizeInBytes)));
    m_MemoryAllocs.Increment();
    m_PerFrameAllocs.Increment();

    return ptr;
}

void MemoryHeap::Free(void* ptr)
{
    if (!ptr)
        return;

    m_MemoryAllocated.Sub(mi_malloc_size(ptr));
    m_MemoryAllocs.Decrement();
    m_PerFrameFrees.Increment();
    mi_free(ptr);
}

size_t MemoryHeap::GetSize(void* ptr)
{
    return ptr ? mi_malloc_size(ptr) : 0;
}

void* MemoryHeap::_Realloc(void* ptr, size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags)
{
    if (!ptr)
        return _Alloc(sizeInBytes, alignment, flags);

    if (flags & MALLOC_DISCARD)
    {
        Free(ptr);
        return _Alloc(sizeInBytes, alignment, flags);
    }

    m_MemoryAllocated.Sub(mi_malloc_size(ptr));

    ptr = alignment == 0 ? mi_realloc(ptr, sizeInBytes) : mi_realloc_aligned(ptr, sizeInBytes, alignment);
    if (HK_LIKELY(ptr))
    {
        HK_ASSERT(sizeInBytes <= mi_malloc_size(ptr));
        sizeInBytes = mi_malloc_size(ptr);
        m_PeakAllocated.Store(std::max(m_PeakAllocated.Load(), m_MemoryAllocated.Add(sizeInBytes)));

        m_PerFrameAllocs.Increment();
        m_MemoryAllocs.Increment();
    }

    m_PerFrameFrees.Increment();
    m_MemoryAllocs.Decrement();

    return ptr;
}

#endif

void* MemoryHeap::Alloc(size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags)
{
    HK_IF_NOT_ASSERT(sizeInBytes != 0)
    {
        sizeInBytes = 1;
    }

    void* ptr = _Alloc(sizeInBytes, alignment, flags);
    if (HK_UNLIKELY(!ptr))
        CoreApplication::sTerminateWithError("Failed on allocation of {} bytes\n", sizeInBytes);
    return ptr;
}

void* MemoryHeap::Realloc(void* ptr, size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags)
{
    HK_IF_NOT_ASSERT(sizeInBytes != 0)
    {
        sizeInBytes = 1;
    }

    ptr = _Realloc(ptr, sizeInBytes, alignment, flags);
    if (HK_UNLIKELY(!ptr))
        CoreApplication::sTerminateWithError("Failed on allocation of {} bytes\n", sizeInBytes);
    return ptr;
}

MemoryStat MemoryHeap::sMemoryGetStat()
{
    MemoryStat stat = {};
    for (int n = 0; n < HEAP_MAX; n++)
    {
        MemoryStat heapStat = Core::MemoryHeaps[n].GetStat();
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

void MemoryHeap::sMemoryNewFrame()
{
    using namespace Core;
    for (int n = 0; n < HEAP_MAX; n++)
    {
        MemoryHeaps[n].m_PerFrameAllocs.Store(0);
        MemoryHeaps[n].m_PerFrameFrees.Store(0);
    }
}

void MemoryHeap::sMemoryCleanup()
{
}

HK_NAMESPACE_END


void* operator new(std::size_t sz)
{
    if (void* ptr = Hk::Core::GetHeapAllocator<Hk::HEAP_MISC>().Alloc(std::max<size_t>(1, sz)))
        return ptr;

    // By default, exceptions are disabled in Hork.
#ifdef __cpp_exceptions
    throw std::bad_alloc{};
#else
    // Calling abort() to prevent compiler warnings.
    std::abort();
#endif
}

void operator delete(void* ptr) noexcept
{
    Hk::Core::GetHeapAllocator<Hk::HEAP_MISC>().Free(ptr);
}

void* operator new[](std::size_t sz)
{
    if (void* ptr = Hk::Core::GetHeapAllocator<Hk::HEAP_MISC>().Alloc(std::max<size_t>(1, sz)))
        return ptr;

    // By default, exceptions are disabled in Hork.
#ifdef __cpp_exceptions
    throw std::bad_alloc{};
#else
    // Calling abort() to prevent compiler warnings.
    std::abort();
#endif
}

void operator delete[](void* ptr) noexcept
{
    Hk::Core::GetHeapAllocator<Hk::HEAP_MISC>().Free(ptr);
}
