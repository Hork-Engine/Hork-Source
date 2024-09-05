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

#include "Memory.h"
#include "String.h"

HK_NAMESPACE_BEGIN

static HK_FORCEINLINE unsigned int Decode85Byte(const char c)
{
    return c >= '\\' ? c - 36 : c - 35;
}

static HK_FORCEINLINE char Encode85Byte(unsigned int x)
{
    x = (x % 85) + 35;
    return (x >= '\\') ? x + 1 : x;
}

namespace Core
{

size_t DecodeBase85(byte const* base85, byte* dst)
{
    size_t rsize = ((Core::Strlen((const char*)base85) + 4) / 5) * 4;

    if (dst)
    {
        while (*base85)
        {
            uint32_t d = 0;

            d += Decode85Byte(base85[4]);
            d *= 85;
            d += Decode85Byte(base85[3]);
            d *= 85;
            d += Decode85Byte(base85[2]);
            d *= 85;
            d += Decode85Byte(base85[1]);
            d *= 85;
            d += Decode85Byte(base85[0]);

            dst[0] = d & 0xff;
            dst[1] = (d >> 8) & 0xff;
            dst[2] = (d >> 16) & 0xff;
            dst[3] = (d >> 24) & 0xff;

            base85 += 5;
            dst += 4;
        }
    }

    return rsize;
}

size_t EncodeBase85(byte const* src, size_t size, byte* base85)
{
    size_t rsize = ((size + 3) / 4) * 5 + 1;

    if (base85)
    {
        size_t chunks = size / 4;
        for (size_t chunk = 0; chunk < chunks; chunk++)
        {
            uint32_t d = ((uint32_t*)src)[chunk];

            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
        }
        int residual = size - chunks * 4;
        if (residual > 0)
        {
            uint32_t d = 0;

            Core::Memcpy(&d, ((uint32_t*)src) + chunks, residual);

            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
            *base85++ = Encode85Byte(d);
            d /= 85;
        }
        *base85 = 0;
    }

    return rsize;
}

}

HK_NAMESPACE_END
