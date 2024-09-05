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

#include "VertexMemoryGPU.h"
#include <Engine/Core/Platform.h>

HK_NAMESPACE_BEGIN

VertexMemoryGPU::VertexMemoryGPU(RenderCore::IDevice* pDevice) :
    m_pDevice(pDevice)
{
    m_UsedMemory = 0;
    m_UsedMemoryHuge = 0;
}

VertexMemoryGPU::~VertexMemoryGPU()
{
    CheckMemoryLeaks();
    for (VertexHandle* handle : m_HugeHandles)
    {
        RenderCore::IBuffer* buffer = (RenderCore::IBuffer*)handle->Address;
        buffer->RemoveRef();
    }
}

VertexHandle* VertexMemoryGPU::AllocateVertex(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer)
{
    //HK_ASSERT( IsAligned< VERTEX_SIZE_ALIGN >( _SizeInBytes ) );
    return Allocate(_SizeInBytes, _Data, _GetMemoryCB, _UserPointer);
}

VertexHandle* VertexMemoryGPU::AllocateIndex(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer)
{
    //HK_ASSERT( IsAligned< INDEX_SIZE_ALIGN >( _SizeInBytes ) );
    return Allocate(_SizeInBytes, _Data, _GetMemoryCB, _UserPointer);
}

void VertexMemoryGPU::Deallocate(VertexHandle* _Handle)
{
    if (!_Handle)
    {
        return;
    }

    if (_Handle->Size > VERTEX_MEMORY_GPU_BLOCK_SIZE)
    {
        DeallocateHuge(_Handle);
        return;
    }

    //LOG( "Deallocated buffer at block {}, offset {}, size {}\n", _Handle->GetBlockIndex(), _Handle->GetBlockOffset(), _Handle->Size );

    Block* block = &m_Blocks[_Handle->GetBlockIndex()];

    size_t chunkSize = Align(_Handle->Size, VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT);

    block->UsedMemory -= chunkSize;

    HK_ASSERT(block->UsedMemory >= 0);

    if (block->AllocOffset == _Handle->GetBlockOffset() + chunkSize)
    {
        block->AllocOffset -= chunkSize;
    }

    if (block->UsedMemory == 0)
    {
        block->AllocOffset = 0;
    }

    m_UsedMemory -= chunkSize;

    m_Handles.RemoveUnsorted(m_Handles.IndexOf(_Handle));

    m_HandlePool.Deallocate(_Handle);
}

void VertexMemoryGPU::Update(VertexHandle* _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void* _Data)
{
    if (_Handle->Size > VERTEX_MEMORY_GPU_BLOCK_SIZE)
    {
        UpdateHuge(_Handle, _ByteOffset, _SizeInBytes, _Data);
        return;
    }

    m_BufferHandles[_Handle->GetBlockIndex()]->WriteRange(_Handle->GetBlockOffset() + _ByteOffset, _SizeInBytes, _Data);
}

void VertexMemoryGPU::Defragment(bool bDeallocateEmptyBlocks, bool bForceUpload)
{
    struct
    {
        bool operator()(VertexHandle const* A, VertexHandle const* B)
        {
            return A->Size > B->Size;
        }
    } cmp;

    std::sort(m_Handles.Begin(), m_Handles.End(), cmp);

    // NOTE: We can allocate new GPU buffers for blocks and just copy buffer-to-buffer on GPU side and then deallocate old buffers.
    // It is gonna be faster than CPU->GPU transition and get rid of implicit synchroization on the driver, but takes more memory.

    m_Blocks.Clear();

    for (int i = 0; i < m_Handles.Size(); i++)
    {
        VertexHandle* handle = m_Handles[i];

        size_t handleSize        = handle->Size;
        int    handleBlockIndex  = handle->GetBlockIndex();
        size_t handleBlockOffset = handle->GetBlockOffset();
        size_t chunkSize         = Align(handleSize, VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT);

        bool bRelocated = false;
        for (int j = 0; j < m_Blocks.Size(); j++)
        {
            if (m_Blocks[j].AllocOffset + handleSize <= VERTEX_MEMORY_GPU_BLOCK_SIZE)
            {
                if (handleBlockIndex != j || handleBlockOffset != m_Blocks[j].AllocOffset || bForceUpload)
                {
                    handle->MakeAddress(j, m_Blocks[j].AllocOffset);

                    m_BufferHandles[handle->GetBlockIndex()]->WriteRange(handle->GetBlockOffset(), handle->Size, handle->GetMemoryCB(handle->UserPointer));
                }
                m_Blocks[j].AllocOffset += chunkSize;
                m_Blocks[j].UsedMemory += chunkSize;
                bRelocated = true;
                break;
            }
        }
        if (!bRelocated)
        {
            int blockIndex = m_Blocks.Size();
            size_t offset = 0;

            //if ( bForceUpload ) {
            //    // If buffers are in use driver will allocate a new storage and implicit synchroization will not occur.
            //    BufferHandles[blockIndex]->Orphan();
            //}

            if (handleBlockIndex != blockIndex || handleBlockOffset != offset || bForceUpload)
            {
                handle->MakeAddress(blockIndex, offset);

                m_BufferHandles[blockIndex]->WriteRange(offset, handle->Size, handle->GetMemoryCB(handle->UserPointer));
            }

            Block newBlock;
            newBlock.AllocOffset = chunkSize;
            newBlock.UsedMemory  = chunkSize;
            m_Blocks.Add(newBlock);
        }
    }

    int freeBuffers = m_BufferHandles.Size() - m_Blocks.Size();
    if (freeBuffers > 0)
    {
        if (bDeallocateEmptyBlocks)
        {
            // Destroy and deallocate unused GPU buffers
            //for ( int i = 0 ; i < freeBuffers ; i++ ) {
            //    m_BufferHandles[ m_Blocks.Size() + i ].Reset();
            //}
            m_BufferHandles.Resize(m_Blocks.Size());
        }
        else
        {
            // Emit empty blocks
            int firstBlock = m_Blocks.Size();
            m_Blocks.Resize(m_BufferHandles.Size());
            for (int i = 0; i < freeBuffers; i++)
            {
                m_Blocks[firstBlock + i].AllocOffset = 0;
                m_Blocks[firstBlock + i].UsedMemory = 0;
            }
        }
    }
}

void VertexMemoryGPU::GetPhysicalBufferAndOffset(VertexHandle* _Handle, RenderCore::IBuffer** _Buffer, size_t* _Offset)
{
    if (_Handle->IsHuge())
    {
        *_Buffer = (RenderCore::IBuffer*)_Handle->Address;
        *_Offset = 0;
        return;
    }

    *_Buffer = m_BufferHandles[_Handle->GetBlockIndex()];
    *_Offset = _Handle->GetBlockOffset();
}

int VertexMemoryGPU::FindBlock(size_t _RequiredSize)
{
    for (int i = 0; i < m_Blocks.Size(); i++)
    {
        if (m_Blocks[i].AllocOffset + _RequiredSize <= VERTEX_MEMORY_GPU_BLOCK_SIZE)
        {
            return i;
        }
    }
    return -1;
}

VertexHandle* VertexMemoryGPU::Allocate(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer)
{
    if (_SizeInBytes > VERTEX_MEMORY_GPU_BLOCK_SIZE)
    {
        // Huge block

        if (!bAllowHugeAllocs)
        {
            CoreApplication::sTerminateWithError("VertexMemoryGPU::Allocate: huge alloc {} bytes\n", _SizeInBytes);
        }

        return AllocateHuge(_SizeInBytes, _Data, _GetMemoryCB, _UserPointer);
    }

    int i = FindBlock(_SizeInBytes);

    const int AUTO_DEFRAG_FACTOR = (MaxBlocks == 1) ? 1 : 8;

    // If block was not found, try to defragmentate memory
    if (i == -1 && bAutoDefrag && GetUnusedMemory() >= _SizeInBytes * AUTO_DEFRAG_FACTOR)
    {

        bool bDeallocateEmptyBlocks = false;
        bool bForceUpload           = false;

        Defragment(bDeallocateEmptyBlocks, bForceUpload);

        i = FindBlock(_SizeInBytes);
    }

    Block* block = i == -1 ? nullptr : &m_Blocks[i];

    if (!block)
    {

        if (MaxBlocks && m_Blocks.Size() >= MaxBlocks)
        {
            CoreApplication::sTerminateWithError("VertexMemoryGPU::Allocate: failed on allocation of {} bytes\n", _SizeInBytes);
        }

        Block newBlock;
        newBlock.AllocOffset = 0;
        newBlock.UsedMemory  = 0;
        m_Blocks.Add(newBlock);

        i = m_Blocks.Size() - 1;

        block = &m_Blocks[i];

        AddGPUBuffer();
    }

    VertexHandle* handle = m_HandlePool.Allocate();

    handle->MakeAddress(i, block->AllocOffset);
    handle->Size        = _SizeInBytes;
    handle->GetMemoryCB = _GetMemoryCB;
    handle->UserPointer = _UserPointer;

    m_Handles.Add(handle);

    size_t chunkSize = Align(_SizeInBytes, VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT);

    block->AllocOffset += chunkSize;
    block->UsedMemory += chunkSize;

    m_UsedMemory += chunkSize;

    if (_Data)
    {
        m_BufferHandles[handle->GetBlockIndex()]->WriteRange(handle->GetBlockOffset(), handle->Size, _Data);
    }

    //LOG( "Allocated buffer at block {}, offset {}, size {}\n", handle->GetBlockIndex(), handle->GetBlockOffset(), handle->Size );

    return handle;
}

VertexHandle* VertexMemoryGPU::AllocateHuge(size_t _SizeInBytes, const void* _Data, GetMemoryCallback _GetMemoryCB, void* _UserPointer)
{
    VertexHandle* handle = m_HandlePool.Allocate();

    handle->Size        = _SizeInBytes;
    handle->GetMemoryCB = _GetMemoryCB;
    handle->UserPointer = _UserPointer;

    RenderCore::BufferDesc bufferCI = {};
    bufferCI.SizeInBytes             = _SizeInBytes;
    // Mutable storage with flag MUTABLE_STORAGE_STATIC is much faster during rendering than immutable (tested on NVidia GeForce GTX 770)
    bufferCI.MutableClientAccess = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    bufferCI.MutableUsage        = RenderCore::MUTABLE_STORAGE_STATIC;

    Ref<RenderCore::IBuffer> buffer;
    m_pDevice->CreateBuffer(bufferCI, _Data, &buffer);
    buffer->SetDebugName("Vertex memory HUGE buffer");

    RenderCore::IBuffer* pBuffer = buffer.RawPtr();
    pBuffer->AddRef();

    handle->Address = (size_t)pBuffer;

    m_UsedMemoryHuge += _SizeInBytes;

    m_HugeHandles.Add(handle);

    return handle;
}

void VertexMemoryGPU::DeallocateHuge(VertexHandle* _Handle)
{
    m_UsedMemoryHuge -= _Handle->Size;

    RenderCore::IBuffer* buffer = (RenderCore::IBuffer*)_Handle->Address;
    buffer->RemoveRef();

    m_HugeHandles.RemoveUnsorted(m_HugeHandles.IndexOf(_Handle));

    m_HandlePool.Deallocate(_Handle);
}

void VertexMemoryGPU::UpdateHuge(VertexHandle* _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void* _Data)
{
    ((RenderCore::IBuffer*)_Handle->Address)->WriteRange(_ByteOffset, _SizeInBytes, _Data);
}

void VertexMemoryGPU::UploadBuffers()
{
    // We not only upload the buffer data, we also perform defragmentation here.

    bool bDeallocateEmptyBlocks = true;
    bool bForceUpload           = true;

    Defragment(bDeallocateEmptyBlocks, bForceUpload);
}

void VertexMemoryGPU::UploadBuffersHuge()
{
    for (int i = 0; i < m_HugeHandles.Size(); i++)
    {
        VertexHandle* handle = m_HugeHandles[i];
        ((RenderCore::IBuffer*)handle->Address)->WriteRange(0, handle->Size, handle->GetMemoryCB(handle->UserPointer));
    }
}

void VertexMemoryGPU::AddGPUBuffer()
{
    // Create GPU buffer

    RenderCore::BufferDesc bufferCI = {};
    bufferCI.SizeInBytes             = VERTEX_MEMORY_GPU_BLOCK_SIZE;
    // Mutable storage with flag MUTABLE_STORAGE_STATIC is much faster during rendering than immutable (tested on NVidia GeForce GTX 770)
    bufferCI.MutableClientAccess = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    bufferCI.MutableUsage        = RenderCore::MUTABLE_STORAGE_STATIC;

    Ref<RenderCore::IBuffer> buffer;
    m_pDevice->CreateBuffer(bufferCI, nullptr, &buffer);

    buffer->SetDebugName("Vertex memory block buffer");

    m_BufferHandles.Add(buffer);

    //LOG("Allocated a new block (total blocks {})\n", m_BufferHandles.Size());
}

void VertexMemoryGPU::CheckMemoryLeaks()
{
    for (VertexHandle* handle : m_Handles)
    {
        LOG("==== Vertex Memory Leak ====\n");
        LOG("Chunk Address: {} Size: {}\n", handle->Address, handle->Size);
    }
    for (VertexHandle* handle : m_HugeHandles)
    {
        LOG("==== Vertex Memory Leak ====\n");
        LOG("Chunk Address: {} Size: {} (Huge)\n", handle->Address, handle->Size);
    }
}

StreamedMemoryGPU::StreamedMemoryGPU(RenderCore::IDevice* pDevice) :
    m_pDevice(pDevice), m_pImmediateContext(pDevice->GetImmediateContext())
{
    RenderCore::BufferDesc bufferCI = {};

    bufferCI.SizeInBytes = STREAMED_MEMORY_GPU_BLOCK_SIZE * STREAMED_MEMORY_GPU_BUFFERS_COUNT;

    bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)(RenderCore::IMMUTABLE_MAP_WRITE | RenderCore::IMMUTABLE_MAP_PERSISTENT | RenderCore::IMMUTABLE_MAP_COHERENT);
    bufferCI.bImmutableStorage     = true;

    pDevice->CreateBuffer(bufferCI, nullptr, &m_Buffer);

    m_Buffer->SetDebugName("Streamed memory buffer");

    m_pMappedMemory = m_pImmediateContext->MapBuffer(m_Buffer,
                                                 RenderCore::MAP_TRANSFER_WRITE,
                                                 RenderCore::MAP_NO_INVALIDATE, //RenderCore::MAP_INVALIDATE_ENTIRE_BUFFER,
                                                 RenderCore::MAP_PERSISTENT_COHERENT,
                                                 false, // flush explicit
                                                 false  // unsynchronized
    );

    if (!m_pMappedMemory)
    {
        CoreApplication::sTerminateWithError("StreamedMemoryGPU::Initialize: cannot initialize persistent mapped buffer size {}\n", bufferCI.SizeInBytes);
    }

    Core::ZeroMem(m_ChainBuffer, sizeof(m_ChainBuffer));

    m_BufferIndex = 0;
    m_MaxMemoryUsage = 0;
    m_LastAllocatedBlockSize = 0;

    m_VertexBufferAlignment = 32; // TODO: Get from driver!!!
    m_IndexBufferAlignment = 16;  // TODO: Get from driver!!!
    m_ConstantBufferAlignment = pDevice->GetDeviceCaps(RenderCore::DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT);
}

StreamedMemoryGPU::~StreamedMemoryGPU()
{
    for (int i = 0; i < STREAMED_MEMORY_GPU_BUFFERS_COUNT; i++)
    {
        ChainBuffer* pChainBuffer = &m_ChainBuffer[i];

        Wait(pChainBuffer->Sync);
        m_pImmediateContext->RemoveSync(pChainBuffer->Sync);
    }

    if (m_Buffer)
    {
        m_pImmediateContext->UnmapBuffer(m_Buffer);
    }
}

size_t StreamedMemoryGPU::AllocateVertex(size_t _SizeInBytes, const void* _Data)
{
    return Allocate(_SizeInBytes, m_VertexBufferAlignment, _Data);
}

size_t StreamedMemoryGPU::AllocateIndex(size_t _SizeInBytes, const void* _Data)
{
    return Allocate(_SizeInBytes, m_IndexBufferAlignment, _Data);
}

size_t StreamedMemoryGPU::AllocateJoint(size_t _SizeInBytes, const void* _Data)
{
    return Allocate(_SizeInBytes, m_ConstantBufferAlignment, _Data);
}

size_t StreamedMemoryGPU::AllocateConstant(size_t _SizeInBytes, const void* _Data)
{
    return Allocate(_SizeInBytes, m_ConstantBufferAlignment, _Data);
}

size_t StreamedMemoryGPU::AllocateWithCustomAlignment(size_t _SizeInBytes, int _Alignment, const void* _Data)
{
    return Allocate(_SizeInBytes, _Alignment, _Data);
}

void* StreamedMemoryGPU::Map(size_t _StreamHandle)
{
    return (byte*)m_pMappedMemory + _StreamHandle;
}

void StreamedMemoryGPU::GetPhysicalBufferAndOffset(size_t StreamHandle, RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    *ppBuffer = m_Buffer;
    *pOffset = StreamHandle;
}

RenderCore::IBuffer* StreamedMemoryGPU::GetBufferGPU()
{
    return m_Buffer;
}

void StreamedMemoryGPU::Wait(RenderCore::SyncObject Sync)
{
    const uint64_t timeOutNanoseconds = 1;
    if (Sync)
    {
        RenderCore::CLIENT_WAIT_STATUS status;
        do {
            status = m_pImmediateContext->ClientWait(Sync, timeOutNanoseconds);
        } while (status != RenderCore::CLIENT_WAIT_ALREADY_SIGNALED && status != RenderCore::CLIENT_WAIT_CONDITION_SATISFIED);
    }
}

void StreamedMemoryGPU::Wait()
{
    Wait(m_ChainBuffer[m_BufferIndex].Sync);
}

void StreamedMemoryGPU::Swap()
{
    m_pImmediateContext->RemoveSync(m_ChainBuffer[m_BufferIndex].Sync);
    m_ChainBuffer[m_BufferIndex].Sync = m_pImmediateContext->FenceSync();

    m_MaxMemoryUsage = Math::Max(m_MaxMemoryUsage, m_ChainBuffer[m_BufferIndex].UsedMemory);
    m_BufferIndex = (m_BufferIndex + 1) % STREAMED_MEMORY_GPU_BUFFERS_COUNT;
    m_ChainBuffer[m_BufferIndex].HandlesCount = 0;
    m_ChainBuffer[m_BufferIndex].UsedMemory = 0;

    m_LastAllocatedBlockSize = 0;
}

size_t StreamedMemoryGPU::Allocate(size_t _SizeInBytes, int _Alignment, const void* _Data)
{
    HK_ASSERT(_SizeInBytes > 0);

    if (_SizeInBytes == 0)
    {
        // Don't allow to alloc empty chunks
        _SizeInBytes = 1;
    }

    ChainBuffer* pChainBuffer = &m_ChainBuffer[m_BufferIndex];

    size_t alignedOffset = Align(pChainBuffer->UsedMemory, _Alignment);

    if (alignedOffset + _SizeInBytes > STREAMED_MEMORY_GPU_BLOCK_SIZE)
    {
        CoreApplication::sTerminateWithError("StreamedMemoryGPU::Allocate: failed on allocation of {} bytes\nIncrease STREAMED_MEMORY_GPU_BLOCK_SIZE\n", _SizeInBytes);
    }

    m_LastAllocatedBlockSize = _SizeInBytes;

    pChainBuffer->UsedMemory = alignedOffset + _SizeInBytes;
    pChainBuffer->HandlesCount++;

    alignedOffset += m_BufferIndex * STREAMED_MEMORY_GPU_BLOCK_SIZE;

    if (_Data)
    {
        Core::Memcpy((byte*)m_pMappedMemory + alignedOffset, _Data, _SizeInBytes);
    }

    return alignedOffset;
}

void StreamedMemoryGPU::ShrinkLastAllocatedMemoryBlock(size_t _SizeInBytes)
{
    HK_ASSERT(_SizeInBytes <= m_LastAllocatedBlockSize);

    ChainBuffer* pChainBuffer = &m_ChainBuffer[m_BufferIndex];
    pChainBuffer->UsedMemory = pChainBuffer->UsedMemory - m_LastAllocatedBlockSize + _SizeInBytes;

    m_LastAllocatedBlockSize = _SizeInBytes;
}

HK_NAMESPACE_END
