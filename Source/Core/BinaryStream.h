/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "EndianSwap.h"
#include <Platform/String.h>

/**

IBinaryStream

Interface class for binary stream

*/
class IBinaryStream
{
public:
    IBinaryStream()
    {}

    virtual ~IBinaryStream()
    {}

    virtual const char* GetFileName() const = 0;

    virtual size_t Read(void* pBuffer, size_t SizeInBytes) = 0;

    virtual size_t Write(const void* pBuffer, size_t SizeInBytes) = 0;

    virtual char* Gets(char* pBuffer, size_t SizeInBytes) = 0;

    virtual void Flush() = 0;

    virtual size_t GetOffset() = 0;

    virtual bool SeekSet(int32_t Offset) = 0;

    virtual bool SeekCur(int32_t Offset) = 0;

    virtual bool SeekEnd(int32_t Offset) = 0;

    virtual size_t SizeInBytes() = 0;

    virtual bool Eof() = 0;

    void ReadCString(char* pBuffer, size_t SizeInBytes)
    {
        if (!SizeInBytes)
            return;

        size_t size     = ReadUInt32();
        size_t capacity = std::min(size + 1, SizeInBytes);
        Read(pBuffer, capacity - 1);
        pBuffer[capacity - 1] = 0;

        size_t skipBytes = size - (SizeInBytes - 1);
        if (skipBytes > 0)
        {
            SeekCur(skipBytes);
        }
    }

    int8_t ReadInt8()
    {
        int8_t i;
        Read(&i, sizeof(i));
        return i;
    }

    template <typename T>
    void ReadArrayInt8(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadInt8ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadInt8ToBuffer(int8_t* pBuffer, int Count)
    {
        Read(pBuffer, Count);
    }

    uint8_t ReadUInt8()
    {
        uint8_t i;
        Read(&i, sizeof(i));
        return i;
    }

    template <typename T>
    void ReadArrayUInt8(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadUInt8ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadUInt8ToBuffer(uint8_t* pBuffer, int Count)
    {
        Read(pBuffer, Count);
    }

    int16_t ReadInt16()
    {
        int16_t i;
        Read(&i, sizeof(i));
        return Core::LittleWord(i);
    }

    template <typename T>
    void ReadArrayInt16(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadInt16ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadInt16ToBuffer(int16_t* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleWord(pBuffer[i]);
        }
    }

    uint16_t ReadUInt16()
    {
        uint16_t i;
        Read(&i, sizeof(i));
        return Core::LittleWord(i);
    }

    template <typename T>
    void ReadArrayUInt16(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadUInt16ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadUInt16ToBuffer(uint16_t* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleWord(pBuffer[i]);
        }
    }

    int32_t ReadInt32()
    {
        int32_t i;
        Read(&i, sizeof(i));
        return Core::LittleDWord(i);
    }

    template <typename T>
    void ReadArrayInt32(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadInt32ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadInt32ToBuffer(int32_t* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleDWord(pBuffer[i]);
        }
    }

    uint32_t ReadUInt32()
    {
        uint32_t i;
        Read(&i, sizeof(i));
        return Core::LittleDWord(i);
    }

    template <typename T>
    void ReadArrayUInt32(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadUInt32ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadUInt32ToBuffer(uint32_t* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleDWord(pBuffer[i]);
        }
    }

    int64_t ReadInt64()
    {
        int64_t i;
        Read(&i, sizeof(i));
        return Core::LittleDDWord(i);
    }

    template <typename T>
    void ReadArrayInt64(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadInt64ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadInt64ToBuffer(int64_t* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleDDWord(pBuffer[i]);
        }
    }

    uint64_t ReadUInt64()
    {
        uint64_t i;
        Read(&i, sizeof(i));
        return Core::LittleDDWord(i);
    }

    template <typename T>
    void ReadArrayUInt64(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadUInt64ToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadUInt64ToBuffer(uint64_t* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleDDWord(pBuffer[i]);
        }
    }

    float ReadFloat()
    {
        uint32_t i = ReadUInt32();
        return *(float*)&i;
    }

    template <typename T>
    void ReadArrayFloat(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadFloatToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadFloatToBuffer(float* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleFloat(pBuffer[i]);
        }
    }

    double ReadDouble()
    {
        uint64_t i = ReadUInt64();
        return *(double*)&i;
    }

    template <typename T>
    void ReadArrayDouble(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadDoubleToBuffer(Array.ToPtr(), Array.Size());
    }

    void ReadDoubleToBuffer(double* pBuffer, int Count)
    {
        Read(pBuffer, Count * sizeof(pBuffer[0]));
        for (int i = 0; i < Count; i++)
        {
            pBuffer[i] = Core::LittleDouble(pBuffer[i]);
        }
    }

    bool ReadBool()
    {
        return !!ReadUInt8();
    }

    template <typename T>
    void ReadObject(T& Object)
    {
        Object.Read(*this);
    }

    template <typename T>
    void ReadArrayOfStructs(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        for (int i = 0; i < Array.Size(); i++)
        {
            ReadObject(Array[i]);
        }
    }

    void WriteCString(const char* pRawStr)
    {
        int len = Platform::Strlen(pRawStr);
        WriteUInt32(len);
        Write(pRawStr, len);
    }

    void WriteInt8(int8_t i)
    {
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayInt8(T const& Array)
    {
        WriteUInt32(Array.Size());
        Write(Array.ToPtr(), Array.Size());
    }

    void WriteUInt8(uint8_t i)
    {
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayUInt8(T const& Array)
    {
        WriteUInt32(Array.Size());
        Write(Array.ToPtr(), Array.Size());
    }

    void WriteInt16(int16_t i)
    {
        i = Core::LittleWord(i);
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayInt16(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteInt16(Array[i]);
        }
    }

    void WriteUInt16(uint16_t i)
    {
        i = Core::LittleWord(i);
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayUInt16(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteUInt16(Array[i]);
        }
    }

    void WriteInt32(int32_t i)
    {
        i = Core::LittleDWord(i);
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayInt32(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteInt32(Array[i]);
        }
    }

    void WriteUInt32(uint32_t i)
    {
        i = Core::LittleDWord(i);
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayUInt32(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteUInt32(Array[i]);
        }
    }

    void WriteInt64(int64_t i)
    {
        i = Core::LittleDDWord(i);
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayInt64(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteInt64(Array[i]);
        }
    }

    void WriteUInt64(uint64_t i)
    {
        i = Core::LittleDDWord(i);
        Write(&i, sizeof(i));
    }

    template <typename T>
    void WriteArrayUInt64(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteUInt64(Array[i]);
        }
    }

    void WriteFloat(float f)
    {
        WriteUInt32(*(uint32_t*)&f);
    }

    template <typename T>
    void WriteArrayFloat(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteFloat(Array[i]);
        }
    }

    void WriteDouble(double f)
    {
        WriteUInt64(*(uint64_t*)&f);
    }

    template <typename T>
    void WriteArrayDouble(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteDouble(Array[i]);
        }
    }

    void WriteBool(bool b)
    {
        WriteUInt8(uint8_t(b));
    }

    template <typename T>
    void WriteObject(T const& Object)
    {
        Object.Write(*this);
    }

    template <typename T>
    void WriteArrayOfStructs(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (int i = 0; i < Array.Size(); i++)
        {
            WriteObject(Array[i]);
        }
    }

    void Rewind()
    {
        SeekSet(0);
    }

    void Printf(const char* Format, ...)
    {
        extern thread_local char LogBuffer[16384]; // Use existing log buffer
        va_list                  VaList;
        va_start(VaList, Format);
        int len = Platform::VSprintf(LogBuffer, sizeof(LogBuffer), Format, VaList);
        va_end(VaList);
        Write(LogBuffer, len);
    }
};
