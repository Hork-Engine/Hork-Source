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

#include <Engine/Core/Allocators/PoolAllocator.h>
#include <Engine/Core/Containers/Vector.h>
#include <Engine/RenderCore/Device.h>

HK_NAMESPACE_BEGIN

constexpr size_t VERTEX_MEMORY_GPU_BLOCK_SIZE             = 32 << 20; // 32 MB
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_COUNT            = 256;      // max memory VERTEX_MEMORY_GPU_BLOCK_SIZE * VERTEX_MEMORY_GPU_BLOCK_COUNT = 8 GB
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_INDEX_MASK       = 0xff00000000000000;
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_INDEX_SHIFT      = 56;
constexpr size_t VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK      = 0x00ffffffffffffff;
constexpr int    VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT = 32;

constexpr size_t STREAMED_MEMORY_GPU_BLOCK_SIZE    = 32 << 20; // 32 MB
constexpr int    STREAMED_MEMORY_GPU_BUFFERS_COUNT = 3;        // required STREAMED_MEMORY_GPU_BLOCK_SIZE * STREAMED_MEMORY_GPU_BUFFERS_COUNT = 96 MB in use

constexpr int VERTEX_SIZE_ALIGN = 32;
constexpr int INDEX_SIZE_ALIGN  = 16;
constexpr int JOINT_SIZE_ALIGN  = 16;

typedef void* (*GetMemoryCallback)(void* _UserPointer);

/** VertexHandle holds internal data. Don't modify it outside of VertexMemoryGPU */
struct VertexHandle
{
    size_t            Address;
    size_t            Size;
    GetMemoryCallback GetMemoryCB;
    void*             UserPointer;

    /** Pack memory address */
    void MakeAddress(int BlockIndex, size_t Offset)
    {
        HK_ASSERT(BlockIndex < VERTEX_MEMORY_GPU_BLOCK_COUNT);
        HK_ASSERT(Offset <= VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK);
        Address = ((size_t(BlockIndex) & 0xff) << VERTEX_MEMORY_GPU_BLOCK_INDEX_SHIFT) | (Offset & VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK);
    }

    /** Unpack block index */
    int GetBlockIndex() const { return (Address & VERTEX_MEMORY_GPU_BLOCK_INDEX_MASK) >> VERTEX_MEMORY_GPU_BLOCK_INDEX_SHIFT; }

    /** Unpack offset in memory block */
    size_t GetBlockOffset() const { return Address & VERTEX_MEMORY_GPU_BLOCK_OFFSET_MASK; }

    /** Huge chunks are in separate GPU buffers */
    bool IsHuge() const { return Size > VERTEX_MEMORY_GPU_BLOCK_SIZE; }
};

class VertexMemoryGPU final : public Noncopyable
{
public:
    /** Allow auto defragmentation */
    bool bAutoDefrag = true;

    /** Allow to allocate huge chunks > VERTEX_MEMORY_GPU_BLOCK_SIZE */
    bool bAllowHugeAllocs = true;

    /** Max blocks count. */
    uint8_t MaxBlocks = 0;

    VertexMemoryGPU(RenderCore::IDevice* pDevice);

    ~VertexMemoryGPU();

    /** Allocate vertex data */
    VertexHandle* AllocateVertex(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer);

    /** Allocate index data */
    VertexHandle* AllocateIndex(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer);

    /** Deallocate data */
    void Deallocate(VertexHandle* _Handle);

    /** Update chunk data */
    void Update(VertexHandle* _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void* _Data);

    /** Memory defragmentation */
    void Defragment(bool bDeallocateEmptyBlocks, bool bForceUpload);

    /** GPU buffer and offset from handle */
    void GetPhysicalBufferAndOffset(VertexHandle* _Handle, RenderCore::IBuffer** _Buffer, size_t* _Offset);

    /** Total allocated GPU memory for blocks */
    size_t GetAllocatedMemory() const { return m_Blocks.Size() * VERTEX_MEMORY_GPU_BLOCK_SIZE; }

    /** Used memory */
    size_t GetUsedMemory() const { return m_UsedMemory; }

    /** Unused memory */
    size_t GetUnusedMemory() const { return GetAllocatedMemory() - GetUsedMemory(); }

    /** Used memory for huge chunks */
    size_t GetUsedMemoryHuge() const { return m_UsedMemoryHuge; }

    /** Total handles for chunks */
    int GetHandlesCount() const { return m_Handles.Size(); }

    /** Total handles for huge chunks */
    int GetHandlesCountHuge() const { return m_HugeHandles.Size(); }

    /** Total handles for all chunks */
    int GetTotalHandles() const { return GetHandlesCount() + GetHandlesCountHuge(); }

    /** Total block count */
    int GetBlocksCount() const { return m_Blocks.Size(); }

private:
    /** Find a free block */
    int FindBlock(size_t _RequiredSize);

    /** Allocate function */
    VertexHandle* Allocate(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer);

    /** Allocate a separate huge chunk > VERTEX_MEMORY_GPU_BLOCK_SIZE */
    VertexHandle* AllocateHuge(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer);

    /** Deallocate separate huge chunk */
    void DeallocateHuge(VertexHandle* _Handle);

    /** Update vertex data */
    void UpdateHuge(VertexHandle* _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void* _Data);

    /** Upload data to GPU */
    void UploadBuffers();

    /** Upload data to GPU */
    void UploadBuffersHuge();

    /** Add a new GPU buffer */
    void AddGPUBuffer();

    /** Check leaks */
    void CheckMemoryLeaks();

    struct Block
    {
        size_t AllocOffset;
        size_t UsedMemory;
    };

    Ref<RenderCore::IDevice> m_pDevice;
    Vector<VertexHandle*> m_Handles;
    Vector<VertexHandle*> m_HugeHandles;
    Vector<Block> m_Blocks;
    Vector<Ref<RenderCore::IBuffer>> m_BufferHandles;
    PoolAllocator<VertexHandle> m_HandlePool;

    size_t m_UsedMemory;
    size_t m_UsedMemoryHuge;
};

class StreamedMemoryGPU final : public Noncopyable
{
public:
    explicit StreamedMemoryGPU(RenderCore::IDevice* Device);
    ~StreamedMemoryGPU();

    /** Allocate vertex data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateVertex(size_t _SizeInBytes, const void* _Data = nullptr);

    /** Allocate index data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateIndex(size_t _SizeInBytes, const void* _Data = nullptr);

    /** Allocate joint data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateJoint(size_t _SizeInBytes, const void* _Data = nullptr);

    /** Allocate constant data. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateConstant(size_t _SizeInBytes, const void* _Data = nullptr);

    /** Allocate data with custum alignment. Return stream handle. Stream handle is actual during current frame. */
    size_t AllocateWithCustomAlignment(size_t _SizeInBytes, int _Alignment, const void* _Data = nullptr);

    /** Change size of last allocated memory block */
    void ShrinkLastAllocatedMemoryBlock(size_t _SizeInBytes);

    /** Map data. Mapped data is actual during current frame. */
    void* Map(size_t _StreamHandle);

    /** Get physical buffer and offset */
    void GetPhysicalBufferAndOffset(size_t StreamHandle, RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Get physical buffer */
    RenderCore::IBuffer* GetBufferGPU();

    /** Internal. Wait buffer before filling. */
    void Wait();

    /** Internal. Swap write buffers. */
    void Swap();

    /** Get total allocated memory */
    size_t GetAllocatedMemory() const { return STREAMED_MEMORY_GPU_BLOCK_SIZE; }

    /** Get total used memory */
    size_t GetUsedMemory() const { return m_ChainBuffer[m_BufferIndex].UsedMemory; }

    /** Get total used memory on previous frame */
    size_t GetUsedMemoryPrev() const { return m_ChainBuffer[(m_BufferIndex + STREAMED_MEMORY_GPU_BUFFERS_COUNT - 1) % STREAMED_MEMORY_GPU_BUFFERS_COUNT].UsedMemory; }

    /** Get free memory */
    size_t GetUnusedMemory() const { return GetAllocatedMemory() - GetUsedMemory(); }

    /** Get max memory usage since initialization */
    size_t GetMaxMemoryUsage() const { return m_MaxMemoryUsage; }

    /** Get stream handles count */
    int GetHandlesCount() const { return m_ChainBuffer[m_BufferIndex].HandlesCount; }

private:
    size_t Allocate(size_t _SizeInBytes, int _Alignment, const void* _Data);

    void Wait(RenderCore::SyncObject Sync);

    struct ChainBuffer
    {
        size_t                 UsedMemory;
        int                    HandlesCount;
        RenderCore::SyncObject Sync;
    };

    Ref<RenderCore::IDevice>  m_pDevice;
    RenderCore::IImmediateContext* m_pImmediateContext;
    ChainBuffer               m_ChainBuffer[STREAMED_MEMORY_GPU_BUFFERS_COUNT];
    Ref<RenderCore::IBuffer>  m_Buffer;
    void*                     m_pMappedMemory;
    int                       m_BufferIndex;
    size_t                    m_MaxMemoryUsage;
    size_t                    m_LastAllocatedBlockSize;
    int                       m_VertexBufferAlignment;
    int                       m_IndexBufferAlignment;
    int                       m_ConstantBufferAlignment;
};

HK_NAMESPACE_END
