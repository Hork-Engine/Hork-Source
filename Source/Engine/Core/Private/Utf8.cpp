/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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

namespace FCore {

int DecodeUTF1b( const byte * s ) {
    // ASCII, в том числе латинский алфавит, простейшие знаки препинания и арабские цифры

    return *s;
}

int DecodeUTF2b( const byte * s ) {
    // кириллица, расширенная латиница, арабский, армянский, греческий, еврейский и коптский алфавит; сирийское письмо, тана, нко; МФА; некоторые знаки препинания

    int b1 = *s & 0x1F;
    int b2 = *(s+1) & 0x3F;

    return ( b1 << 6 ) | b2;
}

int DecodeUTF3b( const byte * s ) {
    // все другие современные формы письменности, в том числе грузинский алфавит, индийское, китайское, корейское и японское письмо; сложные знаки препинания; математические и другие специальные символы

    int b1 = *s & 0x0F;
    int b2 = *(s+1) & 0x3F;
    int b3 = *(s+2) & 0x3F;

    return ( b1 << 12 ) | ( b2 << 6 ) | b3;
}

int DecodeUTF4b( const byte * s ) {
    // музыкальные символы, редкие китайские иероглифы, вымершие формы письменности

    int b1 = *s & 0x07;
    int b2 = *(s+1) & 0x3F;
    int b3 = *(s+2) & 0x3F;
    int b4 = *(s+3) & 0x3F;

    return ( b1 << 18 ) | ( b2 << 12 ) | ( b3 << 6 ) | b4;
}

int DecodeUTF5b( const byte * s ) {
    // не используется в Unicode

    int b1 = *s & 0x03;
    int b2 = *(s+1) & 0x3F;
    int b3 = *(s+2) & 0x3F;
    int b4 = *(s+3) & 0x3F;
    int b5 = *(s+4) & 0x3F;

    return ( b1 << 24 ) | ( b2 << 18 ) | ( b3 << 12 ) | ( b4 << 6 ) | b5;
}

int DecodeUTF6b( const byte * s ) {
    // не используется в Unicode

    int b1 = *s & 0x01;
    int b2 = *(s+1) & 0x3F;
    int b3 = *(s+2) & 0x3F;
    int b4 = *(s+3) & 0x3F;
    int b5 = *(s+4) & 0x3F;
    int b6 = *(s+5) & 0x3F;

    return ( b1 << 30 ) | ( b2 << 24 ) | ( b3 << 18 ) | ( b4 << 12 ) | ( b5 << 6 ) | b6;
}

int GetUTF8CharacterByteLength( const byte * s ) {
    int byteLen = 0;
    for ( int bit = 7 ; bit >= 2 ; bit-- ) {
        if ( !(*s & AN_BIT( bit )) ) {
            byteLen = 7 - bit;
            if ( byteLen < 1 ) byteLen = 1;
            break;
        }
    }
    return byteLen;
}

int GetUTF8StrLength( const byte * s ) {
    int strLength = 0;
    while ( *s ) {
        int byteLen = GetUTF8CharacterByteLength( s );
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

int DecodeUTF8Char( const byte * s, int & ch ) {
    int byteLen = GetUTF8CharacterByteLength( s );

    switch ( byteLen ) {
        case 1: ch = DecodeUTF1b( s ); break;
        case 2: ch = DecodeUTF2b( s ); break;
        case 3: ch = DecodeUTF3b( s ); break;
        case 4: ch = DecodeUTF4b( s ); break;
        case 5: ch = DecodeUTF5b( s ); break;
        case 6: ch = DecodeUTF6b( s ); break;
        default: /*Not UTF-8*/ break;
    }

    return byteLen;
}

int DecodeUTF8Str( const byte * s, int * out ) {
    int * pout = out;
    while ( *s ) {
        int byteLen = DecodeUTF8Char( s, *out );
        if ( byteLen > 0 ) {
            s += byteLen;
        }
        else {
            break;
        }
        out++;
    }
    *out = 0;
    return out - pout;
}

}

