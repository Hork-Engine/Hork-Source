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

#include "TempAllocator.h"

#include <Hork/Core/CoreApplication.h>

HK_NAMESPACE_BEGIN

TempAllocator::TempAllocator(size_t capacity)
{
    m_Base = static_cast<uint8_t*>(Core::GetHeapAllocator<HEAP_MISC>().Alloc(capacity, Alignment));
    m_Size = capacity;
}

TempAllocator::~TempAllocator()
{
    HK_ASSERT(m_Top == 0);
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_Base);
}

void* TempAllocator::Alloc(size_t sizeInBytes)
{
    if (sizeInBytes == 0)
    {
        return nullptr;
    }
    else
    {
        sizeInBytes = Align(sizeInBytes, Alignment);
        size_t newTop = m_Top + sizeInBytes;
        void* address;
        if (newTop <= m_Size)
        {
            address = m_Base + m_Top;
        }
        else
        {
            static bool warn = true;
            if (warn)
            {
                LOG("TempAllocator: The temporary buffer exceeded {:.1f} megabytes. Fallback to general-purpose allocator.\n", static_cast<float>(m_Size) / 1024 / 1024);
                warn = false;
            }
            address = Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeInBytes, Alignment);
        }
        m_Top = newTop;
        return address;
    }
}

void TempAllocator::Free(void* address, size_t sizeInBytes)
{
    if (address == nullptr)
    {
        HK_ASSERT(sizeInBytes == 0);
    }
    else
    {
        sizeInBytes = Align(sizeInBytes, Alignment);
        size_t newTop = m_Top - sizeInBytes;
        if (m_Top <= m_Size)
        {
            if (m_Base + newTop != address)
            {
                CoreApplication::sTerminateWithError("TempAllocator: Freeing in the wrong order");
            }
        }
        else
        {
            Core::GetHeapAllocator<HEAP_MISC>().Free(address);
        }
        m_Top = newTop;
    }
}

HK_NAMESPACE_END
