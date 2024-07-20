/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "../Memory.h"
#include "../Logger.h"

HK_NAMESPACE_BEGIN

template <typename T, size_t BlockCapacity = 1024>
class PoolAllocator final : public Noncopyable
{
    static_assert(BlockCapacity > 0, "Invalid BlockCapacity");

    constexpr static size_t Alignment = std::max(alignof(T), alignof(size_t));
    constexpr static size_t ChunkSize = std::max(sizeof(T), sizeof(size_t));

public:
    PoolAllocator();
    ~PoolAllocator();

    PoolAllocator(PoolAllocator&& Rhs) noexcept
    {
        Core::Swap(m_Blocks, Rhs.m_Blocks);
        Core::Swap(m_CurBlock, Rhs.m_CurBlock);
        Core::Swap(m_TotalChunks, Rhs.m_TotalChunks);
        Core::Swap(m_TotalBlocks, Rhs.m_TotalBlocks);
    }

    PoolAllocator& operator=(PoolAllocator&& Rhs) noexcept
    {
        Free();

        Core::Swap(m_Blocks, Rhs.m_Blocks);
        Core::Swap(m_CurBlock, Rhs.m_CurBlock);
        Core::Swap(m_TotalChunks, Rhs.m_TotalChunks);
        Core::Swap(m_TotalBlocks, Rhs.m_TotalBlocks);

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
    int GetTotalBlocks() const { return m_TotalBlocks; }

    /**Returns the total number of allocated chunks. */
    int GetTotalChunks() const { return m_TotalChunks; }

private:
    union Chunk
    {
        byte   Data[ChunkSize];
        Chunk* Next;
    };
    struct Block
    {
        Chunk  Chunks[BlockCapacity];
        Chunk* FreeList;
        Block* Next;
        int    Allocated;
    };
    Block* m_Blocks;
    Block* m_CurBlock;
    int m_TotalChunks;
    int m_TotalBlocks;

    /** Create a new block */
    Block* AllocateBlock();
};

template <typename T, size_t BlockCapacity>
HK_INLINE PoolAllocator<T, BlockCapacity>::PoolAllocator()
{
    m_Blocks = nullptr;
    m_CurBlock = nullptr;
    m_TotalChunks = 0;
    m_TotalBlocks = 0;
}

template <typename T, size_t BlockCapacity>
HK_INLINE PoolAllocator<T, BlockCapacity>::~PoolAllocator()
{
    Free();
}

template <typename T, size_t BlockCapacity>
HK_INLINE void PoolAllocator<T, BlockCapacity>::Free()
{
    while (m_Blocks)
    {
        Block* block = m_Blocks;
        m_Blocks = block->Next;
        Core::GetHeapAllocator<HEAP_MISC>().Free(block);
    }
    m_CurBlock = nullptr;
    m_TotalChunks = 0;
    m_TotalBlocks = 0;
}

template <typename T, size_t BlockCapacity>
HK_INLINE void PoolAllocator<T, BlockCapacity>::CleanupEmptyBlocks()
{
    Block* prev = nullptr;
    Block* next;

    //DEBUG( "PoolAllocator: total blocks {}\n", m_TotalBlocks );

    // Keep at least one block allocated
    for (Block* block = m_Blocks; block && m_TotalBlocks > 1; block = next)
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
                m_Blocks = next;
            }
            if (m_CurBlock == block)
            {
                m_CurBlock = nullptr;
            }
            Core::GetHeapAllocator<HEAP_MISC>().Free(block);
            m_TotalBlocks--;
        }
        else
        {
            prev = block;
        }
    }

    if (!m_CurBlock)
    {
        for (Block* block = m_Blocks; block; block = block->Next)
        {
            if (block->FreeList)
            {
                m_CurBlock = block;
                break;
            }
        }
    }
}

template <typename T, size_t BlockCapacity>
HK_INLINE typename PoolAllocator<T, BlockCapacity>::Block* PoolAllocator<T, BlockCapacity>::AllocateBlock()
{
    Block* block   = (Block*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(Block), Alignment);
    block->FreeList = block->Chunks;
    int i;
    for (i = 0; i < BlockCapacity - 1; ++i)
    {
        block->Chunks[i].Next = &block->Chunks[i + 1];
    }
    block->Chunks[i].Next = nullptr;
    block->Allocated = 0;
    block->Next      = m_Blocks;
    m_Blocks         = block;
    m_CurBlock       = block;
    ++m_TotalBlocks;
    //DEBUG("PoolAllocator::AllocateBlock: allocated a new block\n");
    return block;
}

template <typename T, size_t BlockCapacity>
HK_INLINE T* PoolAllocator<T, BlockCapacity>::Allocate()
{
    if (m_CurBlock && !m_CurBlock->FreeList)
    {
        // Try to find a block with free chunks
        for (Block* block = m_Blocks; block; block = block->Next)
        {
            if (block->FreeList)
            {
                m_CurBlock = block;
                break;
            }
        }
    }

    if (!m_CurBlock || !m_CurBlock->FreeList)
    {
        AllocateBlock();
    }

    Chunk* chunk = m_CurBlock->FreeList;
    m_CurBlock->FreeList = chunk->Next;
    ++m_CurBlock->Allocated;
    ++m_TotalChunks;
    HK_ASSERT(IsAlignedPtr(&chunk->Data[0], Alignment));
    return (T*)&chunk->Data[0];
}

template <typename T, size_t BlockCapacity>
HK_INLINE void PoolAllocator<T, BlockCapacity>::Deallocate(void* _Bytes)
{
    Chunk* chunk = (Chunk*)_Bytes;
    m_CurBlock = nullptr;
    for (Block* block = m_Blocks; block; block = block->Next)
    {
        if (chunk >= &block->Chunks[0] && chunk < &block->Chunks[BlockCapacity])
        {
            m_CurBlock = block;
            chunk->Next     = block->FreeList;
            block->FreeList = chunk;
            --block->Allocated;
            --m_TotalChunks;
            break;
        }
    }
    HK_ASSERT(m_CurBlock != nullptr);
}

HK_NAMESPACE_END
