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

#include <Platform/Utf8.h>

namespace Core
{

#define __utf8_is_1b(s) (!(*(s)&0x80))
#define __utf8_is_2b(s) ((*(s)&0xE0) == 0xC0)
#define __utf8_is_3b(s) ((*(s)&0xF0) == 0xE0)
#define __utf8_is_4b(s) ((*(s)&0xF8) == 0xF0)

int UTF8CharSizeInBytes(const char* _Unicode)
{
    if (__utf8_is_1b(_Unicode))
    {
        return 1;
    }
    if (__utf8_is_2b(_Unicode))
    {
        if (_Unicode[1] == 0)
        {
            return 1;
        }
        return 2;
    }
    if (__utf8_is_3b(_Unicode))
    {
        if (_Unicode[1] == 0 || _Unicode[2] == 0)
        {
            return 1;
        }
        return 3;
    }
    if (__utf8_is_4b(_Unicode))
    {
        if (_Unicode[1] == 0 || _Unicode[2] == 0 || _Unicode[3] == 0)
        {
            return 1;
        }
        return 4;
    }
    return 0;
}

int UTF8StrLength(const char* _Unicode)
{
    int strLength = 0;
    while (*_Unicode)
    {
        const int byteLen = UTF8CharSizeInBytes(_Unicode);
        if (byteLen > 0)
        {
            _Unicode += byteLen;
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
int EncodeUTF8Char( unsigned int _Ch, char _Encoded[4] ) {
    if ( _Ch <= 0x7f ) {
        _Encoded[0] = _Ch;
        return 1;
    }
    if ( _Ch <= 0x7ff ) {
        _Encoded[0] = ( _Ch >> 6 ) | 0xC0;
        _Encoded[1] = ( _Ch & 0x3F ) | 0x80;
        return 2;
    }
    if ( _Ch <= 0xffff ) {
        _Encoded[0] = ( _Ch >> 12 ) | 0xE0;
        _Encoded[1] = ( ( _Ch >> 6 ) & 0x3F ) | 0x80;
        _Encoded[2] = ( _Ch & 0x3F ) | 0x80;
        return 3;
    }
    if ( _Ch <= 0x10FFFF ) {
        _Encoded[0] = ( _Ch >> 18 ) | 0xF0;
        _Encoded[1] = ( ( _Ch >> 12 ) & 0x3F ) | 0x80;
        _Encoded[2] = ( ( _Ch >> 6 ) & 0x3F ) | 0x80;
        _Encoded[3] = ( _Ch & 0x3F ) | 0x80;
        return 4;
    }
    _Encoded[0] = '?';
    return 1;
}
#endif

#if 0
int WideCharDecodeUTF8(const char* _Unicode, SWideChar& _Ch)
{
    const unsigned char* s = (const unsigned char*)_Unicode;
    if (__utf8_is_1b(s))
    {
        _Ch = *s;
        return 1;
    }
    if (__utf8_is_2b(s))
    {
        _Ch = 0xFFFD;
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
            _Ch          = (b1 << 6) | b2;
        }
        return 2;
    }
    if (__utf8_is_3b(s))
    {
        _Ch = 0xFFFD;
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
            _Ch          = (b1 << 12) | (b2 << 6) | b3;
        }

        return 3;
    }
    if (__utf8_is_4b(s))
    {
        _Ch = 0xFFFD;
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

            _Ch = c;
        }
        return 4;
    }
    _Ch = 0;
    return 0;
}

int WideCharDecodeUTF8(const char* _Unicode, const char* _UnicodeEnd, SWideChar& _Ch)
{
    HK_ASSERT(_UnicodeEnd != nullptr);
    const unsigned char* s = (const unsigned char*)_Unicode;
    const unsigned char* e = (const unsigned char*)_UnicodeEnd;
    if (__utf8_is_1b(s))
    {
        _Ch = *s;
        return 1;
    }
    if (__utf8_is_2b(s))
    {
        _Ch = 0xFFFD;
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
            _Ch          = (b1 << 6) | b2;
        }
        return 2;
    }
    if (__utf8_is_3b(s))
    {
        _Ch = 0xFFFD;
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
            _Ch          = (b1 << 12) | (b2 << 6) | b3;
        }

        return 3;
    }
    if (__utf8_is_4b(s))
    {
        _Ch = 0xFFFD;
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

            _Ch = c;
        }
        return 4;
    }
    _Ch = 0;
    return 0;
}
#endif

int WideCharDecodeUTF8(const char* _Unicode, SWideChar& _Ch)
{
    return WideCharDecodeUTF8(_Unicode, nullptr, _Ch);
}

// Convert UTF-8 to 32-bit character, process single character input.
// A nearly-branchless UTF-8 decoder, based on work of Christopher Wellons (https://github.com/skeeto/branchless-utf8).
// We handle UTF-8 decoding error by skipping forward.
int WideCharDecodeUTF8(const char* _Unicode, const char* _UnicodeEnd, SWideChar& _Ch)
{
    static const char lengths[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
    static const int masks[]  = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
    static const uint32_t mins[] = { 0x400000, 0, 0x80, 0x800, 0x10000 };
    static const int shiftc[] = { 0, 18, 12, 6, 0 };
    static const int shifte[] = { 0, 6, 4, 2, 0 };
    int len = lengths[*(const unsigned char*)_Unicode >> 3];
    int wanted = len + !len;

    if (_UnicodeEnd == nullptr)
        _UnicodeEnd = _Unicode + wanted; // Max length, nulls will be taken into account.

    // Copy at most 'len' bytes, stop copying at 0 or past _UnicodeEnd. Branch predictor does a good job here,
    // so it is fast even with excessive branching.
    unsigned char s[4];
    s[0] = _Unicode + 0 < _UnicodeEnd ? _Unicode[0] : 0;
    s[1] = _Unicode + 1 < _UnicodeEnd ? _Unicode[1] : 0;
    s[2] = _Unicode + 2 < _UnicodeEnd ? _Unicode[2] : 0;
    s[3] = _Unicode + 3 < _UnicodeEnd ? _Unicode[3] : 0;

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
        // No bytes are consumed when *_Unicode == 0 || _Unicode == _UnicodeEnd.
        // One byte is consumed in case of invalid first byte of _Unicode.
        // All available bytes (at most `len` bytes) are consumed on incomplete/invalid second to last bytes.
        // Invalid or incomplete input may consume less bytes than wanted, therefore every byte has to be inspected in s.
        wanted = std::min(wanted, !!s[0] + !!s[1] + !!s[2] + !!s[3]);
        c = 0xFFFD;     // Invalid Unicode code point (standard value)
    }

    _Ch = c;

    return wanted;
}

int WideStrDecodeUTF8(const char* _Unicode, SWideChar* _Str, int _MaxLength)
{
    if (_MaxLength <= 0)
    {
        return 0;
    }
    SWideChar* pout = _Str;
    while (*_Unicode && --_MaxLength > 0)
    {
        const int byteLen = WideCharDecodeUTF8(_Unicode, *_Str);
        if (!byteLen)
        {
            break;
        }
        _Unicode += byteLen;
        _Str++;
    }
    *_Str = 0;
    return _Str - pout;
}

int WideStrDecodeUTF8(const char* _Unicode, const char* _UnicodeEnd, SWideChar* _Str, int _MaxLength)
{
    if (_MaxLength <= 0)
    {
        return 0;
    }
    SWideChar* pout = _Str;
    while (*_Unicode && --_MaxLength > 0)
    {
        const int byteLen = WideCharDecodeUTF8(_Unicode, _UnicodeEnd, *_Str);
        if (!byteLen)
        {
            break;
        }
        _Unicode += byteLen;
        _Str++;
    }
    *_Str = 0;
    return _Str - pout;
}

int WideStrUTF8Bytes(SWideChar const* _Str, SWideChar const* _StrEnd)
{
    int byteLen = 0;
    while ((!_StrEnd || _Str < _StrEnd) && *_Str)
    {
        byteLen += WideCharUTF8Bytes(*_Str++);
    }
    return byteLen;
}

int WideStrLength(SWideChar const* _Str)
{
    SWideChar const* p = _Str;
    while (*p) { p++; }
    return p - _Str;
}

int WideCharEncodeUTF8(char* _Buf, int _BufSize, unsigned int _Ch)
{
    if (_Ch < 0x80)
    {
        _Buf[0] = (char)_Ch;
        return 1;
    }
    if (_Ch < 0x800)
    {
        if (_BufSize < 2) return 0;
        _Buf[0] = (char)(0xc0 + (_Ch >> 6));
        _Buf[1] = (char)(0x80 + (_Ch & 0x3f));
        return 2;
    }
    if (_Ch >= 0xdc00 && _Ch < 0xe000)
    {
        return 0;
    }
    if (_Ch >= 0xd800 && _Ch < 0xdc00)
    {
        if (_BufSize < 4) return 0;
        _Buf[0] = (char)(0xf0 + (_Ch >> 18));
        _Buf[1] = (char)(0x80 + ((_Ch >> 12) & 0x3f));
        _Buf[2] = (char)(0x80 + ((_Ch >> 6) & 0x3f));
        _Buf[3] = (char)(0x80 + ((_Ch)&0x3f));
        return 4;
    }
    //else if ( c < 0x10000 )
    {
        if (_BufSize < 3) return 0;
        _Buf[0] = (char)(0xe0 + (_Ch >> 12));
        _Buf[1] = (char)(0x80 + ((_Ch >> 6) & 0x3f));
        _Buf[2] = (char)(0x80 + ((_Ch)&0x3f));
        return 3;
    }
}

int WideStrEncodeUTF8(char* _Buf, int _BufSize, SWideChar const* _Str, SWideChar const* _StrEnd)
{
    if (_BufSize <= 0)
    {
        return 0;
    }
    char*       pBuf = _Buf;
    const char* pEnd = _Buf + _BufSize - 1;
    while (pBuf < pEnd && (!_StrEnd || _Str < _StrEnd) && *_Str)
    {
        SWideChar ch = *_Str++;
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
    return (int)(pBuf - _Buf);
}

} // namespace Core
