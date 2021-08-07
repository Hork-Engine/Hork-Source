/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Runtime/Public/VertexMemoryGPU.h>
#include <Core/Public/Core.h>
#include <Core/Public/CoreMath.h>

AVertexMemoryGPU::AVertexMemoryGPU( RenderCore::IDevice * Device )
    : RenderDevice( Device )
{
    UsedMemory = 0;
    UsedMemoryHuge = 0;
}

AVertexMemoryGPU::~AVertexMemoryGPU()
{
    CheckMemoryLeaks();
    for ( SVertexHandle * handle : HugeHandles ) {
        RenderCore::IBuffer * buffer = (RenderCore::IBuffer *)handle->Address;
        buffer->RemoveRef();
    }
}

SVertexHandle * AVertexMemoryGPU::AllocateVertex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer )
{
    //AN_ASSERT( IsAligned< VERTEX_SIZE_ALIGN >( _SizeInBytes ) );
    return Allocate( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
}

SVertexHandle * AVertexMemoryGPU::AllocateIndex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer )
{
    //AN_ASSERT( IsAligned< INDEX_SIZE_ALIGN >( _SizeInBytes ) );
    return Allocate( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
}

void AVertexMemoryGPU::Deallocate( SVertexHandle * _Handle )
{
    if ( !_Handle ) {
        return;
    }

    if ( _Handle->Size > VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
        DeallocateHuge( _Handle );
        return;
    }

    //GLogger.Printf( "Deallocated buffer at block %d, offset %d, size %d\n", _Handle->GetBlockIndex(), _Handle->GetBlockOffset(), _Handle->Size );

    SBlock * block = &Blocks[_Handle->GetBlockIndex()];

    size_t chunkSize = Align( _Handle->Size, VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT );

    block->UsedMemory -= chunkSize;

    AN_ASSERT( block->UsedMemory >= 0 );

    if ( block->AllocOffset == _Handle->GetBlockOffset() + chunkSize ) {
        block->AllocOffset -= chunkSize;
    }

    if ( block->UsedMemory == 0 ) {
        block->AllocOffset = 0;
    }

    UsedMemory -= chunkSize;

    Handles.RemoveSwap( Handles.IndexOf( _Handle ) );

    HandlePool.Deallocate( _Handle );
}

void AVertexMemoryGPU::Update( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data )
{
    if ( _Handle->Size > VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
        UpdateHuge( _Handle, _ByteOffset, _SizeInBytes, _Data );
        return;
    }

    BufferHandles[_Handle->GetBlockIndex()]->WriteRange( _Handle->GetBlockOffset() + _ByteOffset, _SizeInBytes, _Data );
}

void AVertexMemoryGPU::Defragment( bool bDeallocateEmptyBlocks, bool bForceUpload )
{
    struct {
        bool operator()( SVertexHandle const * A, SVertexHandle const * B ) {
            return A->Size > B->Size;
        }
    } cmp;

    StdSort( Handles.Begin(), Handles.End(), cmp );

    // NOTE: We can allocate new GPU buffers for blocks and just copy buffer-to-buffer on GPU side and then deallocate old buffers.
    // It is gonna be faster than CPU->GPU transition and get rid of implicit synchroization on the driver, but takes more memory.

    Blocks.Clear();

    for ( int i = 0 ; i < Handles.Size() ; i++ ) {
        SVertexHandle * handle = Handles[i];

        size_t handleSize = handle->Size;
        int handleBlockIndex = handle->GetBlockIndex();
        size_t handleBlockOffset = handle->GetBlockOffset();
        size_t chunkSize = Align( handleSize, VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT );

        bool bRelocated = false;
        for ( int j = 0 ; j < Blocks.Size() ; j++ ) {
            if ( Blocks[j].AllocOffset + handleSize <= VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
                if ( handleBlockIndex != j || handleBlockOffset != Blocks[j].AllocOffset || bForceUpload ) {
                    handle->MakeAddress( j, Blocks[j].AllocOffset );

                    BufferHandles[handle->GetBlockIndex()]->WriteRange( handle->GetBlockOffset(), handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
                }
                Blocks[j].AllocOffset += chunkSize;
                Blocks[j].UsedMemory += chunkSize;
                bRelocated = true;
                break;
            }
        }
        if ( !bRelocated ) {
            int blockIndex = Blocks.Size();
            size_t offset = 0;

            //if ( bForceUpload ) {
            //    // If buffers are in use driver will allocate a new storage and implicit synchroization will not occur.
            //    BufferHandles[blockIndex]->Orphan();
            //}

            if ( handleBlockIndex != blockIndex || handleBlockOffset != offset || bForceUpload ) {
                handle->MakeAddress( blockIndex, offset );

                BufferHandles[blockIndex]->WriteRange( offset, handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
            }

            SBlock newBlock;
            newBlock.AllocOffset = chunkSize;
            newBlock.UsedMemory = chunkSize;
            Blocks.Append( newBlock );
        }
    }

    int freeBuffers = BufferHandles.Size() - Blocks.Size();
    if ( freeBuffers > 0 ) {
        if ( bDeallocateEmptyBlocks ) {
            // Destroy and deallocate unused GPU buffers
            //for ( int i = 0 ; i < freeBuffers ; i++ ) {
            //    BufferHandles[ Blocks.Size() + i ].Reset();
            //}
            BufferHandles.Resize( Blocks.Size() );
        }
        else {
            // Emit empty blocks
            int firstBlock = Blocks.Size();
            Blocks.Resize( BufferHandles.Size() );
            for ( int i = 0 ; i < freeBuffers ; i++ ) {
                Blocks[firstBlock + i].AllocOffset = 0;
                Blocks[firstBlock + i].UsedMemory = 0;
            }
        }
    }
}

void AVertexMemoryGPU::GetPhysicalBufferAndOffset( SVertexHandle * _Handle, RenderCore::IBuffer ** _Buffer, size_t * _Offset )
{
    if ( _Handle->IsHuge() ) {
        *_Buffer = (RenderCore::IBuffer *)_Handle->Address;
        *_Offset = 0;
        return;
    }

    *_Buffer = BufferHandles[_Handle->GetBlockIndex()];
    *_Offset = _Handle->GetBlockOffset();
}

int AVertexMemoryGPU::FindBlock( size_t _RequiredSize )
{
    for ( int i = 0 ; i < Blocks.Size() ; i++ ) {
        if ( Blocks[i].AllocOffset + _RequiredSize <= VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
            return i;
        }
    }
    return -1;
}

SVertexHandle * AVertexMemoryGPU::Allocate( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer )
{
    if ( _SizeInBytes > VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
        // Huge block

        if ( !bAllowHugeAllocs ) {
            CriticalError( "AVertexMemoryGPU::Allocate: huge alloc %d bytes\n", _SizeInBytes );
        }

        return AllocateHuge( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
    }

    int i = FindBlock( _SizeInBytes );

    const int AUTO_DEFRAG_FACTOR = (MaxBlocks == 1) ? 1 : 8;

    // If block was not found, try to defragmentate memory
    if ( i == -1 && bAutoDefrag && GetUnusedMemory() >= _SizeInBytes * AUTO_DEFRAG_FACTOR ) {

        bool bDeallocateEmptyBlocks = false;
        bool bForceUpload = false;

        Defragment( bDeallocateEmptyBlocks, bForceUpload );

        i = FindBlock( _SizeInBytes );
    }

    SBlock * block = i == -1 ? nullptr : &Blocks[i];

    if ( !block ) {

        if ( MaxBlocks && Blocks.Size() >= MaxBlocks ) {
            CriticalError( "AVertexMemoryGPU::Allocate: failed on allocation of %d bytes\n", _SizeInBytes );
        }

        SBlock newBlock;
        newBlock.AllocOffset = 0;
        newBlock.UsedMemory = 0;
        Blocks.Append( newBlock );

        i = Blocks.Size()-1;

        block = &Blocks[i];

        AddGPUBuffer();
    }

    SVertexHandle * handle = HandlePool.Allocate();

    handle->MakeAddress( i, block->AllocOffset );
    handle->Size = _SizeInBytes;
    handle->GetMemoryCB = _GetMemoryCB;
    handle->UserPointer = _UserPointer;

    Handles.Append( handle );

    size_t chunkSize = Align( _SizeInBytes, VERTEX_MEMORY_GPU_CHUNK_OFFSET_ALIGNMENT );

    block->AllocOffset += chunkSize;
    block->UsedMemory += chunkSize;

    UsedMemory += chunkSize;

    if ( _Data ) {
        BufferHandles[handle->GetBlockIndex()]->WriteRange( handle->GetBlockOffset(), handle->Size, _Data );
    }

    //GLogger.Printf( "Allocated buffer at block %d, offset %d, size %d\n", handle->GetBlockIndex(), handle->GetBlockOffset(), handle->Size );

    return handle;
}

SVertexHandle * AVertexMemoryGPU::AllocateHuge( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer )
{
    SVertexHandle * handle = HandlePool.Allocate();

    handle->Size = _SizeInBytes;
    handle->GetMemoryCB = _GetMemoryCB;
    handle->UserPointer = _UserPointer;

    RenderCore::SBufferCreateInfo bufferCI = {};
    bufferCI.SizeInBytes = _SizeInBytes;
    // Mutable storage with flag MUTABLE_STORAGE_STATIC is much faster during rendering than immutable (tested on NVidia GeForce GTX 770)
    bufferCI.MutableClientAccess = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    bufferCI.MutableUsage = RenderCore::MUTABLE_STORAGE_STATIC;

    TRef< RenderCore::IBuffer > buffer;
    RenderDevice->CreateBuffer( bufferCI, _Data, &buffer );
    buffer->SetDebugName( "Vertex memory HUGE buffer" );

    RenderCore::IBuffer * pBuffer = buffer.GetObject();
    pBuffer->AddRef();

    handle->Address = (size_t)pBuffer;

    UsedMemoryHuge += _SizeInBytes;

    HugeHandles.Append( handle );

    return handle;
}

void AVertexMemoryGPU::DeallocateHuge( SVertexHandle * _Handle )
{
    UsedMemoryHuge -= _Handle->Size;

    RenderCore::IBuffer * buffer = (RenderCore::IBuffer *)_Handle->Address;
    buffer->RemoveRef();

    HugeHandles.RemoveSwap( HugeHandles.IndexOf( _Handle ) );

    HandlePool.Deallocate( _Handle );
}

void AVertexMemoryGPU::UpdateHuge( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data )
{
    ((RenderCore::IBuffer *)_Handle->Address)->WriteRange( _ByteOffset, _SizeInBytes, _Data );
}

void AVertexMemoryGPU::UploadBuffers()
{
    // We not only upload the buffer data, we also perform defragmentation here.

    bool bDeallocateEmptyBlocks = true;
    bool bForceUpload = true;

    Defragment( bDeallocateEmptyBlocks, bForceUpload );
}

void AVertexMemoryGPU::UploadBuffersHuge()
{
    for ( int i = 0 ; i < HugeHandles.Size() ; i++ ) {
        SVertexHandle * handle = HugeHandles[i];
        ((RenderCore::IBuffer *)handle->Address)->WriteRange( 0, handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
    }
}

void AVertexMemoryGPU::AddGPUBuffer()
{
    // Create GPU buffer

    RenderCore::SBufferCreateInfo bufferCI = {};
    bufferCI.SizeInBytes = VERTEX_MEMORY_GPU_BLOCK_SIZE;
    // Mutable storage with flag MUTABLE_STORAGE_STATIC is much faster during rendering than immutable (tested on NVidia GeForce GTX 770)
    bufferCI.MutableClientAccess = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    bufferCI.MutableUsage = RenderCore::MUTABLE_STORAGE_STATIC;

    TRef< RenderCore::IBuffer > buffer;
    RenderDevice->CreateBuffer( bufferCI, nullptr, &buffer );

    buffer->SetDebugName( "Vertex memory block buffer" );

    BufferHandles.Append( buffer );

    GLogger.Printf( "Allocated a new block (total blocks %d)\n", BufferHandles.Size() );
}

void AVertexMemoryGPU::CheckMemoryLeaks()
{
    for ( SVertexHandle * handle : Handles ) {
        GLogger.Print( "==== Vertex Memory Leak ====\n" );
        GLogger.Printf( "Chunk Address: %u Size: %d\n", handle->Address, handle->Size );
    }
    for ( SVertexHandle * handle : HugeHandles ) {
        GLogger.Print( "==== Vertex Memory Leak ====\n" );
        GLogger.Printf( "Chunk Address: %u Size: %d (Huge)\n", handle->Address, handle->Size );
    }
}

AStreamedMemoryGPU::AStreamedMemoryGPU( RenderCore::IDevice * Device )
    : RenderDevice( Device )
{
    RenderCore::SBufferCreateInfo bufferCI = {};

    bufferCI.SizeInBytes = STREAMED_MEMORY_GPU_BLOCK_SIZE * STREAMED_MEMORY_GPU_BUFFERS_COUNT;

    bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)
            ( RenderCore::IMMUTABLE_MAP_WRITE | RenderCore::IMMUTABLE_MAP_PERSISTENT | RenderCore::IMMUTABLE_MAP_COHERENT );
    bufferCI.bImmutableStorage = true;

    RenderDevice->CreateBuffer( bufferCI, nullptr, &Buffer );

    Buffer->SetDebugName( "Streamed memory buffer" );

    pMappedMemory = Buffer->Map( RenderCore::MAP_TRANSFER_WRITE,
                                 RenderCore::MAP_NO_INVALIDATE,//RenderCore::MAP_INVALIDATE_ENTIRE_BUFFER,
                                 RenderCore::MAP_PERSISTENT_COHERENT,
                                 false, // flush explicit
                                 false  // unsynchronized
                               );

    if ( !pMappedMemory ) {
        CriticalError( "AStreamedMemoryGPU::Initialize: cannot initialize persistent mapped buffer size %d\n", bufferCI.SizeInBytes );
    }

    Core::ZeroMem( ChainBuffer, sizeof( SChainBuffer ) );

    BufferIndex = 0;
    MaxMemoryUsage = 0;
    LastAllocatedBlockSize = 0;

    VertexBufferAlignment = 32; // TODO: Get from driver!!!
    IndexBufferAlignment = 16; // TODO: Get from driver!!!
    ConstantBufferAlignment = RenderDevice->GetDeviceCaps( RenderCore::DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT );

    RenderDevice->GetImmediateContext( &pImmediateContext );
}

AStreamedMemoryGPU::~AStreamedMemoryGPU()
{
    for ( int i = 0 ; i < STREAMED_MEMORY_GPU_BUFFERS_COUNT ; i++ ) {
        SChainBuffer * pChainBuffer = &ChainBuffer[i];

        Wait( pChainBuffer->Sync );
        pImmediateContext->RemoveSync( pChainBuffer->Sync );
    }

    if ( Buffer ) {
        Buffer->Unmap();
    }
}

size_t AStreamedMemoryGPU::AllocateVertex( size_t _SizeInBytes, const void * _Data )
{
    return Allocate( _SizeInBytes, VertexBufferAlignment, _Data );
}

size_t AStreamedMemoryGPU::AllocateIndex( size_t _SizeInBytes, const void * _Data )
{
    return Allocate( _SizeInBytes, IndexBufferAlignment, _Data );
}

size_t AStreamedMemoryGPU::AllocateJoint( size_t _SizeInBytes, const void * _Data )
{
    return Allocate( _SizeInBytes, ConstantBufferAlignment, _Data );
}

size_t AStreamedMemoryGPU::AllocateConstant( size_t _SizeInBytes, const void * _Data )
{
    return Allocate( _SizeInBytes, ConstantBufferAlignment, _Data );
}

size_t AStreamedMemoryGPU::AllocateWithCustomAlignment( size_t _SizeInBytes, int _Alignment, const void * _Data )
{
    return Allocate( _SizeInBytes, _Alignment, _Data );
}

void * AStreamedMemoryGPU::Map( size_t _StreamHandle )
{
    return (byte *)pMappedMemory + _StreamHandle;
}

void AStreamedMemoryGPU::GetPhysicalBufferAndOffset( size_t _StreamHandle, RenderCore::IBuffer ** _Buffer, size_t * _Offset )
{
    *_Buffer = Buffer;
    *_Offset = _StreamHandle;
}

RenderCore::IBuffer * AStreamedMemoryGPU::GetBufferGPU()
{
    return Buffer;
}

void AStreamedMemoryGPU::Wait( RenderCore::SyncObject Sync )
{
    const uint64_t timeOutNanoseconds = 1;
    if ( Sync ) {
        RenderCore::CLIENT_WAIT_STATUS status;
        do {
            status = pImmediateContext->ClientWait( Sync, timeOutNanoseconds );
        } while ( status != RenderCore::CLIENT_WAIT_ALREADY_SIGNALED && status != RenderCore::CLIENT_WAIT_CONDITION_SATISFIED );
    }
}

void AStreamedMemoryGPU::Wait()
{
    Wait( ChainBuffer[BufferIndex].Sync );
}

void AStreamedMemoryGPU::Swap()
{
    pImmediateContext->RemoveSync( ChainBuffer[BufferIndex].Sync );
    ChainBuffer[BufferIndex].Sync = pImmediateContext->FenceSync();

    MaxMemoryUsage = Math::Max( MaxMemoryUsage, ChainBuffer[BufferIndex].UsedMemory );
    BufferIndex = ( BufferIndex + 1 ) % STREAMED_MEMORY_GPU_BUFFERS_COUNT;
    ChainBuffer[BufferIndex].HandlesCount = 0;
    ChainBuffer[BufferIndex].UsedMemory = 0;

    LastAllocatedBlockSize = 0;
}

size_t AStreamedMemoryGPU::Allocate( size_t _SizeInBytes, int _Alignment, const void * _Data )
{
    AN_ASSERT( _SizeInBytes > 0 );

    if ( _SizeInBytes == 0 ) {
        // Don't allow to alloc empty chunks
        _SizeInBytes = 1;
    }

    SChainBuffer * pChainBuffer = &ChainBuffer[BufferIndex];

    size_t alignedOffset = Align( pChainBuffer->UsedMemory, _Alignment );

    if ( alignedOffset + _SizeInBytes > STREAMED_MEMORY_GPU_BLOCK_SIZE ) {
        CriticalError( "AStreamedMemoryGPU::Allocate: failed on allocation of %d bytes\nIncrease STREAMED_MEMORY_GPU_BLOCK_SIZE\n", _SizeInBytes );
    }

    LastAllocatedBlockSize = _SizeInBytes;

    pChainBuffer->UsedMemory = alignedOffset + _SizeInBytes;
    pChainBuffer->HandlesCount++;

    alignedOffset += BufferIndex * STREAMED_MEMORY_GPU_BLOCK_SIZE;

    if ( _Data ) {
        Core::Memcpy( (byte *)pMappedMemory + alignedOffset, _Data, _SizeInBytes );
    }

    return alignedOffset;
}

void AStreamedMemoryGPU::ShrinkLastAllocatedMemoryBlock( size_t _SizeInBytes )
{
    AN_ASSERT( _SizeInBytes <= LastAllocatedBlockSize );

    SChainBuffer * pChainBuffer = &ChainBuffer[BufferIndex];
    pChainBuffer->UsedMemory = pChainBuffer->UsedMemory - LastAllocatedBlockSize + _SizeInBytes;

    LastAllocatedBlockSize = _SizeInBytes;
}
