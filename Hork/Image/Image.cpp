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

#include "Image.h"
#include "ImageEncoders.h"
#include <Hork/Core/CoreApplication.h>

#include <Hork/Core/Logger.h>
#include <Hork/Core/Color.h>

#define STBIR_MALLOC(sz, context) Hk::Core::GetHeapAllocator<Hk::HEAP_TEMP>().Alloc(sz)
#define STBIR_FREE(p, context)    Hk::Core::GetHeapAllocator<Hk::HEAP_TEMP>().Free(p)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#define STBIR_MAX_CHANNELS 4
#include <stb/stb_image_resize.h>

HK_NAMESPACE_BEGIN

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

TEXTURE_FORMAT FindTextureFormat(StringView name)
{
    for (auto const& info : TexFormat)
    {
        if (!name.Icmp(info.Name))
            return info.Format;
    }
    LOG("FindTextureFormat: texture format {} is not found\n", name);
    return TEXTURE_FORMAT_UNDEFINED;
}

IMAGE_RESAMPLE_EDGE_MODE GetResampleEdgeMode(StringView name)
{
    if (!name.Icmp("clamp"))
        return IMAGE_RESAMPLE_EDGE_CLAMP;
    if (!name.Icmp("reflect"))
        return IMAGE_RESAMPLE_EDGE_REFLECT;
    if (!name.Icmp("wrap"))
        return IMAGE_RESAMPLE_EDGE_WRAP;
    if (!name.Icmp("zero"))
        return IMAGE_RESAMPLE_EDGE_ZERO;
    LOG("GetResampleEdgeMode: unknown mode {}, return wrap mode\n", name);
    return IMAGE_RESAMPLE_EDGE_WRAP;
}

IMAGE_RESAMPLE_FILTER GetResampleFilter(StringView name)
{
    if (!name.Icmp("box"))
        return IMAGE_RESAMPLE_FILTER_BOX;
    if (!name.Icmp("triangle"))
        return IMAGE_RESAMPLE_FILTER_TRIANGLE;
    if (!name.Icmp("cubicspline"))
        return IMAGE_RESAMPLE_FILTER_CUBICBSPLINE;
    if (!name.Icmp("catmullrom"))
        return IMAGE_RESAMPLE_FILTER_CATMULLROM;
    if (!name.Icmp("mitchell"))
        return IMAGE_RESAMPLE_FILTER_MITCHELL;
    LOG("GetResampleFilter: unknown filter {}, return mitchell filter\n", name);
    return IMAGE_RESAMPLE_FILTER_MITCHELL;
}

IMAGE_RESAMPLE_FILTER_3D GetResampleFilter3D(StringView name)
{
    if (!name.Icmp("average"))
        return IMAGE_RESAMPLE_FILTER_3D_AVERAGE;
    if (!name.Icmp("min"))
        return IMAGE_RESAMPLE_FILTER_3D_MIN;
    if (!name.Icmp("max"))
        return IMAGE_RESAMPLE_FILTER_3D_MAX;
    LOG("GetResampleFilter3D: unknown filter {}, return average filter\n", name);
    return IMAGE_RESAMPLE_FILTER_3D_AVERAGE;
}

uint32_t CalcNumMips(TEXTURE_FORMAT Format, uint32_t Width, uint32_t Height, uint32_t Depth)
{
    HK_ASSERT(Width > 0);
    HK_ASSERT(Height > 0);
    HK_ASSERT(Depth > 0);

    TextureFormatInfo const& info = GetTextureFormatInfo(Format);

    bool bCompressed = IsCompressedFormat(Format);

    const uint32_t blockSize = info.BlockSize;

    if (bCompressed)
    {
        HK_VERIFY_R(Depth == 1, "CalcNumMips: Compressed 3D textures are not supported");
        HK_VERIFY_R((Width % blockSize) == 0, "CalcNumMips: Width must be a multiple of blockSize for compressed textures");
        HK_VERIFY_R((Height % blockSize) == 0, "CalcNumMips: Height must be a multiple of blockSize for compressed textures");
    }

    uint32_t numMips = 0;
    uint32_t sz      = Math::Max3(Width, Height, Depth);

    if (bCompressed)
        sz /= blockSize;

    numMips = Math::Log2(sz) + 1;

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
        Core::Memcpy(m_pData, Bytes, Width * Height * blockSizeInBytes);
    }
    else
    {
        size_t offset = (Y * viewHeight + X) * blockSizeInBytes;

        uint8_t* ptr = m_pData + offset;
        uint8_t const* src = (uint8_t const*)Bytes;
        for (uint32_t y = 0; y < Height; ++y)
        {
            Core::Memcpy(ptr, src, Width * blockSizeInBytes);
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
        Core::Memcpy(Bytes, m_pData, Width * Height * blockSizeInBytes);
    }
    else
    {
        uint8_t const* ptr = m_pData + offset;
        uint8_t* dst = (uint8_t*)Bytes;
        for (uint32_t y = 0; y < Height; ++y)
        {
            Core::Memcpy(dst, ptr, Width * blockSizeInBytes);
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
        HK_VERIFY((m_Desc.Width % blockSize) == 0, "ImageStorage: Width must be a multiple of blockSize for compressed textures");
        HK_VERIFY((m_Desc.Height % blockSize) == 0, "ImageStorage: Height must be a multiple of blockSize for compressed textures");

        HK_VERIFY(m_Desc.NumMipmaps == 1 || IsPowerOfTwo(m_Desc.Width), "ImageStorage: Width must be a power of two for compressed mipmapped textures");
        HK_VERIFY(m_Desc.NumMipmaps == 1 || IsPowerOfTwo(m_Desc.Height), "ImageStorage: Width must be a power of two for compressed mipmapped textures");
    }

    uint32_t numMips = 0;
    uint32_t sz      = std::max(m_Desc.Width, m_Desc.Height);
    if (m_Desc.Type == TEXTURE_3D)
        sz = std::max(sz, m_Desc.Depth);

    if (bCompressed)
        sz /= blockSize;

    numMips = Math::Log2(sz) + 1;

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
        for (uint32_t i = 0; i < m_Desc.NumMipmaps; i++)
        {
            uint32_t w = std::max<uint32_t>(blockSize, (m_Desc.Width >> i));
            uint32_t h = std::max<uint32_t>(blockSize, (m_Desc.Height >> i));

            sizeInBytes += w * h;
        }
            
        if (blockSize > 1)
        {
            HK_ASSERT(sizeInBytes % (blockSize * blockSize) == 0);
            sizeInBytes /= blockSize * blockSize;
        }
        
        sizeInBytes *= m_Desc.SliceCount;
        sizeInBytes *= info.BytesPerBlock;
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

        for (uint32_t i = 0; i < Desc.MipmapIndex; i++)
        {
            w = std::max<uint32_t>(blockSize, (m_Desc.Width >> i));
            h = std::max<uint32_t>(blockSize, (m_Desc.Height >> i));

            offset += w * h * m_Desc.SliceCount;
        }
        
        w = std::max<uint32_t>(blockSize, (m_Desc.Width >> Desc.MipmapIndex));
        h = std::max<uint32_t>(blockSize, (m_Desc.Height >> Desc.MipmapIndex));
        
        offset += Desc.SliceIndex * w * h;
        
        if (blockSize > 1)
        {
            HK_ASSERT(offset % (blockSize * blockSize) == 0);
            offset /= blockSize * blockSize;
        }
        
        offset *= blockSizeInBytes;
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
    int alphaChannel       = ((flags & IMAGE_STORAGE_NO_ALPHA) || !d.HasAlpha()) ? STBIR_ALPHA_CHANNEL_NONE : numChannels - 1;
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

    TextureFormatInfo const& info = GetTextureFormatInfo(m_Desc.Format);

    IMAGE_DATA_TYPE DataType = info.DataType;

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
            Hk::GenerateMipmaps<Decoder_R4G4B4A4>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R5G6B5:
            Hk::GenerateMipmaps<Decoder_R5G6B5>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R5G5B5A1:
            Hk::GenerateMipmaps<Decoder_R5G5B5A1>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R10G10B10A2:
            Hk::GenerateMipmaps<Decoder_R10G10B10A2>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_ENCODED_R11G11B10F:
            Hk::GenerateMipmaps<Decoder_R11G11B10F>(*this, SliceIndex, resampleMode, resampleFilter);
            return true;
        case IMAGE_DATA_TYPE_HALF:
            if (m_Desc.Format == TEXTURE_FORMAT_R16_FLOAT)
                Hk::GenerateMipmaps<Decoder_R16F>(*this, SliceIndex, resampleMode, resampleFilter);
            else if (m_Desc.Format == TEXTURE_FORMAT_RG16_FLOAT)
                Hk::GenerateMipmaps<Decoder_RG16F>(*this, SliceIndex, resampleMode, resampleFilter);
            else if (m_Desc.Format == TEXTURE_FORMAT_RGBA16_FLOAT)
                Hk::GenerateMipmaps<Decoder_RGBA16F>(*this, SliceIndex, resampleMode, resampleFilter);
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
    int alphaChannel       = ((flags & IMAGE_STORAGE_NO_ALPHA) || !info.bHasAlpha) ? STBIR_ALPHA_CHANNEL_NONE : numChannels - 1;
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

ImageStorage CreateImage(RawImage const& rawImage, ImageMipmapConfig const* pMipmapConfig, IMAGE_STORAGE_FLAGS Flags, IMAGE_IMPORT_FLAGS ImportFlags)
{
    if (!rawImage)
        return {};

    TEXTURE_FORMAT format;
    TEXTURE_FORMAT compressionFormat = TEXTURE_FORMAT_UNDEFINED;
    bool           bAddAlphaChannel = false;
    bool           bSwapChannels    = false;
    bool           bSwapChannelsIfCompressed = false;
    RawImage      tempImage;

    switch (rawImage.GetFormat())
    {
        case RAW_IMAGE_FORMAT_UNDEFINED:
        default:
            HK_ASSERT(0);
            return {};

        case RAW_IMAGE_FORMAT_R8:
            ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT;
            format            = TEXTURE_FORMAT_R8_UNORM;
            compressionFormat = TEXTURE_FORMAT_BC4_UNORM;
            break;

        case RAW_IMAGE_FORMAT_R8_ALPHA:
            ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT;
            format                  = TEXTURE_FORMAT_RG8_UNORM;
            compressionFormat       = TEXTURE_FORMAT_BC5_UNORM;
            break;

        case RAW_IMAGE_FORMAT_RGB8:
            bAddAlphaChannel        = true;
            Flags |= IMAGE_STORAGE_NO_ALPHA;
            ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT;
            if (ImportFlags & IMAGE_IMPORT_ASSUME_8BIT_RGB_IMAGES_ARE_SRGB)
            {
                format            = TEXTURE_FORMAT_SRGBA8_UNORM;
                compressionFormat = TEXTURE_FORMAT_BC1_UNORM_SRGB; // Use BC1 as no alpha channel is used.
            }
            else
            {
                format            = TEXTURE_FORMAT_RGBA8_UNORM;
                compressionFormat = TEXTURE_FORMAT_BC1_UNORM; // Use BC1 as no alpha channel is used.
            }
            break;

        case RAW_IMAGE_FORMAT_BGR8:
            bAddAlphaChannel        = true;
            Flags |= IMAGE_STORAGE_NO_ALPHA;
            ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT;
            if (ImportFlags & IMAGE_IMPORT_ASSUME_8BIT_RGB_IMAGES_ARE_SRGB)
            {
                format            = TEXTURE_FORMAT_SBGRA8_UNORM;
                compressionFormat = TEXTURE_FORMAT_BC1_UNORM_SRGB; // Use BC1 as no alpha channel is used.
            }
            else
            {
                format                    = TEXTURE_FORMAT_BGRA8_UNORM;
                compressionFormat         = TEXTURE_FORMAT_BC1_UNORM; // Use BC1 as no alpha channel is used.
            }
            bSwapChannelsIfCompressed = true;
            break;

        case RAW_IMAGE_FORMAT_RGBA8:
            ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT;
            if (ImportFlags & IMAGE_IMPORT_ASSUME_8BIT_RGB_IMAGES_ARE_SRGB)
            {
                format = TEXTURE_FORMAT_SRGBA8_UNORM;

                if (Flags & IMAGE_STORAGE_NO_ALPHA)
                    compressionFormat = TEXTURE_FORMAT_BC1_UNORM_SRGB;
                else
                    compressionFormat = TEXTURE_FORMAT_BC3_UNORM_SRGB;
            }
            else
            {
                format = TEXTURE_FORMAT_RGBA8_UNORM;

                if (Flags & IMAGE_STORAGE_NO_ALPHA)
                    compressionFormat = TEXTURE_FORMAT_BC1_UNORM;
                else
                    compressionFormat = TEXTURE_FORMAT_BC3_UNORM;
            }
            break;

        case RAW_IMAGE_FORMAT_BGRA8:
            ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT;
            if (ImportFlags & IMAGE_IMPORT_ASSUME_8BIT_RGB_IMAGES_ARE_SRGB)
            {
                format = TEXTURE_FORMAT_SBGRA8_UNORM;

                if (Flags & IMAGE_STORAGE_NO_ALPHA)
                    compressionFormat = TEXTURE_FORMAT_BC1_UNORM_SRGB;
                else
                    compressionFormat = TEXTURE_FORMAT_BC3_UNORM_SRGB;
            }
            else
            {
                format = TEXTURE_FORMAT_BGRA8_UNORM;

                if (Flags & IMAGE_STORAGE_NO_ALPHA)
                    compressionFormat = TEXTURE_FORMAT_BC1_UNORM;
                else
                    compressionFormat = TEXTURE_FORMAT_BC3_UNORM;
            }
            bSwapChannelsIfCompressed = true;
            break;

        case RAW_IMAGE_FORMAT_R32_FLOAT:
            if (ImportFlags & IMAGE_IMPORT_USE_COMPRESSION)
                ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT; // because BC6h compression takes f32 as input
            format = ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT ? TEXTURE_FORMAT_R16_FLOAT : TEXTURE_FORMAT_R32_FLOAT;
            // There is no analogue of the compression format
            break;

        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
            if (ImportFlags & IMAGE_IMPORT_USE_COMPRESSION)
                ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT; // because BC6h compression takes f32 as input
            format = ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT ? TEXTURE_FORMAT_RG16_FLOAT : TEXTURE_FORMAT_RG32_FLOAT;
            // There is no analogue of the compression format
            break;

        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
            if ((ImportFlags & IMAGE_IMPORT_USE_COMPRESSION) && (ImportFlags & IMAGE_IMPORT_ALLOW_HDRI_COMPRESSION))
            {
                ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT; // because BC6h compression takes f32 as input
                Flags |= IMAGE_STORAGE_NO_ALPHA;
                bAddAlphaChannel        = true;
                format                  = TEXTURE_FORMAT_RGBA32_FLOAT;
                compressionFormat       = TEXTURE_FORMAT_BC6H_UFLOAT;
            }
            else
            {
                format = ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGB32_FLOAT;
                if (ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT)
                {
                    Flags |= IMAGE_STORAGE_NO_ALPHA;
                    bAddAlphaChannel = true;
                }
            }
            break;

        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
            bSwapChannels = true;
            if ((ImportFlags & IMAGE_IMPORT_USE_COMPRESSION) && (ImportFlags & IMAGE_IMPORT_ALLOW_HDRI_COMPRESSION))
            {
                ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT; // because BC6h compression takes f32 as input
                Flags |= IMAGE_STORAGE_NO_ALPHA;
                bAddAlphaChannel        = true;
                format                  = TEXTURE_FORMAT_RGBA32_FLOAT;
                compressionFormat       = TEXTURE_FORMAT_BC6H_UFLOAT;
            }
            else
            {
                format = ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGB32_FLOAT;
                if (ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT)
                {
                    Flags |= IMAGE_STORAGE_NO_ALPHA;
                    bAddAlphaChannel = true;
                }
            }
            break;

        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
            if ((ImportFlags & IMAGE_IMPORT_USE_COMPRESSION) && (ImportFlags & IMAGE_IMPORT_ALLOW_HDRI_COMPRESSION))
            {
                ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT; // because BC6h compression takes f32 as input
                format                  = TEXTURE_FORMAT_RGBA32_FLOAT;
                compressionFormat       = TEXTURE_FORMAT_BC6H_UFLOAT; // NOTE: If we use BC6h compression, we lose the alpha channel.
            }
            else
            {
                format = ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGBA32_FLOAT;
            }
            break;

        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            bSwapChannels = true;
            if ((ImportFlags & IMAGE_IMPORT_USE_COMPRESSION) && (ImportFlags & IMAGE_IMPORT_ALLOW_HDRI_COMPRESSION))
            {
                ImportFlags &= ~IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT; // because BC6h compression takes f32 as input
                format                  = TEXTURE_FORMAT_RGBA32_FLOAT;
                compressionFormat       = TEXTURE_FORMAT_BC6H_UFLOAT; // NOTE: If we use BC6h compression, we lose the alpha channel.
            }
            else
            {
                format = ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT ? TEXTURE_FORMAT_RGBA16_FLOAT : TEXTURE_FORMAT_RGBA32_FLOAT;
            }
            break;
    }

    if (compressionFormat == TEXTURE_FORMAT_UNDEFINED)
        ImportFlags &= ~IMAGE_IMPORT_USE_COMPRESSION;

    TextureFormatInfo const& info = GetTextureFormatInfo(compressionFormat);

    bool bUseTempImage = false;

    if (ImportFlags & IMAGE_IMPORT_USE_COMPRESSION)
    {
        bool bMipmapped = pMipmapConfig != nullptr;

        uint32_t requiredWidth  = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetWidth()), info.BlockSize) : Align(rawImage.GetWidth(), info.BlockSize);
        uint32_t requiredHeight = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetHeight()), info.BlockSize) : Align(rawImage.GetHeight(), info.BlockSize);

        if (rawImage.GetWidth() != requiredWidth || rawImage.GetHeight() != requiredHeight)
        {
            RawImageResampleParams resample;

            resample.HorizontalEdgeMode = resample.VerticalEdgeMode = pMipmapConfig ? pMipmapConfig->EdgeMode : IMAGE_RESAMPLE_EDGE_WRAP;
            resample.HorizontalFilter = resample.VerticalFilter = pMipmapConfig ? pMipmapConfig->Filter : IMAGE_RESAMPLE_FILTER_MITCHELL;

            resample.Flags = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;
            if (info.bSRGB)
                resample.Flags |= RAW_IMAGE_RESAMPLE_COLORSPACE_SRGB;
            if (rawImage.NumChannels() == 4 && !(Flags & IMAGE_STORAGE_NO_ALPHA))
            {
                resample.Flags |= RAW_IMAGE_RESAMPLE_HAS_ALPHA;

                if (Flags & IMAGE_STORAGE_ALPHA_PREMULTIPLIED)
                    resample.Flags |= RAW_IMAGE_RESAMPLE_ALPHA_PREMULTIPLIED;
            }

            resample.ScaledWidth  = requiredWidth;
            resample.ScaledHeight = requiredHeight;

            tempImage = ResampleRawImage(rawImage, resample);
            if (bSwapChannelsIfCompressed)
                tempImage.SwapRGB();

            bUseTempImage = true;
        }
        else if (bSwapChannelsIfCompressed)
        {
            tempImage = rawImage.Clone();
            tempImage.SwapRGB();

            bUseTempImage = true;
        }
    }

    RawImage const& sourceImage = bUseTempImage ? tempImage : rawImage;

    ImageStorageDesc desc;
    desc.Type       = TEXTURE_2D;
    desc.Format     = format;
    desc.Width      = sourceImage.GetWidth();
    desc.Height     = sourceImage.GetHeight();
    desc.SliceCount = 1;
    desc.NumMipmaps = pMipmapConfig ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
    desc.Flags      = Flags;

    ImageStorage uncompressedImage(desc);

    ImageSubresourceDesc subres;
    subres.SliceIndex  = 0;
    subres.MipmapIndex = 0;

    ImageSubresource subresource = uncompressedImage.GetSubresource(subres);

    if (!bAddAlphaChannel && !bSwapChannels)
    {
        // Fast path
        if (ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT)
        {
            switch (uncompressedImage.NumChannels())
            {
                case 1:
                    Decoder_R16F().Encode(subresource.GetData(), sourceImage.GetData(), subresource.GetWidth(), subresource.GetHeight());
                    break;
                case 2:
                    Decoder_RG16F().Encode(subresource.GetData(), sourceImage.GetData(), subresource.GetWidth(), subresource.GetHeight());
                    break;
                case 4:
                    Decoder_RGBA16F().Encode(subresource.GetData(), sourceImage.GetData(), subresource.GetWidth(), subresource.GetHeight());
                    break;
                default:
                    // Never happen
                    HK_ASSERT(0);
                    break;
            }
        }
        else
        {
            subresource.Write(0, 0, sourceImage.GetWidth(), sourceImage.GetHeight(), sourceImage.GetData());
        }
    }
    else
    {
        const int r = bSwapChannels ? 2 : 0;
        const int g = 1;
        const int b = bSwapChannels ? 0 : 2;

        int dstNumChannels = uncompressedImage.NumChannels();
        int srcNumChannels = sourceImage.NumChannels();

        HK_ASSERT(dstNumChannels >= 3 && srcNumChannels >= 3);

        if (ImportFlags & IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT)
        {
            uint16_t*      dst   = (uint16_t*)subresource.GetData();
            uint16_t*      dst_e = dst + subresource.GetWidth() * subresource.GetHeight() * uncompressedImage.NumChannels();
            float const*   src   = (float const*)sourceImage.GetData();
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
                uint8_t*       dst_e = dst + subresource.GetWidth() * subresource.GetHeight() * uncompressedImage.NumChannels();
                uint8_t const* src   = (uint8_t const*)sourceImage.GetData();

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
                float*       dst_e = dst + subresource.GetWidth() * subresource.GetHeight() * uncompressedImage.NumChannels();
                float const* src   = (float const*)sourceImage.GetData();

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
        uncompressedImage.GenerateMipmaps(*pMipmapConfig);

    if (!(ImportFlags & IMAGE_IMPORT_USE_COMPRESSION))
        return uncompressedImage;

    desc.Format     = compressionFormat;
    desc.NumMipmaps = CalcNumMips(desc.Format, desc.Width, desc.Height);

    ImageStorage compressedImage(desc);

    for (uint32_t level = 0; level < desc.NumMipmaps; ++level)
    {
        ImageSubresource src = uncompressedImage.GetSubresource({0, level});
        ImageSubresource dst = compressedImage.GetSubresource({0, level});

        HK_ASSERT(src.GetWidth() == dst.GetWidth() && src.GetHeight() == dst.GetHeight());

        switch (desc.Format)
        {
            case TEXTURE_FORMAT_BC1_UNORM:
                HK_ASSERT(uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_RGBA8_UNORM || uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_BGRA8_UNORM);
                TextureBlockCompression::CompressBC1(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            case TEXTURE_FORMAT_BC1_UNORM_SRGB:
                HK_ASSERT(uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_SRGBA8_UNORM || uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_SBGRA8_UNORM);
                TextureBlockCompression::CompressBC1(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            case TEXTURE_FORMAT_BC3_UNORM:
                HK_ASSERT(uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_RGBA8_UNORM || uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_BGRA8_UNORM);
                TextureBlockCompression::CompressBC3(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            case TEXTURE_FORMAT_BC3_UNORM_SRGB:
                HK_ASSERT(uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_SRGBA8_UNORM || uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_SBGRA8_UNORM);
                TextureBlockCompression::CompressBC3(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            case TEXTURE_FORMAT_BC4_UNORM:
                HK_ASSERT(uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_R8_UNORM);                
                TextureBlockCompression::CompressBC4(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;
            case TEXTURE_FORMAT_BC5_UNORM:
                HK_ASSERT(uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_RG8_UNORM);                
                TextureBlockCompression::CompressBC5(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
                break;            
            case TEXTURE_FORMAT_BC6H_UFLOAT:
                HK_ASSERT(uncompressedImage.GetDesc().Format == TEXTURE_FORMAT_RGBA32_FLOAT);
                TextureBlockCompression::CompressBC6h(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight(), false);
                break;
            default:
                HK_ASSERT(0); // Never happen
        }
    }

    return compressedImage;
}

ImageStorage CreateImage(IBinaryStreamReadInterface& Stream, ImageMipmapConfig const* pMipmapConfig, IMAGE_STORAGE_FLAGS Flags, TEXTURE_FORMAT Format)
{
    using namespace TextureBlockCompression;

    if (!Stream.IsValid())
        return {};

    switch (Format)
    {
        case TEXTURE_FORMAT_UNDEFINED: {
            RawImage rawImage = CreateRawImage(Stream);
            if (!rawImage)
                return {};

            return CreateImage(rawImage, pMipmapConfig, Flags, IMAGE_IMPORT_FLAGS_DEFAULT);
        }
        case TEXTURE_FORMAT_R8_UINT:
        case TEXTURE_FORMAT_R8_SINT:
        case TEXTURE_FORMAT_R8_UNORM:
        case TEXTURE_FORMAT_R8_SNORM:
        case TEXTURE_FORMAT_RG8_UINT:
        case TEXTURE_FORMAT_RG8_SINT:
        case TEXTURE_FORMAT_RG8_UNORM:
        case TEXTURE_FORMAT_RG8_SNORM: {
            RawImage rawImage = CreateRawImage(Stream, GetTextureFormatInfo(Format).bHasGreen ? RAW_IMAGE_FORMAT_R8_ALPHA : RAW_IMAGE_FORMAT_R8);
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
            RawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_BGRA8);
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
            RawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_BGR8);
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
            RawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_BGRA8);
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
            RawImage rawImage = CreateRawImage(Stream, (Format == TEXTURE_FORMAT_BGRA8_UNORM || Format == TEXTURE_FORMAT_SBGRA8_UNORM) ? RAW_IMAGE_FORMAT_BGRA8 : RAW_IMAGE_FORMAT_RGBA8);
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
            RawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGBA32_FLOAT); // FIXME: Maybe BGRA?
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
            RawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGB32_FLOAT); // FIXME: Maybe BGR?
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
            RawImage rawImage;
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
            RawImage rawImage;
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

            RawImage rawImage = CreateRawImage(Stream, rawImageFormat);
            if (!rawImage)
                return {};

            TextureFormatInfo const& info = GetTextureFormatInfo(Format);

            bool bMipmapped = pMipmapConfig != nullptr;

            uint32_t requiredWidth  = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetWidth()), info.BlockSize) : Align(rawImage.GetWidth(), info.BlockSize);
            uint32_t requiredHeight = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetHeight()), info.BlockSize) : Align(rawImage.GetHeight(), info.BlockSize);

            // Image must be block aligned
            if (rawImage.GetWidth() != requiredWidth || rawImage.GetHeight() != requiredHeight)
            {
                RawImageResampleParams resample;
                resample.ScaledWidth        = requiredWidth;
                resample.ScaledHeight       = requiredHeight;
                resample.HorizontalEdgeMode = resample.VerticalEdgeMode = pMipmapConfig ? pMipmapConfig->EdgeMode : IMAGE_RESAMPLE_EDGE_WRAP;
                resample.HorizontalFilter = resample.VerticalFilter = pMipmapConfig ? pMipmapConfig->Filter : IMAGE_RESAMPLE_FILTER_MITCHELL;

                resample.Flags = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;
                if (info.bSRGB)
                    resample.Flags |= RAW_IMAGE_RESAMPLE_COLORSPACE_SRGB;
                if (info.bHasAlpha && !(Flags & IMAGE_STORAGE_NO_ALPHA))
                {
                    resample.Flags |= RAW_IMAGE_RESAMPLE_HAS_ALPHA;

                    if (Flags & IMAGE_STORAGE_ALPHA_PREMULTIPLIED)
                        resample.Flags |= RAW_IMAGE_RESAMPLE_ALPHA_PREMULTIPLIED;
                }
                rawImage = ResampleRawImage(rawImage, resample);
            }

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = bMipmapped ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
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
                uint32_t curWidth  = subresource.GetWidth();
                uint32_t curHeight = subresource.GetHeight();

                HeapBlob blob(std::max<uint32_t>(info.BlockSize, curWidth >> 1) * std::max<uint32_t>(info.BlockSize, curHeight >> 1) * bpp);

                void* data = rawImage.GetData();
                void* temp = blob.GetData();

                IMAGE_RESAMPLE_EDGE_MODE resampleMode   = pMipmapConfig->EdgeMode;
                IMAGE_RESAMPLE_FILTER    resampleFilter = pMipmapConfig->Filter;

                int numChannels        = bpp;
                int alphaChannel       = ((Flags & IMAGE_STORAGE_NO_ALPHA) || !info.bHasAlpha) ? STBIR_ALPHA_CHANNEL_NONE : numChannels - 1;
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
            RawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGBA32_FLOAT);
            if (!rawImage)
                return {};

            TextureFormatInfo const& info = GetTextureFormatInfo(Format);

            bool bMipmapped = pMipmapConfig != nullptr;

            uint32_t requiredWidth  = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetWidth()), info.BlockSize) : Align(rawImage.GetWidth(), info.BlockSize);
            uint32_t requiredHeight = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetHeight()), info.BlockSize) : Align(rawImage.GetHeight(), info.BlockSize);

            // Image must be block aligned
            if (rawImage.GetWidth() != requiredWidth || rawImage.GetHeight() != requiredHeight)
            {
                RawImageResampleParams resample;
                resample.ScaledWidth        = requiredWidth;
                resample.ScaledHeight       = requiredHeight;
                resample.HorizontalEdgeMode = resample.VerticalEdgeMode = pMipmapConfig ? pMipmapConfig->EdgeMode : IMAGE_RESAMPLE_EDGE_WRAP;
                resample.HorizontalFilter   = resample.VerticalFilter = pMipmapConfig ? pMipmapConfig->Filter : IMAGE_RESAMPLE_FILTER_MITCHELL;
                resample.Flags              = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;
                rawImage = ResampleRawImage(rawImage, resample);
            }

            ImageStorageDesc desc;
            desc.Type       = TEXTURE_2D;
            desc.Format     = Format;
            desc.Width      = rawImage.GetWidth();
            desc.Height     = rawImage.GetHeight();
            desc.SliceCount = 1;
            desc.NumMipmaps = bMipmapped ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
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

ImageStorage CreateImage(StringView FileName, ImageMipmapConfig const* pMipmapConfig, IMAGE_STORAGE_FLAGS Flags, TEXTURE_FORMAT Format)
{
    auto stream = File::sOpenRead(FileName);
    if (!stream)
        return {};
    return CreateImage(stream, pMipmapConfig, Flags, Format);
}

ImageStorage LoadSkyboxImages(SkyboxImportSettings const& Settings)
{
    RawImage rawImage[6];

    bool bHDRI = Settings.Format == SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT || Settings.Format == SKYBOX_IMPORT_TEXTURE_FORMAT_BC6H_UFLOAT;

    RAW_IMAGE_FORMAT rawImageFormat;
    switch (Settings.Format)
    {
        case SKYBOX_IMPORT_TEXTURE_FORMAT_SRGBA8_UNORM:
        case SKYBOX_IMPORT_TEXTURE_FORMAT_BC1_UNORM_SRGB:
            rawImageFormat = RAW_IMAGE_FORMAT_RGBA8;
            break;
        case SKYBOX_IMPORT_TEXTURE_FORMAT_SBGRA8_UNORM:
            rawImageFormat = RAW_IMAGE_FORMAT_BGRA8;
            break;
        case SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT:
            rawImageFormat = RAW_IMAGE_FORMAT_RGB32_FLOAT;
            break;
        case SKYBOX_IMPORT_TEXTURE_FORMAT_BC6H_UFLOAT:
            rawImageFormat = RAW_IMAGE_FORMAT_RGBA32_FLOAT;
            break;
        default:
            LOG("LoadSkyboxImages: unexpected texture format specified\n");
            return {};
    }

    for (uint32_t i = 0; i < 6; i++)
    {
        rawImage[i] = CreateRawImage(Settings.Faces[i], rawImageFormat);
        if (!rawImage[i])
            return {};

        if (rawImage[i].GetWidth() != rawImage[0].GetWidth() || rawImage[i].GetWidth() != rawImage[i].GetHeight())
        {
            LOG("LoadSkyboxImages: Invalid image size\n");
            return {};
        }
    }

    TextureFormatInfo const& info = GetTextureFormatInfo((TEXTURE_FORMAT)Settings.Format);

    uint32_t w = Align(rawImage[0].GetWidth(), info.BlockSize);
    uint32_t h = Align(rawImage[0].GetHeight(), info.BlockSize);

    if (w != rawImage[0].GetWidth() || h != rawImage[0].GetHeight())
    {
        RawImageResampleParams resample;

        resample.HorizontalEdgeMode = resample.VerticalEdgeMode = IMAGE_RESAMPLE_EDGE_CLAMP;
        resample.HorizontalFilter = resample.VerticalFilter = IMAGE_RESAMPLE_FILTER_MITCHELL;

        resample.Flags = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;

        if (info.bSRGB)
            resample.Flags |= RAW_IMAGE_RESAMPLE_COLORSPACE_SRGB;

        resample.ScaledWidth  = w;
        resample.ScaledHeight = h;

        for (uint32_t i = 0; i < 6; i++)
        {
            rawImage[i] = ResampleRawImage(rawImage[i], resample);
        }        
    }

    if (bHDRI && (Settings.HDRIScale != 1.0f || Settings.HDRIPow != 1.0f))
    {
        int    numChannels = rawImage[0].NumChannels();
        size_t count = (size_t)w * h * numChannels;

        for (uint32_t i = 0; i < 6; i++)
        {
            float* data = (float*)rawImage[i].GetData();
            
            for (size_t j = 0; j < count; j += numChannels)
            {
                data[j]     = Math::Pow(data[j + 0] * Settings.HDRIScale, Settings.HDRIPow);
                data[j + 1] = Math::Pow(data[j + 1] * Settings.HDRIScale, Settings.HDRIPow);
                data[j + 2] = Math::Pow(data[j + 2] * Settings.HDRIScale, Settings.HDRIPow);
            }
        }
    }

    ImageStorageDesc desc;
    desc.Type       = TEXTURE_CUBE;
    desc.Width      = w;
    desc.Height     = h;
    desc.SliceCount = 6;
    desc.NumMipmaps = 1;
    desc.Flags      = IMAGE_STORAGE_NO_ALPHA;
    desc.Format     = (TEXTURE_FORMAT)Settings.Format;

    ImageStorage storage(desc);

    ImageSubresourceDesc subres;
    subres.MipmapIndex = 0;
    for (uint32_t i = 0; i < 6; i++)
    {
        subres.SliceIndex = i;

        ImageSubresource subresource = storage.GetSubresource(subres);

        switch (Settings.Format)
        {
            case SKYBOX_IMPORT_TEXTURE_FORMAT_SRGBA8_UNORM:
            case SKYBOX_IMPORT_TEXTURE_FORMAT_SBGRA8_UNORM:
                subresource.Write(0, 0, subresource.GetWidth(), subresource.GetHeight(), rawImage[i].GetData());
                break;
            case SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT:
                Decoder_R11G11B10F().Encode(subresource.GetData(), rawImage[i].GetData(), subresource.GetWidth(), subresource.GetHeight());
                break;
            case SKYBOX_IMPORT_TEXTURE_FORMAT_BC1_UNORM_SRGB:
                TextureBlockCompression::CompressBC1(rawImage[i].GetData(), subresource.GetData(), subresource.GetWidth(), subresource.GetHeight());
                break;
            case SKYBOX_IMPORT_TEXTURE_FORMAT_BC6H_UFLOAT:
                TextureBlockCompression::CompressBC6h(rawImage[i].GetData(), subresource.GetData(), subresource.GetWidth(), subresource.GetHeight(), false);
                break;
            default:
                HK_ASSERT(0);
        }
    }

    return storage;
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
    int alphaChannel       = Desc.bHasAlpha ? numChannels - 1 : STBIR_ALPHA_CHANNEL_NONE;
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

    if (!Desc.pImage)
    {
        LOG("ResampleRawImage: invalid source\n");
        return false;
    }

    if (!pDest)
    {
        LOG("ResampleRawImage: invalid dest\n");
        return false;
    }

    if (Desc.Width == 0 || Desc.Height == 0 || Desc.ScaledWidth == 0 || Desc.ScaledHeight == 0)
    {
        LOG("ResampleRawImage: invalid size\n");
        return false;
    }

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

    int alphaChannel = Desc.bHasAlpha ? numChannels - 1 : STBIR_ALPHA_CHANNEL_NONE;

    int result =
        stbir_resize(Desc.pImage, Desc.Width, Desc.Height, Desc.Width * info.BytesPerBlock,
                     pDest, Desc.ScaledWidth, Desc.ScaledHeight, Desc.ScaledWidth * info.BytesPerBlock,
                     get_stbir_datatype(info.DataType),
                     numChannels,
                     alphaChannel,
                     Desc.bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                     (stbir_edge)Desc.HorizontalEdgeMode, (stbir_edge)Desc.VerticalEdgeMode,
                     (stbir_filter)Desc.HorizontalFilter, (stbir_filter)Desc.VerticalFilter,
                     info.bSRGB ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR,
                     NULL);

    HK_ASSERT(result == 1);
    HK_UNUSED(result);

    return true;
}

RawImage ResampleRawImage(RawImage const& Source, RawImageResampleParams const& Desc)
{
    if (!Source)
    {
        LOG("ResampleRawImage: source is invalid\n");
        return {};
    }

    if (Desc.ScaledWidth == 0 || Desc.ScaledHeight == 0)
    {
        LOG("ResampleRawImage: invalid size\n");
        return {};
    }

    stbir_datatype datatype;

    switch (Source.GetFormat())
    {
        case RAW_IMAGE_FORMAT_R8:
        case RAW_IMAGE_FORMAT_R8_ALPHA:
        case RAW_IMAGE_FORMAT_RGB8:
        case RAW_IMAGE_FORMAT_BGR8:
        case RAW_IMAGE_FORMAT_RGBA8:
        case RAW_IMAGE_FORMAT_BGRA8:
            datatype = STBIR_TYPE_UINT8;
            break;

        case RAW_IMAGE_FORMAT_R32_FLOAT:
        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            datatype = STBIR_TYPE_FLOAT;
            break;

        default:
            LOG("ResampleRawImage: invalid image format\n");
            return {};
    }

    RawImage dest(Desc.ScaledWidth, Desc.ScaledHeight, Source.GetFormat());

    RawImageFormatInfo const& info = GetRawImageFormatInfo(Source.GetFormat());

    int result =
            stbir_resize(Source.GetData(), Source.GetWidth(), Source.GetHeight(), Source.GetWidth() * info.BytesPerPixel,
                         dest.GetData(), dest.GetWidth(), dest.GetHeight(), dest.GetWidth() * info.BytesPerPixel,
                         datatype,
                         Source.NumChannels(),
                         (Desc.Flags & RAW_IMAGE_RESAMPLE_HAS_ALPHA) ? Source.NumChannels() - 1 : STBIR_ALPHA_CHANNEL_NONE,
                         (Desc.Flags & RAW_IMAGE_RESAMPLE_HAS_ALPHA) && (Desc.Flags & RAW_IMAGE_RESAMPLE_ALPHA_PREMULTIPLIED) ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                         (stbir_edge)Desc.HorizontalEdgeMode, (stbir_edge)Desc.VerticalEdgeMode,
                         (stbir_filter)Desc.HorizontalFilter, (stbir_filter)Desc.VerticalFilter,
                         (Desc.Flags & RAW_IMAGE_RESAMPLE_COLORSPACE_SRGB) ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR,
                         NULL);

    HK_ASSERT(result == 1);
    HK_UNUSED(result);

    return dest;
}

static void NormalizeVectors(Float3* pVectors, size_t Count)
{
    Float3* end = pVectors + Count;
    while (pVectors < end)
    {
        *pVectors = *pVectors * 2.0f - 1.0f;
        pVectors->NormalizeSelf();
        ++pVectors;
    }
}

// Assume normals already normalized. The width and height of the normal map must be a multiple of blockSize if compression is enabled.
ImageStorage CreateNormalMap(Float3 const* pNormals, uint32_t Width, uint32_t Height, NORMAL_MAP_PACK Pack, bool bUseCompression, bool bMipmapped, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode, IMAGE_RESAMPLE_FILTER ResampleFilter)
{
    using namespace TextureBlockCompression;

    // clang-format off
    struct CompressInfo
    {
        RawImage         (*PackRoutine)(Float3 const* Normals, uint32_t Width, uint32_t Height);
        void             (*CompressionRoutine)(void const* pSrc, void* pDest, uint32_t Width, uint32_t Height);
        TEXTURE_FORMAT   CompressedFormat;
        TEXTURE_FORMAT   UncompressedFormat;
        RAW_IMAGE_FORMAT ValidateFormat;
    };
    constexpr CompressInfo compressInfo[] = {
        PackNormalsRGBA_BC1_Compatible,          CompressBC1, TEXTURE_FORMAT_BC1_UNORM, TEXTURE_FORMAT_RGBA8_UNORM, RAW_IMAGE_FORMAT_RGBA8,    // NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE
        PackNormalsRG_BC5_Compatible,            CompressBC5, TEXTURE_FORMAT_BC5_UNORM, TEXTURE_FORMAT_RG8_UNORM,   RAW_IMAGE_FORMAT_R8_ALPHA, // NORMAL_MAP_PACK_RG_BC5_COMPATIBLE
        PackNormalsSpheremap_BC5_Compatible,     CompressBC5, TEXTURE_FORMAT_BC5_UNORM, TEXTURE_FORMAT_RG8_UNORM,   RAW_IMAGE_FORMAT_R8_ALPHA, // NORMAL_MAP_PACK_SPHEREMAP_BC5_COMPATIBLE
        PackNormalsStereographic_BC5_Compatible, CompressBC5, TEXTURE_FORMAT_BC5_UNORM, TEXTURE_FORMAT_RG8_UNORM,   RAW_IMAGE_FORMAT_R8_ALPHA, // NORMAL_MAP_PACK_STEREOGRAPHIC_BC5_COMPATIBLE
        PackNormalsParaboloid_BC5_Compatible,    CompressBC5, TEXTURE_FORMAT_BC5_UNORM, TEXTURE_FORMAT_RG8_UNORM,   RAW_IMAGE_FORMAT_R8_ALPHA, // NORMAL_MAP_PACK_PARABOLOID_BC5_COMPATIBLE
        PackNormalsRGBA_BC3_Compatible,          CompressBC3, TEXTURE_FORMAT_BC3_UNORM, TEXTURE_FORMAT_RGBA8_UNORM, RAW_IMAGE_FORMAT_RGBA8,    // NORMAL_MAP_PACK_RGBA_BC3_COMPATIBLE
    };
    // clang-format on

    CompressInfo const& compress = compressInfo[Pack];

    uint32_t blockSize = GetTextureFormatInfo(compressInfo[Pack].CompressedFormat).BlockSize;

    uint32_t requiredWidth  = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(Width), blockSize) : Align(Width, blockSize);
    uint32_t requiredHeight = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(Height), blockSize) : Align(Height, blockSize);

    if (bUseCompression && ((Width != requiredWidth) || (Height != requiredHeight)))
    {
        LOG("CreateNormalMap: The width and height of the normal map must be a power of two and a multiple of blockSize if compression is enabled.\n");
        return {};
    }

    // FIXME: Should we call PackRoutine for each mip level?
    RawImage source = compress.PackRoutine(pNormals, Width, Height);

    HK_ASSERT(compress.ValidateFormat == source.GetFormat());
    if (compress.ValidateFormat != source.GetFormat())
    {
        LOG("CreateNormalMap: Uncompatible raw image format\n");
        return {};
    }

    ImageStorageDesc desc;
    desc.Type       = TEXTURE_2D;
    desc.Format     = compress.UncompressedFormat;
    desc.Width      = source.GetWidth();
    desc.Height     = source.GetHeight();
    desc.SliceCount = 1;
    desc.NumMipmaps = bMipmapped ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
    desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

    ImageStorage uncompressedImage(desc);

    ImageSubresourceDesc subres;
    subres.SliceIndex  = 0;
    subres.MipmapIndex = 0;

    ImageSubresource subresource = uncompressedImage.GetSubresource(subres);

    subresource.Write(0, 0, source.GetWidth(), source.GetHeight(), source.GetData());

    if (bMipmapped)
    {
        ImageMipmapConfig mipmapConfig;
        mipmapConfig.EdgeMode = ResampleEdgeMode;
        mipmapConfig.Filter   = ResampleFilter;
        uncompressedImage.GenerateMipmaps(mipmapConfig);
    }

    if (!bUseCompression)
    {
        return uncompressedImage;
    }

    desc.Format = compress.CompressedFormat;
    desc.NumMipmaps = bMipmapped ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
    ImageStorage compressedImage(desc);

    for (uint32_t level = 0; level < desc.NumMipmaps; ++level)
    {
        ImageSubresource src = uncompressedImage.GetSubresource({0, level});
        ImageSubresource dst = compressedImage.GetSubresource({0, level});

        HK_ASSERT(src.GetWidth() == dst.GetWidth() && src.GetHeight() == dst.GetHeight());

        compress.CompressionRoutine(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
    }
    return compressedImage;
}

ImageStorage CreateNormalMap(IBinaryStreamReadInterface& Stream, NORMAL_MAP_PACK Pack, bool bUseCompression, bool bMipmapped, bool bConvertFromDirectXNormalMap, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode)
{
    const IMAGE_RESAMPLE_FILTER ResampleFilter = IMAGE_RESAMPLE_FILTER_TRIANGLE; //IMAGE_RESAMPLE_FILTER_MITCHELL; // TODO: Check what filter is better for normal maps

    RawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGB32_FLOAT);
    if (!rawImage)
        return {};

    if (bConvertFromDirectXNormalMap)
        rawImage.InvertGreen();

    // NOTE: Currently all compression methods have blockSize = 4
    const uint32_t blockSize = 4;

    uint32_t requiredWidth  = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetWidth()), blockSize) : Align(rawImage.GetWidth(), blockSize);
    uint32_t requiredHeight = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(rawImage.GetHeight()), blockSize) : Align(rawImage.GetHeight(), blockSize);

    if (bUseCompression && ((rawImage.GetWidth() != requiredWidth) || (rawImage.GetHeight() != requiredHeight)))
    {
        RawImageResampleParams resample;
        resample.HorizontalEdgeMode = resample.VerticalEdgeMode = ResampleEdgeMode;
        resample.HorizontalFilter = resample.VerticalFilter = ResampleFilter;

        resample.ScaledWidth = requiredWidth;
        resample.ScaledHeight = requiredHeight;

        rawImage = ResampleRawImage(rawImage, resample);
    }

    NormalizeVectors((Float3*)rawImage.GetData(), rawImage.GetWidth() * rawImage.GetHeight());

    return CreateNormalMap((Float3 const*)rawImage.GetData(), rawImage.GetWidth(), rawImage.GetHeight(), Pack, bUseCompression, bMipmapped, ResampleEdgeMode, ResampleFilter);
}

ImageStorage CreateNormalMap(StringView FileName, NORMAL_MAP_PACK Pack, bool bUseCompression, bool bMipmapped, bool bConvertFromDirectXNormalMap, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode)
{
    auto stream = File::sOpenRead(FileName);
    if (!stream)
        return {};
    return CreateNormalMap(stream, Pack, bUseCompression, bMipmapped, bConvertFromDirectXNormalMap, ResampleEdgeMode);
}

ImageStorage CreateRoughnessMap(uint8_t const* pRoughnessMap, uint32_t Width, uint32_t Height, bool bUseCompression, bool bMipmapped, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode, IMAGE_RESAMPLE_FILTER ResampleFilter)
{
    using namespace TextureBlockCompression;

    uint32_t blockSize = GetTextureFormatInfo(TEXTURE_FORMAT_BC4_UNORM).BlockSize;

    uint32_t requiredWidth  = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(Width), blockSize) : Align(Width, blockSize);
    uint32_t requiredHeight = bMipmapped ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(Height), blockSize) : Align(Height, blockSize);

    if (bUseCompression && ((Width != requiredWidth) || (Height != requiredHeight)))
    {
        LOG("CreateRoughnessMap: The width and height of the roughness map must be a power of two and a multiple of blockSize if compression is enabled.\n");
        return {};
    }

    ImageStorageDesc desc;
    desc.Type       = TEXTURE_2D;
    desc.Format     = TEXTURE_FORMAT_R8_UNORM;
    desc.Width      = Width;
    desc.Height     = Height;
    desc.SliceCount = 1;
    desc.NumMipmaps = bMipmapped ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;
    desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

    ImageStorage     uncompressedImage(desc);
    ImageSubresource subresource = uncompressedImage.GetSubresource({0, 0});

    subresource.Write(0, 0, Width, Height, pRoughnessMap);

    if (bMipmapped)
    {
        ImageMipmapConfig mipmapConfig;
        mipmapConfig.EdgeMode = ResampleEdgeMode;
        mipmapConfig.Filter   = ResampleFilter;
        uncompressedImage.GenerateMipmaps(mipmapConfig);
    }

    if (!bUseCompression)
    {
        return uncompressedImage;
    }

    desc.Format = TEXTURE_FORMAT_BC4_UNORM;
    desc.NumMipmaps = bMipmapped ? CalcNumMips(desc.Format, desc.Width, desc.Height) : 1;

    ImageStorage compressedImage(desc);

    for (uint32_t level = 0; level < desc.NumMipmaps; ++level)
    {
        ImageSubresource src = uncompressedImage.GetSubresource({0, level});
        ImageSubresource dst = compressedImage.GetSubresource({0, level});

        HK_ASSERT(src.GetWidth() == dst.GetWidth() && src.GetHeight() == dst.GetHeight());

        CompressBC4(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
    }
    return compressedImage;
}


bool CreateNormalAndRoughness(NormalRoughnessImportSettings const& Settings, ImageStorage& NormalMapImage, ImageStorage& RoughnessMapImage)
{
    using namespace TextureBlockCompression;

    const IMAGE_RESAMPLE_FILTER NormalMapResampleFilter    = IMAGE_RESAMPLE_FILTER_TRIANGLE; // TODO: Check what filter is better for normal maps
    const IMAGE_RESAMPLE_FILTER RoughnessMapResampleFilter = IMAGE_RESAMPLE_FILTER_TRIANGLE;

    RawImage roughnessImage = CreateRawImage(Settings.RoughnessMap, RAW_IMAGE_FORMAT_R8);
    if (!roughnessImage)
        return {};

    // NOTE: Currently all compression methods have blockSize = 4
    const uint32_t blockSize = 4;

    if (Settings.bCompressRoughness_BC4)
    {
        uint32_t requiredWidth  = Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(roughnessImage.GetWidth()), blockSize);
        uint32_t requiredHeight = Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(roughnessImage.GetHeight()), blockSize);

        if ((roughnessImage.GetWidth() != requiredWidth) || (roughnessImage.GetHeight() != requiredHeight))
        {
            RawImageResampleParams resample;
            resample.HorizontalEdgeMode = Settings.ResampleEdgeMode;
            resample.VerticalEdgeMode   = Settings.ResampleEdgeMode;
            resample.HorizontalFilter   = RoughnessMapResampleFilter;
            resample.VerticalFilter     = RoughnessMapResampleFilter;
            resample.ScaledWidth        = requiredWidth;
            resample.ScaledHeight       = requiredHeight;

            roughnessImage = ResampleRawImage(roughnessImage, resample);
        }
    }

    RawImage normalMapSource = CreateRawImage(Settings.NormalMap, RAW_IMAGE_FORMAT_RGB32_FLOAT);
    if (!normalMapSource)
        return {};

    if (Settings.bConvertFromDirectXNormalMap)
        normalMapSource.InvertGreen();

    RawImage normalMapImage;

    if (normalMapSource.GetWidth() != roughnessImage.GetWidth() || normalMapSource.GetHeight() != roughnessImage.GetHeight())
    {
        RawImageResampleParams resample;
        resample.HorizontalEdgeMode = Settings.ResampleEdgeMode;
        resample.VerticalEdgeMode   = Settings.ResampleEdgeMode;
        resample.HorizontalFilter   = NormalMapResampleFilter;
        resample.VerticalFilter     = NormalMapResampleFilter;
        resample.ScaledWidth        = roughnessImage.GetWidth();
        resample.ScaledHeight       = roughnessImage.GetHeight();

        normalMapImage = ResampleRawImage(normalMapSource, resample);
    }
    else
    {
        normalMapImage = normalMapSource.Clone();
    }

    // Normalize normal map
    NormalizeVectors((Float3*)normalMapImage.GetData(), normalMapImage.GetWidth() * normalMapImage.GetHeight());

    ImageStorage roughnessMapUncompressed = CreateRoughnessMap((uint8_t const*)roughnessImage.GetData(), roughnessImage.GetWidth(), roughnessImage.GetHeight(), false, true, Settings.ResampleEdgeMode, RoughnessMapResampleFilter);

    RawImage averageNormals;

    // Update roughness
    for (uint32_t level = 1; level < roughnessMapUncompressed.GetDesc().NumMipmaps; ++level)
    {
        ImageSubresource roughness = roughnessMapUncompressed.GetSubresource({0, level});        

        RawImageResampleParams resample;
        resample.Flags              = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;
        resample.HorizontalEdgeMode = Settings.ResampleEdgeMode;
        resample.VerticalEdgeMode   = Settings.ResampleEdgeMode;
        resample.HorizontalFilter   = NormalMapResampleFilter;
        resample.VerticalFilter     = NormalMapResampleFilter;
        resample.ScaledWidth        = roughness.GetWidth();
        resample.ScaledHeight       = roughness.GetHeight();

        averageNormals = ResampleRawImage(level == 1 ? normalMapImage : averageNormals, resample);

        uint32_t pixCount = roughness.GetWidth() * roughness.GetHeight();
        for (uint32_t i = 0; i < pixCount; i++)
        {
            Float3 n = ((Float3*)averageNormals.GetData())[i];

            float r2 = n.LengthSqr();
            if (r2 > 1e-8f && r2 < 1.0f)
            {
                if (Settings.RoughnessBake == NormalRoughnessImportSettings::ROUGHNESS_BAKE_vMF)
                {
                    // vMF
                    // Equation from http://graphicrants.blogspot.com/2018/05/normal-map-filtering-using-vmf-part-3.html
                    float variance = 2.0f * Math::RSqrt(r2) * (1.0f - r2) / (3.0f - r2);

                    float roughnessVal = ((uint8_t*)roughness.GetData())[i] / 255.0f;

                    ((uint8_t*)roughness.GetData())[i] = Math::Round(Math::Saturate(Math::Sqrt(roughnessVal * roughnessVal + variance)) * 255.0f);
                }
                else
                {
                    auto RoughnessToSpecPower = [](float Roughness) -> float
                    {
                        return 2.0f / (Roughness * Roughness) - 2.0f;
                    };
                    auto SpecPowerToRoughness = [](float Spec) -> float
                    {
                        return sqrt(2.0f / (Spec + 2.0f));
                    };

                    // Toksvig
                    // https://blog.selfshadow.com/2011/07/22/specular-showdown/
                    float r         = sqrt(r2);
                    float specPower = RoughnessToSpecPower(Math::Max<uint8_t>(((uint8_t*)roughness.GetData())[i], 1) / 255.0f);
                    float ft        = r / Math::Lerp(specPower, 1.0f, r);

                    ((uint8_t*)roughness.GetData())[i] = Math::Round(Math::Saturate(SpecPowerToRoughness(ft * specPower)) * 255.0f);
                }
            }
        }
    }

    if (Settings.bCompressRoughness_BC4)
    {
        ImageStorageDesc desc;
        desc.Type       = TEXTURE_2D;
        desc.Format     = TEXTURE_FORMAT_BC4_UNORM;
        desc.Width      = roughnessMapUncompressed.GetDesc().Width;
        desc.Height     = roughnessMapUncompressed.GetDesc().Height;
        desc.SliceCount = 1;
        desc.NumMipmaps = CalcNumMips(desc.Format, desc.Width, desc.Height);
        desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

        ImageStorage roughnessMapCompressed(desc);

        for (uint32_t level = 0; level < desc.NumMipmaps; ++level)
        {
            ImageSubresource src = roughnessMapUncompressed.GetSubresource({0, level});
            ImageSubresource dst = roughnessMapCompressed.GetSubresource({0, level});

            HK_ASSERT(src.GetWidth() == dst.GetWidth() && src.GetHeight() == dst.GetHeight());

            CompressBC4(src.GetData(), dst.GetData(), dst.GetWidth(), dst.GetHeight());
        }

        RoughnessMapImage = std::move(roughnessMapCompressed);
    }
    else
    {
        RoughnessMapImage = std::move(roughnessMapUncompressed);
    }

    uint32_t requiredNormalMapWidth  = Settings.bCompressNormals ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(normalMapSource.GetWidth()), blockSize) : normalMapSource.GetWidth();
    uint32_t requiredNormalMapHeight = Settings.bCompressNormals ? Math::Max<uint32_t>(Math::ToClosestPowerOfTwo(normalMapSource.GetHeight()), blockSize) : normalMapSource.GetHeight();

    if (requiredNormalMapWidth == normalMapImage.GetWidth() && requiredNormalMapHeight == normalMapImage.GetHeight())
    {
        // Use normalMapImage
    }
    else if (((normalMapSource.GetWidth() != requiredNormalMapWidth) || (normalMapSource.GetHeight() != requiredNormalMapHeight)))
    {
        RawImageResampleParams resample;
        resample.HorizontalEdgeMode = Settings.ResampleEdgeMode;
        resample.VerticalEdgeMode   = Settings.ResampleEdgeMode;
        resample.HorizontalFilter   = NormalMapResampleFilter;
        resample.VerticalFilter     = NormalMapResampleFilter;
        resample.ScaledWidth        = requiredNormalMapWidth;
        resample.ScaledHeight       = requiredNormalMapHeight;

        normalMapImage = ResampleRawImage(normalMapSource, resample);

        // Normalize normal map
        NormalizeVectors((Float3*)normalMapImage.GetData(), normalMapImage.GetWidth() * normalMapImage.GetHeight());
    }

    NormalMapImage = CreateNormalMap((Float3 const*)normalMapImage.GetData(), normalMapImage.GetWidth(), normalMapImage.GetHeight(), Settings.Pack, Settings.bCompressNormals, true, Settings.ResampleEdgeMode, NormalMapResampleFilter);

    return true;
}

namespace
{

Color4 ApplyColorGrading(ColorGradingSettings const& Settings, Color4 const& Color)
{
    float lum = Color.GetLuminance();

    Color4 mult;

    mult.SetTemperature(Math::Clamp(Settings.ColorTemperature, 1000.0f, 40000.0f));

    Color4 c;
    c.R = Math::Lerp(Color.R, Color.R * mult.R, Settings.ColorTemperatureStrength.X);
    c.G = Math::Lerp(Color.G, Color.G * mult.G, Settings.ColorTemperatureStrength.Y);
    c.B = Math::Lerp(Color.B, Color.B * mult.B, Settings.ColorTemperatureStrength.Z);
    c.A = 1;

    float newLum = c.GetLuminance();
    float scale = Math::Lerp(1.0f, (newLum > 1e-6) ? (lum / newLum) : 1.0f, Settings.ColorTemperatureBrightnessNormalization);

    c *= scale;

    lum = c.GetLuminance();

    float r = Math::Lerp(lum, c.R, Settings.Presaturation.X);
    float g = Math::Lerp(lum, c.G, Settings.Presaturation.Y);
    float b = Math::Lerp(lum, c.B, Settings.Presaturation.Z);

    r = 2.0f * Settings.Gain[0] * (r + ((Settings.Lift[0] * 2.0f - 1.0) * (1.0 - r)));
    g = 2.0f * Settings.Gain[1] * (g + ((Settings.Lift[1] * 2.0f - 1.0) * (1.0 - g)));
    b = 2.0f * Settings.Gain[2] * (b + ((Settings.Lift[2] * 2.0f - 1.0) * (1.0 - b)));

    r = Math::Pow(r, 0.5f / Settings.Gamma.X);
    g = Math::Pow(g, 0.5f / Settings.Gamma.Y);
    b = Math::Pow(b, 0.5f / Settings.Gamma.Z);

    return Color4(r, g, b, Color.A);
}

} // namespace

ImageStorage CreateColorGradingLUT(ColorGradingSettings const& Settings)
{
    ImageStorageDesc desc;

    desc.Type = TEXTURE_3D;
    desc.Width = 16;
    desc.Height = 16;
    desc.Depth = 16;
    desc.Format = TEXTURE_FORMAT_SBGRA8_UNORM;
    desc.Flags = IMAGE_STORAGE_NO_ALPHA;

    ImageStorage image(desc);

    Color4 color;

    const float scale = 1.0f / 15.0f;

    for (uint32_t slice = 0; slice < image.GetDesc().SliceCount; ++slice)
    {
        ImageSubresourceDesc subresDesc;
        subresDesc.SliceIndex = slice;
        subresDesc.MipmapIndex = 0;

        ImageSubresource subresource = image.GetSubresource(subresDesc);

        color.B = scale * slice;

        uint8_t* dest = (uint8_t*)subresource.GetData();

        for (int y = 0; y < 16; y++)
        {
            color.G = scale * y;
            for (int x = 0; x < 16; x++)
            {
                color.R = scale * x;

                Color4 result = ApplyColorGrading(Settings, color);

                dest[0] = Math::Clamp(result.B * 255.0f, 0.0f, 255.0f);
                dest[1] = Math::Clamp(result.G * 255.0f, 0.0f, 255.0f);
                dest[2] = Math::Clamp(result.R * 255.0f, 0.0f, 255.0f);
                dest[3] = 255;
                dest += 4;
            }
        }
    }

    return image;
}

ImageStorage CreateLuminanceColorGradingLUT()
{
    ImageStorageDesc desc;

    desc.Type = TEXTURE_3D;
    desc.Width = 16;
    desc.Height = 16;
    desc.Depth = 16;
    desc.Format = TEXTURE_FORMAT_SBGRA8_UNORM;
    desc.Flags = IMAGE_STORAGE_NO_ALPHA;

    ImageStorage image(desc);

    for (uint32_t slice = 0; slice < image.GetDesc().SliceCount; ++slice)
    {
        ImageSubresourceDesc subresDesc;
        subresDesc.SliceIndex = slice;
        subresDesc.MipmapIndex = 0;

        ImageSubresource subresource = image.GetSubresource(subresDesc);

        uint8_t* dest = (uint8_t*)subresource.GetData();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                dest[0] = dest[1] = dest[2] = Math::Clamp(x * (0.2126f / 15.0f * 255.0f) + y * (0.7152f / 15.0f * 255.0f) + slice * (0.0722f / 15.0f * 255.0f), 0.0f, 255.0f);
                dest[3] = 255;
                dest += 4;
            }
        }
    }

    return image;
}

ImageStorage CreateColorGradingLUTFrom2DImage(IBinaryStreamReadInterface& Stream)
{
    ImageStorage source = CreateImage(Stream, nullptr, IMAGE_STORAGE_NO_ALPHA, TEXTURE_FORMAT_SBGRA8_UNORM);

    if (source && source.GetDesc().Width == 16 * 16 && source.GetDesc().Height == 16)
    {
        ImageStorageDesc desc;

        desc.Type = TEXTURE_3D;
        desc.Width = 16;
        desc.Height = 16;
        desc.Depth = 16;
        desc.Format = TEXTURE_FORMAT_SBGRA8_UNORM;
        desc.Flags = IMAGE_STORAGE_NO_ALPHA;

        ImageStorage image(desc);

        const uint8_t* sourceData = (const uint8_t*)source.GetData();

        for (uint32_t slice = 0; slice < image.GetDesc().SliceCount; ++slice)
        {
            ImageSubresourceDesc subresDesc;
            subresDesc.SliceIndex = slice;
            subresDesc.MipmapIndex = 0;

            ImageSubresource subresource = image.GetSubresource(subresDesc);

            uint8_t* dest = (uint8_t*)subresource.GetData();
            for (int y = 0; y < 16; y++)
            {
                Core::Memcpy(dest, sourceData, 16 * 4);
                sourceData += 16 * 16 * 4;
                dest += 16 * 4;
            }

            sourceData -= 16 * 16 * 16 * 4 + 16 * 4;
        }

        return image;
    }

    return {};
}


HK_NAMESPACE_END
