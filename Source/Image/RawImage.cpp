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
#include <Image/SvgDocument.h>
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
#define SUPPORT_WEBP

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

HK_FORCEINLINE static uint8_t FloatToU8(float f)
{
    return Math::Round(Math::Saturate(f) * 255.0f);
}

static bool LoadImageStb(IBinaryStreamReadInterface& Stream, uint32_t NumRequiredChannels, bool bAsHDRI, uint32_t* pWidth, uint32_t* pHeight, uint32_t* pNumChannels, void** ppData)
{
    HK_ASSERT(NumRequiredChannels <= 4);
    
    int w, h, numChannels;
        
    const stbi_io_callbacks callbacks = {Stbi_Read, Stbi_Skip, Stbi_Eof};
    
    size_t streamOffset = Stream.GetOffset();

    void* source = bAsHDRI ? (void*)stbi_loadf_from_callbacks(&callbacks, &Stream, &w, &h, &numChannels, NumRequiredChannels) :
                             (void*)stbi_load_from_callbacks(&callbacks, &Stream, &w, &h, &numChannels, NumRequiredChannels);
                                 
    if (!source)
    {
        Stream.SeekSet(streamOffset);
        LOG("LoadImage: failed to load {}\n", Stream.GetName());
        return false;
    }
        
    *pWidth = w;
    *pHeight = h;
    *pNumChannels = numChannels;
    *ppData = source;
    return true;
}

#ifdef SUPPORT_EXR

#include <tinyexr/tinyexr.h>

static bool LoadImageEXR(IBinaryStreamReadInterface& Stream, uint32_t NumRequiredChannels, bool bAsHDRI, uint32_t* pWidth, uint32_t* pHeight, uint32_t* pNumChannels, void** ppData)
{   
    HK_ASSERT(NumRequiredChannels <= 4);

    size_t streamOffset = Stream.GetOffset();

    int w, h;
    float* source;
    {
        HeapBlob memory = Stream.ReadBlob(Stream.SizeInBytes() - streamOffset);
        
        int r = LoadEXRFromMemory(&source, &w, &h, (unsigned char*)memory.GetData(), memory.Size(), nullptr);
        if (r != TINYEXR_SUCCESS)
        {
            Stream.SeekSet(streamOffset);
            LOG("LoadImageEXR: failed to load {}\n", Stream.GetName());
            return false;
        }
    }

    const int numChannels = 4;
    
    if (!NumRequiredChannels)
        NumRequiredChannels = numChannels;
    
    size_t pixcount = (size_t)w * h * numChannels;

    void* data = nullptr;

    if (bAsHDRI)
    {
        if (NumRequiredChannels != numChannels)
        {
            data         = STBI_MALLOC((size_t)w * h * NumRequiredChannels * sizeof(float));
            float* pDest = (float*)data;
            if (NumRequiredChannels == 1)
            {
                for (size_t i = 0; i < pixcount; i += numChannels)
                {
                    *pDest++ = source[i];
                }
            }
            else if (NumRequiredChannels == 2)
            {
                for (size_t i = 0; i < pixcount; i += numChannels)
                {
                    *pDest++ = source[i];
                    *pDest++ = source[i + 3]; // store alpha in second channel
                }
            }
            else if (NumRequiredChannels == 3)
            {
                for (size_t i = 0; i < pixcount; i += numChannels)
                {
                    *pDest++ = source[i];
                    *pDest++ = source[i + 1];
                    *pDest++ = source[i + 2];
                }
            }
            else
            {
                HK_ASSERT(0);
            }
        }
        else
        {
            data = STBI_MALLOC((size_t)w * h * numChannels * sizeof(float));
            Platform::Memcpy(data, source, (size_t)w * h * numChannels * sizeof(float));
        }
    }
    else
    {
        if (NumRequiredChannels != numChannels)
        {
            data           = STBI_MALLOC((size_t)w * h * NumRequiredChannels);
            uint8_t* pDest = (uint8_t*)data;
            if (NumRequiredChannels == 1)
            {
                for (size_t i = 0; i < pixcount; i += numChannels)
                {
                    *pDest++ = FloatToU8(source[i]);
                }
            }
            else if (NumRequiredChannels == 2)
            {
                for (size_t i = 0; i < pixcount; i += numChannels)
                {
                    *pDest++ = FloatToU8(source[i]);
                    *pDest++ = FloatToU8(source[i + 3]); // store alpha in second channel
                }
            }
            else if (NumRequiredChannels == 3)
            {
                for (size_t i = 0; i < pixcount; i += numChannels)
                {
                    *pDest++ = FloatToU8(source[i]);
                    *pDest++ = FloatToU8(source[i + 1]);
                    *pDest++ = FloatToU8(source[i + 2]);
                }
            }
            else
            {
                HK_ASSERT(0);
            }
        }
        else
        {
            data = STBI_MALLOC((size_t)w * h * numChannels);

            uint8_t* pDest = (uint8_t*)data;
            for (size_t i = 0; i < pixcount; i += numChannels)
            {
                *pDest++ = FloatToU8(source[i]);
                *pDest++ = FloatToU8(source[i + 1]);
                *pDest++ = FloatToU8(source[i + 2]);
                *pDest++ = FloatToU8(source[i + 3]);
            }
        }
    }

    free(source); // tinyexr uses standard malloc/free
    
    *pWidth = w;
    *pHeight = h;
    *pNumChannels = numChannels;
    *ppData = data;

    return true;
}

#else

static bool LoadImageEXR(IBinaryStreamReadInterface& Stream, uint32_t NumRequiredChannels, bool bAsHDRI, uint32_t* pWidth, uint32_t* pHeight, uint32_t* pNumChannels, void** ppData)
{
    LOG("LoadImageEXR: exr images are not allowed\n);
    return false;
}

#endif

#ifdef SUPPORT_WEBP

#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/mux.h>

static bool LoadImageWebp(IBinaryStreamReadInterface& Stream, uint32_t NumRequiredChannels, bool bAsHDRI, uint32_t* pWidth, uint32_t* pHeight, uint32_t* pNumChannels, void** ppData)
{   
    HK_ASSERT(NumRequiredChannels <= 4);

    size_t streamOffset = Stream.GetOffset();
    
    HeapBlob memory = Stream.ReadBlob(Stream.SizeInBytes() - streamOffset);
        
    WebPBitstreamFeatures features;

	if (WebPGetFeatures((const uint8_t*)memory.GetData(), memory.Size(), &features) != VP8_STATUS_OK)
	{
        Stream.SeekSet(streamOffset);
        LOG("LoadImageWebp: failed to load {}\n", Stream.GetName());
        return false;
    }
    
    uint32_t numChannels = features.has_alpha ? 4 : 3;
    if (!NumRequiredChannels)
        NumRequiredChannels = numChannels;
    
    uint8_t* pixels = nullptr;
    bool decoded = false;
    
    if (NumRequiredChannels == 3 || NumRequiredChannels == 4)
    {
        size_t sizeInBytes = (size_t)features.width * features.height * NumRequiredChannels;
        pixels = (uint8_t*)STBI_MALLOC(sizeInBytes);
        decoded = NumRequiredChannels == 4 ?
                       WebPDecodeRGBAInto((const uint8_t*)memory.GetData(), memory.Size(), pixels, sizeInBytes, NumRequiredChannels * features.width) != nullptr
                    :
                       WebPDecodeRGBInto((const uint8_t*)memory.GetData(), memory.Size(), pixels, sizeInBytes, NumRequiredChannels * features.width) != nullptr;
        if (!decoded)
            STBI_FREE(pixels);
    }
    else if (NumRequiredChannels == 1)
    {
        size_t sizeInBytes = (size_t)features.width * features.height * 3;
        pixels = (uint8_t*)STBI_MALLOC(sizeInBytes);
        decoded = WebPDecodeRGBInto((const uint8_t*)memory.GetData(), memory.Size(), pixels, sizeInBytes, 3 * features.width) != nullptr;
        if (decoded)
        {
            uint8_t* temp  = (uint8_t*)STBI_MALLOC((size_t)features.width * features.height * NumRequiredChannels);
            uint8_t* pDest = (uint8_t*)temp;
                
            for (size_t i = 0; i < sizeInBytes; i += 3)
            {
                *pDest++ = pixels[i];
            }
            STBI_FREE(pixels);
            pixels = temp;
        }
        else
            STBI_FREE(pixels);
    }
    else if (NumRequiredChannels == 2)
    {
        size_t sizeInBytes = (size_t)features.width * features.height * 4;
        pixels = (uint8_t*)STBI_MALLOC(sizeInBytes);
        decoded = WebPDecodeRGBAInto((const uint8_t*)memory.GetData(), memory.Size(), pixels, sizeInBytes, 4 * features.width) != nullptr;
        if (decoded)
        {
            uint8_t* temp  = (uint8_t*)STBI_MALLOC((size_t)features.width * features.height * NumRequiredChannels);
            uint8_t* pDest = (uint8_t*)temp;
                
            // Convert to Red_Alpha
            for (size_t i = 0; i < sizeInBytes; i += 4)
            {
                *pDest++ = pixels[i];
                *pDest++ = pixels[i + 3]; // store alpha in second channel
            }
            STBI_FREE(pixels);
            pixels = temp;
        }
        else
            STBI_FREE(pixels);
    }

    if (!decoded)
	{
        Stream.SeekSet(streamOffset);
        LOG("LoadImageWebp: failed to decode {}\n", Stream.GetName());
	    return false;
    }
    
    if (bAsHDRI)
    {
        // Convert to HDRI
        
        size_t pixcount = (size_t)features.width * features.height * NumRequiredChannels;
        
        float* temp = (float*)STBI_MALLOC(pixcount * sizeof(float));

        for (size_t i = 0; i < pixcount; i++)
        {
            temp[i] = pixels[i] / 255.0f;
        }
        
        STBI_FREE(pixels);
        pixels = (uint8_t*)temp;
    }
    
    *pWidth = features.width;
    *pHeight = features.height;
    *pNumChannels = numChannels;
    *ppData = pixels;

    return true;
}

#else

static bool LoadImageWebp(IBinaryStreamReadInterface& Stream, uint32_t NumRequiredChannels, bool bAsHDRI, uint32_t* pWidth, uint32_t* pHeight, uint32_t* pNumChannels, void** ppData)
{
    LOG("LoadImageWebp: webp images are not allowed\n");
    return false;
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

    size_t size = (size_t)Width * Height * numChannels;
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
    if (!m_pData)
        return;
    
    Platform::GetHeapAllocator<HEAP_IMAGE>().Free(m_pData);
    
    m_pData  = nullptr;
    m_Width  = 0;
    m_Height = 0;
    m_Format = RAW_IMAGE_FORMAT_UNDEFINED;
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

    unsignedInt8[0] = FloatToU8(Color[0]);
    unsignedInt8[1] = FloatToU8(Color[1]);
    unsignedInt8[2] = FloatToU8(Color[2]);
    unsignedInt8[3] = FloatToU8(Color[3]);

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

void ARawImage::PremultiplyAlpha()
{
    if (m_Format != RAW_IMAGE_FORMAT_RGBA8 && m_Format != RAW_IMAGE_FORMAT_BGRA8)
    {
        LOG("ARawImage::PremultiplyAlpha: Expected image format RAW_IMAGE_FORMAT_RGBA8 or RAW_IMAGE_FORMAT_BGRA8\n");
        return;
    }

    uint8_t* data   = (uint8_t*)m_pData;
    uint8_t* data_e = (uint8_t*)m_pData + (size_t)m_Width * m_Height * 4;

    while (data < data_e)
    {
        if (data[3] != 255)
        {
            float scale = data[3] / 255.0f;

            data[0] = LinearToSRGB_UChar(LinearFromSRGB_UChar(data[0]) * scale);
            data[1] = LinearToSRGB_UChar(LinearFromSRGB_UChar(data[1]) * scale);
            data[2] = LinearToSRGB_UChar(LinearFromSRGB_UChar(data[2]) * scale);
        }

        data += 4;
    }
}

void ARawImage::UnpremultiplyAlpha()
{
    if (m_Format != RAW_IMAGE_FORMAT_RGBA8 && m_Format != RAW_IMAGE_FORMAT_BGRA8)
    {
        LOG("ARawImage::UnpremultiplyAlpha: Expected image format RAW_IMAGE_FORMAT_RGBA8 or RAW_IMAGE_FORMAT_BGRA8\n");
        return;
    }

    uint8_t* data   = (uint8_t*)m_pData;
    uint8_t* data_e = (uint8_t*)m_pData + (size_t)m_Width * m_Height * 4;

    while (data < data_e)
    {
        if (data[3] != 0)
        {
            float scale = 255.0f / data[3];

            data[0] = LinearToSRGB_UChar(LinearFromSRGB_UChar(data[0]) * scale);
            data[1] = LinearToSRGB_UChar(LinearFromSRGB_UChar(data[1]) * scale);
            data[2] = LinearToSRGB_UChar(LinearFromSRGB_UChar(data[2]) * scale);
        }

        data += 4;
    }
}

static bool IsTga(IBinaryStreamReadInterface& Stream)
{
    // From stb_image
   
    // discard Offset
    Stream.SeekCur(1);
   
    uint8_t colorType = Stream.ReadUInt8();
   
    // only RGB or indexed allowed
    if (colorType > 1)
        return false;
   
    uint8_t imageType = Stream.ReadUInt8();
   
    if (colorType == 1)
    {
        // colormapped (paletted) image
        if (imageType != 1 && imageType != 9)
            return false; // colortype 1 demands image type 1 or 9
      
        // skip index of first colormap entry and number of entries
        Stream.SeekCur(4);
      
        // Сheck bits per palette color entry
        uint8_t bitsCount = Stream.ReadUInt8();
      
        if ( (bitsCount != 8) && (bitsCount != 15) && (bitsCount != 16) && (bitsCount != 24) && (bitsCount != 32) )
            return false;
      
        // skip image x and y origin
        Stream.SeekCur(4);
    }
    else
    {
        // "normal" image w/o colormap
        if ((imageType != 2) && (imageType != 3) && (imageType != 10) && (imageType != 11))
            return false; // only RGB or grey allowed, +/- RLE
      
        // skip colormap specification and image x/y origin
        Stream.SeekCur(9);
    }
   
    // test width
    if (Stream.ReadUInt16() < 1)
        return false;
   
    // test height
    if (Stream.ReadUInt16() < 1)
        return false;
   
    uint8_t bitsCount = Stream.ReadUInt8();
   
    if ((colorType == 1) && (bitsCount != 8) && (bitsCount != 16))
        return false; // for colormapped images, bpp is size of an index
   
    if ((bitsCount != 8) && (bitsCount != 15) && (bitsCount != 16) && (bitsCount != 24) && (bitsCount != 32))
        return false;

    return true;
}

static IMAGE_FILE_FORMAT GetImageFileFormatInternal(IBinaryStreamReadInterface& Stream)
{   
    size_t streamOffset = Stream.GetOffset();
    
    uint8_t fileSignature[8];
    
    Stream.Read(fileSignature, sizeof(fileSignature));
    
    const uint8_t pngSignature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (memcmp(fileSignature, pngSignature, 8) == 0)
    {
        return IMAGE_FILE_FORMAT_PNG;
    }
    
    if (fileSignature[0] == 'B' && fileSignature[1] == 'M')
    {
        Stream.SeekSet(streamOffset + 14);
        uint32_t sz = Stream.ReadUInt32();
        if (sz == 12 || sz == 40 || sz == 56 || sz == 108 || sz == 124)
            return IMAGE_FILE_FORMAT_BMP;
    }
    
    if (memcmp(fileSignature, "8BPS", 4) == 0)
    {
        return IMAGE_FILE_FORMAT_PSD;
    }
    
    if (memcmp(fileSignature, "\x53\x80\xF6\x34", 4) == 0)
    {
        Stream.SeekSet(streamOffset + 88);
        
        uint8_t tag[4];
        Stream.Read(tag, sizeof(tag));
        if (memcmp(tag, "PICT", 4) == 0)
        {
            return IMAGE_FILE_FORMAT_PIC;
        }
    }

    if (fileSignature[0] == 'P' && (fileSignature[1] == '5' || fileSignature[1] == '6'))
    {
        return IMAGE_FILE_FORMAT_PNM;
    }

#ifdef SUPPORT_WEBP
    if (memcmp(fileSignature, "RIFF", 4) == 0)
    {
        Stream.SeekSet(streamOffset + 8);

        uint8_t tag[4];
        Stream.Read(tag, sizeof(tag));

        if (memcmp(tag, "WEBP", sizeof(tag)) == 0)
        {
            return IMAGE_FILE_FORMAT_WEBP;
        }
    }
#endif

    if (memcmp(fileSignature, "#?RGBE\n", 7) == 0)
    {
        return IMAGE_FILE_FORMAT_HDR;
    }
    
    Stream.SeekSet(streamOffset);
    
    uint8_t hdrSignature[11];
    Stream.Read(hdrSignature, sizeof(hdrSignature));
    if (memcmp(hdrSignature, "#?RADIANCE\n", 11) == 0)
    {
        return IMAGE_FILE_FORMAT_HDR;
    }
    
#ifdef SUPPORT_EXR
    EXRVersion version;
    if (ParseEXRVersionFromMemory(&version, fileSignature, sizeof(fileSignature)) == TINYEXR_SUCCESS)
    {
        return IMAGE_FILE_FORMAT_EXR;
    }
#endif
   
    if (fileSignature[0] == 0xff)
    {
        Stream.SeekSet(streamOffset + 1);
        
        uint8_t marker;
        do {
            marker = Stream.ReadUInt8();
        } while (marker == 0xff);
        
        if (marker == 0xd8)
        {
            return IMAGE_FILE_FORMAT_JPEG;
        }
    }
   
    Stream.SeekSet(streamOffset);
    if (IsTga(Stream))
    {
        return IMAGE_FILE_FORMAT_TGA;
    }

    return IMAGE_FILE_FORMAT_UNKNOWN;
}

IMAGE_FILE_FORMAT GetImageFileFormat(IBinaryStreamReadInterface& Stream)
{
    size_t streamOffset = Stream.GetOffset();
    IMAGE_FILE_FORMAT format = GetImageFileFormatInternal(Stream);
    Stream.SeekSet(streamOffset);
    return format;
}

struct ExtensionToFileFormatMapping
{
    IMAGE_FILE_FORMAT FileFormat;
    AStringView       Extension;
};

static ExtensionToFileFormatMapping ExtensionToFileFormatMappings[] = {
    {IMAGE_FILE_FORMAT_JPEG, ".jpg"},
    {IMAGE_FILE_FORMAT_JPEG, ".jpeg"},
    {IMAGE_FILE_FORMAT_PNG, ".png"},
    {IMAGE_FILE_FORMAT_TGA, ".tga"},
    {IMAGE_FILE_FORMAT_BMP, ".bmp"},
    {IMAGE_FILE_FORMAT_PSD, ".psd"},
    {IMAGE_FILE_FORMAT_PIC, ".pic"},
    {IMAGE_FILE_FORMAT_PNM, ".pnm"},
    {IMAGE_FILE_FORMAT_WEBP, ".webp"},
    {IMAGE_FILE_FORMAT_HDR, ".hdr"},
    {IMAGE_FILE_FORMAT_EXR, ".exr"}};

IMAGE_FILE_FORMAT GetImageFileFormat(AStringView FileName)
{
    AStringView extension = PathUtils::GetExt(FileName);

    for (ExtensionToFileFormatMapping& mapping : ExtensionToFileFormatMappings)
    {
        if (!extension.Icmp(mapping.Extension))
            return mapping.FileFormat;
    }
    return IMAGE_FILE_FORMAT_UNKNOWN;
}

ARawImage CreateRawImage(IBinaryStreamReadInterface& Stream, RAW_IMAGE_FORMAT Format)
{
    if (!Stream.IsValid())
        return {};

    const uint32_t numRequiredChannels = RawImageFormatLUT[Format].NumChannels;
    bool bAsHDRI;
    
    IMAGE_FILE_FORMAT fileFormat = GetImageFileFormat(Stream);

    switch (Format)
    {
        case RAW_IMAGE_FORMAT_UNDEFINED:
            bAsHDRI = fileFormat == IMAGE_FILE_FORMAT_HDR || fileFormat == IMAGE_FILE_FORMAT_EXR;
            break;
        case RAW_IMAGE_FORMAT_R8:
        case RAW_IMAGE_FORMAT_R8_ALPHA:
        case RAW_IMAGE_FORMAT_RGB8:
        case RAW_IMAGE_FORMAT_BGR8:
        case RAW_IMAGE_FORMAT_RGBA8:
        case RAW_IMAGE_FORMAT_BGRA8:
            bAsHDRI = false;
            break;
        case RAW_IMAGE_FORMAT_R32_FLOAT:
        case RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT:
        case RAW_IMAGE_FORMAT_RGB32_FLOAT:
        case RAW_IMAGE_FORMAT_BGR32_FLOAT:
        case RAW_IMAGE_FORMAT_RGBA32_FLOAT:
        case RAW_IMAGE_FORMAT_BGRA32_FLOAT:
            bAsHDRI = true;
            break;
        default:
            HK_ASSERT(0);
            return {};
    }
    
    uint32_t w, h, numChannels;
    void* data;
    
    switch (fileFormat)
    {
        case IMAGE_FILE_FORMAT_JPEG:
        case IMAGE_FILE_FORMAT_PNG:
        case IMAGE_FILE_FORMAT_TGA:
        case IMAGE_FILE_FORMAT_BMP:
        case IMAGE_FILE_FORMAT_PSD:
        case IMAGE_FILE_FORMAT_PIC:
        case IMAGE_FILE_FORMAT_PNM:
        case IMAGE_FILE_FORMAT_HDR:
            if (!LoadImageStb(Stream, numRequiredChannels, bAsHDRI, &w, &h, &numChannels, &data))
                return {};
            break;
        case IMAGE_FILE_FORMAT_WEBP:
            if (!LoadImageWebp(Stream, numRequiredChannels, bAsHDRI, &w, &h, &numChannels, &data))
                return {};
            break;
        case IMAGE_FILE_FORMAT_EXR:
            if (!LoadImageEXR(Stream, numRequiredChannels, bAsHDRI, &w, &h, &numChannels, &data))
                return {};
            break;
        case IMAGE_FILE_FORMAT_UNKNOWN:
        default:
            LOG("CreateRawImage: unknown image format {}\n", Stream.GetName());
            return {};
    };

    numChannels = numRequiredChannels ? numRequiredChannels : numChannels;

    if (Format == RAW_IMAGE_FORMAT_BGR8 || Format == RAW_IMAGE_FORMAT_BGRA8 || Format == RAW_IMAGE_FORMAT_BGR32_FLOAT || Format == RAW_IMAGE_FORMAT_BGRA32_FLOAT)
    {
        if (bAsHDRI)
        {
            float* d   = (float*)data;
            float* d_e = d + w * h * numChannels;
            while (d < d_e)
            {
                Core::Swap(d[0], d[2]);
                d += numChannels;
            }
        }
        else
        {
            uint8_t* d   = (uint8_t*)data;
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
                Format = bAsHDRI ? RAW_IMAGE_FORMAT_R32_FLOAT : RAW_IMAGE_FORMAT_R8;
                break;
            case 2:
                Format = bAsHDRI ? RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT : RAW_IMAGE_FORMAT_R8_ALPHA;
                break;
            case 3:
                Format = bAsHDRI ? RAW_IMAGE_FORMAT_RGB32_FLOAT : RAW_IMAGE_FORMAT_RGB8;
                break;
            case 4:
                Format = bAsHDRI ? RAW_IMAGE_FORMAT_RGBA32_FLOAT : RAW_IMAGE_FORMAT_RGBA8;
                break;
            default:
                HK_ASSERT(0);
        }
    }

    ARawImage image;
    image.SetExternalData(w, h, Format, data);

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

ARawImage CreateRawImage(SvgDocument const& Document, uint32_t Width, uint32_t Height, Float4 const& BackgroundColor)
{
    if (!Document)
        return {};

    if (Width == 0 || Height == 0)
        return {};

    ARawImage image(Width, Height, RAW_IMAGE_FORMAT_BGRA8, BackgroundColor);

    Document.RenderToImage(image.GetData(), image.GetWidth(), image.GetHeight(), image.GetWidth() * image.GetHeight() * image.GetBytesPerPixel());

    return image;
}

ARawImage CreateRawImageFromSVG(IBinaryStreamReadInterface& Stream, Float2 const& Scale, Float4 const& BackgroundColor)
{
    if (Scale.X <= 0.0f || Scale.Y <= 0.0f)
        return {};

    SvgDocument document = CreateSVG(Stream);
    if (!document)
        return {};

    uint32_t w = document.GetWidth() * Scale.X;
    uint32_t h = document.GetHeight() * Scale.Y;

    return CreateRawImage(document, w, h, BackgroundColor);
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

            float m = Math::Max3(r, g, b);
            if (m > 1.0f)
            {
                m = 1.0f / m;
                r *= m;
                g *= m;
                b *= m;
            }
        }

        dst[0] = LinearToSRGB_UChar(r);
        dst[1] = LinearToSRGB_UChar(g);
        dst[2] = LinearToSRGB_UChar(b);
        dst[3] = FloatToU8(src[3]);
    }
}

bool WritePNG(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData)
{
    if (Width == 0 || Height == 0)
    {
        LOG("WritePNG: Invalid image size {} x {}\n", Width, Height);
        return false;
    }
    if (!pData)
    {
        LOG("WritePNG: Invalid image data\n");
        return false;
    }

    if (!stbi_write_png_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData, Width * NumChannels))
    {
        LOG("WritePNG: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }
    return true;
}

bool WriteBMP(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData)
{
    if (Width == 0 || Height == 0)
    {
        LOG("WriteBMP: Invalid image size {} x {}\n", Width, Height);
        return false;
    }
    if (!pData)
    {
        LOG("WriteBMP: Invalid image data\n");
        return false;
    }

    if (!stbi_write_bmp_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData))
    {
        LOG("WriteBMP: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }

    return true;
}

bool WriteTGA(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData)
{
    if (Width == 0 || Height == 0)
    {
        LOG("WriteTGA: Invalid image size {} x {}\n", Width, Height);
        return false;
    }
    if (!pData)
    {
        LOG("WriteTGA: Invalid image data\n");
        return false;
    }
    
    if (!stbi_write_tga_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData))
    {
        LOG("WriteTGA: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }

    return true;
}

bool WriteJPG(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData, float Quality)
{
    if (Width == 0 || Height == 0)
    {
        LOG("WriteJPG: Invalid image size {} x {}\n", Width, Height);
        return false;
    }
    if (!pData)
    {
        LOG("WriteJPG: Invalid image data\n");
        return false;
    }

    if (!stbi_write_jpg_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData, (int)(Math::Saturate(Quality) * 99 + 1)))
    {
        LOG("WriteJPG: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }

    return true;
}

bool WriteHDR(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const float* pData)
{
    if (Width == 0 || Height == 0)
    {
        LOG("WriteHDR: Invalid image size {} x {}\n", Width, Height);
        return false;
    }
    if (!pData)
    {
        LOG("WriteHDR: Invalid image data\n");
        return false;
    }
    
    if (!stbi_write_hdr_to_func(Stbi_Write, &Stream, Width, Height, NumChannels, pData))
    {
        LOG("WriteHDR: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }

    return true;
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

    if (Width == 0 || Height == 0)
    {
        LOG("WriteEXR: Invalid image size {} x {}\n", Width, Height);
        return false;
    }
    if (!pData)
    {
        LOG("WriteEXR: Invalid image data\n");
        return false;
    }

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
    {
        LOG("WriteEXR: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }

    size_t bytesToWrite = Stream.Write(memory, size);
    free(memory);

    if (bytesToWrite != size)
    {
        LOG("WriteEXR: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }

    return true;
#else
    LOG("WriteEXR: EXR is not allowed\n");
    return false;
#endif
}

bool WriteWEBP(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData, float Quality, bool bLossless)
{
#ifdef SUPPORT_WEBP
    if (Width == 0 || Height == 0)
    {
        LOG("WriteWEBP: Invalid image size {} x {}\n", Width, Height);
        return false;
    }
    if (Width > WEBP_MAX_DIMENSION || Height > WEBP_MAX_DIMENSION)
    {
        LOG("WriteWEBP: Maximum dimension supported by WebP is {}\n", WEBP_MAX_DIMENSION);
        return false;
    }
    if (NumChannels != 3 && NumChannels != 4)
    {
        LOG("WriteWEBP: Unsupported channel count {}\n", NumChannels);
        return false;
    }
    if (!pData)
    {
        LOG("WriteWEBP: Invalid image data\n");
        return false;
    }
	
    WebPPicture pic;
    WebPConfig config;
    WebPMemoryWriter memoryWriter;
	
    if (!WebPConfigPreset(&config, WEBP_PRESET_DEFAULT, Math::Saturate(Quality) * 100.0f) || !WebPPictureInit(&pic))
    {
        LOG("WriteWEBP: initialization failed\n");
        return false;
    }
    config.lossless = int(bLossless);

    // Preserve the exact RGB values under transparent area
    config.exact = 1;
	
    pic.use_argb = 1;
    pic.width = Width;
    pic.height = Height;
    pic.writer = WebPMemoryWrite;
    pic.custom_ptr = &memoryWriter;
    WebPMemoryWriterInit(&memoryWriter);
	
    int importResult = NumChannels == 3 ?
        WebPPictureImportRGB(&pic, (const uint8_t*)pData, NumChannels * Width) :
        WebPPictureImportRGBA(&pic, (const uint8_t*)pData, NumChannels * Width);
	
    if (!importResult)
    {
        LOG("WriteWEBP: failed to import image data\n");
        WebPPictureFree(&pic);
        WebPMemoryWriterClear(&memoryWriter);
        return false;
    }
	
    if (!WebPEncode(&config, &pic))
    {
        LOG("WriteWEBP: failed to encode image data, WebPEncodingError = {}", (int)pic.error_code);
        WebPPictureFree(&pic);
        WebPMemoryWriterClear(&memoryWriter);
        return false;
    }
	
    WebPPictureFree(&pic);
    
    bool writeError = Stream.Write(memoryWriter.mem, memoryWriter.size) != memoryWriter.size;
    WebPMemoryWriterClear(&memoryWriter);

    if (writeError)
    {
        LOG("WriteWEBP: failed to write {}x{}, {}\n", Width, Height, NumChannels);
        return false;
    }
	
    return true;
#else
    LOG("WriteWEBP: WEBP is not allowed\n");
    return false;
#endif
}

bool WriteImage(AStringView FileName, ImageWriteConfig const& Config)
{
    AStringView ext = PathUtils::GetExt(FileName);

    if (ext.Icompare(".hdr") || ext.Icompare(".exr"))
    {
        LOG("WriteImage: Use WriteImgeHDRI to save as .hdr or .exr\n");
        return false;
    }

    if (!(ext.Icompare(".png") || ext.Icompare(".bmp") || ext.Icompare(".tga") || ext.Icompare(".jpg") || ext.Icompare(".jpeg") || ext.Icompare(".webp")))
    {
        LOG("WriteImage: Unsupported image write format {}\n", FileName);
        return false;
    }

    AFile f = AFile::OpenWrite(FileName);
    if (!f)
        return false;

    bool result = false;

    if (ext.Icompare(".png"))
        result = WritePNG(f, Config.Width, Config.Height, Config.NumChannels, Config.pData);
    else if (ext.Icompare(".bmp"))
        result = WriteBMP(f, Config.Width, Config.Height, Config.NumChannels, Config.pData);
    else if (ext.Icompare(".tga"))
        result = WriteTGA(f, Config.Width, Config.Height, Config.NumChannels, Config.pData);
    else if (ext.Icompare(".jpg") || ext.Icompare(".jpeg"))
        result = WriteJPG(f, Config.Width, Config.Height, Config.NumChannels, Config.pData, Config.Quality);
    else if (ext.Icompare(".webp"))
        result = WriteWEBP(f, Config.Width, Config.Height, Config.NumChannels, Config.pData, Config.Quality, Config.bLossless);

    if (!result)
    {
        LOG("WriteImage: Failed to write image {}\n", FileName);
    }
    
    return result;
}

bool WriteImageHDRI(AStringView FileName, ImageWriteConfig const& Config)
{
    AStringView ext = PathUtils::GetExt(FileName);

    if (!(ext.Icompare(".hdr") || ext.Icompare(".exr")))
    {
        LOG("WriteImageHDRI: Expected .hdr or .exr file format\n");
        return false;
    }

    AFile f = AFile::OpenWrite(FileName);
    if (!f)
        return false;

    bool result = false;

    if (ext.Icompare(".hdr"))
        result = WriteHDR(f, Config.Width, Config.Height, Config.NumChannels, (const float*)Config.pData);
    else if (ext.Icompare(".exr"))
        result = WriteEXR(f, Config.Width, Config.Height, Config.NumChannels, (const float*)Config.pData, Config.bSaveExrAsHalf);

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

    ImageWriteConfig config;
    config.Width = Image.GetWidth();
    config.Height = Image.GetHeight();
    config.NumChannels = Image.NumChannels();
    config.pData = Image.GetData();
    config.Quality = 1.0f;
    config.bLossless = true;
    config.bSaveExrAsHalf = true;

    if (!bHDRI)
    {
        if (bHDRIExt)
        {
            LOG("WriteImage: attempted to save LDR image in HDRI format {}\n", FileName);
            return false;
        }
        return WriteImage(FileName, config);
    }
    else
    {
        if (!bHDRIExt)
        {
            LOG("WriteImage: attempted to save HDR image in LDR format {}\n", FileName);
            return false;
        }
        return WriteImageHDRI(FileName, config);
    }
}
