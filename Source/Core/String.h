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

#include <Platform/Memory/Memory.h>
#include <Platform/Path.h>

#include "HashFunc.h"
#include "BinaryStream.h"

class AString;

using AStdString = std::basic_string<char, std::char_traits<char>, TStdZoneAllocator<char>>;

class AStringView
{
public:
    HK_FORCEINLINE AStringView() :
        Data(""), Size(0)
    {
    }

    HK_FORCEINLINE AStringView(const char* Rhs) :
        Data(Rhs), Size(Platform::Strlen(Rhs))
    {
    }

    HK_FORCEINLINE AStringView(const char* _Str, int _Length) :
        Data(_Str), Size(_Length)
    {
        HK_ASSERT(_Length >= 0);
    }

    HK_FORCEINLINE AStringView(AString const& Str);
    HK_FORCEINLINE AStringView(AStdString const& Str);

    AStringView(AStringView const& Str) = default;
    AStringView& operator=(AStringView const& Rhs) = default;

    AStringView& operator=(AString const& Rhs);

    friend bool operator==(AStringView Lhs, AStringView Rhs);
    friend bool operator!=(AStringView Lhs, AStringView Rhs);

    HK_FORCEINLINE const char& operator[](const int _Index) const
    {
        HK_ASSERT_((_Index >= 0) && (_Index <= Size), "AStringView[]");
        return Data[_Index];
    }

    AString ToString() const;

    /** Return is string empty or not */
    HK_FORCEINLINE bool IsEmpty() const
    {
        return Size == 0;
    }

    /** Return string length */
    HK_FORCEINLINE int Length() const
    {
        return Size;
    }

    /** Pointer to the beggining of the string */
    HK_FORCEINLINE const char* Begin() const
    {
        return Data;
    }

    /** Pointer to the ending of the string */
    HK_FORCEINLINE const char* End() const
    {
        return Data + Size;
    }

    /** Get raw pointer */
    HK_FORCEINLINE const char* ToPtr() const
    {
        return Data;
    }

    /** Find the character. Return character position in string or -1. */
    int Contains(char _Ch) const
    {
        const char* end = Data + Size;
        for (const char* s = Data; s < end; s++)
        {
            if (*s == _Ch)
            {
                return s - Data;
            }
        }
        return -1;
    }

    /** Find the substring. Return substring position in string or -1. */
    int FindSubstring(AStringView _Substring) const
    {
        if (_Substring.IsEmpty())
        {
            return -1;
        }
        const char* s   = Data;
        const char* end = Data + Size;
        while (s < end)
        {
            if (AStringView(s, end - s).CmpN(_Substring, _Substring.Length()) == 0)
            {
                return (int)(s - Data);
            }
            ++s;
        }
        return -1;
    }

    /** Find the substring. Return substring position in string or -1. */
    int FindSubstringIcmp(AStringView _Substring) const
    {
        if (_Substring.IsEmpty())
        {
            return -1;
        }
        const char* s   = Data;
        const char* end = Data + Size;
        while (*s)
        {
            if (AStringView(s, end - s).IcmpN(_Substring, _Substring.Length()) == 0)
            {
                return (int)(s - Data);
            }
            ++s;
        }
        return -1;
    }

    /** Get substring. */
    AStringView GetSubstring(size_t _Pos, size_t _Size) const
    {
        if (_Pos > _Size || _Size == 0)
        {
            return AStringView();
        }

        if (_Pos + _Size > Size)
        {
            _Size = Size - _Pos;
        }

        return AStringView(Data + _Pos, _Size);
    }

    HK_FORCEINLINE uint32_t HexToUInt32() const
    {
        return Platform::HexToUInt32(Data, std::min(Size, 8));
    }

    HK_FORCEINLINE uint64_t HexToUInt64() const
    {
        return Platform::HexToUInt64(Data, std::min(Size, 16));
    }

    /** Compare the strings (case insensitive) */
    int Icmp(AStringView Rhs) const;

    /** Compare the strings (case sensitive) */
    int Cmp(AStringView Rhs) const;

    /** Compare the strings (case insensitive) */
    int IcmpN(AStringView Rhs, int _Num) const;

    /** Compare the strings (case sensitive) */
    int CmpN(AStringView Rhs, int _Num) const;

    /** Path utility. Return index of the path end. */
    int FindPath() const
    {
        const char* p = Data + Size;
        while (--p >= Data)
        {
            if (Platform::IsPathSeparator(*p))
            {
                return p - Data + 1;
            }
        }
        return 0;
    }

    /** Path utility. Get filename without path. */
    HK_FORCEINLINE AStringView GetFilenameNoPath() const
    {
        const char* p = Data + Size;
        while (--p >= Data && !Platform::IsPathSeparator(*p))
        {
            ;
        }
        ++p;
        return AStringView(p, Data + Size - p);
    }

    /** Path utility. Get full filename without extension. */
    HK_FORCEINLINE AStringView GetFilenameNoExt() const
    {
        int         sz = Size;
        const char* p  = Data + Size;
        while (--p >= Data)
        {
            if (*p == '.')
            {
                sz = p - Data;
                break;
            }
            if (Platform::IsPathSeparator(*p))
            {
                break; // no extension
            }
        }
        return AStringView(Data, sz);
    }

    /** Path utility. Get path without file name. */
    HK_FORCEINLINE AStringView GetFilePath() const
    {
        const char* p = Data + Size;
        while (--p > Data && !Platform::IsPathSeparator(*p))
        {
            ;
        }
        return AStringView(Data, p - Data);
    }

    /** Path utility. Check file extension. */
    bool CompareExt(AStringView _Ext, bool _CaseInsensitive = true) const
    {
        if (_Ext.IsEmpty())
        {
            return false;
        }
        const char* p   = Data + Size;
        const char* ext = _Ext.Data + _Ext.Size;
        if (_CaseInsensitive)
        {
            char c1, c2;
            while (--ext >= _Ext.Data)
            {

                if (--p < Data)
                {
                    return false;
                }

                c1 = *p;
                c2 = *ext;

                if (c1 != c2)
                {
                    if (c1 >= 'a' && c1 <= 'z')
                    {
                        c1 -= ('a' - 'A');
                    }
                    if (c2 >= 'a' && c2 <= 'z')
                    {
                        c2 -= ('a' - 'A');
                    }
                    if (c1 != c2)
                    {
                        return false;
                    }
                }
            }
        }
        else
        {
            while (--ext >= _Ext.Data)
            {
                if (--p < Data || *p != *ext)
                {
                    return false;
                }
            }
        }
        return true;
    }

    /** Path utility. Return index where the extension begins. */
    int FindExt() const
    {
        const char* p = Data + Size;
        while (--p >= Data && !Platform::IsPathSeparator(*p))
        {
            if (*p == '.')
            {
                return p - Data;
            }
        }
        return Size;
    }

    /** Path utility. Return index where the extension begins after dot. */
    int FindExtWithoutDot() const
    {
        const char* p = Data + Size;
        while (--p >= Data && !Platform::IsPathSeparator(*p))
        {
            if (*p == '.')
            {
                return p - Data + 1;
            }
        }
        return Size;
    }

    /** Path utility. Get filename extension. */
    AStringView GetExt() const
    {
        const char* p = Data + FindExt();
        return AStringView(p, Data + Size - p);
    }

    /** Path utility. Get filename extension without dot. */
    AStringView GetExtWithoutDot() const
    {
        const char* p = Data + FindExtWithoutDot();
        return AStringView(p, Data + Size - p);
    }

    AStringView TruncateHead(int _Count) const
    {
        if (_Count > Size)
        {
            _Count = Size;
        }
        return AStringView(Data + _Count, Size - _Count);
    }

    AStringView TruncateTail(int _Count) const
    {
        if (_Count > Size)
        {
            _Count = Size;
        }
        return AStringView(Data, Size - _Count);
    }

    /** Get string hash */
    HK_FORCEINLINE int Hash() const
    {
        return Core::Hash(Data, Size);
    }

    /** Get string hash case insensitive */
    HK_FORCEINLINE int HashCase() const
    {
        return Core::HashCase(Data, Size);
    }

    HK_FORCEINLINE void Write(IBinaryStream& _Stream) const
    {
        _Stream.WriteUInt32(Size);
        _Stream.WriteBuffer(Data, Size);
    }

private:
    const char* Data;
    int         Size;
};


/**

AString

*/
class AString final
{
    using Allocator = AZoneAllocator;

public:
    static const int GRANULARITY   = 32;
    static const int BASE_CAPACITY = 32;

    AString();
    AString(AString const& Rhs);
    AString(AString&& Rhs) noexcept;
    AString(AStringView Rhs);
    AString(const char* _Begin, const char* _End);
    ~AString();

    const char& operator[](const int _Index) const;
    char&       operator[](const int _Index);

    AString& operator=(AString const& Rhs);
    AString& operator=(AString&& Rhs) noexcept;
    AString& operator=(AStringView Rhs);

    friend AString operator+(AStringView Lhs, AStringView Rhs);
    friend AString operator+(AStringView _Str, char _Char);
    friend AString operator+(char _Char, AStringView _Str);

    AString& operator+=(AStringView Rhs);
    AString& operator+=(char _Char);

    /** Clear string but not free a memory */
    void Clear();

    /** Clear string and free a memory */
    void Free();

    /** Set a new length for the string */
    void Resize(int _Length);

    /** Return is string empty or not */
    bool IsEmpty() const;

    /** Return string length */
    int Length() const;

    /** Get const char pointer of the string */
    const char* CStr() const;

    /** Pointer to the beggining of the string */
    const char* Begin() const;

    /** Pointer to the ending of the string */
    const char* End() const;

    /** Get raw pointer */
    char* ToPtr() const;

    /** Append the string */
    void Concat(AStringView Rhs);

    /** Append the character */
    void Concat(char _Char);

    /** Insert the string at specified index */
    void Insert(AStringView _Str, int _Index);

    /** Insert the character at specified index */
    void Insert(char _Char, int _Index);

    /** Replace the string at specified index */
    void Replace(AStringView _Str, int _Index);

    /** Replace all substrings with a new string */
    void Replace(AStringView _Substring, AStringView _NewStr);

    /** Cut substring at specified index */
    void Cut(int _Index, int _Count);

    AStringView TruncateHead(int _Count) const
    {
        if (_Count > Size)
        {
            _Count = Size;
        }
        return AStringView(Data + _Count, Size - _Count);
    }

    AStringView TruncateTail(int _Count) const
    {
        if (_Count > Size)
        {
            _Count = Size;
        }
        return AStringView(Data, Size - _Count);
    }

    /** Find the character. Return character position in string or -1. */
    HK_FORCEINLINE int Contains(char _Ch) const
    {
        return AStringView(Data, Size).Contains(_Ch);
    }

    /** Find the substring. Return substring position in string or -1. */
    HK_FORCEINLINE int FindSubstring(AStringView _Substring) const
    {
        return AStringView(Data, Size).FindSubstring(_Substring);
    }

    /** Find the substring. Return substring position in string or -1. */
    HK_FORCEINLINE int FindSubstringIcmp(AStringView _Substring) const
    {
        return AStringView(Data, Size).FindSubstringIcmp(_Substring);
    }

    /** Get substring. */
    HK_FORCEINLINE AStringView GetSubstring(size_t _Pos, size_t _Size) const
    {
        return AStringView(Data, Size).GetSubstring(_Pos, _Size);
    }

    /** Convert to lower case */
    void ToLower();

    /** Convert to upper case */
    void ToUpper();

    HK_FORCEINLINE uint32_t HexToUInt32() const
    {
        return AStringView(Data, Size).HexToUInt32();
    }

    HK_FORCEINLINE uint64_t HexToUInt64() const
    {
        return AStringView(Data, Size).HexToUInt64();
    }

    /** Compare the strings (case insensitive) */
    HK_FORCEINLINE int Icmp(AStringView Rhs) const
    {
        return AStringView(Data, Size).Icmp(Rhs);
    }

    /** Compare the strings (case sensitive) */
    HK_FORCEINLINE int Cmp(AStringView Rhs) const
    {
        return AStringView(Data, Size).Cmp(Rhs);
    }

    /** Compare the strings (case insensitive) */
    HK_FORCEINLINE int IcmpN(AStringView Rhs, int _Num) const
    {
        return AStringView(Data, Size).IcmpN(Rhs, _Num);
    }

    /** Compare the strings (case sensitive) */
    HK_FORCEINLINE int CmpN(AStringView Rhs, int _Num) const
    {
        return AStringView(Data, Size).CmpN(Rhs, _Num);
    }

    /** Skip trailing zeros for the numbers. */
    void ClipTrailingZeros();

    /** Path utility. Fix OS-specific separator. */
    void FixSeparator();

    /** Path utility.
    Fix path string insitu: replace separator \\ to /, skip series of /,
    skip redunant sequinces of dir/../dir2 -> dir. */
    void FixPath();

    /** Path utility. Cut the path. */
    void ClipPath();

    /** Path utility. Get filename without path. */
    HK_FORCEINLINE AStringView GetFilenameNoPath() const
    {
        return AStringView(Data, Size).GetFilenameNoPath();
    }

    /** Path utility. Return index of the path end. */
    HK_FORCEINLINE int FindPath() const
    {
        return AStringView(Data, Size).FindPath();
    }

    /** Path utility. Cut the extension. */
    void ClipExt();

    /** Path utility. Get full filename without extension. */
    HK_FORCEINLINE AStringView GetFilenameNoExt() const
    {
        return AStringView(Data, Size).GetFilenameNoExt();
    }

    /** Path utility. Cut the file name. */
    void ClipFilename();

    /** Path utility. Get path without file name. */
    HK_FORCEINLINE AStringView GetFilePath() const
    {
        return AStringView(Data, Size).GetFilePath();
    }

    /** Path utility. Check file extension. */
    HK_FORCEINLINE bool CompareExt(AStringView _Ext, bool _CaseInsensitive = true) const
    {
        return AStringView(Data, Size).CompareExt(_Ext, _CaseInsensitive);
    }

    /** Path utility. Set file extension if not exists. */
    void UpdateExt(AStringView _Extension);

    /** Path utility. Add or replace existing file extension. */
    void ReplaceExt(AStringView _Extension);

    /** Path utility. Return index where the extension begins. */
    HK_FORCEINLINE int FindExt() const
    {
        return AStringView(Data, Size).FindExt();
    }

    /** Path utility. Return index where the extension begins after dot. */
    HK_FORCEINLINE int FindExtWithoutDot() const
    {
        return AStringView(Data, Size).FindExt();
    }

    /** Path utility. Get filename extension. */
    HK_FORCEINLINE AStringView GetExt() const
    {
        return AStringView(Data, Size).GetExt();
    }

    /** Path utility. Get filename extension without dot. */
    HK_FORCEINLINE AStringView GetExtWithoutDot() const
    {
        return AStringView(Data, Size).GetExtWithoutDot();
    }

    /** Get string hash */
    HK_FORCEINLINE int Hash() const
    {
        return Core::Hash(Data, Size);
    }

    /** Get string hash case insensitive */
    HK_FORCEINLINE int HashCase() const
    {
        return Core::HashCase(Data, Size);
    }

    void FromFile(IBinaryStream& _Stream);

    HK_FORCEINLINE void Write(IBinaryStream& _Stream) const
    {
        AStringView(Data, Size).Write(_Stream);
    }

    void Read(IBinaryStream& _Stream);

    static const char* NullCString() { return ""; }

    static AString const& NullString() { return NullStr; }

private:
    void GrowCapacity(int _Capacity, bool _CopyOld);

protected:
    char  Base[BASE_CAPACITY];
    char* Data;
    int   Capacity;
    int   Size;

private:
    static const AString NullStr;
};

HK_FORCEINLINE AString::AString() :
    Data(Base), Capacity(BASE_CAPACITY), Size(0)
{
    Base[0] = 0;
}

HK_FORCEINLINE AString::AString(AString const& Rhs) :
    AString()
{
    const int newLen = Rhs.Length();
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, Rhs.ToPtr(), newLen);
    Data[newLen] = 0;
    Size         = newLen;
}

HK_FORCEINLINE AString::AString(AString&& Rhs) noexcept
{
    if (Rhs.Data == &Rhs.Base[0])
    {
        Platform::Memcpy(Base, Rhs.Base, Rhs.Size);
        Data       = Base;
        Capacity   = Rhs.Capacity;
        Size       = Rhs.Size;
        Base[Size] = 0;
    }
    else
    {
        Data     = Rhs.Data;
        Capacity = Rhs.Capacity;
        Size     = Rhs.Size;

        Rhs.Data      = Rhs.Base;
        Rhs.Capacity = BASE_CAPACITY;
    }
    Rhs.Size     = 0;
    Rhs.Data[0] = '\0';
}

HK_FORCEINLINE AString::AString(AStringView Rhs) :
    AString()
{
    const int newLen = Rhs.Length();
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, Rhs.ToPtr(), newLen);
    Data[newLen] = 0;
    Size         = newLen;
}

HK_FORCEINLINE AString::AString(const char* _Begin, const char* _End) :
    AString()
{
    const int newLen = _End - _Begin;
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, _Begin, newLen);
    Data[newLen] = 0;
    Size         = newLen;
}

HK_FORCEINLINE AString::~AString()
{
    if (Data != Base)
    {
        Allocator::Inst().Free(Data);
    }
}

HK_FORCEINLINE const char& AString::operator[](const int _Index) const
{
    HK_ASSERT_((_Index >= 0) && (_Index <= Size), "AString[]");
    return Data[_Index];
}

HK_FORCEINLINE char& AString::operator[](const int _Index)
{
    HK_ASSERT_((_Index >= 0) && (_Index <= Size), "AString[]");
    return Data[_Index];
}

HK_FORCEINLINE AString& AString::operator=(AString const& Rhs)
{
    const int newLen = Rhs.Length();
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, Rhs.ToPtr(), newLen);
    Data[newLen] = 0;
    Size         = newLen;
    return *this;
}

HK_FORCEINLINE AString& AString::operator=(AString&& Rhs) noexcept
{
    Free();

    if (Rhs.Data == &Rhs.Base[0])
    {
        Platform::Memcpy(Base, Rhs.Base, Rhs.Size);
        Data       = Base;
        Capacity   = Rhs.Capacity;
        Size       = Rhs.Size;
        Base[Size] = 0;
    }
    else
    {
        Data     = Rhs.Data;
        Capacity = Rhs.Capacity;
        Size     = Rhs.Size;

        Rhs.Data      = Rhs.Base;
        Rhs.Capacity = BASE_CAPACITY;
    }
    Rhs.Size     = 0;
    Rhs.Data[0] = '\0';

    return *this;
}

HK_FORCEINLINE AString& AString::operator=(AStringView Rhs)
{
    const int newLen = Rhs.Length();
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, Rhs.ToPtr(), newLen);
    Data[newLen] = 0;
    Size         = newLen;
    return *this;
}

HK_FORCEINLINE void AString::Clear()
{
    Size    = 0;
    Data[0] = '\0';
}

HK_FORCEINLINE void AString::Free()
{
    if (Data != Base)
    {
        Allocator::Inst().Free(Data);
        Data     = Base;
        Capacity = BASE_CAPACITY;
    }
    Size    = 0;
    Data[0] = '\0';
}

HK_FORCEINLINE AString operator+(AStringView Lhs, AStringView Rhs)
{
    AString result(Lhs);
    result.Concat(Rhs);
    return result;
}

HK_FORCEINLINE AString operator+(AStringView _Str, char _Char)
{
    AString result(_Str);
    result.Concat(_Char);
    return result;
}

HK_FORCEINLINE AString operator+(char _Char, AStringView _Str)
{
    AString result(_Str);
    result.Concat(_Char);
    return result;
}

HK_FORCEINLINE AString& AString::operator+=(AStringView _Str)
{
    Concat(_Str);
    return *this;
}

HK_FORCEINLINE AString& AString::operator+=(char _Char)
{
    Concat(_Char);
    return *this;
}

HK_FORCEINLINE const char* AString::CStr() const
{
    return Data;
}

HK_FORCEINLINE const char* AString::Begin() const
{
    return Data;
}
HK_FORCEINLINE const char* AString::End() const
{
    return Data + Size;
}

HK_FORCEINLINE int AString::Length() const
{
    return Size;
}

HK_FORCEINLINE bool AString::IsEmpty() const
{
    return Size == 0;
}

HK_FORCEINLINE char* AString::ToPtr() const
{
    return Data;
}

HK_FORCEINLINE void AString::Resize(int _Length)
{
    GrowCapacity(_Length + 1, true);
    if (_Length > Size)
    {
        Platform::Memset(&Data[Size], ' ', _Length - Size);
    }
    Size       = _Length;
    Data[Size] = 0;
}

HK_FORCEINLINE void AString::FixSeparator()
{
    Platform::FixSeparator(Data);
}

HK_FORCEINLINE void AString::ReplaceExt(AStringView _Extension)
{
    ClipExt();
    Concat(_Extension);
}

HK_FORCEINLINE void AString::FromFile(IBinaryStream& _Stream)
{
    _Stream.SeekEnd(0);
    long fileSz = _Stream.Tell();
    _Stream.SeekSet(0);
    GrowCapacity(fileSz + 1, false);
    _Stream.ReadBuffer(Data, fileSz);
    Data[fileSz] = 0;
    Size         = fileSz;
}

HK_FORCEINLINE void AString::Read(IBinaryStream& _Stream)
{
    int len = _Stream.ReadUInt32();
    GrowCapacity(len + 1, false);
    _Stream.ReadBuffer(Data, len);
    Data[len] = 0;
    Size      = len;
}


HK_FORCEINLINE AStringView::AStringView(AString const& Str) :
    AStringView(Str.ToPtr(), Str.Length())
{
}

HK_FORCEINLINE AStringView::AStringView(AStdString const& Str) :
    AStringView(Str.c_str(), Str.length())
{
}

HK_FORCEINLINE AStringView& AStringView::operator=(AString const& Rhs)
{
    Data = Rhs.ToPtr();
    Size = Rhs.Length();
    return *this;
}

HK_FORCEINLINE bool operator==(AStringView Lhs, AStringView Rhs)
{
    return Lhs.Cmp(Rhs) == 0;
}

HK_FORCEINLINE bool operator!=(AStringView Lhs, AStringView Rhs)
{
    return Lhs.Cmp(Rhs) != 0;
}

HK_FORCEINLINE AString AStringView::ToString() const
{
    return AString(Data, Data + Size);
}


/**

TSprintfBuffer

Usage:

TSprintfBuffer< 128 > buffer;
buffer.Sprintf( "%d %f", 10, 15.1f );

AString s = TSprintfBuffer< 128 >().Sprintf( "%d %f", 10, 15.1f );

*/
template <int Size>
struct TSprintfBuffer
{
    char Data[Size];

    char* Sprintf(const char* _Format, ...)
    {
        static_assert(Size > 0, "Invalid buffer size");
        HK_ASSERT(_Format);
        va_list VaList;
        va_start(VaList, _Format);
        Platform::VSprintf(Data, Size, _Format, VaList);
        va_end(VaList);
        return Data;
    }
};
