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

#include "CircularBuffer.h"
#include "RenderLocal.h"
#include <Hork/Core/Platform.h>
#include <Hork/Core/CoreApplication.h>

HK_NAMESPACE_BEGIN

CircularBuffer::CircularBuffer(size_t InBufferSize) :
    m_BufferSize(InBufferSize)
{
    RenderCore::BufferDesc bufferCI = {};

    bufferCI.SizeInBytes = m_BufferSize * SWAP_CHAIN_SIZE;

    bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)(RenderCore::IMMUTABLE_MAP_WRITE | RenderCore::IMMUTABLE_MAP_PERSISTENT | RenderCore::IMMUTABLE_MAP_COHERENT);
    bufferCI.bImmutableStorage = true;

    GDevice->CreateBuffer(bufferCI, nullptr, &m_Buffer);

    m_Buffer->SetDebugName("Circular buffer");

    m_pMappedMemory = rcmd->MapBuffer(m_Buffer,
                                    RenderCore::MAP_TRANSFER_WRITE,
                                    RenderCore::MAP_NO_INVALIDATE, //RenderCore::MAP_INVALIDATE_ENTIRE_BUFFER,
                                    RenderCore::MAP_PERSISTENT_COHERENT,
                                    false,  // flush explicit
                                    false); // unsynchronized

    if (!m_pMappedMemory)
    {
        CoreApplication::sTerminateWithError("CircularBuffer::ctor: cannot initialize persistent mapped buffer size {}\n", bufferCI.SizeInBytes);
    }

    for (int i = 0; i < SWAP_CHAIN_SIZE; i++)
    {
        m_ChainBuffer[i].UsedMemory = 0;
        m_ChainBuffer[i].Sync = 0;
    }

    m_BufferIndex = 0;

    m_ConstantBufferAlignment = GDevice->GetDeviceCaps(RenderCore::DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT);
}

CircularBuffer::~CircularBuffer()
{
    for (int i = 0; i < SWAP_CHAIN_SIZE; i++)
    {
        Wait(m_ChainBuffer[i].Sync);
        rcmd->RemoveSync(m_ChainBuffer[i].Sync);
    }

    rcmd->UnmapBuffer(m_Buffer);
}

size_t CircularBuffer::Allocate(size_t InSize)
{
    HK_ASSERT(InSize > 0 && InSize <= m_BufferSize);

    ChainBuffer* pChainBuffer = &m_ChainBuffer[m_BufferIndex];

    size_t alignedOffset = Align(pChainBuffer->UsedMemory, m_ConstantBufferAlignment);

    if (alignedOffset + InSize > m_BufferSize)
    {
        pChainBuffer = Swap();
        alignedOffset = 0;
    }

    pChainBuffer->UsedMemory = alignedOffset + InSize;

    alignedOffset += m_BufferIndex * m_BufferSize;

    return alignedOffset;
}

CircularBuffer::ChainBuffer* CircularBuffer::Swap()
{
    ChainBuffer* pCurrent = &m_ChainBuffer[m_BufferIndex];
    rcmd->RemoveSync(pCurrent->Sync);

    pCurrent->Sync = rcmd->FenceSync();

    m_BufferIndex = (m_BufferIndex + 1) % SWAP_CHAIN_SIZE;

    pCurrent = &m_ChainBuffer[m_BufferIndex];
    pCurrent->UsedMemory = 0;

    Wait(pCurrent->Sync);

    //LOG( "Swap at {}\n", GFrameData->FrameNumber );

    return pCurrent;
}

void CircularBuffer::Wait(RenderCore::SyncObject Sync)
{
    const uint64_t timeOutNanoseconds = 1;
    if (Sync)
    {
        RenderCore::CLIENT_WAIT_STATUS status;
        do {
            status = rcmd->ClientWait(Sync, timeOutNanoseconds);
        } while (status != RenderCore::CLIENT_WAIT_ALREADY_SIGNALED && status != RenderCore::CLIENT_WAIT_CONDITION_SATISFIED);
    }
}

HK_NAMESPACE_END
