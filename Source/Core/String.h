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

#include <Platform/Memory/Memory.h>
#include <Platform/Path.h>
#include <Platform/Logger.h>
#include <Platform/Utf8.h>
#include <Platform/String.h>

#include "BaseMath.h"

HK_NAMESPACE_BEGIN

using StringSizeType = uint32_t;

static constexpr StringSizeType NullTerminatedBit = StringSizeType(1) << (sizeof(StringSizeType) * 8 - 1);
static constexpr StringSizeType StringSizeMask    = ~NullTerminatedBit;
static constexpr StringSizeType MaxStringSize     = StringSizeMask - 1;

template <typename CharT, typename Allocator>
class TString;

template <typename CharT>
struct TGlobalStringView;

template <typename CharT>
class TStringView;

/** Converts the given character to lowercase */
template <typename CharT>
CharT ToLower(CharT Ch)
{
    return (Ch >= 'A' && Ch <= 'Z') ? Ch - 'A' + 'a' : Ch;
}

/** Converts the given character to uppercase */
template <typename CharT>
CharT ToUpper(CharT Ch)
{
    return (Ch >= 'a' && Ch <= 'z') ? Ch - 'a' + 'A' : Ch;
}

template <typename CharT>
uint32_t StringHash(const CharT* pRawStringBegin, const CharT* pRawStringEnd)
{
    // SDBM hash
    uint32_t     hash = 0;
    const CharT* p    = pRawStringBegin;
    const CharT* end  = pRawStringEnd;
    while (p < end)
        hash = (*p++) + (hash << 6) + (hash << 16) - hash;
    return hash;
}

template <typename CharT>
uint32_t StringHashCaseInsensitive(const CharT* pRawStringBegin, const CharT* pRawStringEnd)
{
    // SDBM hash
    uint32_t     hash = 0;
    const CharT* p    = pRawStringBegin;
    const CharT* end  = pRawStringEnd;
    while (p < end)
        hash = ToLower(*p++) + (hash << 6) + (hash << 16) - hash;
    return hash;
}

template <typename CharT>
StringSizeType StringLength(const CharT* pRawString)
{
    const CharT* p = pRawString;
    while (*p)
        ++p;
    size_t size = (size_t)(p - pRawString);
    HK_ASSERT(size <= MaxStringSize);
    return static_cast<StringSizeType>(size);
}

template <typename CharT, typename Allocator>
StringSizeType StringLength(TString<CharT, Allocator> const& Str)
{
    return Str.Size();
}

template <typename CharT>
StringSizeType StringLength(const CharT* pRawStringBegin, const CharT* pRawStringEnd)
{
    HK_ASSERT(pRawStringBegin <= pRawStringEnd && (size_t)(pRawStringEnd - pRawStringBegin) <= MaxStringSize);
    return (StringSizeType)(pRawStringEnd - pRawStringBegin);
}

template <typename CharT>
StringSizeType StringLength(TGlobalStringView<CharT> const& Str)
{
    return StringLength(Str.CStr());
}

template <typename CharT>
StringSizeType StringLength(TStringView<CharT> const& Str)
{
    return Str.Size();
}

template <typename CharT>
struct TGlobalStringView
{
    const CharT* CStr() const
    {
        return pRawString;
    }

    // NOTE: Internal. Do not modify.
    const CharT* pRawString;
};

using GlobalStringView = TGlobalStringView<char>;
using AGlobalStringViewW = TGlobalStringView<WideChar>;

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wliteral-suffix"
#endif

HK_FORCEINLINE GlobalStringView operator"" s(const char* s, size_t sz)
{
    GlobalStringView v;
    v.pRawString = s;
    return v;
}

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

template <typename CharT>
CharT const* NullStr();

template <>
HK_INLINE char const* NullStr<char>()
{
    return "";
}

template <>
HK_INLINE WideChar const* NullStr<WideChar>()
{
    static constexpr WideChar s[1] = {0};
    return s;
}

template <typename CharT>
class TStringView
{
public:
    static_assert(sizeof(CharT) == 1 || sizeof(CharT) == 2, "Only 8 and 16 bit characters supported");

    using SizeType = StringSizeType;

    static constexpr bool IsWideString()
    {
        return sizeof(CharT) == 2;
    }

    HK_FORCEINLINE TStringView() :
        m_pData(""), m_Size(0 | NullTerminatedBit)
    {}

    HK_FORCEINLINE TStringView(std::nullptr_t) :
        m_pData(""), m_Size(0 | NullTerminatedBit)
    {}

    HK_FORCEINLINE TStringView(const CharT* pRawString) :
        m_pData(pRawString ? pRawString : NullStr<CharT>()), m_Size((pRawString ? StringLength(pRawString) : 0) | NullTerminatedBit)
    {}

    HK_FORCEINLINE TStringView(const CharT* pRawStringBegin, const CharT* pRawStringEnd, bool bNullTerminated = false) :
        m_pData(pRawStringBegin), m_Size(StringLength(pRawStringBegin, pRawStringEnd))
    {
        HK_ASSERT(pRawStringBegin != nullptr);
        HK_ASSERT(pRawStringEnd != nullptr);
        if (bNullTerminated)
            m_Size |= NullTerminatedBit;
    }

    HK_FORCEINLINE TStringView(const CharT* pRawString, SizeType Size, bool bNullTerminated = false) :
        m_pData(pRawString), m_Size(Size)
    {
        HK_ASSERT(Size <= MaxStringSize);
        if (bNullTerminated)
            m_Size |= NullTerminatedBit;
    }

    template <typename Allocator>
    HK_FORCEINLINE TStringView(TString<CharT, Allocator> const& Str) :
        m_pData(Str.ToPtr()), m_Size(Str.Size() | NullTerminatedBit)
    {}

    HK_FORCEINLINE TStringView(TGlobalStringView<CharT> Str) :
        m_pData(Str.CStr()), m_Size(StringLength(Str.CStr()) | NullTerminatedBit)
    {}

    TStringView(TStringView const& Str) = default;

    TStringView& operator=(TStringView const& Rhs) = default;

    template <typename Allocator>
    TStringView& operator=(TString<CharT, Allocator> const& Rhs);

    HK_FORCEINLINE CharT const& operator[](SizeType Index) const
    {
        HK_ASSERT_(Index < Size(), "Undefined behavior accessing out of bounds");
        return m_pData[Index];
    }

    HK_FORCEINLINE operator bool() const
    {
        return !IsEmpty();
    }

    /** Is string empty. */
    HK_FORCEINLINE bool IsEmpty() const
    {
        return Size() == 0;
    }

    /** The number of characters in the string. */
    HK_FORCEINLINE SizeType Length() const
    {
        return m_Size & StringSizeMask;
    }

    /** The number of characters in the string. */
    HK_FORCEINLINE SizeType Size() const
    {
        return m_Size & StringSizeMask;
    }

    HK_FORCEINLINE bool IsNullTerminated() const
    {
        return !!(m_Size & NullTerminatedBit);
    }

    /** Gives a raw pointer to the beginning of the substring. */
    HK_FORCEINLINE const CharT* Begin() const
    {
        return m_pData;
    }

    /** Gives a raw pointer to the end of the substring. */
    HK_FORCEINLINE const CharT* End() const
    {
        return m_pData + Size();
    }

    /** Gives a raw pointer to the beginning of the substring. */
    HK_FORCEINLINE const CharT* ToPtr() const
    {
        return m_pData;
    }

    /** Finds a character. Returns the position of a character in a string, or -1. */
    HK_NODISCARD SizeType Contains(CharT Char) const
    {
        for (const CharT *s = m_pData, *end = m_pData + Size(); s < end; s++)
            if (*s == Char)
                return (SizeType)(s - m_pData);
        return (SizeType)-1;
    }

    /** Finds a substring. Returns the position of the substring in the string, or -1. */
    HK_NODISCARD SizeType FindSubstring(TStringView Substr) const
    {
        size_t size = Substr.Size();
        if (!size)
            return (SizeType)-1;

        const CharT* s   = m_pData;
        const CharT* end = m_pData + Size();
        while (s < end && (SizeType)(end - s) >= size)
        {
            if (TStringView(s, end).CmpN(Substr, size) == 0)
                return (SizeType)(s - m_pData);
            ++s;
        }
        return (SizeType)-1;
    }

    /** Finds a substring. Returns the position of the substring in the string, or -1. */
    HK_NODISCARD SizeType FindSubstringIcmp(TStringView Substr) const
    {
        size_t size = Substr.Size();
        if (!size)
            return (SizeType)-1;

        const CharT* s   = m_pData;
        const CharT* end = m_pData + Size();
        while (s < end && (SizeType)(end - s) >= size)
        {
            if (TStringView(s, end).IcmpN(Substr, size) == 0)
                return (SizeType)(s - m_pData);
            ++s;
        }
        return (SizeType)-1;
    }

    HK_NODISCARD SizeType FindCharacter(CharT Char) const
    {
        const CharT* s   = m_pData;
        const CharT* end = m_pData + Size();
        while (s < end)
        {
            if (*s == Char)
                return (SizeType)(s - m_pData);
            ++s;
        }
        return (SizeType)-1;
    }

    HK_NODISCARD HK_FORCEINLINE TStringView GetSubstring(SizeType _Pos, SizeType _Size) const
    {
        HK_ASSERT_(_Pos + _Size <= Size(), "Undefined behavior accessing out of bounds");
        return TStringView(m_pData + _Pos, _Size, IsNullTerminated() && (_Pos + _Size == Size()));
    }

    HK_NODISCARD HK_FORCEINLINE TStringView GetSubstring(SizeType _Pos) const
    {
        HK_ASSERT_(_Pos <= Size(), "Undefined behavior accessing out of bounds");
        return TStringView(m_pData + _Pos, Size() - _Pos, IsNullTerminated() && _Pos == 0);
    }

    /** Compares strings (case insensitive). */
    HK_NODISCARD int Icmp(TStringView Rhs) const;

    /** Compares strings (case sensitive). */
    HK_NODISCARD int Cmp(TStringView Rhs) const;

    /** Compares strings (case insensitive). */
    HK_NODISCARD int IcmpN(TStringView Rhs, SizeType Num) const;

    /** Compares strings (case sensitive). */
    HK_NODISCARD int CmpN(TStringView Rhs, SizeType Num) const;

    HK_NODISCARD bool Icompare(TStringView Rhs) const
    {
        SizeType size = Size();

        if (size != Rhs.Size())
            return false;

        for (SizeType n = 0; n < size; n++)
        {
            if (ToLower(m_pData[n]) != ToLower(Rhs.m_pData[n]))
                return false;
        }
        return true;
    }

    HK_NODISCARD bool Compare(TStringView Rhs) const
    {
        SizeType size = Size();

        if (size != Rhs.Size())
            return false;

        for (SizeType n = 0; n < size; n++)
        {
            if (m_pData[n] != Rhs.m_pData[n])
                return false;
        }
        return true;
    }

    HK_NODISCARD TStringView TruncateHead(SizeType Count) const
    {
        SizeType size = Size();
        if (Count > size)
            Count = size;
        return TStringView(m_pData + Count, size - Count, IsNullTerminated());
    }

    HK_NODISCARD TStringView TruncateTail(SizeType Count) const
    {
        SizeType size = Size();
        if (Count > size)
            Count = size;
        return TStringView(m_pData, Count, IsNullTerminated() && Count == size);
    }

    /** Gives a string hash. */
    HK_NODISCARD HK_FORCEINLINE uint32_t Hash() const
    {
        return StringHash(Begin(), End());
    }

    /** Gives a string hash (case insensitive). */
    HK_NODISCARD HK_FORCEINLINE uint32_t HashCaseInsensitive() const
    {
        return StringHashCaseInsensitive(Begin(), End());
    }

private:
    const CharT* m_pData;
    SizeType     m_Size;
};

enum STRING_RESIZE_FLAGS : uint32_t
{
    STRING_RESIZE_DEFAULT        = 0,
    STRING_RESIZE_NO_FILL_SPACES = HK_BIT(0)
};

HK_FLAG_ENUM_OPERATORS(STRING_RESIZE_FLAGS)

template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
class TString final
{
public:
    static_assert(sizeof(CharT) == 1 || sizeof(CharT) == 2, "Only 8 and 16 bit characters supported");

    using SizeType = typename TStringView<CharT>::SizeType;

    static constexpr bool IsWideString()
    {
        return sizeof(CharT) == 2;
    }

    static const SizeType Granularity  = 32;
    static const SizeType BaseCapacity = 32;

    TString();
    TString(TString const& Rhs);
    TString(TString&& Rhs) noexcept;
    TString(TStringView<CharT> Rhs);
    TString(const CharT* pRawString);
    TString(const CharT* pRawStringBegin, const CharT* pRawStringEnd);
    ~TString();

    CharT const& operator[](SizeType Index) const;
    CharT&       operator[](SizeType Index);

    TString& operator=(TString const& Rhs);
    TString& operator=(TString&& Rhs) noexcept;
    TString& operator=(TStringView<CharT> Rhs);
    TString& operator=(const CharT* pRawString);

    TString& operator+=(TStringView<CharT> Rhs);
    TString& operator+=(CharT Char);

    TString& operator/=(TStringView<CharT> Rhs);
    TString& operator/=(TString const& Rhs);
    TString& operator/=(const CharT* Rhs);

    /** Clears the string, but does not free the memory. */
    void Clear();

    /** Clears the string and frees memory. */
    void Free();

    /** Sets the new length of the string.
        If the new number of characters is less than the current one, the string will be truncated.
        If the new character count is greater than the current one, it will fill the newly added characters with spaces. */
    void Resize(SizeType Size, STRING_RESIZE_FLAGS Flags = STRING_RESIZE_DEFAULT);

    /** Reserve memory for the specified number of characters. */
    void Reserve(SizeType NumCharacters);

    /** Is string empty. */
    HK_FORCEINLINE bool IsEmpty() const
    {
        return m_Size == 0;
    }

    /** The number of characters in the string. */
    HK_FORCEINLINE SizeType Length() const
    {
        return m_Size;
    }

    /** The number of characters in the string. */
    HK_FORCEINLINE SizeType Size() const
    {
        return m_Size;
    }

    /** Gives a raw pointer to the beginning of the string. */
    const CharT* CStr() const;

    /** Gives a raw pointer to the beginning of the string. */
    const CharT* CBegin() const;

    /** Gives a raw pointer to the beginning of the string. */
    CharT* Begin() const;

    /** Gives a raw pointer to the end of the string. */
    const CharT* CEnd() const;

    /** Gives a raw pointer to the end of the string. */
    CharT* End() const;

    /** Gives a raw pointer to the beginning of the string. */
    CharT* ToPtr() const;

    /** Adds a string to the end of a string. */
    void Concat(TStringView<CharT> Rhs);

    /** Adds a character to the end of a string. */
    void Concat(CharT Char);

    /** Inserts a string at the specified index. */
    void InsertAt(SizeType Index, TStringView<CharT> Str);

    /** Inserts a character at the specified index. */
    void InsertAt(SizeType Index, CharT Char);

    /** Replaces the string at the specified index. */
    void ReplaceAt(SizeType Index, TStringView<CharT> Str);

    /** Replaces all substrings with a new string. */
    void Replace(TStringView<CharT> Substr, TStringView<CharT> NewStr);

    /** Cuts the substring at the specified index. */
    void Cut(SizeType Index, SizeType Count);

    HK_NODISCARD TStringView<CharT> TruncateHead(SizeType Count) const
    {
        if (Count > m_Size)
            Count = m_Size;
        return TStringView<CharT>(m_pData + Count, m_Size - Count, true);
    }

    HK_NODISCARD TStringView<CharT> TruncateTail(SizeType Count) const
    {
        if (Count > m_Size)
            Count = m_Size;
        return TStringView<CharT>(m_pData, Count, Count == m_Size);
    }

    /** Finds a character. Returns the position of a character in a string, or -1. */
    HK_NODISCARD HK_FORCEINLINE SizeType Contains(CharT Char) const
    {
        return TStringView<CharT>(m_pData, m_Size).Contains(Char);
    }

    /** Finds a substring. Returns the position of the substring in the string, or -1. */
    HK_NODISCARD HK_FORCEINLINE SizeType FindSubstring(TStringView<CharT> Substr) const
    {
        return TStringView<CharT>(m_pData, m_Size).FindSubstring(Substr);
    }

    /** Finds a substring. Returns the position of the substring in the string, or -1. */
    HK_NODISCARD HK_FORCEINLINE SizeType FindSubstringIcmp(TStringView<CharT> Substr) const
    {
        return TStringView<CharT>(m_pData, m_Size).FindSubstringIcmp(Substr);
    }

    HK_NODISCARD SizeType FindCharacter(CharT Char) const
    {
        return TStringView<CharT>(m_pData, m_Size).FindCharacter(Char);
    }

    /** Get substring. */
    HK_NODISCARD HK_FORCEINLINE TStringView<CharT> GetSubstring(SizeType _Pos, SizeType _Size) const
    {
        return TStringView<CharT>(m_pData, m_Size, true).GetSubstring(_Pos, _Size);
    }

    /** Convert the string to lowercase. */
    void ToLower()
    {
        CharT* p = m_pData;
        while (*p)
            *p = Hk::ToLower(*p), p++;
    }

    /** Convert the string to uppercase. */
    void ToUpper()
    {
        CharT* p = m_pData;
        while (*p)
            *p = Hk::ToUpper(*p), p++;
    }

    /** Compares strings (case insensitive). */
    HK_NODISCARD HK_FORCEINLINE int Icmp(TStringView<CharT> Rhs) const
    {
        return TStringView<CharT>(m_pData, m_Size).Icmp(Rhs);
    }

    /** Compares strings (case sensitive). */
    HK_NODISCARD HK_FORCEINLINE int Cmp(TStringView<CharT> Rhs) const
    {
        return TStringView<CharT>(m_pData, m_Size).Cmp(Rhs);
    }

    /** Compares strings (case insensitive). */
    HK_NODISCARD HK_FORCEINLINE int IcmpN(TStringView<CharT> Rhs, SizeType Num) const
    {
        return TStringView<CharT>(m_pData, m_Size).IcmpN(Rhs, Num);
    }

    /** Compares strings (case sensitive). */
    HK_NODISCARD HK_FORCEINLINE int CmpN(TStringView<CharT> Rhs, SizeType Num) const
    {
        return TStringView<CharT>(m_pData, m_Size).CmpN(Rhs, Num);
    }

    HK_NODISCARD HK_FORCEINLINE bool Icompare(TStringView<CharT> Rhs) const
    {
        return TStringView<CharT>(m_pData, m_Size).Icompare(Rhs);
    }

    HK_NODISCARD HK_FORCEINLINE bool Compare(TStringView<CharT> Rhs) const
    {
        return TStringView<CharT>(m_pData, m_Size).Compare(Rhs);
    }

    /** Gives a string hash. */
    HK_NODISCARD HK_FORCEINLINE uint32_t Hash() const
    {
        return StringHash(Begin(), End());
    }

    /** Gives a string hash (case insensitive). */
    HK_NODISCARD HK_FORCEINLINE uint32_t HashCaseInsensitive() const
    {
        return StringHashCaseInsensitive(Begin(), End());
    }

private:
    void Construct(const CharT* pRawString, SizeType Size);
    void GrowCapacity(SizeType _Capacity, bool bCopyOld);

protected:
    CharT    m_Base[BaseCapacity];
    CharT*   m_pData    = m_Base;
    SizeType m_Capacity = BaseCapacity;
    SizeType m_Size     = 0;
};

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::Construct(const CharT* pRawString, SizeType Size)
{
    HK_ASSERT(Size <= MaxStringSize);

    GrowCapacity(Size + 1, false);

    Platform::Memcpy(m_pData, pRawString, Size * sizeof(CharT));

    m_pData[Size] = 0;
    m_Size        = Size;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>::TString()
{
    m_Base[0] = 0;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>::TString(TString<CharT, Allocator> const& Rhs)
{
    Construct(Rhs.ToPtr(), Rhs.Size());
}

template <typename CharT, typename Allocator>
HK_INLINE TString<CharT, Allocator>::TString(TString<CharT, Allocator>&& Rhs) noexcept
{
    if (Rhs.m_pData == &Rhs.m_Base[0])
    {
        Platform::Memcpy(m_Base, Rhs.m_Base, Rhs.m_Size * sizeof(CharT));
        m_pData        = m_Base;
        m_Capacity     = Rhs.m_Capacity;
        m_Size         = Rhs.m_Size;
        m_Base[m_Size] = 0;
    }
    else
    {
        m_pData    = Rhs.m_pData;
        m_Capacity = Rhs.m_Capacity;
        m_Size     = Rhs.m_Size;

        Rhs.m_pData    = Rhs.m_Base;
        Rhs.m_Capacity = BaseCapacity;
    }
    Rhs.m_Size     = 0;
    Rhs.m_pData[0] = 0;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>::TString(TStringView<CharT> Rhs)
{
    Construct(Rhs.ToPtr(), Rhs.Size());
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>::TString(const CharT* pRawString)
{
    Construct(pRawString, StringLength(pRawString));
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>::TString(const CharT* pRawStringBegin, const CharT* pRawStringEnd)
{
    Construct(pRawStringBegin, StringLength(pRawStringBegin, pRawStringEnd));
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>::~TString()
{
    if (m_pData != m_Base)
    {
        Allocator().deallocate(m_pData);
    }
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE CharT const& TString<CharT, Allocator>::operator[](SizeType Index) const
{
    HK_ASSERT_(Index < m_Size, "Undefined behavior accessing out of bounds");
    return m_pData[Index];
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE CharT& TString<CharT, Allocator>::operator[](SizeType Index)
{
    HK_ASSERT_(Index < m_Size, "Undefined behavior accessing out of bounds");
    return m_pData[Index];
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>& TString<CharT, Allocator>::operator=(TString<CharT, Allocator> const& Rhs)
{
    Construct(Rhs.ToPtr(), Rhs.Size());
    return *this;
}

template <typename CharT, typename Allocator>
HK_INLINE TString<CharT, Allocator>& TString<CharT, Allocator>::operator=(TString<CharT, Allocator>&& Rhs) noexcept
{
    Free();

    if (Rhs.m_pData == &Rhs.m_Base[0])
    {
        Platform::Memcpy(m_Base, Rhs.m_Base, Rhs.m_Size * sizeof(CharT));
        m_pData        = m_Base;
        m_Capacity     = Rhs.m_Capacity;
        m_Size         = Rhs.m_Size;
        m_Base[m_Size] = 0;
    }
    else
    {
        m_pData    = Rhs.m_pData;
        m_Capacity = Rhs.m_Capacity;
        m_Size     = Rhs.m_Size;

        Rhs.m_pData    = Rhs.m_Base;
        Rhs.m_Capacity = BaseCapacity;
    }
    Rhs.m_Size     = 0;
    Rhs.m_pData[0] = 0;

    return *this;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>& TString<CharT, Allocator>::operator=(TStringView<CharT> Rhs)
{
    Construct(Rhs.ToPtr(), Rhs.Size());
    return *this;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>& TString<CharT, Allocator>::operator=(const CharT* pRawString)
{
    Construct(pRawString, StringLength(pRawString));
    return *this;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE void TString<CharT, Allocator>::Clear()
{
    m_Size     = 0;
    m_pData[0] = 0;
}

template <typename CharT, typename Allocator>
HK_INLINE void TString<CharT, Allocator>::Free()
{
    if (m_pData != m_Base)
    {
        Allocator().deallocate(m_pData);
        m_pData    = m_Base;
        m_Capacity = BaseCapacity;
    }
    m_Size     = 0;
    m_pData[0] = 0;
}

template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
HK_FORCEINLINE TString<CharT, Allocator> ConcatStrings(TStringView<CharT> Lhs, TStringView<CharT> Rhs)
{
    TString<CharT, Allocator> result;
    result.Reserve(Lhs.Size() + Rhs.Size());
    result.Concat(Lhs);
    result.Concat(Rhs);
    return result;
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TStringView<CharT> Lhs, TStringView<CharT> const& Rhs)
{
    return ConcatStrings<CharT, Allocator>(Lhs, Rhs);
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TStringView<CharT> Lhs, TString<CharT, Allocator> const& Rhs)
{
    return ConcatStrings<CharT, Allocator>(Lhs, TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TStringView<CharT> Lhs, const CharT* Rhs)
{
    return ConcatStrings<CharT, Allocator>(Lhs, TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TStringView<CharT> Lhs, CharT Rhs)
{
    return ConcatStrings<CharT, Allocator>(Lhs, TStringView<CharT>(&Rhs, 1));
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TString<CharT, Allocator> const& Lhs, TStringView<CharT> Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(Lhs), Rhs);
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TString<CharT, Allocator> const& Lhs, TString<CharT, Allocator> const& Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(Lhs), TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TString<CharT, Allocator> const& Lhs, const CharT* Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(Lhs), TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator> operator+(TString<CharT, Allocator> const& Lhs, CharT Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(Lhs), TStringView<CharT>(&Rhs, 1));
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
HK_FORCEINLINE TString<CharT, Allocator> operator+(const CharT* Lhs, TStringView<CharT> Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(Lhs), Rhs);
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator> operator+(const CharT* Lhs, TString<CharT, Allocator> const& Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(Lhs), TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
HK_FORCEINLINE TString<CharT, Allocator> operator+(CharT Lhs, TStringView<CharT> Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(&Lhs, 1), Rhs);
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator> operator+(CharT Lhs, TString<CharT, Allocator> const& Rhs)
{
    return ConcatStrings<CharT, Allocator>(TStringView<CharT>(&Lhs, 1), TStringView<CharT>(Rhs));
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>& TString<CharT, Allocator>::operator+=(TStringView<CharT> Rhs)
{
    Concat(Rhs);
    return *this;
}
template <typename CharT, typename Allocator>
HK_FORCEINLINE TString<CharT, Allocator>& TString<CharT, Allocator>::operator+=(CharT Char)
{
    Concat(Char);
    return *this;
}

template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
TString<CharT, Allocator> ConcatPaths(TStringView<CharT> Lhs, TStringView<CharT> Rhs)
{
    auto size    = Lhs.Size();
    auto rhsSize = Rhs.Size();
    auto newSize = size + rhsSize + 1;

    TString<CharT, Allocator> s;
    s.Reserve(newSize);

    s += Lhs;
    if (size == 0 || Lhs.ToPtr()[size - 1] != '/')
        s += '/';

    const CharT* p;
    const CharT* end;
    for (p = Rhs.Begin(), end = Rhs.End(); p < end && *p == '/'; p++)
    {}

    if (Rhs.IsNullTerminated())
        s += p;
    else
        s += TStringView<CharT>(p, end);

    return s;
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
TString<CharT, Allocator> operator/(TStringView<CharT> Lhs, TStringView<CharT> const& Rhs)
{
    return ConcatPaths<CharT, Allocator>(Lhs, TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator>
TString<CharT, Allocator> operator/(TStringView<CharT> Lhs, TString<CharT, Allocator> const& Rhs)
{
    return ConcatPaths<CharT, Allocator>(Lhs, TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
TString<CharT, Allocator> operator/(TStringView<CharT> Lhs, const CharT* Rhs)
{
    return ConcatPaths<CharT, Allocator>(Lhs, TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator>
TString<CharT, Allocator> operator/(TString<CharT, Allocator> const& Lhs, TString<CharT, Allocator> const& Rhs)
{
    return ConcatPaths<CharT, Allocator>(TStringView<CharT>(Lhs), TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator>
TString<CharT, Allocator> operator/(TString<CharT, Allocator> const& Lhs, TStringView<CharT> Rhs)
{
    return ConcatPaths<CharT, Allocator>(TStringView<CharT>(Lhs), Rhs);
}
template <typename CharT, typename Allocator>
TString<CharT, Allocator> operator/(TString<CharT, Allocator> const& Lhs, const CharT* Rhs)
{
    return ConcatPaths<CharT, Allocator>(TStringView<CharT>(Lhs), TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator>
TString<CharT, Allocator> operator/(const CharT* Lhs, TString<CharT, Allocator> const& Rhs)
{
    return ConcatPaths<CharT, Allocator>(TStringView<CharT>(Lhs), TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
TString<CharT, Allocator> operator/(const CharT* Lhs, TStringView<CharT> Rhs)
{
    return ConcatPaths<CharT, Allocator>(TStringView<CharT>(Lhs), Rhs);
}

template <typename CharT, typename Allocator>
TString<CharT, Allocator>& TString<CharT, Allocator>::operator/=(TStringView<CharT> Rhs)
{
    auto size    = Size();
    auto rhsSize = Rhs.Size();
    auto newSize = size + rhsSize + 1;

    Reserve(newSize);

    if (size == 0 || m_pData[size - 1] != '/')
        Concat('/');

    const CharT* p;
    const CharT* end;
    for (p = Rhs.Begin(), end = Rhs.End(); p < end && *p == '/'; p++)
    {}

    if (Rhs.IsNullTerminated())
        Concat(p);
    else
        Concat(TStringView<CharT>(p, end));

    return *this;
}
template <typename CharT, typename Allocator>
TString<CharT, Allocator>& TString<CharT, Allocator>::operator/=(TString<CharT, Allocator> const& Rhs)
{
    return operator/=(TStringView<CharT>(Rhs));
}
template <typename CharT, typename Allocator>
TString<CharT, Allocator>& TString<CharT, Allocator>::operator/=(const CharT* Rhs)
{
    return operator/=(TStringView<CharT>(Rhs));
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE const CharT* TString<CharT, Allocator>::CStr() const
{
    return m_pData;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE const CharT* TString<CharT, Allocator>::CBegin() const
{
    return m_pData;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE CharT* TString<CharT, Allocator>::Begin() const
{
    return m_pData;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE const CharT* TString<CharT, Allocator>::CEnd() const
{
    return m_pData + m_Size;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE CharT* TString<CharT, Allocator>::End() const
{
    return m_pData + m_Size;
}

template <typename CharT, typename Allocator>
HK_FORCEINLINE CharT* TString<CharT, Allocator>::ToPtr() const
{
    return m_pData;
}

template <typename CharT, typename Allocator>
HK_INLINE void TString<CharT, Allocator>::Resize(SizeType Size, STRING_RESIZE_FLAGS Flags)
{
    if (Size > m_Size)
    {
        Reserve(Size);

        if (!(Flags & STRING_RESIZE_NO_FILL_SPACES))
        {
            // Fill rest of the line by spaces
            if (IsWideString())
            {
                CharT* p = &m_pData[m_Size];
                CharT* e = &m_pData[Size];
                while (p < e)
                    *p++ = ' ';
            }
            else
            {
                Platform::Memset(&m_pData[m_Size], ' ', Size - m_Size);
            }
        }
    }
    m_Size          = Size;
    m_pData[m_Size] = 0;
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::Reserve(SizeType NumCharacters)
{
    HK_ASSERT(NumCharacters <= MaxStringSize);
    GrowCapacity(NumCharacters + 1, true);
}

template <typename CharT>
template <typename Allocator>
HK_FORCEINLINE TStringView<CharT>& TStringView<CharT>::operator=(TString<CharT, Allocator> const& Rhs)
{
    m_pData = Rhs.ToPtr();
    m_Size  = Rhs.Size();
    m_Size |= NullTerminatedBit;
    return *this;
}

template <typename CharT>
HK_FORCEINLINE bool operator==(TStringView<CharT> Lhs, TStringView<CharT> Rhs) { return Lhs.Cmp(Rhs) == 0; }
template <typename CharT>
HK_FORCEINLINE bool operator==(TStringView<CharT> Lhs, const CharT* Rhs) { return Lhs.Cmp(Rhs) == 0; }
template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator==(TStringView<CharT> Lhs, TString<CharT, Allocator> const& Rhs) { return Lhs.Cmp(Rhs) == 0; }

template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator==(TString<CharT, Allocator> const& Lhs, TString<CharT, Allocator> const& Rhs) { return Lhs.Cmp(Rhs) == 0; }
template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator==(TString<CharT, Allocator> const& Lhs, const CharT* Rhs) { return Lhs.Cmp(Rhs) == 0; }
template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator==(TString<CharT, Allocator> const& Lhs, TStringView<CharT> Rhs) { return Lhs.Cmp(Rhs) == 0; }

template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator==(const CharT* Lhs, TString<CharT, Allocator> const& Rhs) { return Rhs.Cmp(Lhs) == 0; }
template <typename CharT>
HK_FORCEINLINE bool operator==(const CharT* Lhs, TStringView<CharT> Rhs) { return Rhs.Cmp(Lhs) == 0; }

template <typename CharT>
HK_FORCEINLINE bool operator!=(TStringView<CharT> Lhs, TStringView<CharT> Rhs) { return Lhs.Cmp(Rhs) != 0; }
template <typename CharT>
HK_FORCEINLINE bool operator!=(TStringView<CharT> Lhs, const CharT* Rhs) { return Lhs.Cmp(Rhs) != 0; }
template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator!=(TStringView<CharT> Lhs, TString<CharT, Allocator> const& Rhs) { return Lhs.Cmp(Rhs) != 0; }

template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator!=(TString<CharT, Allocator> const& Lhs, TString<CharT, Allocator> const& Rhs) { return Lhs.Cmp(Rhs) != 0; }
template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator!=(TString<CharT, Allocator> const& Lhs, const CharT* Rhs) { return Lhs.Cmp(Rhs) != 0; }
template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator!=(TString<CharT, Allocator> const& Lhs, TStringView<CharT> Rhs) { return Lhs.Cmp(Rhs) != 0; }

template <typename CharT, typename Allocator>
HK_FORCEINLINE bool operator!=(const CharT* Lhs, TString<CharT, Allocator> const& Rhs) { return Rhs.Cmp(Lhs) != 0; }
template <typename CharT>
HK_FORCEINLINE bool operator!=(const CharT* Lhs, TStringView<CharT> Rhs) { return Rhs.Cmp(Lhs) != 0; }


template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::GrowCapacity(SizeType _Capacity, bool bCopyOld)
{
    if (_Capacity <= m_Capacity)
        return;

    SizeType mod = _Capacity % Granularity;
    m_Capacity   = mod ? _Capacity + Granularity - mod : _Capacity;
    if (m_pData == m_Base)
    {
        m_pData = (CharT*)Allocator().allocate(m_Capacity * sizeof(CharT));
        if (bCopyOld)
            Platform::Memcpy(m_pData, m_Base, (m_Size + 1) * sizeof(CharT));
    }
    else
        m_pData = (CharT*)Allocator().reallocate(m_pData, m_Capacity * sizeof(CharT), bCopyOld);
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::Concat(TStringView<CharT> Rhs)
{
    SizeType rhsSize = Rhs.Size();
    if (rhsSize == 1)
    {
        Concat(Rhs[0]);
        return;
    }
    SizeType size = m_Size + rhsSize;
    Reserve(size);
    Platform::Memcpy(&m_pData[m_Size], Rhs.ToPtr(), rhsSize * sizeof(CharT));
    m_Size          = size;
    m_pData[m_Size] = 0;
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::Concat(CharT Ch)
{
    if (Ch == 0)
        return;
    Reserve(m_Size + 1);
    m_pData[m_Size++] = Ch;
    m_pData[m_Size]   = 0;
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::InsertAt(SizeType Index, TStringView<CharT> Str)
{
    if (Index > m_Size || Str.IsEmpty())
        return;

    SizeType n = Str.Size();
    Reserve(m_Size + n);

    SizeType moveCount = m_Size - Index + 1;
    CharT*   p         = &m_pData[m_Size];
    for (SizeType i = 0; i < moveCount; ++i, --p)
        *(p + n) = *p;
    Platform::Memcpy(&m_pData[Index], Str.ToPtr(), n * sizeof(CharT));
    m_Size += n;
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::InsertAt(SizeType Index, CharT Char)
{
    if (Index > m_Size || Char == 0)
        return;

    Reserve(m_Size + 1);

    SizeType moveCount = m_Size - Index + 1;
    CharT*   p         = &m_pData[m_Size];
    for (SizeType i = 0; i < moveCount; ++i, --p)
        *(p + 1) = *p;
    m_pData[Index] = Char;
    m_Size++;
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::ReplaceAt(SizeType Index, TStringView<CharT> Str)
{
    if (Index > m_Size || Str.IsEmpty())
        return;
    SizeType n    = Str.Size();
    SizeType size = Index + n;
    Reserve(size);
    Platform::Memcpy(&m_pData[Index], Str.ToPtr(), n * sizeof(CharT));
    m_Size          = size;
    m_pData[m_Size] = 0;
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::Replace(TStringView<CharT> Substr, TStringView<CharT> NewStr)
{
    if (Substr.IsEmpty())
        return;

    SizeType size = Substr.Size();
    SizeType index;
    SizeType offset = 0;
    while ((SizeType)-1 != (index = TStringView<CharT>(m_pData + offset, m_Size - offset).FindSubstring(Substr)))
    {
        index += offset;

        Cut(index, size);
        InsertAt(index, NewStr);

        offset = index + NewStr.Size();

        HK_ASSERT(offset <= m_Size);
    }
}

template <typename CharT, typename Allocator>
void TString<CharT, Allocator>::Cut(SizeType Index, SizeType Count)
{
    if (Index >= m_Size || Count == 0)
        return;
    if (Index + Count > m_Size)
        Count = m_Size - Index;

    SizeType srcIndex = Index + Count;
    Platform::Memcpy(m_pData + Index, m_pData + srcIndex, (m_Size - srcIndex + 1) * sizeof(CharT));

    m_Size = m_Size - Count;
}

template <typename CharT>
int TStringView<CharT>::Icmp(TStringView<CharT> Str) const
{
    const CharT* s1 = Begin();
    const CharT* e1 = End();
    const CharT* s2 = Str.Begin();
    const CharT* e2 = Str.End();

    CharT c1, c2;

    using UnsignedCharT = typename std::make_unsigned<CharT>::type;

    do {
        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

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
                return (int)((UnsignedCharT)c1 - (UnsignedCharT)c2);
            }
        }
    } while (c1);

    return 0;
}

template <typename CharT>
int TStringView<CharT>::Cmp(TStringView<CharT> Str) const
{
    const CharT* s1 = Begin();
    const CharT* e1 = End();
    const CharT* s2 = Str.Begin();
    const CharT* e2 = Str.End();

    CharT c1, c2;

    using UnsignedCharT = typename std::make_unsigned<CharT>::type;

    do {
        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

        if (c1 != c2)
        {
            return (int)((UnsignedCharT)c1 - (UnsignedCharT)c2);
        }
    } while (c1);

    return 0;
}

template <typename CharT>
int TStringView<CharT>::IcmpN(TStringView<CharT> Str, SizeType Num) const
{
    const CharT* s1 = Begin();
    const CharT* e1 = End();
    const CharT* s2 = Str.Begin();
    const CharT* e2 = Str.End();

    CharT c1, c2;

    using UnsignedCharT = typename std::make_unsigned<CharT>::type;

    do {
        if (!Num--)
        {
            return 0;
        }

        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

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
                return (int)((UnsignedCharT)c1 - (UnsignedCharT)c2);
            }
        }
    } while (c1);

    return 0;
}

template <typename CharT>
int TStringView<CharT>::CmpN(TStringView<CharT> Str, SizeType Num) const
{
    const CharT* s1 = Begin();
    const CharT* e1 = End();
    const CharT* s2 = Str.Begin();
    const CharT* e2 = Str.End();

    CharT c1, c2;

    using UnsignedCharT = typename std::make_unsigned<CharT>::type;

    do {
        if (!Num--)
        {
            return 0;
        }

        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

        if (c1 != c2)
        {
            return (int)((UnsignedCharT)c1 - (UnsignedCharT)c2);
        }
    } while (c1);

    return 0;
}


/**

TSprintfBuffer

Usage:

TSprintfBuffer< 128 > buffer;
buffer.Sprintf( "%d %f", 10, 15.1f );

String s = TSprintfBuffer< 128 >().Sprintf( "%d %f", 10, 15.1f );

*/
template <size_t Size>
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

using String         = TString<char>;
using WideString     = TString<WideChar>;
using StringView     = TStringView<char>;
using WideStringView = TStringView<WideChar>;

class Formatter
{
    fmt::memory_buffer m_Buffer;

public:
    template <typename... T>
    HK_INLINE StringView operator()(fmt::format_string<T...> Format, T&&... args)
    {
        m_Buffer.clear();
        fmt::detail::vformat_to(m_Buffer, fmt::string_view(Format), fmt::make_format_args(args...));
        return StringView(m_Buffer.begin(), m_Buffer.end());
    }

    template <typename T>
    StringView ToString(T const& Val)
    {
        return (*this)("{}", Val);
    }
};

#define HK_FORMAT(x, ...) Formatter()(HK_FMT(x), __VA_ARGS__)

namespace Core
{

HK_INLINE String GetString(WideStringView wideStr)
{
    const WideChar* s = wideStr.Begin();
    const WideChar* e = wideStr.End();

    auto size = WideStrUTF8Bytes(s, e);

    String str;
    str.Reserve(size);

    char buf[4];
    while (s < e)
    {
        int n = WideCharEncodeUTF8(buf, HK_ARRAY_SIZE(buf), *s++);
        str.Concat(StringView(buf, n));
    }
    return str;
}

HK_INLINE String GetString(AGlobalStringViewW wideStr)
{
    const WideChar* s = wideStr.CStr();
    const WideChar* e = s + StringLength(wideStr);

    auto size = WideStrUTF8Bytes(s, e);

    String str;
    str.Reserve(size);

    char buf[4];
    while (s < e)
    {
        int n = WideCharEncodeUTF8(buf, HK_ARRAY_SIZE(buf), *s++);
        str.Concat(StringView(buf, n));
    }
    return str;
}

HK_INLINE WideString GetWideString(StringView utfStr)
{
    const char* s = utfStr.Begin();
    const char* e = utfStr.End();

    auto numCharacters = UTF8StrLength(s, e);

    WideString wideStr;
    wideStr.Reserve(numCharacters);

    WideChar ch;
    while (s < e)
    {
        s += WideCharDecodeUTF8(s, e, ch);
        wideStr.Concat(ch);
    }
    return wideStr;
}

HK_INLINE WideString GetWideString(GlobalStringView utfStr)
{
    const char* s = utfStr.CStr();
    const char* e = s + StringLength(utfStr);

    auto numCharacters = UTF8StrLength(s, e);

    WideString wideStr;
    wideStr.Reserve(numCharacters);

    WideChar ch;
    while (s < e)
    {
        s += WideCharDecodeUTF8(s, e, ch);
        wideStr.Concat(ch);
    }
    return wideStr;
}



template <typename T>
HK_INLINE String ToString(T const& Val)
{
    return Formatter().ToString(Val);
}

template <typename T, std::enable_if_t<std::is_integral<T>::value || std::is_floating_point<T>::value, bool> = true>
HK_INLINE String ToHexString(T const& _Value, bool bLeadingZeros = false, bool bPrefix = false)
{
    TSprintfBuffer<32> value;
    TSprintfBuffer<32> format;
    const char*        prefixStr = bPrefix ? "0x" : "";

    constexpr size_t typeSize = sizeof(T);
    static_assert(typeSize == 1 || typeSize == 2 || typeSize == 4 || typeSize == 8, "ToHexString");

    switch (typeSize)
    {
        case 1:
            format.Sprintf(bLeadingZeros ? "%s%%02x" : "%s%%x", prefixStr);
            return value.Sprintf(format.Data, *reinterpret_cast<const uint8_t*>(&_Value));
        case 2:
            format.Sprintf(bLeadingZeros ? "%s%%04x" : "%s%%x", prefixStr);
            return value.Sprintf(format.Data, *reinterpret_cast<const uint16_t*>(&_Value));
        case 4:
            format.Sprintf(bLeadingZeros ? "%s%%08x" : "%s%%x", prefixStr);
            return value.Sprintf(format.Data, *reinterpret_cast<const uint32_t*>(&_Value));
        case 8: {
            uint64_t Temp = *reinterpret_cast<const uint64_t*>(&_Value);
            if (bLeadingZeros)
            {
                return value.Sprintf("%s%08x%08x", prefixStr, Math::INT64_HIGH_INT(Temp), Math::INT64_LOW_INT(Temp));
            }
            else
            {
                return value.Sprintf("%s%I64x", prefixStr, Temp);
            }
        }
    }
    return "";
}

} // namespace Core

HK_NAMESPACE_END

template <> struct fmt::formatter<Hk::TString<char>>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::TString<char> const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::detail::copy_str<char, const char*>(v.Begin(), v.End(), ctx.out());
    }
};
template <> struct fmt::formatter<Hk::TStringView<char>>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::TStringView<char> const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::detail::copy_str<char, const char*>(v.Begin(), v.End(), ctx.out());
    }
};
template <> struct fmt::formatter<Hk::TGlobalStringView<char>>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::TGlobalStringView<char> const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::detail::copy_str<char, const char*>(v.CStr(), v.CStr() + StringLength(v), ctx.out());
    }
};
template <> struct fmt::formatter<Hk::TString<Hk::WideChar>>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::TString<Hk::WideChar> const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        String str = Core::GetString(v);
        return fmt::detail::copy_str<char, const char*>(str.Begin(), str.End(), ctx.out());
    }
};
template <> struct fmt::formatter<Hk::TStringView<Hk::WideChar>>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::TStringView<Hk::WideChar> const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        String str = Core::GetString(v);
        return fmt::detail::copy_str<char, const char*>(str.Begin(), str.End(), ctx.out());
    }
};
template <> struct fmt::formatter<Hk::TGlobalStringView<Hk::WideChar>>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext> auto format(Hk::TGlobalStringView<Hk::WideChar> const& v, FormatContext& ctx) -> decltype(ctx.out())
    {
        String str = Core::GetString(v);
        return fmt::detail::copy_str<char, const char*>(str.Begin(), str.End(), ctx.out());
    }
};

HK_NAMESPACE_BEGIN

template <typename CharT, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_STRING>>
struct TPathUtils
{
    TPathUtils() = delete;

    /** Path utility. Return index of the path end. */
    static StringSizeType FindPath(TStringView<CharT> Path)
    {
        const CharT* p = Path.End();
        while (--p >= Path.Begin())
            if (Platform::IsPathSeparator(*p))
                return (StringSizeType)(p - Path.Begin() + 1);
        return 0;
    }

    /** Path utility. Get filename without path. */
    static TStringView<CharT> GetFilenameNoPath(TStringView<CharT> Path)
    {
        const CharT* e = Path.End();
        const CharT* p = e;
        while (--p >= Path.Begin() && !Platform::IsPathSeparator(*p))
        {}
        ++p;
        return TStringView<CharT>(p, (StringSizeType)(e - p), Path.IsNullTerminated());
    }

    /** Path utility. Get full filename without extension. */
    static TStringView<CharT> GetFilenameNoExt(TStringView<CharT> Path)
    {
        StringSizeType size = Path.Size();
        const CharT*   p    = Path.Begin() + size;
        while (--p >= Path.Begin())
        {
            if (*p == '.')
            {
                size = (StringSizeType)(p - Path.Begin());
                break;
            }
            if (Platform::IsPathSeparator(*p))
                break; // no extension
        }
        return TStringView<CharT>(Path.Begin(), size, Path.IsNullTerminated() && size == Path.Size());
    }

    /** Path utility. Get path without file name. */
    static TStringView<CharT> GetFilePath(TStringView<CharT> Path)
    {
        const CharT* p = Path.End();
        while (--p > Path.Begin() && !Platform::IsPathSeparator(*p))
        {}
        StringSizeType size = (StringSizeType)(p - Path.Begin());
        return TStringView<CharT>(Path.Begin(), size, Path.IsNullTerminated() && size == Path.Size());
    }

    /** Path utility. Check file extension. */
    static bool CompareExt(TStringView<CharT> Path, TStringView<CharT> Ext, bool bCaseInsensitive = true)
    {
        if (Ext.IsEmpty())
            return false;

        const CharT* p   = Path.End();
        const CharT* ext = Ext.End();
        if (bCaseInsensitive)
        {
            CharT c1, c2;
            while (--ext >= Ext.Begin())
            {
                if (--p < Path.Begin())
                    return false;

                c1 = *p;
                c2 = *ext;

                if (c1 != c2)
                {
                    if (c1 >= 'a' && c1 <= 'z')
                        c1 -= ('a' - 'A');
                    if (c2 >= 'a' && c2 <= 'z')
                        c2 -= ('a' - 'A');
                    if (c1 != c2)
                        return false;
                }
            }
        }
        else
        {
            while (--ext >= Ext.Begin())
                if (--p < Path.Begin() || *p != *ext)
                    return false;
        }
        return true;
    }

    /** Path utility. Return index where the extension begins. */
    static StringSizeType FindExt(TStringView<CharT> Path)
    {
        StringSizeType size = Path.Size();
        const CharT*   p    = Path.Begin() + size;
        while (--p >= Path.Begin() && !Platform::IsPathSeparator(*p))
            if (*p == '.')
                return (StringSizeType)(p - Path.Begin());
        return size;
    }

    /** Path utility. Return index where the extension begins after dot. */
    static StringSizeType FindExtWithoutDot(TStringView<CharT> Path)
    {
        StringSizeType size = Path.Size();
        const CharT*   p    = Path.Begin() + size;
        while (--p >= Path.Begin() && !Platform::IsPathSeparator(*p))
            if (*p == '.')
                return (StringSizeType)(p - Path.Begin() + 1);
        return size;
    }

    /** Path utility. Get filename extension. */
    static TStringView<CharT> GetExt(TStringView<CharT> Path)
    {
        const CharT* p = Path.Begin() + FindExt(Path);
        return TStringView<CharT>(p, (StringSizeType)(Path.End() - p), Path.IsNullTerminated());
    }

    /** Path utility. Get filename extension without dot. */
    static TStringView<CharT> GetExtWithoutDot(TStringView<CharT> Path)
    {
        const CharT* p = Path.Begin() + FindExtWithoutDot(Path);
        return TStringView<CharT>(p, (StringSizeType)(Path.End() - p), Path.IsNullTerminated());
    }

    /** Path utility. Fix OS-specific separator. */
    static void FixSeparatorInplace(TString<CharT, Allocator>& Path)
    {
        CharT* p = Path.ToPtr();
        while (*p)
        {
            if (*p == '\\')
                *p = '/';
            p++;
        }
    }

    /** Path utility. Fix OS-specific separator. */
    static TString<CharT, Allocator> FixSeparator(TStringView<CharT> Path)
    {
        TString<CharT, Allocator> fixedPath(Path);
        FixSeparatorInplace(fixedPath);
        return fixedPath;
    }

    /** Path utility.
    Fix path string: replace separator \\ to /, skip series of /,
    skip redunant sequences of dir/../dir2 -> dir. */
    static void FixPathInplace(TString<CharT, Allocator>& Path)
    {
        StringSizeType newSize = Platform::FixPath(Path.ToPtr(), Path.Size());
        Path.Resize(newSize);
    }

    /** Path utility.
    Fix path string: replace separator \\ to /, skip series of /,
    skip redunant sequences of dir/../dir2 -> dir. */
    static TString<CharT, Allocator> FixPath(TStringView<CharT> Path)
    {
        TString<CharT, Allocator> fixedPath(Path);
        StringSizeType            newSize = Platform::FixPath(fixedPath.ToPtr(), fixedPath.Size());
        fixedPath.Resize(newSize);
        return fixedPath;
    }

    /** Path utility. Add or replace existing file extension. */
    static void SetExtensionInplace(TString<CharT, Allocator>& Path, TStringView<CharT> Extension, bool bReplace)
    {
        StringSizeType pos = FindExtWithoutDot(Path);
        if (pos == Path.Size())
        {
            Path.Reserve(pos + Extension.Size() + 1);
            Path += ".";
            Path += Extension;
        }
        else
        {
            if (bReplace)
            {
                Path.Resize(pos);
                Path += Extension;
            }
        }
    }

    /** Path utility. Add or replace existing file extension. */
    static TString<CharT, Allocator> SetExtension(TStringView<CharT> Path, TStringView<CharT> Extension, bool bReplace)
    {
        StringSizeType pos = FindExtWithoutDot(Path);

        if (pos == Path.Size())
        {
            TString<CharT, Allocator> newPath;
            newPath.Reserve(pos + Extension.Size() + 1);
            newPath += Path;
            newPath += CharT('.');
            newPath += Extension;
            return newPath;
        }

        if (bReplace)
        {
            TStringView<CharT> filePath = Path.GetSubstring(0, pos);

            TString<CharT, Allocator> newPath;
            newPath.Reserve(filePath.Size() + Extension.Size());
            newPath += filePath;
            newPath += Extension;
            return newPath;
        }

        return Path;
    }
};

using PathUtils  = TPathUtils<char>;
using PathUtilsW = TPathUtils<WideChar>;

HK_NAMESPACE_END
