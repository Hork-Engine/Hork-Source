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

#include <Platform/EndianSwap.h>
#include <Platform/String.h>
#include <Platform/Format.h>

class IBinaryStreamBaseInterface
{
public:
    virtual ~IBinaryStreamBaseInterface()
    {}

    virtual size_t SizeInBytes() const = 0;

    virtual size_t GetOffset() const = 0;

    virtual bool SeekSet(int32_t Offset) = 0;

    virtual bool SeekCur(int32_t Offset) = 0;

    virtual bool SeekEnd(int32_t Offset) = 0;

    void Rewind()
    {
        SeekSet(0);
    }

    virtual bool Eof() const = 0;

    virtual const char* GetFileName() const = 0;
};


class IBinaryStreamReadInterface : public virtual IBinaryStreamBaseInterface
{
public:
    virtual ~IBinaryStreamReadInterface()
    {}

    virtual size_t Read(void* pBuffer, size_t SizeInBytes) = 0;

    virtual char* Gets(char* pBuffer, size_t SizeInBytes) = 0;

    void ReadCString(char* pBuffer, size_t SizeInBytes)
    {
        if (!SizeInBytes)
            return;

        size_t size     = ReadUInt32();
        size_t capacity = std::min(size + 1, SizeInBytes);
        Read(pBuffer, capacity - 1);
        pBuffer[capacity - 1] = 0;

        if (size > SizeInBytes - 1)
        {
            SeekCur(size - (SizeInBytes - 1));
        }
    }

    int8_t ReadInt8()
    {
        int8_t i;
        ReadWords(&i);
        return i;
    }

    uint8_t ReadUInt8()
    {
        uint8_t i;
        ReadWords(&i);
        return i;
    }

    int16_t ReadInt16()
    {
        int16_t i;
        ReadWords(&i);
        return i;
    }

    uint16_t ReadUInt16()
    {
        uint16_t i;
        ReadWords(&i);
        return i;
    }

    int32_t ReadInt32()
    {
        int32_t i;
        ReadWords(&i);
        return i;
    }

    uint32_t ReadUInt32()
    {
        uint32_t i;
        ReadWords(&i);
        return i;
    }

    int64_t ReadInt64()
    {
        int64_t i;
        ReadWords(&i);
        return i;
    }

    uint64_t ReadUInt64()
    {
        uint64_t i;
        ReadWords(&i);
        return i;
    }

    float ReadFloat()
    {
        float f;
        ReadFloats(&f);
        return f;
    }

    double ReadDouble()
    {
        double f;
        ReadFloats(&f);
        return f;
    }

    bool ReadBool()
    {
        return !!ReadUInt8();
    }

    template <typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
    void ReadWords(T* pBuffer, const size_t Count = 1)
    {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported integer");

        Read(pBuffer, Count * sizeof(T));

        #ifdef HK_BIG_ENDIAN
        switch (sizeof(T))
        {
            case 1:
                break;
            case 2:
                for (size_t i = 0; i < Count; i++)
                    pBuffer[i] = Core::LittleWord(pBuffer[i]);
                break;
            case 4:
                for (size_t i = 0; i < Count; i++)
                    pBuffer[i] = Core::LittleDWord(pBuffer[i]);
                break;
            case 8:
                for (size_t i = 0; i < Count; i++)
                    pBuffer[i] = Core::LittleDDWord(pBuffer[i]);
                break;
        }
        #endif
    }

    template <typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
    void ReadFloats(T* pBuffer, const size_t Count = 1)
    {
        static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point");

        Read(pBuffer, Count * sizeof(T));

        #ifdef HK_BIG_ENDIAN
        switch (sizeof(T))
        {
            case 4:
                for (size_t i = 0; i < Count; i++)
                    pBuffer[i] = Core::LittleFloat(pBuffer[i]);
                break;
            case 8:
                for (size_t i = 0; i < Count; i++)
                    pBuffer[i] = Core::LittleDouble(pBuffer[i]);
                break;
        }
        #endif
    }  

    template <typename T>
    void ReadObject(T& Object)
    {
        Object.Read(*this);
    }

    template <typename T, std::enable_if_t<std::is_integral<typename T::ValueType>::value, bool> = true>
    void ReadArray(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadWords(Array.ToPtr(), Array.Size());
    }

    template <typename T, std::enable_if_t<std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void ReadArray(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        ReadFloats(Array.ToPtr(), Array.Size());
    }

    template <typename T, std::enable_if_t<!std::is_integral<typename T::ValueType>::value && !std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void ReadArray(T& Array)
    {
        uint32_t size = ReadUInt32();
        Array.ResizeInvalidate(size);
        for (typename T::SizeType i = 0; i < Array.Size(); i++)
        {
            ReadObject(Array[i]);
        }
    }
};

class IBinaryStreamWriteInterface : public virtual IBinaryStreamBaseInterface
{
public:
    virtual size_t Write(const void* pBuffer, size_t SizeInBytes) = 0;

    virtual void Flush() = 0;

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

    void WriteUInt8(uint8_t i)
    {
        Write(&i, sizeof(i));
    }

    void WriteInt16(int16_t i)
    {
        i = Core::LittleWord(i);
        Write(&i, sizeof(i));
    }

    void WriteUInt16(uint16_t i)
    {
        i = Core::LittleWord(i);
        Write(&i, sizeof(i));
    }

    void WriteInt32(int32_t i)
    {
        i = Core::LittleDWord(i);
        Write(&i, sizeof(i));
    }

    void WriteUInt32(uint32_t i)
    {
        i = Core::LittleDWord(i);
        Write(&i, sizeof(i));
    }

    void WriteInt64(int64_t i)
    {
        i = Core::LittleDDWord(i);
        Write(&i, sizeof(i));
    }

    void WriteUInt64(uint64_t i)
    {
        i = Core::LittleDDWord(i);
        Write(&i, sizeof(i));
    }

    void WriteFloat(float f)
    {
        f = Core::LittleFloat(f);
        Write(&f, sizeof(f));
    }

    void WriteDouble(double f)
    {
        f = Core::LittleDouble(f);
        Write(&f, sizeof(f));
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

    template <typename T, std::enable_if_t<std::is_integral<typename T::ValueType>::value, bool> = true>
    void WriteArray(T& Array)
    {
        using ElementType = typename T::ValueType;

        static_assert(sizeof(ElementType) == 4 || sizeof(ElementType) == 8, "Unsupported integer");

        WriteUInt32(Array.Size());

        switch (sizeof(ElementType))
        {
            case 1:
                Write(Array.ToPtr(), Array.Size());
                break;
            case 2:
                for (typename T::SizeType i = 0; i < Array.Size(); i++)
                    WriteUInt16(Array[i]);
                break;
            case 4:
                for (typename T::SizeType i = 0; i < Array.Size(); i++)
                    WriteUInt32(Array[i]);
                break;
            case 8:
                for (typename T::SizeType i = 0; i < Array.Size(); i++)
                    WriteUInt64(Array[i]);
                break;
        }
    }

    template <typename T, std::enable_if_t<std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void WriteArray(T& Array)
    {
        using ElementType = typename T::ValueType;

        static_assert(sizeof(ElementType) == 4 || sizeof(ElementType) == 8, "Unsupported floating point");

        WriteUInt32(Array.Size());

        switch (sizeof(ElementType))
        {
            case 4:
                for (typename T::SizeType i = 0; i < Array.Size(); i++)
                    WriteFloat(Array[i]);
                break;
            case 8:
                for (typename T::SizeType i = 0; i < Array.Size(); i++)
                    WriteDouble(Array[i]);
                break;
        }
    }

    template<typename T, std::enable_if_t<!std::is_integral<typename T::ValueType>::value && !std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void WriteArray(T const& Array)
    {
        WriteUInt32(Array.Size());
        for (typename T::SizeType i = 0; i < Array.Size(); i++)
        {
            WriteObject(Array[i]);
        }
    }

    template <typename... T>
    HK_FORCEINLINE void FormattedPrint(fmt::format_string<T...> Format, T&&... args)
    {
        fmt::memory_buffer buffer;
        fmt::detail::vformat_to(buffer, fmt::string_view(Format), fmt::make_format_args(args...));
        Write(buffer.data(), buffer.size());
    }
};
