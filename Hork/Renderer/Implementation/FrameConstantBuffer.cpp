/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#if 0
#include "FrameConstantBuffer.h"
#include "RenderLocal.h"

FrameConstantBuffer::FrameConstantBuffer( size_t InBufferSize )
    : BufferSize( InBufferSize )
{
    RHI::BufferDesc bufferCI = {};

    bufferCI.SizeInBytes = BufferSize * SWAP_CHAIN_SIZE;

    bufferCI.ImmutableStorageFlags = (RHI::IMMUTABLE_STORAGE_FLAGS)(RHI::IMMUTABLE_MAP_WRITE | RHI::IMMUTABLE_MAP_PERSISTENT | RHI::IMMUTABLE_MAP_COHERENT);
    bufferCI.bImmutableStorage = true;

    GDevice->CreateBuffer( bufferCI, nullptr, &Buffer );

    pMappedMemory = Buffer->Map( RHI::MAP_TRANSFER_WRITE,
                                 RHI::MAP_NO_INVALIDATE,//RHI::MAP_INVALIDATE_ENTIRE_BUFFER,
                                 RHI::MAP_PERSISTENT_COHERENT,
                                 false, // flush explicit
                                 false ); // unsynchronized

    if ( !pMappedMemory ) {
        CriticalError( "FrameConstantBuffer::ctor: cannot initialize persistent mapped buffer size {}\n", bufferCI.SizeInBytes );
    }

    for ( int i = 0 ; i < SWAP_CHAIN_SIZE ; i++ ) {
        ChainBuffer[i].UsedMemory = 0;
        ChainBuffer[i].Sync = 0;
    }

    BufferIndex = 0;

    UniformBufferOffsetAlignment = GDevice->GetDeviceCaps( RHI::DEVICE_CAPS_UNIFORM_BUFFER_OFFSET_ALIGNMENT );
}

FrameConstantBuffer::~FrameConstantBuffer()
{
    for ( int i = 0 ; i < SWAP_CHAIN_SIZE ; i++ ) {
        Wait( ChainBuffer[i].Sync );
        rcmd->RemoveSync( ChainBuffer[i].Sync );
    }

    Buffer->Unmap();
}

size_t FrameConstantBuffer::Allocate( size_t InSize )
{
    HK_ASSERT( InSize <= BufferSize );

    if ( InSize == 0 ) {
        // Don't allow to alloc empty chunks
        InSize = 1;
    }

    ChainBuffer * pChainBuffer = &ChainBuffer[BufferIndex];

    size_t alignedOffset = Align( pChainBuffer->UsedMemory, UniformBufferOffsetAlignment );

    if ( alignedOffset + InSize > BufferSize ) {
        CriticalError( "FrameConstantBuffer::Allocate: failed on allocation of {} bytes\nIncrease buffer size\n", InSize );
    }

    pChainBuffer->UsedMemory = alignedOffset + InSize;

    alignedOffset += BufferIndex * BufferSize;

    return alignedOffset;
}

void FrameConstantBuffer::Begin()
{
    Wait( ChainBuffer[BufferIndex].Sync );
}

void FrameConstantBuffer::End()
{
    ChainBuffer * pCurrent = &ChainBuffer[BufferIndex];
    rcmd->RemoveSync( pCurrent->Sync );

    pCurrent->Sync = rcmd->FenceSync();

    BufferIndex = (BufferIndex + 1) % SWAP_CHAIN_SIZE;

    pCurrent = &ChainBuffer[BufferIndex];
    pCurrent->UsedMemory = 0;
}

void FrameConstantBuffer::Wait( RHI::SyncObject Sync )
{
    const uint64_t timeOutNanoseconds = 1;
    if ( Sync ) {
        RHI::CLIENT_WAIT_STATUS status;
        do {
            status = rcmd->ClientWait( Sync, timeOutNanoseconds );
        } while ( status != RHI::CLIENT_WAIT_ALREADY_SIGNALED && status != RHI::CLIENT_WAIT_CONDITION_SATISFIED );
    }
}
#endif
