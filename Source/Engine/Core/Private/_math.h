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

#include <Engine/Core/Public/Integer.h>

template< typename integral_type >
AN_FORCEINLINE integral_type StringToInt( const char * _String ) {
    uint64_t val;
    uint64_t sign;
    char c;

    constexpr int integral_type_size = sizeof( integral_type );
    constexpr uint64_t sign_bit = 1 << ( integral_type_size - 1 );

    if ( *_String == '-' ) {
        sign = sign_bit;
        _String++;
    }
    else {
        sign = 0;
    }

    val = 0;

    // check for hex
    if ( _String[0] == '0' && (_String[1] == 'x' || _String[1] == 'X') ) {
        _String += 2;
        while ( 1 ) {
            c = *_String++;
            if ( c >= '0' && c <= '9' ) {
                val = (val<<4) + c - '0';
            }
            else if ( c >= 'a' && c <= 'f' ) {
                val = (val<<4) + c - 'a' + 10;
            }
            else if ( c >= 'A' && c <= 'F' ) {
                val = (val<<4) + c - 'A' + 10;
            }
            else {
                return val|sign;
            }
        }
    }

    // check for character
    if ( _String[0] == '\'' ) {
        return sign|_String[1];
    }

    while ( 1 ) {
        c = *_String++;
        if ( c < '0' || c > '9' ) {
            return val|sign;
        }
        val = val*10 + c - '0';
    }

    return 0;
}

template< typename real_type >
AN_FORCEINLINE real_type StringToReal( const char * _String ) {
    double val;
    int sign;
    char c;
    int decimal, total;

    if ( *_String == '-' ) {
        sign = -1;
        _String++;
    }
    else {
        sign = 1;
    }

    val = 0;

    // check for hex
    if ( _String[0] == '0' && (_String[1] == 'x' || _String[1] == 'X') ) {
        _String += 2;
        while ( 1 ) {
            c = *_String++;
            if ( c >= '0' && c <= '9' ) {
                val = (val*16) + c - '0';
            }
            else if ( c >= 'a' && c <= 'f' ) {
                val = (val*16) + c - 'a' + 10;
            }
            else if ( c >= 'A' && c <= 'F' ) {
                val = (val*16) + c - 'A' + 10;
            }
            else {
                return val * sign;
            }
        }
    }

    // check for character
    if ( _String[0] == '\'' ) {
        return sign * _String[1];
    }

    decimal = -1;
    total = 0;
    while ( 1 ) {
        c = *_String++;
        if ( c == '.' ) {
            decimal = total;
            continue;
        }
        if ( c < '0' || c > '9' ) {
            break;
        }
        val = val*10 + c - '0';
        total++;
    }

    if ( decimal == -1 ) {
        return val * sign;
    }
    while ( total > decimal ) {
        val /= 10;
        total--;
    }

    return val * sign;
}
