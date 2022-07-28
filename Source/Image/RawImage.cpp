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

#include <Image/RawImage.h>
#include <Core/Color.h>
#include <Core/Compress.h>
#include <Core/HeapBlob.h>
#include <Platform/Logger.h>

#define STBI_MALLOC(sz)                     Platform::GetHeapAllocator<HEAP_IMAGE>().Alloc(sz, 16)
#define STBI_FREE(p)                        Platform::GetHeapAllocator<HEAP_IMAGE>().Free(p)
#define STBI_REALLOC(p, newsz)              Platform::GetHeapAllocator<HEAP_IMAGE>().Realloc(p, newsz, 16)
#define STBI_REALLOC_SIZED(p, oldsz, newsz) STBI_REALLOC(p, newsz)
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_STDIO
#define STBI_NO_GIF // Maybe in future gif will be used, but not now
#include "stb/stb_image.h"

#define STBIW_MALLOC(sz)        Platform::GetHeapAllocator<HEAP_IMAGE>().Alloc(sz, 16)
#define STBIW_FREE(p)           Platform::GetHeapAllocator<HEAP_IMAGE>().Free(p)
#define STBIW_REALLOC(p, newsz) Platform::GetHeapAllocator<HEAP_IMAGE>().Realloc(p, newsz, 16)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
//#define STBI_WRITE_NO_STDIO
#define STBIW_ZLIB_COMPRESS      stbi_zlib_compress_override                   // Override compression method
#define STBIW_CRC32(buffer, len) Core::Crc32(Core::CRC32_INITIAL, buffer, len) // Override crc32 method
#define PNG_COMPRESSION_LEVEL    8

struct StbImageInit
{
    StbImageInit()
    {
        // Do not perform gamma space conversion
        stbi_hdr_to_ldr_gamma(1.0f);
        stbi_ldr_to_hdr_gamma(1.0f);
    }
};

static StbImageInit __StbImageInit;

// Miniz mz_compress2 provides better compression than default stb compression method
HK_FORCEINLINE unsigned char* stbi_zlib_compress_override(unsigned char* data, int data_len, int* out_len, int quality)
{
    size_t buflen = Core::ZMaxCompressedSize(data_len);

    unsigned char* buf = (unsigned char*)STBIW_MALLOC(buflen);
    if (buf == NULL)
    {
        LOG("stbi_zlib_compress_override Malloc failed\n");
        return NULL;
    }
    if (!Core::ZCompress(buf, &buflen, data, data_len, PNG_COMPRESSION_LEVEL))
    {
        LOG("stbi_zlib_compress_override ZCompress failed\n");
        STBIW_FREE(buf);
        return NULL;
    }
    *out_len = buflen;
    if (buflen == 0)
    { // compiler bug workaround (MSVC, Release)
        LOG("WritePNG: invalid compression\n");
    }
    return buf;
}

#include "stb/stb_image_write.h"

#define SUPPORT_EXR

static int Stbi_Read(void* user, char* data, int size)
{
    IBinaryStreamReadInterface* stream = (IBinaryStreamReadInterface*)user;
    return stream->Read(data, size);
}

static void Stbi_Skip(void* user, int n)
{
    IBinaryStreamReadInterface* stream = (IBinaryStreamReadInterface*)user;
    stream->SeekCur(n);
}

static int Stbi_Eof(void* user)
{
    IBinaryStreamReadInterface* stream = (IBinaryStreamReadInterface*)user;
    return stream->Eof();
}

static void Stbi_Write(void* context, void* data, int size)
{
    IBinaryStreamWriteInterface* stream = (IBinaryStreamWriteInterface*)context;
    stream->Write(data, size);
}

#ifdef SUPPORT_EXR

#    include <tinyexr/tinyexr.h>

HK_FORCEINLINE static uint8_t FloatToU8(float f)
{
    return Math::Round(Math::Saturate(f) * 255.0f);
}

static void* LoadEXR(IBinaryStreamReadInterface& Stream, int* w, int* h, int* channels, int desiredchannels, bool ldr)
{
    HK_ASSERT(desiredchannels >= 0 && desiredchannels <= 4);

    size_t streamOffset = Stream.GetOffset();

    HeapBlob memory = Stream.ReadBlob(Stream.SizeInBytes() - streamOffset);

    float* data;

    *channels = 0;

    int r = LoadEXRFromMemory(&data, w, h, (unsigned char*)memory.GetData(), memory.Size(), nullptr);

    memory.Reset();

    if (r != TINYEXR_SUCCESS)
    {
        Stream.SeekSet(streamOffset);
        return nullptr;
    }

    const int numChannels = 4;

    *channels = numChannels;

    void* result = nullptr;

    int pixcount = (*w) * (*h) * numChannels;

    if (ldr)
    {
        if (desiredchannels != 0 && desiredchannels != numChannels)
        {
            result           = STBI_MALLOC((*w) * (*h) * desiredchannels);
            uint8_t* pResult = (uint8_t*)result;
            if (desiredchannels == 1)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = FloatToU8(data[i]);
                }
            }
            else if (desiredchannels == 2)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = FloatToU8(data[i]);
                    *pResult++ = FloatToU8(data[i + 3]); // store alpha in second channel
                }
            }
            else if (desiredchannels == 3)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = FloatToU8(data[i]);
                    *pResult++ = FloatToU8(data[i + 1]);
                    *pResult++ = FloatToU8(data[i + 2]);
                }
            }
            else
            {
                HK_ASSERT(0);
            }
        }
        else
        {
            result = STBI_MALLOC((*w) * (*h) * numChannels);

            uint8_t* pResult = (uint8_t*)result;
            for (int i = 0; i < pixcount; i += numChannels)
            {
                *pResult++ = FloatToU8(data[i]);
                *pResult++ = FloatToU8(data[i + 1]);
                *pResult++ = FloatToU8(data[i + 2]);
                *pResult++ = FloatToU8(data[i + 3]);
            }
        }
    }
    else
    {
        if (desiredchannels != 0 && desiredchannels != numChannels)
        {
            result         = STBI_MALLOC((*w) * (*h) * desiredchannels * sizeof(float));
            float* pResult = (float*)result;
            if (desiredchannels == 1)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = data[i];
                }
            }
            else if (desiredchannels == 2)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = data[i];
                    *pResult++ = data[i + 3]; // store alpha in second channel
                }
            }
            else if (desiredchannels == 3)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = data[i];
                    *pResult++ = data[i + 1];
                    *pResult++ = data[i + 2];
                }
            }
            else
            {
                HK_ASSERT(0);
            }
        }
        else
        {
            result = STBI_MALLOC((*w) * (*h) * numChannels * sizeof(float));
            Platform::Memcpy(result, data, (*w) * (*h) * numChannels * sizeof(float));
        }
    }

    free(data); // tinyexr uses standard malloc/free

    return result;
}

#endif

// clang-format off
static const RawImageFormatInfo RawImageFormatLUT[] =
{
        0, 0,  // RAW_IMAGE_FORMAT_UNDEFINED,

        1, 1,  // RAW_IMAGE_FORMAT_R8,
        2, 2,  // RAW_IMAGE_FORMAT_R8_ALPHA,
        3, 3,  // RAW_IMAGE_FORMAT_RGB8,
        3, 3,  // RAW_IMAGE_FORMAT_BGR8,
        4, 4,  // RAW_IMAGE_FORMAT_RGBA8,
        4, 4,  // RAW_IMAGE_FORMAT_BGRA8,

        1, 4,  // RAW_IMAGE_FORMAT_R32_FLOAT,
        2, 8,  // RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT,
        3, 12, // RAW_IMAGE_FORMAT_RGB32_FLOAT,
        3, 12, // RAW_IMAGE_FORMAT_BGR32_FLOAT,
        4, 16, // RAW_IMAGE_FORMAT_RGBA32_FLOAT,
        4, 16, // RAW_IMAGE_FORMAT_BGRA32_FLOAT
};
// clang-format on

RawImageFormatInfo const& GetRawImageFormatInfo(RAW_IMAGE_FORMAT Format)
{
    return RawImageFormatLUT[Format];
}

static size_t CalcRawImageSize(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format)
{
    HK_VERIFY(Width, "CalcRawImageSize: Invalid image width");
    HK_VERIFY(Height, "CalcRawImageSize: Invalid image height");
    HK_VERIFY(Format != RAW_IMAGE_FORMAT_UNDEFINED, "CalcRawImageSize: Invalid image format");

    int numChannels = RawImageFormatLUT[Format].NumChannels;
    bool bHDRI;

    switch (Format)
    {
        case RAW_IMAGE_FORMAT_UNDEFINED:
            return 0;
        case RAW_IMAGE_FORMAT_R8:
        case RAW_IMAGE_FORMAT_R8_ALPHA:
        case RAW_IMAGE_FORMAT_RGB8:
        case RAW_IMAGE_FORMAT_BGR8:
        case RAW_IMAGE_FORMAT_RGBA8:
        case RAW_IMAGE_FORMAT_BGRA8:
            bHDRI = false;
            break;
        case RAW_IMAGE_FORMAT_R32_FLOAT:
        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            bHDRI = true;
            break;
        default:
            HK_ASSERT(0);
            return 0;
    }

    size_t size = Width * Height * numChannels;
    if (bHDRI)
        size *= sizeof(float);

    return size;
}

void ARawImage::Reset(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, void const* pData)
{
    Reset();

    size_t size = CalcRawImageSize(Width, Height, Format);
    if (!size)
        return;

    m_pData  = Platform::GetHeapAllocator<HEAP_IMAGE>().Alloc(size);
    m_Width  = Width;
    m_Height = Height;
    m_Format = Format;

    if (pData)
    {
        Platform::Memcpy(m_pData, pData, size);
    }
}

void ARawImage::Reset()
{
    Platform::GetHeapAllocator<HEAP_IMAGE>().Free(m_pData);
    m_pData = nullptr;
}

ARawImage ARawImage::Clone() const
{
    if (!m_pData)
        return {};

    ARawImage rawImage;
    rawImage.Reset(m_Width, m_Height, m_Format, m_pData);
    return rawImage;
}

int ARawImage::NumChannels() const
{
    return RawImageFormatLUT[m_Format].NumChannels;
}

size_t ARawImage::GetBytesPerPixel() const
{
    return RawImageFormatLUT[m_Format].BytesPerPixel;
}

void ARawImage::Clear(Float4 const& Color)
{
    if (m_Format == RAW_IMAGE_FORMAT_UNDEFINED)
        return;

    uint8_t unsignedInt8[4];

    unsignedInt8[0] = Math::Saturate(Color[0]) * 255.0f;
    unsignedInt8[1] = Math::Saturate(Color[1]) * 255.0f;
    unsignedInt8[2] = Math::Saturate(Color[2]) * 255.0f;
    unsignedInt8[3] = Math::Saturate(Color[3]) * 255.0f;

    uint32_t pixCount = m_Width * m_Height;

    switch (m_Format)
    {
        case RAW_IMAGE_FORMAT_R8:
            Platform::Memset(m_pData, unsignedInt8[0], pixCount);
            break;
        case RAW_IMAGE_FORMAT_R8_ALPHA:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((uint8_t*)m_pData)[n * 2 + 0] = unsignedInt8[0];
                ((uint8_t*)m_pData)[n * 2 + 1] = unsignedInt8[3];
            }
            break;
        case RAW_IMAGE_FORMAT_RGB8:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((uint8_t*)m_pData)[n * 3 + 0] = unsignedInt8[0];
                ((uint8_t*)m_pData)[n * 3 + 1] = unsignedInt8[1];
                ((uint8_t*)m_pData)[n * 3 + 2] = unsignedInt8[2];
            }
            break;
        case RAW_IMAGE_FORMAT_BGR8:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((uint8_t*)m_pData)[n * 3 + 0] = unsignedInt8[2];
                ((uint8_t*)m_pData)[n * 3 + 1] = unsignedInt8[1];
                ((uint8_t*)m_pData)[n * 3 + 2] = unsignedInt8[0];
            }
            break;
        case RAW_IMAGE_FORMAT_RGBA8:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((uint8_t*)m_pData)[n * 4 + 0] = unsignedInt8[0];
                ((uint8_t*)m_pData)[n * 4 + 1] = unsignedInt8[1];
                ((uint8_t*)m_pData)[n * 4 + 2] = unsignedInt8[2];
                ((uint8_t*)m_pData)[n * 4 + 3] = unsignedInt8[3];
            }
            break;
        case RAW_IMAGE_FORMAT_BGRA8:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((uint8_t*)m_pData)[n * 4 + 0] = unsignedInt8[2];
                ((uint8_t*)m_pData)[n * 4 + 1] = unsignedInt8[1];
                ((uint8_t*)m_pData)[n * 4 + 2] = unsignedInt8[0];
                ((uint8_t*)m_pData)[n * 4 + 3] = unsignedInt8[3];
            }
            break;
        case RAW_IMAGE_FORMAT_R32_FLOAT:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((float*)m_pData)[n] = Color[0];
            }
            break;
        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((Float2*)m_pData)[n] = Float2(Color[0], Color[3]);
            }
            break;
        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((Float3*)m_pData)[n] = Float3(Color);
            }
            break;
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((float*)m_pData)[n * 3 + 0] = Color[2];
                ((float*)m_pData)[n * 3 + 1] = Color[1];
                ((float*)m_pData)[n * 3 + 2] = Color[0];
            }
            break;
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((Float4*)m_pData)[n] = Color;
            }
            break;
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            for (uint32_t n = 0; n < pixCount; ++n)
            {
                ((float*)m_pData)[n * 4 + 0] = Color[2];
                ((float*)m_pData)[n * 4 + 1] = Color[1];
                ((float*)m_pData)[n * 4 + 2] = Color[0];
                ((float*)m_pData)[n * 4 + 3] = Color[3];
            }
            break;
        default:
            HK_ASSERT(0);
    }
}

void ARawImage::FlipX()
{
    if (m_pData)
    {
        size_t bytesPerPixel = RawImageFormatLUT[m_Format].BytesPerPixel;
        FlipImageX(m_pData, m_Width, m_Height, bytesPerPixel, m_Width * bytesPerPixel);
    }
}

void ARawImage::FlipY()
{
    if (m_pData)
    {
        size_t bytesPerPixel = RawImageFormatLUT[m_Format].BytesPerPixel;
        FlipImageY(m_pData, m_Width, m_Height, bytesPerPixel, m_Width * bytesPerPixel);
    }
}

template <typename T>
void SwapRGB(T* pData, uint32_t Width, uint32_t Height, uint32_t NumChannels)
{
    if (NumChannels < 3)
        return;

    T* d   = pData;
    T* d_e = d + Width * Height * NumChannels;
    while (d < d_e)
    {
        Core::Swap(d[0], d[2]);
        d += NumChannels;
    }
}

void ARawImage::SwapRGB()
{
    switch (m_Format)
    {
        case RAW_IMAGE_FORMAT_RGB8:
        case RAW_IMAGE_FORMAT_BGR8:
        case RAW_IMAGE_FORMAT_RGBA8:
        case RAW_IMAGE_FORMAT_BGRA8: {
            ::SwapRGB((uint8_t*)m_pData, m_Width, m_Height, NumChannels());
            break;
        }

        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT: {
            ::SwapRGB((float*)m_pData, m_Width, m_Height, NumChannels());
            break;
        }
        default:
            break;
    }
}

template <typename T>
void InvertChannel(T* pData, uint32_t Width, uint32_t Height, uint32_t NumChannels, uint32_t ChannelIndex, const T MaxValue)
{
    if (ChannelIndex >= NumChannels)
    {
        LOG("ARawImage::InvertChannel: channel index is out of range\n");
        return;
    }

    T* d   = pData;
    T* d_e = d + Width * Height * NumChannels;
    while (d < d_e)
    {
        d[ChannelIndex] = MaxValue - d[ChannelIndex];
        d += NumChannels;
    }
}

void ARawImage::InvertChannel(uint32_t ChannelIndex)
{
    switch (m_Format)
    {
        case RAW_IMAGE_FORMAT_R8:
        case RAW_IMAGE_FORMAT_R8_ALPHA:
        case RAW_IMAGE_FORMAT_RGB8:
        case RAW_IMAGE_FORMAT_BGR8:
        case RAW_IMAGE_FORMAT_RGBA8:
        case RAW_IMAGE_FORMAT_BGRA8: {
            ::InvertChannel((uint8_t*)m_pData, m_Width, m_Height, NumChannels(), ChannelIndex, uint8_t(255));
            break;
        }

        case RAW_IMAGE_FORMAT_R32_FLOAT:
        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT: {
            ::InvertChannel((float*)m_pData, m_Width, m_Height, NumChannels(), ChannelIndex, 1.0f);
            break;
        }
        default:
            break;
    }
}

static bool IsEXR(IBinaryStreamReadInterface& Stream)
{
#ifdef SUPPORT_EXR
    size_t streamOffset = Stream.GetOffset();

    const size_t kEXRVersionSize = 8;

    unsigned char buf[kEXRVersionSize];
    if (Stream.Read(buf, sizeof(buf)) != sizeof(buf))
    {
        Stream.SeekSet(streamOffset);
        return false;
    }
    Stream.SeekSet(streamOffset);

    EXRVersion version;
    return ParseEXRVersionFromMemory(&version, buf, sizeof(buf)) == TINYEXR_SUCCESS;
#else
    return false;
#endif
}

static bool IsHDR(IBinaryStreamReadInterface& Stream)
{
    const stbi_io_callbacks callbacks = {Stbi_Read, Stbi_Skip, Stbi_Eof};

    size_t streamOffset = Stream.GetOffset();

    bool result = stbi_is_hdr_from_callbacks(&callbacks, &Stream) != 0;

    Stream.SeekSet(streamOffset);
    return result;
}

bool IsHDRImage(IBinaryStreamReadInterface& Stream)
{
    return IsHDR(Stream) || IsEXR(Stream);
}

ARawImage CreateRawImage(IBinaryStreamReadInterface& Stream, RAW_IMAGE_FORMAT Format)
{
    if (!Stream.IsValid())
        return {};

    const stbi_io_callbacks callbacks = {Stbi_Read, Stbi_Skip, Stbi_Eof};

    int numRequiredChannels = RawImageFormatLUT[Format].NumChannels;
    bool bHDRI;

    switch (Format)
    {
        case RAW_IMAGE_FORMAT_UNDEFINED:
            bHDRI = IsHDRImage(Stream);
            break;
        case RAW_IMAGE_FORMAT_R8:
        case RAW_IMAGE_FORMAT_R8_ALPHA:
        case RAW_IMAGE_FORMAT_RGB8:
        case RAW_IMAGE_FORMAT_BGR8:
        case RAW_IMAGE_FORMAT_RGBA8:
        case RAW_IMAGE_FORMAT_BGRA8:
            bHDRI = false;
            break;
        case RAW_IMAGE_FORMAT_R32_FLOAT:
        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            bHDRI = true;
            break;
        default:
            HK_ASSERT(0);
            return {};
    }

    size_t streamOffset = Stream.GetOffset();

    int   w, h, numChannels;

    void* source = bHDRI ? (void*)stbi_loadf_from_callbacks(&callbacks, &Stream, &w, &h, &numChannels, numRequiredChannels) :
                           (void*)stbi_load_from_callbacks(&callbacks, &Stream, &w, &h, &numChannels, numRequiredChannels);

    if (!source)
    {
        Stream.SeekSet(streamOffset);

        if (Format == RAW_IMAGE_FORMAT_UNDEFINED)
            bHDRI = true;

#ifdef SUPPORT_EXR
        source = LoadEXR(Stream, &w, &h, &numChannels, numRequiredChannels, Format == RAW_IMAGE_FORMAT_UNDEFINED ? false : !bHDRI);
#endif
        if (!source)
        {
            Stream.SeekSet(streamOffset);

            LOG("CreateRawImage: couldn't load {}\n", Stream.GetName());
            return {};
        }
    }

    numChannels = numRequiredChannels ? numRequiredChannels : numChannels;

    if (Format == RAW_IMAGE_FORMAT_BGR8 || Format == RAW_IMAGE_FORMAT_BGRA8 || Format == RAW_IMAGE_FORMAT_BGR32_FLOAT || Format == RAW_IMAGE_FORMAT_BGRA32_FLOAT)
    {
        if (bHDRI)
        {
            float* d   = (float*)source;
            float* d_e = d + w * h * numChannels;
            while (d < d_e)
            {
                Core::Swap(d[0], d[2]);
                d += numChannels;
            }
        }
        else
        {
            uint8_t* d   = (uint8_t*)source;
            uint8_t* d_e = d + w * h * numChannels;
            while (d < d_e)
            {
                Core::Swap(d[0], d[2]);
                d += numChannels;
            }
        }
    }

    if (Format == RAW_IMAGE_FORMAT_UNDEFINED)
    {
        switch (numChannels)
        {
            case 1:
                Format = bHDRI ? RAW_IMAGE_FORMAT_R32_FLOAT : RAW_IMAGE_FORMAT_R8;
                break;
            case 2:
                Format = bHDRI ? RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT : RAW_IMAGE_FORMAT_R8_ALPHA;
                break;
            case 3:
                Format = bHDRI ? RAW_IMAGE_FORMAT_RGB32_FLOAT : RAW_IMAGE_FORMAT_RGB8;
                break;
            case 4:
                Format = bHDRI ? RAW_IMAGE_FORMAT_RGBA32_FLOAT : RAW_IMAGE_FORMAT_RGBA8;
                break;
            default:
                HK_ASSERT(0);
        }
    }

    ARawImage image;
    image.SetExternalData(w, h, Format, source);

    return image;
}

ARawImage CreateRawImage(AStringView FileName, RAW_IMAGE_FORMAT Format)
{
    return CreateRawImage(AFile::OpenRead(FileName).ReadInterface(), Format);
}

ARawImage CreateEmptyRawImage(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, Float4 const& Color)
{
    if (Format == RAW_IMAGE_FORMAT_UNDEFINED)
    {
        LOG("CreateEmptyRawImage: Expected valid image format\n");
        return {};
    }

    return ARawImage(Width, Height, Format, Color);
}

ARawImage LoadNormalMapAsRawVectors(IBinaryStreamReadInterface& Stream)
{
    ARawImage rawImage = CreateRawImage(Stream, RAW_IMAGE_FORMAT_RGB32_FLOAT);
    if (!rawImage)
        return {};

    Float3* pNormal = (Float3*)rawImage.GetData();
    Float3* end     = pNormal + rawImage.GetWidth() * rawImage.GetHeight();

    while (pNormal < end)
    {
        *pNormal = *pNormal * 2.0f - 1.0f;
        pNormal->NormalizeSelf();
        ++pNormal;
    }
    return rawImage;
}

ARawImage LoadNormalMapAsRawVectors(AStringView FileName)
{
    AFile f = AFile::OpenRead(FileName);
    if (!f)
    {
        LOG("LoadNormalMapAsRawVectors: couldn't open {}\n", FileName);
        return {};
    }
    return LoadNormalMapAsRawVectors(f);
}

static void MemSwap(uint8_t* Block, size_t BlockSz, uint8_t* _Ptr1, uint8_t* _Ptr2, size_t _Size)
{
    const size_t blockCount = _Size / BlockSz;
    size_t       i;
    for (i = 0; i < blockCount; i++)
    {
        Platform::Memcpy(Block, _Ptr1, BlockSz);
        Platform::Memcpy(_Ptr1, _Ptr2, BlockSz);
        Platform::Memcpy(_Ptr2, Block, BlockSz);
        _Ptr2 += BlockSz;
        _Ptr1 += BlockSz;
    }
    i = _Size - i * BlockSz;
    if (i > 0)
    {
        Platform::Memcpy(Block, _Ptr1, i);
        Platform::Memcpy(_Ptr1, _Ptr2, i);
        Platform::Memcpy(_Ptr2, Block, i);
    }
}

void FlipImageX(void* pData, uint32_t Width, uint32_t Height, size_t BytesPerPixel, size_t RowStride)
{
    size_t lineWidth = Width * BytesPerPixel;
    size_t halfWidth = Width >> 1;
    uint8_t* temp      = (uint8_t*)StackAlloc(BytesPerPixel);
    uint8_t* image     = (uint8_t*)pData;
    for (size_t y = 0; y < Height; y++)
    {
        uint8_t* s = image;
        uint8_t* e = image + lineWidth;
        for (size_t x = 0; x < halfWidth; x++)
        {
            e -= BytesPerPixel;
            Platform::Memcpy(temp, s, BytesPerPixel);
            Platform::Memcpy(s, e, BytesPerPixel);
            Platform::Memcpy(e, temp, BytesPerPixel);
            s += BytesPerPixel;
        }
        image += RowStride;
    }
}

void FlipImageY(void* pData, uint32_t Width, uint32_t Height, size_t BytesPerPixel, size_t RowStride)
{
    const size_t blockSizeInBytes = 4096;
    uint8_t      block[blockSizeInBytes];
    size_t       lineWidth  = Width * BytesPerPixel;
    size_t       halfHeight = Height >> 1;
    uint8_t*     image      = (uint8_t*)pData;
    uint8_t*     e          = image + Height * RowStride;
    for (size_t y = 0; y < halfHeight; y++)
    {
        e -= RowStride;
        MemSwap(block, blockSizeInBytes, image, e, lineWidth);
        image += RowStride;
    }
}

void LinearToPremultipliedAlphaSRGB(const float* pSrc,
                                    void*        pDest_SRGBA8,
                                    uint32_t     Width,
                                    uint32_t     Height,
                                    float        fOverbright)
{
    const float* src = pSrc;
    uint8_t*     dst = (uint8_t*)pDest_SRGBA8;
    float        r, g, b;

    for (uint32_t i = 0, pixCount = Width * Height; i < pixCount; i++, src += 4, dst += 4)
    {
        r = src[0] * src[3];
        g = src[1] * src[3];
        b = src[2] * src[3];

        if (fOverbright > 0.0f)
        {
            r *= fOverbright;
            g *= fOverbright;
            b *= fOverbright;
#if 1
            float m = Math::Max3(r, g, b);
            if (m > 1.0f)
            {
                m = 1.0f / m;
                r *= m;
                g *= m;
                b *= m;
            }
#else
            if (r > 1.0f) r = 1.0f;
            if (g > 1.0f) g = 1.0f;
            if (b > 1.0f) b = 1.0f;
#endif
        }

        dst[0] = LinearToSRGB_UChar(r);
        dst[1] = LinearToSRGB_UChar(g);
        dst[2] = LinearToSRGB_UChar(b);
        dst[3] = FloatToU8(src[3]);
    }
}

bool WritePNG(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData)
{
    return !!stbi_write_png_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData, Width * NumChannels);
}

bool WriteBMP(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData)
{
    return !!stbi_write_bmp_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData);
}

bool WriteTGA(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData)
{
    return !!stbi_write_tga_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData);
}

bool WriteJPG(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData, float Quality)
{
    return !!stbi_write_jpg_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData, (int)(Math::Saturate(Quality) * 99 + 1));
}

bool WriteHDR(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const float* pData)
{
    return !!stbi_write_hdr_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData);
}

bool WriteEXR(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const float* pData, bool bSaveAsHalf)
{
#ifdef SUPPORT_EXR
    EXRChannelInfo channels[4];
    int            pixelTypes[4];
    int            requestedPixelTypes[4];
    float*         layers[4] = {0, 0, 0, 0};
    uint32_t       numPixels = Width * Height;
    uint32_t       numWriteChannels;

    Platform::ZeroMem(channels, sizeof(channels));

    switch (NumChannels)
    {
        case 1:
            Platform::Strcpy(channels[0].name, sizeof(channels[0].name), "A");
            numWriteChannels = 1;
            break;
        case 2:
            Platform::Strcpy(channels[0].name, sizeof(channels[0].name), "A");
            Platform::Strcpy(channels[1].name, sizeof(channels[1].name), "B");
            Platform::Strcpy(channels[2].name, sizeof(channels[2].name), "G");
            Platform::Strcpy(channels[3].name, sizeof(channels[3].name), "R");
            numWriteChannels = 4;
            break;
        case 3:
            Platform::Strcpy(channels[0].name, sizeof(channels[0].name), "B");
            Platform::Strcpy(channels[1].name, sizeof(channels[1].name), "G");
            Platform::Strcpy(channels[2].name, sizeof(channels[2].name), "R");
            numWriteChannels = 3;
            break;
        case 4:
            Platform::Strcpy(channels[0].name, sizeof(channels[0].name), "A");
            Platform::Strcpy(channels[1].name, sizeof(channels[1].name), "B");
            Platform::Strcpy(channels[2].name, sizeof(channels[2].name), "G");
            Platform::Strcpy(channels[3].name, sizeof(channels[3].name), "R");
            numWriteChannels = 4;
            break;
        default:
            LOG("WriteEXR: Unsupported channel count {}\n", NumChannels);
            return false;
    }

    for (uint32_t i = 0; i < numWriteChannels; i++)
    {
        pixelTypes[i]          = TINYEXR_PIXELTYPE_FLOAT;
        requestedPixelTypes[i] = bSaveAsHalf ? TINYEXR_PIXELTYPE_HALF : TINYEXR_PIXELTYPE_FLOAT;
    }

    HeapBlob channelData;

    if (NumChannels == 1)
    {
        layers[0] = const_cast<float*>(pData);
    }
    else
    {
        channelData.Reset(NumChannels * numPixels * sizeof(float));

        layers[0] = (float*)channelData.GetData();
        layers[1] = layers[0] + numPixels;
        layers[2] = layers[0] + numPixels * 2;
        layers[3] = layers[0] + numPixels * 3;

        if (NumChannels == 2)
        {
            ExtractImageChannel(pData, layers[0], Width, Height, NumChannels, 1); // A
            ExtractImageChannel(pData, layers[1], Width, Height, NumChannels, 0); // B
            layers[2] = layers[1];
            layers[3] = layers[1];
        }
        else if (NumChannels == 3)
        {
            ExtractImageChannel(pData, layers[0], Width, Height, NumChannels, 2); // B
            ExtractImageChannel(pData, layers[1], Width, Height, NumChannels, 1); // G
            ExtractImageChannel(pData, layers[2], Width, Height, NumChannels, 0); // R
        }
        else if (NumChannels == 4)
        {
            ExtractImageChannel(pData, layers[0], Width, Height, NumChannels, 3); // A
            ExtractImageChannel(pData, layers[1], Width, Height, NumChannels, 2); // B
            ExtractImageChannel(pData, layers[2], Width, Height, NumChannels, 1); // G
            ExtractImageChannel(pData, layers[3], Width, Height, NumChannels, 0); // R
        }
    }

    EXRImage image;
    InitEXRImage(&image);

    image.images       = reinterpret_cast<unsigned char**>(layers);
    image.width        = Width;
    image.height       = Height;
    image.num_channels = numWriteChannels;

    EXRHeader header;
    InitEXRHeader(&header);

    header.channels              = channels;
    header.pixel_types           = pixelTypes;
    header.num_channels          = numWriteChannels;
    header.compression_type      = (Width < 16) && (Height < 16) ? TINYEXR_COMPRESSIONTYPE_NONE : TINYEXR_COMPRESSIONTYPE_ZIP;
    header.requested_pixel_types = requestedPixelTypes;

    unsigned char* memory = nullptr;
    size_t         size   = SaveEXRImageToMemory(&image, &header, &memory, nullptr);

    if (size == 0)
        return false;

    size_t bytesToWrite = Stream.Write(memory, size);

    free(memory);

    return bytesToWrite == size;
#else
    LOG("WriteEXR: EXR is not supported\n");
    return false;
#endif
}

bool WriteImage(AStringView FileName, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData)
{
    AStringView ext = PathUtils::GetExt(FileName);

    if (ext.Icompare(".hdr") || ext.Icompare(".exr"))
    {
        LOG("WriteImage: Use WriteImgeHDRI to save as .hdr or .exr\n");
        return false;
    }

    if (!(ext.Icompare(".png") || ext.Icompare(".bmp") || ext.Icompare(".tga") || ext.Icompare(".jpg") || ext.Icompare(".jpeg")))
    {
        LOG("WriteImage: Unknown image extension {}\n", FileName);
        return false;
    }

    AFile f = AFile::OpenWrite(FileName);
    if (!f)
        return false;

    bool result = false;

    if (ext.Icompare(".png"))
        result = WritePNG(f, Width, Height, NumChannels, pData);
    else if (ext.Icompare(".bmp"))
        result = WriteBMP(f, Width, Height, NumChannels, pData);
    else if (ext.Icompare(".tga"))
        result = WriteTGA(f, Width, Height, NumChannels, pData);
    else if (ext.Icompare(".jpg") || ext.Icompare(".jpeg"))
        result = WriteJPG(f, Width, Height, NumChannels, pData, 0.95f);

    if (!result)
    {
        LOG("WriteImage: Failed to write image {}\n", FileName);
    }
    
    return result;
}

bool WriteImageHDRI(AStringView FileName, uint32_t Width, uint32_t Height, uint32_t NumChannels, const float* pData)
{
    AStringView ext = PathUtils::GetExt(FileName);

    if (!(ext.Icompare(".hdr") || ext.Icompare(".exr")))
    {
        LOG("WriteImageHDRI: Expected .hdr or .exr extension\n");
        return false;
    }

    AFile f = AFile::OpenWrite(FileName);
    if (!f)
        return false;

    bool result = false;

    if (ext.Icompare(".hdr"))
        result = WriteHDR(f, Width, Height, NumChannels, pData);
    else if (ext.Icompare(".exr"))
        result = WriteEXR(f, Width, Height, NumChannels, pData, true);

    if (!result)
    {
        LOG("WriteImageHDRI: Failed to write image {}\n", FileName);
    }

    return result;
}

bool WriteImage(AStringView FileName, ARawImage const& Image)
{
    bool bHDRI = false;
    switch (Image.GetFormat())
    {
        case RAW_IMAGE_FORMAT_R8:
        case RAW_IMAGE_FORMAT_R8_ALPHA:
        case RAW_IMAGE_FORMAT_RGB8:
        case RAW_IMAGE_FORMAT_BGR8:
        case RAW_IMAGE_FORMAT_RGBA8:
        case RAW_IMAGE_FORMAT_BGRA8:
            break;
        case RAW_IMAGE_FORMAT_R32_FLOAT:
        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            bHDRI = true;
            break;
        default:
            LOG("WriteImage: unknown image format\n");
            return false;
    }

    AStringView ext = PathUtils::GetExt(FileName);
    bool        bHDRIExt = ext.Icompare(".hdr") || ext.Icompare(".exr");

    if (!bHDRI)
    {
        if (bHDRIExt)
        {
            LOG("WriteImage: attempted to save LDR image in HDRI format {}\n", FileName);
            return false;
        }
        return WriteImage(FileName, Image.GetWidth(), Image.GetHeight(), Image.NumChannels(), Image.GetData());
    }
    else
    {
        if (!bHDRIExt)
        {
            LOG("WriteImage: attempted to save HDR image in LDR format {}\n", FileName);
            return false;
        }
        return WriteImageHDRI(FileName, Image.GetWidth(), Image.GetHeight(), Image.NumChannels(), (const float*)Image.GetData());
    }
}
