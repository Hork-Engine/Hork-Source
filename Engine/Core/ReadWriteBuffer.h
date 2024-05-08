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

#pragma once

#include "BinaryStream.h"

HK_NAMESPACE_BEGIN

class ReadWriteBuffer final : public IBinaryStreamReadInterface, public IBinaryStreamWriteInterface
{
public:
    ReadWriteBuffer() = default;

    ReadWriteBuffer(ReadWriteBuffer const&) = delete;
    ReadWriteBuffer& operator=(ReadWriteBuffer const&) = delete;

    ReadWriteBuffer(ReadWriteBuffer&& rhs) noexcept;
    ReadWriteBuffer& operator=(ReadWriteBuffer&& rhs) noexcept;

    ~ReadWriteBuffer();

    void            SetName(StringView Name);

    StringView      GetName() const override;

    void            SetExternalBuffer(void* pMemoryBuffer, size_t SizeInBytes);

    void            SetInternalBuffer(size_t BaseCapacity);

    void            Reset();

    void            Reserve(size_t Capacity);

    void            Clear();

    void            Resize(size_t Size);
 
    void*           RawPtr();

    size_t          Capacity() const;

    void            SetGranularity(uint32_t Granularity);

    size_t          Read(void* pBuffer, size_t SizeInBytes) override;

    size_t          Write(const void* pBuffer, size_t SizeInBytes) override;

    char*           Gets(char* pBuffer, size_t SizeInBytes) override;

    void            Flush() override;

    size_t          GetOffset() const override;

    bool            SeekSet(int32_t Offset) override;

    bool            SeekCur(int32_t Offset) override;

    bool            SeekEnd(int32_t Offset) override;

    size_t          SizeInBytes() const override;

    bool            IsEOF() const override;

    bool            IsValid() const override;

private:
    String      m_Name;
    byte*       m_RawPtr{};
    size_t      m_RWOffset{};
    size_t      m_Size{};
    size_t      m_Capacity{};
    uint32_t    m_Granularity{1024};
    bool        m_bExternalBuffer{};
};

HK_NAMESPACE_END
