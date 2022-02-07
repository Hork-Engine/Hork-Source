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

#include <Platform/Memory/Memory.h>
#include <Platform/Logger.h>
#include <Platform/Platform.h>
#include <Platform/WindowsDefs.h>

#include <malloc.h>
#include <memory.h>
#include <xmmintrin.h>
#include <immintrin.h>

//////////////////////////////////////////////////////////////////////////////////////////
//
// Memory public
//
//////////////////////////////////////////////////////////////////////////////////////////

AHeapMemory GHeapMemory;
AHunkMemory GHunkMemory;
AZoneMemory GZoneMemory;

//////////////////////////////////////////////////////////////////////////////////////////
//
// Memory common functions/constants/defines
//
//////////////////////////////////////////////////////////////////////////////////////////

#define MEMORY_TRASH_TEST

// Uncomment the following two lines to check for a memory leak
//#define ZONE_MEMORY_LEAK_ADDRESS <local address>
//#define ZONE_MEMORY_LEAK_SIZE    <size>

#ifdef HK_ZONE_MULTITHREADED_ALLOC
#    define ZONE_SYNC_GUARD      ASyncGuard syncGuard(Sync);
#    define ZONE_SYNC_GUARD_STAT ASpinLockGuard lockGuard(StatisticLock);
#else
#    define ZONE_SYNC_GUARD
#    define ZONE_SYNC_GUARD_STAT
#endif

using TRASH_MARKER = uint16_t;

static const TRASH_MARKER TrashMarker = 0xfeee;

static ALogger MemLogger;

//////////////////////////////////////////////////////////////////////////////////////////
//
// Heap Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

AHeapMemory::AHeapMemory()
{
    HeapChain.pNext = HeapChain.pPrev = &HeapChain;

    TotalMemoryUsage    = 0;
    TotalMemoryOverhead = 0;
    MaxMemoryUsage      = 0;

    HK_VALIDATE_TYPE_SIZE(SHeapChunk, 32);
}

AHeapMemory::~AHeapMemory()
{
}

void AHeapMemory::Initialize()
{
}

void AHeapMemory::Deinitialize()
{
    CheckMemoryLeaks();
    Clear();
}

void AHeapMemory::Clear()
{
    SHeapChunk* nextHeap;
    for (SHeapChunk* heap = HeapChain.pNext; heap != &HeapChain; heap = nextHeap)
    {
        nextHeap = heap->pNext;
        Free(heap + 1);
    }
}

void AHeapMemory::IncMemoryStatistics(size_t _MemoryUsage, size_t _Overhead)
{
    ASpinLockGuard lockGuard(StatisticLock);
    TotalMemoryUsage += _MemoryUsage;
    TotalMemoryOverhead += _Overhead;
    MaxMemoryUsage = std::max(MaxMemoryUsage, TotalMemoryUsage);
}

void AHeapMemory::DecMemoryStatistics(size_t _MemoryUsage, size_t _Overhead)
{
    ASpinLockGuard lockGuard(StatisticLock);
    TotalMemoryUsage -= _MemoryUsage;
    TotalMemoryOverhead -= _Overhead;
}

namespace Platform
{

void* SysAlloc(size_t _SizeInBytes, int _Alignment)
{
    void* ptr;
#ifdef HK_COMPILER_MSVC
    ptr = _aligned_malloc(_SizeInBytes, _Alignment);
#else
    ptr = aligned_alloc(_Alignment, _SizeInBytes);
#endif
    if (!ptr)
    {
        CriticalError("SysAlloc: Failed on allocation of %u bytes\n", _SizeInBytes);
    }
    return ptr;
}

void* SysRealloc(void* _Bytes, size_t _SizeInBytes, int _Alignment)
{
    void* ptr;
#ifdef HK_COMPILER_MSVC
    ptr = _aligned_realloc(_Bytes, _SizeInBytes, _Alignment);
#else
    // Need a workaround here :(
    auto oldSize = malloc_usable_size(_Bytes);
    if (oldSize >= _SizeInBytes && IsAlignedPtr(_Bytes, _Alignment))
        return _Bytes;

    ptr = aligned_alloc(_Alignment, _SizeInBytes);
    if (ptr)
        Platform::Memcpy(ptr, _Bytes, oldSize);
    free(_Bytes);
#endif
    if (!ptr)
    {
        CriticalError("SysRealloc: Failed on allocation of %u bytes\n", _SizeInBytes);
    }
    return ptr;
}


void SysFree(void* _Bytes)
{
    if (_Bytes)
    {
#ifdef HK_COMPILER_MSVC
        _aligned_free(_Bytes);
#else
        free(_Bytes);
#endif
    }
}

} // namespace Platform

namespace
{

#ifdef HK_DEBUG
static constexpr bool HeapDebug()
{
    return true;
}
#else
static bool HeapDebug()
{
    static bool bHeapDebug = Platform::HasArg("-bHeapDebug");
    return bHeapDebug;
}
#endif

#ifdef HK_DEBUG
static constexpr bool MemoryTrashTest()
{
#    ifdef MEMORY_TRASH_TEST
    return true;
#    else
    return false;
#    endif
}
#else
static bool MemoryTrashTest()
{
    static bool bMemoryTrashTest = Platform::HasArg("-bMemoryTrashTest");
    return bMemoryTrashTest;
}
#endif

} // namespace

void* AHeapMemory::Alloc(size_t _BytesCount, int _Alignment)
{
    HK_ASSERT(_Alignment <= 128 && IsPowerOfTwo(_Alignment));

    if (_Alignment < 16)
    {
        _Alignment = 16;
    }

    if (_BytesCount == 0)
    {
        // invalid bytes count
        CriticalError("AHeapMemory::Alloc: Invalid bytes count\n");
    }

    if (!HeapDebug())
    {
        return Platform::SysAlloc(Align(_BytesCount, _Alignment), _Alignment);
    }

    size_t chunkSizeInBytes = _BytesCount + sizeof(SHeapChunk);

    if (MemoryTrashTest())
    {
        chunkSizeInBytes += sizeof(TRASH_MARKER);
    }

    byte* bytes;
    byte* aligned;

    if (_Alignment == 16)
    {
        chunkSizeInBytes = Align(chunkSizeInBytes, 16);
        bytes            = (byte*)Platform::SysAlloc(chunkSizeInBytes, 16);
        aligned          = bytes + sizeof(SHeapChunk);
    }
    else
    {
        chunkSizeInBytes += _Alignment - 1;
        chunkSizeInBytes = Align(chunkSizeInBytes, sizeof(void*));
        bytes            = (byte*)Platform::SysAlloc(chunkSizeInBytes, sizeof(void*));
        aligned          = (byte*)AlignPtr(bytes + sizeof(SHeapChunk), _Alignment);
    }

    HK_ASSERT(IsAlignedPtr(aligned, 16));

    SHeapChunk* heap  = (SHeapChunk*)(aligned)-1;
    heap->Size        = chunkSizeInBytes;
    heap->DataSize    = _BytesCount;
    heap->Alignment   = _Alignment;
    heap->AlignOffset = aligned - bytes;

    {
        ASpinLockGuard lockGuard(Mutex);
        heap->pNext            = HeapChain.pNext;
        heap->pPrev            = &HeapChain;
        HeapChain.pNext->pPrev = heap;
        HeapChain.pNext        = heap;
    }

    if (MemoryTrashTest())
    {
        *(TRASH_MARKER*)(aligned + heap->DataSize) = TrashMarker;
    }

    IncMemoryStatistics(heap->Size, heap->Size - heap->DataSize);

    return aligned;
}

void AHeapMemory::Free(void* _Bytes)
{
    if (!HeapDebug())
    {
        return Platform::SysFree(_Bytes);
    }

    if (_Bytes)
    {
        byte* bytes = (byte*)_Bytes;

        SHeapChunk* heap = (SHeapChunk*)(bytes)-1;

        if (MemoryTrashTest())
        {
            if (*(TRASH_MARKER*)(bytes + heap->DataSize) != TrashMarker)
            {
                CriticalError("AHeapMemory::HeapFree: Warning: memory was trashed\n");
                // error()
            }
        }

        bytes -= heap->AlignOffset;

        {
            ASpinLockGuard lockGuard(Mutex);
            heap->pPrev->pNext = heap->pNext;
            heap->pNext->pPrev = heap->pPrev;
        }

        DecMemoryStatistics(heap->Size, heap->Size - heap->DataSize);

        Platform::SysFree(bytes);
    }
}

void* AHeapMemory::Realloc(void* _Data, int _NewBytesCount, int _NewAlignment, bool _KeepOld)
{
    HK_ASSERT(_NewAlignment <= 128 && IsPowerOfTwo(_NewAlignment));

    if (_NewAlignment < 16)
    {
        _NewAlignment = 16;
    }

    if (_NewBytesCount == 0)
    {
        // invalid bytes count
        CriticalError("AHeapMemory::Realloc: Invalid bytes count\n");
    }

    if (!HeapDebug())
    {
        return Platform::SysRealloc(_Data, Align(_NewBytesCount, _NewAlignment), _NewAlignment);
    }

    if (!_Data)
    {
        return Alloc(_NewBytesCount, _NewAlignment);
    }

    byte* bytes = (byte*)_Data;

    SHeapChunk* heap = (SHeapChunk*)(bytes)-1;

    int alignment = heap->Alignment;

    if (MemoryTrashTest())
    {
        if (*(TRASH_MARKER*)(bytes + heap->DataSize) != TrashMarker)
        {
            CriticalError("AHeapMemory::Realloc: Warning: memory was trashed\n");
        }
    }

    if (heap->DataSize >= _NewBytesCount && alignment == _NewAlignment)
    {
        // big enough
        return _Data;
    }
    // reallocate
    if (_KeepOld)
    {
        bytes = (byte*)Alloc(_NewBytesCount, _NewAlignment);
        Platform::Memcpy(bytes, _Data, heap->DataSize);
        Free(_Data);
        return bytes;
    }
    Free(_Data);
    return Alloc(_NewBytesCount, _NewAlignment);
}

void AHeapMemory::PointerTrashTest(void* _Bytes)
{
    if (!HeapDebug())
    {
        return;
    }

    if (MemoryTrashTest())
    {
        byte* bytes = (byte*)_Bytes;

        SHeapChunk* heap = (SHeapChunk*)(bytes)-1;

        if (*(TRASH_MARKER*)(bytes + heap->DataSize) != TrashMarker)
        {
            CriticalError("AHeapMemory::PointerTrashTest: Warning: memory was trashed\n");
        }
    }
}

void AHeapMemory::CheckMemoryLeaks()
{
    ASpinLockGuard lockGuard(Mutex);
    for (SHeapChunk* heap = HeapChain.pNext; heap != &HeapChain; heap = heap->pNext)
    {
        MemLogger.Print("==== Heap Memory Leak ====\n");
        MemLogger.Printf("Heap Address: %u Size: %d\n", (size_t)(heap + 1), heap->DataSize);
    }
}

size_t AHeapMemory::GetTotalMemoryUsage()
{
    ASpinLockGuard lockGuard(StatisticLock);
    return TotalMemoryUsage;
}

size_t AHeapMemory::GetTotalMemoryOverhead()
{
    ASpinLockGuard lockGuard(StatisticLock);
    return TotalMemoryOverhead;
}

size_t AHeapMemory::GetMaxMemoryUsage()
{
    ASpinLockGuard lockGuard(StatisticLock);
    return MaxMemoryUsage;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Hunk Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

static const int MinHunkFragmentLength = 64; // Must be > sizeof( SHunk )

struct SHunk
{
    int32_t Size;
    int32_t Mark;
    SHunk*  pPrev;
};

struct SHunkMemory
{
    size_t  Size;
    SHunk*  Hunk; // pointer to next hunk
    SHunk*  Cur;  // pointer to current hunk
    int32_t Mark;
    int32_t Pad;
};

void*  AHunkMemory::GetHunkMemoryAddress() const { return MemoryBuffer; }
int    AHunkMemory::GetHunkMemorySizeInMegabytes() const { return MemoryBuffer ? MemoryBuffer->Size >> 10 >> 10 : 0; }
size_t AHunkMemory::GetTotalMemoryUsage() const { return TotalMemoryUsage; }
size_t AHunkMemory::GetTotalMemoryOverhead() const { return TotalMemoryOverhead; }
size_t AHunkMemory::GetTotalFreeMemory() const { return MemoryBuffer ? MemoryBuffer->Size - TotalMemoryUsage : 0; }
size_t AHunkMemory::GetMaxMemoryUsage() const { return MaxMemoryUsage; }

void AHunkMemory::Initialize(void* _MemoryAddress, int _SizeInMegabytes)
{
    size_t sizeInBytes = _SizeInMegabytes << 20;

    MemoryBuffer              = (SHunkMemory*)_MemoryAddress;
    MemoryBuffer->Size        = sizeInBytes;
    MemoryBuffer->Mark        = 0;
    MemoryBuffer->Hunk        = (SHunk*)(MemoryBuffer + 1);
    MemoryBuffer->Hunk->Size  = MemoryBuffer->Size - sizeof(SHunkMemory);
    MemoryBuffer->Hunk->Mark  = -1; // marked free
    MemoryBuffer->Hunk->pPrev = MemoryBuffer->Cur = nullptr;

    TotalMemoryUsage    = 0;
    MaxMemoryUsage      = 0;
    TotalMemoryOverhead = 0;

    HK_VALIDATE_TYPE_SIZE(SHunkMemory, 32);
    HK_VALIDATE_TYPE_SIZE(SHunk, 16);

    if (!IsAlignedPtr(MemoryBuffer->Hunk, 16))
    {
        CriticalError("AHunkMemory::Initialize: chunk must be at 16 byte boundary\n");
    }
}

void AHunkMemory::Deinitialize()
{
    CheckMemoryLeaks();

    MemoryBuffer = nullptr;

    TotalMemoryUsage    = 0;
    MaxMemoryUsage      = 0;
    TotalMemoryOverhead = 0;
}

void AHunkMemory::Clear()
{
    MemoryBuffer->Mark        = 0;
    MemoryBuffer->Hunk        = (SHunk*)(MemoryBuffer + 1);
    MemoryBuffer->Hunk->Size  = MemoryBuffer->Size - sizeof(SHunkMemory);
    MemoryBuffer->Hunk->Mark  = -1; // marked free
    MemoryBuffer->Hunk->pPrev = MemoryBuffer->Cur = nullptr;

    TotalMemoryUsage    = 0;
    MaxMemoryUsage      = 0;
    TotalMemoryOverhead = 0;
}

int AHunkMemory::SetHunkMark()
{
    return ++MemoryBuffer->Mark;
}

HK_FORCEINLINE void SetTrashMarker(SHunk* _Hunk)
{
    if (MemoryTrashTest())
    {
        *(TRASH_MARKER*)((byte*)(_Hunk) + _Hunk->Size - sizeof(TRASH_MARKER)) = TrashMarker;
    }
}

HK_FORCEINLINE bool HunkTrashTest(const SHunk* _Hunk)
{
    if (MemoryTrashTest())
    {
        return *(const TRASH_MARKER*)((const byte*)(_Hunk) + _Hunk->Size - sizeof(TRASH_MARKER)) != TrashMarker;
    }
    else
    {
        return false;
    }
}

void* AHunkMemory::Alloc(size_t _BytesCount)
{
    if (!MemoryBuffer)
    {
        CriticalError("AHunkMemory::Alloc: Not initialized\n");
    }

    if (_BytesCount == 0)
    {
        // invalid bytes count
        CriticalError("AHunkMemory::Alloc: Invalid bytes count\n");
    }

    // check memory trash
    if (MemoryBuffer->Cur && HunkTrashTest(MemoryBuffer->Cur))
    {
        CriticalError("AHunkMemory::Alloc: Memory was trashed\n");
    }

    SHunk* hunk = MemoryBuffer->Hunk;

    if (hunk->Mark != -1)
    {
        // not enough memory
        CriticalError("AHunkMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount);
    }

    int requiredSize = _BytesCount;

    // Add chunk header
    requiredSize += sizeof(SHunk);

    // Add trash marker
    if (MemoryTrashTest())
    {
        requiredSize += sizeof(TRASH_MARKER);
    }

    // Align chunk to 16-byte boundary
    requiredSize = Align(requiredSize, 16);

    if (requiredSize > hunk->Size)
    {
        // not enough memory
        CriticalError("AHunkMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount);
    }

    hunk->Mark = MemoryBuffer->Mark;

    int hunkNewSize = hunk->Size - requiredSize;
    if (hunkNewSize >= MinHunkFragmentLength)
    {
        SHunk* newHunk     = (SHunk*)((byte*)(hunk) + requiredSize);
        newHunk->Size      = hunkNewSize;
        newHunk->Mark      = -1;
        newHunk->pPrev     = hunk;
        MemoryBuffer->Hunk = newHunk;
        hunk->Size         = requiredSize;
    }

    MemoryBuffer->Cur = hunk;

    SetTrashMarker(hunk);

    IncMemoryStatistics(hunk->Size, sizeof(SHunk));

    void* aligned = hunk + 1;

    HK_ASSERT(IsAlignedPtr(aligned, 16));

    return aligned;
}

void AHunkMemory::ClearToMark(int _Mark)
{
    if (MemoryBuffer->Mark < _Mark)
    {
        return;
    }

    if (_Mark <= 0)
    {
        // Simple case
        Clear();
        return;
    }

    // check memory trash
    if (MemoryBuffer->Cur && HunkTrashTest(MemoryBuffer->Cur))
    {
        CriticalError("AHunkMemory::ClearToMark: Memory was trashed\n");
    }

    int grow = 0;

    SHunk* hunk = MemoryBuffer->Hunk;
    if (hunk->Mark == -1)
    {
        // get last allocated hunk
        grow              = hunk->Size;
        MemoryBuffer->Cur = hunk = hunk->pPrev;
    }

    while (hunk && hunk->Mark >= _Mark)
    {
        DecMemoryStatistics(hunk->Size, sizeof(SHunk));
        if (HunkTrashTest(hunk))
        {
            CriticalError("AHunkMemory::ClearToMark: Warning: memory was trashed\n");
        }
        hunk->Size += grow;
        hunk->Mark         = -1; // mark free
        MemoryBuffer->Hunk = hunk;
        grow               = hunk->Size;
        MemoryBuffer->Cur = hunk = hunk->pPrev;
    }

    MemoryBuffer->Mark = _Mark;
}

void AHunkMemory::ClearLastHunk()
{
    int grow = 0;

    SHunk* hunk = MemoryBuffer->Hunk;
    if (hunk->Mark == -1)
    {
        // get last allocated hunk
        grow              = hunk->Size;
        MemoryBuffer->Cur = hunk = hunk->pPrev;
    }

    if (hunk)
    {
        DecMemoryStatistics(hunk->Size, sizeof(SHunk));
        if (HunkTrashTest(hunk))
        {
            CriticalError("AHunkMemory::ClearLastHunk: Warning: memory was trashed\n");
            // error()
        }
        hunk->Size += grow;
        hunk->Mark         = -1; // mark free
        MemoryBuffer->Hunk = hunk;
        MemoryBuffer->Cur  = hunk->pPrev;
    }
}

void AHunkMemory::CheckMemoryLeaks()
{
    if (TotalMemoryUsage > 0)
    {
        // check memory trash
        if (MemoryBuffer->Cur && HunkTrashTest(MemoryBuffer->Cur))
        {
            MemLogger.Print("AHunkMemory::CheckMemoryLeaks: Memory was trashed\n");
        }

        SHunk* hunk = MemoryBuffer->Hunk;
        if (hunk->Mark == -1)
        {
            // get last allocated hunk
            hunk = hunk->pPrev;
        }
        while (hunk)
        {
            MemLogger.Print("==== Hunk Memory Leak ====\n");
            MemLogger.Printf("Hunk Address: %u Size: %d\n", (size_t)(hunk + 1), hunk->Size);
            hunk = hunk->pPrev;
        }
    }
}

void AHunkMemory::IncMemoryStatistics(size_t _MemoryUsage, size_t _Overhead)
{
    TotalMemoryUsage += _MemoryUsage;
    TotalMemoryOverhead += _Overhead;
    MaxMemoryUsage = std::max(MaxMemoryUsage, TotalMemoryUsage);
}

void AHunkMemory::DecMemoryStatistics(size_t _MemoryUsage, size_t _Overhead)
{
    TotalMemoryUsage -= _MemoryUsage;
    TotalMemoryOverhead -= _Overhead;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Zone Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

struct SZoneChunk
{
    SZoneChunk* pNext; // NOTE: We can remove next, becouse next = (SZoneChunk*)(((byte*)chunk) + chunk->size)
    SZoneChunk* pPrev;
    int32_t     Size;
    int32_t     DataSize;
    byte        Pad[8];
};

struct SZoneBuffer
{
    SZoneChunk* Rover;
    SZoneChunk  ChunkList;
    int32_t     Size;
    byte        Pad[16];
};

HK_VALIDATE_TYPE_SIZE(SZoneChunk, 32);
HK_VALIDATE_TYPE_SIZE(SZoneBuffer, 64);

static const int ChunkHeaderLength     = sizeof(SZoneChunk);
static const int MinZoneFragmentLength = 64; // Must be > ChunkHeaderLength

HK_FORCEINLINE size_t AdjustChunkSize(size_t _BytesCount)
{

    // Add chunk header
    _BytesCount += ChunkHeaderLength;

    // Add trash marker
    if (MemoryTrashTest())
    {
        _BytesCount += sizeof(TRASH_MARKER);
    }

    // Align chunk to 16-byte boundary
    _BytesCount = Align(_BytesCount, 16);

    return _BytesCount;
}

HK_FORCEINLINE void SetTrashMarker(SZoneChunk* _Chunk)
{
    if (MemoryTrashTest())
    {
        *(TRASH_MARKER*)((byte*)(_Chunk) + (-_Chunk->Size) - sizeof(TRASH_MARKER)) = TrashMarker;
    }
}

HK_FORCEINLINE bool ChunkTrashTest(const SZoneChunk* _Chunk)
{
    if (MemoryTrashTest())
    {
        return *(const TRASH_MARKER*)((const byte*)(_Chunk) + (-_Chunk->Size) - sizeof(TRASH_MARKER)) != TrashMarker;
    }
    else
    {
        return false;
    }
}

void* AZoneMemory::GetZoneMemoryAddress() const
{
    return MemoryBuffer;
}

int AZoneMemory::GetZoneMemorySizeInMegabytes() const
{
    return MemoryBuffer ? MemoryBuffer->Size >> 10 >> 10 : 0;
}

size_t AZoneMemory::GetTotalMemoryUsage() const
{
    ZONE_SYNC_GUARD_STAT
    return TotalMemoryUsage;
}

size_t AZoneMemory::GetTotalMemoryOverhead() const
{
    ZONE_SYNC_GUARD_STAT
    return TotalMemoryOverhead;
}

size_t AZoneMemory::GetTotalFreeMemory() const
{
    ZONE_SYNC_GUARD_STAT
    return MemoryBuffer ? MemoryBuffer->Size - TotalMemoryUsage : 0;
}

size_t AZoneMemory::GetMaxMemoryUsage() const
{
    ZONE_SYNC_GUARD_STAT
    return MaxMemoryUsage;
}

void AZoneMemory::Initialize(void* _MemoryAddress, int _SizeInMegabytes)
{
    size_t sizeInBytes = _SizeInMegabytes << 20;

    MemoryBuffer                  = (SZoneBuffer*)_MemoryAddress;
    MemoryBuffer->Size            = sizeInBytes;
    MemoryBuffer->ChunkList.pPrev = MemoryBuffer->ChunkList.pNext = MemoryBuffer->Rover = (SZoneChunk*)(MemoryBuffer + 1);
    MemoryBuffer->ChunkList.Size                                                        = 0;
    MemoryBuffer->Rover->Size                                                           = MemoryBuffer->Size - sizeof(SZoneBuffer);
    MemoryBuffer->Rover->pNext                                                          = &MemoryBuffer->ChunkList;
    MemoryBuffer->Rover->pPrev                                                          = &MemoryBuffer->ChunkList;

    if (!IsAlignedPtr(MemoryBuffer->Rover, 16))
    {
        CriticalError("AZoneMemory::Initialize: chunk must be at 16 byte boundary\n");
    }

    TotalMemoryUsage    = 0;
    TotalMemoryOverhead = 0;
    MaxMemoryUsage      = 0;
}

void AZoneMemory::Deinitialize()
{
    CheckMemoryLeaks();

    MemoryBuffer = nullptr;

    TotalMemoryUsage    = 0;
    TotalMemoryOverhead = 0;
    MaxMemoryUsage      = 0;
}

void AZoneMemory::Clear()
{
    if (!MemoryBuffer)
    {
        return;
    }

    ZONE_SYNC_GUARD

    MemoryBuffer->ChunkList.pPrev = MemoryBuffer->ChunkList.pNext = MemoryBuffer->Rover = (SZoneChunk*)(MemoryBuffer + 1);
    MemoryBuffer->ChunkList.Size                                                        = 0;
    MemoryBuffer->Rover->Size                                                           = MemoryBuffer->Size - sizeof(SZoneBuffer);
    MemoryBuffer->Rover->pNext                                                          = &MemoryBuffer->ChunkList;
    MemoryBuffer->Rover->pPrev                                                          = &MemoryBuffer->ChunkList;

    {
        ZONE_SYNC_GUARD_STAT
        TotalMemoryUsage    = 0;
        TotalMemoryOverhead = 0;
        MaxMemoryUsage      = 0;
    }

    // Allocated "on heap" memory is still present
}

void AZoneMemory::IncMemoryStatistics(size_t _MemoryUsage, size_t _Overhead)
{
    ZONE_SYNC_GUARD_STAT
    TotalMemoryUsage += _MemoryUsage;
    TotalMemoryOverhead += _Overhead;
    MaxMemoryUsage = std::max(MaxMemoryUsage, TotalMemoryUsage);
}

void AZoneMemory::DecMemoryStatistics(size_t _MemoryUsage, size_t _Overhead)
{
    ZONE_SYNC_GUARD_STAT
    TotalMemoryUsage -= _MemoryUsage;
    TotalMemoryOverhead -= _Overhead;
}

SZoneChunk* AZoneMemory::FindFreeChunk(int _RequiredSize)
{
    SZoneChunk* rover = MemoryBuffer->Rover;
    SZoneChunk* start = rover->pPrev;
    SZoneChunk* cur;

    do {
        if (rover == start)
        {
            return nullptr;
        }
        cur   = rover;
        rover = rover->pNext;
    } while (cur->Size < _RequiredSize);

    return cur;
}

void* AZoneMemory::Alloc(size_t _BytesCount)
{
#if 0
    return SysAlloc( Align(_BytesCount,16), 16 );
#else
    if (!MemoryBuffer)
    {
        CriticalError("AZoneMemory::Alloc: Not initialized\n");
    }

    if (_BytesCount == 0)
    {
        // invalid bytes count
        CriticalError("AZoneMemory::Alloc: Invalid bytes count\n");
    }

    ZONE_SYNC_GUARD

    size_t requiredSize = AdjustChunkSize(_BytesCount);

    SZoneChunk* cur = FindFreeChunk(requiredSize);
    if (!cur)
    {
        // no free chunks
        CriticalError("AZoneMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount);
    }

    int recidualChunkSpace = cur->Size - requiredSize;
    if (recidualChunkSpace >= MinZoneFragmentLength)
    { // don't allow to create very small chunks
        SZoneChunk* newChunk = (SZoneChunk*)((byte*)(cur) + requiredSize);
        HK_ASSERT(IsAlignedPtr(newChunk, 16));
        newChunk->Size = recidualChunkSpace;
        newChunk->pPrev = cur;
        newChunk->pNext = cur->pNext;
        newChunk->pNext->pPrev = newChunk;
        cur->pNext = newChunk;
        cur->Size = requiredSize;
    }

    byte* pointer = (byte*)(cur + 1);

    HK_ASSERT(IsAlignedPtr(cur, 16));
    HK_ASSERT(IsAlignedPtr(pointer, 16));

    IncMemoryStatistics(cur->Size, cur->Size - _BytesCount);

    cur->Size = -cur->Size; // Set size to negative to mark chunk used
    cur->DataSize = _BytesCount;
    MemoryBuffer->Rover = cur->pNext;

#    if defined ZONE_MEMORY_LEAK_ADDRESS && defined HK_DEBUG
    size_t localAddr = (size_t)(cur + 1) - (size_t)GetZoneMemoryAddress();
    size_t size = (-cur->Size);
    if (localAddr == ZONE_MEMORY_LEAK_ADDRESS && size == ZONE_MEMORY_LEAK_SIZE)
    {
        GLogger.Printf("Problem alloc\n");
#        ifdef HK_OS_WIN32
        DebugBreak();
#        else
        //__asm__( "int $3" );
        raise(SIGTRAP);
#        endif
    }
#    endif

    SetTrashMarker(cur);

    return pointer;
#endif
}
//#pragma warning(disable:4702)
void* AZoneMemory::Realloc(void* _Data, int _NewBytesCount, bool _KeepOld)
{
#if 1

#    if 0
    if ( _Data ) {
        void * d = SysAlloc( _NewBytesCount, 16 );
        Platform::Memcpy(d,_Data,_NewBytesCount);
        SysFree(_Data);
        return d;
    }
    return nullptr;
#    else
    if (!_Data)
    {
        return Alloc(_NewBytesCount);
    }

    SZoneChunk* chunk = (SZoneChunk*)(_Data)-1;

    if (chunk->DataSize >= _NewBytesCount)
    {
        // data is big enough
        //MemLogger.Printf( "_BytesCount >= _NewBytesCount\n" );
        return _Data;
    }

    // Check freed
    bool bFreed = chunk->Size > 0;

    if (bFreed)
    {
        //MemLogger.Printf( "freed pointer\n" );
        return Alloc(_NewBytesCount);
    }

    if (!_KeepOld)
    {
        Free(_Data);
        return Alloc(_NewBytesCount);
    }

    int   sz   = chunk->DataSize;
    void* temp = GHunkMemory.Alloc(sz);
    Platform::Memcpy(temp, _Data, sz);
    Free(_Data);
    void* pNewData = Alloc(_NewBytesCount);
    if (pNewData != _Data)
    {
        Platform::Memcpy(pNewData, temp, sz);
    }
    else
    {
        //MemLogger.Printf( "Caching memcpy\n" );
    }
    GHunkMemory.ClearLastHunk();

    return pNewData;
#    endif

#else

#    if 0
    return SysRealloc( _Data, _NewBytesCount, 16 );
#    else
    if (!_Data)
    {
        return Alloc(_NewBytesCount);
    }

    SZoneChunk* chunk = (SZoneChunk*)(_Data)-1;

    if (chunk->DataSize >= _NewBytesCount)
    {
        // data is big enough
        //MemLogger.Printf( "_BytesCount >= _NewBytesCount\n" );
        return _Data;
    }

    // Check freed
    bool bFreed = chunk->Size > 0;

    if (bFreed)
    {
        //MemLogger.Printf( "freed pointer\n" );
        return Alloc(_NewBytesCount);
    }

    ZONE_SYNC_GUARD

    size_t oldSize = chunk->DataSize;

    if (ChunkTrashTest(chunk))
    {
        CriticalError("AZoneMemory::Realloc: Warning: memory was trashed\n");
        // error()
    }

    int requiredSize = AdjustChunkSize(_NewBytesCount);
    if (requiredSize <= -chunk->Size)
    {
        // data is big enough
        //MemLogger.Printf( "data is big enough\n" );
        return _Data;
    }

    // restore positive chunk size
    chunk->Size = -chunk->Size;

    HK_ASSERT(chunk->Size > 0);

    // try to use next chunk
    SZoneChunk* cur = chunk->pNext;
    if (cur->Size > 0 && cur->Size + chunk->Size >= requiredSize)
    {

#        if 1
        requiredSize -= chunk->Size;
        int recidualChunkSpace = cur->Size - requiredSize;
        if (recidualChunkSpace >= MinZoneFragmentLength)
        {
#            if 0
            SZoneChunk * newChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
            HK_ASSERT( IsAlignedPtr( newChunk, 16 ) );
            newChunk->Size = recidualChunkSpace;
            newChunk->pPrev = chunk;
            newChunk->pNext = cur->pNext;
            newChunk->pNext->pPrev = newChunk;
            chunk->pNext = newChunk;
            chunk->Size += requiredSize;
            IncMemoryStatistics( requiredSize, requiredSize - _NewBytesCount );
#            else
            chunk->Size = -chunk->Size;
            void* d     = malloc(_NewBytesCount);
            Platform::Memcpy(d, _Data, oldSize);
            Free(_Data);
            _Data = Alloc(_NewBytesCount);
            Platform::Memcpy(_Data, d, _NewBytesCount);
            free(d);
            return _Data;
#            endif
        }
        else
        {
#            if 1
            chunk->pNext        = cur->pNext;
            chunk->pNext->pPrev = chunk;
            chunk->Size += cur->Size;
            IncMemoryStatistics(cur->Size, cur->Size - _NewBytesCount);

            //Platform::Memset( (byte *)_Data + chunk->DataSize, 0, _NewBytesCount - chunk->DataSize );
#            else
            //            chunk->Size = -chunk->Size;
            //            void * d = malloc( oldSize );
            //            Platform::Memcpy( d, _Data, oldSize );
            //            Free( _Data );
            //            _Data = Alloc( _NewBytesCount );
            //            Platform::Memcpy( _Data, d, oldSize );
            //            free( d );
            //            return _Data;
            chunk->Size = -chunk->Size;
            void* d     = malloc(_NewBytesCount);
            Platform::Memcpy(d, _Data, oldSize);
            Free(_Data);
            _Data = Alloc(_NewBytesCount);
            Platform::Memcpy(_Data, d, oldSize);
            free(d);
            return _Data;
#            endif
        }

        // Set size to negative to mark chunk used
        chunk->Size         = -chunk->Size;
        chunk->DataSize     = _NewBytesCount;
        MemoryBuffer->Rover = chunk->pNext;

        SetTrashMarker(chunk);

        //MemLogger.Printf( "using next chunk\n" );

        HK_ASSERT(_Data == chunk + 1);

        return _Data;
#        else
        chunk->Size = -chunk->Size;
        void* d     = malloc(_NewBytesCount);
        Platform::Memcpy(d, _Data, oldSize);
        Free(_Data);
        _Data = Alloc(_NewBytesCount);
        Platform::Memcpy(_Data, d, _NewBytesCount);
        free(d);
        return _Data;
#        endif
    }
    else
    {

        //chunk->Size = -chunk->Size;
        //void * d = malloc( _NewBytesCount );
        //Platform::Memcpy(d,_Data, oldSize );
        //Free(_Data);
        //_Data=Alloc(_NewBytesCount);
        //Platform::Memcpy(_Data,d,_NewBytesCount);
        //free(d);
        //return _Data;
#        if 1
        //MemLogger.Printf( "reallocating new chunk with copy\n" );

        cur = FindFreeChunk(requiredSize);
        if (!cur)
        {

            chunk->Size = -chunk->Size;

            // no free chunks

            CriticalError("AZoneMemory::Realloc: Failed on allocation of %u bytes\n", _NewBytesCount);
        }

        DecMemoryStatistics(chunk->Size, chunk->Size - chunk->DataSize);

        int recidualChunkSpace = cur->Size - requiredSize;
        if (recidualChunkSpace >= MinZoneFragmentLength)
        {
            SZoneChunk* newChunk = (SZoneChunk*)((byte*)(cur) + requiredSize);
            HK_ASSERT(IsAlignedPtr(newChunk, 16));
            newChunk->Size         = recidualChunkSpace;
            newChunk->pPrev        = cur;
            newChunk->pNext        = cur->pNext;
            newChunk->pNext->pPrev = newChunk;
            cur->pNext             = newChunk;
            cur->Size              = requiredSize;
        }

        byte* pointer = (byte*)(cur + 1);

        HK_ASSERT(IsAlignedPtr(cur, 16));
        HK_ASSERT(IsAlignedPtr(pointer, 16));

        IncMemoryStatistics(cur->Size, cur->Size - _NewBytesCount);

        cur->Size           = -cur->Size; // Set size to negative to mark chunk used
        cur->DataSize       = _NewBytesCount;
        MemoryBuffer->Rover = cur->pNext;

        SetTrashMarker(cur);

        if (_KeepOld)
        {
            Platform::Memcpy(pointer, _Data, oldSize);
        }

        // Deallocate old chunk

        SZoneChunk* prevChunk = chunk->pPrev;
        SZoneChunk* nextChunk = chunk->pNext;

        if (prevChunk->Size > 0)
        {
            // Merge prev and current chunks to one free chunk

            prevChunk->Size += chunk->Size;
            prevChunk->pNext        = chunk->pNext;
            prevChunk->pNext->pPrev = prevChunk;

            if (chunk == MemoryBuffer->Rover)
            {
                MemoryBuffer->Rover = prevChunk;
            }
            chunk = prevChunk;
        }

        if (nextChunk->Size > 0)
        {
            // Merge current and next chunks to one free chunk

            chunk->Size += nextChunk->Size;
            chunk->pNext        = nextChunk->pNext;
            chunk->pNext->pPrev = chunk;

            if (nextChunk == MemoryBuffer->Rover)
            {
                MemoryBuffer->Rover = chunk;
            }
        }

        return pointer;
#        endif
    }
#    endif

#endif
}

void AZoneMemory::Free(void* _Bytes)
{
#if 0
    SysFree( _Bytes );
#else
    if (!MemoryBuffer)
    {
        return;
    }

    if (!_Bytes)
    {
        return;
    }

    ZONE_SYNC_GUARD

    SZoneChunk* chunk = (SZoneChunk*)(_Bytes)-1;

    if (chunk->Size > 0)
    {
        // freed pointer
        return;
    }

    if (ChunkTrashTest(chunk))
    {
        CriticalError("AZoneMemory::Free: Warning: memory was trashed\n");
        // error()
    }

    chunk->Size = -chunk->Size;

    DecMemoryStatistics(chunk->Size, chunk->Size - chunk->DataSize);

    SZoneChunk* prevChunk = chunk->pPrev;
    SZoneChunk* nextChunk = chunk->pNext;

    if (prevChunk->Size > 0)
    {
        // Merge prev and current chunks to one free chunk

        prevChunk->Size += chunk->Size;
        prevChunk->pNext = chunk->pNext;
        prevChunk->pNext->pPrev = prevChunk;

        if (chunk == MemoryBuffer->Rover)
        {
            MemoryBuffer->Rover = prevChunk;
        }
        chunk = prevChunk;
    }

    if (nextChunk->Size > 0)
    {
        // Merge current and next chunks to one free chunk

        chunk->Size += nextChunk->Size;
        chunk->pNext = nextChunk->pNext;
        chunk->pNext->pPrev = chunk;

        if (nextChunk == MemoryBuffer->Rover)
        {
            MemoryBuffer->Rover = chunk;
        }
    }
#endif
}

void AZoneMemory::CheckMemoryLeaks()
{
    ZONE_SYNC_GUARD

    size_t totalMemoryUsage;
    {
        ZONE_SYNC_GUARD_STAT
        totalMemoryUsage = TotalMemoryUsage;
    }

    if (totalMemoryUsage > 0)
    {
        SZoneChunk* rover = MemoryBuffer->Rover;
        SZoneChunk* start = rover->pPrev;
        SZoneChunk* cur;

        do {
            cur = rover;
            if (cur->Size < 0)
            {
                MemLogger.Print("==== Zone Memory Leak ====\n");
                MemLogger.Printf("Chunk Address: %u (Local: %u) Size: %d\n", (size_t)(cur + 1), (size_t)(cur + 1) - (size_t)GetZoneMemoryAddress(), (-cur->Size));

                //int sz = -cur->Size;
                //char * b = (char*)(cur+1);
                //while ( sz-->0 ) {
                //    MemLogger.Printf("%c",*b > 31 && *b < 127 ? *b : '?');
                //    b++;
                //}
            }

            if (rover == start)
            {
                break;
            }
            rover = rover->pNext;

        } while (1);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Utilites
//
//////////////////////////////////////////////////////////////////////////////////////////

namespace Platform
{

void _MemcpySSE(byte* _Dst, const byte* _Src, size_t _SizeInBytes)
{
#if 0
    memcpy(_Dst,_Src,_SizeInBytes);
#else
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
#endif
}

void _ZeroMemSSE(byte* _Dst, size_t _SizeInBytes)
{
#if 0
    ZeroMem( _Dst, _SizeInBytes );
#else
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
#endif
}

void _MemsetSSE(byte* _Dst, int _Val, size_t _SizeInBytes)
{
    HK_ASSERT(IsSSEAligned((size_t)_Dst));

#if 0
    Memset( _Dst, _Val, _SizeInBytes );
#else

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

#endif
}

} // namespace Platform
