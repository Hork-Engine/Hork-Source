/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <Core/Public/Alloc.h>
#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>

#include <malloc.h>
#include <memory.h>
#include <xmmintrin.h>

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

#define ENABLE_TRASH_TEST

#ifdef AN_MULTITHREADED_ALLOC
#define SYNC_GUARD ASyncGuard syncGuard( Sync );
#else
#define SYNC_GUARD
#endif

typedef uint16_t TRASH_MARKER;

static const TRASH_MARKER TrashMarker = 0xfeee;

static ALogger MemLogger;

//////////////////////////////////////////////////////////////////////////////////////////
//
// Heap Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

static AAtomicLong HeapTotalMemoryUsage( 0 );
static AAtomicLong HeapTotalMemoryOverhead( 0 );
static AAtomicLong HeapMaxMemoryUsage( 0 );

AN_FORCEINLINE void IncMemoryStatisticsOnHeap( size_t _MemoryUsage, size_t _Overhead ) {
    HeapTotalMemoryUsage.FetchAdd( _MemoryUsage );
    HeapTotalMemoryOverhead.FetchAdd( _Overhead );
    HeapMaxMemoryUsage.Store( StdMax( HeapMaxMemoryUsage.Load(), HeapTotalMemoryUsage.Load() ) );
}

AN_FORCEINLINE void DecMemoryStatisticsOnHeap( size_t _MemoryUsage, size_t _Overhead ) {
    HeapTotalMemoryUsage.FetchSub( _MemoryUsage );
    HeapTotalMemoryOverhead.FetchSub( _Overhead );
}

AHeapMemory::AHeapMemory() {
    HeapChain.pNext = HeapChain.pPrev = &HeapChain;

    AN_SIZEOF_STATIC_CHECK( SHeapChunk, 32 );
}

AHeapMemory::~AHeapMemory() {
}

void AHeapMemory::Initialize() {

}

void AHeapMemory::Deinitialize() {
    CheckMemoryLeaks();
    Clear();
}

void AHeapMemory::Clear() {
    SHeapChunk * nextHeap;
    for ( SHeapChunk * heap = HeapChain.pNext ; heap != &HeapChain ; heap = nextHeap ) {
        nextHeap = heap->pNext;
        Free( heap + 1 );
    }
}

AN_FORCEINLINE void * SysAlloc( size_t _SizeInBytes, int _Alignment ) {
#ifdef AN_COMPILER_MSVC
    return _aligned_malloc( _SizeInBytes, _Alignment );
#else
    return aligned_alloc( _Alignment, _SizeInBytes );
#endif
}

AN_FORCEINLINE void SysFree( void * _Bytes ) {
#ifdef AN_COMPILER_MSVC
    return _aligned_free( _Bytes );
#else
    return free( _Bytes );
#endif
}

void * AHeapMemory::Alloc( size_t _BytesCount, int _Alignment ) {
#if 0
    return SysAlloc( Align(_BytesCount,16), 16 );
#else
    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "AHeapMemory::Alloc: Invalid bytes count\n" );
    }

    size_t chunkSizeInBytes = _BytesCount + sizeof( SHeapChunk );

#ifdef ENABLE_TRASH_TEST
    chunkSizeInBytes += sizeof( TRASH_MARKER );
#endif

    byte * bytes;
    byte * aligned;

    AN_ASSERT( _Alignment <= 128 && IsPowerOfTwo( _Alignment ) );

    if ( _Alignment <= 16 ) {
        _Alignment = 16;
        chunkSizeInBytes = Align( chunkSizeInBytes, 16 );
        bytes = ( byte * )SysAlloc( chunkSizeInBytes, 16 );
        aligned = bytes + sizeof( SHeapChunk );
    } else {
        chunkSizeInBytes += _Alignment - 1;
        chunkSizeInBytes = Align( chunkSizeInBytes, sizeof(void *) );
        bytes = ( byte * )SysAlloc( chunkSizeInBytes, sizeof(void *) );
        aligned = ( byte * )AlignPtr( bytes + sizeof( SHeapChunk ), _Alignment );
    }

    AN_ASSERT( IsAlignedPtr( aligned, 16 ) );

    if ( bytes ) {
        SHeapChunk * heap = ( SHeapChunk * )( aligned ) - 1;
        heap->Size = chunkSizeInBytes;
        heap->DataSize = _BytesCount;
        heap->Alignment = _Alignment;
        heap->AlignOffset = aligned - bytes;

        {
            SYNC_GUARD
            heap->pNext = HeapChain.pNext;
            heap->pPrev = &HeapChain;
            HeapChain.pNext->pPrev = heap;
            HeapChain.pNext = heap;
        }

#ifdef ENABLE_TRASH_TEST
        *( TRASH_MARKER * )( aligned + heap->DataSize ) = TrashMarker;
#endif

        IncMemoryStatisticsOnHeap( heap->Size, heap->Size - heap->DataSize );

        return aligned;
    }

    CriticalError( "AHeapMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount );
    return nullptr;
#endif
}

void AHeapMemory::Free( void * _Bytes ) {
#if 0
    return SysFree( _Bytes );
#else
    if ( _Bytes ) {
        byte * bytes = ( byte * )_Bytes;

        SHeapChunk * heap = ( SHeapChunk * )( bytes ) - 1;

#ifdef ENABLE_TRASH_TEST
        if ( *( TRASH_MARKER * )( bytes + heap->DataSize ) != TrashMarker ) {
            MemLogger.Print( "AHeapMemory::HeapFree: Warning: memory was trashed\n" );
            // error()
        }
#endif

        bytes -= heap->AlignOffset;

        {
            SYNC_GUARD
            heap->pPrev->pNext = heap->pNext;
            heap->pNext->pPrev = heap->pPrev;
        }

        DecMemoryStatisticsOnHeap( heap->Size, heap->Size - heap->DataSize );

        SysFree( bytes );
    }
#endif
}

void * AHeapMemory::Realloc( void * _Data, int _NewBytesCount, bool _KeepOld ) {
#if 0
    void * bytes;
    if ( _KeepOld ) {
        bytes = SysAlloc( Align(_NewBytesCount,16), 16 );
        Core::Memcpy( bytes, _Data, _NewBytesCount );
        SysFree( _Data );
    } else {
        SysFree( _Data );
        bytes = SysAlloc( Align(_NewBytesCount,16), 16 );
    }
    return bytes;
#else
    if ( !_Data ) {
        // first alloc, use default alignment
        return Alloc( _NewBytesCount, 16 );
    }

    byte * bytes = ( byte * )_Data;

    SHeapChunk * heap = ( SHeapChunk * )( bytes ) - 1;

    int alignment = heap->Alignment;

#ifdef ENABLE_TRASH_TEST
    if ( *( TRASH_MARKER * )( bytes + heap->DataSize ) != TrashMarker ) {
        MemLogger.Print( "AHeapMemory::Realloc: Warning: memory was trashed\n" );
    }
#endif
    if ( heap->DataSize >= _NewBytesCount ) {
        // big enough
        return _Data;
    }
    // reallocate
    if ( _KeepOld ) {
        bytes = ( byte * )Alloc( _NewBytesCount, alignment );
        Core::MemcpySSE( bytes, _Data, heap->DataSize );
        Free( _Data );
        return bytes;
    }
    Free( _Data );
    return Alloc( _NewBytesCount, alignment );
#endif
}

void AHeapMemory::PointerTrashTest( void * _Bytes ) {
#ifdef ENABLE_TRASH_TEST
    byte * bytes = (byte *)_Bytes;

    SHeapChunk * heap = ( SHeapChunk * )( bytes )-1;

    if ( *( TRASH_MARKER * )( bytes + heap->DataSize ) != TrashMarker ) {
        MemLogger.Print( "AHeapMemory::PointerTrashTest: Warning: memory was trashed\n" );
    }
#endif
}

void AHeapMemory::CheckMemoryLeaks() {
    for ( SHeapChunk * heap = HeapChain.pNext ; heap != &HeapChain ; heap = heap->pNext ) {
        MemLogger.Print( "==== Heap Memory Leak ====\n" );
        MemLogger.Printf( "Heap Address: %u Size: %d\n", (size_t)( heap + 1 ), heap->DataSize );
    }
}

size_t AHeapMemory::GetTotalMemoryUsage() {
    return HeapTotalMemoryUsage.Load();
}

size_t AHeapMemory::GetTotalMemoryOverhead() {
    return HeapTotalMemoryOverhead.Load();
}

size_t AHeapMemory::GetMaxMemoryUsage() {
    return HeapMaxMemoryUsage.Load();
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Hunk Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

static const int MinHunkFragmentLength = 64; // Must be > sizeof( SHunk )

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
    int32_t Pad;
};

void * AHunkMemory::GetHunkMemoryAddress() const { return MemoryBuffer; }
int AHunkMemory::GetHunkMemorySizeInMegabytes() const { return MemoryBuffer ? MemoryBuffer->Size >> 10 >> 10 : 0; }
size_t AHunkMemory::GetTotalMemoryUsage() const { return TotalMemoryUsage; }
size_t AHunkMemory::GetTotalMemoryOverhead() const { return TotalMemoryOverhead; }
size_t AHunkMemory::GetTotalFreeMemory() const { return MemoryBuffer ? MemoryBuffer->Size - TotalMemoryUsage : 0; }
size_t AHunkMemory::GetMaxMemoryUsage() const { return MaxMemoryUsage; }

void AHunkMemory::Initialize( void * _MemoryAddress, int _SizeInMegabytes ) {
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

    AN_SIZEOF_STATIC_CHECK( SHunkMemory, 32 );
    AN_SIZEOF_STATIC_CHECK( SHunk, 16 );

    if ( !IsAlignedPtr( MemoryBuffer->Hunk, 16 ) ) {
        CriticalError( "AHunkMemory::Initialize: chunk mest be at 16 byte boundary\n" );
    }
}

void AHunkMemory::Deinitialize() {
    CheckMemoryLeaks();

    MemoryBuffer = nullptr;

    TotalMemoryUsage = 0;
    MaxMemoryUsage = 0;
    TotalMemoryOverhead = 0;
}

void AHunkMemory::Clear() {
    MemoryBuffer->Mark = 0;
    MemoryBuffer->Hunk = ( SHunk * )( MemoryBuffer + 1 );
    MemoryBuffer->Hunk->Size = MemoryBuffer->Size - sizeof( SHunkMemory );
    MemoryBuffer->Hunk->Mark = -1; // marked free
    MemoryBuffer->Hunk->pPrev = MemoryBuffer->Cur = nullptr;

    TotalMemoryUsage = 0;
    MaxMemoryUsage = 0;
    TotalMemoryOverhead = 0;
}

int AHunkMemory::SetHunkMark() {
    return ++MemoryBuffer->Mark;
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

void * AHunkMemory::Alloc( size_t _BytesCount ) {
    if ( !MemoryBuffer ) {
        CriticalError( "AHunkMemory::Alloc: Not initialized\n" );
    }

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "AHunkMemory::Alloc: Invalid bytes count\n" );
    }

    // check memory trash
    if ( MemoryBuffer->Cur && HunkTrashTest( MemoryBuffer->Cur ) ) {
        CriticalError( "AHunkMemory::Alloc: Memory was trashed\n" );
    }

    SHunk * hunk = MemoryBuffer->Hunk;

    if ( hunk->Mark != -1 ) {
        // not enough memory
        CriticalError( "AHunkMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount );
    }

    int requiredSize = _BytesCount;

    // Add chunk header
    requiredSize += sizeof( SHunk );

    // Add trash marker
#ifdef ENABLE_TRASH_TEST
    requiredSize += sizeof( TRASH_MARKER );
#endif

    // Align chunk to 16-byte boundary
    requiredSize = Align( requiredSize, 16 );

    if ( requiredSize > hunk->Size ) {
        // not enough memory
        CriticalError( "AHunkMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount );
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

    void * aligned = hunk + 1;

    AN_ASSERT( IsAlignedPtr( aligned, 16 ) );

    return aligned;
}

void AHunkMemory::ClearToMark( int _Mark ) {
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
        CriticalError( "AHunkMemory::ClearToMark: Memory was trashed\n" );
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
            CriticalError( "AHunkMemory::ClearToMark: Warning: memory was trashed\n" );
        }
        hunk->Size += grow;
        hunk->Mark = -1; // mark free
        MemoryBuffer->Hunk = hunk;
        grow = hunk->Size;
        MemoryBuffer->Cur = hunk = hunk->pPrev;
    }

    MemoryBuffer->Mark = _Mark;
}

void AHunkMemory::ClearLastHunk() {
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
            MemLogger.Print( "AHunkMemory::ClearLastHunk: Warning: memory was trashed\n" );
            // error()
        }
        hunk->Size += grow;
        hunk->Mark = -1; // mark free
        MemoryBuffer->Hunk = hunk;
        MemoryBuffer->Cur = hunk->pPrev;
    }
}

void AHunkMemory::CheckMemoryLeaks() {
    if ( TotalMemoryUsage > 0 ) {
        // check memory trash
        if ( MemoryBuffer->Cur && HunkTrashTest( MemoryBuffer->Cur ) ) {
            MemLogger.Print( "AHunkMemory::CheckMemoryLeaks: Memory was trashed\n" );
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

void AHunkMemory::IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    TotalMemoryUsage += _MemoryUsage;
    TotalMemoryOverhead += _Overhead;
    MaxMemoryUsage = StdMax( MaxMemoryUsage, TotalMemoryUsage );
}

void AHunkMemory::DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    TotalMemoryUsage -= _MemoryUsage;
    TotalMemoryOverhead -= _Overhead;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Zone Memory
//
//////////////////////////////////////////////////////////////////////////////////////////

struct SZoneChunk {
    SZoneChunk * pNext; // NOTE: We can remove next, becouse next = (SZoneChunk*)(((byte*)chunk) + chunk->size)
    SZoneChunk * pPrev;
    int32_t Size;
    int32_t DataSize;
    byte Pad[8];
};

struct SZoneBuffer {
    SZoneChunk * Rover;
    SZoneChunk ChunkList;
    int32_t Size;
    byte Pad[16];
};

AN_SIZEOF_STATIC_CHECK( SZoneChunk, 32 );
AN_SIZEOF_STATIC_CHECK( SZoneBuffer, 64 );

static const int ChunkHeaderLength = sizeof( SZoneChunk );
static const int MinZoneFragmentLength = 64; // Must be > ChunkHeaderLength

AN_FORCEINLINE size_t AdjustChunkSize( size_t _BytesCount ) {

    // Add chunk header
    _BytesCount += ChunkHeaderLength;

    // Add trash marker
#ifdef ENABLE_TRASH_TEST
    _BytesCount += sizeof( TRASH_MARKER );
#endif

    // Align chunk to 16-byte boundary
    _BytesCount = Align( _BytesCount, 16 );

    return _BytesCount;
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

void * AZoneMemory::GetZoneMemoryAddress() const {
    return MemoryBuffer;
}

int AZoneMemory::GetZoneMemorySizeInMegabytes() const {
    return MemoryBuffer ? MemoryBuffer->Size >> 10 >> 10 : 0;
}

size_t AZoneMemory::GetTotalMemoryUsage() const {
    return TotalMemoryUsage.Load();
}

size_t AZoneMemory::GetTotalMemoryOverhead() const {
    return TotalMemoryOverhead.Load();
}

size_t AZoneMemory::GetTotalFreeMemory() const {
    return MemoryBuffer ? MemoryBuffer->Size - TotalMemoryUsage.Load() : 0;
}

size_t AZoneMemory::GetMaxMemoryUsage() const {
    return MaxMemoryUsage.Load();
}

void AZoneMemory::Initialize( void * _MemoryAddress, int _SizeInMegabytes ) {
    size_t sizeInBytes = _SizeInMegabytes << 20;

    MemoryBuffer = ( SZoneBuffer * )_MemoryAddress;
    MemoryBuffer->Size = sizeInBytes;
    MemoryBuffer->ChunkList.pPrev = MemoryBuffer->ChunkList.pNext = MemoryBuffer->Rover = ( SZoneChunk * )( MemoryBuffer + 1 );
    MemoryBuffer->ChunkList.Size = 0;
    MemoryBuffer->Rover->Size = MemoryBuffer->Size - sizeof( SZoneBuffer );
    MemoryBuffer->Rover->pNext = &MemoryBuffer->ChunkList;
    MemoryBuffer->Rover->pPrev = &MemoryBuffer->ChunkList;

    if ( !IsAlignedPtr( MemoryBuffer->Rover, 16 ) ) {
        CriticalError( "AZoneMemory::Initialize: chunk mest be at 16 byte boundary\n" );
    }

    TotalMemoryUsage.StoreRelaxed( 0 );
    TotalMemoryOverhead.StoreRelaxed( 0 );
    MaxMemoryUsage.StoreRelaxed( 0 );
}

void AZoneMemory::Deinitialize() {
    CheckMemoryLeaks();

    MemoryBuffer = nullptr;

    TotalMemoryUsage.StoreRelaxed( 0 );
    TotalMemoryOverhead.StoreRelaxed( 0 );
    MaxMemoryUsage.StoreRelaxed( 0 );
}

void AZoneMemory::Clear() {
    if ( !MemoryBuffer ) {
        return;
    }

    SYNC_GUARD

    MemoryBuffer->ChunkList.pPrev = MemoryBuffer->ChunkList.pNext = MemoryBuffer->Rover = ( SZoneChunk * )( MemoryBuffer + 1 );
    MemoryBuffer->ChunkList.Size = 0;
    MemoryBuffer->Rover->Size = MemoryBuffer->Size - sizeof( SZoneBuffer );
    MemoryBuffer->Rover->pNext = &MemoryBuffer->ChunkList;
    MemoryBuffer->Rover->pPrev = &MemoryBuffer->ChunkList;

    TotalMemoryUsage.Store( 0 );
    TotalMemoryOverhead.Store( 0 );
    MaxMemoryUsage.Store( 0 );

    // Allocated "on heap" memory is still present
}

void AZoneMemory::IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    TotalMemoryUsage.FetchAdd( _MemoryUsage );
    TotalMemoryOverhead.FetchAdd( _Overhead );
    MaxMemoryUsage.Store( StdMax( MaxMemoryUsage.Load(), TotalMemoryUsage.Load() ) );
}

void AZoneMemory::DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead ) {
    TotalMemoryUsage.FetchSub( _MemoryUsage );
    TotalMemoryOverhead.FetchSub( _Overhead );
}

SZoneChunk * AZoneMemory::FindFreeChunk( int _RequiredSize ) {
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

void * AZoneMemory::Alloc( size_t _BytesCount ) {
#if 0
    return SysAlloc( Align(_BytesCount,16), 16 );
#else
    if ( !MemoryBuffer ) {
        CriticalError( "AZoneMemory::Alloc: Not initialized\n" );
    }

    if ( _BytesCount == 0 ) {
        // invalid bytes count
        CriticalError( "AZoneMemory::Alloc: Invalid bytes count\n" );
    }

    SYNC_GUARD

    size_t requiredSize = AdjustChunkSize( _BytesCount );

    SZoneChunk * cur = FindFreeChunk( requiredSize );
    if ( !cur ) {
        // no free chunks
        CriticalError( "AZoneMemory::Alloc: Failed on allocation of %u bytes\n", _BytesCount );
    }

    int recidualChunkSpace = cur->Size - requiredSize;
    if ( recidualChunkSpace >= MinZoneFragmentLength ) { // don't allow to create very small chunks
        SZoneChunk * newChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
        AN_ASSERT( IsAlignedPtr( newChunk, 16 ) );
        newChunk->Size = recidualChunkSpace;
        newChunk->pPrev = cur;
        newChunk->pNext = cur->pNext;
        newChunk->pNext->pPrev = newChunk;
        cur->pNext = newChunk;
        cur->Size = requiredSize;
    }

    byte * pointer = ( byte * )( cur + 1 );

    AN_ASSERT( IsAlignedPtr( cur, 16 ) );
    AN_ASSERT( IsAlignedPtr( pointer, 16 ) );

    IncMemoryStatistics( cur->Size, cur->Size - _BytesCount );

    cur->Size = -cur->Size; // Set size to negative to mark chunk used
    cur->DataSize = _BytesCount;
    MemoryBuffer->Rover = cur->pNext;

    SetTrashMarker( cur );

    return pointer;
#endif
}
//#pragma warning(disable:4702)
void * AZoneMemory::Realloc( void * _Data, int _NewBytesCount, bool _KeepOld ) {
#if 1
    if ( !_Data ) {
        return Alloc( _NewBytesCount );
    }

    SZoneChunk * chunk = ( SZoneChunk * )( _Data ) - 1;

    if ( chunk->DataSize >= _NewBytesCount ) {
        // data is big enough
        //MemLogger.Printf( "_BytesCount >= _NewBytesCount\n" );
        return _Data;
    }

    // Check freed
    bool bFreed = chunk->Size > 0;

    if ( bFreed ) {
        //MemLogger.Printf( "freed pointer\n" );
        return Alloc( _NewBytesCount );
    }

    if ( !_KeepOld ) {
        Free( _Data );
        return Alloc( _NewBytesCount );
    }

    int sz = chunk->DataSize;
    void * temp = GHunkMemory.Alloc( sz );
    Core::MemcpySSE( temp, _Data, sz );
    Free( _Data );
    void * pNewData = Alloc( _NewBytesCount );
    if ( pNewData != _Data ) {
        Core::MemcpySSE( pNewData, temp, sz );
    } else {
        //MemLogger.Printf( "Caching memcpy\n" );
    }
    GHunkMemory.ClearLastHunk();

    return pNewData;

#else

#if 0
    return SysRealloc( _Data, _NewBytesCount, 16 );
#else
    if ( !_Data ) {
        return Alloc( _NewBytesCount );
    }

    SZoneChunk * chunk = ( SZoneChunk * )( _Data ) - 1;

    if ( chunk->DataSize >= _NewBytesCount ) {
        // data is big enough
        //MemLogger.Printf( "_BytesCount >= _NewBytesCount\n" );
        return _Data;
    }

    // Check freed
    bool bFreed = chunk->Size > 0;

    if ( bFreed ) {
        //MemLogger.Printf( "freed pointer\n" );
        return Alloc( _NewBytesCount );
    }

    SYNC_GUARD

    size_t oldSize = chunk->DataSize;

    if ( ChunkTrashTest( chunk ) ) {
        MemLogger.Print( "AZoneMemory::Realloc: Warning: memory was trashed\n" );
        // error()
    }

    int requiredSize = AdjustChunkSize( _NewBytesCount );
    if ( requiredSize <= -chunk->Size ) {
        // data is big enough
        //MemLogger.Printf( "data is big enough\n" );
        return _Data;
    }

    // restore positive chunk size
    chunk->Size = -chunk->Size;

    AN_ASSERT( chunk->Size > 0 );

    // try to use next chunk
    SZoneChunk * cur = chunk->pNext;
    if ( cur->Size > 0 && cur->Size + chunk->Size >= requiredSize ) {

#if 1
        requiredSize -= chunk->Size;
        int recidualChunkSpace = cur->Size - requiredSize;
        if ( recidualChunkSpace >= MinZoneFragmentLength ) {
#if 0
            SZoneChunk * newChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
            AN_ASSERT( IsAlignedPtr( newChunk, 16 ) );
            newChunk->Size = recidualChunkSpace;
            newChunk->pPrev = chunk;
            newChunk->pNext = cur->pNext;
            newChunk->pNext->pPrev = newChunk;
            chunk->pNext = newChunk;
            chunk->Size += requiredSize;
            IncMemoryStatistics( requiredSize, requiredSize - _NewBytesCount );
#else
            chunk->Size = -chunk->Size;
            void * d = malloc( _NewBytesCount );
            Core::Memcpy( d, _Data, oldSize );
            Free( _Data );
            _Data = Alloc( _NewBytesCount );
            Core::Memcpy( _Data, d, _NewBytesCount );
            free( d );
            return _Data;
#endif
        } else {
#if 1
            chunk->pNext = cur->pNext;
            chunk->pNext->pPrev = chunk;
            chunk->Size += cur->Size;
            IncMemoryStatistics( cur->Size, cur->Size - _NewBytesCount );

            //Core::Memset( (byte *)_Data + chunk->DataSize, 0, _NewBytesCount - chunk->DataSize );
#else
//            chunk->Size = -chunk->Size;
//            void * d = malloc( oldSize );
//            Core::Memcpy( d, _Data, oldSize );
//            Free( _Data );
//            _Data = Alloc( _NewBytesCount );
//            Core::Memcpy( _Data, d, oldSize );
//            free( d );
//            return _Data;
            chunk->Size = -chunk->Size;
            void * d = malloc( _NewBytesCount );
            Core::Memcpy( d, _Data, oldSize );
            Free( _Data );
            _Data = Alloc( _NewBytesCount );
            Core::Memcpy( _Data, d, oldSize );
            free( d );
            return _Data;
#endif
        }

        // Set size to negative to mark chunk used
        chunk->Size = -chunk->Size;
        chunk->DataSize = _NewBytesCount;
        MemoryBuffer->Rover = chunk->pNext;

        SetTrashMarker( chunk );

        //MemLogger.Printf( "using next chunk\n" );

        AN_ASSERT( _Data == chunk+1 );

        return _Data;
#else
        chunk->Size = -chunk->Size;
        void * d = malloc( _NewBytesCount );
        Core::Memcpy( d, _Data, oldSize );
        Free( _Data );
        _Data = Alloc( _NewBytesCount );
        Core::Memcpy( _Data, d, _NewBytesCount );
        free( d );
        return _Data;
#endif
    } else {

        //chunk->Size = -chunk->Size;
        //void * d = malloc( _NewBytesCount );
        //Core::Memcpy(d,_Data, oldSize );
        //Free(_Data);
        //_Data=Alloc(_NewBytesCount);
        //Core::Memcpy(_Data,d,_NewBytesCount);
        //free(d);
        //return _Data;
#if 1
        //MemLogger.Printf( "reallocating new chunk with copy\n" );

        cur = FindFreeChunk( requiredSize );
        if ( !cur ) {

            chunk->Size = -chunk->Size;

            // no free chunks

            CriticalError( "AZoneMemory::Realloc: Failed on allocation of %u bytes\n", _NewBytesCount );
        }

        DecMemoryStatistics( chunk->Size, chunk->Size - chunk->DataSize );

        int recidualChunkSpace = cur->Size - requiredSize;
        if ( recidualChunkSpace >= MinZoneFragmentLength ) {
            SZoneChunk * newChunk = ( SZoneChunk * )( ( byte * )( cur ) + requiredSize );
            AN_ASSERT( IsAlignedPtr( newChunk, 16 ) );
            newChunk->Size = recidualChunkSpace;
            newChunk->pPrev = cur;
            newChunk->pNext = cur->pNext;
            newChunk->pNext->pPrev = newChunk;
            cur->pNext = newChunk;
            cur->Size = requiredSize;
        }

        byte * pointer = ( byte * )( cur + 1 );

        AN_ASSERT( IsAlignedPtr( cur, 16 ) );
        AN_ASSERT( IsAlignedPtr( pointer, 16 ) );

        IncMemoryStatistics( cur->Size, cur->Size - _NewBytesCount );

        cur->Size = -cur->Size; // Set size to negative to mark chunk used
        cur->DataSize = _NewBytesCount;
        MemoryBuffer->Rover = cur->pNext;

        SetTrashMarker( cur );

        if ( _KeepOld ) {
            Core::MemcpySSE( pointer, _Data, oldSize );
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

        return pointer;
#endif
    }
#endif

#endif
}

void AZoneMemory::Free( void * _Bytes ) {
#if 0
    SysFree( _Bytes );
#else
    if ( !MemoryBuffer ) {
        return;
    }

    if ( !_Bytes ) {
        return;
    }

    SYNC_GUARD

    SZoneChunk * chunk = ( SZoneChunk * )( _Bytes ) - 1;

    if ( chunk->Size > 0 ) {
        // freed pointer
        return;
    }

    if ( ChunkTrashTest( chunk ) ) {
        MemLogger.Print( "AZoneMemory::Free: Warning: memory was trashed\n" );
        // error()
    }

    chunk->Size = -chunk->Size;

    DecMemoryStatistics( chunk->Size, chunk->Size - chunk->DataSize );

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
#endif
}

void AZoneMemory::CheckMemoryLeaks() {
    SYNC_GUARD

    if ( TotalMemoryUsage.Load() > 0 ) {
        SZoneChunk * rover = MemoryBuffer->Rover;
        SZoneChunk * start = rover->pPrev;
        SZoneChunk * cur;

        do {
            cur = rover;
            if ( cur->Size < 0 ) {
                MemLogger.Print( "==== Zone Memory Leak ====\n" );
                MemLogger.Printf( "Chunk Address: %u (Local: %u) Size: %d\n", (size_t)( cur + 1 ), (size_t)( cur + 1 ) - (size_t)GetZoneMemoryAddress(), (-cur->Size) );

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

void * AZoneAllocator::ImplAlloc( size_t _BytesCount ) {
    void * bytes = GZoneMemory.Alloc( _BytesCount );
    //PrintMemoryStatistics( GZoneMemory, "Alloc" );
    return bytes;
}

void * AZoneAllocator::ImplRealloc( void * _Data, size_t _NewBytesCount, bool _KeepOld ) {
    void * bytes = GZoneMemory.Realloc( _Data, _NewBytesCount, _KeepOld );
    //PrintMemoryStatistics( GZoneMemory, "Realloc" );
    return bytes;
}

void AZoneAllocator::ImplFree( void * _Bytes ) {
    GZoneMemory.Free( _Bytes );
    //PrintMemoryStatistics( GZoneMemory, "Free" );
}

//
// Heap allocator
//

void * AHeapAllocator::ImplAlloc( size_t _BytesCount ) {
    void * bytes = GHeapMemory.Alloc( _BytesCount, 16 );
    //PrintMemoryStatistics( GHeapMemory, "Alloc" );
    return bytes;
}

void * AHeapAllocator::ImplRealloc( void * _Data, size_t _NewBytesCount, bool _KeepOld ) {
    void * bytes = GHeapMemory.Realloc( _Data, _NewBytesCount, _KeepOld );
    //PrintMemoryStatistics( GHeapMemory, "Realloc" );
    return bytes;
}

void AHeapAllocator::ImplFree( void * _Bytes ) {
    GHeapMemory.Free( _Bytes );
    //PrintMemoryStatistics( GHeapMemory, "Free" );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Utilites
//
//////////////////////////////////////////////////////////////////////////////////////////

namespace Core {

void _MemcpySSE( byte * _Dst, const byte * _Src, size_t _SizeInBytes ) {
#if 0
    memcpy(_Dst,_Src,_SizeInBytes);
#else
    AN_ASSERT( IsSSEAligned( (size_t)_Dst ) );
    AN_ASSERT( IsSSEAligned( (size_t)_Src ) );

    int n = 0;

    while ( n + 256 <= _SizeInBytes ) {
        __m128i d0 = _mm_load_si128( (__m128i *)&_Src[n + 0*16] );
        __m128i d1 = _mm_load_si128( (__m128i *)&_Src[n + 1*16] );
        __m128i d2 = _mm_load_si128( (__m128i *)&_Src[n + 2*16] );
        __m128i d3 = _mm_load_si128( (__m128i *)&_Src[n + 3*16] );
        __m128i d4 = _mm_load_si128( (__m128i *)&_Src[n + 4*16] );
        __m128i d5 = _mm_load_si128( (__m128i *)&_Src[n + 5*16] );
        __m128i d6 = _mm_load_si128( (__m128i *)&_Src[n + 6*16] );
        __m128i d7 = _mm_load_si128( (__m128i *)&_Src[n + 7*16] );
        _mm_stream_si128( (__m128i *)&_Dst[n + 0*16], d0 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 1*16], d1 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 2*16], d2 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 3*16], d3 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 4*16], d4 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 5*16], d5 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 6*16], d6 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 7*16], d7 );
        d0 = _mm_load_si128( (__m128i *)&_Src[n +  8*16] );
        d1 = _mm_load_si128( (__m128i *)&_Src[n +  9*16] );
        d2 = _mm_load_si128( (__m128i *)&_Src[n + 10*16] );
        d3 = _mm_load_si128( (__m128i *)&_Src[n + 11*16] );
        d4 = _mm_load_si128( (__m128i *)&_Src[n + 12*16] );
        d5 = _mm_load_si128( (__m128i *)&_Src[n + 13*16] );
        d6 = _mm_load_si128( (__m128i *)&_Src[n + 14*16] );
        d7 = _mm_load_si128( (__m128i *)&_Src[n + 15*16] );
        _mm_stream_si128( (__m128i *)&_Dst[n +  8*16], d0 );
        _mm_stream_si128( (__m128i *)&_Dst[n +  9*16], d1 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 10*16], d2 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 11*16], d3 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 12*16], d4 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 13*16], d5 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 14*16], d6 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 15*16], d7 );
        n += 256;
    }

    while ( n + 128 <= _SizeInBytes ) {
        __m128i d0 = _mm_load_si128( (__m128i *)&_Src[n + 0*16] );
        __m128i d1 = _mm_load_si128( (__m128i *)&_Src[n + 1*16] );
        __m128i d2 = _mm_load_si128( (__m128i *)&_Src[n + 2*16] );
        __m128i d3 = _mm_load_si128( (__m128i *)&_Src[n + 3*16] );
        __m128i d4 = _mm_load_si128( (__m128i *)&_Src[n + 4*16] );
        __m128i d5 = _mm_load_si128( (__m128i *)&_Src[n + 5*16] );
        __m128i d6 = _mm_load_si128( (__m128i *)&_Src[n + 6*16] );
        __m128i d7 = _mm_load_si128( (__m128i *)&_Src[n + 7*16] );
        _mm_stream_si128( (__m128i *)&_Dst[n + 0*16], d0 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 1*16], d1 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 2*16], d2 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 3*16], d3 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 4*16], d4 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 5*16], d5 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 6*16], d6 );
        _mm_stream_si128( (__m128i *)&_Dst[n + 7*16], d7 );
        n += 128;
    }

    while ( n + 16 <= _SizeInBytes ) {
        __m128i d0 = _mm_load_si128( (__m128i *)&_Src[n] );
        _mm_stream_si128( (__m128i *)&_Dst[n], d0 );
        n += 16;
    }

    while ( n + 4 <= _SizeInBytes ) {
        *(uint32_t *)&_Dst[n] = *(const uint32_t *)&_Src[n];
        n += 4;
    }

    while ( n < _SizeInBytes ) {
        _Dst[n] = _Src[n];
        n++;
    }

    _mm_sfence();
#endif
}

void _ZeroMemSSE( byte * _Dst, size_t _SizeInBytes ) {
#if 0
    ZeroMem( _Dst, _SizeInBytes );
#else
    AN_ASSERT( IsSSEAligned( (size_t)_Dst ) );

    int n = 0;

    __m128i zero = _mm_setzero_si128();

    while ( n + 256 <= _SizeInBytes ) {
        _mm_stream_si128( (__m128i *)&_Dst[n + 0*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 1*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 2*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 3*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 4*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 5*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 6*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 7*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 8*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 9*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 10*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 11*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 12*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 13*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 14*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 15*16], zero );
        n += 256;
    }

    while ( n + 128 <= _SizeInBytes ) {
        _mm_stream_si128( (__m128i *)&_Dst[n + 0*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 1*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 2*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 3*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 4*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 5*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 6*16], zero );
        _mm_stream_si128( (__m128i *)&_Dst[n + 7*16], zero );
        n += 128;
    }

    while ( n + 16 <= _SizeInBytes ) {
        _mm_stream_si128( (__m128i *)&_Dst[n], zero );
        n += 16;
    }

    while ( n + 4 <= _SizeInBytes ) {
        *(uint32_t *)&_Dst[n] = 0;
        n += 4;
    }

    while ( n < _SizeInBytes ) {
        _Dst[n] = 0;
        n++;
    }

    _mm_sfence();
#endif
}

void _MemsetSSE( byte * _Dst, int _Val, size_t _SizeInBytes ) {
    AN_ASSERT( IsSSEAligned( (size_t)_Dst ) );

#if 0
    Memset( _Dst, _Val, _SizeInBytes );
#else

    alignas(16) int32_t data[4];

    memset( data, _Val, sizeof( data ) );

    __m128i val = _mm_load_si128( (__m128i *)&data[0] );

    int n = 0;

    while ( n + 256 <= _SizeInBytes ) {
        _mm_stream_si128( (__m128i *)&_Dst[n + 0*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 1*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 2*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 3*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 4*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 5*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 6*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 7*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 8*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 9*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 10*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 11*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 12*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 13*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 14*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 15*16], val );
        n += 256;
    }

    while ( n + 128 <= _SizeInBytes ) {
        _mm_stream_si128( (__m128i *)&_Dst[n + 0*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 1*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 2*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 3*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 4*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 5*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 6*16], val );
        _mm_stream_si128( (__m128i *)&_Dst[n + 7*16], val );
        n += 128;
    }

    while ( n + 16 <= _SizeInBytes ) {
        _mm_stream_si128( (__m128i *)&_Dst[n], val );
        n += 16;
    }

    while ( n + 4 <= _SizeInBytes ) {
        *(uint32_t *)&_Dst[n] = data[0];
        n += 4;
    }

    while ( n < _SizeInBytes ) {
        _Dst[n] = data[0];
        n++;
    }

    _mm_sfence();

#endif
}

}
