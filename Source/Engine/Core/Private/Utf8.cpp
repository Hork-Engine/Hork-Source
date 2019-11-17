/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Engine/Core/Public/Utf8.h>

namespace  Core {

#define __utf8_is_1b(s)   (!(*(s) & 0x80))
#define __utf8_is_2b(s)   (( *(s) & 0xE0 ) == 0xC0)
#define __utf8_is_3b(s)   (( *(s) & 0xF0 ) == 0xE0)
#define __utf8_is_4b(s)   (( *(s) & 0xF8 ) == 0xF0)

int UTF8CharByteLength( const char * _Unicode ) {
    if ( __utf8_is_1b(_Unicode) ) {
        return 1;
    }
    if ( __utf8_is_2b(_Unicode) ) {
        if ( _Unicode[1] == 0 ) {
            return 1;
        }
        return 2;
    }
    if ( __utf8_is_3b(_Unicode) ) {
        if ( _Unicode[1] == 0 || _Unicode[2] == 0 ) {
            return 1;
        }
        return 3;
    }
    if ( __utf8_is_4b(_Unicode) ) {
        if ( _Unicode[1] == 0 || _Unicode[2] == 0 || _Unicode[3] == 0 ) {
            return 1;
        }
        return 4;
    }
    return 0;
}

int UTF8StrLength( const char * _Unicode ) {
    int strLength = 0;
    while ( *_Unicode ) {
        const int byteLen = UTF8CharByteLength( _Unicode );
        if ( byteLen > 0 ) {
            _Unicode += byteLen;
            strLength++;
        }
        else {
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

int WideCharDecodeUTF8( const char * _Unicode, FWideChar & _Ch ) {
    const unsigned char * s = (const unsigned char *)_Unicode;
    if ( __utf8_is_1b(s) ) {
        _Ch = *s;
        return 1;
    }
    if ( __utf8_is_2b(s) ) {
        _Ch = 0xFFFD;
        if ( s[1] == 0 ) {
            return 1;
        }
        if ( s[0] < 0xC2 ) {
            return 2;
        }
        if ( ( s[1] & 0xC0 ) == 0x80 ) {
            const int b1 = s[0] & 0x1F;
            const int b2 = s[1] & 0x3F;
            _Ch = ( b1 << 6 ) | b2;
        }
        return 2;
    }
    if ( __utf8_is_3b(s) ) {
        _Ch = 0xFFFD;
        if ( s[1] == 0 || s[2] == 0 ) {
            return 1;
        }
        if ( s[0] == 0xE0 && (s[1] < 0xA0 || s[1] > 0xBF) ) {
            return 3;
        }
        if ( s[0] == 0xED && s[1] > 0x9F ) { // s[1] < 0x80 is checked below
            return 3;
        }

        if ( ( s[1] & 0xC0 ) == 0x80
             && ( s[2] & 0xC0 ) == 0x80 ) {
            const int b1 = s[0] & 0x0F;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            _Ch = ( b1 << 12 ) | ( b2 << 6 ) | b3;
        }

        return 3;
    }
    if ( __utf8_is_4b(s) ) {
        _Ch = 0xFFFD;
        if ( s[1] == 0 || s[2] == 0 || s[3] == 0 ) {
            return 1;
        }
        if ( s[0] > 0xF4 ) {
            return 4;
        }
        if ( s[0] == 0xF0 && ( s[1] < 0x90 || s[1] > 0xBF ) ) {
            return 4;
        }
        if ( s[1] == 0xF4 && s[1] > 0x8F ) { // s[1] < 0x80 is checked below
            return 4;
        }

        if ( ( s[1] & 0xC0 ) == 0x80
             && ( s[2] & 0xC0 ) == 0x80
             && ( s[3] & 0xC0 ) == 0x80 ) {

            const int b1 = s[0] & 0x07;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            const int b4 = s[3] & 0x3F;

            const int c = ( b1 << 18 ) | ( b2 << 12 ) | ( b3 << 6 ) | b4;

            // utf-8 encodings of values used in surrogate pairs are invalid
            if ( (c & 0xFFFFF800) == 0xD800 || c > 0xFFFF ) {
                return 4;
            }

            _Ch = c;
        }
        return 4;
    }
    _Ch = 0;
    return 0;
}

int WideCharDecodeUTF8( const char * _Unicode, const char * _UnicodeEnd, FWideChar & _Ch ) {
    AN_Assert( _UnicodeEnd != nullptr );
    const unsigned char * s = (const unsigned char *)_Unicode;
    const unsigned char * e = (const unsigned char *)_UnicodeEnd;
    if ( __utf8_is_1b(s) ) {
        _Ch = *s;
        return 1;
    }
    if ( __utf8_is_2b(s) ) {
        _Ch = 0xFFFD;
        if ( e-s < 2 ) {
            return 1;
        }
        if ( s[0] < 0xC2 ) {
            return 2;
        }
        if ( ( s[1] & 0xC0 ) == 0x80 ) {
            const int b1 = s[0] & 0x1F;
            const int b2 = s[1] & 0x3F;
            _Ch = ( b1 << 6 ) | b2;
        }
        return 2;
    }
    if ( __utf8_is_3b(s) ) {
        _Ch = 0xFFFD;
        if ( e-s < 3 ) {
            return 1;
        }

        if ( s[0] == 0xE0 && (s[1] < 0xA0 || s[1] > 0xBF) ) {
            return 3;
        }
        if ( s[0] == 0xED && s[1] > 0x9F ) { // s[1] < 0x80 is checked below
            return 3;
        }

        if ( ( s[1] & 0xC0 ) == 0x80
             && ( s[2] & 0xC0 ) == 0x80 ) {
            const int b1 = s[0] & 0x0F;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            _Ch = ( b1 << 12 ) | ( b2 << 6 ) | b3;
        }

        return 3;
    }
    if ( __utf8_is_4b(s) ) {
        _Ch = 0xFFFD;
        if ( e-s < 4 ) {
            return 1;
        }

        if ( s[0] > 0xF4 ) {
            return 4;
        }
        if ( s[0] == 0xF0 && ( s[1] < 0x90 || s[1] > 0xBF ) ) {
            return 4;
        }
        if ( s[1] == 0xF4 && s[1] > 0x8F ) { // s[1] < 0x80 is checked below
            return 4;
        }

        if ( ( s[1] & 0xC0 ) == 0x80
             && ( s[2] & 0xC0 ) == 0x80
             && ( s[3] & 0xC0 ) == 0x80 ) {

            const int b1 = s[0] & 0x07;
            const int b2 = s[1] & 0x3F;
            const int b3 = s[2] & 0x3F;
            const int b4 = s[3] & 0x3F;

            const int c = ( b1 << 18 ) | ( b2 << 12 ) | ( b3 << 6 ) | b4;

            // utf-8 encodings of values used in surrogate pairs are invalid
            if ( (c & 0xFFFFF800) == 0xD800 || c > 0xFFFF ) {
                return 4;
            }

            _Ch = c;
        }
        return 4;
    }
    _Ch = 0;
    return 0;
}

int WideStrDecodeUTF8( const char * _Unicode, FWideChar * _Str, int _MaxLength ) {
    if ( _MaxLength <= 0 ) {
        return 0;
    }
    FWideChar * pout = _Str;
    while ( *_Unicode && --_MaxLength > 0 ) {
        const int byteLen = WideCharDecodeUTF8( _Unicode, *_Str );
        if ( !byteLen ) {
            break;
        }
        _Unicode += byteLen;
        _Str++;
    }
    *_Str = 0;
    return _Str - pout;
}

int WideStrDecodeUTF8( const char * _Unicode, const char * _UnicodeEnd, FWideChar * _Str, int _MaxLength ) {
    if ( _MaxLength <= 0 ) {
        return 0;
    }
    FWideChar * pout = _Str;
    while ( *_Unicode && --_MaxLength > 0 ) {
        const int byteLen = WideCharDecodeUTF8( _Unicode, _UnicodeEnd, *_Str );
        if ( !byteLen ) {
            break;
        }
        _Unicode += byteLen;
        _Str++;
    }
    *_Str = 0;
    return _Str - pout;
}

int WideStrUTF8Bytes( FWideChar const * _Str, FWideChar const * _StrEnd ) {
    int byteLen = 0;
    while ( (!_StrEnd || _Str < _StrEnd) && *_Str ) {
        byteLen += WideCharUTF8Bytes( *_Str++ );
    }
    return byteLen;
}

int WideStrLength( FWideChar const * _Str ) {
    FWideChar const * p = _Str;
    while ( *p ) { p++; }
    return p - _Str;
}

int WideCharEncodeUTF8( char * _Buf, int _BufSize, unsigned int _Ch ) {
    if ( _Ch < 0x80 ) {
        _Buf[0] = (char)_Ch;
        return 1;
    }
    if ( _Ch < 0x800 ) {
        if ( _BufSize < 2 ) return 0;
        _Buf[0] = (char)(0xc0 + (_Ch >> 6));
        _Buf[1] = (char)(0x80 + (_Ch & 0x3f));
        return 2;
    }
    if ( _Ch >= 0xdc00 && _Ch < 0xe000 ) {
        return 0;
    }
    if ( _Ch >= 0xd800 && _Ch < 0xdc00 ) {
        if (_BufSize < 4) return 0;
        _Buf[0] = (char)(0xf0 + (_Ch >> 18));
        _Buf[1] = (char)(0x80 + ((_Ch >> 12) & 0x3f));
        _Buf[2] = (char)(0x80 + ((_Ch >> 6) & 0x3f));
        _Buf[3] = (char)(0x80 + ((_Ch ) & 0x3f));
        return 4;
    }
    //else if ( c < 0x10000 )
    {
        if ( _BufSize < 3 ) return 0;
        _Buf[0] = (char)(0xe0 + (_Ch >> 12));
        _Buf[1] = (char)(0x80 + ((_Ch>> 6) & 0x3f));
        _Buf[2] = (char)(0x80 + ((_Ch ) & 0x3f));
        return 3;
    }
}

int WideStrEncodeUTF8( char * _Buf, int _BufSize, FWideChar const * _Str, FWideChar const * _StrEnd ) {
    if ( _BufSize <= 0 ) {
        return 0;
    }
    char * pBuf = _Buf;
    const char * pEnd = _Buf + _BufSize - 1;
    while ( pBuf < pEnd && ( !_StrEnd || _Str < _StrEnd ) && *_Str ) {
        FWideChar ch = *_Str++;
        if ( ch < 0x80 ) {
            *pBuf++ = (char)ch;
        } else {
            pBuf += WideCharEncodeUTF8( pBuf, (int)(pEnd-pBuf), ch );
        }
    }
    *pBuf = 0;
    return (int)(pBuf - _Buf);
}

}
