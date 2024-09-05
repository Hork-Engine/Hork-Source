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

HK_NAMESPACE_BEGIN

template <size_t BlockSize = 64 << 10>
class LinearAllocator final : public Noncopyable
{
public:
    LinearAllocator() = default;

    ~LinearAllocator()
    {
        Free();
    }

    LinearAllocator(LinearAllocator&& rhs) noexcept
    {
        Core::Swap(m_Blocks, rhs.m_Blocks);
        Core::Swap(m_TotalAllocs, rhs.m_TotalAllocs);
        Core::Swap(m_TotalMemoryUsage, rhs.m_TotalMemoryUsage);
    }

    LinearAllocator& operator=(LinearAllocator&& rhs) noexcept
    {
        Free();

        Core::Swap(m_Blocks, rhs.m_Blocks);
        Core::Swap(m_TotalAllocs, rhs.m_TotalAllocs);
        Core::Swap(m_TotalMemoryUsage, rhs.m_TotalMemoryUsage);

        return *this;
    }

    // Creates a new object.
    template <typename T, typename... Args>
    T* New(Args&&... args)
    {
        void* ptr = Allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T* AlignedNew(size_t alignment, Args&&... args)
    {
        void* ptr = Allocate(sizeof(T), alignment);
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // Destroys an object and frees memory.
    template <typename T>
    void Delete(T* ptr)
    {
        ptr->~T();
        TryFree(ptr);
    }

    // Allocates an object or structure. Doesn't call constructors.
    template <typename T>
    T* Allocate()
    {
        return static_cast<T*>(Allocate(sizeof(T), alignof(T)));
    }

    // Allocates raw memory
    void* Allocate(size_t sizeInBytes, size_t alignment = sizeof(size_t))
    {
        Block* block;
        size_t  address;

        HK_ASSERT(IsPowerOfTwo(alignment));

        alignment   = std::max(sizeof(size_t), alignment);
        sizeInBytes = Align(sizeInBytes, alignment);

        if (!FindBlock(sizeInBytes, alignment, block, address))
        {
            // Allocate new block

            size_t size = std::max<size_t>(sizeInBytes, BlockSize) + sizeof(Block) + (alignment - 1);

            block             = (Block*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(size);
            block->Address    = (size_t)AlignPtr(block + 1, alignment);
            block->MaxAddress = (size_t)((byte*)block + size);
            block->CurAddress = block->Address;
            block->Next       = m_Blocks;
            m_Blocks = block;
            m_TotalAllocs++;

            address = block->Address;
        }

        block->LastAllocationAddress  = address;
        block->LastAllocationAddress2 = block->CurAddress;

        void* ptr = reinterpret_cast<byte*>(0) + address;
        address += sizeInBytes;

        m_TotalMemoryUsage += address - block->CurAddress;
        block->CurAddress = address;

        HK_ASSERT(IsAlignedPtr(ptr, alignment));
        return ptr;
    }

    // Tries to free memory. Returns the size used by the pointer on success, otherwise returns 0.
    size_t TryFree(void* ptr)
    {
        if (!ptr)
            return 0;

        size_t address = (size_t)ptr;

        Block* block = GetBlockByAddress(address);
        if (!block)
            return 0;

        if (block->LastAllocationAddress != address)
            return 0;

        size_t size = block->CurAddress - block->LastAllocationAddress2;

        block->CurAddress            = block->LastAllocationAddress2;
        block->LastAllocationAddress = block->LastAllocationAddress2;

        m_TotalMemoryUsage -= size;

        return size;
    }

    // Tries to get the size used by the given pointer. Returns 0 on failure.
    size_t TryGetSize(void* ptr)
    {
        if (!ptr)
            return 0;

        size_t address = (size_t)ptr;

        Block* block = GetBlockByAddress(address);
        if (!block)
            return 0;

        if (block->LastAllocationAddress != address)
            return 0;

        return block->CurAddress - block->LastAllocationAddress;
    }

    // Checks if memory can be easily reallocated.
    bool EasyReallocate(void* ptr, size_t sizeInBytes, size_t alignment = sizeof(size_t))
    {
        if (!ptr)
            return true;

        alignment = std::max(sizeof(size_t), alignment);

        if (!IsAlignedPtr(ptr, alignment))
            return false;

        size_t address = (size_t)ptr;

        Block* block = GetBlockByAddress(address);
        if (!block)
            return false;

        if (block->LastAllocationAddress != address)
            return false;

        size_t size = block->CurAddress - block->LastAllocationAddress;

        sizeInBytes = Align(sizeInBytes, alignment);

        return sizeInBytes <= size || block->LastAllocationAddress + sizeInBytes <= block->MaxAddress;
    }

    // Reallocates raw memory.
    void* Reallocate(void* ptr, size_t sizeInBytes, size_t alignment = sizeof(size_t), bool discard = false)
    {
        if (!ptr)
            return Allocate(sizeInBytes, alignment);

        alignment = std::max(sizeof(size_t), alignment);

        if (!IsAlignedPtr(ptr, alignment))
        {
            if (discard)
            {
                TryFree(ptr);
                return Allocate(sizeInBytes, alignment);
            }

            void*  newPtr = Allocate(sizeInBytes, alignment);
            size_t size   = TryGetSize(ptr);
            Core::Memcpy(newPtr, ptr, size ? size : sizeInBytes);
            return newPtr;
        }

        size_t address = (size_t)ptr;

        Block* block = GetBlockByAddress(address);
        HK_ASSERT(block);

        if (block->LastAllocationAddress == address)
        {
            sizeInBytes = Align(sizeInBytes, alignment);

            size_t size = block->CurAddress - block->LastAllocationAddress;
            if (sizeInBytes <= size || block->LastAllocationAddress + sizeInBytes <= block->MaxAddress)
            {
                block->CurAddress = block->LastAllocationAddress + sizeInBytes;

                m_TotalMemoryUsage -= size;
                m_TotalMemoryUsage += sizeInBytes;

                return ptr;
            }
        }

        void* newPtr = Allocate(sizeInBytes, alignment);
        if (!discard)
            Core::Memcpy(newPtr, ptr, sizeInBytes);
        return newPtr;
    }

    // Tries to enlarge the memory size for the given pointer. Returns nullptr on failure.
    void* Extend(void* ptr, size_t sizeInBytes, size_t alignment = sizeof(size_t))
    {
        if (!ptr)
            return Allocate(sizeInBytes, alignment);

        alignment = std::max(sizeof(size_t), alignment);

        if (!IsAlignedPtr(ptr, alignment))
            return nullptr;

        size_t address = (size_t)ptr;

        Block* block = GetBlockByAddress(address);
        HK_ASSERT(block);

        if (block->LastAllocationAddress == address)
        {
            sizeInBytes = Align(sizeInBytes, alignment);

            size_t size = block->CurAddress - block->LastAllocationAddress;
            if (sizeInBytes <= size || block->LastAllocationAddress + sizeInBytes <= block->MaxAddress)
            {
                block->CurAddress = block->LastAllocationAddress + sizeInBytes;

                m_TotalMemoryUsage -= size;
                m_TotalMemoryUsage += sizeInBytes;

                return ptr;
            }
        }

        return nullptr;
    }

    // Free allocated memory.
    void Free()
    {
        for (Block* block = m_Blocks; block;)
        {
            Block* next = block->Next;
            Core::GetHeapAllocator<HEAP_MISC>().Free(block);
            block = next;
        }
        m_Blocks = nullptr;
        m_TotalAllocs = 0;
        m_TotalMemoryUsage = 0;
    }

    // Clears and merges memory blocks into one.
    void ResetAndMerge()
    {
        // If we have more than one block
        if (m_Blocks && m_Blocks->Next)
        {
            size_t blockMemoryUsage = GetBlockMemoryUsage();

            Free();

            // In most cases, the alignment is less than or equal to 16. Therefore, we use this alignment by default.
            const size_t Alignment = 16;

            blockMemoryUsage = blockMemoryUsage + sizeof(Block) + (Alignment - 1);

            Block* block                  = (Block*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(blockMemoryUsage);
            block->Address                = (size_t)AlignPtr(block + 1, Alignment);
            block->MaxAddress             = (size_t)((byte*)block + blockMemoryUsage);
            block->CurAddress             = block->Address;
            block->LastAllocationAddress  = 0;
            block->LastAllocationAddress2 = 0;
            block->Next                   = m_Blocks;
            m_Blocks                      = block;
            m_TotalAllocs++;
        }
        else
            Reset();
    }

    // Clears memory blocks. Doesn't free memory blocks.
    void Reset()
    {
        for (Block* block = m_Blocks; block; block = block->Next)
        {
            block->CurAddress = block->Address;
        }
        m_TotalMemoryUsage = 0;
    }

    size_t GetBlockCount() const
    {
        return m_TotalAllocs;
    }

    size_t GetTotalMemoryUsage() const
    {
        return m_TotalMemoryUsage;
    }

    size_t GetBlockMemoryUsage() const
    {
        size_t blockMemoryUsage = 0;
        for (Block* block = m_Blocks; block; block = block->Next)
        {
            blockMemoryUsage += block->MaxAddress - block->Address;
        }
        return blockMemoryUsage;
    }

private:
    struct Block
    {
        size_t Address;
        size_t MaxAddress;
        size_t CurAddress;
        size_t LastAllocationAddress;
        size_t LastAllocationAddress2;
        Block* Next;
    };
    Block* m_Blocks = nullptr;
    size_t m_TotalAllocs = 0;
    size_t m_TotalMemoryUsage = 0;

    // Returns true if the block was found, otherwise the block and address are undefined.
    bool FindBlock(size_t sizeInBytes, size_t alignment, Block*& block, size_t& address) const
    {
        for (block = m_Blocks; block; block = block->Next)
        {
            address = Align(block->CurAddress, alignment);
            if (address + sizeInBytes <= block->MaxAddress)
            {
                return true;
            }
        }
        return false;
    }

    Block* GetBlockByAddress(size_t address) const
    {
        for (Block* block = m_Blocks; block; block = block->Next)
        {
            if (address >= block->Address && address < block->MaxAddress)
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
    explicit FrameMemoryAllocator(const char* name = nullptr)
    {}

    FrameMemoryAllocator(FrameMemoryAllocator const& rhs)
    {}

    FrameMemoryAllocator(FrameMemoryAllocator const& x, const char* name)
    {}

    FrameMemoryAllocator& operator=(FrameMemoryAllocator const& rhs)
    {
        return *this;
    }

    void* allocate(size_t n, int flags = 0)
    {
        return sGetAllocator().Allocate(n, EASTL_SYSTEM_ALLOCATOR_MIN_ALIGNMENT);
    }

    void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
    {
        return sGetAllocator().Allocate(n, alignment);
    }

    void* reallocate(void* p, size_t n, bool copyOld)
    {
        return sGetAllocator().Reallocate(p, n, EASTL_SYSTEM_ALLOCATOR_MIN_ALIGNMENT, !copyOld);
    }

    void deallocate(void* p, size_t n)
    {
        sGetAllocator().TryFree(p);
    }

    void deallocate(void* p)
    {
        sGetAllocator().TryFree(p);
    }

    static LinearAllocator<>& sGetAllocator()
    {
        static LinearAllocator<> frameMemory;
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

        return static_cast<T*>(FrameMemoryAllocator::sGetAllocator().Allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        FrameMemoryAllocator::sGetAllocator().TryFree(p);
    }
};
template <typename T, typename U> bool operator==(TStdFrameAllocator<T> const&, TStdFrameAllocator<U> const&) { return true; }
template <typename T, typename U> bool operator!=(TStdFrameAllocator<T> const&, TStdFrameAllocator<U> const&) { return false; }

} // namespace Allocators

class FrameResource
{
public:
    virtual ~FrameResource() {}

    void* operator new(size_t sizeInBytes)
    {
        return Allocators::FrameMemoryAllocator::sGetAllocator().Allocate(sizeInBytes);
    }

    void operator delete(void* ptr)
    {
        Allocators::FrameMemoryAllocator::sGetAllocator().TryFree(ptr);
    }
};

HK_NAMESPACE_END
