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

#pragma once

#include "Atomic.h"
#include "Thread.h"

/*

Memory utilites

*/

/** Clear memory with size aligned to 8 byte block length */
AN_FORCEINLINE void ClearMemory8( void * _Buffer, uint64_t _ClearValue, size_t _Size ) {
    size_t count = ( ( _Size + 7 ) & ~7 ) >> 3;
    uint64_t * data = ( uint64_t * )_Buffer;
    while ( count-- ) {
        *data++ = _ClearValue;
    }
}

/** Clear memory with size aligned to 8 byte block length */
AN_FORCEINLINE void ZeroMemory8( void * _Buffer, size_t _Size ) {
    size_t count = ( ( _Size + 7 ) & ~7 ) >> 3;
    uint64_t * data = ( uint64_t * )_Buffer;
    while ( count-- ) {
        *data++ = 0;
    }
}

/** Built-in memset function replacement */
AN_FORCEINLINE void Memset( void * d, int v, size_t sz ) {
    byte * p = (byte*)d;
    while ( sz-- ) {
        *p++ = v;
    }
}

/** Built-in memset function replacement */
AN_FORCEINLINE void ZeroMem( void * d, size_t sz ) {
    byte * p = (byte*)d;
    while ( sz-- ) {
        *p++ = 0;
    }
}

/**

AHeapMemory

Allocates memory on heap

*/
class ANGIE_API AHeapMemory final {
    AN_FORBID_COPY( AHeapMemory )

public:
    AHeapMemory();
    ~AHeapMemory();

    /** Initialize memory (main thread only) */
    void Initialize();

    /** Deinitialize memory (main thread only) */
    void Deinitialize();

    /** Heap memory allocation (thread safe) */
    void * HeapAlloc( size_t _BytesCount, int _Alignment );

    /** Heap memory allocation (thread safe) */
    void * HeapAllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue = 0 );

    /** Heap memory deallocation (thread safe) */
    void HeapFree( void * _Bytes );

    /** Check if memory was trashed (thread safe) */
    void PointerTrashTest( void * _Bytes );

    /** Clearing whole heap memory (main thread only) */
    void Clear();

    /** Statistics: current memory usage */
    static size_t GetTotalMemoryUsage();

    /** Statistics: current memory usage overhead */
    static size_t GetTotalMemoryOverhead();

    /** Statistics: max memory usage during memory using */
    static size_t GetMaxMemoryUsage();

private:
    void CheckMemoryLeaks();

    struct SHeapChunk {
        uint32_t Size;
        SHeapChunk * pNext;
        SHeapChunk * pPrev;
        int16_t Padding; // to keep heap aligned
    };

    SHeapChunk HeapChain;

    AThreadSync Sync;
};

AN_FORCEINLINE void * AHeapMemory::HeapAllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue ) {
    void * bytes = HeapAlloc( _BytesCount, _Alignment );
    ClearMemory8( bytes, _ClearValue, _BytesCount );
    return bytes;
}

/**

AHunkMemory

For large temporary blocks like textures, meshes, etc

Example:

int Mark = HunkMemory.SetHunkMark();

byte * buffer1 = HunkMemory.HunkMemory( bufferSize1 );
byte * buffer2 = HunkMemory.HunkMemory( bufferSize2 );
...

HunkMemory.ClearToMark( Mark ); // here all buffers allocated after SetHunkMark will be deallocated

*/
class ANGIE_API AHunkMemory final {
    AN_FORBID_COPY( AHunkMemory )

public:
    AHunkMemory() {}

    /** Initialize memory */
    void Initialize( void * _MemoryAddress, int _SizeInMegabytes );

    /** Deinitialize memory */
    void Deinitialize();

    void * GetHunkMemoryAddress() const;
    int GetHunkMemorySizeInMegabytes() const;

    /** Hunk memory allocation */
    void * HunkMemory( size_t _BytesCount, int _Alignment );

    /** Hunk memory allocation */
    void * HunkMemoryCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue = 0 );

    /** Memory control functions */
    int SetHunkMark();
    void ClearToMark( int _Mark );
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

    void IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );
    void DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );

    struct SHunkMemory * MemoryBuffer = nullptr;

    size_t TotalMemoryUsage = 0;
    size_t TotalMemoryOverhead = 0;
    size_t MaxMemoryUsage = 0;
};

AN_FORCEINLINE void * AHunkMemory::HunkMemoryCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue ) {
    void * bytes = HunkMemory( _BytesCount, _Alignment );
    ClearMemory8( bytes, _ClearValue, _BytesCount );
    return bytes;
}

/**

AZoneMemory

For small blocks, objects or strings

*/
class ANGIE_API AZoneMemory final {
    AN_FORBID_COPY( AZoneMemory )

public:
    AZoneMemory() {}

    /** Initialize memory */
    void Initialize( void * _MemoryAddress, int _SizeInMegabytes );

    /** Deinitialize memory */
    void Deinitialize();

    void * GetZoneMemoryAddress() const;
    int GetZoneMemorySizeInMegabytes() const;

    /** Zone memory allocation */
    void * Alloc( size_t _BytesCount, int _Alignment );

    /** Zone memory allocation */
    void * AllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue = 0 );

    /** Tries to extend existing buffer */
    void * Extend( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld );

    /** Tries to extend existing buffer */
    void * ExtendCleared( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld, uint64_t _ClearValue = 0 );

    /** Zone memory deallocation */
    void Dealloc( void * _Bytes );

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
    struct SZoneChunk * FindFreeChunk( int _RequiredSize );

    void CheckMemoryLeaks();

    void IncMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );
    void DecMemoryStatistics( size_t _MemoryUsage, size_t _Overhead );

    struct SZoneBuffer * MemoryBuffer = nullptr;

    AAtomicLong TotalMemoryUsage;
    AAtomicLong TotalMemoryOverhead;
    AAtomicLong MaxMemoryUsage;

    AThreadSync Sync;
};

AN_FORCEINLINE void * AZoneMemory::AllocCleared( size_t _BytesCount, int _Alignment, uint64_t _ClearValue ) {
    void * bytes = Alloc( _BytesCount, _Alignment );
    ClearMemory8( bytes, _ClearValue, _BytesCount );
    return bytes;
}

AN_FORCEINLINE void * AZoneMemory::ExtendCleared( void * _Data, int _BytesCount, int _NewBytesCount, int _NewAlignment, bool _KeepOld, uint64_t _ClearValue ) {
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

/**

TTemplateAllocator

Allocator base interface

*/
template< typename T >
class TTemplateAllocator {
public:
    void * AllocAligned( size_t _BytesCount, int Alignment ) {
        AN_ASSERT_( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "alignment must be power of two" );
        return static_cast< T * >( this )->ImplAllocate( _BytesCount, Alignment );
    }

    void * AllocClearedAligned( size_t _BytesCount, uint64_t _ClearValue, int Alignment ) {
        AN_ASSERT_( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "alignment must be power of two" );
        void * bytes = static_cast< T * >( this )->ImplAllocate( _BytesCount, Alignment );
        ClearMemory8( bytes, _ClearValue, _BytesCount );
        return bytes;
    }

    void * ExtendAligned( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, int Alignment ) {
        AN_ASSERT_( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "alignment must be power of two" );
        return static_cast< T * >( this )->ImplExtend( _Data, _BytesCount, _NewBytesCount, Alignment, _KeepOld );
    }

    void * ExtendClearedAligned( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue, int Alignment ) {
        AN_ASSERT_( Alignment <= 128 && IsPowerOfTwoConstexpr( Alignment ), "alignment must be power of two" );
        void * bytes = static_cast< T * >( this )->ImplExtend( _Data, _BytesCount, _NewBytesCount, Alignment, _KeepOld );
        if ( _KeepOld ) {
            if ( _NewBytesCount > _BytesCount ) {
                ClearMemory8( ( byte * )bytes + _BytesCount, _ClearValue, _NewBytesCount - _BytesCount );
            }
        } else {
            ClearMemory8( bytes, _ClearValue, _NewBytesCount );
        }
        return bytes;
    }

    void Dealloc( void * _Bytes ) {
        static_cast< T * >( this )->ImplDeallocate( _Bytes );
    }

    void * Alloc1( size_t _BytesCount ) { return AllocAligned( _BytesCount, 1 ); }
    void * Alloc16( size_t _BytesCount ) { return AllocAligned( _BytesCount, 16 ); }
    void * Alloc32( size_t _BytesCount ) { return AllocAligned( _BytesCount, 32 ); }

    void * AllocCleared1( size_t _BytesCount, uint64_t _ClearValue = 0 ) { return AllocClearedAligned( _BytesCount, _ClearValue, 1 ); }
    void * AllocCleared16( size_t _BytesCount, uint64_t _ClearValue = 0 ) { return AllocClearedAligned( _BytesCount, _ClearValue, 16 ); }
    void * AllocCleared32( size_t _BytesCount, uint64_t _ClearValue = 0 ) { return AllocClearedAligned( _BytesCount, _ClearValue, 32 ); }

    void * Extend1( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld ) { return ExtendAligned( _Data, _BytesCount, _NewBytesCount, _KeepOld, 1 ); }
    void * Extend16( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld ) { return ExtendAligned( _Data, _BytesCount, _NewBytesCount, _KeepOld, 16 ); }
    void * Extend32( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld ) { return ExtendAligned( _Data, _BytesCount, _NewBytesCount, _KeepOld, 32 ); }

    void * ExtendCleared1( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue = 0 ) { return ExtendClearedAligned( _Data, _BytesCount, _NewBytesCount, _KeepOld, _ClearValue, 1 ); }
    void * ExtendCleared16( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue = 0 ) { return ExtendClearedAligned( _Data, _BytesCount, _NewBytesCount, _KeepOld, _ClearValue, 16 ); }
    void * ExtendCleared32( void * _Data, size_t _BytesCount, size_t _NewBytesCount, bool _KeepOld, uint64_t _ClearValue = 0 ) { return ExtendClearedAligned( _Data, _BytesCount, _NewBytesCount, _KeepOld, _ClearValue, 32 ); }
};

/**

AZoneAllocator

Use for small objects

*/
class ANGIE_API AZoneAllocator final : public TTemplateAllocator< AZoneAllocator > {
    AN_FORBID_COPY( AZoneAllocator )

public:
    AZoneAllocator() {}
    static AZoneAllocator & Inst() { static AZoneAllocator inst; return inst; }
    void * ImplAllocate( size_t _BytesCount, int _Alignment );
    void * ImplExtend( void * _Data, size_t _BytesCount, size_t _NewBytesCount, int _NewAlignment, bool _KeepOld );
    void ImplDeallocate( void * _Bytes );
};

/**

AHeapAllocator

Use for huge data allocation

*/
class ANGIE_API AHeapAllocator final : public TTemplateAllocator< AHeapAllocator > {
    AN_FORBID_COPY( AHeapAllocator )

public:
    AHeapAllocator() {}
    static AHeapAllocator & Inst() { static AHeapAllocator inst; return inst; }
    void * ImplAllocate( size_t _BytesCount, int _Alignment );
    void * ImplExtend( void * _Data, size_t _BytesCount, size_t _NewBytesCount, int _NewAlignment, bool _KeepOld );
    void ImplDeallocate( void * _Bytes );
};

void * HugeAlloc( size_t _Size );
void HugeFree( void * _Data );

/**

Dynamic Stack Memory

*/

#define StackAlloc( _NumBytes ) alloca( _NumBytes )


extern ANGIE_API AHeapMemory GHeapMemory;
extern ANGIE_API AHunkMemory GHunkMemory;
extern ANGIE_API AZoneMemory GZoneMemory;
