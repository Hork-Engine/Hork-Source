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

template <size_t BlockSize = 64 << 10>
class TLinearAllocator
{
    HK_FORBID_COPY(TLinearAllocator)

public:
    TLinearAllocator()
    {}

    ~TLinearAllocator()
    {
        Free();
    }

    TLinearAllocator(TLinearAllocator&& Rhs) noexcept
    {
        Core::Swap(Blocks, Rhs.Blocks);
        Core::Swap(TotalAllocs, Rhs.TotalAllocs);
        Core::Swap(TotalMemoryUsage, Rhs.TotalMemoryUsage);
    }

    TLinearAllocator& operator=(TLinearAllocator&& Rhs) noexcept
    {
        Free();

        Core::Swap(Blocks, Rhs.Blocks);
        Core::Swap(TotalAllocs, Rhs.TotalAllocs);
        Core::Swap(TotalMemoryUsage, Rhs.TotalMemoryUsage);

        return *this;
    }

    // Creates a new object.
    template <typename T, typename... Args>
    T* New(Args&&... args)
    {
        void* ptr = Allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // Destroys an object and frees memory.
    template <typename T>
    void Delete(T* Ptr)
    {
        Ptr->~T();
        TryFree(Ptr);
    }

    // Allocates an object or structure. Doesn't call constructors.
    template <typename T>
    T* Allocate()
    {
        return static_cast<T*>(Allocate(sizeof(T), alignof(T)));
    }

    // Allocates raw memory
    void* Allocate(size_t SizeInBytes, size_t Alignment = sizeof(size_t))
    {
        SBlock* block;
        size_t  address;

        HK_ASSERT(IsPowerOfTwo(Alignment));

        Alignment   = std::max(sizeof(size_t), Alignment);
        SizeInBytes = Align(SizeInBytes, Alignment);

        if (!FindBlock(SizeInBytes, Alignment, block, address))
        {
            // Allocate new block

            size_t size = std::max<size_t>(SizeInBytes, BlockSize) + sizeof(SBlock) + (Alignment - 1);

            block             = (SBlock*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(size);
            block->Address    = (size_t)AlignPtr(block + 1, Alignment);
            block->MaxAddress = (size_t)((byte*)block + size);
            block->CurAddress = block->Address;
            block->Next       = Blocks;
            Blocks            = block;
            TotalAllocs++;

            address = block->Address;
        }

        block->LastAllocationAddress  = address;
        block->LastAllocationAddress2 = block->CurAddress;

        void* ptr = reinterpret_cast<byte*>(0) + address;
        address += SizeInBytes;

        TotalMemoryUsage += address - block->CurAddress;
        block->CurAddress = address;

        HK_ASSERT(IsAlignedPtr(ptr, Alignment));
        return ptr;
    }

    // Tries to free memory. Returns the size used by the pointer on success, otherwise returns 0.
    size_t TryFree(void* Ptr)
    {
        if (!Ptr)
            return 0;

        size_t address = (size_t)Ptr;

        SBlock* block = GetBlockByAddress(address);
        if (!block)
            return 0;

        if (block->LastAllocationAddress != address)
            return 0;

        size_t size = block->CurAddress - block->LastAllocationAddress2;

        block->CurAddress            = block->LastAllocationAddress2;
        block->LastAllocationAddress = block->LastAllocationAddress2;

        TotalMemoryUsage -= size;

        return size;
    }

    // Tries to get the size used by the given pointer. Returns 0 on failure.
    size_t TryGetSize(void* Ptr)
    {
        if (!Ptr)
            return 0;

        size_t address = (size_t)Ptr;

        SBlock* block = GetBlockByAddress(address);
        if (!block)
            return 0;

        if (block->LastAllocationAddress != address)
            return 0;

        return block->CurAddress - block->LastAllocationAddress;
    }

    // Checks if memory can be easily reallocated.
    bool EasyReallocate(void* Ptr, size_t SizeInBytes, size_t Alignment = sizeof(size_t))
    {
        if (!Ptr)
            return true;

        Alignment = std::max(sizeof(size_t), Alignment);

        if (!IsAlignedPtr(Ptr, Alignment))
            return false;

        size_t address = (size_t)Ptr;

        SBlock* block = GetBlockByAddress(address);
        if (!block)
            return false;

        if (block->LastAllocationAddress != address)
            return false;

        size_t size = block->CurAddress - block->LastAllocationAddress;

        SizeInBytes = Align(SizeInBytes, Alignment);

        return SizeInBytes <= size || block->LastAllocationAddress + SizeInBytes <= block->MaxAddress;
    }

    // Reallocates raw memory.
    void* Reallocate(void* Ptr, size_t SizeInBytes, size_t Alignment = sizeof(size_t), bool bDiscard = false)
    {
        if (!Ptr)
            return Allocate(SizeInBytes, Alignment);

        Alignment = std::max(sizeof(size_t), Alignment);

        if (!IsAlignedPtr(Ptr, Alignment))
        {
            if (bDiscard)
            {
                TryFree(Ptr);
                return Allocate(SizeInBytes, Alignment);
            }

            void*  newPtr = Allocate(SizeInBytes, Alignment);
            size_t size   = TryGetSize(Ptr);
            Platform::Memcpy(newPtr, Ptr, size ? size : SizeInBytes);
            return newPtr;
        }

        size_t address = (size_t)Ptr;

        SBlock* block = GetBlockByAddress(address);
        HK_ASSERT(block);

        if (block->LastAllocationAddress == address)
        {
            SizeInBytes = Align(SizeInBytes, Alignment);

            size_t size = block->CurAddress - block->LastAllocationAddress;
            if (SizeInBytes <= size || block->LastAllocationAddress + SizeInBytes <= block->MaxAddress)
            {
                block->CurAddress = block->LastAllocationAddress + SizeInBytes;

                TotalMemoryUsage -= size;
                TotalMemoryUsage += SizeInBytes;

                return Ptr;
            }
        }

        void* newPtr = Allocate(SizeInBytes, Alignment);
        if (!bDiscard)
            Platform::Memcpy(newPtr, Ptr, SizeInBytes);
        return newPtr;
    }

    // Tries to enlarge the memory size for the given pointer. Returns nullptr on failure.
    void* Extend(void* Ptr, size_t SizeInBytes, size_t Alignment = sizeof(size_t))
    {
        if (!Ptr)
            return Allocate(SizeInBytes, Alignment);

        Alignment = std::max(sizeof(size_t), Alignment);

        if (!IsAlignedPtr(Ptr, Alignment))
            return nullptr;

        size_t address = (size_t)Ptr;

        SBlock* block = GetBlockByAddress(address);
        HK_ASSERT(block);

        if (block->LastAllocationAddress == address)
        {
            SizeInBytes = Align(SizeInBytes, Alignment);

            size_t size = block->CurAddress - block->LastAllocationAddress;
            if (SizeInBytes <= size || block->LastAllocationAddress + SizeInBytes <= block->MaxAddress)
            {
                block->CurAddress = block->LastAllocationAddress + SizeInBytes;

                TotalMemoryUsage -= size;
                TotalMemoryUsage += SizeInBytes;

                return Ptr;
            }
        }

        return nullptr;
    }

    // Free allocated memory.
    void Free()
    {
        for (SBlock* block = Blocks; block;)
        {
            SBlock* next = block->Next;
            Platform::GetHeapAllocator<HEAP_MISC>().Free(block);
            block = next;
        }
        Blocks           = nullptr;
        TotalAllocs      = 0;
        TotalMemoryUsage = 0;
    }

    // Clears and merges memory blocks into one.
    void ResetAndMerge()
    {
        // If we have more than one block
        if (Blocks && Blocks->Next)
        {
            size_t blockMemoryUsage = GetBlockMemoryUsage();

            Free();

            // In most cases, the alignment is less than or equal to 16. Therefore, we use this alignment by default.
            const size_t Alignment = 16;

            blockMemoryUsage = blockMemoryUsage + sizeof(SBlock) + (Alignment - 1);

            SBlock* block                 = (SBlock*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(blockMemoryUsage);
            block->Address                = (size_t)AlignPtr(block + 1, Alignment);
            block->MaxAddress             = (size_t)((byte*)block + blockMemoryUsage);
            block->CurAddress             = block->Address;
            block->LastAllocationAddress  = 0;
            block->LastAllocationAddress2 = 0;
            block->Next                   = Blocks;
            Blocks                        = block;
            TotalAllocs++;
        }
        else
            Reset();
    }

    // Clears memory blocks. Doesn't free memory blocks.
    void Reset()
    {
        for (SBlock* block = Blocks; block; block = block->Next)
        {
            block->CurAddress = block->Address;
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
            blockMemoryUsage += block->MaxAddress - block->Address;
        }
        return blockMemoryUsage;
    }

private:
    struct SBlock
    {
        size_t  Address;
        size_t  MaxAddress;
        size_t  CurAddress;
        size_t  LastAllocationAddress;
        size_t  LastAllocationAddress2;
        SBlock* Next;
    };
    SBlock* Blocks           = nullptr;
    size_t  TotalAllocs      = 0;
    size_t  TotalMemoryUsage = 0;

    // Returns true if the block was found, otherwise the block and address are undefined.
    bool FindBlock(size_t SizeInBytes, size_t Alignment, SBlock*& Block, size_t& Address) const
    {
        for (Block = Blocks; Block; Block = Block->Next)
        {
            Address = Align(Block->CurAddress, Alignment);
            if (Address + SizeInBytes <= Block->MaxAddress)
            {
                return true;
            }
        }
        return false;
    }

    SBlock* GetBlockByAddress(size_t Address) const
    {
        for (SBlock* block = Blocks; block; block = block->Next)
        {
            if (Address >= block->Address && Address < block->MaxAddress)
                return block;
        }
        return nullptr;
    }
};

namespace Allocators
{

class FrameMemoryAllocator : public MemoryAllocatorBase
{
public:
    explicit FrameMemoryAllocator(const char* pName = nullptr)
    {}

    FrameMemoryAllocator(FrameMemoryAllocator const& rhs)
    {}

    FrameMemoryAllocator(FrameMemoryAllocator const& x, const char* pName)
    {}

    FrameMemoryAllocator& operator=(FrameMemoryAllocator const& rhs)
    {
        return *this;
    }

    void* allocate(size_t n, int flags = 0)
    {
        return GetAllocator().Allocate(n, EASTL_SYSTEM_ALLOCATOR_MIN_ALIGNMENT);
    }

    void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
    {
        return GetAllocator().Allocate(n, alignment);
    }

    void* reallocate(void* p, size_t n, bool copyOld)
    {
        return GetAllocator().Reallocate(p, n, EASTL_SYSTEM_ALLOCATOR_MIN_ALIGNMENT, !copyOld);
    }

    void deallocate(void* p, size_t n)
    {
        GetAllocator().TryFree(p);
    }

    void deallocate(void* p)
    {
        GetAllocator().TryFree(p);
    }

    static TLinearAllocator<>& GetAllocator()
    {
        static TLinearAllocator<> frameMemory;
        return frameMemory;
    }
};

HK_FORCEINLINE bool operator==(FrameMemoryAllocator const&, FrameMemoryAllocator const&) { return true; }
HK_FORCEINLINE bool operator!=(FrameMemoryAllocator const&, FrameMemoryAllocator const&) { return false; }

template <typename T>
struct TStdFrameAllocator
{
    typedef T value_type;

    TStdFrameAllocator() = default;
    template <typename U> constexpr TStdFrameAllocator(TStdFrameAllocator<U> const&) noexcept {}

    HK_NODISCARD T* allocate(std::size_t n) noexcept
    {
        HK_ASSERT(n <= std::numeric_limits<std::size_t>::max() / sizeof(T));

        return static_cast<T*>(FrameMemoryAllocator::GetAllocator().Allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        FrameMemoryAllocator::GetAllocator().TryFree(p);
    }
};
template <typename T, typename U> bool operator==(TStdFrameAllocator<T> const&, TStdFrameAllocator<U> const&) { return true; }
template <typename T, typename U> bool operator!=(TStdFrameAllocator<T> const&, TStdFrameAllocator<U> const&) { return false; }

} // namespace Allocators

class AFrameResource
{
public:
    virtual ~AFrameResource() {}

    void* operator new(size_t SizeInBytes)
    {
        return Allocators::FrameMemoryAllocator::GetAllocator().Allocate(SizeInBytes);
    }

    void operator delete(void* Ptr)
    {
        Allocators::FrameMemoryAllocator::GetAllocator().TryFree(Ptr);
    }
};
