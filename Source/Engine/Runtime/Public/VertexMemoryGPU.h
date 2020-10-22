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
#include <Core/Public/PodArray.h>
#include <RenderCore/Device.h>

constexpr size_t VERTEX_MEMORY_GPU_BLOCK_SIZE = 32<<20; // 32 MB
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_COUNT = 256;   // max memory VERTEX_MEMORY_GPU_BLOCK_SIZE * VERTEX_MEMORY_GPU_BLOCK_COUNT = 8 GB
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_INDEX_MASK = 0xff00000000000000;
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_INDEX_SHIFT = 56;
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK = 0x00ffffffffffffff;
constexpr int VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT = 32;

constexpr size_t STREAMED_MEMORY_GPU_BLOCK_SIZE = 32<<20; // 32 MB
constexpr int STREAMED_MEMORY_GPU_BUFFERS_COUNT = 3;    // required STREAMED_MEMORY_GPU_BLOCK_SIZE * STREAMED_MEMORY_GPU_BUFFERS_COUNT = 96 MB in use

constexpr int VERTEX_SIZE_ALIGN = 32;
constexpr int INDEX_SIZE_ALIGN = 16;
constexpr int JOINT_SIZE_ALIGN = 16;

typedef void *(*SGetMemoryCallback)( void * _UserPointer );

/** SVertexHandle holds internal data. Don't modify it outside of AVertexMemoryGPU */
struct SVertexHandle
{
    size_t Address;
    size_t Size;
    SGetMemoryCallback GetMemoryCB;
    void * UserPointer;

    /** Pack memory address */
    void MakeAddress( int BlockIndex, size_t Offset ) {
        AN_ASSERT( BlockIndex < VERTEX_MEMORY_GPU_BLOCK_COUNT );
        AN_ASSERT( Offset <= VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK );
        Address = ( ( size_t(BlockIndex) & 0xff ) << VERTEX_MEMORY_GPU_BLOCK_INDEX_SHIFT ) | ( Offset & VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK );
    }

    /** Unpack block index */
    int GetBlockIndex() const { return ( Address & VERTEX_MEMORY_GPU_BLOCK_INDEX_MASK ) >> VERTEX_MEMORY_GPU_BLOCK_INDEX_SHIFT; }

    /** Unpack offset in memory block */
    size_t GetBlockOffset() const { return Address & VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK; }

    /** Huge chunks are in separate GPU buffers */
    bool IsHuge() const { return Size > VERTEX_MEMORY_GPU_BLOCK_SIZE; }
};

class AVertexMemoryGPU : public ARefCounted {
    AN_FORBID_COPY( AVertexMemoryGPU )

public:
    /** Allow auto defragmentation */
    bool bAutoDefrag = true;

    /** Allow to allocate huge chunks > VERTEX_MEMORY_GPU_BLOCK_SIZE */
    bool bAllowHugeAllocs = true;

    /** Max blocks count. */
    uint8_t MaxBlocks = 0;

    AVertexMemoryGPU( RenderCore::IDevice * Device );

    ~AVertexMemoryGPU();

    /** Allocate vertex data */
    SVertexHandle * AllocateVertex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    /** Allocate index data */
    SVertexHandle * AllocateIndex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    /** Deallocate data */
    void Deallocate( SVertexHandle * _Handle );

    /** Update chunk data */
    void Update( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data );

    /** Memory defragmentation */
    void Defragment( bool bDeallocateEmptyBlocks, bool bForceUpload );

    /** GPU buffer and offset from handle */
    void GetPhysicalBufferAndOffset( SVertexHandle * _Handle, RenderCore::IBuffer ** _Buffer, size_t * _Offset );

    /** Total allocated GPU memory for blocks */
    size_t GetAllocatedMemory() const { return Blocks.Size() * VERTEX_MEMORY_GPU_BLOCK_SIZE; }

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

private:
    /** Find a free block */
    int FindBlock( size_t _RequiredSize );

    /** Allocate function */
    SVertexHandle * Allocate( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    /** Allocate a separate huge chunk > VERTEX_MEMORY_GPU_BLOCK_SIZE */
    SVertexHandle * AllocateHuge( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer );

    /** Deallocate separate huge chunk */
    void DeallocateHuge( SVertexHandle * _Handle );

    /** Update vertex data */
    void UpdateHuge( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data );

    /** Upload data to GPU */
    void UploadBuffers();

    /** Upload data to GPU */
    void UploadBuffersHuge();

    /** Add a new GPU buffer */
    void AddGPUBuffer();

    /** Check leaks */
    void CheckMemoryLeaks();

    struct SBlock {
        size_t AllocOffset;
        size_t UsedMemory;
    };

    TRef< RenderCore::IDevice > RenderDevice;
    TPodArray< SVertexHandle * > Handles;
    TPodArray< SVertexHandle * > HugeHandles;
    TPodArray< SBlock > Blocks;
    TStdVector< TRef< RenderCore::IBuffer > > BufferHandles;
    TPoolAllocator< SVertexHandle > HandlePool;

    size_t UsedMemory;
    size_t UsedMemoryHuge;
};

class AStreamedMemoryGPU : public ARefCounted {
    AN_FORBID_COPY( AStreamedMemoryGPU )

public:
    AStreamedMemoryGPU( RenderCore::IDevice * Device );

    ~AStreamedMemoryGPU();

    /** Allocate vertex data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateVertex( size_t _SizeInBytes, const void * _Data = nullptr );

    /** Allocate index data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateIndex( size_t _SizeInBytes, const void * _Data = nullptr );

    /** Allocate joint data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateJoint( size_t _SizeInBytes, const void * _Data = nullptr );

    /** Allocate constant data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateConstant( size_t _SizeInBytes, const void * _Data = nullptr );

    /** Change size of last allocated memory block */
    void ShrinkLastAllocatedMemoryBlock( size_t _SizeInBytes );

    /** Map data. Mapped data is actual during current frame. */
    void * Map( size_t _StreamHandle );

    /** Get physical buffer and offset */
    void GetPhysicalBufferAndOffset( size_t _StreamHandle, RenderCore::IBuffer ** _Buffer, size_t * _Offset );

    /** Get physical buffer */
    RenderCore::IBuffer * GetBufferGPU();

    /** Internal. Wait buffer before filling. */
    void Wait();

    /** Internal. Swap write buffers. */
    void Swap();

    /** Get total allocated memory */
    size_t GetAllocatedMemory() const { return STREAMED_MEMORY_GPU_BLOCK_SIZE; }

    /** Get total used memory */
    size_t GetUsedMemory() const { return ChainBuffer[BufferIndex].UsedMemory; }

    /** Get total used memory on previous frame */
    size_t GetUsedMemoryPrev() const { return ChainBuffer[(BufferIndex+STREAMED_MEMORY_GPU_BUFFERS_COUNT-1)%STREAMED_MEMORY_GPU_BUFFERS_COUNT].UsedMemory; }

    /** Get free memory */
    size_t GetUnusedMemory() const { return GetAllocatedMemory() - GetUsedMemory(); }

    /** Get max memory usage since initialization */
    size_t GetMaxMemoryUsage() const { return MaxMemoryUsage; }

    /** Get stream handles count */
    int GetHandlesCount() const { return ChainBuffer[BufferIndex].HandlesCount; }

private:
    size_t Allocate( size_t _SizeInBytes, int _Alignment, const void * _Data );

    void Wait( RenderCore::SyncObject Sync );

    struct SChainBuffer {
        size_t UsedMemory;
        int HandlesCount;
        RenderCore::SyncObject Sync;
    };

    TRef< RenderCore::IDevice > RenderDevice;
    RenderCore::IImmediateContext * pImmediateContext;
    SChainBuffer ChainBuffer[STREAMED_MEMORY_GPU_BUFFERS_COUNT];
    TRef< RenderCore::IBuffer > Buffer;
    void * pMappedMemory;
    int BufferIndex;
    size_t MaxMemoryUsage;
    size_t LastAllocatedBlockSize;
    int VertexBufferAlignment;
    int IndexBufferAlignment;
    int ConstantBufferAlignment;
};
