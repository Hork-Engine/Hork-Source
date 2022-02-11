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

#include <Core/Image.h>
#include <Core/Color.h>
#include <Core/Compress.h>
#include <Platform/Logger.h>

#define STBI_MALLOC(sz)                     GHeapMemory.Alloc(sz, 16)
#define STBI_FREE(p)                        GHeapMemory.Free(p)
#define STBI_REALLOC(p, newsz)              GHeapMemory.Realloc(p, newsz, 16, true)
#define STBI_REALLOC_SIZED(p, oldsz, newsz) STBI_REALLOC(p, newsz)
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_STDIO
#define STBI_NO_GIF // Maybe in future gif will be used, but not now
#include "stb/stb_image.h"

#define STBIW_MALLOC(sz)        GHeapMemory.Alloc(sz, 16)
#define STBIW_FREE(p)           GHeapMemory.Free(p)
#define STBIW_REALLOC(p, newsz) GHeapMemory.Realloc(p, newsz, 16, true)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBI_WRITE_NO_STDIO
#define STBIW_ZLIB_COMPRESS      stbi_zlib_compress_override                   // Override compression method
#define STBIW_CRC32(buffer, len) Core::Crc32(Core::CRC32_INITIAL, buffer, len) // Override crc32 method
#define PNG_COMPRESSION_LEVEL    8

// Miniz mz_compress2 provides better compression than default stb compression method
HK_FORCEINLINE unsigned char* stbi_zlib_compress_override(unsigned char* data, int data_len, int* out_len, int quality)
{
    //GLogger.Printf( "stbi_zlib_compress_override %d\n", quality );
    //GLogger.Printf( "stbi_zlib_compress_override data_len %d quality %d\n", data_len, quality );
    size_t buflen = Core::ZMaxCompressedSize(data_len);
    //GLogger.Printf( "stbi_zlib_compress_override buflen %d\n", buflen );
    unsigned char* buf = (unsigned char*)STBIW_MALLOC(buflen);
    if (buf == NULL)
    {
        GLogger.Printf("stbi_zlib_compress_override Malloc failed\n");
        return NULL;
    }
    if (!Core::ZCompress(buf, &buflen, data, data_len, PNG_COMPRESSION_LEVEL))
    {
        GLogger.Printf("stbi_zlib_compress_override ZCompress failed\n");
        STBIW_FREE(buf);
        return NULL;
    }
    *out_len = buflen;
    if (buflen == 0)
    { // compiler bug workaround (MSVC, Release)
        GLogger.Printf("WritePNG: invalid compression\n");
    }
    return buf;
}

#include "stb/stb_image_write.h"

#define STBIR_MALLOC(sz, context) GHunkMemory.Alloc(sz)
#define STBIR_FREE(p, context)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#define STBIR_MAX_CHANNELS 4
#include "stb/stb_image_resize.h"

#define SUPPORT_EXR

AImage::AImage()
{
    pRawData     = nullptr;
    Width        = 0;
    Height       = 0;
    NumMipLevels = 0;
    PixelFormat  = IMAGE_PF_AUTO_GAMMA2;
}

AImage::~AImage()
{
    Free();
}

static int Stbi_Read(void* user, char* data, int size)
{
    IBinaryStream* stream = (IBinaryStream*)user;
    stream->ReadBuffer(data, size);
    return stream->GetReadBytesCount();
}

static void Stbi_Skip(void* user, int n)
{
    IBinaryStream* stream = (IBinaryStream*)user;
    stream->SeekCur(n);
}

static int Stbi_Eof(void* user)
{
    IBinaryStream* stream = (IBinaryStream*)user;
    return stream->Eof();
}

static void Stbi_Write(void* context, void* data, int size)
{
    IBinaryStream* stream = (IBinaryStream*)context;
    stream->WriteBuffer(data, size);
}

static HK_FORCEINLINE bool IsAuto(EImagePixelFormat _PixelFormat)
{
    switch (_PixelFormat)
    {
        case IMAGE_PF_AUTO:
        case IMAGE_PF_AUTO_GAMMA2:
        case IMAGE_PF_AUTO_16F:
        case IMAGE_PF_AUTO_32F:
            return true;
        default:
            break;
    }
    return false;
}

static HK_FORCEINLINE int GetNumChannels(EImagePixelFormat _PixelFormat)
{
    switch (_PixelFormat)
    {
        case IMAGE_PF_AUTO:
        case IMAGE_PF_AUTO_GAMMA2:
        case IMAGE_PF_AUTO_16F:
        case IMAGE_PF_AUTO_32F:
            return 0;
        case IMAGE_PF_R:
        case IMAGE_PF_R16F:
        case IMAGE_PF_R32F:
            return 1;
        case IMAGE_PF_RG:
        case IMAGE_PF_RG16F:
        case IMAGE_PF_RG32F:
            return 2;
        //case IMAGE_PF_RGB:
        //case IMAGE_PF_RGB_GAMMA2:
        //case IMAGE_PF_RGB16F:
        case IMAGE_PF_RGB32F:
            return 3;
        case IMAGE_PF_RGBA:
        case IMAGE_PF_RGBA_GAMMA2:
        case IMAGE_PF_RGBA16F:
        case IMAGE_PF_RGBA32F:
            return 4;
        //case IMAGE_PF_BGR:
        //case IMAGE_PF_BGR_GAMMA2:
        //case IMAGE_PF_BGR16F:
        case IMAGE_PF_BGR32F:
            return 3;
        case IMAGE_PF_BGRA:
        case IMAGE_PF_BGRA_GAMMA2:
        case IMAGE_PF_BGRA16F:
        case IMAGE_PF_BGRA32F:
            return 4;
    }
    HK_ASSERT(0);
    return 0;
}

static HK_FORCEINLINE bool IsHalfFloat(EImagePixelFormat _PixelFormat)
{
    switch (_PixelFormat)
    {
        case IMAGE_PF_AUTO_16F:
        case IMAGE_PF_R16F:
        case IMAGE_PF_RG16F:
        //case IMAGE_PF_RGB16F:
        case IMAGE_PF_RGBA16F:
        //case IMAGE_PF_BGR16F:
        case IMAGE_PF_BGRA16F:
            return true;
        default:
            break;
    }
    return false;
}

static HK_FORCEINLINE bool IsFloat(EImagePixelFormat _PixelFormat)
{
    switch (_PixelFormat)
    {
        case IMAGE_PF_AUTO_32F:
        case IMAGE_PF_R32F:
        case IMAGE_PF_RG32F:
        case IMAGE_PF_RGB32F:
        case IMAGE_PF_RGBA32F:
        case IMAGE_PF_BGR32F:
        case IMAGE_PF_BGRA32F:
            return true;
        default:
            break;
    }
    return false;
}

static HK_FORCEINLINE bool IsHDRI(EImagePixelFormat _PixelFormat)
{
    return IsHalfFloat(_PixelFormat) || IsFloat(_PixelFormat);
}

static HK_FORCEINLINE int IsGamma2(EImagePixelFormat _PixelFormat)
{
    switch (_PixelFormat)
    {
        case IMAGE_PF_AUTO_GAMMA2:
        //case IMAGE_PF_RGB_GAMMA2:
        case IMAGE_PF_RGBA_GAMMA2:
        //case IMAGE_PF_BGR_GAMMA2:
        case IMAGE_PF_BGRA_GAMMA2:
            return true;
        default:
            break;
    }
    return false;
}

static HK_FORCEINLINE bool IsBGR(EImagePixelFormat _PixelFormat)
{
    switch (_PixelFormat)
    {
        case IMAGE_PF_AUTO:
        case IMAGE_PF_AUTO_GAMMA2:
        case IMAGE_PF_AUTO_16F:
        case IMAGE_PF_AUTO_32F:
            // BGR is by default
            return true;
        case IMAGE_PF_R:
        case IMAGE_PF_R16F:
        case IMAGE_PF_R32F:
        case IMAGE_PF_RG:
        case IMAGE_PF_RG16F:
        case IMAGE_PF_RG32F:
        //case IMAGE_PF_RGB:
        //case IMAGE_PF_RGB_GAMMA2:
        //case IMAGE_PF_RGB16F:
        case IMAGE_PF_RGB32F:
        case IMAGE_PF_RGBA:
        case IMAGE_PF_RGBA_GAMMA2:
        case IMAGE_PF_RGBA16F:
        case IMAGE_PF_RGBA32F:
            return false;
        //case IMAGE_PF_BGR:
        //case IMAGE_PF_BGR_GAMMA2:
        //case IMAGE_PF_BGR16F:
        case IMAGE_PF_BGR32F:
        case IMAGE_PF_BGRA:
        case IMAGE_PF_BGRA_GAMMA2:
        case IMAGE_PF_BGRA16F:
        case IMAGE_PF_BGRA32F:
            return true;
    }
    HK_ASSERT(0);
    return false;
}

bool AImage::Load(AStringView _Path, SImageMipmapConfig const* _MipmapGen, EImagePixelFormat _PixelFormat)
{
    AFileStream stream;

    Free();

    if (!stream.OpenRead(_Path))
    {
        return false;
    }

    return Load(stream, _MipmapGen, _PixelFormat);
}

#ifdef SUPPORT_EXR

#    include <tinyexr/tinyexr.h>

HK_FORCEINLINE static byte FloatToByte(float _Color)
{
    return Math::Floor(Math::Saturate(_Color) * 255.0f + 0.5f);
}

static void* LoadEXR(IBinaryStream& _Stream, int* w, int* h, int* channels, int desiredchannels, bool ldr)
{
    HK_ASSERT(desiredchannels >= 0 && desiredchannels <= 4);

    long streamOffset = _Stream.Tell();
    _Stream.SeekEnd(0);
    long  sizeInBytes = _Stream.Tell() - streamOffset;
    void* memory      = GHeapMemory.Alloc(sizeInBytes);
    _Stream.SeekSet(streamOffset);
    _Stream.ReadBuffer(memory, sizeInBytes);

    float*      data;
    const char* err;

    *channels = 0;

    int r = LoadEXRFromMemory(&data, w, h, (unsigned char*)memory, sizeInBytes, &err);

    GHeapMemory.Free(memory);

    if (r != TINYEXR_SUCCESS)
    {
        _Stream.SeekSet(streamOffset);
        return nullptr;
    }

    int numChannels = 4;

    *channels = numChannels;

    void* result = nullptr;

    int pixcount = (*w) * (*h) * numChannels;

    if (ldr)
    {
        if (desiredchannels != 0 && desiredchannels != numChannels)
        {
            result        = STBI_MALLOC((*w) * (*h) * desiredchannels);
            byte* pResult = (byte*)result;
            if (desiredchannels == 1)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = LinearToSRGB_UChar(data[i]);
                }
            }
            else if (desiredchannels == 2)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = LinearToSRGB_UChar(data[i]);
                    *pResult++ = numChannels > 1 ? LinearToSRGB_UChar(data[i + 1]) : 0;
                }
            }
            else if (desiredchannels == 3)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = LinearToSRGB_UChar(data[i]);
                    *pResult++ = numChannels > 1 ? LinearToSRGB_UChar(data[i + 1]) : 0;
                    *pResult++ = numChannels > 2 ? LinearToSRGB_UChar(data[i + 2]) : 0;
                }
            }
            else
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = LinearToSRGB_UChar(data[i]);
                    *pResult++ = numChannels > 1 ? LinearToSRGB_UChar(data[i + 1]) : 0;
                    *pResult++ = numChannels > 2 ? LinearToSRGB_UChar(data[i + 2]) : 0;
                    *pResult++ = numChannels > 3 ? FloatToByte(data[i + 3]) * 255 : 255;
                }
            }
        }
        else
        {
            result = STBI_MALLOC((*w) * (*h) * numChannels);

            int numColorChannels = Math::Min(numChannels, 3);

            byte* pResult = (byte*)result;
            for (int i = 0; i < pixcount; i += numChannels, pResult += numChannels)
            {
                for (int c = 0; c < numColorChannels; c++)
                {
                    pResult[c] = LinearToSRGB_UChar(data[i + c]);
                }
            }

            if (numChannels == 4)
            {
                pResult = (byte*)result;
                for (int i = 0; i < pixcount; i += numChannels, pResult += numChannels)
                {
                    pResult[3] = FloatToByte(data[i + 3]);
                }
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
                    *pResult++ = numChannels > 1 ? data[i + 1] : 0;
                }
            }
            else if (desiredchannels == 3)
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = data[i];
                    *pResult++ = numChannels > 1 ? data[i + 1] : 0;
                    *pResult++ = numChannels > 2 ? data[i + 2] : 0;
                }
            }
            else
            {
                for (int i = 0; i < pixcount; i += numChannels)
                {
                    *pResult++ = data[i];
                    *pResult++ = numChannels > 1 ? data[i + 1] : 0;
                    *pResult++ = numChannels > 2 ? data[i + 2] : 0;
                    *pResult++ = numChannels > 3 ? data[i + 3] : 1;
                }
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

bool AImage::Load(IBinaryStream& _Stream, SImageMipmapConfig const* _MipmapGen, EImagePixelFormat _PixelFormat)
{
    const stbi_io_callbacks callbacks = {Stbi_Read, Stbi_Skip, Stbi_Eof};
    int                     w, h, numChannels, numRequiredChannels;
    bool                    bHDRI = IsHDRI(_PixelFormat);
    void*                   source;

    numRequiredChannels = GetNumChannels(_PixelFormat);

    long streamOffset = _Stream.Tell();

    if (bHDRI)
    {
        source = stbi_loadf_from_callbacks(&callbacks, &_Stream, &w, &h, &numChannels, numRequiredChannels);
    }
    else
    {
        source = stbi_load_from_callbacks(&callbacks, &_Stream, &w, &h, &numChannels, numRequiredChannels);
    }

    if (!source)
    {
        _Stream.SeekSet(streamOffset);

#ifdef SUPPORT_EXR
        source = LoadEXR(_Stream, &w, &h, &numChannels, numRequiredChannels, !bHDRI);
#endif
        if (!source)
        {
            _Stream.SeekSet(streamOffset);

            GLogger.Printf("AImage::Load: couldn't load %s\n", _Stream.GetFileName());
            return false;
        }
    }

    numChannels = numRequiredChannels ? numRequiredChannels : numChannels;

    bool bAddAlphaChannel = false;

    switch (_PixelFormat)
    {
        case IMAGE_PF_AUTO:
            switch (numChannels)
            {
                case 1:
                    _PixelFormat = IMAGE_PF_R;
                    break;
                case 2:
                    _PixelFormat = IMAGE_PF_RG;
                    break;
                case 3:
                    bAddAlphaChannel = true;
                    _PixelFormat     = IMAGE_PF_BGRA;
                    break;
                case 4:
                    _PixelFormat = IMAGE_PF_BGRA;
                    break;
                default:
                    HK_ASSERT(0);
                    _PixelFormat = IMAGE_PF_BGRA;
                    break;
            }
            break;
        case IMAGE_PF_AUTO_GAMMA2:
            switch (numChannels)
            {
                case 1:
                    _PixelFormat = IMAGE_PF_R; // FIXME: support R_GAMMA?
                    break;
                case 2:
                    _PixelFormat = IMAGE_PF_RG; // FIXME: support RG_GAMMA?
                    break;
                case 3:
                    bAddAlphaChannel = true;
                    _PixelFormat     = IMAGE_PF_BGRA_GAMMA2;
                    break;
                case 4:
                    _PixelFormat = IMAGE_PF_BGRA_GAMMA2;
                    break;
                default:
                    HK_ASSERT(0);
                    _PixelFormat = IMAGE_PF_BGRA_GAMMA2;
                    break;
            }
            break;
        case IMAGE_PF_AUTO_16F:
            switch (numChannels)
            {
                case 1:
                    _PixelFormat = IMAGE_PF_R16F;
                    break;
                case 2:
                    _PixelFormat = IMAGE_PF_RG16F;
                    break;
                case 3:
                    bAddAlphaChannel = true;
                    _PixelFormat     = IMAGE_PF_BGRA16F;
                    break;
                case 4:
                    _PixelFormat = IMAGE_PF_BGRA16F;
                    break;
                default:
                    HK_ASSERT(0);
                    _PixelFormat = IMAGE_PF_BGRA16F;
                    break;
            }
            break;
        case IMAGE_PF_AUTO_32F:
            switch (numChannels)
            {
                case 1:
                    _PixelFormat = IMAGE_PF_R32F;
                    break;
                case 2:
                    _PixelFormat = IMAGE_PF_RG32F;
                    break;
                case 3:
                    _PixelFormat = IMAGE_PF_BGR32F;
                    break;
                case 4:
                    _PixelFormat = IMAGE_PF_BGRA32F;
                    break;
                default:
                    HK_ASSERT(0);
                    _PixelFormat = IMAGE_PF_BGRA32F;
                    break;
            }
            break;
        default:
            break;
    }

    if (IsBGR(_PixelFormat))
    {
        // swap r & b channels to store image as BGR
        int count = w * h * numChannels;
        if (bHDRI)
        {
            float* pSource = (float*)source;
            for (int i = 0; i < count; i += numChannels)
            {
                std::swap(pSource[i], pSource[i + 2]);
            }
        }
        else
        {
            byte* pSource = (byte*)source;
            for (int i = 0; i < count; i += numChannels)
            {
                std::swap(pSource[i], pSource[i + 2]);
            }
        }
    }

    if (bAddAlphaChannel)
    {
        if (_PixelFormat == IMAGE_PF_BGRA || _PixelFormat == IMAGE_PF_BGRA_GAMMA2)
        {
            int numPixels = w * h;

            void* newSource = stbi__malloc(numPixels * 4);

            byte* pSource    = (byte*)source;
            byte* pNewSource = (byte*)newSource;

            for (int i = 0; i < numPixels; ++i)
            {
                pNewSource[0] = pSource[0];
                pNewSource[1] = pSource[1];
                pNewSource[2] = pSource[2];
                pNewSource[3] = 255;

                pNewSource += 4;
                pSource += 3;
            }

            stbi_image_free(source);
            source = newSource;
        }
        else if (_PixelFormat == IMAGE_PF_BGRA16F)
        {
            int numPixels = w * h;

            void* newSource = stbi__malloc(numPixels * 4 * sizeof(float));

            float* pSource    = (float*)source;
            float* pNewSource = (float*)newSource;

            for (int i = 0; i < numPixels; ++i)
            {
                pNewSource[0] = pSource[0];
                pNewSource[1] = pSource[1];
                pNewSource[2] = pSource[2];
                pNewSource[3] = 1.0f;

                pNewSource += 4;
                pSource += 3;
            }

            stbi_image_free(source);
            source = newSource;
        }
        else
        {
            HK_ASSERT(0);
        }
    }

    FromRawData(source, w, h, _MipmapGen, _PixelFormat, true);

    return true;
}

void AImage::FromRawData(const void* _Source, int _Width, int _Height, SImageMipmapConfig const* _MipmapGen, EImagePixelFormat _PixelFormat)
{
    FromRawData(_Source, _Width, _Height, _MipmapGen, _PixelFormat, false);
}

void AImage::FromRawData(const void* _Source, int _Width, int _Height, SImageMipmapConfig const* _MipmapGen, EImagePixelFormat _PixelFormat, bool bReuseSourceBuffer)
{
    bool bHDRI        = IsHDRI(_PixelFormat);
    bool bLinearSpace = bHDRI || !IsGamma2(_PixelFormat);
    bool bHalf        = IsHalfFloat(_PixelFormat);
    int  numChannels  = GetNumChannels(_PixelFormat);

    Free();

    Width        = _Width;
    Height       = _Height;
    NumMipLevels = 1;
    PixelFormat  = _PixelFormat;

    if (bReuseSourceBuffer)
    {
        pRawData = const_cast<void*>(_Source);
    }
    else
    {
        size_t sizeInBytes = Width * Height * numChannels * (bHDRI ? 4 : 1);
        pRawData           = GHeapMemory.Alloc(sizeInBytes);
        Platform::Memcpy(pRawData, _Source, sizeInBytes);
    }

    if (_MipmapGen && !(Width == 1 && Height == 1))
    {
        SSoftwareMipmapGenerator mipmapGen;

        mipmapGen.SourceImage         = pRawData;
        mipmapGen.Width               = Width;
        mipmapGen.Height              = Height;
        mipmapGen.NumChannels         = numChannels;
        mipmapGen.AlphaChannel        = numChannels == 4 ? 3 : -1;
        mipmapGen.bLinearSpace        = bLinearSpace;
        mipmapGen.EdgeMode            = _MipmapGen->EdgeMode;
        mipmapGen.Filter              = _MipmapGen->Filter;
        mipmapGen.bPremultipliedAlpha = _MipmapGen->bPremultipliedAlpha;
        mipmapGen.bHDRI               = bHDRI;

        int requiredMemorySize;
        ComputeRequiredMemorySize(mipmapGen, requiredMemorySize, NumMipLevels);

        void* tmp = GHeapMemory.Alloc(requiredMemorySize);

        GenerateMipmaps(mipmapGen, tmp);

        GHeapMemory.Free(pRawData);
        pRawData = tmp;
    }

    if (bHalf)
    {
        int imageSize = 0;
        for (int i = 0; i < NumMipLevels; i++)
        {
            int w = Math::Max(1, Width >> i);
            int h = Math::Max(1, Height >> i);
            imageSize += w * h;
        }
        imageSize *= numChannels;

        uint16_t* tmp = (uint16_t*)GHeapMemory.Alloc(imageSize * sizeof(uint16_t));
        Math::FloatToHalf((float*)pRawData, tmp, imageSize);
        GHeapMemory.Free(pRawData);
        pRawData = tmp;
    }
}

void AImage::Free()
{
    GHeapMemory.Free(pRawData);
    pRawData     = nullptr;
    Width        = 0;
    Height       = 0;
    NumMipLevels = 0;
    PixelFormat  = IMAGE_PF_AUTO_GAMMA2;
}

static void DownscaleSimpleAverage(int _CurWidth, int _CurHeight, int _NewWidth, int _NewHeight, int _NumChannels, int _AlphaChannel, bool _LinearSpace, const byte* _SrcData, byte* _DstData)
{
    int Bpp = _NumChannels;

    if (_CurWidth == _NewWidth && _CurHeight == _NewHeight)
    {
        Platform::Memcpy(_DstData, _SrcData, _NewWidth * _NewHeight * Bpp);
        return;
    }

    float a, b, c, d, avg;

    int i, j, x, y, ch, idx;

    for (i = 0; i < _NewWidth; i++)
    {
        for (j = 0; j < _NewHeight; j++)
        {

            idx = j * _NewWidth + i;

            for (ch = 0; ch < _NumChannels; ch++)
            {
                if (_LinearSpace || ch == _AlphaChannel)
                {
                    if (_NewWidth == _CurWidth)
                    {
                        x = i;
                        y = j << 1;

                        a = _SrcData[(y * _CurWidth + x) * Bpp + ch];
                        c = _SrcData[((y + 1) * _CurWidth + x) * Bpp + ch];

                        avg = (a + c) * 0.5f;
                    }
                    else if (_NewHeight == _CurHeight)
                    {
                        x = i << 1;
                        y = j;

                        a = _SrcData[(y * _CurWidth + x) * Bpp + ch];
                        b = _SrcData[(y * _CurWidth + x + 1) * Bpp + ch];

                        avg = (a + b) * 0.5f;
                    }
                    else
                    {
                        x = i << 1;
                        y = j << 1;

                        a = _SrcData[(y * _CurWidth + x) * Bpp + ch];
                        b = _SrcData[(y * _CurWidth + x + 1) * Bpp + ch];
                        c = _SrcData[((y + 1) * _CurWidth + x) * Bpp + ch];
                        d = _SrcData[((y + 1) * _CurWidth + x + 1) * Bpp + ch];

                        avg = (a + b + c + d) * 0.25f;
                    }

                    _DstData[idx * Bpp + ch] = Math::Floor(Math::Clamp(avg, 0.0f, 255.0f) + 0.5f);
                }
                else
                {
                    if (_NewWidth == _CurWidth)
                    {
                        x = i;
                        y = j << 1;

                        a = LinearFromSRGB_UChar(_SrcData[(y * _CurWidth + x) * Bpp + ch]);
                        c = LinearFromSRGB_UChar(_SrcData[((y + 1) * _CurWidth + x) * Bpp + ch]);

                        avg = (a + c) * 0.5f;
                    }
                    else if (_NewHeight == _CurHeight)
                    {
                        x = i << 1;
                        y = j;

                        a = LinearFromSRGB_UChar(_SrcData[(y * _CurWidth + x) * Bpp + ch]);
                        b = LinearFromSRGB_UChar(_SrcData[(y * _CurWidth + x + 1) * Bpp + ch]);

                        avg = (a + b) * 0.5f;
                    }
                    else
                    {
                        x = i << 1;
                        y = j << 1;

                        a = LinearFromSRGB_UChar(_SrcData[(y * _CurWidth + x) * Bpp + ch]);
                        b = LinearFromSRGB_UChar(_SrcData[(y * _CurWidth + x + 1) * Bpp + ch]);
                        c = LinearFromSRGB_UChar(_SrcData[((y + 1) * _CurWidth + x) * Bpp + ch]);
                        d = LinearFromSRGB_UChar(_SrcData[((y + 1) * _CurWidth + x + 1) * Bpp + ch]);

                        avg = (a + b + c + d) * 0.25f;
                    }

                    _DstData[idx * Bpp + ch] = LinearToSRGB_UChar(avg);
                }
            }
        }
    }
}

static void DownscaleSimpleAverageHDRI(int _CurWidth, int _CurHeight, int _NewWidth, int _NewHeight, int _NumChannels, const float* _SrcData, float* _DstData)
{
    int Bpp = _NumChannels;

    if (_CurWidth == _NewWidth && _CurHeight == _NewHeight)
    {
        Platform::Memcpy(_DstData, _SrcData, _NewWidth * _NewHeight * Bpp * sizeof(float));
        return;
    }

    float a, b, c, d, avg;

    int i, j, x, y, ch, idx;

    for (i = 0; i < _NewWidth; i++)
    {
        for (j = 0; j < _NewHeight; j++)
        {

            idx = j * _NewWidth + i;

            for (ch = 0; ch < _NumChannels; ch++)
            {

                if (_NewWidth == _CurWidth)
                {
                    x = i;
                    y = j << 1;

                    a = _SrcData[(y * _CurWidth + x) * Bpp + ch];
                    c = _SrcData[((y + 1) * _CurWidth + x) * Bpp + ch];

                    avg = (a + c) * 0.5f;
                }
                else if (_NewHeight == _CurHeight)
                {
                    x = i << 1;
                    y = j;

                    a = _SrcData[(y * _CurWidth + x) * Bpp + ch];
                    b = _SrcData[(y * _CurWidth + x + 1) * Bpp + ch];

                    avg = (a + b) * 0.5f;
                }
                else
                {
                    x = i << 1;
                    y = j << 1;

                    a = _SrcData[(y * _CurWidth + x) * Bpp + ch];
                    b = _SrcData[(y * _CurWidth + x + 1) * Bpp + ch];
                    c = _SrcData[((y + 1) * _CurWidth + x) * Bpp + ch];
                    d = _SrcData[((y + 1) * _CurWidth + x + 1) * Bpp + ch];

                    avg = (a + b + c + d) * 0.25f;
                }

                _DstData[idx * Bpp + ch] = avg;
            }
        }
    }
}

static void GenerateMipmaps(const byte* ImageData, int ImageWidth, int ImageHeight, int NumChannels, int AlphaChannel, EMipmapEdgeMode EdgeMode, EMipmapFilter Filter, bool bLinearSpace, bool bPremultipliedAlpha, byte* Dest)
{
    Platform::Memcpy(Dest, ImageData, ImageWidth * ImageHeight * NumChannels);

    int MemoryOffset = ImageWidth * ImageHeight * NumChannels;

    int CurWidth  = ImageWidth;
    int CurHeight = ImageHeight;

    if (ImageWidth == 1 && ImageHeight == 1)
    {
        return;
    }

    for (int i = 1;; i++)
    {
        int MipWidth  = Math::Max(1, ImageWidth >> i);
        int MipHeight = Math::Max(1, ImageHeight >> i);

        byte* MipData = Dest + MemoryOffset;
        MemoryOffset += MipWidth * MipHeight * NumChannels;

#if 1
        // NOTE: We assume that this function is called from main thread,
        // so we can use advantage of hunk memory allocator
        int hunkMark = GHunkMemory.SetHunkMark();

        stbir_resize(ImageData, CurWidth, CurHeight, NumChannels * CurWidth,
                     MipData, MipWidth, MipHeight, NumChannels * MipWidth,
                     STBIR_TYPE_UINT8,
                     NumChannels,
                     AlphaChannel,
                     bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                     (stbir_edge)EdgeMode, (stbir_edge)EdgeMode,
                     (stbir_filter)Filter, (stbir_filter)Filter,
                     bLinearSpace ? STBIR_COLORSPACE_LINEAR : STBIR_COLORSPACE_SRGB,
                     NULL);

        GHunkMemory.ClearToMark(hunkMark);
#else
        DownscaleSimpleAverage(CurWidth, CurHeight, MipWidth, MipHeight, NumChannels, AlphaChannel, bLinearSpace, ImageData, MipData);
#endif

        ImageData = MipData;

        CurWidth  = MipWidth;
        CurHeight = MipHeight;

        if (MipWidth == 1 && MipHeight == 1)
        {
            break;
        }
    }
}

static void GenerateMipmapsHDRI(const float* ImageData, int ImageWidth, int ImageHeight, int NumChannels, EMipmapEdgeMode EdgeMode, EMipmapFilter Filter, bool bPremultipliedAlpha, float* _Dest)
{
    Platform::Memcpy(_Dest, ImageData, ImageWidth * ImageHeight * NumChannels * sizeof(float));

    int MemoryOffset = ImageWidth * ImageHeight * NumChannels;

    int CurWidth  = ImageWidth;
    int CurHeight = ImageHeight;

    if (ImageWidth == 1 && ImageHeight == 1)
    {
        return;
    }

    for (int i = 1;; i++)
    {
        int MipWidth  = Math::Max(1, ImageWidth >> i);
        int MipHeight = Math::Max(1, ImageHeight >> i);

        float* MipData = _Dest + MemoryOffset;
        MemoryOffset += MipWidth * MipHeight * NumChannels;

#if 1
        // NOTE: We assume that this function is called from main thread,
        // so we can use advantage of hunk memory allocator
        int hunkMark = GHunkMemory.SetHunkMark();

        stbir_resize(ImageData, CurWidth, CurHeight, NumChannels * CurWidth * sizeof(float),
                     MipData, MipWidth, MipHeight, NumChannels * MipWidth * sizeof(float),
                     STBIR_TYPE_FLOAT,
                     NumChannels,
                     -1,
                     bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                     (stbir_edge)EdgeMode, (stbir_edge)EdgeMode,
                     (stbir_filter)Filter, (stbir_filter)Filter,
                     STBIR_COLORSPACE_LINEAR,
                     NULL);

        GHunkMemory.ClearToMark(hunkMark);
#else
        DownscaleSimpleAverageHDRI(CurWidth, CurHeight, MipWidth, MipHeight, NumChannels, ImageData, MipData);
#endif

        ImageData = MipData;

        CurWidth  = MipWidth;
        CurHeight = MipHeight;

        if (MipWidth == 1 && MipHeight == 1)
        {
            break;
        }
    }
}

void ComputeRequiredMemorySize(SSoftwareMipmapGenerator const& _Config, int& _RequiredMemory, int& _NumMipLevels)
{
    _RequiredMemory = 0;
    _NumMipLevels   = 0;

    for (int i = 0;; i++)
    {
        int MipWidth  = Math::Max(1, _Config.Width >> i);
        int MipHeight = Math::Max(1, _Config.Height >> i);
        _RequiredMemory += MipWidth * MipHeight;
        _NumMipLevels++;
        if (MipWidth == 1 && MipHeight == 1)
        {
            break;
        }
    }

    _RequiredMemory *= _Config.NumChannels;

    if (_Config.bHDRI)
    {
        _RequiredMemory *= sizeof(float);
    }
}

void GenerateMipmaps(SSoftwareMipmapGenerator const& _Config, void* _Data)
{
    if (_Config.bHDRI)
    {
        ::GenerateMipmapsHDRI((const float*)_Config.SourceImage, _Config.Width, _Config.Height, _Config.NumChannels, _Config.EdgeMode, _Config.Filter, _Config.bPremultipliedAlpha, (float*)_Data);
    }
    else
    {
        ::GenerateMipmaps((const byte*)_Config.SourceImage, _Config.Width, _Config.Height, _Config.NumChannels, _Config.AlphaChannel, _Config.EdgeMode, _Config.Filter, _Config.bLinearSpace, _Config.bPremultipliedAlpha, (byte*)_Data);
    }
}

static void MemSwap(byte* Block, const size_t BlockSz, byte* _Ptr1, byte* _Ptr2, const size_t _Size)
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

void FlipImageX(void* _ImageData, int _Width, int _Height, int _BytesPerPixel, int _BytesPerLine)
{
    int   lineWidth = _Width * _BytesPerPixel;
    int   halfWidth = _Width >> 1;
    byte* temp      = (byte*)StackAlloc(_BytesPerPixel);
    byte* image     = (byte*)_ImageData;
    for (int y = 0; y < _Height; y++)
    {
        byte* s = image;
        byte* e = image + lineWidth;
        for (int x = 0; x < halfWidth; x++)
        {
            e -= _BytesPerPixel;
            Platform::Memcpy(temp, s, _BytesPerPixel);
            Platform::Memcpy(s, e, _BytesPerPixel);
            Platform::Memcpy(e, temp, _BytesPerPixel);
            s += _BytesPerPixel;
        }
        image += _BytesPerLine;
    }
}

void FlipImageY(void* _ImageData, int _Width, int _Height, int _BytesPerPixel, int _BytesPerLine)
{
    const size_t blockSizeInBytes = 4096;
    byte         block[blockSizeInBytes];
    int          lineWidth  = _Width * _BytesPerPixel;
    int          halfHeight = _Height >> 1;
    byte*        image      = (byte*)_ImageData;
    byte*        e          = image + _Height * _BytesPerLine;
    for (int y = 0; y < halfHeight; y++)
    {
        e -= _BytesPerLine;
        MemSwap(block, blockSizeInBytes, image, e, lineWidth);
        image += _BytesPerLine;
    }
}

void LinearToPremultipliedAlphaSRGB(const float* SourceImage,
                                    int          Width,
                                    int          Height,
                                    bool         bOverbright,
                                    float        fOverbright,
                                    bool         bReplaceAlpha,
                                    float        fReplaceAlpha,
                                    byte*        sRGB)
{

    const float* src = SourceImage;
    byte*        dst = sRGB;
    float        r, g, b;

    int pixCount = Width * Height;

    byte replaceAlpha = FloatToByte(fReplaceAlpha);

    for (int i = 0; i < pixCount; i++, src += 4, dst += 4)
    {
        r = src[0] * src[3];
        g = src[1] * src[3];
        b = src[2] * src[3];

        if (bOverbright)
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
        dst[3] = bReplaceAlpha ? replaceAlpha : FloatToByte(src[3]);
    }
}

void ResizeImage(SImageResizeDesc const& InDesc, void* pScaledImage)
{
    HK_ASSERT(IMAGE_DATA_TYPE_UINT8 == STBIR_TYPE_UINT8);
    HK_ASSERT(IMAGE_DATA_TYPE_UINT16 == STBIR_TYPE_UINT16);
    HK_ASSERT(IMAGE_DATA_TYPE_UINT32 == STBIR_TYPE_UINT32);
    HK_ASSERT(IMAGE_DATA_TYPE_FLOAT == STBIR_TYPE_FLOAT);

    // NOTE: We assume that this function is called from main thread,
    // so we can use advantage of hunk memory allocator
    int hunkMark = GHunkMemory.SetHunkMark();

    int result =
        stbir_resize(InDesc.pImage, InDesc.Width, InDesc.Height, InDesc.NumChannels * InDesc.Width,
                     pScaledImage, InDesc.ScaledWidth, InDesc.ScaledHeight, InDesc.NumChannels * InDesc.ScaledWidth,
                     (stbir_datatype)InDesc.DataType,
                     InDesc.NumChannels,
                     InDesc.AlphaChannel,
                     InDesc.bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                     (stbir_edge)InDesc.HorizontalEdgeMode, (stbir_edge)InDesc.VerticalEdgeMode,
                     (stbir_filter)InDesc.HorizontalFilter, (stbir_filter)InDesc.VerticalFilter,
                     InDesc.bLinearSpace ? STBIR_COLORSPACE_LINEAR : STBIR_COLORSPACE_SRGB,
                     NULL);

    HK_ASSERT(result == 1);

    GHunkMemory.ClearToMark(hunkMark);
}

bool WritePNG(IBinaryStream& _Stream, int _Width, int _Height, int _NumChannels, const void* _ImageData, int _BytesPerLine)
{
    return !!stbi_write_png_to_func(Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData, _BytesPerLine);
}

bool WriteBMP(IBinaryStream& _Stream, int _Width, int _Height, int _NumChannels, const void* _ImageData)
{
    return !!stbi_write_bmp_to_func(Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData);
}

bool WriteTGA(IBinaryStream& _Stream, int _Width, int _Height, int _NumChannels, const void* _ImageData)
{
    return !!stbi_write_tga_to_func(Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData);
}

bool WriteJPG(IBinaryStream& _Stream, int _Width, int _Height, int _NumChannels, const void* _ImageData, int _Quality)
{
    return !!stbi_write_jpg_to_func(Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData, _Quality);
}

bool WriteHDR(IBinaryStream& _Stream, int _Width, int _Height, int _NumChannels, const float* _ImageData)
{
    return !!stbi_write_hdr_to_func(Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData);
}
