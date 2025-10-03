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

#pragma once

#include "Thread.h"
#include <cstring> // std::memcpy

HK_NAMESPACE_BEGIN

enum MEMORY_HEAP
{
    HEAP_STRING,
    HEAP_VECTOR,
    HEAP_HASH_SET,
    HEAP_HASH_MAP,
    HEAP_CPU_VERTEX_BUFFER,
    HEAP_CPU_INDEX_BUFFER,
    HEAP_IMAGE,
    HEAP_AUDIO_DATA,
    HEAP_RHI,
    HEAP_PHYSICS,
    HEAP_NAVIGATION,
    HEAP_TEMP,
    HEAP_MISC,
    HEAP_WORLD_OBJECTS,

    HEAP_MAX
};

enum MALLOC_FLAGS : uint32_t
{
    MALLOC_FLAGS_DEFAULT = 0,
    MALLOC_ZERO    = HK_BIT(0),
    MALLOC_DISCARD = HK_BIT(1)
};

HK_FLAG_ENUM_OPERATORS(MALLOC_FLAGS)

struct MemoryStat
{
    size_t FrameAllocs;
    size_t FrameFrees;
    size_t MemoryAllocated;
    size_t MemoryAllocs;
    size_t MemoryPeakAlloc;
};

struct MemoryHeap
{
    static void       sMemoryNewFrame();
    static void       sMemoryCleanup();
    static MemoryStat sMemoryGetStat();

    void*      Alloc(size_t sizeInBytes, size_t alignment = 16, MALLOC_FLAGS flags = MALLOC_FLAGS_DEFAULT);
    void*      Realloc(void* ptr, size_t sizeInBytes, size_t alignment = 16, MALLOC_FLAGS flags = MALLOC_FLAGS_DEFAULT);
    void       Free(void* ptr);
    size_t     GetSize(void* ptr);
    MemoryStat GetStat();

private:
    void* _Alloc(size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags);
    void* _Realloc(void* ptr, size_t sizeInBytes, size_t alignment, MALLOC_FLAGS flags);

    AtomicLong m_MemoryAllocated{};
    AtomicLong m_MemoryAllocs{};
    AtomicLong m_PeakAllocated{};
    AtomicLong m_PerFrameAllocs{};
    AtomicLong m_PerFrameFrees{};
};

namespace Core
{

template <MEMORY_HEAP Heap>
MemoryHeap& GetHeapAllocator()
{
    extern MemoryHeap MemoryHeaps[];
    return MemoryHeaps[Heap];
}

/// Built-in memcpy function replacement
void _MemcpySSE(byte* dst, const byte* src, size_t sizeInBytes);

/// Built-in memset function replacement
void _MemsetSSE(byte* dst, int val, size_t sizeInBytes);

/// Built-in memset function replacement
void _ZeroMemSSE(byte* dst, size_t sizeInBytes);

/// Built-in memcpy function replacement
HK_FORCEINLINE void Memcpy(void* dst, const void* src, size_t sizeInBytes)
{
    if (IsSSEAligned((size_t)dst) && IsSSEAligned((size_t)src))
    {
        _MemcpySSE((byte*)dst, (const byte*)src, sizeInBytes);
    }
    else
    {
        std::memcpy(dst, src, sizeInBytes);
    }
}

/// Built-in memset function replacement
HK_FORCEINLINE void Memset(void* dst, int val, size_t sizeInBytes)
{
    if (IsSSEAligned((size_t)dst))
    {
        _MemsetSSE((byte*)dst, val, sizeInBytes);
    }
    else
    {
        std::memset(dst, val, sizeInBytes);
    }
}

/// Built-in memset function replacement
HK_FORCEINLINE void ZeroMem(void* dst, size_t sizeInBytes)
{
    if (IsSSEAligned((size_t)dst))
    {
        _ZeroMemSSE((byte*)dst, sizeInBytes);
    }
    else
    {
        std::memset(dst, 0, sizeInBytes);
    }
}

/// Built-in memmove function replacement
HK_FORCEINLINE void* Memmove(void* dst, const void* src, size_t sizeInBytes)
{
    return std::memmove(dst, src, sizeInBytes);
}

} // namespace Core


/**

Dynamic Stack Memory

*/

#define HkStackAlloc(_NumBytes) alloca(_NumBytes)


namespace Allocators
{

using EASTLDummyAllocator = eastl::dummy_allocator;

class MemoryAllocatorBase
{
public:
    const char* get_name() const
    {
        return "MemoryAllocator";
    }

    void set_name(const char* name)
    {}
};

template <MEMORY_HEAP Heap>
class HeapMemoryAllocator : public MemoryAllocatorBase
{
public:
    explicit HeapMemoryAllocator(const char* name = nullptr)
    {}

    HeapMemoryAllocator(HeapMemoryAllocator const& rhs)
    {}

    HeapMemoryAllocator(HeapMemoryAllocator const& x, const char* name)
    {}

    HeapMemoryAllocator& operator=(HeapMemoryAllocator const& rhs)
    {
        return *this;
    }

    void* allocate(size_t n, int flags = 0)
    {
        return Core::GetHeapAllocator<Heap>().Alloc(n, EASTL_SYSTEM_ALLOCATOR_MIN_ALIGNMENT);
    }

    void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
    {
        return Core::GetHeapAllocator<Heap>().Alloc(n, alignment);
    }

    void* reallocate(void* p, size_t n, bool copyOld)
    {
        return Core::GetHeapAllocator<Heap>().Realloc(p, n, EASTL_SYSTEM_ALLOCATOR_MIN_ALIGNMENT, copyOld ? MALLOC_FLAGS_DEFAULT : MALLOC_DISCARD);
    }

    void deallocate(void* p, size_t n)
    {
        Core::GetHeapAllocator<Heap>().Free(p);
    }

    void deallocate(void* p)
    {
        Core::GetHeapAllocator<Heap>().Free(p);
    }
};

template <MEMORY_HEAP Heap>
HK_FORCEINLINE bool operator==(HeapMemoryAllocator<Heap> const&, HeapMemoryAllocator<Heap> const&) { return true; }

template <MEMORY_HEAP Heap>
HK_FORCEINLINE bool operator!=(HeapMemoryAllocator<Heap> const&, HeapMemoryAllocator<Heap> const&) { return false; }


template <typename T, MEMORY_HEAP Heap>
struct StdHeapAllocator
{
    typedef T value_type;

    StdHeapAllocator() = default;
    template <typename U> constexpr StdHeapAllocator(StdHeapAllocator<U, Heap> const&) noexcept {}

    HK_NODISCARD T* allocate(std::size_t count) noexcept
    {
        HK_ASSERT(count <= std::numeric_limits<std::size_t>::max() / sizeof(T));

        return static_cast<T*>(Core::GetHeapAllocator<Heap>().Alloc(count * sizeof(T)));
    }

    void deallocate(T* bytes, std::size_t count) noexcept
    {
        Core::GetHeapAllocator<Heap>().Free(bytes);
    }
};
template <typename T, typename U, MEMORY_HEAP Heap> bool operator==(StdHeapAllocator<T, Heap> const&, StdHeapAllocator<U, Heap> const&) { return true; }
template <typename T, typename U, MEMORY_HEAP Heap> bool operator!=(StdHeapAllocator<T, Heap> const&, StdHeapAllocator<U, Heap> const&) { return false; }

}

HK_NAMESPACE_END
