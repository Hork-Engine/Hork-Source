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

#include "HeapBlob.h"

HK_NAMESPACE_BEGIN

HeapBlob::~HeapBlob()
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_HeapPtr);
}

void HeapBlob::Reset(size_t sizeInBytes, void const* data, MALLOC_FLAGS flags)
{
    if (m_HeapSize == sizeInBytes)
    {
        if (data && m_HeapSize > 0)
            Core::Memcpy(m_HeapPtr, data, m_HeapSize);
        return;
    }

    Core::GetHeapAllocator<HEAP_MISC>().Free(m_HeapPtr);

    m_HeapSize = sizeInBytes;

    if (sizeInBytes > 0)
    {
        m_HeapPtr = Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeInBytes + 1, 16, flags);
        if (m_HeapPtr)
        {
            if (data)
                Core::Memcpy(m_HeapPtr, data, sizeInBytes);
            ((uint8_t*)m_HeapPtr)[sizeInBytes] = 0;
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
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_HeapPtr);
    m_HeapPtr  = nullptr;
    m_HeapSize = 0;
}

HK_NAMESPACE_END
