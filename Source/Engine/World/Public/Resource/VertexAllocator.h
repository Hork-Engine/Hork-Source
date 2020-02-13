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

#include <Core/Public/PoolAllocator.h>
#include <Runtime/Public/RenderCore.h>

constexpr size_t VERTEX_ALLOCATOR_BLOCK_SIZE = 32<<20; // 32 MB

typedef void *(*SGetMemoryCallback)( void * _UserPointer );

constexpr size_t MAX_BLOCK_COUNT = 256;
constexpr size_t BLOCK_INDEX_MASK = 0xff00000000000000;
constexpr size_t BLOCK_INDEX_SHIFT = 56;
constexpr size_t BLOCK_OFFSET_MASK = 0x00ffffffffffffff;
constexpr int VERTEX_SIZE_ALIGN = 32;
constexpr int INDEX_SIZE_ALIGN = 16;
constexpr int JOINT_SIZE_ALIGN = 16;
constexpr int CHUNK_OFFSET_ALIGNMENT = 32;

struct SVertexHandle {
    size_t Address;
    size_t Size;
    SGetMemoryCallback GetMemoryCB;
    void * UserPointer;

    /** Pack memory address */
    void MakeAddress( int BlockIndex, size_t Offset ) {
        AN_ASSERT( BlockIndex < MAX_BLOCK_COUNT );
        AN_ASSERT( Offset <= BLOCK_OFFSET_MASK );
        Address = ( ( size_t(BlockIndex) & 0xff ) << BLOCK_INDEX_SHIFT ) | ( Offset & BLOCK_OFFSET_MASK );
    }

    /** Unpack block index */
    int GetBlockIndex() const { return ( Address & BLOCK_INDEX_MASK ) >> BLOCK_INDEX_SHIFT; }

    /** Unpack offset in memory block */
    size_t GetBlockOffset() const { return Address & BLOCK_OFFSET_MASK; }

    bool IsHuge() const { return Size > VERTEX_ALLOCATOR_BLOCK_SIZE; }
};

class AVertexAllocator : public IGPUResourceOwner {
    AN_FORBID_COPY( AVertexAllocator )

public:
    /** Allow auto defragmentation */
    bool bAutoDefrag = true;

    /** Allow to allocate huge chunks > VERTEX_ALLOCATOR_BLOCK_SIZE */
    bool bAllowHugeAllocs = true;

    /** Max blocks count. */
    uint8_t MaxBlocks = 0;

    AVertexAllocator();

    ~AVertexAllocator();

    void Initialize();

    void Purge();

    /** Allocate vertex data */
    SVertexHandle * AllocateVertex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    /** Allocate index data */
    SVertexHandle * AllocateIndex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    /** Deallocate data */
    void Deallocate( SVertexHandle * _Handle );

    /** Update chunk data */
    void Update( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data );

    /** Memory defragmentation */
    void Defragment( bool bDeallocateEmptyBlocks );

    /** GPU buffer and offset from handle */
    void GetHandleBuffer( SVertexHandle * _Handle, ABufferGPU ** _Buffer, size_t * _Offset );

    /** Total allocated GPU memory for blocks */
    size_t GetAllocatedMemory() const { return Blocks.Size() * VERTEX_ALLOCATOR_BLOCK_SIZE; }

    /** Used memory */
    size_t GetUsedMemory() const { return UsedMemory; }

    /** Unused memory */
    size_t GetUnusedMemory() const { return GetAllocatedMemory() - GetUsedMemory(); }

    /** Used memory for huge chunks */
    size_t GetUsedMemoryHuge() const { return UsedMemoryHuge; }

    /** Total handles for chunks */
    int GetHandlesCount() const { return Handles.Size(); }

    /** Total handles for huge chunks */
    int GetHandlesCountHuge() const { return HugeHandles.Size(); }

    /** Total handles for all chunks */
    int GetTotalHandles() const { return GetHandlesCount() + GetHandlesCountHuge(); }

    /** Total block count */
    int GetBlocksCount() const { return Blocks.Size(); }

protected:
    /** IGPUResourceOwner interface */
    void UploadResourcesGPU() override;

private:

    int FindBlock( size_t _RequiredSize );

    SVertexHandle * Allocate( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    SVertexHandle * AllocateHuge( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    void DeallocateHuge( SVertexHandle * _Handle );

    void UpdateHuge( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data );

    void UploadBuffers();

    void UploadBuffersHuge();

    void AddGPUBuffer();

    struct SBlock {
        size_t AllocOffset;
        size_t UsedMemory;
    };

    TPodArray< SVertexHandle * > Handles;
    TPodArray< SVertexHandle * > HugeHandles;
    TPodArray< SBlock > Blocks;
    TPodArray< ABufferGPU * > BufferHandles;
    TPoolAllocator< SVertexHandle > HandlePool;

    size_t UsedMemory;
    size_t UsedMemoryHuge;
};


constexpr size_t DYNAMIC_VERTEX_ALLOCATOR_BLOCK_SIZE = 32<<20; // 32 MB

class ADynamicVertexAllocator : public IGPUResourceOwner {
    AN_FORBID_COPY( ADynamicVertexAllocator )

public:
    ADynamicVertexAllocator();

    ~ADynamicVertexAllocator();

    void Initialize();

    void Purge();

    size_t AllocateVertex( size_t _SizeInBytes, const void * _Data );

    size_t AllocateIndex( size_t _SizeInBytes, const void * _Data );

    size_t AllocateJoint( size_t _SizeInBytes, const void * _Data );

    int GetVertexBufferAlignment() const;

    int GetIndexBufferAlignment() const;

    int GetJointBufferAlignment() const;

    void Update( size_t _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data );

    void GetHandleBuffer( size_t _Handle, ABufferGPU ** _Buffer, size_t * _Offset );

    void SwapFrames();

    size_t GetAllocatedMemory() const { return DYNAMIC_VERTEX_ALLOCATOR_BLOCK_SIZE; }

    size_t GetUsedMemory() const { return FrameData[FrameWrite].UsedMemory; }

    size_t GetUnusedMemory() const { return GetAllocatedMemory() - GetUsedMemory(); }

    size_t GetMaxMemoryUsage() const { return MaxMemoryUsage; }

    int GetHandlesCount() const { return FrameData[FrameWrite].HandlesCount; }

protected:
    /** IGPUResourceOwner interface */
    void UploadResourcesGPU() override;

private:
    size_t Allocate( size_t _SizeInBytes, int _Alignment, const void * _Data );

    struct SFrameData {
        ABufferGPU * Buffer;
        size_t UsedMemory;
        int HandlesCount;
    };

    SFrameData FrameData[2];
    int FrameWrite;
    size_t MaxMemoryUsage;
};

extern AVertexAllocator GVertexAllocator;
extern ADynamicVertexAllocator GDynamicVertexAllocator;
