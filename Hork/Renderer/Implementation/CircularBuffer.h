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

#include <Hork/RHI/Common/ImmediateContext.h>

HK_NAMESPACE_BEGIN

class CircularBuffer : public RefCounted
{
public:
    CircularBuffer(size_t InBufferSize);
    virtual ~CircularBuffer();

    size_t Allocate(size_t InSize);

    byte* GetMappedMemory() { return (byte*)m_pMappedMemory; }

    RHI::IBuffer* GetBuffer() { return m_Buffer; }

private:
    struct ChainBuffer
    {
        size_t UsedMemory;
        RHI::SyncObject Sync;
    };

    ChainBuffer* Swap();

    void Wait(RHI::SyncObject Sync);

    Ref<RHI::IBuffer> m_Buffer;
    void* m_pMappedMemory;
    int m_BufferIndex;

    enum
    {
        SWAP_CHAIN_SIZE = 3
    };

    ChainBuffer m_ChainBuffer[SWAP_CHAIN_SIZE];
    size_t m_BufferSize;

    unsigned int m_ConstantBufferAlignment;
};

HK_NAMESPACE_END
