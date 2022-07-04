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

#pragma once

#include "Memory.h"
#include "../Logger.h"

template <typename T, size_t BlockCapacity = 1024>
class TPoolAllocator
{
    HK_FORBID_COPY(TPoolAllocator)

    static_assert(BlockCapacity > 0, "Invalid BlockCapacity");

    constexpr static size_t Alignment = std::max(alignof(T), alignof(size_t));
    constexpr static size_t ChunkSize = std::max(sizeof(T), sizeof(size_t));

public:
    TPoolAllocator();
    ~TPoolAllocator();

    TPoolAllocator(TPoolAllocator&& Rhs) noexcept
    {
        Core::Swap(Blocks, Rhs.Blocks);
        Core::Swap(CurBlock, Rhs.CurBlock);
        //Core::Swap(FreeList, Rhs.FreeList);
        Core::Swap(TotalChunks, Rhs.TotalChunks);
        Core::Swap(TotalBlocks, Rhs.TotalBlocks);
    }

    TPoolAllocator& operator=(TPoolAllocator&& Rhs) noexcept
    {
        Free();

        Core::Swap(Blocks, Rhs.Blocks);
        Core::Swap(CurBlock, Rhs.CurBlock);
        //Core::Swap(FreeList, Rhs.FreeList);
        Core::Swap(TotalChunks, Rhs.TotalChunks);
        Core::Swap(TotalBlocks, Rhs.TotalBlocks);

        return *this;
    }

    /** Allocates an object from the pool. Doesn't call constructors. */
    T* Allocate();

    /** Deallocate object from pool */
    void Deallocate(void* _Bytes);

    /** Free pool. */
    void Free();

    /** Remove empty blocks. */
    void CleanupEmptyBlocks();

    /** Returns the total number of allocated blocks. */
    int GetTotalBlocks() const { return TotalBlocks; }

    /**Returns the total number of allocated chunks. */
    int GetTotalChunks() const { return TotalChunks; }

private:
    union SChunk
    {
        byte    Data[ChunkSize];
        SChunk* Next;
    };
    struct SBlock
    {
        SChunk  Chunks[BlockCapacity];
        SChunk* FreeList;
        SBlock* Next;
        int     Allocated;
    };
    SBlock* Blocks;
    SBlock* CurBlock;
    //SChunk * FreeList;
    int TotalChunks;
    int TotalBlocks;

    /** Create a new block */
    SBlock* AllocateBlock();
};

template <typename T, size_t BlockCapacity>
HK_INLINE TPoolAllocator<T, BlockCapacity>::TPoolAllocator()
{
    Blocks   = nullptr;
    CurBlock = nullptr;
    //FreeList = nullptr;
    TotalChunks = 0;
    TotalBlocks = 0;
}

template <typename T, size_t BlockCapacity>
HK_INLINE TPoolAllocator<T, BlockCapacity>::~TPoolAllocator()
{
    Free();
}

template <typename T, size_t BlockCapacity>
HK_INLINE void TPoolAllocator<T, BlockCapacity>::Free()
{
    while (Blocks)
    {
        SBlock* block = Blocks;
        Blocks        = block->Next;
        Platform::GetHeapAllocator<HEAP_MISC>().Free(block);
    }
    CurBlock = nullptr;
    //FreeList = nullptr;
    TotalChunks = 0;
    TotalBlocks = 0;
}

template <typename T, size_t BlockCapacity>
HK_INLINE void TPoolAllocator<T, BlockCapacity>::CleanupEmptyBlocks()
{
    SBlock* prev = nullptr;
    SBlock* next;

    //DEBUG( "TPoolAllocator: total blocks {}\n", TotalBlocks );

    // Keep at least one block allocated
    for (SBlock* block = Blocks; block && TotalBlocks > 1; block = next)
    {
        next = block->Next;

        if (block->Allocated == 0)
        {
            if (prev)
            {
                prev->Next = next;
            }
            else
            {
                Blocks = next;
            }
            if (CurBlock == block)
            {
                CurBlock = nullptr;
            }
            Platform::GetHeapAllocator<HEAP_MISC>().Free(block);
            TotalBlocks--;
        }
        else
        {
            prev = block;
        }
    }

    if (!CurBlock)
    {
        for (SBlock* block = Blocks; block; block = block->Next)
        {
            if (block->FreeList)
            {
                CurBlock = block;
                break;
            }
        }
    }
}

template <typename T, size_t BlockCapacity>
HK_INLINE typename TPoolAllocator<T, BlockCapacity>::SBlock* TPoolAllocator<T, BlockCapacity>::AllocateBlock()
{
    SBlock* block   = (SBlock*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(SBlock), Alignment);
    block->FreeList = block->Chunks;
#if 0
    for ( int i = 0 ; i < BlockCapacity ; ++i ) {
        block->Chunks[ i ].Next = FreeList;
        FreeList = &block->Chunks[ i ];
    }
#else
    int i;
    for (i = 0; i < BlockCapacity - 1; ++i)
    {
        block->Chunks[i].Next = &block->Chunks[i + 1];
    }
    block->Chunks[i].Next = nullptr;
#endif
    block->Allocated = 0;
    block->Next      = Blocks;
    Blocks           = block;
    CurBlock         = block;
    ++TotalBlocks;
    DEBUG("TPoolAllocator::AllocateBlock: allocated a new block\n");
    return block;
}

template <typename T, size_t BlockCapacity>
HK_INLINE T* TPoolAllocator<T, BlockCapacity>::Allocate()
{
    if (CurBlock && !CurBlock->FreeList)
    {
        // Try to find a block with free chunks
        for (SBlock* block = Blocks; block; block = block->Next)
        {
            if (block->FreeList)
            {
                CurBlock = block;
                break;
            }
        }
    }

    if (!CurBlock || !CurBlock->FreeList)
    {
        AllocateBlock();
    }

    SChunk* chunk      = CurBlock->FreeList;
    CurBlock->FreeList = chunk->Next;
    ++CurBlock->Allocated;
    ++TotalChunks;
    HK_ASSERT(IsAlignedPtr(&chunk->Data[0], Alignment));
    return (T*)&chunk->Data[0];
}

template <typename T, size_t BlockCapacity>
HK_INLINE void TPoolAllocator<T, BlockCapacity>::Deallocate(void* _Bytes)
{
    SChunk* chunk = (SChunk*)_Bytes;
    CurBlock      = nullptr;
    for (SBlock* block = Blocks; block; block = block->Next)
    {
        if (chunk >= &block->Chunks[0] && chunk < &block->Chunks[BlockCapacity])
        {
            CurBlock        = block;
            chunk->Next     = block->FreeList;
            block->FreeList = chunk;
            --block->Allocated;
            --TotalChunks;
            break;
        }
    }
    HK_ASSERT(CurBlock != nullptr);
}
