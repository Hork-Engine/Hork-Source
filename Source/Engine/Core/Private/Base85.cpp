/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Core/Public/CString.h>
#include <Core/Public/Alloc.h>

static AN_FORCEINLINE unsigned int Decode85Byte( const char c ) {
    return c >= '\\' ? c - 36 : c - 35;
}

static AN_FORCEINLINE char Encode85Byte( unsigned int x ) {
    x = ( x % 85 ) + 35;
    return ( x >= '\\' ) ? x+1 : x;
}

namespace Core {

size_t DecodeBase85( byte const * _Base85, byte * _Dst ) {
    size_t size = ((Strlen( (const char *)_Base85 ) + 4) / 5) * 4;

    if ( _Dst ) {
        while ( *_Base85 ) {
            uint32_t d = 0;
        
            d += Decode85Byte( _Base85[4] );
            d *= 85;
            d += Decode85Byte( _Base85[3] );
            d *= 85;
            d += Decode85Byte( _Base85[2] );
            d *= 85;
            d += Decode85Byte( _Base85[1] );
            d *= 85;
            d += Decode85Byte( _Base85[0] );

            _Dst[0] = d & 0xff;
            _Dst[1] = (d >> 8) & 0xff;
            _Dst[2] = (d >> 16) & 0xff;
            _Dst[3] = (d >> 24) & 0xff;

            _Base85 += 5;
            _Dst += 4;
        }
    }

    return size;
}

size_t EncodeBase85( byte const * _Source, size_t _SourceSize, byte * _Base85 ) {
    size_t size = ( ( _SourceSize + 3 ) / 4 ) * 5 + 1;

    if ( _Base85 ) {
        size_t chunks = _SourceSize / 4;
        for ( size_t chunk = 0 ; chunk < chunks ; chunk++ ) {
            uint32_t d = ( ( uint32_t * )_Source )[ chunk ];

            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
        }
        int residual = _SourceSize - chunks * 4;
        if ( residual > 0 ) {
            uint32_t d = 0;

            Memcpy( &d, ( ( uint32_t * )_Source ) + chunks, residual );

            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
            *_Base85++ = Encode85Byte( d );
            d /= 85;
        }
        *_Base85 = 0;
    }

    return size;
}

}
