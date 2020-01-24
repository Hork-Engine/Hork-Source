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

#include "PrimitiveLinkPool.h"

#include <Core/Public/Logger.h>

APrimitiveLinkPool GPrimitiveLinkPool;

APrimitiveLinkPool::APrimitiveLinkPool() {
    Blocks = nullptr;
    TotalAllocated = 0;
    TotalBlocks = 0;
}

APrimitiveLinkPool::~APrimitiveLinkPool() {
    Free();
}

void APrimitiveLinkPool::Free() {
    SBlock * next;
    for ( SBlock * block = Blocks ; block ; block = next ) {
        next = block->Next;

        GHeapMemory.HeapFree( block );
    }
    Blocks = nullptr;
    TotalAllocated = 0;
    TotalBlocks = 0;
}

void APrimitiveLinkPool::CleanupEmptyBlocks() {
    SBlock * prev = nullptr;
    SBlock * next;
    for ( SBlock * block = Blocks ; block ; block = next ) {
        next = block->Next;

        if ( block->Allocated == 0 && TotalBlocks > 1 ) { // keep at least one block allocated
            if ( prev ) {
                prev->Next = next;
            } else {
                Blocks = next;
            }
            GHeapMemory.HeapFree( block );
            TotalBlocks--;
        } else {
            prev = block;
        }
    }
}

APrimitiveLinkPool::SBlock * APrimitiveLinkPool::AllocateBlock() {
    int i;
    SBlock * block = ( SBlock * )GHeapMemory.HeapAlloc( sizeof( SBlock ), 1 );
    //memset( block->Pool, 0, sizeof( block->Pool ) );
    block->FreeLinks = block->Pool;
    for ( i = 0 ; i < MAX_BLOCK_SIZE - 1 ; i++ ) {
        block->FreeLinks[i].Next = &block->FreeLinks[ i + 1 ];
    }
    block->FreeLinks[i].Next = NULL;
    block->Allocated = 0;
    block->Next = Blocks;
    Blocks = block;
    TotalBlocks++;
    GLogger.Printf( "APrimitiveLinkPool::AllocateBlock: allocated a new block\n" );
    return block;
}

SPrimitiveLink * APrimitiveLinkPool::AllocateLink() {
    SPrimitiveLink * link;
    SBlock * freeBlock = nullptr;

    for ( SBlock * block = Blocks ; block ; block = block->Next ) {
        if ( block->FreeLinks ) {
            freeBlock = block;
            break;
        }
    }

    if ( !freeBlock ) {
        freeBlock = AllocateBlock();
    }

    link = freeBlock->FreeLinks;
    freeBlock->FreeLinks = freeBlock->FreeLinks->Next;
    ++freeBlock->Allocated;
    ++TotalAllocated;
    //GLogger.Printf( "APrimitiveLinkPool::AllocateLink: total blocks %d total links %d\n", TotalBlocks, TotalAllocated );
    return link;
}

void APrimitiveLinkPool::FreeLink( SPrimitiveLink * Link ) {
    for ( SBlock * block = Blocks ; block ; block = block->Next ) {
        // Find block
        if ( Link >= &block->Pool[0] && Link < &block->Pool[MAX_BLOCK_SIZE] ) {
            // free
            Link->Next = block->FreeLinks;
            block->FreeLinks = Link;
            --block->Allocated;
            --TotalAllocated;
            return;
        }
    }
}
