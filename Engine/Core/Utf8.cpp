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

#include "Utf8.h"

HK_NAMESPACE_BEGIN

namespace Core
{

#define __utf8_is_1b(s) (!(*(s)&0x80))
#define __utf8_is_2b(s) ((*(s)&0xE0) == 0xC0)
#define __utf8_is_3b(s) ((*(s)&0xF0) == 0xE0)
#define __utf8_is_4b(s) ((*(s)&0xF8) == 0xF0)

int UTF8CharSizeInBytes(const char* unicode)
{
    if (__utf8_is_1b(unicode))
    {
        return 1;
    }
    if (__utf8_is_2b(unicode))
    {
        if (unicode[1] == 0)
        {
            return 1;
        }
        return 2;
    }
    if (__utf8_is_3b(unicode))
    {
        if (unicode[1] == 0 || unicode[2] == 0)
        {
            return 1;
        }
        return 3;
    }
    if (__utf8_is_4b(unicode))
    {
        if (unicode[1] == 0 || unicode[2] == 0 || unicode[3] == 0)
        {
            return 1;
        }
        return 4;
    }
    return 0;
}

int UTF8StrLength(const char* unicode)
{
    int strLength = 0;
    while (*unicode)
    {
        const int byteLen = UTF8CharSizeInBytes(unicode);
        if (byteLen > 0)
        {
            unicode += byteLen;
            strLength++;
        }
        else
        {
            break;
        }
    }
    return strLength;
}

int UTF8StrLength(const char* unicode, const char* unicodeEnd)
{
    int strLength = 0;
    while (unicode < unicodeEnd)
    {
        const int byteLen = UTF8CharSizeInBytes(unicode);
        if (byteLen > 0)
        {
            unicode += byteLen;
            strLength++;
        }
        else
        {
            break;
        }
    }
    return strLength;
}

#if 0
int EncodeUTF8Char( unsigned int ch, char encoded[4] ) {
    if ( ch <= 0x7f ) {
        encoded[0] = ch;
        return 1;
    }
    if ( ch <= 0x7ff ) {
        encoded[0] = ( ch >> 6 ) | 0xC0;
        encoded[1] = ( ch & 0x3F ) | 0x80;
        return 2;
    }
    if ( ch <= 0xffff ) {
        encoded[0] = ( ch >> 12 ) | 0xE0;
        encoded[1] = ( ( ch >> 6 ) & 0x3F ) | 0x80;
        encoded[2] = ( ch & 0x3F ) | 0x80;
        return 3;
    }
    if ( ch <= 0x10FFFF ) {
        encoded[0] = ( ch >> 18 ) | 0xF0;
        encoded[1] = ( ( ch >> 12 ) & 0x3F ) | 0x80;
        encoded[2] = ( ( ch >> 6 ) & 0x3F ) | 0x80;
        encoded[3] = ( ch & 0x3F ) | 0x80;
        return 4;
    }
    encoded[0] = '?';
    return 1;
}
#endif

#if 0
int WideCharDecodeUTF8(const char* unicode, WideChar& ch)
{
    const unsigned char* s = (const unsigned char*)unicode;
    if (__utf8_is_1b(s))
    {
        ch = *s;
        return 1;
    }
    if (__utf8_is_2b(s))
    {
        ch = 0xFFFD;
        if (s[1] == 0)
        {
            return 1;
        }
        if (s[0] < 0xC2)
        {
            return 2;
        }
        if ((s[1] & 0xC0) == 0x80)
        {
            const int b1 = s[0] & 0x1F;
            const int b2 = s[1] & 0x3F;
            ch          = (b1 << 6) | b2;
        }
        return 2;
    }
    if (__utf8_is_3b(s))
    {
        ch = 0xFFFD;
        if (s[1] == 0 || s[2] == 0)
        {
            return 1;
        }
        if (s[0] == 0xE0 && (s[1] < 0xA0 || s[1] > 0xBF))
        {
            return 3;
        }
        if (s[0] == 0xED && s[1] > 0x9F)
        { // s[1] < 0x80 is checked below
            return 3;
        }

        if ((s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80)
        {
            const int b1 = s[0] & 0x0F;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            ch          = (b1 << 12) | (b2 << 6) | b3;
        }

        return 3;
    }
    if (__utf8_is_4b(s))
    {
        ch = 0xFFFD;
        if (s[1] == 0 || s[2] == 0 || s[3] == 0)
        {
            return 1;
        }
        if (s[0] > 0xF4)
        {
            return 4;
        }
        if (s[0] == 0xF0 && (s[1] < 0x90 || s[1] > 0xBF))
        {
            return 4;
        }
        if (s[1] == 0xF4 && s[1] > 0x8F)
        { // s[1] < 0x80 is checked below
            return 4;
        }

        if ((s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80)
        {
            const int b1 = s[0] & 0x07;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            const int b4 = s[3] & 0x3F;

            const int c = (b1 << 18) | (b2 << 12) | (b3 << 6) | b4;

            // utf-8 encodings of values used in surrogate pairs are invalid
            if ((c & 0xFFFFF800) == 0xD800 || c > 0xFFFF)
            {
                return 4;
            }

            ch = c;
        }
        return 4;
    }
    ch = 0;
    return 0;
}

int WideCharDecodeUTF8(const char* unicode, const char* unicodeEnd, WideChar& ch)
{
    HK_ASSERT(unicodeEnd != nullptr);
    const unsigned char* s = (const unsigned char*)unicode;
    const unsigned char* e = (const unsigned char*)unicodeEnd;
    if (__utf8_is_1b(s))
    {
        ch = *s;
        return 1;
    }
    if (__utf8_is_2b(s))
    {
        ch = 0xFFFD;
        if (e - s < 2)
        {
            return 1;
        }
        if (s[0] < 0xC2)
        {
            return 2;
        }
        if ((s[1] & 0xC0) == 0x80)
        {
            const int b1 = s[0] & 0x1F;
            const int b2 = s[1] & 0x3F;
            ch          = (b1 << 6) | b2;
        }
        return 2;
    }
    if (__utf8_is_3b(s))
    {
        ch = 0xFFFD;
        if (e - s < 3)
        {
            return 1;
        }

        if (s[0] == 0xE0 && (s[1] < 0xA0 || s[1] > 0xBF))
        {
            return 3;
        }
        if (s[0] == 0xED && s[1] > 0x9F)
        { // s[1] < 0x80 is checked below
            return 3;
        }

        if ((s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80)
        {
            const int b1 = s[0] & 0x0F;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            ch          = (b1 << 12) | (b2 << 6) | b3;
        }

        return 3;
    }
    if (__utf8_is_4b(s))
    {
        ch = 0xFFFD;
        if (e - s < 4)
        {
            return 1;
        }

        if (s[0] > 0xF4)
        {
            return 4;
        }
        if (s[0] == 0xF0 && (s[1] < 0x90 || s[1] > 0xBF))
        {
            return 4;
        }
        if (s[1] == 0xF4 && s[1] > 0x8F)
        { // s[1] < 0x80 is checked below
            return 4;
        }

        if ((s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80)
        {
            const int b1 = s[0] & 0x07;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            const int b4 = s[3] & 0x3F;

            const int c = (b1 << 18) | (b2 << 12) | (b3 << 6) | b4;

            // utf-8 encodings of values used in surrogate pairs are invalid
            if ((c & 0xFFFFF800) == 0xD800 || c > 0xFFFF)
            {
                return 4;
            }

            ch = c;
        }
        return 4;
    }
    ch = 0;
    return 0;
}
#endif

int WideCharDecodeUTF8(const char* unicode, WideChar& ch)
{
    return WideCharDecodeUTF8(unicode, nullptr, ch);
}

// Convert UTF-8 to 32-bit character, process single character input.
// A nearly-branchless UTF-8 decoder, based on work of Christopher Wellons (https://github.com/skeeto/branchless-utf8).
// We handle UTF-8 decoding error by skipping forward.
int WideCharDecodeUTF8(const char* unicode, const char* unicodeEnd, WideChar& ch)
{
    static const char lengths[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
    static const int masks[]  = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
    static const uint32_t mins[] = { 0x400000, 0, 0x80, 0x800, 0x10000 };
    static const int shiftc[] = { 0, 18, 12, 6, 0 };
    static const int shifte[] = { 0, 6, 4, 2, 0 };
    int len = lengths[*(const unsigned char*)unicode >> 3];
    int wanted = len + !len;

    if (unicodeEnd == nullptr)
        unicodeEnd = unicode + wanted; // Max length, nulls will be taken into account.

    // Copy at most 'len' bytes, stop copying at 0 or past unicodeEnd. Branch predictor does a good job here,
    // so it is fast even with excessive branching.
    unsigned char s[4];
    s[0] = unicode + 0 < unicodeEnd ? unicode[0] : 0;
    s[1] = unicode + 1 < unicodeEnd ? unicode[1] : 0;
    s[2] = unicode + 2 < unicodeEnd ? unicode[2] : 0;
    s[3] = unicode + 3 < unicodeEnd ? unicode[3] : 0;

    uint32_t c;

    // Assume a four-byte character and load four bytes. Unused bits are shifted out.
    c  = (uint32_t)(s[0] & masks[len]) << 18;
    c |= (uint32_t)(s[1] & 0x3f) << 12;
    c |= (uint32_t)(s[2] & 0x3f) <<  6;
    c |= (uint32_t)(s[3] & 0x3f) <<  0;
    c >>= shiftc[len];

    // Accumulate the various error conditions.
    int e = 0;
    e  = (c < mins[len]) << 6; // non-canonical encoding
    e |= ((c >> 11) == 0x1b) << 7;  // surrogate half?
    e |= (c > 0xFFFF) << 8;  // out of range?
    e |= (s[1] & 0xc0) >> 2;
    e |= (s[2] & 0xc0) >> 4;
    e |= (s[3]       ) >> 6;
    e ^= 0x2a; // top two bits of each tail byte correct?
    e >>= shifte[len];

    if (e)
    {
        // No bytes are consumed when *unicode == 0 || unicode == unicodeEnd.
        // One byte is consumed in case of invalid first byte of unicode.
        // All available bytes (at most `len` bytes) are consumed on incomplete/invalid second to last bytes.
        // Invalid or incomplete input may consume less bytes than wanted, therefore every byte has to be inspected in s.
        wanted = std::min(wanted, !!s[0] + !!s[1] + !!s[2] + !!s[3]);
        c = 0xFFFD;     // Invalid Unicode code point (standard value)
    }

    ch = c;

    return wanted;
}

int WideStrDecodeUTF8(const char* unicode, WideChar* str, int maxLength)
{
    if (maxLength <= 0)
    {
        return 0;
    }
    WideChar* pout = str;
    while (*unicode && --maxLength > 0)
    {
        const int byteLen = WideCharDecodeUTF8(unicode, *str);
        if (!byteLen)
        {
            break;
        }
        unicode += byteLen;
        str++;
    }
    *str = 0;
    return str - pout;
}

int WideStrDecodeUTF8(const char* unicode, const char* unicodeEnd, WideChar* str, int maxLength)
{
    if (maxLength <= 0)
    {
        return 0;
    }
    WideChar* pout = str;
    while (*unicode && --maxLength > 0)
    {
        const int byteLen = WideCharDecodeUTF8(unicode, unicodeEnd, *str);
        if (!byteLen)
        {
            break;
        }
        unicode += byteLen;
        str++;
    }
    *str = 0;
    return str - pout;
}

int WideStrUTF8Bytes(WideChar const* str, WideChar const* strEnd)
{
    int byteLen = 0;
    while ((!strEnd || str < strEnd) && *str)
    {
        byteLen += WideCharUTF8Bytes(*str++);
    }
    return byteLen;
}

int WideStrLength(WideChar const* str)
{
    WideChar const* p = str;
    while (*p) { p++; }
    return p - str;
}

int WideCharEncodeUTF8(char* buf, int bufSize, unsigned int ch)
{
    if (ch < 0x80)
    {
        buf[0] = (char)ch;
        return 1;
    }
    if (ch < 0x800)
    {
        if (bufSize < 2) return 0;
        buf[0] = (char)(0xc0 + (ch >> 6));
        buf[1] = (char)(0x80 + (ch & 0x3f));
        return 2;
    }
    if (ch >= 0xdc00 && ch < 0xe000)
    {
        return 0;
    }
    if (ch >= 0xd800 && ch < 0xdc00)
    {
        if (bufSize < 4) return 0;
        buf[0] = (char)(0xf0 + (ch >> 18));
        buf[1] = (char)(0x80 + ((ch >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((ch >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((ch)&0x3f));
        return 4;
    }
    //else if ( c < 0x10000 )
    {
        if (bufSize < 3) return 0;
        buf[0] = (char)(0xe0 + (ch >> 12));
        buf[1] = (char)(0x80 + ((ch >> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((ch)&0x3f));
        return 3;
    }
}

int WideStrEncodeUTF8(char* buf, int bufSize, WideChar const* str, WideChar const* strEnd)
{
    if (bufSize <= 0)
    {
        return 0;
    }
    char*       pBuf = buf;
    const char* pEnd = buf + bufSize - 1;
    while (pBuf < pEnd && (!strEnd || str < strEnd) && *str)
    {
        WideChar ch = *str++;
        if (ch < 0x80)
        {
            *pBuf++ = (char)ch;
        }
        else
        {
            pBuf += WideCharEncodeUTF8(pBuf, (int)(pEnd - pBuf), ch);
        }
    }
    *pBuf = 0;
    return (int)(pBuf - buf);
}

} // namespace Core

HK_NAMESPACE_END
