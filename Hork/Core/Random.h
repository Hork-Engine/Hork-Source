/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#pragma once

#include "BaseMath.h"

HK_NAMESPACE_BEGIN

/**
Interface of the random number generators.
*/
template <typename Derived>
class Random
{
public:
    virtual ~Random() {}

    /// Get a random number on [0, max] interval.
    uint32_t Get(uint32_t max)
    {
        if (max == 0)
        {
            return 0;
        }
        uint32_t n;
        uint32_t np2 = Math::ToGreaterPowerOfTwo(max + 1);
        do {
            n = static_cast<Derived*>(this)->Get() & (np2 - 1);
        } while (n > max);
        return n;
    }

    /// Random number on [0.0, 1.0] interval.
    float GetFloat()
    {
        union
        {
            uint32_t i;
            float    f;
        } pun;

        pun.i = 0x3f800000UL | (static_cast<Derived*>(this)->Get() & 0x007fffffUL);
        return pun.f - 1.0f;
    }

    float GetFloat(float min, float max)
    {
        return GetFloat() * (max - min) + min;
    }

    /// Get the max value of the random number.
    uint32_t MaxRandomValue() const { return 4294967295U; }
};


/**
Very simple random number generator with low storage requirements.
*/
class SimpleRand : public Random<SimpleRand>
{
public:
    /// Constructor that uses the given seed.
    SimpleRand(uint32_t seed = 0)
    {
        Seed(seed);
    }

    void Seed(uint32_t seed)
    {
        m_Current = seed;
    }

    /// Get a random number
    uint32_t Get()
    {
        return m_Current = m_Current * 1103515245 + 12345;
    }

private:
    uint32_t m_Current;
};


/**
Mersenne twister random number generator.
*/
class MersenneTwisterRand : public Random<MersenneTwisterRand>
{
public:
    /// Constructor that uses the given seed.
    MersenneTwisterRand(uint32_t seed = 0)
    {
        Seed(seed);
    }

    void Seed(uint32_t seed)
    {
        Initialize(seed);
        Reload();
    }

    /// Get a random number between 0 - 65536.
    uint32_t Get()
    {
        // Pull a 32-bit integer from the generator state
        // Every other access function simply transforms the numbers extracted here
        if (m_Left == 0)
        {
            Reload();
        }
        m_Left--;

        uint32_t s1;
        s1 = *m_Next++;
        s1 ^= (s1 >> 11);
        s1 ^= (s1 << 7) & 0x9d2c5680U;
        s1 ^= (s1 << 15) & 0xefc60000U;
        return (s1 ^ (s1 >> 18));
    };

private:
    void Initialize(uint32_t seed);
    void Reload();

    uint32_t hiBit(uint32_t u) const { return u & 0x80000000U; }
    uint32_t loBit(uint32_t u) const { return u & 0x00000001U; }
    uint32_t loBits(uint32_t u) const { return u & 0x7fffffffU; }
    uint32_t mixBits(uint32_t u, uint32_t v) const { return hiBit(u) | loBits(v); }
    uint32_t twist(uint32_t m, uint32_t s0, uint32_t s1) const { return m ^ (mixBits(s0, s1) >> 1) ^ ((~loBit(s1) + 1) & 0x9908b0dfU); }

private:
    enum
    {
        N = 624
    }; // length of state vector
    enum
    {
        M = 397
    };

    uint32_t m_State[N]; // internal state
    uint32_t* m_Next;    // next value to get from state
    int m_Left;          // number of values left before reload needed
};

namespace Core
{

/// Get a random seed.
HK_FORCEINLINE uint32_t RandomSeed()
{
    return (uint32_t)time(NULL);
}

} // namespace Core

HK_NAMESPACE_END
