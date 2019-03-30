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

#pragma once

#include "BaseTypes.h"

namespace FCore {

constexpr char FallbackCharacter = '?';

AN_FORCEINLINE int DecodeUTF1b( const char * s ) {
    // ASCII, в том числе латинский алфавит, простейшие знаки препинания и арабские цифры

    return *s;
}

AN_FORCEINLINE int DecodeUTF2b( const char * s ) {
    // кириллица, расширенная латиница, арабский, армянский, греческий, еврейский и коптский алфавит; сирийское письмо, тана, нко; МФА; некоторые знаки препинания

    if ( ( s[1] & 0xC0 ) != 0x80 ) {
        return 0xFFFD;
    }

    const int b1 = s[0] & 0x1F;
    const int b2 = s[1] & 0x3F;

    return ( b1 << 6 ) | b2;
}

AN_FORCEINLINE int DecodeUTF3b( const char * s ) {
    // все другие современные формы письменности, в том числе грузинский алфавит, индийское, китайское, корейское и японское письмо; сложные знаки препинания; математические и другие специальные символы

    if ( ( s[1] & 0xC0 ) != 0x80
         || ( s[2] & 0xC0 ) != 0x80 ) {
        return 0xFFFD;
    }

    const int b1 = s[0] & 0x0F;
    const int b2 = s[1] & 0x3F;
    const int b3 = s[2] & 0x3F;

    return ( b1 << 12 ) | ( b2 << 6 ) | b3;
}

AN_FORCEINLINE int DecodeUTF4b( const char * s ) {
    // музыкальные символы, редкие китайские иероглифы, вымершие формы письменности

    if ( ( s[1] & 0xC0 ) != 0x80
         || ( s[2] & 0xC0 ) != 0x80
         || ( s[3] & 0xC0 ) != 0x80 ) {
        return 0xFFFD;
    }

    const int b1 = s[0] & 0x07;
    const int b2 = s[1] & 0x3F;
    const int b3 = s[2] & 0x3F;
    const int b4 = s[3] & 0x3F;

    return ( b1 << 18 ) | ( b2 << 12 ) | ( b3 << 6 ) | b4;
}

#define __utf8_is_1b(s)   (!(*(s) & 0x80))
#define __utf8_is_2b(s)   (( *(s) & 0xE0 ) == 0xC0)
#define __utf8_is_3b(s)   (( *(s) & 0xF0 ) == 0xE0)
#define __utf8_is_4b(s)   (( *(s) & 0xF8 ) == 0xF0)
#define __utf8_bytes_count(s) (__utf8_is_1b(s) ? 1 : __utf8_is_2b(s) ? 2 : __utf8_is_3b(s) ? 3 : __utf8_is_4b(s) ? 4 : 0)

AN_FORCEINLINE int GetUTF8CharacterByteLength( const char * s ) {
    return __utf8_bytes_count(s);
#if 0
    if ( __is_1b(s) ) {
        return 1;
    }
    if ( __is_2b(s) ) {
        return 2;
    }
    if ( __is_3b(s) ) {
        return 3;
    }
    if ( __is_4b(s) ) {
        return 4;
    }
    return 0;
#endif
}

AN_FORCEINLINE int GetUTF8StrLength( const char * s ) {
    int strLength = 0;
    while ( *s ) {
        const int byteLen = __utf8_bytes_count( s );
        if ( byteLen > 0 ) {
            s += byteLen;
            strLength++;
        }
        else {
            break;
        }
    }
    return strLength;
}

AN_FORCEINLINE int DecodeUTF8Char( const char * s, int & ch ) {
    if ( __utf8_is_1b(s) ) {
        ch = DecodeUTF1b( s );
        return 1;
    }
    if ( __utf8_is_2b(s) ) {
        ch = DecodeUTF2b( s );
        return 2;
    }
    if ( __utf8_is_3b(s) ) {
        ch = DecodeUTF3b( s );
        return 3;
    }
    if ( __utf8_is_4b(s) ) {
        ch = DecodeUTF4b( s );
        return 4;
    }
    ch = 0;
    return 0;
}

AN_FORCEINLINE int DecodeUTF8WChar( const char * s, FWideChar & ch ) {
    if ( __utf8_is_1b(s) ) {
        ch = DecodeUTF1b( s );
        return 1;
    }
    if ( __utf8_is_2b(s) ) {
        ch = DecodeUTF2b( s );
        return 2;
    }
    if ( __utf8_is_3b(s) ) {
        ch = DecodeUTF3b( s );
        if ( ch > 0xffff ) {
            ch = FallbackCharacter;
        }
        return 3;
    }
    if ( __utf8_is_4b(s) ) {
        ch = FallbackCharacter;
        return 4;
    }
    ch = 0;
    return 0;
}

AN_FORCEINLINE int DecodeUTF8Str( const char * s, int * out ) {
    int * pout = out;
    while ( *s ) {
        const int byteLen = DecodeUTF8Char( s, *out );
        if ( !byteLen ) {
            break;
        }
        s += byteLen;
        out++;
    }
    *out = 0;
    return out - pout;
}

AN_FORCEINLINE int DecodeUTF8WStr( const char * s, FWideChar * out ) {
    FWideChar * pout = out;
    while ( *s ) {
        const int byteLen = DecodeUTF8WChar( s, *out );
        if ( !byteLen ) {
            break;
        }
        s += byteLen;
        out++;
    }
    *out = 0;
    return out - pout;
}

AN_FORCEINLINE int EncodeUTF8Char( int ch, char encoded[4] ) {
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
    encoded[0] = FallbackCharacter;
    return 1;
}

}
