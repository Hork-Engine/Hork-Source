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

#include "EndianSwap.h"
#include "Format.h"
#include "String.h"
#include "HeapBlob.h"
#include "Ref.h"
#include "Logger.h"

HK_NAMESPACE_BEGIN

class IBinaryStreamBaseInterface : public RefCounted
{
public:
    virtual ~IBinaryStreamBaseInterface() = default;

    virtual bool IsValid() const = 0;

    virtual size_t SizeInBytes() const = 0;

    virtual size_t GetOffset() const = 0;

    virtual bool SeekSet(int32_t offset) = 0;

    virtual bool SeekCur(int32_t offset) = 0;

    virtual bool SeekEnd(int32_t offset) = 0;

    void Rewind()
    {
        SeekSet(0);
    }

    virtual bool IsEOF() const = 0;

    virtual StringView GetName() const = 0;
};


class IBinaryStreamReadInterface : public virtual IBinaryStreamBaseInterface
{
public:
    virtual ~IBinaryStreamReadInterface() = default;

    virtual size_t Read(void* data, size_t sizeInBytes) = 0;

    virtual char* Gets(char* str, size_t sizeInBytes) = 0;

    void ReadStringToBuffer(char* str, size_t sizeInBytes)
    {
        if (!sizeInBytes)
            return;

        size_t size     = ReadUInt32();
        size_t capacity = std::min(size + 1, sizeInBytes);
        Read(str, capacity - 1);
        str[capacity - 1] = 0;

        if (size > sizeInBytes - 1)
        {
            SeekCur(size - (sizeInBytes - 1));
        }
    }

    template <typename CharT = char, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
    TString<CharT, Allocator> ReadString()
    {
        uint32_t len  = ReadUInt32();
        StringSizeType size = len;

        if (len > MaxStringSize)
        {
            LOG("Couldn't read entire string from file - string is too long\n");
            size = MaxStringSize;
        }

        TString<CharT, Allocator> str;
        str.Resize(size, STRING_RESIZE_NO_FILL_SPACES);

        CharT* data = str.ToPtr();

#ifdef HK_BIG_ENDIAN
        if (IsWideString())
        {
            for (StringSizeType i = 0; i < size; i++)
                data[i] = ReadUInt16();
        }
        else
#else
        {
            Read(data, size * sizeof(CharT));
        }
#endif
        if (len != size)
        {
            // Keep the correct offset
            SeekCur((len - size) * sizeof(CharT));
        }

        return str;
    }

    /// Reads the file as a null-terminated string.
    template <typename CharT = char, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
    TString<CharT, Allocator> AsString()
    {
        Rewind();

        StringSizeType size = SizeInBytes() / sizeof(CharT);
        if (size > MaxStringSize)
        {
            LOG("Couldn't read entire string from file - string is too long\n");
            size = MaxStringSize;
        }

        TString<CharT, Allocator> str;
        str.Resize(size, STRING_RESIZE_NO_FILL_SPACES);

        CharT* data = str.ToPtr();

#ifdef HK_BIG_ENDIAN
        if (IsWideString())
        {
            for (StringSizeType i = 0; i < size; i++)
                data[i] = ReadUInt16();
        }
        else
#else
        {
            size_t numchars = Read(data, size * sizeof(CharT)) / sizeof(CharT);
            if (numchars < size)
                str.Resize(numchars);
        }
#endif
        return str;
    }

    HeapBlob ReadBlob(size_t sizeInBytes)
    {
        HeapBlob blob(sizeInBytes);

        Read(blob.GetData(), blob.Size());
        return blob;
    }

    HeapBlob AsBlob()
    {
        Rewind();
        return ReadBlob(SizeInBytes());
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

    Half ReadHalf()
    {
        Half hf;
        hf.v = ReadUInt16();
        return hf;
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
    void ReadWords(T* data, const size_t count = 1)
    {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported integer");

        Read(data, count * sizeof(T));

        #ifdef HK_BIG_ENDIAN
        switch (sizeof(T))
        {
            case 1:
                break;
            case 2:
                for (size_t i = 0; i < count; i++)
                    data[i] = Core::LittleWord(data[i]);
                break;
            case 4:
                for (size_t i = 0; i < count; i++)
                    data[i] = Core::LittleDWord(data[i]);
                break;
            case 8:
                for (size_t i = 0; i < count; i++)
                    data[i] = Core::LittleDDWord(data[i]);
                break;
        }
        #endif
    }

    template <typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
    void ReadFloats(T* data, const size_t count = 1)
    {
        static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point");

        Read(data, count * sizeof(T));

        #ifdef HK_BIG_ENDIAN
        switch (sizeof(T))
        {
            case 4:
                for (size_t i = 0; i < count; i++)
                    data[i] = Core::LittleFloat(data[i]);
                break;
            case 8:
                for (size_t i = 0; i < count; i++)
                    data[i] = Core::LittleDouble(data[i]);
                break;
        }
        #endif
    }  

    template <typename T>
    void ReadObject(T& object)
    {
        object.Read(*this);
    }

    template <typename T, std::enable_if_t<std::is_integral<typename T::ValueType>::value, bool> = true>
    void ReadArray(T& array)
    {
        uint32_t size = ReadUInt32();
        array.ResizeInvalidate(size);
        ReadWords(array.ToPtr(), array.Size());
    }

    template <typename T, std::enable_if_t<std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void ReadArray(T& array)
    {
        uint32_t size = ReadUInt32();
        array.ResizeInvalidate(size);
        ReadFloats(array.ToPtr(), array.Size());
    }

    template <typename T, std::enable_if_t<!std::is_integral<typename T::ValueType>::value && !std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void ReadArray(T& array)
    {
        uint32_t size = ReadUInt32();
        array.ResizeInvalidate(size);
        for (typename T::SizeType i = 0; i < array.Size(); i++)
        {
            ReadObject(array[i]);
        }
    }
};

class IBinaryStreamWriteInterface : public virtual IBinaryStreamBaseInterface
{
public:
    virtual size_t Write(const void* data, size_t sizeInBytes) = 0;

    virtual void Flush() = 0;

    void WriteString(StringView Str)
    {
        StringSizeType size = Str.Size();
        WriteUInt32(size);
        Write(Str.ToPtr(), size);
    }

    void WriteString(WideStringView Str)
    {
        StringSizeType size = Str.Size();
        WriteUInt32(size);

#ifdef HK_BIG_ENDIAN
        for (StringSizeType i = 0; i < size; i++)
            WriteUInt16(Str[i]);
#else
        Write(Str.ToPtr(), size * sizeof(uint16_t));
#endif
    }

    void WriteBlob(BlobRef Blob)
    {
        Write(Blob.GetData(), Blob.Size());
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

    void WriteHalf(Half hf)
    {
        WriteUInt16(hf.v);
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

    template <typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
    void WriteWords(T* data, const size_t count = 1)
    {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported integer");

#ifdef HK_LITTLE_ENDIAN
        Write(data, count * sizeof(T));
#else
        switch (sizeof(T))
        {
            case 1:
                Write(Buffer, count);
                break;
            case 2:
                for (size_t i = 0; i < count; i++)
                    WriteUInt16(data[i]);
                break;
            case 4:
                for (size_t i = 0; i < count; i++)
                    WriteUInt32(data[i]);
                break;
            case 8:
                for (size_t i = 0; i < count; i++)
                    WriteUInt64(data[i]);
                break;
        }
#endif
    }

    template <typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
    void WriteFloats(T* data, const size_t count = 1)
    {
        static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point");

#ifdef HK_LITTLE_ENDIAN
        Write(data, count * sizeof(T));
#else
        switch (sizeof(T))
        {
            case 4:
                for (size_t i = 0; i < count; i++)
                    WriteFloat(data[i]);
                break;
            case 8:
                for (size_t i = 0; i < count; i++)
                    WriteDouble(data[i]);
                break;
        }
#endif
    }  

    template <typename T>
    void WriteObject(T const& object)
    {
        object.Write(*this);
    }

    template <typename T, std::enable_if_t<std::is_integral<typename T::ValueType>::value, bool> = true>
    void WriteArray(T& array)
    {
        WriteUInt32(array.Size());
        WriteWords(array.ToPtr(), array.Size());
    }

    template <typename T, std::enable_if_t<std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void WriteArray(T& array)
    {
        WriteUInt32(array.Size());
        WriteFloats(array.ToPtr(), array.Size());
    }

    template<typename T, std::enable_if_t<!std::is_integral<typename T::ValueType>::value && !std::is_floating_point<typename T::ValueType>::value, bool> = true>
    void WriteArray(T const& array)
    {
        WriteUInt32(array.Size());
        for (typename T::SizeType i = 0; i < array.Size(); i++)
        {
            WriteObject(array[i]);
        }
    }

    template <typename... T>
    HK_FORCEINLINE void FormattedPrint(fmt::format_string<T...> format, T&&... args)
    {
        fmt::memory_buffer buffer;
        fmt::detail::vformat_to(buffer, fmt::string_view(format), fmt::make_format_args(args...));
        Write(buffer.data(), buffer.size());
    }
};

HK_NAMESPACE_END
