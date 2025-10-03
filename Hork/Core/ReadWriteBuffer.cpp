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

#include "ReadWriteBuffer.h"
#include "Logger.h"

HK_NAMESPACE_BEGIN

namespace
{
void* Alloc(size_t sizeInBytes)
{
    return Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeInBytes, 16);
}

void* Realloc(void* pMemory, size_t sizeInBytes)
{
    return Core::GetHeapAllocator<HEAP_MISC>().Realloc(pMemory, sizeInBytes, 16);
}

void Free(void* pMemory)
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(pMemory);
}
}

ReadWriteBuffer::ReadWriteBuffer(ReadWriteBuffer&& rhs) noexcept :
    m_Name(std::move(rhs.m_Name)),
    m_RawPtr(rhs.m_RawPtr),
    m_RWOffset(rhs.m_RWOffset),
    m_Size(rhs.m_Size),
    m_Capacity(rhs.m_Capacity),
    m_Granularity(rhs.m_Granularity),
    m_bExternalBuffer(rhs.m_bExternalBuffer)
{
    rhs.m_RawPtr = nullptr;
    rhs.m_RWOffset = 0;
    rhs.m_Size = 0;
    rhs.m_Capacity = 0;
    rhs.m_Granularity = 1024;
    rhs.m_bExternalBuffer = false;
}

ReadWriteBuffer& ReadWriteBuffer::operator=(ReadWriteBuffer&& rhs) noexcept
{
    if (!m_bExternalBuffer)
        Free(m_RawPtr);

    m_Name = std::move(rhs.m_Name);
    m_RawPtr = rhs.m_RawPtr;
    m_RWOffset = rhs.m_RWOffset;
    m_Size = rhs.m_Size;
    m_Capacity = rhs.m_Capacity;
    m_Granularity = rhs.m_Granularity;
    m_bExternalBuffer = rhs.m_bExternalBuffer;

    rhs.m_RawPtr = nullptr;
    rhs.m_RWOffset = 0;
    rhs.m_Size = 0;
    rhs.m_Capacity = 0;
    rhs.m_Granularity = 1024;
    rhs.m_bExternalBuffer = false;

    return *this;
}

ReadWriteBuffer::~ReadWriteBuffer()
{
    if (!m_bExternalBuffer)
        Free(m_RawPtr);
}

void ReadWriteBuffer::SetName(StringView name)
{
    m_Name = name;
}

StringView ReadWriteBuffer::GetName() const
{
    return m_Name;
}

void ReadWriteBuffer::SetExternalBuffer(void* memoryBuffer, size_t sizeInBytes)
{
    if (!m_bExternalBuffer)
        Free(m_RawPtr);

    m_RawPtr = reinterpret_cast<byte*>(memoryBuffer);
    m_Size = 0;
    m_Capacity = sizeInBytes;
    m_bExternalBuffer = true;
}

void ReadWriteBuffer::SetInternalBuffer(size_t baseCapacity)
{
    if (!m_bExternalBuffer)
        Free(m_RawPtr);

    if (baseCapacity > 0)
        m_RawPtr = (byte*)Alloc(baseCapacity);
    else
        m_RawPtr = nullptr;
    m_Size = 0;
    m_Capacity = baseCapacity;
    m_bExternalBuffer = false;
}

void ReadWriteBuffer::Reset()
{
    SetInternalBuffer(0);
}

void ReadWriteBuffer::Reserve(size_t capacity)
{
    if (m_bExternalBuffer)
    {
        LOG("ReadWriteBuffer::Reserve: Used external buffer, can't reallocate\n");
        return;
    }
    if (m_Capacity < capacity)
    {
        m_Capacity = capacity;
        m_RawPtr = (byte*)Realloc(m_RawPtr, m_Capacity);
    }
}

void ReadWriteBuffer::Clear()
{
    m_Size = m_RWOffset = 0;
}

void ReadWriteBuffer::Resize(size_t size)
{
    if (m_Capacity < size)
    {
        if (m_bExternalBuffer)
        {
            LOG("ReadWriteBuffer::Resize: Failed to resize {} (buffer overflowed)\n", m_Name);
            return;
        }

        m_Capacity = size;
        m_RawPtr = (byte*)Realloc(m_RawPtr, m_Capacity);
    }

    m_Size = size;

    if (m_RWOffset > m_Size)
        m_RWOffset = m_Size;
}

size_t ReadWriteBuffer::Read(void* data, size_t sizeInBytes)
{
    size_t bytesToRead{};

    bytesToRead = std::min(sizeInBytes, m_Size - m_RWOffset);
    Core::Memcpy(data, m_RawPtr + m_RWOffset, bytesToRead);

    sizeInBytes -= bytesToRead;
    if (sizeInBytes)
        Core::ZeroMem((uint8_t*)data + bytesToRead, sizeInBytes);

    m_RWOffset += bytesToRead;
    return bytesToRead;
}

size_t ReadWriteBuffer::Write(const void* data, size_t sizeInBytes)
{
    size_t bytesToWrite{};

    size_t requiredSize = m_RWOffset + sizeInBytes;
    if (requiredSize > m_Capacity)
    {
        if (m_bExternalBuffer)
        {
            LOG("ReadWriteBuffer::Write: Failed to write {} (buffer overflowed)\n", m_Name);
            return 0;
        }
        const int mod = requiredSize % m_Granularity;
        m_Capacity = mod ? requiredSize + m_Granularity - mod : requiredSize;
        m_RawPtr = (byte*)Realloc(m_RawPtr, m_Capacity);
    }
    Core::Memcpy(m_RawPtr + m_RWOffset, data, sizeInBytes);
    bytesToWrite = sizeInBytes;

    m_RWOffset += bytesToWrite;
    m_Size = std::max(m_Size, m_RWOffset);
    return bytesToWrite;
}

char* ReadWriteBuffer::Gets(char* str, size_t sizeInBytes)
{
    if (sizeInBytes == 0 || m_RWOffset >= m_Size)
    {
        return nullptr;
    }

    size_t maxChars = sizeInBytes - 1;
    if (m_RWOffset + maxChars > m_Size)
    {
        maxChars = m_Size - m_RWOffset;
    }

    char *memoryPointer, *memory = (char*)&m_RawPtr[m_RWOffset];
    char* stringPointer = str;

    for (memoryPointer = memory; memoryPointer < &memory[maxChars]; memoryPointer++)
    {
        if (*memoryPointer == '\n')
        {
            *stringPointer++ = *memoryPointer++;
            break;
        }
        *stringPointer++ = *memoryPointer;
    }

    *stringPointer = '\0';
    m_RWOffset += memoryPointer - memory;

    return str;
}

void ReadWriteBuffer::Flush()
{}

size_t ReadWriteBuffer::GetOffset() const
{
    return m_RWOffset;
}

bool ReadWriteBuffer::SeekSet(int32_t offset)
{
    m_RWOffset = std::max(int32_t{0}, offset);
    if (m_RWOffset > m_Size)
        m_RWOffset = m_Size;
    return true;
}

bool ReadWriteBuffer::SeekCur(int32_t offset)
{
    if (offset < 0 && -offset > m_RWOffset)
    {
        m_RWOffset = 0;
    }
    else
    {
        size_t prevOffset = m_RWOffset;
        m_RWOffset += offset;
        if (m_RWOffset > m_Size || (offset > 0 && prevOffset > m_RWOffset))
            m_RWOffset = m_Size;
    }
    return true;
}

bool ReadWriteBuffer::SeekEnd(int32_t offset)
{
    if (offset >= 0)
        m_RWOffset = m_Size;
    else if (-offset > m_Size)
        m_RWOffset = 0;
    else
        m_RWOffset = m_Size + offset;
    return true;
}

size_t ReadWriteBuffer::SizeInBytes() const
{
    return m_Size;
}

bool ReadWriteBuffer::IsEOF() const
{
    return m_RWOffset >= m_Size;
}

size_t ReadWriteBuffer::Capacity() const
{
    return m_Capacity;
}

void ReadWriteBuffer::SetGranularity(uint32_t granularity)
{
    m_Granularity = granularity;
}

void* ReadWriteBuffer::RawPtr()
{
    return m_RawPtr;
}

bool ReadWriteBuffer::IsValid() const
{
    return true;
}

HK_NAMESPACE_END
