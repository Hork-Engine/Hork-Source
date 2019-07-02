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

FHeapMemory GMainHeapMemory;
FHunkMemory GMainHunkMemory;
FMemoryZone GMainMemoryZone;

//////////////////////////////////////////////////////////////////////////////////////////
//
// Memory common functions/constants/defines
//
//////////////////////////////////////////////////////////////////////////////////////////

//#define CLEAR_ALLOCATED_MEMORY
#define ENABLE_TRASH_TEST
#define ENABLE_BLOCK_SIZE_ALIGN

typedef uint16_t trashMarker_t;

static const trashMarker_t TrashMarker = 0xfeee;

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
    HeapMaxMemoryUsage = FMath_Max( HeapMaxMemoryUsage, HeapTotalMemoryUsage );
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
    HeapChain.next = HeapChain.prev = &HeapChain;
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
    heap_t * nextHeap;
    for ( heap_t * heap = HeapChain.next ; heap != &HeapChain ; heap = nextHeap ) {
        nextHeap = heap->next;
        HeapFree( heap + 1 );
    }
}

AN_FORCEINLINE void * AlignPointer( void * _UnalignedPtr, int _Alignment ) {
#if 0
    struct aligner_t {
        union {
            void * pointer;
            size_t i;
        };
    };
    aligner_t aligner;
    const size_t bitMask = ~( _Alignment - 1 );
    aligner.pointer = _UnalignedPtr;
    aligner.i += _Alignment - 1;
    aligner.i &= bitMask;
    return aligner.pointer;
#endif
    return ( void * )( ( (size_t)_UnalignedPtr + _Alignment - 1 ) & ~( _Alignment - 1 ) );
}

void * FHeapMemory::HeapAlloc( size_t _BytesCount, int _Alignment ) {
    AN_Assert( _Alignment <= 128 && IsPowerOfTwoConstexpr( _Alignment ) );

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "FHeapMemory::HeapAlloc: Invalid bytes count\n" );
        return nullptr;
    }

#ifdef ENABLE_BLOCK_SIZE_ALIGN
    _BytesCount = ( _BytesCount + 7 ) & ~7;
#endif

    size_t realAllocatedBytes = _BytesCount + sizeof( heap_t ) + _Alignment - 1;

#ifdef ENABLE_TRASH_TEST
    realAllocatedBytes += sizeof( trashMarker_t );
#endif

    byte * bytes;

    bytes = ( byte * )UnalignedMalloc( realAllocatedBytes );

    if ( bytes ) {
        byte * aligned = ( byte * )AlignPointer( bytes + sizeof( heap_t ), _Alignment );

        //heap_t * heap = ( heap_t * )( bytes + padding );
        heap_t * heap = ( heap_t * )( aligned ) - 1;
        heap->size = realAllocatedBytes;
        heap->padding = aligned - bytes;
        heap->next = HeapChain.next;
        heap->prev = &HeapChain;
        HeapChain.next->prev = heap;
        HeapChain.next = heap;

#ifdef CLEAR_ALLOCATED_MEMORY
        ClearMemory8( aligned, 0, _BytesCount );
#endif
#ifdef ENABLE_TRASH_TEST
        *( trashMarker_t * )( bytes + realAllocatedBytes - sizeof( trashMarker_t ) ) = TrashMarker;
#endif
        IncMemoryStatisticsOnHeap( realAllocatedBytes, heap->padding );

        return aligned;
    }

    CriticalError( "FHeapMemory::HeapAlloc: Failed on allocation of %u bytes\n", _BytesCount );
    return nullptr;
}

void FHeapMemory::HeapFree( void * _Bytes ) {
    if ( _Bytes ) {
        byte * bytes = ( byte * )_Bytes;

        heap_t * heap = ( heap_t * )( bytes ) - 1;
        bytes -= heap->padding;

#ifdef ENABLE_TRASH_TEST
        if ( *( trashMarker_t * )( bytes + heap->size - sizeof( trashMarker_t ) ) != TrashMarker ) {
            MemLogger.Print( "FHeapMemory::HeapFree: Warning: memory was trashed\n" );
            // error()
        }
#endif

        heap->prev->next = heap->next;
        heap->next->prev = heap->prev;

        DecMemoryStatisticsOnHeap( heap->size, heap->padding );

        UnalignedFree( bytes );
    }
}

void FHeapMemory::CheckMemoryLeaks() {
    for ( heap_t * heap = HeapChain.next ; heap != &HeapChain ; heap = heap->next ) {
        MemLogger.Print( "==== Heap Memory Leak ====\n" );
        MemLogger.Printf( "Heap Address: %u Size: %d\n", (size_t)( heap + 1 ), heap->size );
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

struct hunk_t {
    int32_t size;
    int32_t mark;
    hunk_t * prev;
};

struct hunkMemory_t {
    size_t size;
    hunk_t * hunk; // pointer to next hunk
    int32_t mark;
};

void * FHunkMemory::GetHunkMemoryAddress() const { return MemoryBuffer; }
int FHunkMemory::GetHunkMemorySizeInMegabytes() const { return MemoryBuffer ? MemoryBuffer->size >> 10 >> 10 : 0; }
size_t FHunkMemory::GetTotalMemoryUsage() const { return TotalMemoryUsage; }
size_t FHunkMemory::GetTotalMemoryOverhead() const { return TotalMemoryOverhead; }
size_t FHunkMemory::GetTotalFreeMemory() const { return MemoryBuffer ? MemoryBuffer->size - TotalMemoryUsage : 0; }
size_t FHunkMemory::GetMaxMemoryUsage() const { return MaxMemoryUsage; }

void FHunkMemory::Initialize( void * _MemoryAddress, int _SizeInMegabytes ) {
    size_t sizeInBytes = _SizeInMegabytes << 20;

    MemoryBuffer = ( hunkMemory_t * )_MemoryAddress;
    MemoryBuffer->size = sizeInBytes;
    MemoryBuffer->mark = 0;
    MemoryBuffer->hunk = ( hunk_t * )( MemoryBuffer + 1 );
    MemoryBuffer->hunk->size = MemoryBuffer->size - sizeof( hunkMemory_t );
    MemoryBuffer->hunk->mark = -1; // marked free
    MemoryBuffer->hunk->prev = nullptr;

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
    MemoryBuffer->mark = 0;
    MemoryBuffer->hunk = ( hunk_t * )( MemoryBuffer + 1 );
    MemoryBuffer->hunk->size = MemoryBuffer->size - sizeof( hunkMemory_t );
    MemoryBuffer->hunk->mark = -1; // marked free
    MemoryBuffer->hunk->prev = nullptr;

    TotalMemoryUsage = 0;
    MaxMemoryUsage = 0;
    TotalMemoryOverhead = 0;
}

int FHunkMemory::SetHunkMark() {
    return ++MemoryBuffer->mark;
}

AN_FORCEINLINE size_t AdjustHunkSize( size_t _BytesCount, int _Alignment ) {
#ifdef ENABLE_BLOCK_SIZE_ALIGN
    _BytesCount = ( _BytesCount + 7 ) & ~7;
#endif
    size_t requiredSize = _BytesCount + sizeof( hunk_t ) + _Alignment - 1;
#ifdef ENABLE_TRASH_TEST
    requiredSize += sizeof( trashMarker_t );
#endif
    return requiredSize;
}

AN_FORCEINLINE void SetTrashMarker( hunk_t * _Hunk ) {
#ifdef ENABLE_TRASH_TEST
    *( trashMarker_t * )( ( byte * )( _Hunk ) + _Hunk->size - sizeof( trashMarker_t ) ) = TrashMarker;
#endif
}

AN_FORCEINLINE bool HunkTrashTest( const hunk_t * _Hunk ) {
#ifdef ENABLE_TRASH_TEST
    return *( const trashMarker_t * )( ( const byte * )( _Hunk ) + _Hunk->size - sizeof( trashMarker_t ) ) != TrashMarker;
#else
    return false;
#endif
}

void * FHunkMemory::HunkMemory( size_t _BytesCount, int _Alignment ) {
    AN_Assert( _Alignment <= 128 && IsPowerOfTwoConstexpr( _Alignment ) );

    if ( !MemoryBuffer ) {
        CriticalError( "FHunkMemory::HunkMemory: Not initialized\n" );
        return nullptr;
    }

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "FHunkMemory::HunkMemory: Invalid bytes count\n" );
        return nullptr;
    }

    hunk_t * hunk = MemoryBuffer->hunk;

    if ( hunk->mark != -1 ) {
        // not enough memory
        CriticalError( "FHunkMemory::HunkMemory: Failed on allocation of %u bytes\n", _BytesCount );
        return nullptr;
    }

    int requiredSize = AdjustHunkSize( _BytesCount, _Alignment );
    if ( requiredSize > hunk->size ) {
        // not enough memory
        CriticalError( "FHunkMemory::HunkMemory: Failed on allocation of %u bytes\n", _BytesCount );
        return nullptr;
    }

    hunk->mark = MemoryBuffer->mark;

    int hunkNewSize = hunk->size - requiredSize;
    if ( hunkNewSize >= MinHunkFragmentLength ) {
        hunk_t * newHunk = ( hunk_t * )( ( byte * )( hunk ) + requiredSize );
        newHunk->size = hunkNewSize;
        newHunk->mark = -1;
        newHunk->prev = hunk;
        MemoryBuffer->hunk = newHunk;
        hunk->size = requiredSize;
    }

    SetTrashMarker( hunk );

    IncMemoryStatistics( hunk->size, sizeof( hunk_t ) );

    void * aligned = AlignPointer( hunk + 1, _Alignment );
#ifdef CLEAR_ALLOCATED_MEMORY
    ClearMemory8( aligned, 0, _BytesCount );
#endif

    return aligned;
}

void FHunkMemory::ClearToMark( int _Mark ) {
    if ( MemoryBuffer->mark < _Mark ) {
        return;
    }

    if ( _Mark <= 0 ) {
        // Simple case
        Clear();
        return;
    }

    int grow = 0;

    hunk_t * hunk = MemoryBuffer->hunk;
    if ( hunk->mark == -1 ) {
        // get last allocated hunk
        grow = hunk->size;
        hunk = hunk->prev;
    }

    while ( hunk && hunk->mark >= _Mark ) {
        DecMemoryStatistics( hunk->size, sizeof( hunk_t ) );
        if ( HunkTrashTest( hunk ) ) {
            MemLogger.Print( "FHunkMemory::ClearToMark: Warning: memory was trashed\n" );
            // error()
        }
        hunk->size += grow;
        hunk->mark = -1; // mark free
        MemoryBuffer->hunk = hunk;
        grow = hunk->size;
        hunk = hunk->prev;
    }

    MemoryBuffer->mark = _Mark;
}

void FHunkMemory::ClearLastHunk() {
    int grow = 0;

    hunk_t * hunk = MemoryBuffer->hunk;
    if ( hunk->mark == -1 ) {
        // get last allocated hunk
        grow = hunk->size;
        hunk = hunk->prev;
    }

    if ( hunk ) {
        DecMemoryStatistics( hunk->size, sizeof( hunk_t ) );
        if ( HunkTrashTest( hunk ) ) {
            MemLogger.Print( "FHunkMemory::ClearLastHunk: Warning: memory was trashed\n" );
            // error()
        }
        hunk->size += grow;
        hunk->mark = -1; // mark free
        MemoryBuffer->hunk = hunk;
    }
}

void FHunkMemory::CheckMemoryLeaks() {
    if ( TotalMemoryUsage > 0 ) {
        hunk_t * hunk = MemoryBuffer->hunk;
        if ( hunk->mark == -1 ) {
            // get last allocated hunk
            hunk = hunk->prev;
        }
        while ( hunk ) {
            MemLogger.Print( "==== Hunk Memory Leak ====\n" );
            MemLogger.Printf( "Hunk Address: %u Size: %d\n", (size_t)( hunk + 1 ), hunk->size );
            hunk = hunk->prev;
        }
    }
}

void FHunkMemory::IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic increment for multithreading
    TotalMemoryUsage += _MemoryUsage;
    TotalMemoryOverhead += _Overhead;
    MaxMemoryUsage = FMath_Max( MaxMemoryUsage, TotalMemoryUsage );
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
struct chunk_t {
    chunk_t * next;
    chunk_t * prev;
    chunk_t * used_next;
    chunk_t * used_prev;
    int32_t size;
};
struct memoryBuffer_t {
    chunk_t * freeChunk; // pointer to last freed chunk
    int32_t size;
};
#else
// TODO: Align chunk header structure if need
struct chunk_t {
    chunk_t * next; // FIXME: можно избавиться от next, так как next легко вычилслить как ((byte*)chunk) + chunk->size
    chunk_t * prev;
    int32_t size;
};
// TODO: Align header if need
struct memoryBuffer_t {
    chunk_t * rover;
    chunk_t chunkList;
    int32_t size;
};
#endif

static const int ChunkHeaderLength = sizeof( chunk_t );
static const int MinZoneFragmentLength = 64; // Must be > ChunkHeaderLength

AN_FORCEINLINE size_t AdjustChunkSize( size_t _BytesCount, int _Alignment ) {
#ifdef ENABLE_BLOCK_SIZE_ALIGN
    _BytesCount = ( _BytesCount + 7 ) & ~7;
#endif
    size_t requiredSize = _BytesCount + ChunkHeaderLength + _Alignment - 1 + 1; // + 1 for one byte that keeps padding
#ifdef ENABLE_TRASH_TEST
    requiredSize += sizeof( trashMarker_t );
#endif
    return requiredSize;
}

AN_FORCEINLINE void SetTrashMarker( chunk_t * _Chunk ) {
#ifdef ENABLE_TRASH_TEST
    *( trashMarker_t * )( ( byte * )( _Chunk ) + ( -_Chunk->size ) - sizeof( trashMarker_t ) ) = TrashMarker;
#endif
}

AN_FORCEINLINE bool ChunkTrashTest( const chunk_t * _Chunk ) {
#ifdef ENABLE_TRASH_TEST
    return *( const trashMarker_t * )( ( const byte * )( _Chunk ) + ( -_Chunk->size ) - sizeof( trashMarker_t ) ) != TrashMarker;
#else
    return false;
#endif
}

static void * AllocateOnHeap( size_t _BytesCount, int _Alignment ) {
    size_t requiredSize;
    byte * bytes;
    byte * pointer;
    byte * aligned;
    chunk_t * heapChunk;
    int padding;

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "AllocateOnHeap: Invalid bytes count\n" );
        return nullptr;
    }

    requiredSize = AdjustChunkSize( _BytesCount, _Alignment );
    bytes = reinterpret_cast< byte * >( UnalignedMalloc( requiredSize ) );

    if ( bytes ) {
        pointer = bytes + sizeof( chunk_t ) + 1;
        aligned = ( byte * )AlignPointer( pointer, _Alignment );
        padding = aligned - pointer + 1;
        *(aligned - 1) = padding;

        heapChunk = (chunk_t *)bytes;

        heapChunk->next = (chunk_t *)(size_t)0xdeaddead;
        heapChunk->prev = (chunk_t *)(size_t)0xdeaddead;
#ifdef FREE_LIST_BASED
        heapChunk->used_next = (chunk_t *)0xdeaddead;
        heapChunk->used_prev = (chunk_t *)0xdeaddead;
#endif
        heapChunk->size = -(int32_t)requiredSize;
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

void * FMemoryZone::GetZoneMemoryAddress() const { return MemoryBuffer; }
int FMemoryZone::GetZoneMemorySizeInMegabytes() const { return MemoryBuffer ? MemoryBuffer->size >> 10 >> 10 : 0; }
size_t FMemoryZone::GetTotalMemoryUsage() const { return TotalMemoryUsage.Load(); }
size_t FMemoryZone::GetTotalMemoryOverhead() const { return TotalMemoryOverhead.Load(); }
size_t FMemoryZone::GetTotalFreeMemory() const { return MemoryBuffer ? MemoryBuffer->size - TotalMemoryUsage.Load() : 0; }
size_t FMemoryZone::GetMaxMemoryUsage() const { return MaxMemoryUsage.Load(); }

void FMemoryZone::Initialize( void * _MemoryAddress, int _SizeInMegabytes ) {
    size_t sizeInBytes = _SizeInMegabytes << 20;

#ifdef FREE_LIST_BASED
    MemoryBuffer = ( memoryBuffer_t * )_MemoryAddress;
    MemoryBuffer->size = sizeInBytes;
    MemoryBuffer->freeChunk = ( chunk_t * )( memoryBuffer + 1 );
    memset( MemoryBuffer->freeChunk, 0, sizeof( chunk_t ) );
    MemoryBuffer->freeChunk->size = MemoryBuffer->size - sizeof( memoryBuffer_t );;
#else
    MemoryBuffer = ( memoryBuffer_t * )_MemoryAddress;
    MemoryBuffer->size = sizeInBytes;
    MemoryBuffer->chunkList.prev = MemoryBuffer->chunkList.next = MemoryBuffer->rover = ( chunk_t * )( MemoryBuffer + 1 );
    MemoryBuffer->chunkList.size = 0;
    MemoryBuffer->rover->size = MemoryBuffer->size - sizeof( memoryBuffer_t );
    MemoryBuffer->rover->next = &MemoryBuffer->chunkList;
    MemoryBuffer->rover->prev = &MemoryBuffer->chunkList;
#endif

    TotalMemoryUsage.StoreRelaxed( 0 );
    TotalMemoryOverhead.StoreRelaxed( 0 );
    MaxMemoryUsage.StoreRelaxed( 0 );
}

void FMemoryZone::Deinitialize() {
    CheckMemoryLeaks();

    MemoryBuffer = nullptr;

    TotalMemoryUsage.StoreRelaxed( 0 );
    TotalMemoryOverhead.StoreRelaxed( 0 );
    MaxMemoryUsage.StoreRelaxed( 0 );
}

void FMemoryZone::Clear() {
    if ( !MemoryBuffer ) {
        return;
    }

    Sync.BeginScope();

#ifdef FREE_LIST_BASED
    MemoryBuffer->freeChunk = ( chunk_t * )( MemoryBuffer + 1 );
    MemoryBuffer->freeChunk->size = MemoryBuffer->size - sizeof( memoryBuffer_t );;
#else
    MemoryBuffer->chunkList.prev = MemoryBuffer->chunkList.next = MemoryBuffer->rover = ( chunk_t * )( MemoryBuffer + 1 );
    MemoryBuffer->chunkList.size = 0;
    MemoryBuffer->rover->size = MemoryBuffer->size - sizeof( memoryBuffer_t );
    MemoryBuffer->rover->next = &MemoryBuffer->chunkList;
    MemoryBuffer->rover->prev = &MemoryBuffer->chunkList;
#endif

    TotalMemoryUsage.Store( 0 );
    TotalMemoryOverhead.Store( 0 );
    MaxMemoryUsage.Store( 0 );

    Sync.EndScope();

    // Allocated "on heap" memory is still present
}

void FMemoryZone::IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic increment for multithreading
    TotalMemoryUsage.FetchAdd( _MemoryUsage );
    TotalMemoryOverhead.FetchAdd( _Overhead );
    MaxMemoryUsage.Store( FMath_Max( MaxMemoryUsage.Load(), TotalMemoryUsage.Load() ) );
}

void FMemoryZone::DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    // TODO: atomic decrement for multithreading
    TotalMemoryUsage.FetchSub( _MemoryUsage );
    TotalMemoryOverhead.FetchSub( _Overhead );
}

#ifdef FREE_LIST_BASED
chunk_t * FMemoryZone::FindFreeChunk( int _RequiredSize ) {
    chunk_t * cur = FreeChunk;

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

byte * FMemoryZone::AllocateBytes( size_t _BytesCount ) {

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        return nullptr;
    }

    size_t requiredSize = AdjustChunkSize( _BytesCount );

    chunk_t * cur = FindFreeChunk( requiredSize );
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
        FreeChunk = ( chunk_t * )( ( byte * )( cur ) + requiredSize );
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

void FMemoryZone::Dealloc( byte * _Bytes ) {
    if ( !_Bytes ) {
        return;
    }

    chunk_t * chunk = ( chunk_t * )( _Bytes - ChunkHeaderLength );

    if ( chunk->size > 0 ) {
        // freed pointer
        return;
    }

    if ( ChunkTrashTest( chunk ) ) {
        MemLogger.Print( "FMemoryZone::Dealloc: Warning: memory was trashed\n" );
        // error()
    }

    int chunkSize = -chunk->size;

#ifdef ALLOW_ALLOCATE_ON_HEAP
    if ( chunk->next == (chunk_t *)0xdeaddead ) {
        chunk->size = chunkSize;

        // Was allocated on heap
        AlignedFree( chunk );

        DecMemoryStatisticsOnHeap( chunkSize, ChunkHeaderLength );

        return;
    }
#endif

    chunk_t * prevChunk = chunk->used_prev;
    chunk_t * nextChunk = chunk->used_next;

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
            chunk_t * cur = FreeChunk;
            if ( FreeChunk < chunk ) {
                chunk_t * next = FreeChunk->next;
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
                chunk_t * prev = FreeChunk->prev;
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

chunk_t * FMemoryZone::FindFreeChunk( int _RequiredSize ) {
    chunk_t * rover = MemoryBuffer->rover;
    chunk_t * start = rover->prev;
    chunk_t * cur;

    do {
        if ( rover == start ) {
            return nullptr;
        }
        cur = rover;
        rover = rover->next;
    } while ( cur->size < _RequiredSize );

    return cur;
}

void * FMemoryZone::Alloc( size_t _BytesCount, int _Alignment ) {
    AN_Assert( _Alignment <= 128 && IsPowerOfTwoConstexpr( _Alignment ) );

    if ( !MemoryBuffer ) {
        CriticalError( "FMemoryZone::Alloc: Not initialized\n" );
        return nullptr;
    }

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "FMemoryZone::Alloc: Invalid bytes count\n" );
        return nullptr;
    }

    FSyncGuard syncGuard( Sync );

    size_t requiredSize = AdjustChunkSize( _BytesCount, _Alignment );

    chunk_t * cur = FindFreeChunk( requiredSize );
    if ( !cur ) {
        // no free chunks
#ifdef ALLOW_ALLOCATE_ON_HEAP
        return AllocateOnHeap( _BytesCount, _Alignment );
#else
        CriticalError( "FMemoryZone::Alloc: Failed on allocation of %u bytes\n", _BytesCount );
        return nullptr;
#endif
    }

    int recidualChunkSpace = cur->size - requiredSize;
    if ( recidualChunkSpace >= MinZoneFragmentLength ) {
        chunk_t * newChunk = ( chunk_t * )( ( byte * )( cur ) + requiredSize );
        newChunk->size = recidualChunkSpace;
        newChunk->prev = cur;
        newChunk->next = cur->next;
        newChunk->next->prev = newChunk;
        cur->next = newChunk;
        cur->size = requiredSize;
    }

    byte * pointer = ( byte * )cur + sizeof( chunk_t ) + 1;
    byte * aligned = ( byte * )AlignPointer( pointer, _Alignment );
    int padding = aligned - pointer + 1;
    *(aligned - 1) = padding;

    IncMemoryStatistics( cur->size, ChunkHeaderLength + padding );

    cur->size = -cur->size; // Set size to negative to mark chunk used
    MemoryBuffer->rover = cur->next;

    SetTrashMarker( cur );

    byte * bytes = aligned;//( byte * )( cur + 1 );
#ifdef CLEAR_ALLOCATED_MEMORY
    ClearMemory8( bytes, 0, _BytesCount );
#endif

    return bytes;
}

void * FMemoryZone::Extend( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld ) {
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

    chunk_t * chunk = ( chunk_t * )( bytes - ChunkHeaderLength - padding );

    // Check freed
    bool bFreed;
    {
        FSyncGuard syncGuard( Sync );
        bFreed = chunk->size > 0;

#ifdef ALLOW_ALLOCATE_ON_HEAP
        if ( chunk->next == (chunk_t *)0xdeaddead ) {
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
        MemLogger.Print( "FMemoryZone::ExtendAlloc: Warning: memory was trashed\n" );
        // error()
    }

    int requiredSize = AdjustChunkSize( _NewBytesCount, _NewAlignment );
    if ( requiredSize <= -chunk->size ) {
        // data is big enough
        //MemLogger.Printf( "data is big enough\n" );
        return _Data;
    }

    chunk->size = -chunk->size;

    // try to use next chunk
    chunk_t * cur = chunk->next;
    if ( cur->size > 0 && cur->size + chunk->size >= requiredSize ) {
        requiredSize -= chunk->size;
        int recidualChunkSpace = cur->size - requiredSize;
        if ( recidualChunkSpace >= MinZoneFragmentLength ) {
            chunk_t * newChunk = ( chunk_t * )( ( byte * )( cur ) + requiredSize );
            newChunk->size = recidualChunkSpace;
            newChunk->prev = chunk;
            newChunk->next = cur->next;
            newChunk->next->prev = newChunk;
            chunk->next = newChunk;
            chunk->size += requiredSize;
            IncMemoryStatistics( requiredSize, 0 );
        } else {
            chunk->next = cur->next;
            chunk->next->prev = chunk;
            chunk->size += cur->size;
            IncMemoryStatistics( cur->size, 0 );
        }

        chunk->size = -chunk->size; // Set size to negative to mark chunk used
        MemoryBuffer->rover = chunk->next;

        SetTrashMarker( chunk );

        //MemLogger.Printf( "using next chunk\n" );

        return _Data;
    } else {

        //MemLogger.Printf( "reallocating new chunk with copy\n" );

        cur = FindFreeChunk( requiredSize );
        if ( !cur ) {

            chunk->size = -chunk->size;

            // no free chunks

            CriticalError( "FMemoryZone::Extend: Failed on allocation of %u bytes\n", _NewBytesCount );

            return nullptr;
        }

        DecMemoryStatistics( chunk->size, ChunkHeaderLength + padding );

        int recidualChunkSpace = cur->size - requiredSize;
        if ( recidualChunkSpace >= MinZoneFragmentLength ) {
            chunk_t * newChunk = ( chunk_t * )( ( byte * )( cur ) + requiredSize );
            newChunk->size = recidualChunkSpace;
            newChunk->prev = cur;
            newChunk->next = cur->next;
            newChunk->next->prev = newChunk;
            cur->next = newChunk;
            cur->size = requiredSize;
        }

        byte * pointer = ( byte * )cur + sizeof( chunk_t ) + 1;
        byte * aligned = ( byte * )AlignPointer( pointer, _NewAlignment );
        padding = aligned - pointer + 1;
        *(aligned - 1) = padding;

        IncMemoryStatistics( cur->size, ChunkHeaderLength + padding );

        cur->size = -cur->size; // Set size to negative to mark chunk used
        MemoryBuffer->rover = cur->next;

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

        chunk_t * prevChunk = chunk->prev;
        chunk_t * nextChunk = chunk->next;

        if ( prevChunk->size > 0 ) {
            // Merge prev and current chunks to one free chunk

            prevChunk->size += chunk->size;
            prevChunk->next = chunk->next;
            prevChunk->next->prev = prevChunk;

            if ( chunk == MemoryBuffer->rover ) {
                MemoryBuffer->rover = prevChunk;
            }
            chunk = prevChunk;
        }

        if ( nextChunk->size > 0 ) {
            // Merge current and next chunks to one free chunk

            chunk->size += nextChunk->size;
            chunk->next = nextChunk->next;
            chunk->next->prev = chunk;

            if ( nextChunk == MemoryBuffer->rover ) {
                MemoryBuffer->rover = chunk;
            }
        }

        return bytes;
    }
}

void FMemoryZone::Dealloc( void * _Bytes ) {
    if ( !MemoryBuffer ) {
        return;
    }

    if ( !_Bytes ) {
        return;
    }

    FSyncGuard syncGuard( Sync );

    byte * bytes = ( byte * )_Bytes;
    int padding = *(bytes - 1);

    chunk_t * chunk = ( chunk_t * )( bytes - ChunkHeaderLength - padding );

    if ( chunk->size > 0 ) {
        // freed pointer
        return;
    }

    if ( ChunkTrashTest( chunk ) ) {
        MemLogger.Print( "FMemoryZone::Dealloc: Warning: memory was trashed\n" );
        // error()
    }

    chunk->size = -chunk->size;

#ifdef ALLOW_ALLOCATE_ON_HEAP
    if ( chunk->next == (chunk_t *)0xdeaddead ) {

        // Was allocated on heap
        UnalignedFree( chunk );

        DecMemoryStatisticsOnHeap( chunk->size, ChunkHeaderLength + padding );

        return;
    }
#endif

    DecMemoryStatistics( chunk->size, ChunkHeaderLength + padding );

    chunk_t * prevChunk = chunk->prev;
    chunk_t * nextChunk = chunk->next;

    if ( prevChunk->size > 0 ) {
        // Merge prev and current chunks to one free chunk

        prevChunk->size += chunk->size;
        prevChunk->next = chunk->next;
        prevChunk->next->prev = prevChunk;

        if ( chunk == MemoryBuffer->rover ) {
            MemoryBuffer->rover = prevChunk;
        }
        chunk = prevChunk;
    }

    if ( nextChunk->size > 0 ) {
        // Merge current and next chunks to one free chunk

        chunk->size += nextChunk->size;
        chunk->next = nextChunk->next;
        chunk->next->prev = chunk;

        if ( nextChunk == MemoryBuffer->rover ) {
            MemoryBuffer->rover = chunk;
        }
    }
}

void FMemoryZone::CheckMemoryLeaks() {
    FSyncGuard syncGuard( Sync );

    if ( TotalMemoryUsage.Load() > 0 ) {
        chunk_t * rover = MemoryBuffer->rover;
        chunk_t * start = rover->prev;
        chunk_t * cur;

        do {
            cur = rover;
            if ( cur->size < 0 ) {
                MemLogger.Print( "==== Zone Memory Leak ====\n" );
                MemLogger.Printf( "Chunk Address: %u Size: %d\n", (size_t)( cur + 1 ), (-cur->size) );

                //int sz = -cur->size;
                //char * b = (char*)(cur+1);
                //while ( sz-->0 ) {
                //    MemLogger.Printf("%c",*b > 31 && *b < 127 ? *b : '?');
                //    b++;
                //}
            }

            if ( rover == start ) {
                break;
            }
            rover = rover->next;

        } while ( 1 );
    }
}

#endif


//////////////////////////////////////////////////////////////////////////////////////////
//
// Allocator
//
//////////////////////////////////////////////////////////////////////////////////////////

//#define HEAP_MEMORY

template< typename T >
static void PrintMemoryStatistics( T & _Manager, const char * _Message ) {
    MemLogger.Printf( "%s: MaxMemoryUsage %d TotalMemoryUsage %d TotalMemoryOverhead %d\n",
            _Message, _Manager.GetMaxMemoryUsage(), _Manager.GetTotalMemoryUsage(), _Manager.GetTotalMemoryOverhead() );
}

void * FAllocator::AllocateBytes( size_t _BytesCount, int _Alignment ) {
#ifdef HEAP_MEMORY
    void * bytes = GMainHeapMemory.HeapAlloc( _BytesCount, _Alignment );
    //PrintMemoryStatistics( GMainHeapMemory, "AllocateBytes" );
#else
    void * bytes = GMainMemoryZone.Alloc( _BytesCount, _Alignment );
    //PrintMemoryStatistics( GMainMemoryZone, "AllocateBytes" );
#endif
    return bytes;
}

void * FAllocator::ExtendAlloc( void * _Data, size_t _BytesCount, size_t _NewBytesCount, int _NewAlignment, bool _KeepOld ) {
#ifdef HEAP_MEMORY
    void * bytes;
    if ( !_Data ) {
        // first alloc
        bytes = GMainHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
    } else if ( (size_t)_Data % _NewAlignment != 0 ) {
        // alignment changed
        if ( _KeepOld ) {
            bytes = GMainHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
            memcpy( bytes, _Data, _BytesCount );
#ifdef CLEAR_ALLOCATED_MEMORY
            ClearMemory8( (byte*)bytes + _BytesCount, 0, _NewBytesCount - _BytesCount );
#endif
            GMainHeapMemory.Dealloc( _Data );
        } else {
            GMainHeapMemory.Dealloc( _Data );
            bytes = GMainHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
        }
    } else if ( _BytesCount >= _NewBytesCount ) {
        // big enough
        bytes = _Data;
    } else {
        // reallocate
        if ( _KeepOld ) {
            bytes = GMainHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
            memcpy( bytes, _Data, _BytesCount );
#ifdef CLEAR_ALLOCATED_MEMORY
            ClearMemory8( (byte*)bytes + _BytesCount, 0, _NewBytesCount - _BytesCount );
#endif
            GMainHeapMemory.Dealloc( _Data );
        } else {
            GMainHeapMemory.Dealloc( _Data );
            bytes = GMainHeapMemory.HeapAlloc( _NewBytesCount, _NewAlignment );
        }
    }
    //PrintMemoryStatistics( GMainHeapMemory, "ExtendAlloc" );
#else
    void * bytes = GMainMemoryZone.Extend( _Data, _BytesCount, _NewBytesCount, _NewAlignment, _KeepOld );
    //PrintMemoryStatistics( GMainMemoryZone, "ExtendAlloc" );
#endif
    return bytes;
}

void FAllocator::Dealloc( void * _Bytes ) {
#ifdef HEAP_MEMORY
    GMainHeapMemory.HeapFree( _Bytes );
    //PrintMemoryStatistics( GMainHeapMemory, "Dealloc" );
#else
    GMainMemoryZone.Dealloc( _Bytes );
    //PrintMemoryStatistics( GMainMemoryZone, "Dealloc" );
#endif
}
