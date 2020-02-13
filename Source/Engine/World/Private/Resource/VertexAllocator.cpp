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

#include <World/Public/Resource/VertexAllocator.h>
#include <Core/Public/CriticalError.h>

AVertexAllocator GVertexAllocator;
ADynamicVertexAllocator GDynamicVertexAllocator;

AVertexAllocator::AVertexAllocator() {
    UsedMemory = 0;
    UsedMemoryHuge = 0;
}

AVertexAllocator::~AVertexAllocator() {
    AN_ASSERT( UsedMemory == 0 );
    AN_ASSERT( UsedMemoryHuge == 0 );
}

void AVertexAllocator::Initialize() {

}

void AVertexAllocator::Purge() {
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

SVertexHandle * AVertexAllocator::AllocateVertex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {
    //AN_ASSERT( IsAligned( _SizeInBytes, VERTEX_SIZE_ALIGN ) );
    return Allocate( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
}

SVertexHandle * AVertexAllocator::AllocateIndex( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {
    //AN_ASSERT( IsAligned( _SizeInBytes, INDEX_SIZE_ALIGN ) );
    return Allocate( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
}

void AVertexAllocator::Deallocate( SVertexHandle * _Handle ) {
    if ( !_Handle ) {
        return;
    }

    if ( _Handle->Size > VERTEX_ALLOCATOR_BLOCK_SIZE ) {
        DeallocateHuge( _Handle );
        return;
    }

    GLogger.Printf( "Deallocated buffer at block %d, offset %d, size %d\n", _Handle->GetBlockIndex(), _Handle->GetBlockOffset(), _Handle->Size );

    SBlock * block = &Blocks[_Handle->GetBlockIndex()];

    size_t chunkSize = Align( _Handle->Size, CHUNK_OFFSET_ALIGNMENT );

    block->UsedMemory -= chunkSize;

    AN_ASSERT( block->UsedMemory >= 0 );

    if ( block->UsedMemory == 0 ) {
        block->AllocOffset = 0;
    }

    UsedMemory -= chunkSize;

    Handles.RemoveSwap( Handles.IndexOf( _Handle ) );

    HandlePool.Deallocate( _Handle );
}

void AVertexAllocator::Update( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data ) {
    if ( _Handle->Size > VERTEX_ALLOCATOR_BLOCK_SIZE ) {
        UpdateHuge( _Handle, _ByteOffset, _SizeInBytes, _Data );
        return;
    }

    GRenderBackend->WriteBuffer( BufferHandles[_Handle->GetBlockIndex()], _Handle->GetBlockOffset() + _ByteOffset, _SizeInBytes/*_Handle->Size*/, _Data );
}

void AVertexAllocator::Defragment( bool bDeallocateEmptyBlocks ) {

    struct {
        bool operator()( SVertexHandle const * A, SVertexHandle const * B ) {
            return A->Size > B->Size;
        }
    } cmp;

    StdSort( Handles.Begin(), Handles.End(), cmp );

    Blocks.Clear();

    for ( int i = 0 ; i < Handles.Size() ; i++ ) {
        SVertexHandle * handle = Handles[i];

        size_t handleSize = handle->Size;
        int handleBlockIndex = handle->GetBlockIndex();
        size_t handleBlockOffset = handle->GetBlockOffset();
        size_t chunkSize = Align( handleSize, CHUNK_OFFSET_ALIGNMENT );

        bool bRelocated = false;
        for ( int j = 0 ; j < Blocks.Size() ; j++ ) {
            if ( Blocks[j].AllocOffset + handleSize <= VERTEX_ALLOCATOR_BLOCK_SIZE ) {
                if ( handleBlockIndex != j || handleBlockOffset != Blocks[j].AllocOffset ) {
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

            if ( handleBlockIndex != blockIndex || handleBlockOffset != offset ) {
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

void AVertexAllocator::GetHandleBuffer( SVertexHandle * _Handle, ABufferGPU ** _Buffer, size_t * _Offset ) {
    if ( _Handle->IsHuge() ) {
        *_Buffer = (ABufferGPU *)_Handle->Address;
        *_Offset = 0;
        return;
    }

    *_Buffer = BufferHandles[_Handle->GetBlockIndex()];
    *_Offset = _Handle->GetBlockOffset();
}

void AVertexAllocator::UploadResourcesGPU() {
    UploadBuffers();
    UploadBuffersHuge();
}

int AVertexAllocator::FindBlock( size_t _RequiredSize ) {
    for ( int i = 0 ; i < Blocks.Size() ; i++ ) {
        if ( Blocks[i].AllocOffset + _RequiredSize <= VERTEX_ALLOCATOR_BLOCK_SIZE ) {
            return i;
        }
    }
    return -1;
}

SVertexHandle * AVertexAllocator::Allocate( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {

    if ( _SizeInBytes > VERTEX_ALLOCATOR_BLOCK_SIZE ) {
        // Huge block

        if ( !bAllowHugeAllocs ) {
            CriticalError( "AVertexAllocator::Allocate: huge alloc %d bytes\n", _SizeInBytes );
        }

        return AllocateHuge( _SizeInBytes, _Data, _GetMemoryCB, _UserPointer );
    }

    int i = FindBlock( _SizeInBytes );

    const int AUTO_DEFRAG_FACTOR = (MaxBlocks == 1) ? 1 : 8;

    // If block was not found, try to defragmentate memory
    if ( i == -1 && bAutoDefrag && GetUnusedMemory() >= _SizeInBytes * AUTO_DEFRAG_FACTOR ) {
        Defragment( false );

        i = FindBlock( _SizeInBytes );
    }

    SBlock * block = i == -1 ? nullptr : &Blocks[i];

    if ( !block ) {

        if ( MaxBlocks && Blocks.Size() >= MaxBlocks ) {
            CriticalError( "AVertexAllocator::Allocate: failed on allocation of %d bytes\n", _SizeInBytes );
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

    size_t chunkSize = Align( _SizeInBytes, CHUNK_OFFSET_ALIGNMENT );

    block->AllocOffset += chunkSize;
    block->UsedMemory += chunkSize;

    UsedMemory += chunkSize;

    if ( _Data ) {
        GRenderBackend->WriteBuffer( BufferHandles[handle->GetBlockIndex()], handle->GetBlockOffset(), handle->Size, _Data );
    }

    GLogger.Printf( "Allocated buffer at block %d, offset %d, size %d\n", handle->GetBlockIndex(), handle->GetBlockOffset(), handle->Size );

    return handle;
}

SVertexHandle * AVertexAllocator::AllocateHuge( size_t _SizeInBytes, const void * _Data, SGetMemoryCallback _GetMemoryCB, void * _UserPointer ) {
    SVertexHandle * handle = HandlePool.Allocate();

    handle->Size = _SizeInBytes;
    handle->GetMemoryCB = _GetMemoryCB;
    handle->UserPointer = _UserPointer;

    ABufferGPU * buffer = GRenderBackend->CreateBuffer( this );
    GRenderBackend->InitializeBuffer( buffer, _SizeInBytes, false );

    if ( _Data ) {
        GRenderBackend->WriteBuffer( buffer, 0, _SizeInBytes, _Data );
    }

    handle->Address = (size_t)buffer;

    UsedMemoryHuge += _SizeInBytes;

    HugeHandles.Append( handle );

    return handle;
}

void AVertexAllocator::DeallocateHuge( SVertexHandle * _Handle ) {
    UsedMemoryHuge -= _Handle->Size;

    ABufferGPU * buffer = (ABufferGPU *)_Handle->Address;
    GRenderBackend->DestroyBuffer( buffer );

    HugeHandles.RemoveSwap( HugeHandles.IndexOf( _Handle ) );

    HandlePool.Deallocate( _Handle );
}

void AVertexAllocator::UpdateHuge( SVertexHandle * _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data ) {
    GRenderBackend->WriteBuffer( (ABufferGPU *)_Handle->Address, _ByteOffset, _SizeInBytes, _Data );
}

void AVertexAllocator::UploadBuffers() {
    // Upload data to GPU buffers
    for ( int i = 0 ; i < Handles.Size() ; i++ ) {
        SVertexHandle * handle = Handles[i];
        GRenderBackend->WriteBuffer( BufferHandles[handle->GetBlockIndex()], handle->GetBlockOffset(), handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
    }
}

void AVertexAllocator::UploadBuffersHuge() {
    for ( int i = 0 ; i < HugeHandles.Size() ; i++ ) {
        SVertexHandle * handle = HugeHandles[i];
        GRenderBackend->WriteBuffer( (ABufferGPU *)handle->Address, 0, handle->Size, handle->GetMemoryCB( handle->UserPointer ) );
    }
}

void AVertexAllocator::AddGPUBuffer() {
    // Create GPU buffer
    ABufferGPU * buffer = GRenderBackend->CreateBuffer( this );
    GRenderBackend->InitializeBuffer( buffer, VERTEX_ALLOCATOR_BLOCK_SIZE, false );
    BufferHandles.Append( buffer );

    GLogger.Printf( "Allocated a new block (total blocks %d)\n", BufferHandles.Size() );
}


ADynamicVertexAllocator::ADynamicVertexAllocator() {
    memset( FrameData, 0, sizeof( SFrameData ) );

    FrameWrite = 0;
    MaxMemoryUsage = 0;
}

ADynamicVertexAllocator::~ADynamicVertexAllocator() {
    for ( int i = 0 ; i < 2 ; i++ ) {
        AN_ASSERT( FrameData[i].UsedMemory == 0 );
    }
}

void ADynamicVertexAllocator::Initialize() {
    Purge();

    for ( int i = 0 ; i < 2 ; i++ ) {
        SFrameData * frameData = &FrameData[i];

        frameData->Buffer = GRenderBackend->CreateBuffer( this );
        GRenderBackend->InitializeBuffer( frameData->Buffer, DYNAMIC_VERTEX_ALLOCATOR_BLOCK_SIZE, true );

        frameData->UsedMemory = 0;
    }
}

void ADynamicVertexAllocator::Purge() {
    for ( int i = 0 ; i < 2 ; i++ ) {
        SFrameData * frameData = &FrameData[i];

        if ( frameData->Buffer ) {
            GRenderBackend->DestroyBuffer( frameData->Buffer );
            frameData->Buffer = nullptr;
        }

        frameData->HandlesCount = 0;
        frameData->UsedMemory = 0;
    }
}

size_t ADynamicVertexAllocator::AllocateVertex( size_t _SizeInBytes, const void * _Data ) {
    //AN_ASSERT( IsAligned( _SizeInBytes, VERTEX_SIZE_ALIGN ) );
    return Allocate( _SizeInBytes, GetVertexBufferAlignment(), _Data );
}

size_t ADynamicVertexAllocator::AllocateIndex( size_t _SizeInBytes, const void * _Data ) {
    //AN_ASSERT( IsAligned( _SizeInBytes, INDEX_SIZE_ALIGN ) );
    return Allocate( _SizeInBytes, GetIndexBufferAlignment(), _Data );
}

size_t ADynamicVertexAllocator::AllocateJoint( size_t _SizeInBytes, const void * _Data ) {
    //AN_ASSERT( IsAligned( _SizeInBytes, JOINT_SIZE_ALIGN ) );
    return Allocate( _SizeInBytes, GetJointBufferAlignment(), _Data );
}

int ADynamicVertexAllocator::GetVertexBufferAlignment() const {
    return 32; // TODO: Get from driver!!!
}

int ADynamicVertexAllocator::GetIndexBufferAlignment() const {
    return 16; // TODO: Get from driver!!!
}

int ADynamicVertexAllocator::GetJointBufferAlignment() const {
    return 256; // TODO: Get from driver!!!
}

void ADynamicVertexAllocator::Update( size_t _Handle, size_t _ByteOffset, size_t _SizeInBytes, const void * _Data ) {
    SFrameData * frameData = &FrameData[FrameWrite];
    GRenderBackend->WriteBuffer( frameData->Buffer, _Handle + _ByteOffset, _SizeInBytes, _Data );
}

void ADynamicVertexAllocator::GetHandleBuffer( size_t _Handle, ABufferGPU ** _Buffer, size_t * _Offset ) {
    SFrameData * frameData = &FrameData[FrameWrite];

    *_Buffer = frameData->Buffer;
    *_Offset = _Handle;
}

void ADynamicVertexAllocator::SwapFrames() {
    MaxMemoryUsage = Math::Max( MaxMemoryUsage, FrameData[FrameWrite].UsedMemory );
    FrameWrite = ( FrameWrite + 1 ) & 1;
    FrameData[FrameWrite].HandlesCount = 0;
    FrameData[FrameWrite].UsedMemory = 0;
}

void ADynamicVertexAllocator::UploadResourcesGPU() {
}

size_t ADynamicVertexAllocator::Allocate( size_t _SizeInBytes, int _Alignment, const void * _Data ) {
    AN_ASSERT( _SizeInBytes > 0 );

    SFrameData * frameData = &FrameData[FrameWrite];

    size_t alignedOffset = Align( frameData->UsedMemory, _Alignment );

    if ( alignedOffset + _SizeInBytes > DYNAMIC_VERTEX_ALLOCATOR_BLOCK_SIZE ) {
        CriticalError( "ADynamicVertexAllocator::Allocate: failed on allocation of %d bytes\nIncrease DYNAMIC_VERTEX_ALLOCATOR_BLOCK_SIZE\n", _SizeInBytes );
    }

    frameData->UsedMemory = alignedOffset + _SizeInBytes;
    frameData->HandlesCount++;

    if ( _Data ) {
        GRenderBackend->WriteBuffer( frameData->Buffer, alignedOffset, _SizeInBytes, _Data );
    }

    return alignedOffset;
}
