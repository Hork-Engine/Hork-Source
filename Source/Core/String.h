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

    HK_FORCEINLINE AStringView(const char* RawStr, int RawStrLen) :
        Data(RawStr), Size(RawStrLen)
    {
        HK_ASSERT(RawStrLen >= 0);
    }

    HK_FORCEINLINE AStringView(AString const& Str);
    HK_FORCEINLINE AStringView(AStdString const& Str);

    AStringView(AStringView const& Str) = default;
    AStringView& operator=(AStringView const& Rhs) = default;

    AStringView& operator=(AString const& Rhs);

    friend bool operator==(AStringView Lhs, AStringView Rhs);
    friend bool operator!=(AStringView Lhs, AStringView Rhs);

    HK_FORCEINLINE const char& operator[](const int Index) const
    {
        HK_ASSERT_((Index >= 0) && (Index <= Size), "AStringView[]");
        return Data[Index];
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
    int Contains(char Char) const
    {
        const char* end = Data + Size;
        for (const char* s = Data; s < end; s++)
        {
            if (*s == Char)
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
    int IcmpN(AStringView Rhs, int Num) const;

    /** Compare the strings (case sensitive) */
    int CmpN(AStringView Rhs, int Num) const;

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
    bool CompareExt(AStringView Ext, bool bCaseInsensitive = true) const
    {
        if (Ext.IsEmpty())
        {
            return false;
        }
        const char* p   = Data + Size;
        const char* ext = Ext.Data + Ext.Size;
        if (bCaseInsensitive)
        {
            char c1, c2;
            while (--ext >= Ext.Data)
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
            while (--ext >= Ext.Data)
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

    AStringView TruncateHead(int Count) const
    {
        if (Count > Size)
        {
            Count = Size;
        }
        return AStringView(Data + Count, Size - Count);
    }

    AStringView TruncateTail(int Count) const
    {
        if (Count > Size)
        {
            Count = Size;
        }
        return AStringView(Data, Size - Count);
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

    HK_FORCEINLINE void Write(IBinaryStream& Stream) const
    {
        Stream.WriteUInt32(Size);
        Stream.Write(Data, Size);
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
    AString(const char* RawStr);
    AString(const char* RawStrBegin, const char* RawStrEnd);
    ~AString();

    const char& operator[](const int Index) const;
    char&       operator[](const int Index);

    AString& operator=(AString const& Rhs);
    AString& operator=(AString&& Rhs) noexcept;
    AString& operator=(AStringView Rhs);
    AString& operator=(const char* RawStr);

    friend AString operator+(AStringView Lhs, AStringView Rhs);
    friend AString operator+(AStringView Lhs, char Char);
    friend AString operator+(char Char, AStringView Rhs);

    AString& operator+=(AStringView Rhs);
    AString& operator+=(char Char);

    /** Clear string but not free a memory */
    void Clear();

    /** Clear string and free a memory */
    void Free();

    /** Set a new length for the string */
    void Resize(int NewLength);

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
    void Concat(char Char);

    /** Insert the string at specified index */
    void Insert(AStringView Str, int Index);

    /** Insert the character at specified index */
    void Insert(char Char, int Index);

    /** Replace the string at specified index */
    void Replace(AStringView Str, int Index);

    /** Replace all substrings with a new string */
    void Replace(AStringView _Substring, AStringView _NewStr);

    /** Cut substring at specified index */
    void Cut(int Index, int Count);

    AStringView TruncateHead(int Count) const
    {
        if (Count > Size)
        {
            Count = Size;
        }
        return AStringView(Data + Count, Size - Count);
    }

    AStringView TruncateTail(int Count) const
    {
        if (Count > Size)
        {
            Count = Size;
        }
        return AStringView(Data, Size - Count);
    }

    /** Find the character. Return character position in string or -1. */
    HK_FORCEINLINE int Contains(char Char) const
    {
        return AStringView(Data, Size).Contains(Char);
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
    HK_FORCEINLINE int IcmpN(AStringView Rhs, int Num) const
    {
        return AStringView(Data, Size).IcmpN(Rhs, Num);
    }

    /** Compare the strings (case sensitive) */
    HK_FORCEINLINE int CmpN(AStringView Rhs, int Num) const
    {
        return AStringView(Data, Size).CmpN(Rhs, Num);
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
    HK_FORCEINLINE bool CompareExt(AStringView Ext, bool bCaseInsensitive = true) const
    {
        return AStringView(Data, Size).CompareExt(Ext, bCaseInsensitive);
    }

    /** Path utility. Set file extension if not exists. */
    void UpdateExt(AStringView Extension);

    /** Path utility. Add or replace existing file extension. */
    void ReplaceExt(AStringView Extension);

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

    void FromFile(IBinaryStream& Stream);

    HK_FORCEINLINE void Write(IBinaryStream& Stream) const
    {
        AStringView(Data, Size).Write(Stream);
    }

    void Read(IBinaryStream& Stream);

    static const char* NullCString() { return ""; }

    static AString const& NullString() { return NullStr; }

private:
    void GrowCapacity(int _Capacity, bool bCopyOld);

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

HK_FORCEINLINE AString::AString(const char* RawStr) :
    AString()
{
    const int newLen = Platform::Strlen(RawStr);
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, RawStr, newLen);
    Data[newLen] = 0;
    Size         = newLen;
}

HK_FORCEINLINE AString::AString(const char* RawStrBegin, const char* RawStrEnd) :
    AString()
{
    const int newLen = RawStrEnd - RawStrBegin;
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, RawStrBegin, newLen);
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

HK_FORCEINLINE const char& AString::operator[](const int Index) const
{
    HK_ASSERT_((Index >= 0) && (Index <= Size), "AString[]");
    return Data[Index];
}

HK_FORCEINLINE char& AString::operator[](const int Index)
{
    HK_ASSERT_((Index >= 0) && (Index <= Size), "AString[]");
    return Data[Index];
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

HK_FORCEINLINE AString& AString::operator=(const char* RawStr)
{
    const int newLen = Platform::Strlen(RawStr);
    GrowCapacity(newLen + 1, false);
    Platform::Memcpy(Data, RawStr, newLen);
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

HK_FORCEINLINE AString operator+(AStringView Lhs, char Char)
{
    AString result(Lhs);
    result.Concat(Char);
    return result;
}

HK_FORCEINLINE AString operator+(char Char, AStringView Rhs)
{
    AString result(Rhs);
    result.Concat(Char);
    return result;
}

HK_FORCEINLINE AString& AString::operator+=(AStringView Rhs)
{
    Concat(Rhs);
    return *this;
}

HK_FORCEINLINE AString& AString::operator+=(char Char)
{
    Concat(Char);
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

HK_FORCEINLINE void AString::Resize(int NewLength)
{
    GrowCapacity(NewLength + 1, true);
    if (NewLength > Size)
    {
        Platform::Memset(&Data[Size], ' ', NewLength - Size);
    }
    Size       = NewLength;
    Data[Size] = 0;
}

HK_FORCEINLINE void AString::FixSeparator()
{
    Platform::FixSeparator(Data);
}

HK_FORCEINLINE void AString::ReplaceExt(AStringView Extension)
{
    ClipExt();
    Concat(Extension);
}

HK_FORCEINLINE void AString::FromFile(IBinaryStream& Stream)
{
    Stream.SeekEnd(0);
    size_t fileSz = Stream.GetOffset();
    Stream.SeekSet(0);
    GrowCapacity(fileSz + 1, false);
    Stream.Read(Data, fileSz);
    Data[fileSz] = 0;
    Size         = fileSz;
}

HK_FORCEINLINE void AString::Read(IBinaryStream& Stream)
{
    int len = Stream.ReadUInt32();
    GrowCapacity(len + 1, false);
    Stream.Read(Data, len);
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

    char* Sprintf(const char* Format, ...)
    {
        static_assert(Size > 0, "Invalid buffer size");
        HK_ASSERT(Format);
        va_list VaList;
        va_start(VaList, Format);
        Platform::VSprintf(Data, Size, Format, VaList);
        va_end(VaList);
        return Data;
    }
};
