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

#include "ImageEncoders.h"

#include <bc7enc_rdo/rgbcx.h>
#include <bc7enc_rdo/bc7decomp.h>
#include <bc7enc_rdo/bc7enc.h>
#include <bcdec/bcdec.h>

const float pack_midpoints5[32] = {
    0.015686f, 0.047059f, 0.078431f, 0.111765f, 0.145098f, 0.176471f, 0.207843f, 0.241176f, 0.274510f, 0.305882f, 0.337255f, 0.370588f, 0.403922f, 0.435294f, 0.466667f, 0.5f,
    0.533333f, 0.564706f, 0.596078f, 0.629412f, 0.662745f, 0.694118f, 0.725490f, 0.758824f, 0.792157f, 0.823529f, 0.854902f, 0.888235f, 0.921569f, 0.952941f, 0.984314f, FLT_MAX};

const float pack_midpoints6[64] = {
    0.007843f, 0.023529f, 0.039216f, 0.054902f, 0.070588f, 0.086275f, 0.101961f, 0.117647f, 0.133333f, 0.149020f, 0.164706f, 0.180392f, 0.196078f, 0.211765f, 0.227451f, 0.245098f,
    0.262745f, 0.278431f, 0.294118f, 0.309804f, 0.325490f, 0.341176f, 0.356863f, 0.372549f, 0.388235f, 0.403922f, 0.419608f, 0.435294f, 0.450980f, 0.466667f, 0.482353f, 0.500000f,
    0.517647f, 0.533333f, 0.549020f, 0.564706f, 0.580392f, 0.596078f, 0.611765f, 0.627451f, 0.643137f, 0.658824f, 0.674510f, 0.690196f, 0.705882f, 0.721569f, 0.737255f, 0.754902f,
    0.772549f, 0.788235f, 0.803922f, 0.819608f, 0.835294f, 0.850980f, 0.866667f, 0.882353f, 0.898039f, 0.913725f, 0.929412f, 0.945098f, 0.960784f, 0.976471f, 0.992157f, FLT_MAX};

/*void init_tables() {
    for (int i = 0; i < 31; i++) {
        float f0 = float(((i+0) << 3) | ((i+0) >> 2)) / 255.0f;
        float f1 = float(((i+1) << 3) | ((i+1) >> 2)) / 255.0f;
        midpoints5[i] = (f0 + f1) * 0.5;
    }
    midpoints5[31] = FLT_MAX;
    for (int i = 0; i < 63; i++) {
        float f0 = float(((i+0) << 2) | ((i+0) >> 4)) / 255.0f;
        float f1 = float(((i+1) << 2) | ((i+1) >> 4)) / 255.0f;
        midpoints6[i] = (f0 + f1) * 0.5;
    }
    midpoints6[63] = FLT_MAX;
}*/

struct CompressorsInitializer
{
    CompressorsInitializer()
    {
        rgbcx::init();
        bc7enc_compress_block_init();

        for (uint32_t i = 0 ; i <= BC7ENC_MAX_UBER_LEVEL ; i++)
        {
            bc7enc_compress_block_params_init(&bc7_params[i]);
            //bc7_params.m_uber_level = i;
        }

        //GetProfile_bc6h_veryfast(&bc6_params[0]);
        //GetProfile_bc6h_fast(&bc6_params[1]);
        //GetProfile_bc6h_basic(&bc6_params[2]);
        //GetProfile_bc6h_slow(&bc6_params[3]);
        //GetProfile_bc6h_veryslow(&bc6_params[4]);
    }
    bc7enc_compress_block_params bc7_params[BC7ENC_MAX_UBER_LEVEL + 1];
    //bc6h_enc_settings bc6_params[5];
};

static CompressorsInitializer compressorsInitializer;

namespace TextureBlockCompression
{

void Decode_BC1(void const* pSrc, void* pDest, size_t RowStride)
{
    bcdec_bc1(pSrc, pDest, RowStride);
}

void Decode_BC2(void const* pSrc, void* pDest, size_t RowStride)
{
    bcdec_bc2(pSrc, pDest, RowStride);
}

void Decode_BC3(void const* pSrc, void* pDest, size_t RowStride)
{
    bcdec_bc3(pSrc, pDest, RowStride);
}

void Decode_BC4(void const* pSrc, void* pDest, size_t RowStride)
{
    bcdec_bc4(pSrc, pDest, RowStride);
}

void Decode_BC5(void const* pSrc, void* pDest, size_t RowStride)
{
    bcdec_bc5(pSrc, pDest, RowStride);
}

void Decode_BC6h_f16(void const* pSrc, void* pDest, size_t RowStride, bool bSigned)
{
    bcdec_bc6h_half(pSrc, pDest, RowStride / sizeof(uint16_t), bSigned);
}

void Decode_BC6h_f32(void const* pSrc, void* pDest, size_t RowStride, bool bSigned)
{
    bcdec_bc6h_float(pSrc, pDest, RowStride / sizeof(float), bSigned);
}

void Decode_BC7(void const* pSrc, void* pDest, size_t RowStride)
{
#    if 1
    if (RowStride == 16)
    {
        bc7decomp::unpack_bc7(pSrc, (bc7decomp::color_rgba*)pDest);
    }
    else
    {
        bc7decomp::color_rgba decompressedBlock[4][4];

        bc7decomp::unpack_bc7(pSrc, &decompressedBlock[0][0]);

        uint8_t* dst = (uint8_t*)pDest;

        for (int i = 0 ; i < 4 ; i++)
        {
            memcpy(dst, decompressedBlock[i], sizeof(decompressedBlock[0]));
            dst += RowStride;
        }
    }
#    else
    bcdec_bc7(pSrc, pDest, RowStride);
#    endif
}

void Encode_BC1(void const* pSrc, void* pDest, uint32_t Level, bool b3ColorMode, bool bTransparentPixelsForBlack)
{
    HK_ASSERT(Level <= BC1_ENCODE_MAX_LEVEL);

    rgbcx::encode_bc1(Level, pDest, (uint8_t const*)pSrc, b3ColorMode, bTransparentPixelsForBlack);
}

void Encode_BC2(void const* pSrc, void* pDest, uint32_t Level)
{
    HK_ASSERT(Level <= BC2_ENCODE_MAX_LEVEL);

    uint8_t const* pixels = static_cast<uint8_t const*>(pSrc);
    uint8_t*       block  = static_cast<uint8_t*>(pDest);

    for (int i = 0; i < 8; ++i, pixels += 8)
    {
        int q1   = std::min(int(pixels[3] * (15.0f / 255.0f) + 0.5f), 15);
        int q2   = std::min(int(pixels[7] * (15.0f / 255.0f) + 0.5f), 15);
        *block++ = (uint8_t)(q1 | (q2 << 4));
    }
    rgbcx::encode_bc1(Level, block, (uint8_t const*)pSrc, false, false);
}

void Encode_BC3(void const* pSrc, void* pDest, uint32_t Level, bool bMaxQuality)
{
    HK_ASSERT(Level <= BC3_ENCODE_MAX_LEVEL);

    if (bMaxQuality)
        rgbcx::encode_bc3_hq(Level, pDest, (uint8_t const*)pSrc);
    else
        rgbcx::encode_bc3(Level, pDest, (uint8_t const*)pSrc);
}

void Encode_BC4(void const* pSrc, void* pDest, bool bMaxQuality)
{
    if (bMaxQuality)
        rgbcx::encode_bc4_hq(pDest, (uint8_t const*)pSrc, 1);
    else
        rgbcx::encode_bc4(pDest, (uint8_t const*)pSrc, 1);
}

void Encode_BC5(void const* pSrc, void* pDest, bool bMaxQuality)
{
    if (bMaxQuality)
        rgbcx::encode_bc5_hq(pDest, (uint8_t const*)pSrc, 0, 1, 2);
    else
        rgbcx::encode_bc5(pDest, (uint8_t const*)pSrc, 0, 1, 2);
}

void Encode_BC6(void const* pSrc, void* pDest, uint32_t Level)
{
    //HK_ASSERT(Level <= BC6_ENCODE_MAX_LEVEL);

    // TODO: ISPCTextureCompressor?
    // or nvidia-texture-tools/src/bc6h/zoh.h

    //rgba_surface surface;
    //surface.ptr = pSrc;
    //surface.width = 4;
    //surface.height = 4;
    //surface.stride = 4 * sizeof(uint16_t); // FIXME

    //CompressBlocksBC6H(&surface, uint8_t* dst, &compressorsInitializer.bc6_params[Level]);
}

void Encode_BC7(void const* pSrc, void* pDest, uint32_t Level)
{
    HK_ASSERT(Level <= BC7_ENCODE_MAX_LEVEL);

    bc7enc_compress_block(pDest, pSrc, &compressorsInitializer.bc7_params[Level]);
}

}
