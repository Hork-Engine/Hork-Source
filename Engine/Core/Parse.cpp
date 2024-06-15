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

#include "Parse.h"
#include "fast_float.h"

HK_NAMESPACE_BEGIN

namespace Core
{

uint64_t ParseHex(StringView Str, const size_t SizeOf)
{
    HK_ASSERT(SizeOf > 1 && SizeOf <= 8);
    
    uint64_t val = 0;
    
    const char* s = Str.Begin();
    const char* e = Str.End();
    
    if (e - s > SizeOf * 2)
    {   
        LOG("ParseHex: too long number\n");
        return 0;
    }
    
    while (s < e)
    {
        char c = *s++;
        if (c >= '0' && c <= '9')
        {
            val = (val << 4) + (c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            val = (val << 4) + (c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            val = (val << 4) + (c - 'A' + 10);
        }
        else
        {
            LOG("ParseHex: invalid character {}\n", c);
            return 0;
        }
    }
    return val;
}

float ParseFloat(StringView Str)
{
    // Cast from boolean
    if (Str.Icompare("false"))
        return 0.0f;
    else if (Str.Icompare("true"))
        return 1.0f;
    
    const char* s = Str.Begin();
    const char* e = Str.End();

    // check for hex
    if (s + 2 < e && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        uint32_t hex = ParseHex32(Str.TruncateHead(2));
        float fval = *reinterpret_cast<float*>(&hex);
        if (std::isnan(fval))
        {
            LOG("ParseFloat: invalid number {}\n", Str);
            return 0.0f;
        }
        return fval;
    }
    
    double val = 0;
    auto result = fast_float::from_chars(Str.Begin(), Str.End(), val);
    if (result.ec != std::errc())
    {
        LOG("ParseFloat: failed to parse number {}\n", Str);
        return 0.0f;
    }
    return static_cast<float>(val);
}

double ParseDouble(StringView Str)
{
    // Cast from boolean
    if (Str.Icompare("false"))
        return 0.0;
    else if (Str.Icompare("true"))
        return 1.0;
    
    const char* s = Str.Begin();
    const char* e = Str.End();
    
    double val = 0;

    // check for hex
    if (s + 2 < e && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        uint64_t hex = ParseHex64(Str.TruncateHead(2));
        val = *reinterpret_cast<double*>(&hex);
        if (std::isnan(val))
        {
            LOG("ParseDouble: invalid number {}\n", Str);
            return 0.0;
        }
        return val;
    }
    
    auto result = fast_float::from_chars(Str.Begin(), Str.End(), val);
    if (result.ec != std::errc())
    {
        LOG("ParseDouble: failed to parse number {}\n", Str);
        return 0.0;
    }
    return val;
}

float ParseCvar(StringView Str)
{
    // Cast from boolean
    if (Str.Icompare("false"))
        return 0.0f;
    else if (Str.Icompare("true"))
        return 1.0f;
    double val = 0;
    auto result = fast_float::from_chars(Str.Begin(), Str.End(), val);
    return (result.ec != std::errc()) ? 0.0f : static_cast<float>(val);
}

int64_t ParseSigned(StringView Str)
{
    // Cast from boolean
    if (Str.Icompare("false"))
        return 0;
    else if (Str.Icompare("true"))
        return 1;
    
    const char* s = Str.Begin();
    const char* e = Str.End();

    // check for hex
    if (s + 2 < e && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        return ParseHex64(Str.TruncateHead(2));
    }
    
    if (s == e)
    {
        LOG("ParseSigned: empty string\n");
        return 0;
    }
    
    uint64_t val = 0;
      
    bool bNegative = (*s == '-');
    
    const char *p = s + bNegative;
    while (p < e)
    {
        uint8_t digit = static_cast<uint8_t>(*p - '0');
        if (digit > 9)
        {
            if (*p == '.')
            {
                // Try cast from double
                return static_cast<int64_t>(ParseDouble(Str));
            }
            LOG("ParseSigned: invalid character {}\n", *p);
            return 0;
        }
        val = 10 * val + digit;
        ++p;
    }

    size_t numDigits = size_t(p - s);
    const size_t longestDigitCount = 19;
    if (numDigits == 0)
    {
        LOG("ParseSigned: empty string\n");
        return 0;
    }
    if (numDigits > longestDigitCount)
    {
        LOG("ParseSigned: too long number\n");
        return 0;
    }
    
    if (val > uint64_t(INT64_MAX) + uint64_t(bNegative))
    {
        LOG("ParseSigned: overflow\n");
        return 0;
    }
    
    return bNegative ? (~val+1) : val;
}

uint64_t ParseUnsigned(StringView Str)
{
    // Cast from boolean
    if (Str.Icompare("false"))
        return 0;
    else if (Str.Icompare("true"))
        return 1;
    
    const char* s = Str.Begin();
    const char* e = Str.End();

    // check for hex
    if (s + 2 < e && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        return ParseHex64(Str.TruncateHead(2));
    }
    
    uint64_t val = 0;
    
    const char *p = s;
    while (p < e)
    {
        uint8_t digit = static_cast<uint8_t>(*p - '0');
        if (digit > 9)
        {
            if (*p == '.')
            {
                // Try cast from double
                return static_cast<uint64_t>(ParseDouble(Str));
            }
            LOG("ParseUnsigned: Warning: invalid character {}\n", *p);
            return 0;
        }
        val = 10 * val + digit;
        ++p;
    }
    
    size_t numDigits = size_t(p - s);
    if (numDigits == 0)
    {
        LOG("ParseUnsigned: empty string\n");
        return 0;
    }
    if (numDigits > 20)
    {
        LOG("ParseUnsigned: too long number\n");
        return 0;
    }
    
    if (numDigits == 20)
    {
        if (s[0] != uint8_t('1') || val <= uint64_t(INT64_MAX))
        {
            LOG("ParseUnsigned: overflow\n");
            return 0;
        }
    }
    return val;
}

bool ParseBool(StringView Str)
{
    return ParseSigned(Str) != 0;
}

}

HK_NAMESPACE_END
