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

#include "ImageEncoders.h"
#include <Engine/Core/ScopedTimer.h>

#include <bc7enc_rdo/rgbcx.h>
#include <bc7enc_rdo/bc7decomp.h>
#include <bc7enc_rdo/bc7enc.h>
#include <bcdec/bcdec.h>

#ifdef HK_DEBUG
#    define BC6H_ASSERT(x) HK_ASSERT(x)
#    define BC6H_LOG(x)    Hk::LOG("{}", x)
#endif
#define BC6H_HALF_TO_FLOAT(h) Hk::fast_half_to_float(h)
#define BC6H_FLOAT_TO_HALF(f) Hk::half_from_float(f)

#define BC6H_ENC_IMPLEMENTATION
#include <bc6h_enc/bc6h_enc.h>

namespace
{

struct CompressionParams
{
    CompressionParams()
    {
        rgbcx::init();
        bc7enc_compress_block_init();

        for (uint32_t i = 0; i <= BC7ENC_MAX_UBER_LEVEL; i++)
        {
            bc7enc_compress_block_params_init(&bc7_params[i]);
            bc7_params[i].m_uber_level = i;
        }
    }
    bc7enc_compress_block_params bc7_params[BC7ENC_MAX_UBER_LEVEL + 1];
};

CompressionParams compressionParams;

} // namespace

HK_NAMESPACE_BEGIN

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

void Encode_BC6h_f16(void const* pSrc, void* pDest, bool bSigned)
{
    float block[4 * 4 * 4];
    uint16_t* halfs = (uint16_t*)pSrc;
    for (int i = 0; i < 4 * 4 * 4; i++)
    {
        block[i] = f16tof32(halfs[i]);
    }

    Encode_BC6h_f32(block, pDest, bSigned);
}

void Encode_BC6h_f32(void const* pSrc, void* pDest, bool bSigned)
{
    if (bSigned)
        bc6h_enc::EncodeBC6HS(pDest, pSrc);
    else
        bc6h_enc::EncodeBC6HU(pDest, pSrc);
}

void Encode_BC7(void const* pSrc, void* pDest, uint32_t Level)
{
    HK_ASSERT(Level <= BC7_ENCODE_MAX_LEVEL);

    bc7enc_compress_block(pDest, pSrc, &compressionParams.bc7_params[Level]);
}

// Input RGBA8 image, output BC1 compressed image
void CompressBC1(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height)
{
    uint8_t const* src = (uint8_t const*)pSrc;
    uint8_t*       dst = (uint8_t*)pDest;
    uint8_t        block[4 * 4 * 4];
    const uint32_t blockWidth       = 4;
    const uint32_t bpp              = 4;
    const uint32_t blockRowStride   = blockWidth * bpp;
    const size_t   blockSizeInBytes = 8;
    uint32_t       numBlocksX       = Width / blockWidth;
    uint32_t       numBlocksY       = Height / blockWidth;
    size_t         rowStride        = Width * bpp;
    for (uint32_t by = 0; by < numBlocksY; by++)
    {
        uint32_t pixY = by * blockWidth;

        for (uint32_t bx = 0; bx < numBlocksX; bx++)
        {
            uint8_t const* p = src + pixY * rowStride + bx * (blockWidth * bpp);

            memcpy(block + blockRowStride * 0, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 1, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 2, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 3, p, blockRowStride);

            TextureBlockCompression::Encode_BC1(block, dst, 5 /*TextureBlockCompression::BC1_ENCODE_MAX_LEVEL*/, false, false);

            dst += blockSizeInBytes;
        }
    }
}

// Input RGBA8 image, output BC2 compressed image
void CompressBC2(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height)
{
    uint8_t const* src = (uint8_t const*)pSrc;
    uint8_t*       dst = (uint8_t*)pDest;
    uint8_t        block[4 * 4 * 4];
    const uint32_t blockWidth       = 4;
    const uint32_t bpp              = 4;
    const uint32_t blockRowStride   = blockWidth * bpp;
    const size_t   blockSizeInBytes = 16;
    uint32_t       numBlocksX       = Width / blockWidth;
    uint32_t       numBlocksY       = Height / blockWidth;
    size_t         rowStride        = Width * bpp;
    for (uint32_t by = 0; by < numBlocksY; by++)
    {
        uint32_t pixY = by * blockWidth;

        for (uint32_t bx = 0; bx < numBlocksX; bx++)
        {
            uint8_t const* p = src + pixY * rowStride + bx * (blockWidth * bpp);

            memcpy(block + blockRowStride * 0, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 1, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 2, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 3, p, blockRowStride);

            TextureBlockCompression::Encode_BC2(block, dst, 5 /*TextureBlockCompression::BC2_ENCODE_MAX_LEVEL*/);

            dst += blockSizeInBytes;
        }
    }
}

// Input RGBA8 image, output BC3 compressed image
void CompressBC3(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height)
{
    uint8_t const* src = (uint8_t const*)pSrc;
    uint8_t*       dst = (uint8_t*)pDest;
    uint8_t        block[4 * 4 * 4];
    const uint32_t blockWidth       = 4;
    const uint32_t bpp              = 4;
    const uint32_t blockRowStride   = blockWidth * bpp;
    const size_t   blockSizeInBytes = 16;
    uint32_t       numBlocksX       = Width / blockWidth;
    uint32_t       numBlocksY       = Height / blockWidth;
    size_t         rowStride        = Width * bpp;
    for (uint32_t by = 0; by < numBlocksY; by++)
    {
        uint32_t pixY = by * blockWidth;

        for (uint32_t bx = 0; bx < numBlocksX; bx++)
        {
            uint8_t const* p = src + pixY * rowStride + bx * (blockWidth * bpp);

            memcpy(block + blockRowStride * 0, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 1, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 2, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 3, p, blockRowStride);

            TextureBlockCompression::Encode_BC3(block, dst, 5 /*TextureBlockCompression::BC3_ENCODE_MAX_LEVEL*/, true);

            dst += blockSizeInBytes;
        }
    }
}

// Input R8 image, output BC4 compressed image
void CompressBC4(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height)
{
    uint8_t const* src = (uint8_t const*)pSrc;
    uint8_t*       dst = (uint8_t*)pDest;
    uint8_t        block[4 * 4];
    const uint32_t blockWidth       = 4;
    const uint32_t bpp              = 1;
    const uint32_t blockRowStride   = blockWidth * bpp;
    const size_t   blockSizeInBytes = 8;
    uint32_t       numBlocksX       = Width / blockWidth;
    uint32_t       numBlocksY       = Height / blockWidth;
    size_t         rowStride        = Width * bpp;
    for (uint32_t by = 0; by < numBlocksY; by++)
    {
        uint32_t pixY = by * blockWidth;

        for (uint32_t bx = 0; bx < numBlocksX; bx++)
        {
            uint8_t const* p = src + pixY * rowStride + bx * (blockWidth * bpp);

            memcpy(block + blockRowStride * 0, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 1, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 2, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 3, p, blockRowStride);

            TextureBlockCompression::Encode_BC4(block, dst, true);

            dst += blockSizeInBytes;
        }
    }
}

// Input RG8 image, output BC5 compressed image
void CompressBC5(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height)
{
    uint8_t const* src = (uint8_t const*)pSrc;
    uint8_t*       dst = (uint8_t*)pDest;
    uint8_t        block[4 * 4 * 2];
    const uint32_t blockWidth       = 4;
    const uint32_t bpp              = 2;
    const uint32_t blockRowStride   = blockWidth * bpp;
    const size_t   blockSizeInBytes = 16;
    uint32_t       numBlocksX       = Width / blockWidth;
    uint32_t       numBlocksY       = Height / blockWidth;
    size_t         rowStride        = Width * bpp;
    for (uint32_t by = 0; by < numBlocksY; by++)
    {
        uint32_t pixY = by * blockWidth;

        for (uint32_t bx = 0; bx < numBlocksX; bx++)
        {
            uint8_t const* p = src + pixY * rowStride + bx * (blockWidth * bpp);

            memcpy(block + blockRowStride * 0, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 1, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 2, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 3, p, blockRowStride);

            TextureBlockCompression::Encode_BC5(block, dst, true);

            dst += blockSizeInBytes;
        }
    }
}

// Input RGBA32_FLOAT image, output BC6 compressed image
void CompressBC6h(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height, bool bSigned)
{
    ScopedTimer timer("CompressBC6h");

    //uint8_t const* src = (uint8_t const*)pSrc;
    //uint8_t*       dst = (uint8_t*)pDest;
    //uint8_t        block[4 * 4 * 4 * sizeof(float)];
    const uint32_t blockWidth = 4;
    const uint32_t bpp        = 4 * sizeof(float);
    //const uint32_t blockRowStride   = blockWidth * bpp;
    //const size_t   blockSizeInBytes = 16;
    uint32_t numBlocksX = Width / blockWidth;
    uint32_t numBlocksY = Height / blockWidth;
    size_t   rowStride  = Width * bpp;

    const int num_threads = 16;

    Thread threads[num_threads];

    struct ThreadData
    {
        uint32_t    firstBlock;
        uint32_t    numBlocks;
        uint32_t    numBlocksX;
        uint32_t    numBlocksY;
        size_t      rowStride;
        void const* pSrc;
        void*       pDest;
        bool        bSigned;
    };

    uint32_t numBlocks       = numBlocksX * numBlocksY;
    uint32_t blocksPerThread = numBlocks / num_threads;
    uint32_t firstBlock      = 0;

    AtomicInt counter{};

    ThreadData data[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        data[i].firstBlock = firstBlock;
        data[i].numBlocks  = blocksPerThread;
        data[i].numBlocksX = numBlocksX;
        data[i].numBlocksY = numBlocksY;
        data[i].rowStride  = rowStride;
        data[i].pSrc       = pSrc;
        data[i].pDest      = pDest;
        data[i].bSigned    = bSigned;
        firstBlock += blocksPerThread;

        threads[i] = Thread(
            [](ThreadData& data, uint32_t numBlocks, AtomicInt& counter)
            {
                uint8_t block[4 * 4 * 4 * sizeof(float)];

                const uint32_t blockWidth       = 4;
                const uint32_t bpp              = 4 * sizeof(float);
                const uint32_t blockRowStride   = blockWidth * bpp;
                const size_t   blockSizeInBytes = 16;

                for (uint32_t blockIndex = 0; blockIndex < data.numBlocks; blockIndex++)
                {
                    uint32_t i = blockIndex + data.firstBlock;

                    uint32_t bx = i % data.numBlocksX;
                    uint32_t by = i / data.numBlocksX;

                    uint8_t*       dst = (uint8_t*)data.pDest + i * blockSizeInBytes;
                    uint8_t const* p   = (uint8_t const*)data.pSrc + by * blockWidth * data.rowStride + bx * (blockWidth * bpp);

                    memcpy(block + blockRowStride * 0, p, blockRowStride);
                    p += data.rowStride;
                    memcpy(block + blockRowStride * 1, p, blockRowStride);
                    p += data.rowStride;
                    memcpy(block + blockRowStride * 2, p, blockRowStride);
                    p += data.rowStride;
                    memcpy(block + blockRowStride * 3, p, blockRowStride);

                    TextureBlockCompression::Encode_BC6h_f32(block, dst, data.bSigned);

                    int n = counter.Increment();
                    if (!(n % 512))
                        LOG("Blocks processed {} from {}\n", n, numBlocks);
                }
            },
            std::ref(data[i]), numBlocks, std::ref(counter));
    }

    for (int i = 0; i < num_threads; i++)
    {
        threads[i].Join();
    }

    #if 0
    uint8_t const* src = (uint8_t const*)pSrc;
    uint8_t*       dst = (uint8_t*)pDest;
    uint8_t        block[4 * 4 * 4 * sizeof(float)];
    const uint32_t blockWidth = 4;
    const uint32_t bpp        = 4 * sizeof(float);
    const uint32_t blockRowStride   = blockWidth * bpp;
    const size_t   blockSizeInBytes = 16;
    uint32_t numBlocksX = Width / blockWidth;
    uint32_t numBlocksY = Height / blockWidth;
    size_t   rowStride  = Width * bpp;
    for (uint32_t by = 0; by < numBlocksY; by++)
    {
        uint32_t pixY = by * blockWidth;

        for (uint32_t bx = 0; bx < numBlocksX; bx++)
        {
            uint8_t const* p = src + pixY * rowStride + bx * (blockWidth * bpp);

            memcpy(block + blockRowStride * 0, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 1, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 2, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 3, p, blockRowStride);

            TextureBlockCompression::Encode_BC6h_f32(block, dst, bSigned);

            dst += blockSizeInBytes;
        }
    }
    #endif
}

// Input RGBA8 image, output BC7 compressed image
void CompressBC7(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height)
{
    uint8_t const* src = (uint8_t const*)pSrc;
    uint8_t*       dst = (uint8_t*)pDest;
    uint8_t        block[4 * 4 * 4];
    const uint32_t blockWidth       = 4;
    const uint32_t bpp              = 4;
    const uint32_t blockRowStride   = blockWidth * bpp;
    const size_t   blockSizeInBytes = 16;
    uint32_t       numBlocksX       = Width / blockWidth;
    uint32_t       numBlocksY       = Height / blockWidth;
    size_t         rowStride        = Width * bpp;
    for (uint32_t by = 0; by < numBlocksY; by++)
    {
        uint32_t pixY = by * blockWidth;

        for (uint32_t bx = 0; bx < numBlocksX; bx++)
        {
            uint8_t const* p = src + pixY * rowStride + bx * (blockWidth * bpp);

            memcpy(block + blockRowStride * 0, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 1, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 2, p, blockRowStride);
            p += rowStride;
            memcpy(block + blockRowStride * 3, p, blockRowStride);

            TextureBlockCompression::Encode_BC7(block, dst, BC7_ENCODE_MAX_LEVEL);

            dst += blockSizeInBytes;
        }
    }
}

#if 0
void DecompressBC6h(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height, bool bSigned)
{
    //ScopedTimer timer("CompressBC6h");

    //uint8_t const* src = (uint8_t const*)pSrc;
    //uint8_t*       dst = (uint8_t*)pDest;
    //uint8_t        block[4 * 4 * 4 * sizeof(float)];
    const uint32_t blockWidth = 4;
    const uint32_t bpp        = 3 * sizeof(float);
    //const uint32_t blockRowStride   = blockWidth * bpp;
    //const size_t   blockSizeInBytes = 16;
    uint32_t numBlocksX = Width / blockWidth;
    uint32_t numBlocksY = Height / blockWidth;
    size_t   rowStride  = Width * bpp;

    const int num_threads = 16;

    Thread threads[num_threads];

    struct ThreadData
    {
        uint32_t    firstBlock;
        uint32_t    numBlocks;
        uint32_t    numBlocksX;
        uint32_t    numBlocksY;
        size_t      rowStride;
        void const* pSrc;
        void*       pDest;
        bool        bSigned;
    };

    uint32_t numBlocks       = numBlocksX * numBlocksY;
    uint32_t blocksPerThread = numBlocks / num_threads;
    uint32_t firstBlock      = 0;

    AtomicInt counter{};

    ThreadData data[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        data[i].firstBlock = firstBlock;
        data[i].numBlocks  = blocksPerThread;
        data[i].numBlocksX = numBlocksX;
        data[i].numBlocksY = numBlocksY;
        data[i].rowStride  = rowStride;
        data[i].pSrc       = pSrc;
        data[i].pDest      = pDest;
        data[i].bSigned    = bSigned;
        firstBlock += blocksPerThread;

        threads[i] = Thread(
            [](ThreadData& data, uint32_t numBlocks, AtomicInt& counter)
            {
                const uint32_t blockWidth       = 4;
                const uint32_t bpp              = 3 * sizeof(float);
                const size_t   blockSizeInBytes = 16;

                float block[4][4 * 4];

                for (uint32_t blockIndex = 0; blockIndex < data.numBlocks; blockIndex++)
                {
                    uint32_t i = blockIndex + data.firstBlock;

                    uint32_t bx = i % data.numBlocksX;
                    uint32_t by = i / data.numBlocksX;

                    uint8_t const* src = (uint8_t const*)data.pSrc + i * blockSizeInBytes;
                    uint8_t*       p   = (uint8_t*)data.pDest + by * blockWidth * data.rowStride + bx * (blockWidth * bpp);

                    bc6h_enc::DecodeBC6HS(block, src);

                    for (int t = 0; t < 4; t++)
                    {
                        for (int k = 0; k < 4; k++)
                        {
                            ((float*)p)[k * 3 + 0] = block[t][k * 4 + 0];
                            ((float*)p)[k * 3 + 1] = block[t][k * 4 + 1];
                            ((float*)p)[k * 3 + 2] = block[t][k * 4 + 2];
                        }
                        p += data.rowStride;
                    }

                    //TextureBlockCompression::Decode_BC6h_f32(src, p, data.rowStride, data.bSigned);

                    int n = counter.Increment();
                    if (!(n % 512))
                        LOG("Blocks processed {} from {}\n", n, numBlocks);
                }
            },
            std::ref(data[i]), numBlocks, std::ref(counter));
    }

    for (int i = 0; i < num_threads; i++)
    {
        threads[i].Join();
    }
}
#endif

} // namespace TextureBlockCompression


// Perfect Quantization of DXT endpoints
// https://gist.github.com/castano/c92c7626f288f9e99e158520b14a61cf

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



// Assume normals already normalized
RawImage PackNormalsRGBA_BC1_Compatible(Float3 const* Normals, uint32_t Width, uint32_t Height)
{
    RawImage image(Width, Height, RAW_IMAGE_FORMAT_RGBA8);

    uint8_t* data = (uint8_t*)image.GetData();
    for (uint32_t n = 0, count = Width * Height; n < count; ++n)
    {
        data[n * 4 + 0] = Math::Round(Math::Saturate(Normals[n].X * 0.5f + 0.5f) * 255.0f);
        data[n * 4 + 1] = Math::Round(Math::Saturate(Normals[n].Y * 0.5f + 0.5f) * 255.0f);
        data[n * 4 + 2] = Math::Round(Math::Saturate(Normals[n].Z * 0.5f + 0.5f) * 255.0f);
        data[n * 4 + 3] = 255;
    }

    return image;
}

// Assume normals already normalized
RawImage PackNormalsRG_BC5_Compatible(Float3 const* Normals, uint32_t Width, uint32_t Height)
{
    RawImage image(Width, Height, RAW_IMAGE_FORMAT_R8_ALPHA);

    uint8_t* data = (uint8_t*)image.GetData();
    for (uint32_t n = 0, count = Width * Height; n < count; ++n)
    {
        data[n * 2 + 0] = Math::Round(Math::Saturate(Normals[n].X * 0.5f + 0.5f) * 255.0f);
        data[n * 2 + 1] = Math::Round(Math::Saturate(Normals[n].Y * 0.5f + 0.5f) * 255.0f);
    }

    return image;
}

// Assume normals already normalized
RawImage PackNormalsSpheremap_BC5_Compatible(Float3 const* Normals, uint32_t Width, uint32_t Height)
{
    RawImage image(Width, Height, RAW_IMAGE_FORMAT_R8_ALPHA);

    uint8_t* data = (uint8_t*)image.GetData();
    for (uint32_t n = 0, count = Width * Height; n < count; ++n)
    {
        float denom = 1.0f / Math::Sqrt(Normals[n].Z * 8.0f + 8.0f);

        data[n * 2 + 0] = Math::Round(Math::Saturate(Normals[n].X * denom + 0.5f) * 255.0f);
        data[n * 2 + 1] = Math::Round(Math::Saturate(Normals[n].Y * denom + 0.5f) * 255.0f);
    }

    return image;
}

// Assume normals already normalized
RawImage PackNormalsStereographic_BC5_Compatible(Float3 const* Normals, uint32_t Width, uint32_t Height)
{
    RawImage image(Width, Height, RAW_IMAGE_FORMAT_R8_ALPHA);

    uint8_t* data = (uint8_t*)image.GetData();
    for (uint32_t n = 0, count = Width * Height; n < count; ++n)
    {
        data[n * 2 + 0] = Math::Round(Math::Saturate(Normals[n].X / (1.0f + Normals[n].Z) * 0.5f + 0.5f) * 255.0f);
        data[n * 2 + 1] = Math::Round(Math::Saturate(Normals[n].Y / (1.0f + Normals[n].Z) * 0.5f + 0.5f) * 255.0f);
    }

    return image;
}

// Assume normals already normalized
RawImage PackNormalsParaboloid_BC5_Compatible(Float3 const* Normals, uint32_t Width, uint32_t Height)
{
    RawImage image(Width, Height, RAW_IMAGE_FORMAT_R8_ALPHA);

    uint8_t* data = (uint8_t*)image.GetData();
    for (uint32_t n = 0, count = Width * Height; n < count; ++n)
    {
        float nx           = Normals[n].X;
        float ny           = Normals[n].Y;
        float a            = (nx * nx) + (ny * ny);
        float b            = Normals[n].Z;
        float c            = -1.0f;
        float discriminant = b * b - 4.0f * a * c;
        float t            = (-b + sqrtf(discriminant)) / (2.0f * a);
        nx                 = nx * t;
        ny                 = ny * t;

        data[n * 2 + 0] = Math::Round(Math::Saturate(nx * 0.5f + 0.5f) * 255.0f);
        data[n * 2 + 1] = Math::Round(Math::Saturate(ny * 0.5f + 0.5f) * 255.0f);
    }

    return image;
}

// Assume normals already normalized
RawImage PackNormalsRGBA_BC3_Compatible(Float3 const* Normals, uint32_t Width, uint32_t Height)
{
    RawImage image(Width, Height, RAW_IMAGE_FORMAT_RGBA8);

    uint8_t* data = (uint8_t*)image.GetData();
    for (uint32_t n = 0, count = Width * Height; n < count; ++n)
    {
        data[n * 4 + 0] = 255;
        data[n * 4 + 1] = Math::Round(Math::Saturate(Normals[n][1] * 0.5f + 0.5f) * 255.0f);
        data[n * 4 + 2] = Math::Round(Math::Saturate(Normals[n][2] * 0.5f + 0.5f) * 255.0f);
        data[n * 4 + 3] = Math::Round(Math::Saturate(Normals[n][0] * 0.5f + 0.5f) * 255.0f);
    }

    return image;
}

Float3 UnpackNormalRGBA_BC1_Compatible(void const* pData, uint32_t Index)
{
    uint8_t const* data = (uint8_t const*)pData + Index * 4;

    Float3 n;
    n.X = (data[0] / 255.0f) * 2.0f - 1.0f;
    n.Y = (data[1] / 255.0f) * 2.0f - 1.0f;
    n.Z = (data[2] / 255.0f) * 2.0f - 1.0f;

    return n;
}

Float3 UnpackNormalRG_BC5_Compatible(void const* pData, uint32_t Index)
{
    uint8_t const* data = (uint8_t const*)pData + Index * 2;

    Float3 n;
    n.X = (data[0] / 255.0f) * 2.0f - 1.0f;
    n.Y = (data[1] / 255.0f) * 2.0f - 1.0f;
    n.Z = Math::Sqrt(1.0f - (n.X * n.X + n.Y * n.Y));

    return n;
}

Float3 UnpackNormalSpheremap_BC5_Compatible(void const* pData, uint32_t Index)
{
    uint8_t const* data = (uint8_t const*)pData + Index * 2;

    float x = (data[0] / 255.0f) * 4.0f - 2.0f;
    float y = (data[1] / 255.0f) * 4.0f - 2.0f;
    float f = x * x + y * y;

    float s = Math::Sqrt(1.0f - f / 4.0f);

    Float3 n;
    n.X = x * s;
    n.Y = y * s;
    n.Z = 1.0f - f / 2.0f;

    return n;
}

Float3 UnpackNormalStereographic_BC5_Compatible(void const* pData, uint32_t Index)
{
    uint8_t const* data = (uint8_t const*)pData + Index * 2;

    Float3 n;

    n.X = (data[0] / 255.0f) * 2.0f - 1.0f;
    n.Y = (data[1] / 255.0f) * 2.0f - 1.0f;

    float denom = 2.0f / (1 + Math::Saturate(n.X * n.X + n.Y * n.Y));

    n.X *= denom;
    n.Y *= denom;
    n.Z = denom - 1.0f;
    return n;
}

Float3 UnpackNormalParaboloid_BC5_Compatible(void const* pData, uint32_t Index)
{
    uint8_t const* data = (uint8_t const*)pData + Index * 2;

    Float3 n;

    n.X = (data[0] / 255.0f) * 2.0f - 1.0f;
    n.Y = (data[1] / 255.0f) * 2.0f - 1.0f;
    n.Z = 1.0f - Math::Saturate(n.X * n.X + n.Y * n.Y);
    return n;
}

Float3 UnpackNormalRGBA_BC3_Compatible(void const* pData, uint32_t Index)
{
    uint8_t const* data = (uint8_t const*)pData + Index * 4;

    Float3 n;
    n.X = data[3] / 255.0f * 2.0f - 1.0f;
    n.Y = data[1] / 255.0f * 2.0f - 1.0f;
    n.Z = data[2] / 255.0f * 2.0f - 1.0f;
    return n;
}

HK_NAMESPACE_END