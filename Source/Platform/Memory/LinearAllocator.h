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

template <int MIN_BLOCK_SIZE = 64 << 10>
class TLinearAllocator
{
    HK_FORBID_COPY(TLinearAllocator)

public:
    TLinearAllocator()
    {
    }

    ~TLinearAllocator()
    {
        Free();
    }

    void* Allocate(size_t _SizeInBytes)
    {
        SBlock* block = FindBlock(_SizeInBytes);

        if (!block)
        {
            size_t blockSize      = std::max<size_t>(_SizeInBytes, MIN_BLOCK_SIZE);
            block                 = (SBlock*)GHeapMemory.Alloc(blockSize + sizeof(SBlock), 16);
            block->TotalAllocated = 0;
            block->Size           = blockSize;
            block->Next           = Blocks;
            Blocks                = block;
            TotalAllocs++;
        }

        void* ptr = reinterpret_cast<byte*>(block + 1) + block->TotalAllocated;
        block->TotalAllocated += _SizeInBytes;
        block->TotalAllocated = Align(block->TotalAllocated, 16);

        TotalMemoryUsage += _SizeInBytes;

        HK_ASSERT(IsAlignedPtr(ptr, 16));

        return ptr;
    }

    void Free()
    {
        for (SBlock* block = Blocks; block;)
        {
            SBlock* next = block->Next;
            GHeapMemory.Free(block);
            block = next;
        }
        Blocks           = nullptr;
        TotalAllocs      = 0;
        TotalMemoryUsage = 0;
    }

    void Reset()
    {
        for (SBlock* block = Blocks; block; block = block->Next)
        {
            block->TotalAllocated = 0;
        }
        TotalMemoryUsage = 0;
    }

    size_t GetBlockCount() const
    {
        return TotalAllocs;
    }

    size_t GetTotalMemoryUsage() const
    {
        return TotalMemoryUsage;
    }

    size_t GetBlockMemoryUsage() const
    {
        size_t blockMemoryUsage = 0;
        for (SBlock* block = Blocks; block; block = block->Next)
        {
            blockMemoryUsage += block->Size;
        }
        return blockMemoryUsage;
    }

private:
    struct alignas(16) SBlock
    {
        size_t  TotalAllocated;
        size_t  Size;
        SBlock* Next;
    };
    SBlock* Blocks           = nullptr;
    size_t  TotalAllocs      = 0;
    size_t  TotalMemoryUsage = 0;

    HK_VALIDATE_TYPE_SIZE(SBlock, 32);

    SBlock* FindBlock(size_t _SizeInBytes)
    {
        for (SBlock* block = Blocks; block; block = block->Next)
        {
            if (block->TotalAllocated + _SizeInBytes <= block->Size)
            {
                return block;
            }
        }
        return nullptr;
    }
};
