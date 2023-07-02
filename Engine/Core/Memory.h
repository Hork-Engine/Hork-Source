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
    static void       MemoryNewFrame();
    static void       MemoryCleanup();
    static MemoryStat MemoryGetStat();

    void*      Alloc(size_t SizeInBytes, size_t Alignment = 16, MALLOC_FLAGS Flags = MALLOC_FLAGS_DEFAULT);
    void*      Realloc(void* Ptr, size_t SizeInBytes, size_t Alignment = 16, MALLOC_FLAGS Flags = MALLOC_FLAGS_DEFAULT);
    void       Free(void* Ptr);
    size_t     GetSize(void* Ptr);
    MemoryStat GetStat();

private:
    void* _Alloc(size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags);
    void* _Realloc(void* Ptr, size_t SizeInBytes, size_t Alignment, MALLOC_FLAGS Flags);

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

/** Built-in memcpy function replacement */
void _MemcpySSE(byte* _Dst, const byte* _Src, size_t _SizeInBytes);

/** Built-in memset function replacement */
void _MemsetSSE(byte* _Dst, int _Val, size_t _SizeInBytes);

/** Built-in memset function replacement */
void _ZeroMemSSE(byte* _Dst, size_t _SizeInBytes);

/** Built-in memcpy function replacement */
HK_FORCEINLINE void Memcpy(void* _Dst, const void* _Src, size_t _SizeInBytes)
{
    if (IsSSEAligned((size_t)_Dst) && IsSSEAligned((size_t)_Src))
    {
        _MemcpySSE((byte*)_Dst, (const byte*)_Src, _SizeInBytes);
    }
    else
    {
        std::memcpy(_Dst, _Src, _SizeInBytes);
    }
}

/** Built-in memset function replacement */
HK_FORCEINLINE void Memset(void* _Dst, int _Val, size_t _SizeInBytes)
{
    if (IsSSEAligned((size_t)_Dst))
    {
        _MemsetSSE((byte*)_Dst, _Val, _SizeInBytes);
    }
    else
    {
        std::memset(_Dst, _Val, _SizeInBytes);
    }
}

/** Built-in memset function replacement */
HK_FORCEINLINE void ZeroMem(void* _Dst, size_t _SizeInBytes)
{
    if (IsSSEAligned((size_t)_Dst))
    {
        _ZeroMemSSE((byte*)_Dst, _SizeInBytes);
    }
    else
    {
        std::memset(_Dst, 0, _SizeInBytes);
    }
}

/** Built-in memmove function replacement */
HK_FORCEINLINE void* Memmove(void* _Dst, const void* _Src, size_t _SizeInBytes)
{
    return std::memmove(_Dst, _Src, _SizeInBytes);
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

    void set_name(const char* pName)
    {}
};

template <MEMORY_HEAP Heap>
class HeapMemoryAllocator : public MemoryAllocatorBase
{
public:
    explicit HeapMemoryAllocator(const char* pName = nullptr)
    {}

    HeapMemoryAllocator(HeapMemoryAllocator const& rhs)
    {}

    HeapMemoryAllocator(HeapMemoryAllocator const& x, const char* pName)
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
struct TStdHeapAllocator
{
    typedef T value_type;

    TStdHeapAllocator() = default;
    template <typename U> constexpr TStdHeapAllocator(TStdHeapAllocator<U, Heap> const&) noexcept {}

    HK_NODISCARD T* allocate(std::size_t _Count) noexcept
    {
        HK_ASSERT(_Count <= std::numeric_limits<std::size_t>::max() / sizeof(T));

        return static_cast<T*>(Core::GetHeapAllocator<Heap>().Alloc(_Count * sizeof(T)));
    }

    void deallocate(T* _Bytes, std::size_t _Count) noexcept
    {
        Core::GetHeapAllocator<Heap>().Free(_Bytes);
    }
};
template <typename T, typename U, MEMORY_HEAP Heap> bool operator==(TStdHeapAllocator<T, Heap> const&, TStdHeapAllocator<U, Heap> const&) { return true; }
template <typename T, typename U, MEMORY_HEAP Heap> bool operator!=(TStdHeapAllocator<T, Heap> const&, TStdHeapAllocator<U, Heap> const&) { return false; }

}

HK_NAMESPACE_END
