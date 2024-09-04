/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "Containers/Vector.h"
#include "Logger.h"

HK_NAMESPACE_BEGIN

namespace Core
{

uint64_t ParseHex(StringView Str, const size_t SizeOf);

template <typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
T ParseHex(StringView Str)
{
    return static_cast<T>(ParseHex(Str, sizeof(T)));
}

HK_FORCEINLINE uint8_t ParseHex8(StringView Str)
{
    return ParseHex<uint8_t>(Str);
}

HK_FORCEINLINE uint16_t ParseHex16(StringView Str)
{
    return ParseHex<uint16_t>(Str);
}

HK_FORCEINLINE uint32_t ParseHex32(StringView Str)
{
    return ParseHex<uint32_t>(Str);
}

HK_FORCEINLINE uint64_t ParseHex64(StringView Str)
{
    return ParseHex<uint64_t>(Str);
}

float ParseFloat(StringView Str);
double ParseDouble(StringView Str);
int64_t ParseSigned(StringView Str);
uint64_t ParseUnsigned(StringView Str);
bool ParseBool(StringView Str);
float ParseCvar(StringView Str);

HK_FORCEINLINE uint64_t UnsignedBoundsCheck(uint64_t Value, uint64_t MaxValue)
{
    if (Value > MaxValue)
        LOG("The value of {} must be less than {}.\n", Value, MaxValue);
    return Value;
}

HK_FORCEINLINE int64_t SignedBoundsCheck(int64_t Value, int64_t MinValue, int64_t MaxValue)
{
    if (Value < MinValue || Value > MaxValue)
        LOG("The value of {} must be greater then {} and less than {}.\n", Value, MinValue, MaxValue);
    return Value;
}

HK_FORCEINLINE uint8_t ParseUInt8(StringView Str)
{
    return static_cast<uint8_t>(UnsignedBoundsCheck(ParseUnsigned(Str), Math::MaxValue<uint8_t>()));
}

HK_FORCEINLINE uint16_t ParseUInt16(StringView Str)
{
    return static_cast<uint16_t>(UnsignedBoundsCheck(ParseUnsigned(Str), Math::MaxValue<uint16_t>()));
}

HK_FORCEINLINE uint32_t ParseUInt32(StringView Str)
{
    return static_cast<uint32_t>(UnsignedBoundsCheck(ParseUnsigned(Str), Math::MaxValue<uint32_t>()));
}

HK_FORCEINLINE uint64_t ParseUInt64(StringView Str)
{
    return ParseUnsigned(Str);
}

HK_FORCEINLINE int8_t ParseInt8(StringView Str)
{
    return static_cast<int8_t>(SignedBoundsCheck(ParseSigned(Str), Math::MinValue<int8_t>(), Math::MaxValue<int8_t>()));
}

HK_FORCEINLINE int16_t ParseInt16(StringView Str)
{
    return static_cast<int16_t>(SignedBoundsCheck(ParseSigned(Str), Math::MinValue<int16_t>(), Math::MaxValue<int16_t>()));
}

HK_FORCEINLINE int32_t ParseInt32(StringView Str)
{
    return static_cast<int32_t>(SignedBoundsCheck(ParseSigned(Str), Math::MinValue<int32_t>(), Math::MaxValue<int32_t>()));
}

HK_FORCEINLINE int64_t ParseInt64(StringView Str)
{
    return ParseSigned(Str);
}

template <typename T>
T Parse(StringView);

template <>
HK_FORCEINLINE float Parse<float>(StringView Str)
{
    return ParseFloat(Str);
}

template <>
HK_FORCEINLINE double Parse<double>(StringView Str)
{
    return ParseDouble(Str);
}

template <>
HK_FORCEINLINE uint8_t Parse<uint8_t>(StringView Str)
{
    return ParseUInt8(Str);
}

template <>
HK_FORCEINLINE uint16_t Parse<uint16_t>(StringView Str)
{
    return ParseUInt16(Str);
}

template <>
HK_FORCEINLINE uint32_t Parse<uint32_t>(StringView Str)
{
    return ParseUInt32(Str);
}

template <>
HK_FORCEINLINE uint64_t Parse<uint64_t>(StringView Str)
{
    return ParseUInt64(Str);
}

template <>
HK_FORCEINLINE int8_t Parse<int8_t>(StringView Str)
{
    return ParseInt8(Str);
}

template <>
HK_FORCEINLINE int16_t Parse<int16_t>(StringView Str)
{
    return ParseInt16(Str);
}

template <>
HK_FORCEINLINE int32_t Parse<int32_t>(StringView Str)
{
    return ParseInt32(Str);
}

template <>
HK_FORCEINLINE int64_t Parse<int64_t>(StringView Str)
{
    return ParseInt64(Str);
}

template <>
HK_FORCEINLINE bool Parse<bool>(StringView Str)
{
    return ParseBool(Str);
}

HK_INLINE StringView GetToken(StringView& token, StringView string, bool bCrossLine = true)
{
    const char* p = string.Begin();
    const char* end = string.End();

    token = "";

    // skip space
    for (;;)
    {
        if (p == end)
        {
            return StringView(p, (StringSizeType)(end - p));
        }

        if (*p == '\n' && !bCrossLine)
        {
            LOG("Unexpected new line\n");
            return StringView(p, (StringSizeType)(end - p));
        }

        if (*p > 32)
            break;

        p++;
    }

    const char* tokenBegin = p;
    while (p < end)
    {
        if (*p == '\n')
        {
            if (!bCrossLine)
                LOG("Unexpected new line\n");
            break;
        }

        if (*p <= 32)
            break;

        if (*p == '(' || *p == ')')
        {
            if (p == tokenBegin)
                ++p;            
            break;
        }

        ++p;
    }

    token = StringView(tokenBegin, p);

    return StringView(p, (StringSizeType)(end - p), string.IsNullTerminated());
}


template <typename VectorType>
HK_INLINE VectorType ParseVector(StringView string, StringView* newString = nullptr)
{
    VectorType v;

    StringView token;
    StringView tmp;

    if (!newString)
        newString = &tmp;

    StringView& s = *newString;

    s = GetToken(token, string);
    if (!token.Compare("("))
    {
        LOG("Expected '('\n");
        return v;
    }

    for (int i = 0; i < v.NumComponents(); i++)
    {
        s = GetToken(token, s);
        if (token.IsEmpty())
        {
            LOG("Expected value\n");
            return v;
        }

        using ElementType = std::remove_reference_t<decltype(v[i])>;

        v[i] = Core::Parse<ElementType>(token);
    }

    s = GetToken(token, s);
    if (!token.Compare(")"))
    {
        LOG("Expected ')'\n");
    }

    return v;
}

// Parses a vector of variable length. Returns false if there were errors.
HK_INLINE bool ParseVector(StringView string, Vector<StringView>& v)
{
    StringView token;

    v.Clear();

    StringView s = GetToken(token, string);
    if (!token.Compare("("))
    {
        v.Add(token);
        return true;
    }

    do
    {
        s = GetToken(token, s);
        if (token.IsEmpty())
        {
            LOG("ParseVector: Expected value\n");
            return false;
        }

        if (token.Compare(")"))
        {
            return true;
        }

        v.Add(token);
    } while (1);

    s = GetToken(token, s);
    if (!token.Compare(")"))
    {
        LOG("ParseVector: Expected ')'\n");
        return false;
    }

    return true;
}

template <typename MatrixType>
HK_INLINE MatrixType ParseMatrix(StringView string)
{
    MatrixType matrix(1);

    StringView token;
    StringView s = string;

    s = GetToken(token, s);
    if (!token.Compare("("))
    {
        LOG("Expected '('\n");
        return matrix;
    }

    for (int i = 0; i < matrix.NumComponents(); i++)
    {
        using ElementType = std::remove_reference_t<decltype(matrix[i])>;

        matrix[i] = ParseVector<ElementType>(s, &s);
    }

    s = GetToken(token, s);
    if (!token.Compare(")"))
    {
        LOG("Expected ')'\n");
    }

    return matrix;
}

}

HK_NAMESPACE_END
