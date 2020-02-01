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

#include "Logger.h"

template< int MIN_BLOCK_SIZE = 64<<10 >
class TLinearAllocator {
    AN_FORBID_COPY( TLinearAllocator )

public:
    TLinearAllocator() {
        Chunks = nullptr;
        TotalAllocs = 0;
    }

    ~TLinearAllocator() {
        GLogger.Printf( "Total allocs: %d\n", TotalAllocs );
        Free();
    }

    void * Alloc( size_t _SizeInBytes );

    void Free() {
        for ( SChunk * chunk = Chunks ; chunk ; ) {
            SChunk * next = chunk->Next;
            GHeapMemory.HeapFree( chunk );
            chunk = next;
        }
        Chunks = nullptr;
        TotalAllocs = 0;
    }

private:
    struct SChunk {
        size_t TotalAllocated;
        SChunk * Next;
    };
    SChunk * Chunks;
    size_t TotalAllocs;
};

template< int MIN_BLOCK_SIZE >
void * TLinearAllocator< MIN_BLOCK_SIZE >::Alloc( size_t _SizeInBytes ) {
    SChunk * chunk = Chunks;

    if ( !chunk || chunk->TotalAllocated + _SizeInBytes > MIN_BLOCK_SIZE ) {
        size_t chunkSize = Math::Max< size_t >( _SizeInBytes, MIN_BLOCK_SIZE );
        chunk = ( SChunk * )GHeapMemory.HeapAlloc( chunkSize + sizeof( SChunk ), 1 );
        chunk->TotalAllocated = 0;
        chunk->Next = Chunks;
        Chunks = chunk;
        TotalAllocs++;
    }

    void * ptr = reinterpret_cast< byte * >( chunk + 1 ) + chunk->TotalAllocated;
    chunk->TotalAllocated += _SizeInBytes;

    return ptr;
}
