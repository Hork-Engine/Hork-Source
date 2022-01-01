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

#pragma once

#include <Platform/Public/BaseTypes.h>

namespace Core
{

/** Robert Sedgwicks Algorithms in C book */
AN_FORCEINLINE uint32_t RSHash(const char* _Str, int _Length)
{
    const uint32_t b    = 378551;
    uint32_t       a    = 63689;
    uint32_t       hash = 0;

    for (int i = 0; i < _Length; i++)
    {
        hash = hash * a + _Str[i];
        a    = a * b;
    }

    return hash;
}

/** Justin Sobels bitwise hash function */
AN_FORCEINLINE uint32_t JSHash(const char* _Str, int _Length)
{
    uint32_t hash = 1315423911;

    for (int i = 0; i < _Length; i++)
    {
        hash ^= ((hash << 5) + _Str[i] + (hash >> 2));
    }

    return hash;
}

/** Peter J. Weinberger hash algorithm */
AN_FORCEINLINE uint32_t PJWHash(const char* _Str, int _Length)
{
    constexpr uint32_t BitsInUnsignedInt = sizeof(uint32_t) * 8;
    constexpr uint32_t ThreeQuarters     = (BitsInUnsignedInt * 3) / 4;
    constexpr uint32_t OneEighth         = BitsInUnsignedInt / 8;
    constexpr uint32_t HighBits          = uint32_t(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
    uint32_t           hash              = 0;
    uint32_t           test              = 0;

    for (int i = 0; i < _Length; i++)
    {
        hash = (hash << OneEighth) + _Str[i];

        if ((test = hash & HighBits) != 0)
        {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }

    return hash;
}

/** Similar to the PJW Hash function, but tweaked for 32-bit processors. Its the hash function widely used on most UNIX systems. */
AN_FORCEINLINE uint32_t ELFHash(const char* _Str, int _Length)
{
    uint32_t hash = 0;
    uint32_t x    = 0;

    for (int i = 0; i < _Length; i++)
    {
        hash = (hash << 4) + _Str[i];
        if ((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }

    return hash;
}

/** Brian Kernighan and Dennis Ritchie's (Book "The C Programming Language") */
AN_FORCEINLINE uint32_t BKDRHash(const char* _Str, int _Length)
{
    const uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
    uint32_t       hash = 0;

    for (int i = 0; i < _Length; i++)
    {
        hash = (hash * seed) + _Str[i];
    }

    return hash;
}

/** Algorithm is used in the open source SDBM project */
AN_FORCEINLINE uint32_t SDBMHash(const char* _Str, int _Length)
{
    uint32_t hash = 0;

    for (int i = 0; i < _Length; i++)
    {
        hash = _Str[i] + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

/** Algorithm produced by Professor Daniel J. Bernstein */
AN_FORCEINLINE uint32_t DJBHash(const char* _Str, int _Length)
{
    uint32_t hash = 5381;

    for (int i = 0; i < _Length; i++)
    {
        hash = ((hash << 5) + hash) + _Str[i];
    }

    return hash;
}

/** Donald E. Knuth algorithm (The Art Of Computer Programming Volume 3) */
AN_FORCEINLINE uint32_t DEKHash(const char* _Str, int _Length)
{
    uint32_t hash = static_cast<uint32_t>(_Length);

    for (int i = 0; i < _Length; i++)
    {
        hash = ((hash << 5) ^ (hash >> 27)) ^ _Str[i];
    }

    return hash;
}

/** Arash Partow algorithm */
AN_FORCEINLINE uint32_t APHash(const char* _Str, int _Length)
{
    uint32_t hash = 0;

    for (int i = 0; i < _Length; i++)
    {
        hash ^= ((i & 1) == 0) ? ((hash << 7) ^ _Str[i] ^ (hash >> 3)) : (~((hash << 11) ^ _Str[i] ^ (hash >> 5)));
    }

    return hash;
}

/** Paul Hsieh hash http://www.azillionmonkeys.com/qed/hash.html */
#undef __get16
#define __get16(p) ((p)[0] | ((p)[1] << 8))
AN_FORCEINLINE uint32_t PHHash(const char* _Str, int _Length, uint32_t hash = 0)
{
    uint32_t tmp;
    int      rem;

    rem = _Length & 3;
    _Length >>= 2;

    /* Main loop */
    for (; _Length > 0; _Length--)
    {
        hash += __get16(_Str);
        tmp  = (__get16(_Str + 2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        _Str += 4;
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch (rem)
    {
        case 3:
            hash += __get16(_Str);
            hash ^= hash << 16;
            hash ^= _Str[2] << 18;
            hash += hash >> 11;
            break;
        case 2:
            hash += __get16(_Str);
            hash ^= hash << 11;
            hash += hash >> 17;
            break;
        case 1:
            hash += *_Str;
            hash ^= hash << 10;
            hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

/** Modified version PHHash for case-insensetive strings */
AN_FORCEINLINE uint32_t PHHash_Case(const char* _Str, int _Length, uint32_t hash = 0)
{
    uint32_t tmp;
    int      rem;
    char     datatmp[4];

    rem = _Length & 3;
    _Length >>= 2;

    /* Main loop */
    for (; _Length > 0; _Length--)
    {
        datatmp[0] = _Str[0] >= 'A' && _Str[0] <= 'Z' ? _Str[0] - 'A' + 'a' : _Str[0];
        datatmp[1] = _Str[1] >= 'A' && _Str[1] <= 'Z' ? _Str[1] - 'A' + 'a' : _Str[1];
        datatmp[2] = _Str[2] >= 'A' && _Str[2] <= 'Z' ? _Str[2] - 'A' + 'a' : _Str[2];
        datatmp[3] = _Str[3] >= 'A' && _Str[3] <= 'Z' ? _Str[3] - 'A' + 'a' : _Str[3];
        hash += __get16(datatmp);
        tmp  = (__get16(&datatmp[2]) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        _Str += 4;
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch (rem)
    {
        case 3:
            datatmp[0] = _Str[0] >= 'A' && _Str[0] <= 'Z' ? _Str[0] - 'A' + 'a' : _Str[0];
            datatmp[1] = _Str[1] >= 'A' && _Str[1] <= 'Z' ? _Str[1] - 'A' + 'a' : _Str[1];
            datatmp[2] = _Str[2] >= 'A' && _Str[2] <= 'Z' ? _Str[2] - 'A' + 'a' : _Str[2];

            hash += __get16(datatmp);
            hash ^= hash << 16;
            hash ^= datatmp[2] << 18;
            hash += hash >> 11;
            break;
        case 2:
            datatmp[0] = _Str[0] >= 'A' && _Str[0] <= 'Z' ? _Str[0] - 'A' + 'a' : _Str[0];
            datatmp[1] = _Str[1] >= 'A' && _Str[1] <= 'Z' ? _Str[1] - 'A' + 'a' : _Str[1];

            hash += __get16(datatmp);
            hash ^= hash << 11;
            hash += hash >> 17;
            break;
        case 1:
            hash += _Str[0] >= 'A' && _Str[0] <= 'Z' ? _Str[0] - 'A' + 'a' : _Str[0];
            hash ^= hash << 10;
            hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
#undef __get16

/** Modified version PHHash for 32-bit integers */
AN_FORCEINLINE uint32_t PHHash32(uint32_t p, uint32_t hash = 0)
{
    hash += (p >> 24) | (((p >> 16) & 0xff) << 8);
    hash = (hash << 16) ^ uint32_t(((((p >> 8) & 0xff) | (((p)&0xff) << 8)) << 11) ^ hash);
    hash += hash >> 11;
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
    return hash;
}

/** Modified version PHHash for 64-bit integers */
AN_FORCEINLINE uint32_t PHHash64(uint64_t p, uint32_t hash = 0)
{
    hash += (p >> 56) | (((p >> 48) & 0xff) << 8);
    hash = (hash << 16) ^ uint32_t(((((p >> 40) & 0xff) | (((p >> 32) & 0xff) << 8)) << 11) ^ hash);
    hash += hash >> 11;
    hash += ((p >> 24) & 0xff) | (((p >> 16) & 0xff) << 8);
    hash = (hash << 16) ^ uint32_t(((((p >> 8) & 0xff) | (((p)&0xff) << 8)) << 11) ^ hash);
    hash += hash >> 11;
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
    return hash;
}

/** Modified version MurMur3 hash for 32-bit integers */
AN_FORCEINLINE uint32_t MurMur3Hash32(uint32_t k, uint32_t seed)
{
    uint32_t h = seed;
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    h ^= k;
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
    h ^= 4;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

/** Modified version MurMur3 hash for 64-bit integers */
AN_FORCEINLINE uint32_t MurMur3Hash64(uint64_t key, uint32_t seed)
{
    uint32_t h = seed;
    uint32_t k = key >> 32;
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    h ^= k;
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
    k = key & 0xffffffff;
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    h ^= k;
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
    h ^= 8;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

AN_FORCEINLINE int Hash(const char* _Str, int _Length)
{
    return PHHash(_Str, _Length);
}

AN_FORCEINLINE int HashCase(const char* _Str, int _Length)
{
    return PHHash_Case(_Str, _Length);
}

} // namespace Core
