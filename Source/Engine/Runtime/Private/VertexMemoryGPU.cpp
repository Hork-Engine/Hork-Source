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

#include <Runtime/Public/VertexMemoryGPU.h>
#include <Runtime/Public/RuntimeVariable.h>
#include <Core/Public/CriticalError.h>

ARuntimeVariable RVWriteDynamicData( _CTS( "WriteDynamicData" ), _CTS( "1" ) );

AVertexMemoryGPU GVertexMemoryGPU;
AStreamedMemoryGPU GStreamedMemoryGPU;

AVertexMemoryGPU::AVertexMemoryGPU() {
    UsedMemory = 0;
    UsedMemoryHuge = 0;
}

AVertexMemoryGPU::~AVertexMemoryGPU() {
    AN_ASSERT( UsedMemory == 0 );
    AN_ASSERT( UsedMemoryHuge == 0 );
}

void AVertexMemoryGPU::Initialize() {

}

void AVertexMemoryGPU::Deinitialize() {
    CheckMemoryLeaks();
    for ( ABufferGPU * buffer : BufferHandles ) {
        GRenderBackend->DestroyBuffer( buffer );
    }
    for ( SVertexHandle * handle : HugeHandles ) {
        ABufferGPU * buffer = (ABufferGPU *)handle->Address;
        GRenderBackend->DestroyBuffer( buffer );
    }
    Handles.Free();
    HugeHandles.Free();
    Blocks.Free();
    BufferHandles.Free();
    HandlePool.Free();
    UsedMemory = 0;
    UsedMemoryHuge = 0;
}

SVertexHandle * AVertexMemoryGPU::AllocateVertex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {
    //AN_ASSERT( IsAligned< VERTEX_SIZE_ALIGN >( _SizeInBytes ) );
    return Allocate( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
}

SVertexHandle * AVertexMemoryGPU::AllocateIndex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {
    //AN_ASSERT( IsAligned< INDEX_SIZE_ALIGN >( _SizeInBytes ) );
    return Allocate( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
}

void AVertexMemoryGPU::Deallocate( SVertexHandle * _Handle ) {
    if ( !_Handle ) {
        return;
    }

    if ( _Handle->Size > VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
        DeallocateHuge( _Handle );
        return;
    }

    GLogger.Printf( "Deallocated buffer at block %d, offset %d, size %d\n", _Handle->GetBlockIndex(), _Handle->GetBlockOffset(), _Handle->Size );

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

void AVertexMemoryGPU::Update( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data ) {
    if ( _Handle->Size > VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
        UpdateHuge( _Handle, _ByteOffset, _SizeInBytes, _Data );
        return;
    }

    GRenderBackend->WriteBuffer( BufferHandles[_Handle->GetBlockIndex()], _Handle->GetBlockOffset() + _ByteOffset, _SizeInBytes, _Data );
}

void AVertexMemoryGPU::Defragment( bool bDeallocateEmptyBlocks, bool bForceUpload ) {

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

                    GRenderBackend->WriteBuffer( BufferHandles[handle->GetBlockIndex()], handle->GetBlockOffset(), handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
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
            //    GRenderBackend->OrphanBuffer( BufferHandles[blockIndex] );
            //}

            if ( handleBlockIndex != blockIndex || handleBlockOffset != offset || bForceUpload ) {
                handle->MakeAddress( blockIndex, offset );

                GRenderBackend->WriteBuffer( BufferHandles[blockIndex], offset, handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
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
            for ( int i = 0 ; i < freeBuffers ; i++ ) {
                GRenderBackend->DestroyBuffer( BufferHandles[ Blocks.Size() + i ] );
            }
            BufferHandles.Resize( Blocks.Size() );
        } else {
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

void AVertexMemoryGPU::GetPhysicalBufferAndOffset( SVertexHandle * _Handle, ABufferGPU ** _Buffer, size_t * _Offset ) {
    if ( _Handle->IsHuge() ) {
        *_Buffer = (ABufferGPU *)_Handle->Address;
        *_Offset = 0;
        return;
    }

    *_Buffer = BufferHandles[_Handle->GetBlockIndex()];
    *_Offset = _Handle->GetBlockOffset();
}

void AVertexMemoryGPU::UploadResourcesGPU() {
    UploadBuffers();
    UploadBuffersHuge();
}

int AVertexMemoryGPU::FindBlock( size_t _RequiredSize ) {
    for ( int i = 0 ; i < Blocks.Size() ; i++ ) {
        if ( Blocks[i].AllocOffset + _RequiredSize <= VERTEX_MEMORY_GPU_BLOCK_SIZE ) {
            return i;
        }
    }
    return -1;
}

SVertexHandle * AVertexMemoryGPU::Allocate( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {

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
        GRenderBackend->WriteBuffer( BufferHandles[handle->GetBlockIndex()], handle->GetBlockOffset(), handle->Size, _Data );
    }

    GLogger.Printf( "Allocated buffer at block %d, offset %d, size %d\n", handle->GetBlockIndex(), handle->GetBlockOffset(), handle->Size );

    return handle;
}

SVertexHandle * AVertexMemoryGPU::AllocateHuge( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {
    SVertexHandle * handle = HandlePool.Allocate();

    handle->Size = _SizeInBytes;
    handle->GetMemoryCB = _GetMemoryCB;
    handle->UserPointer = _UserPointer;

    ABufferGPU * buffer = GRenderBackend->CreateBuffer( this );
    GRenderBackend->InitializeBuffer( buffer, _SizeInBytes );

    if ( _Data ) {
        GRenderBackend->WriteBuffer( buffer, 0, _SizeInBytes, _Data );
    }

    handle->Address = (size_t)buffer;

    UsedMemoryHuge += _SizeInBytes;

    HugeHandles.Append( handle );

    return handle;
}

void AVertexMemoryGPU::DeallocateHuge( SVertexHandle * _Handle ) {
    UsedMemoryHuge -= _Handle->Size;

    ABufferGPU * buffer = (ABufferGPU *)_Handle->Address;
    GRenderBackend->DestroyBuffer( buffer );

    HugeHandles.RemoveSwap( HugeHandles.IndexOf( _Handle ) );

    HandlePool.Deallocate( _Handle );
}

void AVertexMemoryGPU::UpdateHuge( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data ) {
    GRenderBackend->WriteBuffer( (ABufferGPU *)_Handle->Address, _ByteOffset, _SizeInBytes, _Data );
}

void AVertexMemoryGPU::UploadBuffers() {
    // We not only upload the buffer data, we also perform defragmentation here.

    bool bDeallocateEmptyBlocks = true;
    bool bForceUpload = true;

    Defragment( bDeallocateEmptyBlocks, bForceUpload );
}

void AVertexMemoryGPU::UploadBuffersHuge() {
    for ( int i = 0 ; i < HugeHandles.Size() ; i++ ) {
        SVertexHandle * handle = HugeHandles[i];
        GRenderBackend->WriteBuffer( (ABufferGPU *)handle->Address, 0, handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
    }
}

void AVertexMemoryGPU::AddGPUBuffer() {
    // Create GPU buffer
    ABufferGPU * buffer = GRenderBackend->CreateBuffer( this );
    GRenderBackend->InitializeBuffer( buffer, VERTEX_MEMORY_GPU_BLOCK_SIZE );
    BufferHandles.Append( buffer );

    GLogger.Printf( "Allocated a new block (total blocks %d)\n", BufferHandles.Size() );
}

void AVertexMemoryGPU::CheckMemoryLeaks() {
    for ( SVertexHandle * handle : Handles ) {
        GLogger.Print( "==== Vertex Memory Leak ====\n" );
        GLogger.Printf( "Chunk Address: %u Size: %d\n", handle->Address, handle->Size );
    }
    for ( SVertexHandle * handle : HugeHandles ) {
        GLogger.Print( "==== Vertex Memory Leak ====\n" );
        GLogger.Printf( "Chunk Address: %u Size: %d (Huge)\n", handle->Address, handle->Size );
    }
}

AStreamedMemoryGPU::AStreamedMemoryGPU() {
    Core::ZeroMem( FrameData, sizeof( SFrameData ) );

    Buffer = nullptr;
    pMappedMemory = nullptr;
    FrameWrite = 0;
    MaxMemoryUsage = 0;
}

AStreamedMemoryGPU::~AStreamedMemoryGPU() {
    for ( int i = 0 ; i < STREAMED_MEMORY_GPU_BUFFERS_COUNT ; i++ ) {
        AN_ASSERT( FrameData[i].UsedMemory == 0 );
    }
}

void AStreamedMemoryGPU::Initialize() {
    Deinitialize();

    Buffer = GRenderBackend->CreateBuffer( this );
    pMappedMemory = GRenderBackend->InitializePersistentMappedBuffer( Buffer, STREAMED_MEMORY_GPU_BLOCK_SIZE * STREAMED_MEMORY_GPU_BUFFERS_COUNT );

    for ( int i = 0 ; i < STREAMED_MEMORY_GPU_BUFFERS_COUNT ; i++ ) {
        SFrameData * frameData = &FrameData[i];
        frameData->UsedMemory = 0;
        frameData->Sync = 0;
    }
}

void AStreamedMemoryGPU::Deinitialize() {
    for ( int i = 0 ; i < STREAMED_MEMORY_GPU_BUFFERS_COUNT ; i++ ) {
        SFrameData * frameData = &FrameData[i];

        frameData->HandlesCount = 0;
        frameData->UsedMemory = 0;

        GRenderBackend->WaitSync( frameData->Sync );
        GRenderBackend->RemoveSync( frameData->Sync );
    }

    if ( Buffer ) {
        // TODO: Unmap
        GRenderBackend->DestroyBuffer( Buffer );
        Buffer = nullptr;
    }

    pMappedMemory = nullptr;
}

size_t AStreamedMemoryGPU::AllocateVertex( size_t _SizeInBytes, const void * _Data ) {
    const int VertexBufferAlignment = 32; // TODO: Get from driver!!!
    return Allocate( _SizeInBytes, VertexBufferAlignment, _Data );
}

size_t AStreamedMemoryGPU::AllocateIndex( size_t _SizeInBytes, const void * _Data ) {
    const int IndexBufferAlignment = 16; // TODO: Get from driver!!!
    return Allocate( _SizeInBytes, IndexBufferAlignment, _Data );
}

size_t AStreamedMemoryGPU::AllocateJoint( size_t _SizeInBytes, const void * _Data ) {
    return Allocate( _SizeInBytes, GRenderBackend->GetUniformBufferOffsetAlignment(), _Data );
}

void * AStreamedMemoryGPU::Map( size_t _StreamHandle ) {
    return (byte *)pMappedMemory + _StreamHandle;
}

void AStreamedMemoryGPU::GetPhysicalBufferAndOffset( size_t _StreamHandle, ABufferGPU ** _Buffer, size_t * _Offset ) {
    *_Buffer = Buffer;
    *_Offset = _StreamHandle;
}

ABufferGPU * AStreamedMemoryGPU::GetBufferGPU() {
    return Buffer;
}

void AStreamedMemoryGPU::WaitBuffer() {
    GRenderBackend->WaitSync( FrameData[FrameWrite].Sync );
}

void AStreamedMemoryGPU::SwapFrames() {
    GRenderBackend->RemoveSync( FrameData[FrameWrite].Sync );
    FrameData[FrameWrite].Sync = GRenderBackend->FenceSync();

    MaxMemoryUsage = Math::Max( MaxMemoryUsage, FrameData[FrameWrite].UsedMemory );
    FrameWrite = ( FrameWrite + 1 ) % STREAMED_MEMORY_GPU_BUFFERS_COUNT;
    FrameData[FrameWrite].HandlesCount = 0;
    FrameData[FrameWrite].UsedMemory = 0;
}

void AStreamedMemoryGPU::UploadResourcesGPU() {
}

size_t AStreamedMemoryGPU::Allocate( size_t _SizeInBytes, int _Alignment, const void * _Data ) {
    AN_ASSERT( _SizeInBytes > 0 );

    SFrameData * frameData = &FrameData[FrameWrite];

    size_t alignedOffset = Align( frameData->UsedMemory, _Alignment );

    if ( alignedOffset + _SizeInBytes > STREAMED_MEMORY_GPU_BLOCK_SIZE ) {
        CriticalError( "AStreamedMemoryGPU::Allocate: failed on allocation of %d bytes\nIncrease STREAMED_MEMORY_GPU_BLOCK_SIZE\n", _SizeInBytes );
    }

    frameData->UsedMemory = alignedOffset + _SizeInBytes;
    frameData->HandlesCount++;

    alignedOffset += FrameWrite * STREAMED_MEMORY_GPU_BLOCK_SIZE;

    if ( _Data ) {
        if ( RVWriteDynamicData ) {
            Core::MemcpySSE( (byte *)pMappedMemory + alignedOffset, _Data, _SizeInBytes );
        }
    }

    return alignedOffset;
}
