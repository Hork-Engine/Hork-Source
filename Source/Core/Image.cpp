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

#include "Image.h"
#include "ImageEncoders.h"

#include <Platform/Logger.h>

#define STBIR_MALLOC(sz, context) Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(sz)
#define STBIR_FREE(p, context)    Platform::GetHeapAllocator<HEAP_TEMP>().Free(p)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#define STBIR_MAX_CHANNELS 4
#include "stb/stb_image_resize.h"


// NOTE: The texture format is compatible with the NVRHI API which will be used in the future.
// clang-format off
static const TextureFormatInfo TexFormat[] = {
        //           format                   name             bytes blk         kind                            data type                      red    green  blue   alpha  depth  stencl signed srgb
        { TEXTURE_FORMAT_UNDEFINED,         "UNDEFINED",         0,   0, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UNKNOWN,             false, false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_R8_UINT,           "R8_UINT",           1,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT8,               true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_R8_SINT,           "R8_SINT",           1,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT8,               true,  false, false, false, false, false, true,  false },
        { TEXTURE_FORMAT_R8_UNORM,          "R8_UNORM",          1,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_R8_SNORM,          "R8_SNORM",          1,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_RG8_UINT,          "RG8_UINT",          2,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT8,               true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_RG8_SINT,          "RG8_SINT",          2,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT8,               true,  true,  false, false, false, false, true,  false },
        { TEXTURE_FORMAT_RG8_UNORM,         "RG8_UNORM",         2,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_RG8_SNORM,         "RG8_SNORM",         2,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_R16_UINT,          "R16_UINT",          2,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT16,              true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_R16_SINT,          "R16_SINT",          2,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT16,              true,  false, false, false, false, false, true,  false },
        { TEXTURE_FORMAT_R16_UNORM,         "R16_UNORM",         2,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT16,              true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_R16_SNORM,         "R16_SNORM",         2,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT16,              true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_R16_FLOAT,         "R16_FLOAT",         2,   1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_HALF,                true,  false, false, false, false, false, true,  false },
        { TEXTURE_FORMAT_BGRA4_UNORM,       "BGRA4_UNORM",       2,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_ENCODED_R4G4B4A4,    true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_B5G6R5_UNORM,      "B5G6R5_UNORM",      2,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_ENCODED_R5G6B5,      true,  true,  true,  false, false, false, false, false },
        { TEXTURE_FORMAT_B5G5R5A1_UNORM,    "B5G5R5A1_UNORM",    2,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_ENCODED_R5G5B5A1,    true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_RGBA8_UINT,        "RGBA8_UINT",        4,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT8,               true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_RGBA8_SINT,        "RGBA8_SINT",        4,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT8,               true,  true,  true,  true,  false, false, true,  false },
        { TEXTURE_FORMAT_RGBA8_UNORM,       "RGBA8_UNORM",       4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_RGBA8_SNORM,       "RGBA8_SNORM",       4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_BGRA8_UNORM,       "BGRA8_UNORM",       4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_SRGBA8_UNORM,      "SRGBA8_UNORM",      4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  true,  true,  true,  false, false, false, true  },
        { TEXTURE_FORMAT_SBGRA8_UNORM,      "SBGRA8_UNORM",      4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT8,               true,  true,  true,  true,  false, false, false, true  },
        { TEXTURE_FORMAT_R10G10B10A2_UNORM, "R10G10B10A2_UNORM", 4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_ENCODED_R10G10B10A2, true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_R11G11B10_FLOAT,   "R11G11B10_FLOAT",   4,   1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_ENCODED_R11G11B10F,  true,  true,  true,  false, false, false, false, false },
        { TEXTURE_FORMAT_RG16_UINT,         "RG16_UINT",         4,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT16,              true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_RG16_SINT,         "RG16_SINT",         4,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT16,              true,  true,  false, false, false, false, true,  false },
        { TEXTURE_FORMAT_RG16_UNORM,        "RG16_UNORM",        4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT16,              true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_RG16_SNORM,        "RG16_SNORM",        4,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT16,              true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_RG16_FLOAT,        "RG16_FLOAT",        4,   1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_HALF,                true,  true,  false, false, false, false, true,  false },
        { TEXTURE_FORMAT_R32_UINT,          "R32_UINT",          4,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_R32_SINT,          "R32_SINT",          4,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  false, false, false, false, false, true,  false },
        { TEXTURE_FORMAT_R32_FLOAT,         "R32_FLOAT",         4,   1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_FLOAT,               true,  false, false, false, false, false, true,  false },
        { TEXTURE_FORMAT_RGBA16_UINT,       "RGBA16_UINT",       8,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT16,              true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_RGBA16_SINT,       "RGBA16_SINT",       8,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT16,              true,  true,  true,  true,  false, false, true,  false },
        { TEXTURE_FORMAT_RGBA16_FLOAT,      "RGBA16_FLOAT",      8,   1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_HALF,                true,  true,  true,  true,  false, false, true,  false },
        { TEXTURE_FORMAT_RGBA16_UNORM,      "RGBA16_UNORM",      8,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT16,              true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_RGBA16_SNORM,      "RGBA16_SNORM",      8,   1, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_UINT16,              true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_RG32_UINT,         "RG32_UINT",         8,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_RG32_SINT,         "RG32_SINT",         8,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  true,  false, false, false, false, true,  false },
        { TEXTURE_FORMAT_RG32_FLOAT,        "RG32_FLOAT",        8,   1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_FLOAT,               true,  true,  false, false, false, false, true,  false },
        { TEXTURE_FORMAT_RGB32_UINT,        "RGB32_UINT",        12,  1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  true,  true,  false, false, false, false, false },
        { TEXTURE_FORMAT_RGB32_SINT,        "RGB32_SINT",        12,  1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  true,  true,  false, false, false, true,  false },
        { TEXTURE_FORMAT_RGB32_FLOAT,       "RGB32_FLOAT",       12,  1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_FLOAT,               true,  true,  true,  false, false, false, true,  false },
        { TEXTURE_FORMAT_RGBA32_UINT,       "RGBA32_UINT",       16,  1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_RGBA32_SINT,       "RGBA32_SINT",       16,  1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_UINT32,              true,  true,  true,  true,  false, false, true,  false },
        { TEXTURE_FORMAT_RGBA32_FLOAT,      "RGBA32_FLOAT",      16,  1, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_FLOAT,               true,  true,  true,  true,  false, false, true,  false },
        { TEXTURE_FORMAT_D16,               "D16",               2,   1, TEXTURE_FORMAT_KIND_DEPTHSTENCIL, IMAGE_DATA_TYPE_ENCODED_DEPTH,       false, false, false, false, true,  false, false, false },
        { TEXTURE_FORMAT_D24S8,             "D24S8",             4,   1, TEXTURE_FORMAT_KIND_DEPTHSTENCIL, IMAGE_DATA_TYPE_ENCODED_DEPTH,       false, false, false, false, true,  true,  false, false },
        { TEXTURE_FORMAT_X24G8_UINT,        "X24G8_UINT",        4,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_ENCODED_DEPTH,       false, false, false, false, false, true,  false, false },
        { TEXTURE_FORMAT_D32,               "D32",               4,   1, TEXTURE_FORMAT_KIND_DEPTHSTENCIL, IMAGE_DATA_TYPE_ENCODED_DEPTH,       false, false, false, false, true,  false, false, false },
        { TEXTURE_FORMAT_D32S8,             "D32S8",             8,   1, TEXTURE_FORMAT_KIND_DEPTHSTENCIL, IMAGE_DATA_TYPE_ENCODED_DEPTH,       false, false, false, false, true,  true,  false, false },
        { TEXTURE_FORMAT_X32G8_UINT,        "X32G8_UINT",        8,   1, TEXTURE_FORMAT_KIND_INTEGER,      IMAGE_DATA_TYPE_ENCODED_DEPTH,       false, false, false, false, false, true,  false, false },
        { TEXTURE_FORMAT_BC1_UNORM,         "BC1_UNORM",         8,   4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_BC1_UNORM_SRGB,    "BC1_UNORM_SRGB",    8,   4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, true  },
        { TEXTURE_FORMAT_BC2_UNORM,         "BC2_UNORM",         16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_BC2_UNORM_SRGB,    "BC2_UNORM_SRGB",    16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, true  },
        { TEXTURE_FORMAT_BC3_UNORM,         "BC3_UNORM",         16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_BC3_UNORM_SRGB,    "BC3_UNORM_SRGB",    16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, true  },
        { TEXTURE_FORMAT_BC4_UNORM,         "BC4_UNORM",         8,   4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_BC4_SNORM,         "BC4_SNORM",         8,   4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  false, false, false, false, false, false, false },
        { TEXTURE_FORMAT_BC5_UNORM,         "BC5_UNORM",         16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_BC5_SNORM,         "BC5_SNORM",         16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  false, false, false, false, false, false },
        { TEXTURE_FORMAT_BC6H_UFLOAT,       "BC6H_UFLOAT",       16,  4, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  false, false, false, false, false },
        { TEXTURE_FORMAT_BC6H_SFLOAT,       "BC6H_SFLOAT",       16,  4, TEXTURE_FORMAT_KIND_FLOAT,        IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  false, false, false, true,  false },
        { TEXTURE_FORMAT_BC7_UNORM,         "BC7_UNORM",         16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, false },
        { TEXTURE_FORMAT_BC7_UNORM_SRGB,    "BC7_UNORM_SRGB",    16,  4, TEXTURE_FORMAT_KIND_NORMALIZED,   IMAGE_DATA_TYPE_COMPRESSED,          true,  true,  true,  true,  false, false, false, true  },
    };
// clang-format on

TextureFormatInfo const& GetTextureFormatInfo(TEXTURE_FORMAT Format)
{
    static_assert(sizeof(TexFormat) / sizeof(TextureFormatInfo) == size_t(TEXTURE_FORMAT_MAX),
                  "The format info table doesn't have the right number of elements");

    if (uint32_t(Format) >= uint32_t(TEXTURE_FORMAT_MAX))
        return TexFormat[0];

    TextureFormatInfo const& info = TexFormat[uint32_t(Format)];
    HK_ASSERT(info.Format == Format);
    return info;
}

uint32_t CalcNumMips(TEXTURE_FORMAT Format, uint32_t Width, uint32_t Height, uint32_t Depth)
{
    bool bCompressed = IsCompressedFormat(Format);

    const uint32_t blockSize = 4;

    if (bCompressed)
    {
        HK_VERIFY_R(Depth == 1, "CalcNumMips: Compressed 3D textures are not supported");
        HK_VERIFY_R(Width >= blockSize && (Width % blockSize) == 0, "CalcNumMips: Width must be a multiple of blockSize for compressed textures");
        HK_VERIFY_R(Height >= blockSize && (Height % blockSize) == 0, "CalcNumMips: Height must be a multiple of blockSize for compressed textures");
    }

    uint32_t numMips = 0;
    uint32_t sz      = Math::Max3(Width, Height, Depth);

    if (bCompressed)
        sz /= blockSize;

    while ((sz >>= 1) > 0)
        numMips++;

    return numMips;
}

bool ImageSubresource::Write(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height, void const* Bytes)
{
    TextureFormatInfo const& info = GetTextureFormatInfo(m_Format);

    uint32_t blockSize        = info.BlockSize;
    size_t   blockSizeInBytes = info.BytesPerBlock;

    HK_VERIFY_R((Width % blockSize) == 0, "ImageSubresource::Write: Width must be a multiple of blockSize for compressed textures");
    HK_VERIFY_R((Height % blockSize) == 0, "ImageSubresource::Write: Height must be a multiple of blockSize for compressed textures");
    HK_VERIFY_R((X % blockSize) == 0, "ImageSubresource::Write: The offset must be a multiple of blockSize for compressed textures");
    HK_VERIFY_R((Y % blockSize) == 0, "ImageSubresource::Write: The offset must be a multiple of blockSize for compressed textures");

    HK_VERIFY_R(X + Width <= m_Width, "ImageSubresource::Write: Writing out of bounds");
    HK_VERIFY_R(Y + Height <= m_Height, "ImageSubresource::Write: Writing out of bounds");

    X /= blockSize;
    Y /= blockSize;
    Width /= blockSize;
    Height /= blockSize;

    uint32_t viewWidth  = m_Width / blockSize;
    uint32_t viewHeight = m_Height / blockSize;

    if (X == 0 && Y == 0 && viewWidth == Width && viewHeight == Height)
    {
        Platform::Memcpy(m_pData, Bytes, Width * Height * blockSizeInBytes);
    }
    else
    {
        size_t offset = (Y * viewHeight + X) * blockSizeInBytes;

        uint8_t* ptr = m_pData + offset;
        uint8_t const* src = (uint8_t const*)Bytes;
        for (uint32_t y = 0; y < Height; ++y)
        {
            Platform::Memcpy(ptr, src, Width * blockSizeInBytes);
            ptr += viewWidth * blockSizeInBytes;
            src += Width * blockSizeInBytes;
        }
    }
    return true;
}

bool ImageSubresource::Read(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height, void* Bytes, size_t _SizeInBytes) const
{
    TextureFormatInfo const& info = GetTextureFormatInfo(m_Format);

    uint32_t blockSize        = info.BlockSize;
    size_t   blockSizeInBytes = info.BytesPerBlock;

    HK_VERIFY_R((Width % blockSize) == 0, "ImageSubresource::Read: Width must be a multiple of blockSize for compressed textures");
    HK_VERIFY_R((Height % blockSize) == 0, "ImageSubresource::Read: Height must be a multiple of blockSize for compressed textures");
    HK_VERIFY_R((X % blockSize) == 0, "ImageSubresource::Read: The offset must be a multiple of blockSize for compressed textures");
    HK_VERIFY_R((Y % blockSize) == 0, "ImageSubresource::Read: The offset must be a multiple of blockSize for compressed textures");

    HK_VERIFY_R(X + Width <= m_Width, "ImageSubresource::Read: Reading out of bounds");
    HK_VERIFY_R(Y + Height <= m_Height, "ImageSubresource::Read: Reading out of bounds");

    X /= blockSize;
    Y /= blockSize;
    Width /= blockSize;
    Height /= blockSize;

    uint32_t viewWidth  = m_Width / blockSize;
    uint32_t viewHeight = m_Height / blockSize;

    size_t offset = (Y * viewHeight + X) * blockSizeInBytes;

    HK_VERIFY_R(Width * Height * blockSizeInBytes <= _SizeInBytes, "ImageSubresource::Read: Buffer size is not enough");

    if (X == 0 && Y == 0 && viewWidth == Width && viewHeight == Height)
    {
        Platform::Memcpy(Bytes, m_pData, Width * Height * blockSizeInBytes);
    }
    else
    {
        uint8_t const* ptr = m_pData + offset;
        uint8_t* dst = (uint8_t*)Bytes;
        for (uint32_t y = 0; y < Height; ++y)
        {
            Platform::Memcpy(dst, ptr, Width * blockSizeInBytes);
            ptr += viewWidth * blockSizeInBytes;
            dst += Width * blockSizeInBytes;
        }
    }

    return true;
}

int ImageSubresource::NumChannels() const
{
    TextureFormatInfo const& info = GetTextureFormatInfo(m_Format);

    int numChannels = 0;
    if (info.bHasRed)
        numChannels++;
    if (info.bHasGreen)
        numChannels++;
    if (info.bHasBlue)
        numChannels++;
    if (info.bHasAlpha)
        numChannels++;
    if (info.bHasDepth)
        numChannels++;
    if (info.bHasStencil)
        numChannels++;

    return numChannels;
}

size_t ImageSubresource::GetBytesPerPixel() const
{
    return IsCompressed() ? 0 : GetTextureFormatInfo(m_Format).BytesPerBlock;
}

size_t ImageSubresource::GetBlockSizeInBytes() const
{
    return IsCompressed() ? GetTextureFormatInfo(m_Format).BytesPerBlock : 0;
}

IMAGE_DATA_TYPE ImageSubresource::GetDataType() const
{
    return GetTextureFormatInfo(m_Format).DataType;
}

ImageSubresource ImageSubresource::NextSlice() const
{
    if (m_Desc.SliceIndex + 1 >= m_SliceCount)
    {
        return {};
    }

    ImageSubresource subresource;
    subresource.m_Desc.SliceIndex  = m_Desc.SliceIndex + 1;
    subresource.m_Desc.MipmapIndex = m_Desc.MipmapIndex;
    subresource.m_pData            = m_pData + m_SizeInBytes;
    subresource.m_SizeInBytes      = m_SizeInBytes;
    subresource.m_Width            = m_Width;
    subresource.m_Height           = m_Height;
    subresource.m_SliceCount       = m_SliceCount;
    subresource.m_Format           = m_Format;

    return subresource;
}

void ImageStorage::Reset(ImageStorageDesc const& Desc)
{
    m_Desc = Desc;

    // Validation
    HK_VERIFY(m_Desc.Width >= 1, "ImageStorage: Invalid image size");

    if (m_Desc.Type == TEXTURE_1D || m_Desc.Type == TEXTURE_1D_ARRAY)
    {
        HK_VERIFY(m_Desc.Height == 1, "ImageStorage: Invalid image size");
    }
    else
    {
        HK_VERIFY(m_Desc.Height >= 1, "ImageStorage: Invalid image size");
    }

    if (m_Desc.Type == TEXTURE_CUBE || m_Desc.Type == TEXTURE_CUBE_ARRAY)
    {
        HK_VERIFY(m_Desc.Width == m_Desc.Height, "ImageStorage: Cubemap always has square faces");
    }

    if (m_Desc.Type == TEXTURE_1D || m_Desc.Type == TEXTURE_2D)
    {
        HK_VERIFY(m_Desc.SliceCount == 1, "ImageStorage: Invalid number of slices for 1D/2D texture");
    }
    else if (m_Desc.Type == TEXTURE_CUBE)
    {
        HK_VERIFY(m_Desc.SliceCount == 6, "ImageStorage: The number of slices for cubemaps should always be 6");
    }
    else if (m_Desc.Type == TEXTURE_CUBE_ARRAY)
    {
        HK_VERIFY((m_Desc.SliceCount % 6) == 0, "ImageStorage: Invalid number of slices for cubemap array");
    }
    else
    {
        HK_VERIFY(m_Desc.SliceCount >= 1, "ImageStorage: Invalid number of slices");
    }

    TextureFormatInfo const& info = GetTextureFormatInfo(m_Desc.Format);

    bool bCompressed = info.BlockSize > 1;

    const uint32_t blockSize = info.BlockSize;

    if (bCompressed)
    {
        HK_VERIFY(m_Desc.Type != TEXTURE_1D, "ImageStorage: Compressed 1D textures are not supported");
        HK_VERIFY(m_Desc.Type != TEXTURE_1D_ARRAY, "ImageStorage: Compressed 1D textures are not supported");
        HK_VERIFY(m_Desc.Type != TEXTURE_3D, "ImageStorage: Compressed 3D textures are not supported");
        HK_VERIFY(m_Desc.Width >= blockSize && (m_Desc.Width % blockSize) == 0, "ImageStorage: Width must be a multiple of blockSize for compressed textures");
        HK_VERIFY(m_Desc.Height >= blockSize && (m_Desc.Height % blockSize) == 0, "ImageStorage: Height must be a multiple of blockSize for compressed textures");
    }

    uint32_t numMips = 0;
    uint32_t sz      = std::max(m_Desc.Width, m_Desc.Height);
    if (m_Desc.Type == TEXTURE_3D)
        sz = std::max(sz, m_Desc.Depth);

    if (bCompressed)
        sz /= blockSize;

    while ((sz >>= 1) > 0)
        numMips++;

    HK_VERIFY(m_Desc.NumMipmaps == 1 || m_Desc.NumMipmaps == numMips, "ImageStorage: Invalid number of mipmaps");

    // Calc storage size
    size_t sizeInBytes = 0;
    if (m_Desc.Type == TEXTURE_3D)
    {
        size_t bytesPerPixel = info.BytesPerBlock;
        for (uint32_t i = 0; i < m_Desc.NumMipmaps; i++)
        {
            uint32_t w = std::max<uint32_t>(1, (m_Desc.Width >> i));
            uint32_t h = std::max<uint32_t>(1, (m_Desc.Height >> i));
            uint32_t d = std::max<uint32_t>(1, (m_Desc.Depth >> i));

            sizeInBytes += w * h * d;
        }
        sizeInBytes *= bytesPerPixel;
    }
    else
    {
        if (bCompressed)
        {
            size_t blockSizeInBytes = info.BytesPerBlock;
            for (uint32_t i = 0; i < m_Desc.NumMipmaps; i++)
            {
                uint32_t w = std::max<uint32_t>(blockSize, (m_Desc.Width >> i));
                uint32_t h = std::max<uint32_t>(blockSize, (m_Desc.Height >> i));

                sizeInBytes += w * h;
            }

            HK_ASSERT(sizeInBytes % (blockSize * blockSize) == 0);
            sizeInBytes /= blockSize * blockSize;
            sizeInBytes *= m_Desc.SliceCount;
            sizeInBytes *= blockSizeInBytes;
        }
        else
        {
            size_t bytesPerPixel = info.BytesPerBlock;
            for (uint32_t i = 0; i < m_Desc.NumMipmaps; i++)
            {
                uint32_t w = std::max<uint32_t>(1, (m_Desc.Width >> i));
                uint32_t h = std::max<uint32_t>(1, (m_Desc.Height >> i));

                sizeInBytes += w * h;
            }

            sizeInBytes *= m_Desc.SliceCount;
            sizeInBytes *= bytesPerPixel;
        }
    }

    m_Data.Reset(sizeInBytes);
}

void ImageStorage::Reset()
{
    m_Data.Reset();
}

bool ImageStorage::WriteSubresource(TextureOffset const& Offset, uint32_t Width, uint32_t Height, void const* Bytes)
{
    ImageSubresourceDesc desc;
    desc.SliceIndex  = Offset.Z;
    desc.MipmapIndex = Offset.MipLevel;

    ImageSubresource subresource = GetSubresource(desc);
    HK_VERIFY_R(subresource, "WriteSubresource: Failed to get subresource");

    return subresource.Write(Offset.X, Offset.Y, Width, Height, Bytes);
}

bool ImageStorage::ReadSubresource(TextureOffset const& Offset, uint32_t Width, uint32_t Height, void* Bytes, size_t _SizeInBytes) const
{
    ImageSubresourceDesc desc;
    desc.SliceIndex  = Offset.Z;
    desc.MipmapIndex = Offset.MipLevel;

    ImageSubresource subresource = GetSubresource(desc);
    HK_VERIFY_R(subresource, "ReadSubresource: Failed to get subresource");

    return subresource.Read(Offset.X, Offset.Y, Width, Height, Bytes, _SizeInBytes);
}

ImageSubresource ImageStorage::GetSubresource(ImageSubresourceDesc const& Desc) const
{
    HK_VERIFY_R(Desc.MipmapIndex < m_Desc.NumMipmaps, "GetSubresource: Invalid mipmap index");

    uint32_t maxSlices;

    TextureFormatInfo const& info = GetTextureFormatInfo(m_Desc.Format);

    bool bCompressed = info.BlockSize > 1;

    const uint32_t blockSize = info.BlockSize;

    size_t   offset = 0;
    uint32_t w, h, d;

    // Bytes per block or bytes per pixel
    size_t blockSizeInBytes = info.BytesPerBlock;

    if (m_Desc.Type == TEXTURE_3D)
    {
        for (uint32_t i = 0; i < Desc.MipmapIndex; i++)
        {
            w = std::max<uint32_t>(1, (m_Desc.Width >> i));
            h = std::max<uint32_t>(1, (m_Desc.Height >> i));
            d = std::max<uint32_t>(1, (m_Desc.Depth >> i));

            offset += w * h * d * blockSizeInBytes;
        }

        w = std::max<uint32_t>(1, (m_Desc.Width >> Desc.MipmapIndex));
        h = std::max<uint32_t>(1, (m_Desc.Height >> Desc.MipmapIndex));
        d = std::max<uint32_t>(1, (m_Desc.Depth >> Desc.MipmapIndex));

        maxSlices = d;

        HK_VERIFY_R(Desc.SliceIndex < maxSlices, "GetSubresource: Depth slice is out of bounds");

        offset += Desc.SliceIndex * w * h * blockSizeInBytes;
    }
    else
    {
        maxSlices = m_Desc.SliceCount;

        HK_VERIFY_R(Desc.SliceIndex < maxSlices, "GetSubresource: Array slice is out of bounds");

        if (bCompressed)
        {
            for (uint32_t i = 0; i < Desc.MipmapIndex; i++)
            {
                w = std::max<uint32_t>(blockSize, (m_Desc.Width >> i));
                h = std::max<uint32_t>(blockSize, (m_Desc.Height >> i));

                offset += w * h * m_Desc.SliceCount;
            }

            w = std::max<uint32_t>(blockSize, (m_Desc.Width >> Desc.MipmapIndex));
            h = std::max<uint32_t>(blockSize, (m_Desc.Height >> Desc.MipmapIndex));

            offset += Desc.SliceIndex * w * h;
            HK_ASSERT(offset % (blockSize * blockSize) == 0);
            offset /= blockSize * blockSize;
            offset *= blockSizeInBytes;
        }
        else
        {
            for (uint32_t i = 0; i < Desc.MipmapIndex; i++)
            {
                w = std::max<uint32_t>(1, (m_Desc.Width >> i));
                h = std::max<uint32_t>(1, (m_Desc.Height >> i));

                offset += w * h * m_Desc.SliceCount;
            }

            w = std::max<uint32_t>(1, (m_Desc.Width >> Desc.MipmapIndex));
            h = std::max<uint32_t>(1, (m_Desc.Height >> Desc.MipmapIndex));

            offset += Desc.SliceIndex * w * h;
            offset *= blockSizeInBytes;
        }
    }

    ImageSubresource subres;
    subres.m_Desc        = Desc;
    subres.m_pData       = (uint8_t*)m_Data.GetData() + offset;
    subres.m_SizeInBytes = bCompressed ? w * h / (blockSize * blockSize) * blockSizeInBytes : w * h * blockSizeInBytes;
    subres.m_SliceCount  = maxSlices;
    subres.m_Width       = w;
    subres.m_Height      = h;
    subres.m_Format      = m_Desc.Format;
    return subres;
}

int ImageStorage::NumChannels() const
{
    TextureFormatInfo const& info = GetTextureFormatInfo(m_Desc.Format);

    int numChannels = 0;
    if (info.bHasRed)
        numChannels++;
    if (info.bHasGreen)
        numChannels++;
    if (info.bHasBlue)
        numChannels++;
    if (info.bHasAlpha)
        numChannels++;
    if (info.bHasDepth)
        numChannels++;
    if (info.bHasStencil)
        numChannels++;

    return numChannels;
}

size_t ImageStorage::GetBytesPerPixel() const
{
    return IsCompressed() ? 0 : GetTextureFormatInfo(m_Desc.Format).BytesPerBlock;
}

size_t ImageStorage::GetBlockSizeInBytes() const
{
    return IsCompressed() ? GetTextureFormatInfo(m_Desc.Format).BytesPerBlock : 0;
}

IMAGE_DATA_TYPE ImageStorage::GetDataType() const
{
    return GetTextureFormatInfo(m_Desc.Format).DataType;
}

static stbir_datatype get_stbir_datatype(IMAGE_DATA_TYPE DataType)
{
    switch (DataType)
    {
        case IMAGE_DATA_TYPE_UINT8:
            return STBIR_TYPE_UINT8;
        case IMAGE_DATA_TYPE_UINT16:
            return STBIR_TYPE_UINT16;
        case IMAGE_DATA_TYPE_UINT32:
            return STBIR_TYPE_UINT32;
        case IMAGE_DATA_TYPE_FLOAT:
            return STBIR_TYPE_FLOAT;
        default:
            break;
    }
    HK_ASSERT(0);
    return STBIR_TYPE_UINT8;
}

template <typename Decoder>
static void GenerateMipmaps(ImageStorage& storage, uint32_t SliceIndex, IMAGE_RESAMPLE_EDGE_MODE ResampleMode, IMAGE_RESAMPLE_FILTER filter)
{
    Decoder d;

    ImageSubresourceDesc subres;
    subres.SliceIndex = SliceIndex;
    subres.MipmapIndex = 0;

    ImageSubresource subresource = storage.GetSubresource(subres);

    uint32_t curWidth  = subresource.GetWidth();
    uint32_t curHeight = subresource.GetHeight();

    size_t size = d.GetRequiredMemorySize(curWidth, curHeight);

    HeapBlob blob(size * 2);

    void* tempBuf  = blob.GetData();
    void* tempBuf2 = (uint8_t*)tempBuf + size;

    d.Decode(tempBuf, subresource.GetData(), curWidth, curHeight);

    IMAGE_STORAGE_FLAGS flags = storage.GetDesc().Flags;

    int numChannels        = d.GetNumChannels();
    int alphaChannel       = ((flags & IMAGE_STORAGE_NO_ALPHA) || numChannels != 4) ? STBIR_ALPHA_CHANNEL_NONE : numChannels - 1;
    int stbir_resize_flags = alphaChannel != STBIR_ALPHA_CHANNEL_NONE && (flags & IMAGE_STORAGE_ALPHA_PREMULTIPLIED) ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0;

    stbir_datatype   datatype   = get_stbir_datatype(d.GetDataType());
    stbir_colorspace colorspace = d.IsSRGB() ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR;

    for (uint32_t i = 1; i < storage.GetDesc().NumMipmaps; ++i)
    {
        subres.MipmapIndex = i;

        subresource = storage.GetSubresource(subres);

        uint32_t mipWidth  = subresource.GetWidth();
        uint32_t mipHeight = subresource.GetHeight();

        stbir_resize(tempBuf, curWidth, curHeight, d.GetRowStride(curWidth),
                     tempBuf2, mipWidth, mipHeight, d.GetRowStride(mipWidth),
                     datatype,
                     numChannels,
                     alphaChannel,
                     stbir_resize_flags,
                     (stbir_edge)ResampleMode, (stbir_edge)ResampleMode,
                     (stbir_filter)filter, (stbir_filter)filter,
                     colorspace,
                     NULL);

        d.Encode(subresource.GetData(), tempBuf2, mipWidth, mipHeight);

        Core::Swap(tempBuf, tempBuf2);

        curWidth  = mipWidth;
        curHeight = mipHeight;
    }
}

bool ImageStorage::GenerateMipmaps(uint32_t SliceIndex, ImageMipmapConfig const& MipmapConfig)
{
    if (m_Desc.NumMipmaps <= 1)
        return true;

    if (m_Desc.Type == TEXTURE_3D)
        return false;

    IMAGE_DATA_TYPE DataType = GetTextureFormatInfo(m_Desc.Format).DataType;

    IMAGE_RESAMPLE_EDGE_MODE resampleMode   = MipmapConfig.EdgeMode;
    IMAGE_RESAMPLE_FILTER    resampleFilter = MipmapConfig.Filter;

    switch (DataType)
    {
        case IMAGE_DATA_TYPE_UNKNOWN:
        default:
            HK_ASSERT(0);
            LOG("ImageStorage::GenerateMipmaps: Invalid texture format\n");
            return false;

        case IMAGE_DATA_TYPE_UINT8:
        case IMAGE_DATA_TYPE_UINT16:
        case IMAGE_DATA_TYPE_UINT32:
        case IMAGE_DATA_TYPE_FLOAT:
            break;

        case IMAGE_DATA_TYPE_ENCODED_R4G4B4A4:
            ::GenerateMipmaps<Decoder_R4G4B4A4>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R5G6B5:
            ::GenerateMipmaps<Decoder_R5G6B5>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R5G5B5A1:
            ::GenerateMipmaps<Decoder_R5G5B5A1>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R10G10B10A2:
            ::GenerateMipmaps<Decoder_R10G10B10A2>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R11G11B10F:
            ::GenerateMipmaps<Decoder_R11G11B10F>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_HALF:
            if (m_Desc.Format == TEXTURE_FORMAT_R16_FLOAT)
                ::GenerateMipmaps<Decoder_R16F>(*this, SliceIndex, resampleMode, resampleFilter);
            else if (m_Desc.Format == TEXTURE_FORMAT_RG16_FLOAT)
                ::GenerateMipmaps<Decoder_RG16F>(*this, SliceIndex, resampleMode, resampleFilter);
            else if (m_Desc.Format == TEXTURE_FORMAT_RGBA16_FLOAT)
                ::GenerateMipmaps<Decoder_RGBA16F>(*this, SliceIndex, resampleMode, resampleFilter);
            else
                HK_ASSERT(0);
            return true;

        case IMAGE_DATA_TYPE_ENCODED_DEPTH:
            LOG("ImageStorage::GenerateMipmaps: Mipmap generation for depth texture is not implemented yet.\n");
            return false;

        case IMAGE_DATA_TYPE_COMPRESSED:
            LOG("ImageStorage::GenerateMipmaps: Generating mipmaps for the compressed format is not supported\nYou must generate mipmaps from uncompressed data and then compress each mip level independently.\n");
            return false;
    }

    ImageSubresourceDesc subres;
    subres.SliceIndex = SliceIndex;
    subres.MipmapIndex = 0;

    ImageSubresource subresource = GetSubresource(subres);

    uint32_t curWidth  = subresource.GetWidth();
    uint32_t curHeight = subresource.GetHeight();

    void* data = subresource.GetData();

    IMAGE_STORAGE_FLAGS flags = m_Desc.Flags;

    int numChannels        = NumChannels();
    int alphaChannel       = ((flags & IMAGE_STORAGE_NO_ALPHA) || numChannels != 4) ? STBIR_ALPHA_CHANNEL_NONE : numChannels - 1;
    int stbir_resize_flags = alphaChannel != STBIR_ALPHA_CHANNEL_NONE && (flags & IMAGE_STORAGE_ALPHA_PREMULTIPLIED) ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0;

    stbir_datatype   datatype   = get_stbir_datatype(DataType);
    stbir_colorspace colorspace = m_Desc.Format == TEXTURE_FORMAT_SRGBA8_UNORM || m_Desc.Format == TEXTURE_FORMAT_SBGRA8_UNORM ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR;

    size_t bpp = GetBytesPerPixel();

    for (uint32_t i = 1; i < m_Desc.NumMipmaps; ++i)
    {
        subres.MipmapIndex = i;

        subresource = GetSubresource(subres);

        uint32_t mipWidth  = subresource.GetWidth();
        uint32_t mipHeight = subresource.GetHeight();

        stbir_resize(data, curWidth, curHeight, curWidth * bpp,
                     subresource.GetData(), mipWidth, mipHeight, mipWidth * bpp,
                     datatype,
                     numChannels,
                     alphaChannel,
                     stbir_resize_flags,
                     (stbir_edge)resampleMode, (stbir_edge)resampleMode,
                     (stbir_filter)resampleFilter, (stbir_filter)resampleFilter,
                     colorspace,
                     NULL);

        curWidth  = mipWidth;
        curHeight = mipHeight;
        data      = subresource.GetData();
    }

    return true;
}

bool ImageStorage::GenerateMipmaps(ImageMipmapConfig const& MipmapConfig)
{
    if (m_Desc.Type == TEXTURE_3D)
        return GenerateMipmaps3D(MipmapConfig);

    // TODO: Generate correct mipmaps for Cubemaps.

    for (uint32_t slice = 0; slice < m_Desc.SliceCount; ++slice)
    {
        if (!GenerateMipmaps(slice, MipmapConfig))
            return false;
    }
    return true;
}

bool ImageStorage::GenerateMipmaps3D(ImageMipmapConfig const& MipmapConfig)
{
    if (m_Desc.NumMipmaps <= 1)
        return true;

    LOG("ImageStorage::GenerateMipmaps: Generation of mipmaps for 3D textures is not yet supported.\n");

    // TODO: Generate mipmaps for 3D textures

    return false;
}

void ImageStorage::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt8(m_Desc.Type);
    stream.WriteUInt32(m_Desc.Width);
    stream.WriteUInt32(m_Desc.Height);
    stream.WriteUInt32(m_Desc.Depth);
    stream.WriteUInt32(m_Desc.NumMipmaps);
    stream.WriteUInt8(m_Desc.Format);
    stream.WriteUInt32(m_Desc.Flags);
    stream.WriteUInt32(m_Data.Size());
    stream.Write(m_Data.GetData(), m_Data.Size());
}

void ImageStorage::Read(IBinaryStreamReadInterface& stream)
{
    Reset();

    m_Desc.Type       = (TEXTURE_TYPE)stream.ReadUInt8();
    m_Desc.Width      = stream.ReadUInt32();
    m_Desc.Height     = stream.ReadUInt32();
    m_Desc.Depth      = stream.ReadUInt32();
    m_Desc.NumMipmaps = stream.ReadUInt32();
    m_Desc.Format     = (TEXTURE_FORMAT)stream.ReadUInt8();
    m_Desc.Flags      = (IMAGE_STORAGE_FLAGS)stream.ReadUInt32();

    size_t sizeInBytes = stream.ReadUInt32();

    // TODO: Perform validation

    m_Data.Reset(sizeInBytes);
    stream.Read(m_Data.GetData(), sizeInBytes);
}

ImageStorage CreateImage(ARawImage const& rawImage, bool bConvertHDRIToHalfFloat, ImageMipmapConfig const* pMipmapConfig, IMAGE_STORAGE_FLAGS Flags)
{
    if (!rawImage)
        return {};

    TEXTURE_FORMAT format;
    bool           bAddAlphaChannel = false;
    bool           bSwapChannels    = false;

    switch (rawImage.GetFormat())
    {
        case RAW_IMAGE_FORMAT_UNDEFINED:
        default:
            HK_ASSERT(0);
            return {};

        case RAW_IMAGE_FORMAT_R8:
            bConvertHDRIToHalfFloat = false;
            format                  = TEXTURE_FORMAT_R8_UNORM;
            break;

        case RAW_IMAGE_FORMAT_R8_ALPHA:
            bConvertHDRIToHalfFloat = false;
            format                  = TEXTURE_FORMAT_RG8_UNORM;
            break;

        case RAW_IMAGE_FORMAT_RGB8:
            bAddAlphaChannel        = true;
            bConvertHDRIToHalfFloat = false;
            format                  = TEXTURE_FORMAT_SRGBA8_UNORM;
            break;

        case RAW_IMAGE_FORMAT_BGR8:
            bAddAlphaChannel        = true;
            bConvertHDRIToHalfFloat = false;
            format                  = TEXTURE_FORMAT_SBGRA8_UNORM;
            break;

        case RAW_IMAGE_FORMAT_RGBA8:
            bConvertHDRIToHalfFloat = false;
            format                  = TEXTURE_FORMAT_SRGBA8_UNORM;
            break;

        case RAW_IMAGE_FORMAT_BGRA8:
            bConvertHDRIToHalfFloat = false;
            format                  = TEXTURE_FORMAT_SBGRA8_UNORM;
            break;

        case RAW_IMAGE_FORMAT_R32_FLOAT:
            format = bConvertHDRIToHalfFloat ? TEXTURE_FORMAT_R16_FLOAT : TEXTURE_FORMAT_R32_FLOAT;
            break;

        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
            format = bConvertHDRIToHalfFloat ? TEXTURE_FORMAT_RG16_FLOAT : TEXTURE_FORMAT_RG32_FLOAT;
            break;

        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
            format = bConvertHDRIToHalfFloat ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGB32_FLOAT;
            if (bConvertHDRIToHalfFloat)
                bAddAlphaChannel = true;
            break;

        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
            bSwapChannels = true;
            format        = bConvertHDRIToHalfFloat ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGB32_FLOAT;
            if (bConvertHDRIToHalfFloat)
                bAddAlphaChannel = true;
            break;

        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
            format = bConvertHDRIToHalfFloat ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGBA32_FLOAT;
            break;

        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            bSwapChannels = true;
            format        = bConvertHDRIToHalfFloat ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGBA32_FLOAT;
            break;
    }

    ImageStorageDesc desc;
    desc.Type       = TEXTURE_2D;
    desc.Format     = format;
    desc.Width      = rawImage.GetWidth();
    desc.Height     = rawImage.GetHeight();
    desc.SliceCount = 1;
    desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
    desc.Flags      = Flags;

    ImageStorage storage(desc);

    ImageSubresourceDesc subres;
    subres.SliceIndex    = 0;
    subres.MipmapIndex = 0;

    ImageSubresource subresource = storage.GetSubresource(subres);

    if (!bAddAlphaChannel && !bSwapChannels)
    {
        // Fast path
        if (bConvertHDRIToHalfFloat)
        {
            switch (storage.NumChannels())
            {
                case 1:
                    Decoder_R16F().Encode(subresource.GetData(), rawImage.GetData(), subresource.GetWidth(), subresource.GetHeight());
                    break;
                case 2:
                    Decoder_RG16F().Encode(subresource.GetData(), rawImage.GetData(), subresource.GetWidth(), subresource.GetHeight());
                    break;
                case 4:
                    Decoder_RGBA16F().Encode(subresource.GetData(), rawImage.GetData(), subresource.GetWidth(), subresource.GetHeight());
                    break;
                default:
                    // Never happen
                    HK_ASSERT(0);
                    break;
            }
        }
        else
        {
            subresource.Write(0, 0, rawImage.GetWidth(), rawImage.GetHeight(), rawImage.GetData());
        }
    }
    else
    {
        const int r = bSwapChannels ? 2 : 0;
        const int g = 1;
        const int b = bSwapChannels ? 0 : 2;

        int dstNumChannels = storage.NumChannels();
        int srcNumChannels = rawImage.NumChannels();

        HK_ASSERT(dstNumChannels >= 3 && srcNumChannels >= 3);

        if (bConvertHDRIToHalfFloat)
        {
            uint16_t*      dst   = (uint16_t*)subresource.GetData();
            uint16_t*      dst_e = dst + subresource.GetWidth() * subresource.GetHeight() * storage.NumChannels();
            float const*   src   = (float const*)rawImage.GetData();
            const uint16_t one   = f32tof16(1);

            while (dst < dst_e)
            {
                dst[0] = f32tof16(src[r]);
                dst[1] = f32tof16(src[g]);
                dst[2] = f32tof16(src[b]);

                if (bAddAlphaChannel)
                    dst[3] = one;

                dst += dstNumChannels;
                src += srcNumChannels;
            }
        }
        else
        {
            IMAGE_DATA_TYPE dataType = GetTextureFormatInfo(format).DataType;

            if (dataType == IMAGE_DATA_TYPE_UINT8)
            {
                uint8_t*       dst   = (uint8_t*)subresource.GetData();
                uint8_t*       dst_e = dst + subresource.GetWidth() * subresource.GetHeight() * storage.NumChannels();
                uint8_t const* src   = (uint8_t const*)rawImage.GetData();

                while (dst < dst_e)
                {
                    dst[0] = src[r];
                    dst[1] = src[g];
                    dst[2] = src[b];

                    if (bAddAlphaChannel)
                        dst[3] = 255;

                    dst += dstNumChannels;
                    src += srcNumChannels;
                }
            }
            else if (dataType == IMAGE_DATA_TYPE_FLOAT)
            {
                float*       dst   = (float*)subresource.GetData();
                float*       dst_e = dst + subresource.GetWidth() * subresource.GetHeight() * storage.NumChannels();
                float const* src   = (float const*)rawImage.GetData();

                while (dst < dst_e)
                {
                    dst[0] = src[r];
                    dst[1] = src[g];
                    dst[2] = src[b];

                    if (bAddAlphaChannel)
                        dst[3] = 1.0f;

                    dst += dstNumChannels;
                    src += srcNumChannels;
                }
            }
            else
            {
                // Never happen
                HK_ASSERT(0);
            }
        }
    }

    if (pMipmapConfig)
        storage.GenerateMipmaps(*pMipmapConfig);

    return storage;
}

ImageStorage CreateImage(IBinaryStreamReadInterface& Stream, ImageMipmapConfig const* pMipmapConfig, IMAGE_STORAGE_FLAGS Flags, TEXTURE_FORMAT Format)
{
    using namespace TextureBlockCompression;
    switch (Format)
    {
        case TEXTURE_FORMAT_UNDEFINED: {
            ARawImage rawImage = CreateRawImage(Stream);
            if (!rawImage)
                return {};

            const bool bConvertHDRIToHalfFloat = true;
            return CreateImage(rawImage, bConvertHDRIToHalfFloat, pMipmapConfig, Flags);
        }
        case TEXTURE_FORMAT_R8_UINT:
        case TEXTURE_FORMAT_R8_SINT:
        case TEXTURE_FORMAT_R8_UNORM:
        case TEXTURE_FORMAT_R8_SNORM:
        case TEXTURE_FORMAT_RG8_UINT:
        case TEXTURE_FORMAT_RG8_SINT:
        case TEXTURE_FORMAT_RG8_UNORM:
        case TEXTURE_FORMAT_RG8_SNORM: {
            ARawImage rawImage = CreateRawImage(Stream, GetTextureFormatInfo(Format).bHasGreen ? RAW_IMAGE_FORMAT_R8_ALPHA : RAW_IMAGE_FORMAT_R8);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            subresource.Write(0, 0, rawImage.GetWidth(), rawImage.GetHeight(), rawImage.GetData());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            return storage;
        }
        case TEXTURE_FORMAT_BGRA4_UNORM: {
            ARawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_BGRA8);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            Decoder_R4G4B4A4().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            break;
        }
        case TEXTURE_FORMAT_B5G6R5_UNORM: {
            ARawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_BGR8);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            Decoder_R5G6B5().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            break;
        }
        case TEXTURE_FORMAT_B5G5R5A1_UNORM: {
            ARawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_BGRA8);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            Decoder_R5G5B5A1().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            break;
        }
        case TEXTURE_FORMAT_RGBA8_UINT:
        case TEXTURE_FORMAT_RGBA8_SINT:
        case TEXTURE_FORMAT_RGBA8_UNORM:
        case TEXTURE_FORMAT_RGBA8_SNORM:
        case TEXTURE_FORMAT_BGRA8_UNORM:
        case TEXTURE_FORMAT_SRGBA8_UNORM:
        case TEXTURE_FORMAT_SBGRA8_UNORM: {
            ARawImage rawImage = CreateRawImage(Stream, (Format == TEXTURE_FORMAT_BGRA8_UNORM || Format == TEXTURE_FORMAT_SBGRA8_UNORM) ? RAW_IMAGE_FORMAT_BGRA8 : RAW_IMAGE_FORMAT_RGBA8);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            subresource.Write(0, 0, rawImage.GetWidth(), rawImage.GetHeight(), rawImage.GetData());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            return storage;
        }
        case TEXTURE_FORMAT_R10G10B10A2_UNORM: {
            ARawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGBA32_FLOAT); // FIXME: Maybe BGRA?
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            Decoder_R10G10B10A2().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            break;
        }
        case TEXTURE_FORMAT_R11G11B10_FLOAT: {
            ARawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGB32_FLOAT); // FIXME: Maybe BGR?
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            Decoder_R11G11B10F().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            break;
        }
        case TEXTURE_FORMAT_R16_FLOAT:
        case TEXTURE_FORMAT_RG16_FLOAT:
        case TEXTURE_FORMAT_RGBA16_FLOAT: {
            ARawImage rawImage;
            if (Format == TEXTURE_FORMAT_R16_FLOAT)
                rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_R32_FLOAT);
            else if (Format == TEXTURE_FORMAT_RG16_FLOAT)
                rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT);
            else if (Format == TEXTURE_FORMAT_RGBA16_FLOAT)
                rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGBA32_FLOAT);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            if (Format == TEXTURE_FORMAT_R16_FLOAT)
                Decoder_R16F().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());
            else if (Format == TEXTURE_FORMAT_RG16_FLOAT)
                Decoder_RG16F().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());
            else if (Format == TEXTURE_FORMAT_RGBA16_FLOAT)
                Decoder_RGBA16F().Encode(subresource.GetData(), rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);
            break;
        }
        case TEXTURE_FORMAT_R32_FLOAT:
        case TEXTURE_FORMAT_RG32_FLOAT:
        case TEXTURE_FORMAT_RGB32_FLOAT:
        case TEXTURE_FORMAT_RGBA32_FLOAT: {
            ARawImage rawImage;
            if (Format == TEXTURE_FORMAT_R32_FLOAT)
                rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_R32_FLOAT);
            else if (Format == TEXTURE_FORMAT_RG32_FLOAT)
                rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT);
            else if (Format == TEXTURE_FORMAT_RGB32_FLOAT)
                rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGB32_FLOAT);
            else if (Format == TEXTURE_FORMAT_RGBA32_FLOAT)
                rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGBA32_FLOAT);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            subresource.Write(0, 0, rawImage.GetWidth(), rawImage.GetHeight(), rawImage.GetData());

            if (pMipmapConfig)
                storage.GenerateMipmaps(*pMipmapConfig);

            return storage;
        }
        case TEXTURE_FORMAT_R16_UINT:
        case TEXTURE_FORMAT_R16_SINT:
        case TEXTURE_FORMAT_R16_UNORM:
        case TEXTURE_FORMAT_R16_SNORM:
        case TEXTURE_FORMAT_RG16_UINT:
        case TEXTURE_FORMAT_RG16_SINT:
        case TEXTURE_FORMAT_RG16_UNORM:
        case TEXTURE_FORMAT_RG16_SNORM:
        case TEXTURE_FORMAT_RGBA16_UINT:
        case TEXTURE_FORMAT_RGBA16_SINT:
        case TEXTURE_FORMAT_RGBA16_UNORM:
        case TEXTURE_FORMAT_RGBA16_SNORM:
        case TEXTURE_FORMAT_R32_UINT:
        case TEXTURE_FORMAT_R32_SINT:
        case TEXTURE_FORMAT_RG32_UINT:
        case TEXTURE_FORMAT_RG32_SINT:
        case TEXTURE_FORMAT_RGB32_UINT:
        case TEXTURE_FORMAT_RGB32_SINT:
        case TEXTURE_FORMAT_RGBA32_UINT:
        case TEXTURE_FORMAT_RGBA32_SINT:
            LOG("CreateImage: Loading 16 and 32 bit integer images is not yet supported.\n");
            break;
        case TEXTURE_FORMAT_D16:
        case TEXTURE_FORMAT_D24S8:
        case TEXTURE_FORMAT_X24G8_UINT:
        case TEXTURE_FORMAT_D32:
        case TEXTURE_FORMAT_D32S8:
        case TEXTURE_FORMAT_X32G8_UINT:
            LOG("CreateImage: Loading depth images is not yet supported.\n");
            break;
        case TEXTURE_FORMAT_BC1_UNORM:
        case TEXTURE_FORMAT_BC1_UNORM_SRGB:
        case TEXTURE_FORMAT_BC2_UNORM:
        case TEXTURE_FORMAT_BC2_UNORM_SRGB:
        case TEXTURE_FORMAT_BC3_UNORM:
        case TEXTURE_FORMAT_BC3_UNORM_SRGB:
        case TEXTURE_FORMAT_BC4_UNORM:
        case TEXTURE_FORMAT_BC4_SNORM:
        case TEXTURE_FORMAT_BC5_UNORM:
        case TEXTURE_FORMAT_BC5_SNORM:
        case TEXTURE_FORMAT_BC7_UNORM:
        case TEXTURE_FORMAT_BC7_UNORM_SRGB: {
            bool BC4 = Format == TEXTURE_FORMAT_BC4_UNORM || Format == TEXTURE_FORMAT_BC4_SNORM;
            bool BC5 = Format == TEXTURE_FORMAT_BC5_UNORM || Format == TEXTURE_FORMAT_BC5_SNORM;

            RAW_IMAGE_FORMAT rawImageFormat;
            uint32_t bpp;
            
            if (BC4)
            {
                rawImageFormat = RAW_IMAGE_FORMAT_R8;
                bpp            = 1;
            }
            else if (BC5)
            {
                rawImageFormat = RAW_IMAGE_FORMAT_R8_ALPHA;
                bpp            = 2;
            }
            else
            {
                rawImageFormat = RAW_IMAGE_FORMAT_RGBA8;
                bpp            = 4;
            }

            ARawImage rawImage = CreateRawImage(Stream, rawImageFormat);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            void (*CompressionRoutine)(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height);
            switch (Format)
            {
                case TEXTURE_FORMAT_BC1_UNORM:
                case TEXTURE_FORMAT_BC1_UNORM_SRGB:
                    CompressionRoutine = CompressBC1;
                    break;
                case TEXTURE_FORMAT_BC2_UNORM:
                case TEXTURE_FORMAT_BC2_UNORM_SRGB:
                    CompressionRoutine = CompressBC2;
                    break;
                case TEXTURE_FORMAT_BC3_UNORM:
                case TEXTURE_FORMAT_BC3_UNORM_SRGB:
                    CompressionRoutine = CompressBC3;
                    break;
                case TEXTURE_FORMAT_BC4_UNORM:
                case TEXTURE_FORMAT_BC4_SNORM:
                    CompressionRoutine = CompressBC4;
                    break;
                case TEXTURE_FORMAT_BC5_UNORM:
                case TEXTURE_FORMAT_BC5_SNORM:
                    CompressionRoutine = CompressBC5;
                    break;
                case TEXTURE_FORMAT_BC7_UNORM:
                case TEXTURE_FORMAT_BC7_UNORM_SRGB:
                    CompressionRoutine = CompressBC7;
                    break;
                default:
                    HK_ASSERT(0);

                    // Keep compiler happy
                    CompressionRoutine = nullptr;
                    break;
            }

            CompressionRoutine(rawImage.GetData(), subresource.GetData(), subresource.GetWidth(), subresource.GetHeight());

            if (pMipmapConfig)
            {
                TextureFormatInfo const& info = GetTextureFormatInfo(Format);

                uint32_t curWidth  = subresource.GetWidth();
                uint32_t curHeight = subresource.GetHeight();

                HeapBlob blob(std::max<uint32_t>(info.BlockSize, curWidth >> 1) * std::max<uint32_t>(info.BlockSize, curHeight >> 1) * bpp);

                void* data = rawImage.GetData();
                void* temp = blob.GetData();

                IMAGE_RESAMPLE_EDGE_MODE resampleMode   = pMipmapConfig->EdgeMode;
                IMAGE_RESAMPLE_FILTER    resampleFilter = pMipmapConfig->Filter;

                int numChannels        = bpp;
                int alphaChannel       = ((Flags & IMAGE_STORAGE_NO_ALPHA) || numChannels != 4) ? STBIR_ALPHA_CHANNEL_NONE : numChannels - 1;
                int stbir_resize_flags = alphaChannel != STBIR_ALPHA_CHANNEL_NONE && (Flags & IMAGE_STORAGE_ALPHA_PREMULTIPLIED) ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0;

                stbir_datatype   datatype   = STBIR_TYPE_UINT8;
                stbir_colorspace colorspace = Format == TEXTURE_FORMAT_BC1_UNORM_SRGB ||
                    Format == TEXTURE_FORMAT_BC2_UNORM_SRGB ||
                    Format == TEXTURE_FORMAT_BC3_UNORM_SRGB ||
                    Format == TEXTURE_FORMAT_BC7_UNORM_SRGB ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR;

                for (uint32_t i = 1; i < desc.NumMipmaps; ++i)
                {
                    subres.MipmapIndex = i;

                    subresource = storage.GetSubresource(subres);

                    uint32_t mipWidth  = subresource.GetWidth();
                    uint32_t mipHeight = subresource.GetHeight();

                    stbir_resize(data, curWidth, curHeight, curWidth * bpp,
                                 temp, mipWidth, mipHeight, mipWidth * bpp,
                                 datatype,
                                 numChannels,
                                 alphaChannel,
                                 stbir_resize_flags,
                                 (stbir_edge)resampleMode, (stbir_edge)resampleMode,
                                 (stbir_filter)resampleFilter, (stbir_filter)resampleFilter,
                                 colorspace,
                                 NULL);

                    curWidth  = mipWidth;
                    curHeight = mipHeight;
                    Core::Swap(data, temp);

                    CompressionRoutine(data, subresource.GetData(), mipWidth, mipHeight);
                }
            }
            return storage;
        }
        case TEXTURE_FORMAT_BC6H_UFLOAT:
        case TEXTURE_FORMAT_BC6H_SFLOAT: {
            ARawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGBA32_FLOAT);
            if (!rawImage)
                return {};

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
            desc.Flags      = Flags;

            ImageStorage storage(desc);

            ImageSubresourceDesc subres;
            subres.SliceIndex  = 0;
            subres.MipmapIndex = 0;

            ImageSubresource subresource = storage.GetSubresource(subres);

            bool bSigned = (Format == TEXTURE_FORMAT_BC6H_SFLOAT);
            CompressBC6h(rawImage.GetData(), subresource.GetData(), subresource.GetWidth(), subresource.GetHeight(), bSigned);

            if (pMipmapConfig)
            {
                TextureFormatInfo const& info = GetTextureFormatInfo(Format);

                uint32_t bpp = 4 * sizeof(float);

                uint32_t curWidth  = subresource.GetWidth();
                uint32_t curHeight = subresource.GetHeight();

                HeapBlob blob(std::max<uint32_t>(info.BlockSize, curWidth >> 1) * std::max<uint32_t>(info.BlockSize, curHeight >> 1) * bpp);

                void* data = rawImage.GetData();
                void* temp = blob.GetData();

                IMAGE_RESAMPLE_EDGE_MODE resampleMode   = pMipmapConfig->EdgeMode;
                IMAGE_RESAMPLE_FILTER    resampleFilter = pMipmapConfig->Filter;

                const int numChannels        = 4;
                const int alphaChannel       = STBIR_ALPHA_CHANNEL_NONE;
                const int stbir_resize_flags = 0;

                const stbir_datatype   datatype   = STBIR_TYPE_FLOAT;
                const stbir_colorspace colorspace = STBIR_COLORSPACE_LINEAR;

                for (uint32_t i = 1; i < desc.NumMipmaps; ++i)
                {
                    subres.MipmapIndex = i;

                    subresource = storage.GetSubresource(subres);

                    uint32_t mipWidth  = subresource.GetWidth();
                    uint32_t mipHeight = subresource.GetHeight();

                    stbir_resize(data, curWidth, curHeight, curWidth * bpp,
                                 temp, mipWidth, mipHeight, mipWidth * bpp,
                                 datatype,
                                 numChannels,
                                 alphaChannel,
                                 stbir_resize_flags,
                                 (stbir_edge)resampleMode, (stbir_edge)resampleMode,
                                 (stbir_filter)resampleFilter, (stbir_filter)resampleFilter,
                                 colorspace,
                                 NULL);

                    curWidth  = mipWidth;
                    curHeight = mipHeight;
                    Core::Swap(data, temp);

                    CompressBC6h(data, subresource.GetData(), mipWidth, mipHeight, bSigned);
                }
            }
            return storage;
        }
        default:
            HK_ASSERT(0);
            break;
    }
    return {};
}

ImageStorage CreateImage(AStringView FileName, ImageMipmapConfig const* pMipmapConfig, IMAGE_STORAGE_FLAGS Flags, TEXTURE_FORMAT Format)
{
    AFileStream stream;
    if (!stream.OpenRead(FileName))
        return {};

    return CreateImage(stream, pMipmapConfig, Flags, Format);
}

ImageStorage LoadSkyboxImages(SkyboxImportSettings const& ImportSettings)
{
    ARawImage rawImage[6];

    for (uint32_t i = 0; i < 6; i++)
    {
        rawImage[i] = CreateRawImage(ImportSettings.Faces[i], ImportSettings.bHDRI ? RAW_IMAGE_FORMAT_RGB32_FLOAT : RAW_IMAGE_FORMAT_RGBA8);
        if (!rawImage[i])
            return {};

        if (rawImage[i].GetWidth() != rawImage[0].GetWidth() || rawImage[i].GetWidth() != rawImage[i].GetHeight())
        {
            LOG("LoadSkyboxImages: Invalid image size\n");
            return {};
        }
    }

    ImageStorageDesc desc;
    desc.Type       = TEXTURE_CUBE;
    desc.Width      = rawImage[0].GetWidth();
    desc.Height     = rawImage[0].GetHeight();
    desc.SliceCount = 6;
    desc.NumMipmaps = 1;
    desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

    if (ImportSettings.bHDRI)
    {
        desc.Format = TEXTURE_FORMAT_R11G11B10_FLOAT;

        ImageStorage storage(desc);

        ImageSubresourceDesc subres;
        subres.MipmapIndex = 0;
        for (uint32_t i = 0; i < 6; i++)
        {
            subres.SliceIndex = i;

            ImageSubresource subresource = storage.GetSubresource(subres);

            if (ImportSettings.HDRIScale != 1.0f || ImportSettings.HDRIPow != 1.0f)
            {
                float* HDRI  = (float*)rawImage[i].GetData();
                int    count = rawImage[i].GetWidth() * rawImage[i].GetHeight() * 3;
                for (int j = 0; j < count; j += 3)
                {
                    HDRI[j]     = Math::Pow(HDRI[j + 0] * ImportSettings.HDRIScale, ImportSettings.HDRIPow);
                    HDRI[j + 1] = Math::Pow(HDRI[j + 1] * ImportSettings.HDRIScale, ImportSettings.HDRIPow);
                    HDRI[j + 2] = Math::Pow(HDRI[j + 2] * ImportSettings.HDRIScale, ImportSettings.HDRIPow);
                }
            }

            Decoder_R11G11B10F().Encode(subresource.GetData(), rawImage[i].GetData(), subresource.GetWidth(), subresource.GetHeight());
        }

        return storage;
    }
    else
    {
        desc.Format = TEXTURE_FORMAT_SRGBA8_UNORM;

        ImageStorage storage(desc);

        ImageSubresourceDesc subres;
        subres.MipmapIndex = 0;
        for (uint32_t i = 0; i < 6; i++)
        {
            subres.SliceIndex = i;

            ImageSubresource subresource = storage.GetSubresource(subres);
            subresource.Write(0, 0, subresource.GetWidth(), subresource.GetHeight(), rawImage[i].GetData());
        }

        return storage;
    }
}

template <typename Decoder>
static void ResampleImage(ImageResampleParams const& Desc, void* pDest)
{
    Decoder d;

    size_t size  = d.GetRequiredMemorySize(Desc.Width, Desc.Height);
    size_t size2 = d.GetRequiredMemorySize(Desc.ScaledWidth, Desc.ScaledHeight);

    HeapBlob blob(size + size2);

    void* tempBuf  = blob.GetData();
    void* tempBuf2 = (uint8_t*)tempBuf + size;

    d.Decode(tempBuf, Desc.pImage, Desc.Width, Desc.Height);

    int numChannels        = d.GetNumChannels();
    int alphaChannel       = Desc.AlphaChannel >= 0 && Desc.AlphaChannel < numChannels ? Desc.AlphaChannel : STBIR_ALPHA_CHANNEL_NONE;
    int stbir_resize_flags = alphaChannel != STBIR_ALPHA_CHANNEL_NONE && Desc.bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0;

    stbir_datatype   datatype   = get_stbir_datatype(d.GetDataType());
    stbir_colorspace colorspace = d.IsSRGB() ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR;

    int result =
        stbir_resize(tempBuf, Desc.Width, Desc.Height, d.GetRowStride(Desc.Width),
                     tempBuf2, Desc.ScaledWidth, Desc.ScaledHeight, d.GetRowStride(Desc.ScaledWidth),
                     datatype,
                     numChannels,
                     alphaChannel,
                     stbir_resize_flags,
                     (stbir_edge)Desc.HorizontalEdgeMode, (stbir_edge)Desc.VerticalEdgeMode,
                     (stbir_filter)Desc.HorizontalFilter, (stbir_filter)Desc.VerticalFilter,
                     colorspace,
                     NULL);

    HK_ASSERT(result == 1);
    HK_UNUSED(result);

    d.Encode(pDest, tempBuf2, Desc.ScaledWidth, Desc.ScaledHeight);
}

bool ResampleImage(ImageResampleParams const& Desc, void* pDest)
{
    TextureFormatInfo const& info = GetTextureFormatInfo(Desc.Format);

    switch (info.DataType)
    {
        case IMAGE_DATA_TYPE_UNKNOWN:
        default:
            HK_ASSERT(0);
            LOG("ResampleImage: Invalid image data type\n");
            return false;
        case IMAGE_DATA_TYPE_UINT8:
        case IMAGE_DATA_TYPE_UINT16:
        case IMAGE_DATA_TYPE_UINT32:
        case IMAGE_DATA_TYPE_FLOAT:
            break;
        case IMAGE_DATA_TYPE_HALF:
            if (Desc.Format == TEXTURE_FORMAT_R16_FLOAT)
                ResampleImage<Decoder_R16F>(Desc, pDest);
            else if (Desc.Format == TEXTURE_FORMAT_RG16_FLOAT)
                ResampleImage<Decoder_RG16F>(Desc, pDest);
            else if (Desc.Format == TEXTURE_FORMAT_RGBA16_FLOAT)
                ResampleImage<Decoder_RGBA16F>(Desc, pDest);
            else
            {
                HK_ASSERT(0);
                return false;
            }
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R4G4B4A4:
            ResampleImage<Decoder_R4G4B4A4>(Desc, pDest);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R5G6B5:
            ResampleImage<Decoder_R5G6B5>(Desc, pDest);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R5G5B5A1:
            ResampleImage<Decoder_R5G5B5A1>(Desc, pDest);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R10G10B10A2:
            ResampleImage<Decoder_R10G10B10A2>(Desc, pDest);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R11G11B10F:
            ResampleImage<Decoder_R11G11B10F>(Desc, pDest);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_DEPTH:
            LOG("ResampleImage: Unsupported image data type\n");
            return false;
        case IMAGE_DATA_TYPE_COMPRESSED:
            LOG("ResampleImage: Unsupported image data type\n");
            return false;
    }

    int numChannels = 0;
    if (info.bHasRed)
        numChannels++;
    if (info.bHasGreen)
        numChannels++;
    if (info.bHasBlue)
        numChannels++;
    if (info.bHasAlpha)
        numChannels++;
    if (info.bHasDepth)
        numChannels++;
    if (info.bHasStencil)
        numChannels++;

    int result =
        stbir_resize(Desc.pImage, Desc.Width, Desc.Height, numChannels * Desc.Width,
                     pDest, Desc.ScaledWidth, Desc.ScaledHeight, numChannels * Desc.ScaledWidth,
                     get_stbir_datatype(info.DataType),
                     numChannels,
                     Desc.AlphaChannel,
                     Desc.bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                     (stbir_edge)Desc.HorizontalEdgeMode, (stbir_edge)Desc.VerticalEdgeMode,
                     (stbir_filter)Desc.HorizontalFilter, (stbir_filter)Desc.VerticalFilter,
                     info.bSRGB ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR,
                     NULL);

    HK_ASSERT(result == 1);
    HK_UNUSED(result);

    return true;
}
