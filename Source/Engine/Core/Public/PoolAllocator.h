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

template< typename T, int MAX_BLOCK_SIZE = 1024, int ALIGNMENT = 16 >
class TPoolAllocator {
    AN_FORBID_COPY( TPoolAllocator )

    static_assert( MAX_BLOCK_SIZE >= 1, "Invalid MAX_BLOCK_SIZE" );
    static_assert( ALIGNMENT >= 16 && ALIGNMENT <= 128 && IsPowerOfTwoConstexpr( ALIGNMENT ), "Alignment Check" );

    constexpr static int CHUNK_SIZE = Align( sizeof( T ) < sizeof( size_t ) ? sizeof( size_t ) : sizeof( T ), ALIGNMENT );

public:
    TPoolAllocator();
    ~TPoolAllocator();

    /** Allocate object from pool */
    T * Allocate();

    /** Deallocate object from pool */
    void Deallocate( void * _Bytes );

    /** Free pool */
    void Free();

    /** Remove empty blocks */
    void CleanupEmptyBlocks();

    /** Get total blocks allocated */
    int GetTotalBlocks() const { return TotalBlocks; }

    /** Get total chunks allocated */
    int GetTotalChunks() const { return TotalChunks; }

private:
    union SChunk {
        byte Data[CHUNK_SIZE];
        SChunk * Next;
    };
    struct SBlock {
        SChunk Chunks[MAX_BLOCK_SIZE];
        SChunk * FreeList;
        SBlock * Next;
        int Allocated;
    };
    SBlock * Blocks;
    SBlock * CurBlock;
    //SChunk * FreeList;
    int TotalChunks;
    int TotalBlocks;

    /** Create a new block */
    SBlock * AllocateBlock();
};

template< typename T, int MAX_BLOCK_SIZE, int ALIGNMENT >
AN_INLINE TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::TPoolAllocator() {
    Blocks = nullptr;
    CurBlock = nullptr;
    //FreeList = nullptr;
    TotalChunks = 0;
    TotalBlocks = 0;
}

template< typename T, int MAX_BLOCK_SIZE, int ALIGNMENT >
AN_INLINE TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::~TPoolAllocator() {
    Free();
}

template< typename T, int MAX_BLOCK_SIZE, int ALIGNMENT >
AN_INLINE void TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::Free() {
    while ( Blocks ) {
        SBlock * block = Blocks;
        Blocks = block->Next;
        GHeapMemory.Free( block );
    }
    CurBlock = nullptr;
    //FreeList = nullptr;
    TotalChunks = 0;
    TotalBlocks = 0;
}

template< typename T, int MAX_BLOCK_SIZE, int ALIGNMENT >
AN_INLINE void TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::CleanupEmptyBlocks() {
    SBlock * prev = nullptr;
    SBlock * next;

    //GLogger.Printf( "TPoolAllocator: total blocks %d\n", TotalBlocks );

    // Keep at least one block allocated
    for ( SBlock * block = Blocks ; block && TotalBlocks > 1 ; block = next ) {
        next = block->Next;

        if ( block->Allocated == 0 ) {
            if ( prev ) {
                prev->Next = next;
            } else {
                Blocks = next;
            }
            if ( CurBlock == block ) {
                CurBlock = nullptr;
            }
            GHeapMemory.Free( block );
            TotalBlocks--;
        } else {
            prev = block;
        }
    }

    if ( !CurBlock ) {
        for ( SBlock * block = Blocks ; block ; block = block->Next ) {
            if ( block->FreeList ) {
                CurBlock = block;
                break;
            }
        }
    }
}

template< typename T, int MAX_BLOCK_SIZE, int ALIGNMENT >
AN_INLINE typename TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::SBlock * TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::AllocateBlock() {
    SBlock * block = ( SBlock * )GHeapMemory.Alloc( sizeof( SBlock ), ALIGNMENT );
    block->FreeList = block->Chunks;
#if 0
    for ( int i = 0 ; i < MAX_BLOCK_SIZE ; ++i ) {
        block->Chunks[ i ].Next = FreeList;
        FreeList = &block->Chunks[ i ];
    }
#else
    int i;
    for ( i = 0 ; i < MAX_BLOCK_SIZE - 1 ; ++i ) {
        block->Chunks[ i ].Next = &block->Chunks[ i + 1 ];
    }
    block->Chunks[ i ].Next = nullptr;
#endif
    block->Allocated = 0;
    block->Next = Blocks;
    Blocks = block;
    CurBlock = block;
    ++TotalBlocks;
    GLogger.Printf( "TPoolAllocator::AllocateBlock: allocated a new block\n" );
    return block;
}

template< typename T, int MAX_BLOCK_SIZE, int ALIGNMENT >
AN_INLINE T * TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::Allocate() {
    if ( CurBlock && !CurBlock->FreeList ) {
        // Try to find a block with free chunks
        for ( SBlock * block = Blocks ; block ; block = block->Next ) {
            if ( block->FreeList ) {
                CurBlock = block;
                break;
            }
        }
    }

    if ( !CurBlock || !CurBlock->FreeList ) {
        AllocateBlock();
    }

    SChunk * chunk = CurBlock->FreeList;
    CurBlock->FreeList = chunk->Next;
    ++CurBlock->Allocated;
    ++TotalChunks;
    return ( T * )&chunk->Data[0];
}

template< typename T, int MAX_BLOCK_SIZE, int ALIGNMENT >
AN_INLINE void TPoolAllocator< T, MAX_BLOCK_SIZE, ALIGNMENT >::Deallocate( void * _Bytes ) {
    SChunk * chunk = ( SChunk * )_Bytes;
    CurBlock = nullptr;
    for ( SBlock * block = Blocks ; block ; block = block->Next ) {
        if ( chunk >= &block->Chunks[0] && chunk < &block->Chunks[MAX_BLOCK_SIZE] ) {
            CurBlock = block;
            chunk->Next = block->FreeList;
            block->FreeList = chunk;
            --block->Allocated;
            --TotalChunks;
            break;
        }
    }
    AN_ASSERT( CurBlock != nullptr );
}
