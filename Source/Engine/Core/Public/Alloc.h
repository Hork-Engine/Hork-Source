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

#pragma once

#include "Atomic.h"
#include "Thread.h"

/*

Memory utilites

*/

// Clear memory with size aligned to 8 byte block length
AN_FORCEINLINE void ClearMemory8( void * _Buffer, uint64_t _ClearValue, size_t _Size ) {
    size_t count = ( ( _Size + 7 ) & ~7 ) >> 3;
    uint64_t * data = ( uint64_t * )_Buffer;
    while ( count-- ) {
        *data++ = _ClearValue;
    }
}

// Clear memory with size aligned to 8 byte block length
AN_FORCEINLINE void ZeroMemory8( void * _Buffer, size_t _Size ) {
    size_t count = ( ( _Size + 7 ) & ~7 ) >> 3;
    uint64_t * data = ( uint64_t * )_Buffer;
    while ( count-- ) {
        *data++ = 0;
    }
}

// Built-in memset function replacement
AN_FORCEINLINE void Memset( void * d, int v, size_t sz ) {
    byte * p = (byte*)d;
    while ( sz-- ) {
        *p++ = v;
    }
}

// Built-in memset function replacement
AN_FORCEINLINE void ZeroMem( void * d, size_t sz ) {
    byte * p = (byte*)d;
    while ( sz-- ) {
        *p++ = 0;
    }
}

/*

FHeapMemory

Allocates memory directly on heap

*/
struct heap_t {
    uint32_t size;
    heap_t * next;
    heap_t * prev;
    int16_t padding; // to keep heap aligned
};
class ANGIE_API FHeapMemory final {
    AN_FORBID_COPY( FHeapMemory )

public:
    FHeapMemory();
    ~FHeapMemory();

    // Initialize memory
    void Initialize();

    // Deinitialize memory
    void Deinitialize();

//    // Heap memory allocation
//    void * HeapAlloc( size_t _BytesCount ) { return _HeapAllocAligned( _BytesCount, 1 ); }

//    // Heap memory allocation
//    void * HeapAlloc16( size_t _BytesCount ) { return _HeapAllocAligned( _BytesCount, 16 ); }

//    // Heap memory allocation
//    void * HeapAlloc32( size_t _BytesCount ) { return _HeapAllocAligned( _BytesCount, 32 ); }

//    // Heap memory allocation
//    template< int Alignment >
//    void * HeapAllocAligned( size_t _BytesCount ) {
//        static_assert( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "FHeapMemory" );
//        return _HeapAllocAligned( _BytesCount, Alignment );
//    }

    // Heap memory allocation
    void * HeapAlloc( size_t _BytesCount, int _Alignment );

    // Heap memory allocation
    void * HeapAllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue = 0 );

    // Heap memory deallocation
    void HeapFree( void * _Bytes );

    // Clearing whole heap memory
    void Clear();

    // Statistics: current memory usage
    static size_t GetTotalMemoryUsage();

    // Statistics: current memory usage overhead
    static size_t GetTotalMemoryOverhead();

    // Statistics: max memory usage during memory using
    static size_t GetMaxMemoryUsage();

private:
    void CheckMemoryLeaks();

    heap_t HeapChain;

    // TODO: FThreadSync Sync
};

AN_FORCEINLINE void * FHeapMemory::HeapAllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue ) {
    void * bytes = HeapAlloc( _BytesCount, _Alignment );
    ClearMemory8( bytes, _ClearValue, _BytesCount );
    return bytes;
}

/*

FHunkMemory

For large temporary blocks like textures, meshes, etc

Example:

int Mark = HunkMemory.SetHunkMark();

byte * buffer1 = HunkMemory.HunkMemory( bufferSize1 );
byte * buffer2 = HunkMemory.HunkMemory( bufferSize2 );
...

HunkMemory.ClearToMark( Mark ); // here all buffers allocated after SetHunkMark will be deallocated

*/
class ANGIE_API FHunkMemory final {
    AN_FORBID_COPY( FHunkMemory )

public:
    FHunkMemory() {}

    // Initialize memory
    void Initialize( void * _MemoryAddress, int _SizeInMegabytes );

    // Deinitialize memory
    void Deinitialize();

    void * GetHunkMemoryAddress() const;
    int GetHunkMemorySizeInMegabytes() const;

//    // Hunk memory allocation
//    void * HunkMemory( size_t _BytesCount ) { return _HunkMemory( _BytesCount, 1 ); }

//    // Hunk memory allocation
//    void * HunkMemory16( size_t _BytesCount ) { return _HunkMemory( _BytesCount, 16 ); }

//    // Hunk memory allocation
//    void * HunkMemory32( size_t _BytesCount ) { return _HunkMemory( _BytesCount, 32 ); }

//    // Heap memory allocation
//    template< int Alignment >
//    void * HunkMemoryAligned( size_t _BytesCount ) {
//        static_assert( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "FHunkMemory" );
//        return _HunkMemory( _BytesCount, Alignment );
//    }

    // Hunk memory allocation
    void * HunkMemory( size_t _BytesCount, int _Alignment );

    void * HunkMemoryCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue = 0 );

    // Memory control functions
    int SetHunkMark();
    void ClearToMark( int _Mark );
    void Clear();

    // Clear last allocated hunk
    void ClearLastHunk();

    // Statistics: current memory usage
    size_t GetTotalMemoryUsage() const;

    // Statistics: current memory usage overhead
    size_t GetTotalMemoryOverhead() const;

    // Statistics: current free memory
    size_t GetTotalFreeMemory() const;

    // Statistics: max memory usage during memory using
    size_t GetMaxMemoryUsage() const;

private:
    void CheckMemoryLeaks();

    void IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );
    void DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );

    struct hunkMemory_t * MemoryBuffer = nullptr;

    size_t TotalMemoryUsage = 0;
    size_t TotalMemoryOverhead = 0;
    size_t MaxMemoryUsage = 0;
};

AN_FORCEINLINE void * FHunkMemory::HunkMemoryCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue ) {
    void * bytes = HunkMemory( _BytesCount, _Alignment );
    ClearMemory8( bytes, _ClearValue, _BytesCount );
    return bytes;
}

/*

FMemoryZone

For small blocks, objects or strings

*/
class ANGIE_API FMemoryZone final {
    AN_FORBID_COPY( FMemoryZone )

public:
    FMemoryZone() {}

    // Initialize memory
    void Initialize( void * _MemoryAddress, int _SizeInMegabytes );

    // Deinitialize memory
    void Deinitialize();

    void * GetZoneMemoryAddress() const;
    int GetZoneMemorySizeInMegabytes() const;

    // Zone memory allocation
    void * Alloc( size_t _BytesCount, int _Alignment );

    void * AllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue = 0 );

    // Tries to extend existing buffer
    void * Extend( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld );

    void * ExtendCleared( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld, uint64_t _ClearValue = 0 );

    // Zone memory deallocation
    void Dealloc( void * _Bytes );

    // Clearing whole zone memory
    void Clear();

    // Statistics: current memory usage
    size_t GetTotalMemoryUsage() const;

    // Statistics: current memory usage overhead
    size_t GetTotalMemoryOverhead() const;

    // Statistics: current free memory
    size_t GetTotalFreeMemory() const;

    // Statistics: max memory usage during memory using
    size_t GetMaxMemoryUsage() const;

private:
    struct chunk_t * FindFreeChunk( int _RequiredSize );

    void CheckMemoryLeaks();

    void IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );
    void DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );

    struct memoryBuffer_t * MemoryBuffer = nullptr;

    FAtomicLong TotalMemoryUsage;
    FAtomicLong TotalMemoryOverhead;
    FAtomicLong MaxMemoryUsage;

    FThreadSync Sync;
};

AN_FORCEINLINE void * FMemoryZone::AllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue ) {
    void * bytes = Alloc( _BytesCount, _Alignment );
    ClearMemory8( bytes, _ClearValue, _BytesCount );
    return bytes;
}

AN_FORCEINLINE void * FMemoryZone::ExtendCleared( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld, uint64_t _ClearValue ) {
    void * bytes = Extend( _Data, _BytesCount, _NewBytesCount, _NewAlignment, _KeepOld );
    if ( _KeepOld ) {
        if ( _NewBytesCount > _BytesCount ) {
            ClearMemory8( ( byte * )bytes + _BytesCount, _ClearValue, _NewBytesCount - _BytesCount );
        }
    } else {
        ClearMemory8( bytes, _ClearValue, _NewBytesCount );
    }
    return bytes;
}

/*

FAllocator

Objects allocator

*/
class ANGIE_API FAllocator final {
    AN_FORBID_COPY( FAllocator )

public:
    template< int Alignment >
    static void * Alloc( size_t _BytesCount ) {
        static_assert( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "FAllocator" );
        return AllocateBytes( _BytesCount, Alignment );
    }

    static void * Alloc1( size_t _BytesCount ) { return Alloc< 1 >( _BytesCount ); }
    static void * Alloc16( size_t _BytesCount ) { return Alloc< 16 >( _BytesCount ); }
    static void * Alloc32( size_t _BytesCount ) { return Alloc< 32 >( _BytesCount ); }

    template< int Alignment >
    static void * AllocCleared( size_t _BytesCount, uint64_t _ClearValue = 0 ) {
        static_assert( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "FAllocator" );
        void * bytes = AllocateBytes( _BytesCount, Alignment );
        ClearMemory8( bytes, _ClearValue, _BytesCount );
        return bytes;
    }

    static void * AllocCleared1( size_t _BytesCount ) { return AllocCleared< 1 >( _BytesCount ); }
    static void * AllocCleared16( size_t _BytesCount ) { return AllocCleared< 16 >( _BytesCount ); }
    static void * AllocCleared32( size_t _BytesCount ) { return AllocCleared< 32 >( _BytesCount ); }

    template< int Alignment >
    static void * Extend( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld ) {
        static_assert( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "FAllocator" );
        return ExtendAlloc( _Data, _BytesCount, _NewBytesCount, Alignment, _KeepOld );
    }

    static void * Extend1( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld ) { return Extend< 1 >( _Data, _BytesCount, _NewBytesCount, _KeepOld ); }
    static void * Extend16( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld ) { return Extend< 16 >( _Data, _BytesCount, _NewBytesCount, _KeepOld ); }
    static void * Extend32( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld ) { return Extend< 32 >( _Data, _BytesCount, _NewBytesCount, _KeepOld ); }

    template< int Alignment >
    static void * ExtendCleared( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue = 0 ) {
        static_assert( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "FAllocator" );
        void * bytes = ExtendAlloc( _Data, _BytesCount, _NewBytesCount, Alignment, _KeepOld );
        if ( _KeepOld ) {
            if ( _NewBytesCount > _BytesCount ) {
                ClearMemory8( ( byte * )bytes + _BytesCount, _ClearValue, _NewBytesCount - _BytesCount );
            }
        } else {
            ClearMemory8( bytes, _ClearValue, _NewBytesCount );
        }
        return bytes;
    }

    static void * ExtendCleared1( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue = 0 ) { return ExtendCleared< 1 >( _Data, _BytesCount, _NewBytesCount, _KeepOld, _ClearValue ); }
    static void * ExtendCleared16( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue = 0 ) { return ExtendCleared< 16 >( _Data, _BytesCount, _NewBytesCount, _KeepOld, _ClearValue ); }
    static void * ExtendCleared32( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue = 0 ) { return ExtendCleared< 32 >( _Data, _BytesCount, _NewBytesCount, _KeepOld, _ClearValue ); }

    static void Dealloc( void * _Bytes );

private:
    static void * AllocateBytes( size_t _BytesCount, int _Alignment );
    static void * ExtendAlloc( void * _Data, size_t _BytesCount, size_t _NewBytesCount, int _NewAlignment, bool _KeepOld );
};

/*

Dynamic Stack Memory

*/

#define StackAlloc( _NumBytes ) alloca( _NumBytes )


extern ANGIE_API FHeapMemory GMainHeapMemory;
extern ANGIE_API FHunkMemory GMainHunkMemory;
extern ANGIE_API FMemoryZone GMainMemoryZone;
