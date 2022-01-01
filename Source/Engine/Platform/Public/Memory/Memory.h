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

#include "../Thread.h"
#include <malloc.h>

/*

Memory utilites

*/

namespace Core
{

/** Built-in memcpy function replacement */
void _MemcpySSE(byte* _Dst, const byte* _Src, size_t _SizeInBytes);

/** Built-in memset function replacement */
void _MemsetSSE(byte* _Dst, int _Val, size_t _SizeInBytes);

/** Built-in memset function replacement */
void _ZeroMemSSE(byte* _Dst, size_t _SizeInBytes);

/** Built-in memcpy function replacement */
AN_FORCEINLINE void Memcpy(void* _Dst, const void* _Src, size_t _SizeInBytes)
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
AN_FORCEINLINE void Memset(void* _Dst, int _Val, size_t _SizeInBytes)
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
AN_FORCEINLINE void ZeroMem(void* _Dst, size_t _SizeInBytes)
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
AN_FORCEINLINE void* Memmove(void* _Dst, const void* _Src, size_t _SizeInBytes)
{
    return std::memmove(_Dst, _Src, _SizeInBytes);
}


void* SysAlloc(size_t _SizeInBytes, int _Alignment = 16);

void* SysRealloc(void* _Bytes, size_t _SizeInBytes, int _Alignment = 16);

void SysFree(void* _Bytes);

} // namespace Core

/**

AHeapMemory

Allocates memory on heap

*/
class AHeapMemory final
{
    AN_FORBID_COPY(AHeapMemory)

public:
    AHeapMemory();
    ~AHeapMemory();

    /** Initialize memory (main thread only) */
    void Initialize();

    /** Deinitialize memory (main thread only) */
    void Deinitialize();

    /** Heap memory allocation (thread safe). Minimal alignment is 16. */
    void* Alloc(size_t _BytesCount, int _Alignment = 16);

    /** Heap memory allocation (thread safe) */
    void* ClearedAlloc(size_t _BytesCount, int _Alignment = 16);

    /** Tries to realloc existing buffer (thread safe) */
    void* Realloc(void* _Data, int _NewBytesCount, int _NewAlignment, bool _KeepOld);

    /** Heap memory deallocation (thread safe) */
    void Free(void* _Bytes);

    /** Check if memory was trashed (thread safe) */
    void PointerTrashTest(void* _Bytes);

    /** Clearing whole heap memory (main thread only) */
    void Clear();

    /** Statistics: current memory usage */
    size_t GetTotalMemoryUsage();

    /** Statistics: current memory usage overhead */
    size_t GetTotalMemoryOverhead();

    /** Statistics: max memory usage during memory using */
    size_t GetMaxMemoryUsage();

private:
    void CheckMemoryLeaks();

    void IncMemoryStatistics(size_t _MemoryUsage, size_t _Overhead);
    void DecMemoryStatistics(size_t _MemoryUsage, size_t _Overhead);

    mutable ASpinLock StatisticLock;
    size_t            TotalMemoryUsage;
    size_t            TotalMemoryOverhead;
    size_t            MaxMemoryUsage;

    struct SHeapChunk
    {
        SHeapChunk* pNext;
        SHeapChunk* pPrev;
        uint32_t    Size;
        uint32_t    DataSize;
        uint32_t    AlignOffset; // to keep heap aligned (one byte is enough for it)
        uint32_t    Alignment;   // one byte is enough for it
    };

    SHeapChunk HeapChain;

    ASpinLock Mutex;
};

AN_FORCEINLINE void* AHeapMemory::ClearedAlloc(size_t _BytesCount, int _Alignment)
{
    void* bytes = Alloc(_BytesCount, _Alignment);
    Core::ZeroMem(bytes, _BytesCount);
    return bytes;
}

/**

AHunkMemory

For large temporary blocks like textures, meshes, etc

Example:

int Mark = HunkMemory.SetHunkMark();

byte * buffer1 = HunkMemory.Alloc( bufferSize1 );
byte * buffer2 = HunkMemory.Alloc( bufferSize2 );
...

HunkMemory.ClearToMark( Mark ); // here all buffers allocated after SetHunkMark will be deallocated

Allocated chunks are aligned at 16-byte boundary.
If you need other alignment, do it on top of the allocator.

Only for main thread.

*/
class AHunkMemory final
{
    AN_FORBID_COPY(AHunkMemory)

public:
    AHunkMemory() {}

    /** Initialize memory */
    void Initialize(void* _MemoryAddress, int _SizeInMegabytes);

    /** Deinitialize memory */
    void Deinitialize();

    void* GetHunkMemoryAddress() const;
    int   GetHunkMemorySizeInMegabytes() const;

    /** Hunk memory allocation */
    void* Alloc(size_t _BytesCount);

    /** Hunk memory allocation */
    void* ClearedAlloc(size_t _BytesCount);

    /** Memory control functions */
    int  SetHunkMark();
    void ClearToMark(int _Mark);
    void Clear();

    /** Clear last allocated hunk */
    void ClearLastHunk();

    /** Statistics: current memory usage */
    size_t GetTotalMemoryUsage() const;

    /** Statistics: current memory usage overhead */
    size_t GetTotalMemoryOverhead() const;

    /** Statistics: current free memory */
    size_t GetTotalFreeMemory() const;

    /** Statistics: max memory usage during memory using */
    size_t GetMaxMemoryUsage() const;

private:
    void CheckMemoryLeaks();

    void IncMemoryStatistics(size_t _MemoryUsage, size_t _Overhead);
    void DecMemoryStatistics(size_t _MemoryUsage, size_t _Overhead);

    struct SHunkMemory* MemoryBuffer = nullptr;

    size_t TotalMemoryUsage    = 0;
    size_t TotalMemoryOverhead = 0;
    size_t MaxMemoryUsage      = 0;
};

AN_FORCEINLINE void* AHunkMemory::ClearedAlloc(size_t _BytesCount)
{
    void* bytes = Alloc(_BytesCount);
    Core::ZeroMem(bytes, _BytesCount);
    return bytes;
}

/**

AZoneMemory

For small blocks, objects or strings.
Allocated chunks are aligned at 16-byte boundary.
If you need other alignment, do it on top of the allocator.

Only for main thread.

*/
class AZoneMemory final
{
    AN_FORBID_COPY(AZoneMemory)

public:
    AZoneMemory() {}

    /** Initialize memory */
    void Initialize(void* _MemoryAddress, int _SizeInMegabytes);

    /** Deinitialize memory */
    void Deinitialize();

    void* GetZoneMemoryAddress() const;
    int   GetZoneMemorySizeInMegabytes() const;

    /** Zone memory allocation */
    void* Alloc(size_t _BytesCount);

    /** Zone memory allocation */
    void* ClearedAlloc(size_t _BytesCount);

    /** Tries to realloc existing buffer */
    void* Realloc(void* _Data, int _NewBytesCount, bool _KeepOld);

    /** Zone memory deallocation */
    void Free(void* _Bytes);

    /** Clearing whole zone memory */
    void Clear();

    /** Statistics: current memory usage */
    size_t GetTotalMemoryUsage() const;

    /** Statistics: current memory usage overhead */
    size_t GetTotalMemoryOverhead() const;

    /** Statistics: current free memory */
    size_t GetTotalFreeMemory() const;

    /** Statistics: max memory usage during memory using */
    size_t GetMaxMemoryUsage() const;

private:
    struct SZoneChunk* FindFreeChunk(int _RequiredSize);

    void CheckMemoryLeaks();

    void IncMemoryStatistics(size_t _MemoryUsage, size_t _Overhead);
    void DecMemoryStatistics(size_t _MemoryUsage, size_t _Overhead);

    struct SZoneBuffer* MemoryBuffer = nullptr;

    mutable ASpinLock StatisticLock;
    size_t            TotalMemoryUsage;
    size_t            TotalMemoryOverhead;
    size_t            MaxMemoryUsage;

#ifdef AN_ZONE_MULTITHREADED_ALLOC
    AThreadSync Sync;
#endif
};

AN_FORCEINLINE void* AZoneMemory::ClearedAlloc(size_t _BytesCount)
{
    void* bytes = Alloc(_BytesCount);
    Core::ZeroMem(bytes, _BytesCount);
    return bytes;
}

extern AHeapMemory GHeapMemory;
extern AHunkMemory GHunkMemory;
extern AZoneMemory GZoneMemory;

/**

TTemplateAllocator

Allocator base interface

*/
template <typename T>
class TTemplateAllocator : public ANoncopyable
{
public:
    void* Alloc(size_t _BytesCount)
    {
        return static_cast<T*>(this)->ImplAlloc(_BytesCount);
    }

    void* ClearedAlloc(size_t _BytesCount)
    {
        void* bytes = static_cast<T*>(this)->ImplAlloc(_BytesCount);
        Core::ZeroMem(bytes, _BytesCount);
        return bytes;
    }

    void* Realloc(void* _Data, size_t _NewBytesCount, bool _KeepOld)
    {
        return static_cast<T*>(this)->ImplRealloc(_Data, _NewBytesCount, _KeepOld);
    }

    void Free(void* _Bytes)
    {
        static_cast<T*>(this)->ImplFree(_Bytes);
    }
};

/**

AZoneAllocator

Use for small objects

*/
class AZoneAllocator final : public TTemplateAllocator<AZoneAllocator>
{
public:
    AZoneAllocator() {}

    static AZoneAllocator& Inst()
    {
        static AZoneAllocator inst;
        return inst;
    }

    void* ImplAlloc(size_t _BytesCount)
    {
        return GZoneMemory.Alloc(_BytesCount);
    }

    void* ImplRealloc(void* _Data, size_t _NewBytesCount, bool _KeepOld)
    {
        return GZoneMemory.Realloc(_Data, _NewBytesCount, _KeepOld);
    }

    void ImplFree(void* _Bytes)
    {
        GZoneMemory.Free(_Bytes);
    }
};

/**

AHeapAllocator

Use for huge data allocation

*/
template <int Alignment>
class AHeapAllocator final : public TTemplateAllocator<AHeapAllocator<Alignment>>
{
public:
    AHeapAllocator() {}

    static AHeapAllocator<Alignment>& Inst()
    {
        static AHeapAllocator<Alignment> inst;
        return inst;
    }

    void* ImplAlloc(size_t _BytesCount)
    {
        return GHeapMemory.Alloc(_BytesCount, Alignment);
    }

    void* ImplRealloc(void* _Data, size_t _NewBytesCount, bool _KeepOld)
    {
        return GHeapMemory.Realloc(_Data, _NewBytesCount, Alignment, _KeepOld);
    }

    void ImplFree(void* _Bytes)
    {
        GHeapMemory.Free(_Bytes);
    }
};


#ifdef AN_CPP20
#    define AN_NODISCARD [[nodiscard]]
#else
#    define AN_NODISCARD
#endif

/**

TStdZoneAllocator

*/
template <typename T>
struct TStdZoneAllocator
{
    typedef T value_type;

    TStdZoneAllocator() = default;
    template <typename U> constexpr TStdZoneAllocator(TStdZoneAllocator<U> const&) noexcept {}

    AN_NODISCARD T* allocate(std::size_t _Count) noexcept
    {
        AN_ASSERT(_Count <= std::numeric_limits<std::size_t>::max() / sizeof(T));

        return static_cast<T*>(GZoneMemory.Alloc(_Count * sizeof(T)));
    }

    void deallocate(T* _Bytes, std::size_t _Count) noexcept
    {
        GZoneMemory.Free(_Bytes);
    }
};
template <typename T, typename U> bool operator==(TStdZoneAllocator<T> const&, TStdZoneAllocator<U> const&) { return true; }
template <typename T, typename U> bool operator!=(TStdZoneAllocator<T> const&, TStdZoneAllocator<U> const&) { return false; }

/**

TStdHeapAllocator

*/
template <typename T>
struct TStdHeapAllocator
{
    typedef T value_type;

    TStdHeapAllocator() = default;
    template <typename U> constexpr TStdHeapAllocator(TStdHeapAllocator<U> const&) noexcept {}

    AN_NODISCARD T* allocate(std::size_t _Count) noexcept
    {
        AN_ASSERT(_Count <= std::numeric_limits<std::size_t>::max() / sizeof(T));

        return static_cast<T*>(GHeapMemory.Alloc(_Count * sizeof(T)));
    }

    void deallocate(T* _Bytes, std::size_t _Count) noexcept
    {
        GHeapMemory.Free(_Bytes);
    }
};
template <typename T, typename U> bool operator==(TStdHeapAllocator<T> const&, TStdHeapAllocator<U> const&) { return true; }
template <typename T, typename U> bool operator!=(TStdHeapAllocator<T> const&, TStdHeapAllocator<U> const&) { return false; }


/**

Dynamic Stack Memory

*/

#define StackAlloc(_NumBytes) alloca(_NumBytes)
