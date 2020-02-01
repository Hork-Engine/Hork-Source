/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

namespace Core {

AN_FORCEINLINE constexpr bool IsLittleEndian() {
#ifdef AN_LITTLE_ENDIAN
    return true;
#else
    return false;
#endif
}

AN_FORCEINLINE constexpr bool IsBigEndian() {
    return !IsLittleEndian();
}

AN_FORCEINLINE constexpr word WordSwap( word const & _Val ) {
    return ((_Val & 0xff) << 8) | ((_Val >> 8) & 0xff);
}

AN_FORCEINLINE constexpr dword DWordSwap( dword const & _Val ) {
    return ((_Val & 0xff) << 24) | (((_Val >> 8) & 0xff) << 16) | (((_Val >> 16) & 0xff) << 8) | ((_Val >> 24) & 0xff);
}

AN_FORCEINLINE constexpr ddword DDWordSwap( ddword const & _Val ) {
    return (((_Val) & 0xff) << 56)
        | (((_Val >> 8) & 0xff) << 48)
        | (((_Val >> 16) & 0xff) << 40)
        | (((_Val >> 24) & 0xff) << 32)
        | (((_Val >> 32) & 0xff) << 24)
        | (((_Val >> 40) & 0xff) << 16)
        | (((_Val >> 48) & 0xff) << 8)
        | (((_Val >> 56) & 0xff));
}

AN_FORCEINLINE float FloatSwap( float const & _Val ) {
    union {
        float f;
        byte  b[4];
    } dat1, dat2;

    dat1.f = _Val;
    dat2.b[0] = dat1.b[3];
    dat2.b[1] = dat1.b[2];
    dat2.b[2] = dat1.b[1];
    dat2.b[3] = dat1.b[0];
    return dat2.f;
}

AN_FORCEINLINE double DoubleSwap( double const & _Val ) {
    union {
        double f;
        byte   b[8];
    } dat1, dat2;

    dat1.f = _Val;
    dat2.b[0] = dat1.b[7];
    dat2.b[1] = dat1.b[6];
    dat2.b[2] = dat1.b[5];
    dat2.b[3] = dat1.b[4];
    dat2.b[4] = dat1.b[3];
    dat2.b[5] = dat1.b[2];
    dat2.b[6] = dat1.b[1];
    dat2.b[7] = dat1.b[0];
    return dat2.f;
}

AN_FORCEINLINE void BlockSwap( void * _Bytes, int _ElementSz, int _Count ) {
    AN_ASSERT_( _ElementSz > 0, "BlockSwap" );
    if ( _ElementSz == 1 ) {
        return;
    }
    int half = _ElementSz >> 1;
    byte * block = (byte *)_Bytes;
    byte tmp;
    while ( _Count-- ) {
        byte *b1 = block;
        byte *b2 = block + _ElementSz - 1;
        while ( b1<&block[half] )
            tmp = *b1, *b1 = *b2--, *b1++ = tmp;
        block += _ElementSz;
    }
}

AN_FORCEINLINE constexpr word BigWord( word const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return WordSwap( _Val );
#else
    return _Val;
#endif
}

AN_FORCEINLINE constexpr dword BigDWord( dword const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return DWordSwap( _Val );
#else
    return _Val;
#endif
}

AN_FORCEINLINE constexpr ddword BigDDWord( ddword const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return DDWordSwap( _Val );
#else
    return _Val;
#endif
}

AN_FORCEINLINE float BigFloat( float const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return FloatSwap( _Val );
#else
    return _Val;
#endif
}

AN_FORCEINLINE double BigDouble( double const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return DoubleSwap( _Val );
#else
    return _Val;
#endif
}

AN_FORCEINLINE void BigBlock( void * _Bytes, int _ElementSz, int _Count ) {
#ifdef AN_LITTLE_ENDIAN
    BlockSwap( _Bytes, _ElementSz, _Count );
#endif
}

AN_FORCEINLINE constexpr word LittleWord( word const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return _Val;
#else
    return WordSwap( _Val );
#endif
}

AN_FORCEINLINE constexpr dword LittleDWord( dword const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return _Val;
#else
    return DWordSwap( _Val );
#endif
}

AN_FORCEINLINE constexpr ddword LittleDDWord( ddword const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return _Val;
#else
    return DDWordSwap( _Val );
#endif
}

AN_FORCEINLINE float LittleFloat( float const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return _Val;
#else
    return FloatSwap( _Val );
#endif
}

AN_FORCEINLINE double LittleDouble( double const & _Val ) {
#ifdef AN_LITTLE_ENDIAN
    return _Val;
#else
    return DoubleSwap( _Val );
#endif
}

AN_FORCEINLINE void LittleBlock( void * _Bytes, int _ElementSz, int _Count ) {
#ifndef AN_LITTLE_ENDIAN
    BlockSwap( _Bytes, _ElementSz, _Count );
#endif
}

} // namespace Core
