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

#pragma once

#include "String.h"

HK_NAMESPACE_BEGIN

struct HeapBlob final
{
    HeapBlob() = default;

    HeapBlob(size_t sizeInBytes, void const* data = nullptr, MALLOC_FLAGS flags = MALLOC_FLAGS_DEFAULT)
    {
        Reset(sizeInBytes, data, flags);
    }

    HeapBlob(const HeapBlob&) = delete;
    HeapBlob const& operator=(HeapBlob const&) = delete;

    ~HeapBlob();

    HeapBlob(HeapBlob&& rhs) noexcept :
        m_HeapPtr(rhs.m_HeapPtr), m_HeapSize(rhs.m_HeapSize)
    {
        rhs.m_HeapPtr  = nullptr;
        rhs.m_HeapSize = 0;
    }

    HeapBlob& operator=(HeapBlob&& rhs) noexcept
    {
        Reset();
        m_HeapPtr      = rhs.m_HeapPtr;
        m_HeapSize     = rhs.m_HeapSize;
        rhs.m_HeapPtr  = nullptr;
        rhs.m_HeapSize = 0;
        return *this;
    }

    bool operator==(HeapBlob const& rhs) const
    {
        if (m_HeapSize != rhs.m_HeapSize)
            return false;
        return !std::memcmp(m_HeapPtr, rhs.m_HeapPtr, m_HeapSize);
    }

    bool operator!=(HeapBlob const& rhs) const
    {
        return !(*this == rhs);
    }

    void* GetData() const
    {
        return m_HeapPtr;
    }

    size_t Size() const
    {
        return m_HeapSize;
    }

    bool IsEmpty() const
    {
        return m_HeapSize == 0;
    }

    operator bool() const
    {
        return m_HeapSize != 0;
    }

    void Reset(size_t sizeInBytes, void const* data = nullptr, MALLOC_FLAGS flags = MALLOC_FLAGS_DEFAULT);

    void Reset();

    HeapBlob Clone() const
    {
        return HeapBlob(m_HeapSize, m_HeapPtr);
    }

    operator StringView() const
    {
        return StringView((const char*)m_HeapPtr, m_HeapSize);
    }

    char* ToRawString() const
    {
        return (char*)m_HeapPtr;
    }

    String ToString() const
    {
        return m_HeapPtr ? (const char*)m_HeapPtr : "";
    }

    HK_FORCEINLINE void ZeroMem()
    {
        Core::ZeroMem(m_HeapPtr, m_HeapSize);
    }

private:
    void*  m_HeapPtr{};
    size_t m_HeapSize{};
};

struct BlobRef
{
    BlobRef() = default;

    BlobRef(HeapBlob const& Blob) :
        m_HeapPtr(Blob.GetData()),
        m_HeapSize(Blob.Size())
    {}

    BlobRef(void const* data, size_t size) :
        m_HeapPtr(data),
        m_HeapSize(size)
    {
        HK_ASSERT(m_HeapSize == 0 || m_HeapPtr);
    }

    bool operator==(BlobRef const& rhs) const
    {
        if (m_HeapSize != rhs.m_HeapSize)
            return false;

        return !std::memcmp(m_HeapPtr, rhs.m_HeapPtr, m_HeapSize);
    }

    bool operator!=(BlobRef const& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator==(HeapBlob const& rhs) const
    {
        if (m_HeapSize != rhs.Size())
            return false;

        return !std::memcmp(m_HeapPtr, rhs.GetData(), m_HeapSize);
    }

    bool operator!=(HeapBlob const& rhs) const
    {
        return !(*this == rhs);
    }

    void const* GetData() const
    {
        return m_HeapPtr;
    }

    size_t Size() const
    {
        return m_HeapSize;
    }

    bool IsEmpty() const
    {
        return m_HeapSize == 0;
    }

    operator bool() const
    {
        return m_HeapSize != 0;
    }

    const char* ToRawString() const
    {
        return (const char*)m_HeapPtr;
    }

    String ToString() const
    {
        return m_HeapPtr ? (const char*)m_HeapPtr : "";
    }

    HeapBlob Clone() const
    {
        return HeapBlob(m_HeapSize, m_HeapPtr);
    }

private:
    void const* m_HeapPtr{};
    size_t      m_HeapSize{};
};

HK_NAMESPACE_END
