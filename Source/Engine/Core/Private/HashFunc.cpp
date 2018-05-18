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

#include <Engine/Core/Public/HashFunc.h>

namespace FCore {

unsigned int RSHash( const char * _Str, int _Length ) {
    const unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;

    for ( int i = 0; i < _Length; i++ ) {
        hash = hash * a + _Str[ i ];
        a = a * b;
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int JSHash( const char * _Str, int _Length ) {
    unsigned int hash = 1315423911;

    for ( int i = 0; i < _Length; i++ ) {
        hash ^= ( ( hash << 5 ) + _Str[ i ] + ( hash >> 2 ) );
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int PJWHash( const char * _Str, int _Length ) {
    const unsigned int BitsInUnsignedInt = ( unsigned int )( sizeof( unsigned int )* 8 );
    const unsigned int ThreeQuarters = ( unsigned int )( ( BitsInUnsignedInt * 3 ) / 4 );
    const unsigned int OneEighth = ( unsigned int )( BitsInUnsignedInt / 8 );
    const unsigned int HighBits = ( unsigned int )( 0xFFFFFFFF ) << ( BitsInUnsignedInt - OneEighth );
    unsigned int hash = 0;
    unsigned int test = 0;

    for ( int i = 0; i < _Length; i++ ) {
        hash = ( hash << OneEighth ) + _Str[ i ];

        if ( ( test = hash & HighBits ) != 0 ) {
            hash = ( ( hash ^ ( test >> ThreeQuarters ) ) & ( ~HighBits ) );
        }
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int ELFHash( const char * _Str, int _Length ) {
    unsigned int hash = 0;
    unsigned int x = 0;

    for ( int i = 0; i < _Length; i++ ) {
        hash = ( hash << 4 ) + _Str[ i ];
        if ( ( x = hash & 0xF0000000L ) != 0 ) {
            hash ^= ( x >> 24 );
            hash &= ~x;
        }
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int BKDRHash( const char * _Str, int _Length ) {
    const unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    for ( int i = 0; i < _Length; i++ ) {
        hash = ( hash * seed ) + _Str[ i ];
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int SDBMHash( const char * _Str, int _Length ) {
    unsigned int hash = 0;

    for ( int i = 0; i < _Length; i++ ) {
        hash = _Str[ i ] + ( hash << 6 ) + ( hash << 16 ) - hash;
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int DJBHash( const char * _Str, int _Length ) {
    unsigned int hash = 5381;

    for ( int i = 0; i < _Length; i++ ) {
        hash = ( ( hash << 5 ) + hash ) + _Str[ i ];
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int DEKHash( const char * _Str, int _Length ) {
    unsigned int hash = static_cast< unsigned int >( _Length );

    for ( int i = 0; i < _Length; i++ ) {
        hash = ( ( hash << 5 ) ^ ( hash >> 27 ) ) ^ _Str[ i ];
    }

    return ( hash & 0x7FFFFFFF );
}


unsigned int APHash( const char * _Str, int _Length ) {
    unsigned int hash = 0;

    for ( int i = 0; i < _Length; i++ ) {
        hash ^= ( ( i & 1 ) == 0 ) ? ( ( hash << 7 ) ^ _Str[ i ] ^ ( hash >> 3 ) ) : ( ~( ( hash << 11 ) ^ _Str[ i ] ^ ( hash >> 5 ) ) );
    }

    return ( hash & 0x7FFFFFFF );
}

}

