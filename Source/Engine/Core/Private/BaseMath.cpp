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

#include <Core/Public/BaseMath.h>

#ifdef AN_COMPILER_MSVC
#pragma warning( disable : 4756 ) // overlow
#endif

static float FloatToHalfOverflow() {
    volatile float f = 1e10;
    for ( int j = 0 ; j < 10 ; j++ ) {
        f *= f; // this will overflow before the for loop terminates
    }
    return f;
}

namespace Math {

//-----------------------------------------------------
// Float-to-half conversion -- general case, including
// zeroes, denormalized numbers and exponent overflows.
//-----------------------------------------------------
uint16_t _FloatToHalf( const uint32_t & _I ) {
    //
    // Our floating point number, f, is represented by the bit
    // pattern in integer _I.  Disassemble that bit pattern into
    // the sign, s, the exponent, e, and the significand, m.
    // Shift s into the position where it will go in in the
    // resulting half number.
    // Adjust e, accounting for the different exponent bias
    // of float and half (127 versus 15).
    //

    register int s =  (_I >> 16) & 0x00008000;
    register int e = ((_I >> 23) & 0x000000ff) - (127 - 15);
    register int m =   _I        & 0x007fffff;

    //
    // Now reassemble s, e and m into a half:
    //

    if (e <= 0)
    {
        if (e < -10)
        {
            //
            // E is less than -10.  The absolute value of f is
            // less than HALF_MIN (f may be a small normalized
            // float, a denormalized float or a zero).
            //
            // We convert f to a half zero.
            //

            return 0;
        }

        //
        // E is between -10 and 0.  F is a normalized float,
        // whose magnitude is less than HALF_NRM_MIN.
        //
        // We convert f to a denormalized half.
        //

        m = (m | 0x00800000) >> (1 - e);

        //
        // Round to nearest, round "0.5" up.
        //
        // Rounding may cause the significand to overflow and make
        // our number normalized.  Because of the way a half's bits
        // are laid out, we don't have to treat this case separately;
        // the code below will handle it correctly.
        //

        if (m &  0x00001000)
            m += 0x00002000;

        //
        // Assemble the half from s, e (zero) and m.
        //

        return s | (m >> 13);
    }
    else if (e == 0xff - (127 - 15))
    {
        if (m == 0)
        {
            //
            // F is an infinity; convert f to a half
            // infinity with the same sign as f.
            //

            return s | 0x7c00;
        }
        else
        {
            //
            // F is a NAN; we produce a half NAN that preserves
            // the sign bit and the 10 leftmost bits of the
            // significand of f, with one exception: If the 10
            // leftmost bits are all zero, the NAN would turn
            // into an infinity, so we have to set at least one
            // bit in the significand.
            //

            m >>= 13;
            return s | 0x7c00 | m | (m == 0);
        }
    }
    else
    {
        //
        // E is greater than zero.  F is a normalized float.
        // We try to convert f to a normalized half.
        //

        //
        // Round to nearest, round "0.5" up
        //

        if (m &  0x00001000)
        {
            m += 0x00002000;

            if (m & 0x00800000)
            {
                m =  0;		// overflow in significand,
                e += 1;		// adjust exponent
            }
        }

        //
        // Handle exponent overflow
        //

        if (e > 30)
        {
            FloatToHalfOverflow();	// Cause a hardware floating point overflow;
            return s | 0x7c00;	// if this returns, the half becomes an
        }   			// infinity with the same sign as f.

        //
        // Assemble the half from s, e and m.
        //

        return s | (e << 10) | (m >> 13);
    }
}

// Taken from OpenEXR
uint32_t _HalfToFloat( const uint16_t & _I ) {
    int s = (_I >> 15) & 0x00000001;
    int e = (_I >> 10) & 0x0000001f;
    int m =  _I        & 0x000003ff;

    if (e == 0)
    {
        if (m == 0)
        {
            //
            // Plus or minus zero
            //

            return s << 31;
        }
        else
        {
            //
            // Denormalized number -- renormalize it
            //

            while (!(m & 0x00000400))
            {
                m <<= 1;
                e -=  1;
            }

            e += 1;
            m &= ~0x00000400;
        }
    }
    else if (e == 31)
    {
        if (m == 0)
        {
            //
            // Positive or negative infinity
            //

            return (s << 31) | 0x7f800000;
        }
        else
        {
            //
            // Nan -- preserve sign and significand bits
            //

            return (s << 31) | 0x7f800000 | (m << 13);
        }
    }

    //
    // Normalized number
    //

    e = e + (127 - 15);
    m = m << 13;

    //
    // Assemble s, e and m.
    //

    return (s << 31) | (e << 23) | m;
}

void FloatToHalf( const float * _In, uint16_t * _Out, int _Count ) {
    const float *pIn = _In;
    while ( pIn < &_In[ _Count ] ) {
        *_Out++ = _FloatToHalf( *reinterpret_cast< const uint32_t * >( pIn++ ) );
    }
}

void HalfToFloat( const uint16_t * _In, float * _Out, int _Count ) {
    const unsigned short *pIn = _In;
    while ( pIn < &_In[ _Count ] ) {
        *reinterpret_cast< uint32_t * >( _Out++ ) = _HalfToFloat( *pIn++ );
    }
}

}
