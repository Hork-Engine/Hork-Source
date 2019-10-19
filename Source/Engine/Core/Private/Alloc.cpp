/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Core/Public/Alloc.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/CriticalError.h>

#include <malloc.h>
#include <memory.h>

//////////////////////////////////////////////////////////////////////////////////////////
//
// Memory public
//
//////////////////////////////////////////////////////////////////////////////////////////

FHeapMemory GHeapMemory;
FHunkMemory GHunkMemory;
FZoneMemory GZoneMemory;

//////////////////////////////////////////////////////////////////////////////////////////
//
// Memory common functions/constants/defines
//
//////////////////////////////////////////////////////////////////////////////////////////

//#define CLEAR_ALLOCATED_MEMORY
#define ENABLE_TRASH_TEST
#define ENABLE_BLOCK_SIZE_ALIGN

typedef uint16_t TRASH_MARKER;

static const TRASH_MARKER TrashMarker = 0xfeee;

static FLogger MemLogger;

//////////////////////////////////////////////////////////////////////////////////////////
//
// Heap Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

static size_t HeapTotalMemoryUsage = 0;
static size_t HeapTotalMemoryOverhead = 0;
static size_t HeapMaxMemoryUsage = 0;

AN_FORCEINLINE void IncMemoryStatisticsOnHeap( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic increment for multithreading
    HeapTotalMemoryUsage += _MemoryUsage;
    HeapTotalMemoryOverhead += _Overhead;
    HeapMaxMemoryUsage = StdMax( HeapMaxMemoryUsage, HeapTotalMemoryUsage );
}

AN_FORCEINLINE void DecMemoryStatisticsOnHeap( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic decrement for multithreading
    HeapTotalMemoryUsage -= _MemoryUsage;
    HeapTotalMemoryOverhead -= _Overhead;
}

AN_FORCEINLINE void * UnalignedMalloc( size_t _Size ) {
    return malloc( _Size );
}

AN_FORCEINLINE void UnalignedFree( void * _Ptr ) {
    free( _Ptr );
}

FHeapMemory::FHeapMemory() {
    HeapChain.pNext = HeapChain.pPrev = &HeapChain;
}

FHeapMemory::~FHeapMemory() {
}

void FHeapMemory::Initialize() {

}

void FHeapMemory::Deinitialize() {
    CheckMemoryLeaks();
    Clear();
}

void FHeapMemory::Clear() {
    SHeapChunk * nextHeap;
    for ( SHeapChunk * heap = HeapChain.pNext ; heap != &HeapChain ; heap = nextHeap ) {
        nextHeap = heap->pNext;
        HeapFree( heap + 1 );
    }
}

AN_FORCEINLINE void * AlignPointer( void * _UnalignedPtr, int _Alignment ) {
#if 0
    struct SAligner {
        union {
            void * p;
            size_t i;
        };
    };
    SAligner aligner;
    const size_t bitMask = ~( _Alignment - 1 );
    aligner.p = _UnalignedPtr;
    aligner.i += _Alignment - 1;
    aligner.i &= bitMask;
    return aligner.p;
#endif
    return ( void * )( ( (size_t)_UnalignedPtr + _Alignment - 1 ) & ~( _Alignment - 1 ) );
}

void * FHeapMemory::HeapAlloc( size_t _BytesCount, int _Alignment ) {
    AN_Assert( _Alignment <= 128 && IsPowerOfTwoConstexpr( _Alignment ) );

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "FHeapMemory::HeapAlloc: Invalid bytes count\n" );
    }

#ifdef ENABLE_BLOCK_SIZE_ALIGN
    _BytesCount = ( _BytesCount + 7 ) & ~7;
#endif

    size_t realAllocatedBytes = _BytesCount + sizeof( SHeapChunk ) + _Alignment - 1;

#ifdef ENABLE_TRASH_TEST
    realAllocatedBytes += sizeof( TRASH_MARKER );
#endif

    byte * bytes;

    bytes = ( byte * )UnalignedMalloc( realAllocatedBytes );

    if ( bytes ) {
        byte * aligned = ( byte * )AlignPointer( bytes + sizeof( SHeapChunk ), _Alignment );

        //heap_t * heap = ( heap_t * )( bytes + padding );
        SHeapChunk * heap = ( SHeapChunk * )( aligned ) - 1;
        heap->Size = realAllocatedBytes;
        heap->Padding = aligned - bytes;

        Sync.BeginScope();
        heap->pNext = HeapChain.pNext;
        heap->pPrev = &HeapChain;
        HeapChain.pNext->pPrev = heap;
        HeapChain.pNext = heap;
        Sync.EndScope();

#ifdef CLEAR_ALLOCATED_MEMORY
        ClearMemory8( aligned, 0, _BytesCount );
#endif
#ifdef ENABLE_TRASH_TEST
        *( TRASH_MARKER * )( bytes + realAllocatedBytes - sizeof( TRASH_MARKER ) ) = TrashMarker;
#endif
        IncMemoryStatisticsOnHeap( realAllocatedBytes, heap->Padding );

        return aligned;
    }

    CriticalError( "FHeapMemory::HeapAlloc: Failed on allocation of %u bytes\n", _BytesCount );
    return nullptr;
}

void FHeapMemory::HeapFree( void * _Bytes ) {
    if ( _Bytes ) {
        byte * bytes = ( byte * )_Bytes;

        SHeapChunk * heap = ( SHeapChunk * )( bytes ) - 1;
        bytes -= heap->Padding;

#ifdef ENABLE_TRASH_TEST
        if ( *( TRASH_MARKER * )( bytes + heap->Size - sizeof( TRASH_MARKER ) ) != TrashMarker ) {
            MemLogger.Print( "FHeapMemory::HeapFree: Warning: memory was trashed\n" );
            // error()
        }
#endif

        Sync.BeginScope();
        heap->pPrev->pNext = heap->pNext;
        heap->pNext->pPrev = heap->pPrev;
        Sync.EndScope();

        DecMemoryStatisticsOnHeap( heap->Size, heap->Padding );

        UnalignedFree( bytes );
    }
}

void FHeapMemory::CheckMemoryLeaks() {
    for ( SHeapChunk * heap = HeapChain.pNext ; heap != &HeapChain ; heap = heap->pNext ) {
        MemLogger.Print( "==== Heap Memory Leak ====\n" );
        MemLogger.Printf( "Heap Address: %u Size: %d\n", (size_t)( heap + 1 ), heap->Size );
    }
}

size_t FHeapMemory::GetTotalMemoryUsage() {
    return HeapTotalMemoryUsage;
}

size_t FHeapMemory::GetTotalMemoryOverhead() {
    return HeapTotalMemoryOverhead;
}

size_t FHeapMemory::GetMaxMemoryUsage() {
    return HeapMaxMemoryUsage;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Hunk Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

static const int MinHunkFragmentLength = 64; // Must be > sizeof( hunk_t )

struct SHunk {
    int32_t Size;
    int32_t Mark;
    SHunk * pPrev;
};

struct SHunkMemory {
    size_t Size;
    SHunk * Hunk; // pointer to next hunk
    SHunk * Cur; // pointer to current hunk
    int32_t Mark;
};

void * FHunkMemory::GetHunkMemoryAddress() const { return MemoryBuffer; }
int FHunkMemory::GetHunkMemorySizeInMegabytes() const { return MemoryBuffer ? MemoryBuffer->Size >> 10 >> 10 : 0; }
size_t FHunkMemory::GetTotalMemoryUsage() const { return TotalMemoryUsage; }
size_t FHunkMemory::GetTotalMemoryOverhead() const { return TotalMemoryOverhead; }
size_t FHunkMemory::GetTotalFreeMemory() const { return MemoryBuffer ? MemoryBuffer->Size - TotalMemoryUsage : 0; }
size_t FHunkMemory::GetMaxMemoryUsage() const { return MaxMemoryUsage; }

void FHunkMemory::Initialize( void * _MemoryAddress, int _SizeInMegabytes ) {
    size_t sizeInBytes = _SizeInMegabytes << 20;

    MemoryBuffer = ( SHunkMemory * )_MemoryAddress;
    MemoryBuffer->Size = sizeInBytes;
    MemoryBuffer->Mark = 0;
    MemoryBuffer->Hunk = ( SHunk * )( MemoryBuffer + 1 );
    MemoryBuffer->Hunk->Size = MemoryBuffer->Size - sizeof( SHunkMemory );
    MemoryBuffer->Hunk->Mark = -1; // marked free
    MemoryBuffer->Hunk->pPrev = MemoryBuffer->Cur = nullptr;

    TotalMemoryUsage = 0;
    MaxMemoryUsage = 0;
    TotalMemoryOverhead = 0;
}

void FHunkMemory::Deinitialize() {
    CheckMemoryLeaks();

    MemoryBuffer = nullptr;

    TotalMemoryUsage = 0;
    MaxMemoryUsage = 0;
    TotalMemoryOverhead = 0;
}

void FHunkMemory::Clear() {
    MemoryBuffer->Mark = 0;
    MemoryBuffer->Hunk = ( SHunk * )( MemoryBuffer + 1 );
    MemoryBuffer->Hunk->Size = MemoryBuffer->Size - sizeof( SHunkMemory );
    MemoryBuffer->Hunk->Mark = -1; // marked free
    MemoryBuffer->Hunk->pPrev = MemoryBuffer->Cur = nullptr;

    TotalMemoryUsage = 0;
    MaxMemoryUsage = 0;
    TotalMemoryOverhead = 0;
}

int FHunkMemory::SetHunkMark() {
    return ++MemoryBuffer->Mark;
}

AN_FORCEINLINE size_t AdjustHunkSize( size_t _BytesCount, int _Alignment ) {
#ifdef ENABLE_BLOCK_SIZE_ALIGN
    _BytesCount = ( _BytesCount + 7 ) & ~7;
#endif
    size_t requiredSize = _BytesCount + sizeof( SHunk ) + _Alignment - 1;
#ifdef ENABLE_TRASH_TEST
    requiredSize += sizeof( TRASH_MARKER );
#endif
    return requiredSize;
}

AN_FORCEINLINE void SetTrashMarker( SHunk * _Hunk ) {
#ifdef ENABLE_TRASH_TEST
    *( TRASH_MARKER * )( ( byte * )( _Hunk ) + _Hunk->Size - sizeof( TRASH_MARKER ) ) = TrashMarker;
#endif
}

AN_FORCEINLINE bool HunkTrashTest( const SHunk * _Hunk ) {
#ifdef ENABLE_TRASH_TEST
    return *( const TRASH_MARKER * )( ( const byte * )( _Hunk ) + _Hunk->Size - sizeof( TRASH_MARKER ) ) != TrashMarker;
#else
    return false;
#endif
}

void * FHunkMemory::HunkMemory( size_t _BytesCount, int _Alignment ) {
    AN_Assert( _Alignment <= 128 && IsPowerOfTwoConstexpr( _Alignment ) );

    if ( !MemoryBuffer ) {
        CriticalError( "FHunkMemory::HunkMemory: Not initialized\n" );
    }

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "FHunkMemory::HunkMemory: Invalid bytes count\n" );
    }

    // check memory trash
    if ( MemoryBuffer->Cur && HunkTrashTest( MemoryBuffer->Cur ) ) {
        CriticalError( "FHunkMemory::HunkMemory: Memory was trashed\n" );
    }

    SHunk * hunk = MemoryBuffer->Hunk;

    if ( hunk->Mark != -1 ) {
        // not enough memory
        CriticalError( "FHunkMemory::HunkMemory: Failed on allocation of %u bytes\n", _BytesCount );
    }

    int requiredSize = AdjustHunkSize( _BytesCount, _Alignment );
    if ( requiredSize > hunk->Size ) {
        // not enough memory
        CriticalError( "FHunkMemory::HunkMemory: Failed on allocation of %u bytes\n", _BytesCount );
    }

    hunk->Mark = MemoryBuffer->Mark;

    int hunkNewSize = hunk->Size - requiredSize;
    if ( hunkNewSize >= MinHunkFragmentLength ) {
        SHunk * newHunk = ( SHunk * )( ( byte * )( hunk ) + requiredSize );
        newHunk->Size = hunkNewSize;
        newHunk->Mark = -1;
        newHunk->pPrev = hunk;
        MemoryBuffer->Hunk = newHunk;
        hunk->Size = requiredSize;
    }

    MemoryBuffer->Cur = hunk;

    SetTrashMarker( hunk );

    IncMemoryStatistics( hunk->Size, sizeof( SHunk ) );

    void * aligned = AlignPointer( hunk + 1, _Alignment );
#ifdef CLEAR_ALLOCATED_MEMORY
    ClearMemory8( aligned, 0, _BytesCount );
#endif

    return aligned;
}

void FHunkMemory::ClearToMark( int _Mark ) {
    if ( MemoryBuffer->Mark < _Mark ) {
        return;
    }

    if ( _Mark <= 0 ) {
        // Simple case
        Clear();
        return;
    }

    // check memory trash
    if ( MemoryBuffer->Cur && HunkTrashTest( MemoryBuffer->Cur ) ) {
        CriticalError( "FHunkMemory::ClearToMark: Memory was trashed\n" );
    }

    int grow = 0;

    SHunk * hunk = MemoryBuffer->Hunk;
    if ( hunk->Mark == -1 ) {
        // get last allocated hunk
        grow = hunk->Size;
        MemoryBuffer->Cur = hunk = hunk->pPrev;
    }

    while ( hunk && hunk->Mark >= _Mark ) {
        DecMemoryStatistics( hunk->Size, sizeof( SHunk ) );
        if ( HunkTrashTest( hunk ) ) {
            CriticalError( "FHunkMemory::ClearToMark: Warning: memory was trashed\n" );
        }
        hunk->Size += grow;
        hunk->Mark = -1; // mark free
        MemoryBuffer->Hunk = hunk;
        grow = hunk->Size;
        MemoryBuffer->Cur = hunk = hunk->pPrev;
    }

    MemoryBuffer->Mark = _Mark;
}

void FHunkMemory::ClearLastHunk() {
    int grow = 0;

    SHunk * hunk = MemoryBuffer->Hunk;
    if ( hunk->Mark == -1 ) {
        // get last allocated hunk
        grow = hunk->Size;
        MemoryBuffer->Cur = hunk = hunk->pPrev;
    }

    if ( hunk ) {
        DecMemoryStatistics( hunk->Size, sizeof( SHunk ) );
        if ( HunkTrashTest( hunk ) ) {
            MemLogger.Print( "FHunkMemory::ClearLastHunk: Warning: memory was trashed\n" );
            // error()
        }
        hunk->Size += grow;
        hunk->Mark = -1; // mark free
        MemoryBuffer->Hunk = hunk;
        MemoryBuffer->Cur = hunk->pPrev;
    }
}

void FHunkMemory::CheckMemoryLeaks() {
    if ( TotalMemoryUsage > 0 ) {
        // check memory trash
        if ( MemoryBuffer->Cur && HunkTrashTest( MemoryBuffer->Cur ) ) {
            MemLogger.Print( "FHunkMemory::CheckMemoryLeaks: Memory was trashed\n" );
        }

        SHunk * hunk = MemoryBuffer->Hunk;
        if ( hunk->Mark == -1 ) {
            // get last allocated hunk
            hunk = hunk->pPrev;
        }
        while ( hunk ) {
            MemLogger.Print( "==== Hunk Memory Leak ====\n" );
            MemLogger.Printf( "Hunk Address: %u Size: %d\n", (size_t)( hunk + 1 ), hunk->Size );
            hunk = hunk->pPrev;
        }
    }
}

void FHunkMemory::IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic increment for multithreading
    TotalMemoryUsage += _MemoryUsage;
    TotalMemoryOverhead += _Overhead;
    MaxMemoryUsage = StdMax( MaxMemoryUsage, TotalMemoryUsage );
}

void FHunkMemory::DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic decrement for multithreading
    TotalMemoryUsage -= _MemoryUsage;
    TotalMemoryOverhead -= _Overhead;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Zone Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

//#define FREE_LIST_BASED
//#define ALLOW_ALLOCATE_ON_HEAP
#define HEAP_ALIGN            4 // FIXME: change if need

#ifdef FREE_LIST_BASED
// TODO: Align chunk header structure if need
struct SZoneChunk {
    SZoneChunk * next;
    SZoneChunk * prev;
    SZoneChunk * used_next;
    SZoneChunk * used_prev;
    int32_t size;
};
struct memoryBuffer_t {
    SZoneChunk * freeChunk; // pointer to last freed chunk
    int32_t size;
};
#else
// TODO: Align chunk header structure if need
struct SZoneChunk {
    SZoneChunk * pNext; // FIXME: можно избавиться от next, так как next легко вычилслить как ((byte*)chunk) + chunk->size
    SZoneChunk * pPrev;
    int32_t Size;
};
// TODO: Align header if need
struct SZoneBuffer {
    SZoneChunk * Rover;
    SZoneChunk ChunkList;
    int32_t Size;
};
#endif

static const int ChunkHeaderLength = sizeof( SZoneChunk );
static const int MinZoneFragmentLength = 64; // Must be > ChunkHeaderLength

AN_FORCEINLINE size_t AdjustChunkSize( size_t _BytesCount, int _Alignment ) {
#ifdef ENABLE_BLOCK_SIZE_ALIGN
    _BytesCount = ( _BytesCount + 7 ) & ~7;
#endif
    size_t requiredSize = _BytesCount + ChunkHeaderLength + _Alignment - 1 + 1; // + 1 for one byte that keeps padding
#ifdef ENABLE_TRASH_TEST
    requiredSize += sizeof( TRASH_MARKER );
#endif
    return requiredSize;
}

AN_FORCEINLINE void SetTrashMarker( SZoneChunk * _Chunk ) {
#ifdef ENABLE_TRASH_TEST
    *( TRASH_MARKER * )( ( byte * )( _Chunk ) + ( -_Chunk->Size ) - sizeof( TRASH_MARKER ) ) = TrashMarker;
#endif
}

AN_FORCEINLINE bool ChunkTrashTest( const SZoneChunk * _Chunk ) {
#ifdef ENABLE_TRASH_TEST
    return *( const TRASH_MARKER * )( ( const byte * )( _Chunk ) + ( -_Chunk->Size ) - sizeof( TRASH_MARKER ) ) != TrashMarker;
#else
    return false;
#endif
}

static void * AllocateOnHeap( size_t _BytesCount, int _Alignment ) {
    size_t requiredSize;
    byte * bytes;
    byte * pointer;
    byte * aligned;
    SZoneChunk * heapChunk;
    int padding;

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "AllocateOnHeap: Invalid bytes count\n" );
    }

    requiredSize = AdjustChunkSize( _BytesCount, _Alignment );
    bytes = reinterpret_cast< byte * >( UnalignedMalloc( requiredSize ) );

    if ( bytes ) {
        pointer = bytes + sizeof( SZoneChunk ) + 1;
        aligned = ( byte * )AlignPointer( pointer, _Alignment );
        padding = aligned - pointer + 1;
        *(aligned - 1) = padding;

        heapChunk = (SZoneChunk *)bytes;

        heapChunk->pNext = (SZoneChunk *)(size_t)0xdeaddead;
        heapChunk->pPrev = (SZoneChunk *)(size_t)0xdeaddead;
#ifdef FREE_LIST_BASED
        heapChunk->used_next = (SZoneChunk *)0xdeaddead;
        heapChunk->used_prev = (SZoneChunk *)0xdeaddead;
#endif
        heapChunk->Size = -(int32_t)requiredSize;
        SetTrashMarker( heapChunk );

#ifdef CLEAR_ALLOCATED_MEMORY
        ClearMemory8( aligned, 0, _BytesCount );
#endif
        IncMemoryStatisticsOnHeap( requiredSize, ChunkHeaderLength + padding );

        return aligned;
    }

    CriticalError( "AllocateOnHeap: Failed on allocation of %u bytes\n", _BytesCount );
    return nullptr;
}

void * FZoneMemory::GetZoneMemoryAddress() const { return MemoryBuffer; }
int FZoneMemory::GetZoneMemorySizeInMegabytes() const { return MemoryBuffer ? MemoryBuffer->Size >> 10 >> 10 : 0; }
size_t FZoneMemory::GetTotalMemoryUsage() const { return TotalMemoryUsage.Load(); }
size_t FZoneMemory::GetTotalMemoryOverhead() const { return TotalMemoryOverhead.Load(); }
size_t FZoneMemory::GetTotalFreeMemory() const { return MemoryBuffer ? MemoryBuffer->Size - TotalMemoryUsage.Load() : 0; }
size_t FZoneMemory::GetMaxMemoryUsage() const { return MaxMemoryUsage.Load(); }

void FZoneMemory::Initialize( void * _MemoryAddress, int _SizeInMegabytes ) {
    size_t sizeInBytes = _SizeInMegabytes << 20;

#ifdef FREE_LIST_BASED
    MemoryBuffer = ( SZoneBuffer * )_MemoryAddress;
    MemoryBuffer->Size = sizeInBytes;
    MemoryBuffer->freeChunk = ( SZoneChunk * )( memoryBuffer + 1 );
    memset( MemoryBuffer->freeChunk, 0, sizeof( SZoneChunk ) );
    MemoryBuffer->freeChunk->size = MemoryBuffer->Size - sizeof( SZoneBuffer );;
#else
    MemoryBuffer = ( SZoneBuffer * )_MemoryAddress;
    MemoryBuffer->Size = sizeInBytes;
    MemoryBuffer->ChunkList.pPrev = MemoryBuffer->ChunkList.pNext = MemoryBuffer->Rover = ( SZoneChunk * )( MemoryBuffer + 1 );
    MemoryBuffer->ChunkList.Size = 0;
    MemoryBuffer->Rover->Size = MemoryBuffer->Size - sizeof( SZoneBuffer );
    MemoryBuffer->Rover->pNext = &MemoryBuffer->ChunkList;
    MemoryBuffer->Rover->pPrev = &MemoryBuffer->ChunkList;
#endif

    TotalMemoryUsage.StoreRelaxed( 0 );
    TotalMemoryOverhead.StoreRelaxed( 0 );
    MaxMemoryUsage.StoreRelaxed( 0 );
}

void FZoneMemory::Deinitialize() {
    CheckMemoryLeaks();

    MemoryBuffer = nullptr;

    TotalMemoryUsage.StoreRelaxed( 0 );
    TotalMemoryOverhead.StoreRelaxed( 0 );
    MaxMemoryUsage.StoreRelaxed( 0 );
}

void FZoneMemory::Clear() {
    if ( !MemoryBuffer ) {
        return;
    }

    Sync.BeginScope();

#ifdef FREE_LIST_BASED
    MemoryBuffer->freeChunk = ( SZoneChunk * )( MemoryBuffer + 1 );
    MemoryBuffer->freeChunk->size = MemoryBuffer->Size - sizeof( SZoneBuffer );
#else
    MemoryBuffer->ChunkList.pPrev = MemoryBuffer->ChunkList.pNext = MemoryBuffer->Rover = ( SZoneChunk * )( MemoryBuffer + 1 );
    MemoryBuffer->ChunkList.Size = 0;
    MemoryBuffer->Rover->Size = MemoryBuffer->Size - sizeof( SZoneBuffer );
    MemoryBuffer->Rover->pNext = &MemoryBuffer->ChunkList;
    MemoryBuffer->Rover->pPrev = &MemoryBuffer->ChunkList;
#endif

    TotalMemoryUsage.Store( 0 );
    TotalMemoryOverhead.Store( 0 );
    MaxMemoryUsage.Store( 0 );

    Sync.EndScope();

    // Allocated "on heap" memory is still present
}

void FZoneMemory::IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic increment for multithreading
    TotalMemoryUsage.FetchAdd( _MemoryUsage );
    TotalMemoryOverhead.FetchAdd( _Overhead );
    MaxMemoryUsage.Store( StdMax( MaxMemoryUsage.Load(), TotalMemoryUsage.Load() ) );
}

void FZoneMemory::DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic decrement for multithreading
    TotalMemoryUsage.FetchSub( _MemoryUsage );
    TotalMemoryOverhead.FetchSub( _Overhead );
}

#ifdef FREE_LIST_BASED
SZoneChunk * FZoneMemory::FindFreeChunk( int _RequiredSize ) {
    SZoneChunk * cur = FreeChunk;

    if ( !cur ) {
        // there is no free chunks
        return nullptr;
    }

    while ( cur->size < _RequiredSize ) {
        cur = cur->next; // find in right branch

        if ( !cur ) {

            // find in left brunch
            cur = FreeChunk->prev;
            if ( !cur ) {
                // no free chunks
                return nullptr;
            }

            while ( cur->size < _RequiredSize ) {
                cur = cur->prev;

                if ( !cur ) {
                    // no free chunks
                    return nullptr;
                }
            }
        }
    }

    return cur;
}

byte * FZoneMemory::AllocateBytes( size_t _BytesCount ) {

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        return nullptr;
    }

    size_t requiredSize = AdjustChunkSize( _BytesCount );

    SZoneChunk * cur = FindFreeChunk( requiredSize );
    if ( !cur ) {
        // no free chunks
#ifdef ALLOW_ALLOCATE_ON_HEAP
        return AllocateOnHeap( _BytesCount );
#else
        return nullptr;
#endif
    }

    int chunkNewSize = cur->size - requiredSize;
    if ( chunkNewSize < MinZoneFragmentLength ) {
        // Not enough space for free chunk, so add recidual space to used chunk

        requiredSize += chunkNewSize;

        // Remove current chunk from freed list
        FreeChunk = nullptr;
        if ( cur->prev ) {
            cur->prev->next = cur->next;
            FreeChunk = cur->prev;
        }
        if ( cur->next ) {
            cur->next->prev = cur->prev;
            FreeChunk = cur->next;
        }

    } else {
        FreeChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
        FreeChunk->size = chunkNewSize;
        FreeChunk->prev = cur->prev;
        FreeChunk->next = cur->next;
        if ( cur->prev ) {
            cur->prev->next = FreeChunk;
        }
        if ( cur->next ) {
            cur->next->prev = FreeChunk;
        }
        FreeChunk->used_next = cur->used_next;
        FreeChunk->used_prev = cur;
        if ( FreeChunk->used_next ) {
            FreeChunk->used_next->used_prev = FreeChunk;
        }

        cur->used_next = FreeChunk;
    }

    cur->next = nullptr;
    cur->prev = nullptr;
    cur->size = -requiredSize; // Set size to negative to mark chunk used
    SetTrashMarker( cur );

    byte * bytes = ( byte * )( cur + 1 );
#ifdef CLEAR_ALLOCATED_MEMORY
    ClearMemory8( bytes, 0, _BytesCount );
#endif

    IncMemoryStatistics( requiredSize, ChunkHeaderLength );

    return bytes;
}

void FZoneMemory::Dealloc( byte * _Bytes ) {
    if ( !_Bytes ) {
        return;
    }

    SZoneChunk * chunk = ( SZoneChunk * )( _Bytes - ChunkHeaderLength );

    if ( chunk->size > 0 ) {
        // freed pointer
        return;
    }

    if ( ChunkTrashTest( chunk ) ) {
        MemLogger.Print( "FZoneMemory::Dealloc: Warning: memory was trashed\n" );
        // error()
    }

    int chunkSize = -chunk->size;

#ifdef ALLOW_ALLOCATE_ON_HEAP
    if ( chunk->next == (SZoneChunk *)0xdeaddead ) {
        chunk->size = chunkSize;

        // Was allocated on heap
        AlignedFree( chunk );

        DecMemoryStatisticsOnHeap( chunkSize, ChunkHeaderLength );

        return;
    }
#endif

    SZoneChunk * prevChunk = chunk->used_prev;
    SZoneChunk * nextChunk = chunk->used_next;

    if ( prevChunk && nextChunk && prevChunk->size > 0 && nextChunk->size > 0 ) {
        // Merge prev, current and next chunks to one free chunk

        FreeChunk = prevChunk;
        FreeChunk->size = prevChunk->size + nextChunk->size + chunkSize;
        FreeChunk->next = nextChunk->next;
        FreeChunk->used_next = nextChunk->used_next;
        if ( nextChunk->next ) {
            nextChunk->next->prev = FreeChunk;
        }
        if ( nextChunk->used_next ) {
            nextChunk->used_next->used_prev = FreeChunk;
        }

    } else if ( prevChunk && prevChunk->size > 0 ) {
        // Merge prev and current chunks to one free chunk

        FreeChunk = prevChunk;
        FreeChunk->size = prevChunk->size + chunkSize;
        FreeChunk->next = chunk->next;
        FreeChunk->used_next = chunk->used_next;
        if ( chunk->next ) {
            chunk->next->prev = FreeChunk;
        }
        if ( chunk->used_next ) {
            chunk->used_next->used_prev = FreeChunk;
        }

    } else if ( nextChunk && nextChunk->size > 0 ) {
        // Merge current and next chunks to one free chunk

        FreeChunk = chunk;
        FreeChunk->size = nextChunk->size + chunkSize;
        FreeChunk->next = nextChunk->next;
        FreeChunk->used_next = nextChunk->used_next;
        if ( nextChunk->next ) {
            nextChunk->next->prev = FreeChunk;
        }
        if ( nextChunk->used_next ) {
            nextChunk->used_next->used_prev = FreeChunk;
        }
    } else {
        // Nothing to merge

        chunk->size = chunkSize;

        // Find nearest free chunk to keep sequence ordered
        // FIXME: is there faster way to do this?
        if ( FreeChunk ) {
            SZoneChunk * cur = FreeChunk;
            if ( FreeChunk < chunk ) {
                SZoneChunk * next = FreeChunk->next;
                while ( 1 ) {
                    if ( !next ) {
                        // add to back
                        chunk->prev = cur;
                        cur->next = chunk;
                        break;
                    }
                    if ( next > chunk ) {
                        // insert free chunk
                        chunk->prev = cur;
                        chunk->next = next;
                        next->prev = chunk;
                        cur->next = chunk;
                        break;
                    }
                    next = next->next;
                }
            } else {
                SZoneChunk * prev = FreeChunk->prev;
                while ( 1 ) {
                    if ( !prev ) {
                        // add to front
                        chunk->next = cur;
                        cur->prev = chunk;
                        break;
                    }
                    if ( prev < chunk ) {
                        // insert free chunk
                        chunk->next = cur;
                        chunk->prev = prev;
                        prev->next = chunk;
                        cur->prev = chunk;
                        break;
                    }
                    prev = prev->prev;
                }
            }
        }
        FreeChunk = chunk;
    }

    DecMemoryStatistics( chunkSize, ChunkHeaderLength );
}

#else

SZoneChunk * FZoneMemory::FindFreeChunk( int _RequiredSize ) {
    SZoneChunk * rover = MemoryBuffer->Rover;
    SZoneChunk * start = rover->pPrev;
    SZoneChunk * cur;

    do {
        if ( rover == start ) {
            return nullptr;
        }
        cur = rover;
        rover = rover->pNext;
    } while ( cur->Size < _RequiredSize );

    return cur;
}

void * FZoneMemory::Alloc( size_t _BytesCount, int _Alignment ) {
    AN_Assert( _Alignment <= 128 && IsPowerOfTwoConstexpr( _Alignment ) );

    if ( !MemoryBuffer ) {
        CriticalError( "FZoneMemory::Alloc: Not initialized\n" );
    }

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "FZoneMemory::Alloc: Invalid bytes count\n" );
    }

    FSyncGuard syncGuard( Sync );

    size_t requiredSize = AdjustChunkSize( _BytesCount, _Alignment );

    SZoneChunk * cur = FindFreeChunk( requiredSize );
    if ( !cur ) {
        // no free chunks
#ifdef ALLOW_ALLOCATE_ON_HEAP
        return AllocateOnHeap( _BytesCount, _Alignment );
#else
        CriticalError( "FZoneMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount );
#endif
    }

    int recidualChunkSpace = cur->Size - requiredSize;
    if ( recidualChunkSpace >= MinZoneFragmentLength ) {
        SZoneChunk * newChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
        newChunk->Size = recidualChunkSpace;
        newChunk->pPrev = cur;
        newChunk->pNext = cur->pNext;
        newChunk->pNext->pPrev = newChunk;
        cur->pNext = newChunk;
        cur->Size = requiredSize;
    }

    byte * pointer = ( byte * )cur + sizeof( SZoneChunk ) + 1;
    byte * aligned = ( byte * )AlignPointer( pointer, _Alignment );
    int padding = aligned - pointer + 1;
    *(aligned - 1) = padding;

    IncMemoryStatistics( cur->Size, ChunkHeaderLength + padding );

    cur->Size = -cur->Size; // Set size to negative to mark chunk used
    MemoryBuffer->Rover = cur->pNext;

    SetTrashMarker( cur );

    byte * bytes = aligned;//( byte * )( cur + 1 );
#ifdef CLEAR_ALLOCATED_MEMORY
    ClearMemory8( bytes, 0, _BytesCount );
#endif

    return bytes;
}

void * FZoneMemory::Extend( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld ) {
    AN_Assert( _NewAlignment <= 128 && IsPowerOfTwoConstexpr( _NewAlignment ) );

    if ( !_Data ) {
        //MemLogger.Printf( "first alloc\n" );
        return Alloc( _NewBytesCount, _NewAlignment );
    }

    // Check alignment changed
    if ( (size_t)_Data % _NewAlignment != 0 ) {
        void * temp;
        if ( _KeepOld ) {
            temp = Alloc( _NewBytesCount, _NewAlignment );
            memcpy( temp, _Data, _BytesCount );
#ifdef CLEAR_ALLOCATED_MEMORY
            ClearMemory8( (byte*)temp + _BytesCount, 0, _NewBytesCount - _BytesCount );
#endif
            Dealloc( _Data );
        } else {
            // Prefer to deallocate first
            Dealloc( _Data );
            temp = Alloc( _NewBytesCount, _NewAlignment );
        }
        //MemLogger.Printf( "alignment change\n" );
        return temp;
    }

    if ( _BytesCount >= _NewBytesCount ) {
        // data is big enough
        //MemLogger.Printf( "_BytesCount >= _NewBytesCount\n" );
        return _Data;
    }

    byte * bytes = ( byte * )_Data;
    int padding = *(bytes - 1);

    SZoneChunk * chunk = ( SZoneChunk * )( bytes - ChunkHeaderLength - padding );

    // Check freed
    bool bFreed;
    {
        FSyncGuard syncGuard( Sync );
        bFreed = chunk->Size > 0;

#ifdef ALLOW_ALLOCATE_ON_HEAP
        if ( chunk->next == (SZoneChunk *)0xdeaddead ) {
            TODO ...
        }
#endif
    }

    if ( bFreed ) {
        //MemLogger.Printf( "freed pointer\n" );
        return Alloc( _NewBytesCount, _NewAlignment );
    }

    FSyncGuard syncGuard( Sync );

    if ( ChunkTrashTest( chunk ) ) {
        MemLogger.Print( "FZoneMemory::ExtendAlloc: Warning: memory was trashed\n" );
        // error()
    }

    int requiredSize = AdjustChunkSize( _NewBytesCount, _NewAlignment );
    if ( requiredSize <= -chunk->Size ) {
        // data is big enough
        //MemLogger.Printf( "data is big enough\n" );
        return _Data;
    }

    chunk->Size = -chunk->Size;

    // try to use next chunk
    SZoneChunk * cur = chunk->pNext;
    if ( cur->Size > 0 && cur->Size + chunk->Size >= requiredSize ) {
        requiredSize -= chunk->Size;
        int recidualChunkSpace = cur->Size - requiredSize;
        if ( recidualChunkSpace >= MinZoneFragmentLength ) {
            SZoneChunk * newChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
            newChunk->Size = recidualChunkSpace;
            newChunk->pPrev = chunk;
            newChunk->pNext = cur->pNext;
            newChunk->pNext->pPrev = newChunk;
            chunk->pNext = newChunk;
            chunk->Size += requiredSize;
            IncMemoryStatistics( requiredSize, 0 );
        } else {
            chunk->pNext = cur->pNext;
            chunk->pNext->pPrev = chunk;
            chunk->Size += cur->Size;
            IncMemoryStatistics( cur->Size, 0 );
        }

        chunk->Size = -chunk->Size; // Set size to negative to mark chunk used
        MemoryBuffer->Rover = chunk->pNext;

        SetTrashMarker( chunk );

        //MemLogger.Printf( "using next chunk\n" );

        return _Data;
    } else {

        //MemLogger.Printf( "reallocating new chunk with copy\n" );

        cur = FindFreeChunk( requiredSize );
        if ( !cur ) {

            chunk->Size = -chunk->Size;

            // no free chunks

            CriticalError( "FZoneMemory::Extend: Failed on allocation of %u bytes\n", _NewBytesCount );
        }

        DecMemoryStatistics( chunk->Size, ChunkHeaderLength + padding );

        int recidualChunkSpace = cur->Size - requiredSize;
        if ( recidualChunkSpace >= MinZoneFragmentLength ) {
            SZoneChunk * newChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
            newChunk->Size = recidualChunkSpace;
            newChunk->pPrev = cur;
            newChunk->pNext = cur->pNext;
            newChunk->pNext->pPrev = newChunk;
            cur->pNext = newChunk;
            cur->Size = requiredSize;
        }

        byte * pointer = ( byte * )cur + sizeof( SZoneChunk ) + 1;
        byte * aligned = ( byte * )AlignPointer( pointer, _NewAlignment );
        padding = aligned - pointer + 1;
        *(aligned - 1) = padding;

        IncMemoryStatistics( cur->Size, ChunkHeaderLength + padding );

        cur->Size = -cur->Size; // Set size to negative to mark chunk used
        MemoryBuffer->Rover = cur->pNext;

        SetTrashMarker( cur );

        bytes = aligned;//( byte * )( cur + 1 );

        if ( _KeepOld ) {
            memcpy( bytes, _Data, _BytesCount );
#ifdef CLEAR_ALLOCATED_MEMORY
            ClearMemory8( bytes + _BytesCount, 0, _NewBytesCount - _BytesCount );
#endif
        } else {
#ifdef CLEAR_ALLOCATED_MEMORY
            ClearMemory8( bytes, 0, _NewBytesCount );
#endif
        }

        // Deallocate old chunk

        SZoneChunk * prevChunk = chunk->pPrev;
        SZoneChunk * nextChunk = chunk->pNext;

        if ( prevChunk->Size > 0 ) {
            // Merge prev and current chunks to one free chunk

            prevChunk->Size += chunk->Size;
            prevChunk->pNext = chunk->pNext;
            prevChunk->pNext->pPrev = prevChunk;

            if ( chunk == MemoryBuffer->Rover ) {
                MemoryBuffer->Rover = prevChunk;
            }
            chunk = prevChunk;
        }

        if ( nextChunk->Size > 0 ) {
            // Merge current and next chunks to one free chunk

            chunk->Size += nextChunk->Size;
            chunk->pNext = nextChunk->pNext;
            chunk->pNext->pPrev = chunk;

            if ( nextChunk == MemoryBuffer->Rover ) {
                MemoryBuffer->Rover = chunk;
            }
        }

        return bytes;
    }
}

void FZoneMemory::Dealloc( void * _Bytes ) {
    if ( !MemoryBuffer ) {
        return;
    }

    if ( !_Bytes ) {
        return;
    }

    FSyncGuard syncGuard( Sync );

    byte * bytes = ( byte * )_Bytes;
    int padding = *(bytes - 1);

    SZoneChunk * chunk = ( SZoneChunk * )( bytes - ChunkHeaderLength - padding );

    if ( chunk->Size > 0 ) {
        // freed pointer
        return;
    }

    if ( ChunkTrashTest( chunk ) ) {
        MemLogger.Print( "FZoneMemory::Dealloc: Warning: memory was trashed\n" );
        // error()
    }

    chunk->Size = -chunk->Size;

#ifdef ALLOW_ALLOCATE_ON_HEAP
    if ( chunk->next == (SZoneChunk *)0xdeaddead ) {

        // Was allocated on heap
        UnalignedFree( chunk );

        DecMemoryStatisticsOnHeap( chunk->size, ChunkHeaderLength + padding );

        return;
    }
#endif

    DecMemoryStatistics( chunk->Size, ChunkHeaderLength + padding );

    SZoneChunk * prevChunk = chunk->pPrev;
    SZoneChunk * nextChunk = chunk->pNext;

    if ( prevChunk->Size > 0 ) {
        // Merge prev and current chunks to one free chunk

        prevChunk->Size += chunk->Size;
        prevChunk->pNext = chunk->pNext;
        prevChunk->pNext->pPrev = prevChunk;

        if ( chunk == MemoryBuffer->Rover ) {
            MemoryBuffer->Rover = prevChunk;
        }
        chunk = prevChunk;
    }

    if ( nextChunk->Size > 0 ) {
        // Merge current and next chunks to one free chunk

        chunk->Size += nextChunk->Size;
        chunk->pNext = nextChunk->pNext;
        chunk->pNext->pPrev = chunk;

        if ( nextChunk == MemoryBuffer->Rover ) {
            MemoryBuffer->Rover = chunk;
        }
    }
}

void FZoneMemory::CheckMemoryLeaks() {
    FSyncGuard syncGuard( Sync );

    if ( TotalMemoryUsage.Load() > 0 ) {
        SZoneChunk * rover = MemoryBuffer->Rover;
        SZoneChunk * start = rover->pPrev;
        SZoneChunk * cur;

        do {
            cur = rover;
            if ( cur->Size < 0 ) {
                MemLogger.Print( "==== Zone Memory Leak ====\n" );
                MemLogger.Printf( "Chunk Address: %u Size: %d\n", (size_t)( cur + 1 ), (-cur->Size) );

                //int sz = -cur->Size;
                //char * b = (char*)(cur+1);
                //while ( sz-->0 ) {
                //    MemLogger.Printf("%c",*b > 31 && *b < 127 ? *b : '?');
                //    b++;
                //}
            }

            if ( rover == start ) {
                break;
            }
            rover = rover->pNext;

        } while ( 1 );
    }
}

#endif


//////////////////////////////////////////////////////////////////////////////////////////
//
// Allocators
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename T >
static void PrintMemoryStatistics( T & _Manager, const char * _Message ) {
    MemLogger.Printf( "%s: MaxMemoryUsage %d TotalMemoryUsage %d TotalMemoryOverhead %d\n",
            _Message, _Manager.GetMaxMemoryUsage(), _Manager.GetTotalMemoryUsage(), _Manager.GetTotalMemoryOverhead() );
}

//
// Zone allocator
//

void * FZoneAllocator::ImplAllocate( size_t _BytesCount, int _Alignment ) {
    void * bytes = GZoneMemory.Alloc( _BytesCount, _Alignment );
    //PrintMemoryStatistics( GZoneMemory, "AllocateBytes" );
    return bytes;
}

void * FZoneAllocator::ImplExtend( void * _Data, size_t _BytesCount, size_t _NewBytesCount, int _NewAlignment, bool _KeepOld ) {
    void * bytes = GZoneMemory.Extend( _Data, _BytesCount, _NewBytesCount, _NewAlignment, _KeepOld );
    //PrintMemoryStatistics( GZoneMemory, "ExtendAlloc" );
    return bytes;
}

void FZoneAllocator::ImplDeallocate( void * _Bytes ) {
    GZoneMemory.Dealloc( _Bytes );
    //PrintMemoryStatistics( GZoneMemory, "Dealloc" );
}

void * FHeapAllocator::ImplAllocate( size_t _BytesCount, int _Alignment ) {
    void * bytes = GHeapMemory.HeapAlloc( _BytesCount, _Alignment );
    //PrintMemoryStatistics( GMainHeapMemory, "AllocateBytes" );
    return bytes;
}

//
// Heap allocator
//

void * FHeapAllocator::ImplExtend( void * _Data, size_t _BytesCount, size_t _NewBytesCount, int _NewAlignment, bool _KeepOld ) {
    void * bytes;
    if ( !_Data ) {
        // first alloc
        bytes = GHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
    } else if ( (size_t)_Data % _NewAlignment != 0 ) {
        // alignment changed
        if ( _KeepOld ) {
            bytes = GHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
            memcpy( bytes, _Data, _BytesCount );
#ifdef CLEAR_ALLOCATED_MEMORY
            ClearMemory8( (byte*)bytes + _BytesCount, 0, _NewBytesCount - _BytesCount );
#endif
            GHeapMemory.HeapFree( _Data );
        } else {
            GHeapMemory.HeapFree( _Data );
            bytes = GHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
        }
    } else if ( _BytesCount >= _NewBytesCount ) {
        // big enough
        bytes = _Data;
    } else {
        // reallocate
        if ( _KeepOld ) {
            bytes = GHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
            memcpy( bytes, _Data, _BytesCount );
#ifdef CLEAR_ALLOCATED_MEMORY
            ClearMemory8( (byte*)bytes + _BytesCount, 0, _NewBytesCount - _BytesCount );
#endif
            GHeapMemory.HeapFree( _Data );
        } else {
            GHeapMemory.HeapFree( _Data );
            bytes = GHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
        }
    }
    //PrintMemoryStatistics( GMainHeapMemory, "ExtendAlloc" );
    return bytes;
}

void FHeapAllocator::ImplDeallocate( void * _Bytes ) {
    GHeapMemory.HeapFree( _Bytes );
    //PrintMemoryStatistics( GMainHeapMemory, "Dealloc" );
}

void * HugeAlloc( size_t _Size ) {
    return GHeapMemory.HeapAlloc( _Size, 1 );
}

void HugeFree( void * _Data ) {
    GHeapMemory.HeapFree( _Data );
}
