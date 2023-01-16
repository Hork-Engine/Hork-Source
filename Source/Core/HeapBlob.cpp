/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "HeapBlob.h"

HK_NAMESPACE_BEGIN

HeapBlob::~HeapBlob()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_HeapPtr);
}

void HeapBlob::Reset(size_t SizeInBytes, void const* pData, MALLOC_FLAGS Flags)
{
    if (m_HeapSize == SizeInBytes)
    {
        if (pData && m_HeapSize > 0)
            Platform::Memcpy(m_HeapPtr, pData, m_HeapSize);
        return;
    }

    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_HeapPtr);

    m_HeapSize = SizeInBytes;

    if (SizeInBytes > 0)
    {
        m_HeapPtr = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(SizeInBytes + 1, 16, Flags);
        if (m_HeapPtr)
        {
            if (pData)
                Platform::Memcpy(m_HeapPtr, pData, SizeInBytes);
            ((uint8_t*)m_HeapPtr)[SizeInBytes] = 0;
        }
        else
            m_HeapSize = 0;
    }
    else
    {
        m_HeapPtr = nullptr;
    }
}

void HeapBlob::Reset()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_HeapPtr);
    m_HeapPtr  = nullptr;
    m_HeapSize = 0;
}

HK_NAMESPACE_END
