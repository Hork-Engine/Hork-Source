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

#include "OzzIO.h"

#include <Hork/Core/ReadWriteBuffer.h>
#include <Hork/Core/Logger.h>

#include <ozz/base/io/stream.h>
#include <ozz/base/io/archive.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

HK_NAMESPACE_BEGIN

namespace
{

template <typename T>
UniqueRef<T> ReadChunk(IBinaryStreamReadInterface& stream)
{
    class OzzStream : public ozz::io::Stream
    {
        IBinaryStreamReadInterface& m_ReadInterface;
        size_t m_ChunkOffset;
        size_t m_ChunkSize;

    public:
        OzzStream(IBinaryStreamReadInterface& readInterface, size_t chunkOffset, size_t chunkSize) :
            m_ReadInterface(readInterface), m_ChunkOffset(chunkOffset), m_ChunkSize(chunkSize)
        {}
        virtual bool opened() const override
        {
            return true;
        }
        virtual size_t Read(void* _buffer, size_t _size) override
        {
            return m_ReadInterface.Read(_buffer, _size);
        }
        virtual size_t Write(const void* _buffer, size_t _size) override
        {
            return 0;
        }
        virtual int Seek(int _offset, Origin _origin) override
        {
            bool result;
            switch (_origin)
            {
            case kCurrent:
                result = m_ReadInterface.SeekCur(_offset);
                break;
            case kEnd:
                result = m_ReadInterface.SeekSet(m_ChunkOffset + m_ChunkSize + _offset);
                break;
            case kSet:
                result = m_ReadInterface.SeekSet(m_ChunkOffset + _offset);
                break;
            default:
                result = false;
                break;
            }
            if (result)
                return 0;
            return -1;        
        }
        virtual int Tell() const override
        {
            return m_ReadInterface.GetOffset() - m_ChunkOffset;
        }
        virtual size_t Size() const override
        {
            return m_ChunkSize;
        }
    };

    size_t chunkSize = stream.ReadUInt32();
    size_t chunkOffset = stream.GetOffset();
    if (chunkSize)
    {
        OzzStream ozzStream(stream, chunkOffset, chunkSize);
        ozz::io::IArchive archive(&ozzStream);
        if (!archive.TestTag<T>())
        {
            stream.SeekSet(chunkOffset + chunkSize);
            return {};
        }
        auto data = MakeUnique<T>();
        archive >> *data;
        stream.SeekSet(chunkOffset + chunkSize);

        return data;
    }
    return {};
}

template <typename T>
void WriteChunk(IBinaryStreamWriteInterface& stream, T const* data)
{
    class OzzWriteStream : public ozz::io::Stream
    {
        IBinaryStreamWriteInterface& m_WriteInterface;

    public:
        OzzWriteStream(IBinaryStreamWriteInterface& writeInterface) :
            m_WriteInterface(writeInterface)
        {}
        virtual bool opened() const override
        {
            return true;
        }
        virtual size_t Read(void* _buffer, size_t _size) override
        {
            return 0;
        }
        virtual size_t Write(const void* _buffer, size_t _size) override
        {
            return m_WriteInterface.Write(_buffer, _size);
        }
        virtual int Seek(int _offset, Origin _origin) override
        {
            bool result;
            switch (_origin)
            {
            case kCurrent:
                result = m_WriteInterface.SeekCur(_offset);
                break;
            case kEnd:
                result = m_WriteInterface.SeekEnd(_offset);
                break;
            case kSet:
                result = m_WriteInterface.SeekSet(_offset);
                break;
            default:
                result = false;
                break;
            }
            if (result)
                return 0;
            return -1;        
        }
        virtual int Tell() const override
        {
            return m_WriteInterface.GetOffset();
        }
        virtual size_t Size() const override
        {
            return m_WriteInterface.SizeInBytes();
        }
    };

    if (data)
    {
        ReadWriteBuffer writeBuffer;

        OzzWriteStream ozzStream(writeBuffer);
        ozz::io::OArchive archive(&ozzStream);
        archive << *data;

        stream.WriteUInt32(writeBuffer.SizeInBytes());
        stream.Write(writeBuffer.RawPtr(), writeBuffer.SizeInBytes());
    }
    else
    {
        stream.WriteUInt32(0);
    }
}

}

UniqueRef<OzzSkeleton> OzzReadSkeleton(IBinaryStreamReadInterface& stream)
{
    return ReadChunk<OzzSkeleton>(stream);
}

void OzzWriteSkeleton(IBinaryStreamWriteInterface& stream, OzzSkeleton const* skeleton)
{
    WriteChunk(stream, skeleton);
}

UniqueRef<OzzAnimation> OzzReadAnimation(IBinaryStreamReadInterface& stream)
{
    return ReadChunk<OzzAnimation>(stream);
}

void OzzWriteAnimation(IBinaryStreamWriteInterface& stream, OzzAnimation const* animation)
{
    WriteChunk(stream, animation);
}

HK_NAMESPACE_END
