/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Platform/BaseTypes.h>

namespace Core
{

HK_FORCEINLINE constexpr bool IsLittleEndian()
{
#ifdef HK_LITTLE_ENDIAN
    return true;
#else
    return false;
#endif
}

HK_FORCEINLINE constexpr bool IsBigEndian()
{
    return !IsLittleEndian();
}

HK_FORCEINLINE constexpr uint16_t Swap16(uint16_t _Val)
{
    return ((_Val & 0xff) << 8) | ((_Val >> 8) & 0xff);
}

HK_FORCEINLINE constexpr uint32_t Swap32(uint32_t _Val)
{
    return ((_Val & 0xff) << 24) | (((_Val >> 8) & 0xff) << 16) | (((_Val >> 16) & 0xff) << 8) | ((_Val >> 24) & 0xff);
}

HK_FORCEINLINE constexpr uint64_t Swap64(uint64_t _Val)
{
    return (((_Val)&0xff) << 56) | (((_Val >> 8) & 0xff) << 48) | (((_Val >> 16) & 0xff) << 40) | (((_Val >> 24) & 0xff) << 32) | (((_Val >> 32) & 0xff) << 24) | (((_Val >> 40) & 0xff) << 16) | (((_Val >> 48) & 0xff) << 8) | (((_Val >> 56) & 0xff));
}

HK_FORCEINLINE float Swap32f(float _Val)
{
    union
    {
        float f;
        byte  b[4];
    } dat1, dat2;

    dat1.f    = _Val;
    dat2.b[0] = dat1.b[3];
    dat2.b[1] = dat1.b[2];
    dat2.b[2] = dat1.b[1];
    dat2.b[3] = dat1.b[0];
    return dat2.f;
}

HK_FORCEINLINE double Swap64f(double _Val)
{
    union
    {
        double f;
        byte   b[8];
    } dat1, dat2;

    dat1.f    = _Val;
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

HK_FORCEINLINE constexpr uint16_t BigWord(uint16_t _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return Swap16(_Val);
#else
    return _Val;
#endif
}

HK_FORCEINLINE constexpr uint32_t BigDWord(uint32_t _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return Swap32(_Val);
#else
    return _Val;
#endif
}

HK_FORCEINLINE constexpr uint64_t BigDDWord(uint64_t _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return Swap64(_Val);
#else
    return _Val;
#endif
}

HK_FORCEINLINE float BigFloat(float _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return Swap32f(_Val);
#else
    return _Val;
#endif
}

HK_FORCEINLINE double BigDouble(double _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return Swap64f(_Val);
#else
    return _Val;
#endif
}

HK_FORCEINLINE constexpr uint16_t LittleWord(uint16_t _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return _Val;
#else
    return Swap16(_Val);
#endif
}

HK_FORCEINLINE constexpr uint32_t LittleDWord(uint32_t _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return _Val;
#else
    return Swap32(_Val);
#endif
}

HK_FORCEINLINE constexpr uint64_t LittleDDWord(uint64_t _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return _Val;
#else
    return Swap64(_Val);
#endif
}

HK_FORCEINLINE float LittleFloat(float _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return _Val;
#else
    return Swap32f(_Val);
#endif
}

HK_FORCEINLINE double LittleDouble(double _Val)
{
#ifdef HK_LITTLE_ENDIAN
    return _Val;
#else
    return Swap64f(_Val);
#endif
}

} // namespace Core
