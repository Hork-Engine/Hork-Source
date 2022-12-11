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

#include "String.h"

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
T ParseNumber(StringView);

template <>
HK_FORCEINLINE float ParseNumber<float>(StringView Str)
{
    return ParseFloat(Str);
}

template <>
HK_FORCEINLINE double ParseNumber<double>(StringView Str)
{
    return ParseDouble(Str);
}

template <>
HK_FORCEINLINE uint8_t ParseNumber<uint8_t>(StringView Str)
{
    return ParseUInt8(Str);
}

template <>
HK_FORCEINLINE uint16_t ParseNumber<uint16_t>(StringView Str)
{
    return ParseUInt16(Str);
}

template <>
HK_FORCEINLINE uint32_t ParseNumber<uint32_t>(StringView Str)
{
    return ParseUInt32(Str);
}

template <>
HK_FORCEINLINE uint64_t ParseNumber<uint64_t>(StringView Str)
{
    return ParseUInt64(Str);
}

template <>
HK_FORCEINLINE int8_t ParseNumber<int8_t>(StringView Str)
{
    return ParseInt8(Str);
}

template <>
HK_FORCEINLINE int16_t ParseNumber<int16_t>(StringView Str)
{
    return ParseInt16(Str);
}

template <>
HK_FORCEINLINE int32_t ParseNumber<int32_t>(StringView Str)
{
    return ParseInt32(Str);
}

template <>
HK_FORCEINLINE int64_t ParseNumber<int64_t>(StringView Str)
{
    return ParseInt64(Str);
}

template <>
HK_FORCEINLINE bool ParseNumber<bool>(StringView Str)
{
    return ParseBool(Str);
}

}
