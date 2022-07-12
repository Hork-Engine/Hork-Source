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

#include "Image.h"

HK_FORCEINLINE uint16_t pack_r4g4b4a4(uint8_t const* pixel)
{
    uint16_t r = round(pixel[0] / 255.0f * 15.0f);
    uint16_t g = round(pixel[1] / 255.0f * 15.0f);
    uint16_t b = round(pixel[2] / 255.0f * 15.0f);
    uint16_t a = round(pixel[3] / 255.0f * 15.0f);
    return (r << 12) | (g << 8) | (b << 4) | a;
}

HK_FORCEINLINE void unpack_r4g4b4a4(uint16_t pack, uint8_t* pixel)
{
    pixel[0] = round((pack >> 12) / 15.0f * 255.0f);
    pixel[1] = round(((pack >> 8) & 0xf) / 15.0f * 255.0f);
    pixel[2] = round(((pack >> 4) & 0xf) / 15.0f * 255.0f);
    pixel[3] = round((pack & 0xf) / 15.0f * 255.0f);
}

extern const float pack_midpoints5[32];
extern const float pack_midpoints6[64];

HK_FORCEINLINE uint16_t pack_r5g6b5(float const* color)
{
    alignas(16) float result[4];

    // Clamp
    _mm_storer_ps(result,
                  _mm_min_ps(
                      _mm_max_ps(
                          _mm_set_ps(color[0], color[1], color[2], 0.0f),
                          _mm_setzero_ps()),
                      _mm_set_ps(31.0f, 63.0f, 31.0f, 0.0f)));

    uint32_t r = uint32_t(result[0]);
    uint32_t g = uint32_t(result[1]);
    uint32_t b = uint32_t(result[2]);

    // Round exactly according to 565 bit-expansion.
    r += (color[0] > pack_midpoints5[r]);
    g += (color[1] > pack_midpoints6[g]);
    b += (color[2] > pack_midpoints5[b]);

    return uint16_t((r << 11) | (g << 5) | b);
}

HK_FORCEINLINE uint16_t pack_r5g6b5(uint8_t const* pixel)
{
    float color[3];
    color[0] = pixel[0] / 255.0f;
    color[1] = pixel[1] / 255.0f;
    color[2] = pixel[2] / 255.0f;
    return pack_r5g6b5(color);
}

HK_FORCEINLINE void unpack_r5g6b5(uint16_t pack, uint8_t* pixel)
{
    pixel[0] = ((pack >> 8) & 0xf8) | ((pack >> 13) & 0x7);
    pixel[1] = ((pack >> 3) & 0xfc) | ((pack >> 9) & 0x3);
    pixel[2] = ((pack << 3) & 0xf8) | ((pack >> 2) & 0x7);
}

HK_FORCEINLINE uint16_t pack_r5g5b5a1(uint8_t const* pixel)
{
    uint16_t r = round(pixel[0] / 255.0f * 31.0f);
    uint16_t g = round(pixel[1] / 255.0f * 31.0f);
    uint16_t b = round(pixel[2] / 255.0f * 31.0f);
    uint16_t a = round(pixel[3] / 255.0f);
    return (r << 11) | (g << 6) | (b << 1) | a;
}

HK_FORCEINLINE void unpack_r5g5b5a1(uint16_t pack, uint8_t* pixel)
{
    pixel[0] = round((pack >> 11) / 31.0f * 255.0f);
    pixel[1] = round(((pack >> 6) & 0x1f) / 31.0f * 255.0f);
    pixel[2] = round(((pack >> 1) & 0x1f) / 31.0f * 255.0f);
    pixel[3] = ((pack & 1) << 8) - (pack & 1);
}

HK_FORCEINLINE uint32_t pack_r10g10b10a2(float const* pixel)
{
    // FIXME: Is it correct?
    uint32_t r = round(pixel[0] * 1023.0f);
    uint32_t g = round(pixel[1] * 1023.0f);
    uint32_t b = round(pixel[2] * 1023.0f);
    uint32_t a = round(pixel[3] * 3.0f);
    return (a << 30) | (r << 20) | (g << 10) | b;
}

HK_FORCEINLINE void unpack_r10g10b10a2(uint32_t pack, float* pixel)
{
    // FIXME: Is it correct?
    pixel[0] = ((pack >> 20) & 0x3ff) / 1023.0f;
    pixel[1] = ((pack >> 10) & 0x3ff) / 1023.0f;
    pixel[2] = (pack & 0x3ff) / 1023.0f;
    pixel[3] = (pack >> 30) / 3.0f;
}

HK_FORCEINLINE uint32_t pack_r11g11b10_float(float const* pixel)
{
    uint32_t r = (f32tof16(pixel[0]) << 17) & 0xffe00000;
    uint32_t g = (f32tof16(pixel[1]) << 6) & 0x001ffc00;
    uint32_t b = (f32tof16(pixel[2]) >> 5) & 0x000003ff;
    return r | g | b;
}

HK_FORCEINLINE void unpack_r11g11b10_float(uint32_t pack, float* pixel)
{
    pixel[0] = f16tof32((pack >> 17) & 0x7ff0);
    pixel[1] = f16tof32((pack >> 6) & 0x7ff0);
    pixel[2] = f16tof32((pack << 5) & 0x7fe0);
}

// R4G4B4A4 <-> RGBA8
struct Decoder_R4G4B4A4
{
    // Size for decoded image
    size_t GetRequiredMemorySize(uint32_t w, uint32_t h) const
    {
        return w * h * 4;
    }

    void Decode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint8_t*        dst   = (uint8_t*)pDest;
        uint16_t const* src   = (uint16_t const*)pSrc;
        uint16_t const* src_e = src + w * h;
        while (src < src_e)
        {
            unpack_r4g4b4a4(*src++, dst);
            dst += 4;
        }
    }

    void Encode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint16_t*      dst   = (uint16_t*)pDest;
        uint16_t*      dst_e = dst + w * h;
        uint8_t const* src   = (uint8_t const*)pSrc;
        while (dst < dst_e)
        {
            *dst++ = pack_r4g4b4a4(src);
            src += 4;
        }
    }

    constexpr uint32_t GetNumChannels() const
    {
        return 4;
    }

    constexpr IMAGE_DATA_TYPE GetDataType() const
    {
        return IMAGE_DATA_TYPE_UINT8;
    }

    constexpr bool IsSRGB() const
    {
        return false;
    }

    // Row stride for decoded image
    size_t GetRowStride(uint32_t w) const
    {
        return w * 4;
    }
};

// R5G6B5 <-> RGB8
struct Decoder_R5G6B5
{
    // Size for decoded image
    size_t GetRequiredMemorySize(uint32_t w, uint32_t h) const
    {
        return w * h * 3;
    }

    void Decode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint8_t*        dst   = (uint8_t*)pDest;
        uint16_t const* src   = (uint16_t const*)pSrc;
        uint16_t const* src_e = src + w * h;
        while (src < src_e)
        {
            unpack_r5g6b5(*src++, dst);
            dst += 3;
        }
    }

    void Encode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint16_t*      dst   = (uint16_t*)pDest;
        uint16_t*      dst_e = dst + w * h;
        uint8_t const* src   = (uint8_t const*)pSrc;
        while (dst < dst_e)
        {
            *dst++ = pack_r5g6b5(src);
            src += 3;
        }
    }

    constexpr uint32_t GetNumChannels() const
    {
        return 3;
    }

    constexpr IMAGE_DATA_TYPE GetDataType() const
    {
        return IMAGE_DATA_TYPE_UINT8;
    }

    constexpr bool IsSRGB() const
    {
        return false;
    }

    // Row stride for decoded image
    size_t GetRowStride(uint32_t w) const
    {
        return w * 3;
    }
};

// R5G5B5A1 <-> RGBA8
struct Decoder_R5G5B5A1
{
    // Size for decoded image
    size_t GetRequiredMemorySize(uint32_t w, uint32_t h) const
    {
        return w * h * 4;
    }

    void Decode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint8_t*        dst   = (uint8_t*)pDest;
        uint16_t const* src   = (uint16_t const*)pSrc;
        uint16_t const* src_e = src + w * h;
        while (src < src_e)
        {
            unpack_r5g5b5a1(*src++, dst);
            dst += 4;
        }
    }

    void Encode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint16_t*      dst   = (uint16_t*)pDest;
        uint16_t*      dst_e = dst + w * h;
        uint8_t const* src   = (uint8_t const*)pSrc;
        while (dst < dst_e)
        {
            *dst++ = pack_r5g5b5a1(src);
            src += 4;
        }
    }

    constexpr uint32_t GetNumChannels() const
    {
        return 4;
    }

    constexpr IMAGE_DATA_TYPE GetDataType() const
    {
        return IMAGE_DATA_TYPE_UINT8;
    }

    constexpr bool IsSRGB() const
    {
        return false;
    }

    // Row stride for decoded image
    size_t GetRowStride(uint32_t w) const
    {
        return w * 4;
    }
};

// R10G10B10A2 <-> RGBA32F
struct Decoder_R10G10B10A2
{
    // Size for decoded image
    size_t GetRequiredMemorySize(uint32_t w, uint32_t h) const
    {
        return w * h * 4 * sizeof(float);
    }

    void Decode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        float*          dst = (float*)pDest;
        uint32_t const* src = (uint32_t const*)pSrc;
        uint32_t const* src_e = src + w * h;
        while (src < src_e)
        {
            unpack_r10g10b10a2(*src++, dst);
            dst += 4;
        }
    }

    void Encode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint32_t*    dst = (uint32_t*)pDest;
        uint32_t*    dst_e = dst + w * h;
        float const* src = (float const*)pSrc;
        while (dst < dst_e)
        {
            *dst++ = pack_r10g10b10a2(src);
            src += 4;
        }
    }

    constexpr uint32_t GetNumChannels() const
    {
        return 4;
    }

    constexpr IMAGE_DATA_TYPE GetDataType() const
    {
        return IMAGE_DATA_TYPE_FLOAT;
    }

    constexpr bool IsSRGB() const
    {
        return false;
    }

    // Row stride for decoded image
    size_t GetRowStride(uint32_t w) const
    {
        return w * 4 * sizeof(float);
    }
};

// R11G11B10F <-> RGB32F
struct Decoder_R11G11B10F
{
    // Size for decoded image
    size_t GetRequiredMemorySize(uint32_t w, uint32_t h) const
    {
        return w * h * 3 * sizeof(float);
    }

    void Decode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        float*          dst = (float*)pDest;
        uint32_t const* src = (uint32_t const*)pSrc;
        uint32_t const* src_e = src + w * h;
        while (src < src_e)
        {
            unpack_r11g11b10_float(*src++, dst);
            dst += 3;
        }
    }

    void Encode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint32_t*    dst = (uint32_t*)pDest;
        uint32_t*    dst_e = dst + w * h;
        float const* src = (float const*)pSrc;
        while (dst < dst_e)
        {
            *dst++ = pack_r11g11b10_float(src);
            src += 3;
        }
    }

    constexpr uint32_t GetNumChannels() const
    {
        return 3;
    }

    constexpr IMAGE_DATA_TYPE GetDataType() const
    {
        return IMAGE_DATA_TYPE_FLOAT;
    }

    constexpr bool IsSRGB() const
    {
        return false;
    }

    // Row stride for decoded image
    size_t GetRowStride(uint32_t w) const
    {
        return w * 3 * sizeof(float);
    }
};

// Half float <-> float
template <uint32_t NumChannels>
struct Decoder_HalfFloat
{
    // Size for decoded image
    size_t GetRequiredMemorySize(uint32_t w, uint32_t h) const
    {
        return w * h * NumChannels * sizeof(float);
    }

    void Decode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        float*          dst   = (float*)pDest;
        float*          dst_e = dst + w * h * NumChannels;
        uint16_t const* src   = (uint16_t const*)pSrc;
        while (dst < dst_e)
            *dst++ = f16tof32(*src++);
    }

    void Encode(void* pDest, void const* pSrc, uint32_t w, uint32_t h)
    {
        uint16_t*    dst = (uint16_t*)pDest;
        uint16_t*    dst_e = dst + w * h * NumChannels;
        float const* src = (float const*)pSrc;
        while (dst < dst_e)
            *dst++ = f32tof16(*src++);
    }

    constexpr uint32_t GetNumChannels() const
    {
        return NumChannels;
    }

    constexpr IMAGE_DATA_TYPE GetDataType() const
    {
        return IMAGE_DATA_TYPE_FLOAT;
    }

    constexpr bool IsSRGB() const
    {
        return false;
    }

    // Row stride for decoded image
    size_t GetRowStride(uint32_t w) const
    {
        return w * NumChannels * sizeof(float);
    }
};

using Decoder_R16F = Decoder_HalfFloat<1>;
using Decoder_RG16F = Decoder_HalfFloat<2>;
using Decoder_RGBA16F = Decoder_HalfFloat<4>;
