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

#include "BaseMath.h"

/**
Interface of the random number generators.
*/
template< typename Derived >
class ARandom
{
public:
    virtual ~ARandom() {}

    /** Get a random number on [0, max] interval. */
    unsigned int Get( unsigned int Max ) {
        if ( Max == 0 ) {
            return 0;
        }
        unsigned int n;
        unsigned int np2 = Math::ToGreaterPowerOfTwo( Max+1 );
        do {
            n = static_cast< Derived * >(this)->Get() & (np2-1);
        } while ( n > Max );
        return n;
    }

    /** Random number on [0.0, 1.0] interval. */
    float GetFloat() {
        union
        {
            uint32_t i;
            float f;
        } pun;

        pun.i = 0x3f800000UL | (static_cast< Derived * >(this)->Get() & 0x007fffffUL);
        return pun.f - 1.0f;
    }

    float GetFloat( float Min, float Max ) {
        return GetFloat() * (Max - Min) + Min;
    }

    /** Get the max value of the random number. */
    unsigned int MaxRandomValue() const { return 4294967295U; }
};


/**
Very simple random number generator with low storage requirements.
*/
class ASimpleRand : public ARandom< ASimpleRand >
{
public:
    /** Constructor that uses the given seed. */
    ASimpleRand( unsigned int InSeed = 0 )
    {
        Seed( InSeed );
    }

    void Seed( unsigned int InSeed )
    {
        Current = InSeed;
    }

    /** Get a random number */
    unsigned int Get()
    {
        return Current = Current * 1103515245 + 12345;
    }

private:
    unsigned int Current;
};


/**
Mersenne twister random number generator.
*/
class AMersenneTwisterRand : public ARandom< AMersenneTwisterRand >
{
public:
    /** Constructor that uses the given seed. */
    AMersenneTwisterRand( unsigned int InSeed = 0 )
    {
        Seed( InSeed );
    }

    void Seed( unsigned int InSeed )
    {
        Initialize( InSeed );
        Reload();
    }

    /** Get a random number between 0 - 65536. */
    unsigned int Get()
    {
        // Pull a 32-bit integer from the generator state
        // Every other access function simply transforms the numbers extracted here
        if ( Left == 0 ) {
            Reload();
        }
        Left--;

        unsigned int s1;
        s1 = *Next++;
        s1 ^= (s1 >> 11);
        s1 ^= (s1 <<  7) & 0x9d2c5680U;
        s1 ^= (s1 << 15) & 0xefc60000U;
        return (s1 ^ (s1 >> 18));
    };

private:
    void Initialize( uint32_t seed );
    void Reload();

    unsigned int hiBit( unsigned int u ) const { return u & 0x80000000U; }
    unsigned int loBit( unsigned int u ) const { return u & 0x00000001U; }
    unsigned int loBits( unsigned int u ) const { return u & 0x7fffffffU; }
    unsigned int mixBits( unsigned int u, unsigned int v ) const { return hiBit( u ) | loBits( v ); }
    unsigned int twist( unsigned int m, unsigned int s0, unsigned int s1 ) const { return m ^ (mixBits( s0, s1 )>>1) ^ ((~loBit( s1 )+1) & 0x9908b0dfU); }

private:
    enum { N = 624 };       // length of state vector
    enum { M = 397 };

    unsigned int State[N];	// internal state
    unsigned int * Next;	// next value to get from state
    int Left;		        // number of values left before reload needed		
};

namespace Core {

/** Get a random seed. */
AN_FORCEINLINE unsigned int RandomSeed() {
    return (unsigned int)time( NULL );
}

}
