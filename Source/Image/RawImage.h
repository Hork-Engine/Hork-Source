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

#include <Core/IO.h>
#include <Geometry/VectorMath.h>

enum RAW_IMAGE_FORMAT
{
    RAW_IMAGE_FORMAT_UNDEFINED,

    RAW_IMAGE_FORMAT_R8,
    RAW_IMAGE_FORMAT_R8_ALPHA,
    RAW_IMAGE_FORMAT_RGB8,
    RAW_IMAGE_FORMAT_BGR8,
    RAW_IMAGE_FORMAT_RGBA8,
    RAW_IMAGE_FORMAT_BGRA8,

    RAW_IMAGE_FORMAT_R32_FLOAT,
    RAW_IMAGE_FORMAT_R32_ALPHA_FLOAT,
    RAW_IMAGE_FORMAT_RGB32_FLOAT,
    RAW_IMAGE_FORMAT_BGR32_FLOAT,
    RAW_IMAGE_FORMAT_RGBA32_FLOAT,
    RAW_IMAGE_FORMAT_BGRA32_FLOAT
};

struct RawImageFormatInfo
{
    uint8_t NumChannels;
    uint8_t BytesPerPixel;
};

RawImageFormatInfo const& GetRawImageFormatInfo(RAW_IMAGE_FORMAT Format);

class RawImage
{
public:
    RawImage() = default;

    RawImage(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, void* pData = nullptr)
    {
        Reset(Width, Height, Format, pData);
    }

    RawImage(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, Float4 const& Color)
    {
        Reset(Width, Height, Format);
        Clear(Color);
    }

    virtual ~RawImage()
    {
        Reset();
    }

    RawImage(RawImage const& Rhs) = delete;
    RawImage& operator=(RawImage const& Rhs) = delete;

    RawImage(RawImage&& Rhs) noexcept :
        m_pData(Rhs.m_pData),
        m_Width(Rhs.m_Width),
        m_Height(Rhs.m_Height),
        m_Format(Rhs.m_Format)
    {
        Rhs.m_pData  = nullptr;
        Rhs.m_Width  = 0;
        Rhs.m_Height = 0;
        Rhs.m_Format = RAW_IMAGE_FORMAT_UNDEFINED;
    }

    RawImage& operator=(RawImage&& Rhs) noexcept
    {
        Reset();

        m_pData  = Rhs.m_pData;
        m_Width  = Rhs.m_Width;
        m_Height = Rhs.m_Height;
        m_Format = Rhs.m_Format;

        Rhs.m_pData  = nullptr;
        Rhs.m_Width  = 0;
        Rhs.m_Height = 0;
        Rhs.m_Format = RAW_IMAGE_FORMAT_UNDEFINED;

        return *this;
    }

    RawImage Clone() const;

    void Reset(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, void const* pData = nullptr);

    void Reset();

    void SetExternalData(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, void* pData)
    {
        Reset();
        
        m_pData  = pData;
        m_Width  = Width;
        m_Height = Height;
        m_Format = Format;

        HK_ASSERT(pData);
        HK_ASSERT(Width);
        HK_ASSERT(Height);
        HK_ASSERT(Format != RAW_IMAGE_FORMAT_UNDEFINED);
    }

    void Clear(Float4 const& Color);

    operator bool() const
    {
        return m_pData != nullptr;
    }

    void* GetData()
    {
        return m_pData;
    }

    void const* GetData() const
    {
        return m_pData;
    }

    uint32_t GetWidth() const
    {
        return m_Width;
    }

    uint32_t GetHeight() const
    {
        return m_Height;
    }

    RAW_IMAGE_FORMAT GetFormat() const
    {
        return m_Format;
    }

    int NumChannels() const;

    size_t GetBytesPerPixel() const;

    void Swap(RawImage& Rhs)
    {
        RawImage temp = std::move(Rhs);
        Rhs           = std::move(*this);
        *this         = std::move(temp);
    }

    /** Flip image horizontally. */
    void FlipX();

    /** Flip image vertically. */
    void FlipY();

    /** Swap Red and Green channels. */
    void SwapRGB();

    void InvertChannel(uint32_t ChannelIndex);

    void InvertRed() { InvertChannel(0); }
    void InvertGreen() { InvertChannel(1); }
    void InvertBlue() { InvertChannel(2); }
    void InvertAlpha() { InvertChannel(3); }

    /** Premultiplies the RGB with the alpha channel. The image is assumed to be in the sRGB color space.
    Only allowed for RAW_IMAGE_FORMAT_RGBA8 and RAW_IMAGE_FORMAT_BGRA8 image formats. */
    void PremultiplyAlpha();

    /** Unpremultiplies the RGB with the alpha channel. The image is assumed to be in the sRGB color space.
    Only allowed for RAW_IMAGE_FORMAT_RGBA8 and RAW_IMAGE_FORMAT_BGRA8 image formats. */
    void UnpremultiplyAlpha();

private:
    void*            m_pData{};
    uint32_t         m_Width{};
    uint32_t         m_Height{};
    RAW_IMAGE_FORMAT m_Format{RAW_IMAGE_FORMAT_UNDEFINED};
};

/** Create image from stream. */
RawImage CreateRawImage(IBinaryStreamReadInterface& Stream, RAW_IMAGE_FORMAT Format = RAW_IMAGE_FORMAT_UNDEFINED);

/** Create image from file. */
RawImage CreateRawImage(StringView FileName, RAW_IMAGE_FORMAT Format = RAW_IMAGE_FORMAT_UNDEFINED);

/** Create image from SVG. The resulting image is premultiplied with an alpha channel. */
RawImage CreateRawImage(class SvgDocument const& Document, uint32_t Width, uint32_t Height, Float4 const& BackgroundColor = Float4::Zero());

/** Create image from SVG. The resulting image is premultiplied with an alpha channel. */
RawImage CreateRawImageFromSVG(IBinaryStreamReadInterface& Stream, Float2 const& Scale = Float2(1.0f), Float4 const& BackgroundColor = Float4::Zero());

enum IMAGE_FILE_FORMAT
{
    IMAGE_FILE_FORMAT_UNKNOWN,
    IMAGE_FILE_FORMAT_JPEG,
    IMAGE_FILE_FORMAT_PNG,
    IMAGE_FILE_FORMAT_TGA,
    IMAGE_FILE_FORMAT_BMP,
    IMAGE_FILE_FORMAT_PSD,
    IMAGE_FILE_FORMAT_PIC,
    IMAGE_FILE_FORMAT_PNM,
    IMAGE_FILE_FORMAT_WEBP,
    IMAGE_FILE_FORMAT_HDR,
    IMAGE_FILE_FORMAT_EXR
};

/** Reads an image file format from the stream. */
IMAGE_FILE_FORMAT GetImageFileFormat(IBinaryStreamReadInterface& Stream);

/** Selects an image file format from the file name. */
IMAGE_FILE_FORMAT GetImageFileFormat(StringView FileName);

RawImage CreateEmptyRawImage(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, Float4 const& Color);

RawImage LoadNormalMapAsRawVectors(IBinaryStreamReadInterface& Stream);
RawImage LoadNormalMapAsRawVectors(StringView FileName);

/** Flip image horizontally */
void FlipImageX(void* pData, uint32_t Width, uint32_t Height, size_t BytesPerPixel, size_t RowStride);

/** Flip image vertically */
void FlipImageY(void* pData, uint32_t Width, uint32_t Height, size_t BytesPerPixel, size_t RowStride);

/** Convert linear image to premultiplied alpha sRGB */
void LinearToPremultipliedAlphaSRGB(const float* pSrc,
                                    void*        pDest_SRGBA8,
                                    uint32_t     Width,
                                    uint32_t     Height,
                                    float        fOverbright = 0);

template <typename T>
void ClearImageChannel(T* pData, uint32_t Width, uint32_t Height, uint32_t NumChannels, uint8_t Channel, T ClearValue)
{
    if (Channel < NumChannels)
    {
        for (uint32_t i = 0, pixCount = Width * Height; i < pixCount; i++, pData += NumChannels)
            pData[Channel] = ClearValue;
    }
    else
    {
        LOG("ClearImageChannel: invalid channel index\n");
    }
}

template <typename T>
void CopyImageChannel(T const* pSrc, T* pDest, uint32_t Width, uint32_t Height, uint32_t NumSrcChannels, uint8_t NumDstChannels, uint8_t SrcChannel, uint8_t DstChannel)
{
    if (SrcChannel < NumSrcChannels && DstChannel < NumDstChannels)
    {
        for (uint32_t i = 0, pixCount = Width * Height; i < pixCount; i++, pDest += NumDstChannels, pSrc += NumSrcChannels)
            pDest[DstChannel] = pSrc[SrcChannel];
    }
    else
    {
        LOG("CopyImageChannel: invalid channel index\n");
    }
}

template <typename T>
void ExtractImageChannel(T const* pSrc, T* pDest, uint32_t Width, uint32_t Height, uint32_t NumChannels, uint8_t Channel)
{
    CopyImageChannel<T>(pSrc, pDest, Width, Height, NumChannels, 1, Channel, 0);
}

struct ImageWriteConfig
{
    /** Image width */
    uint32_t    Width{};

    /** Image height */
    uint32_t    Height{};

    /** Number of channels (Red, Red_Alpha, RGB, RGBA).
    NOTE: JPEG does ignore alpha channels in input data.
    */
    uint32_t    NumChannels{};

    /** Image data */
    const void* pData{};

    /** Quality is between 0 and 1.
    JPEG higher quality looks better but results in a bigger image.
    WebP for lossy, 0 gives the smallest size and 1 the largest. For lossless, this parameter is the amount of effort put into the compression:
    0 is the fastest but gives larger files compared to the slowest, but best, 1.*/
    float       Quality{1.0f};

    /** Lossy is only supported for WebP. JPEG is lossy by default, other formats are lossless. */
    bool        bLossless{true};

    /** This option allows to write the EXR floating point image as float16. For other formats, this option is irrelevant. */
    bool        bSaveExrAsHalf{false};
};

/** Write image in PNG format. */
bool WritePNG(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData);

/** Write image in BMP format. */
bool WriteBMP(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData);

/** Write image in TGA format. */
bool WriteTGA(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData);

/**
Write image in JPG format.
JPEG does ignore alpha channels in input data; quality is between 0 and 1.
Higher quality looks better but results in a bigger image.
*/
bool WriteJPG(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData, float Quality = 1.0f);

/** Write image in HDR format */
bool WriteHDR(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const float* pData);

/** Write image in EXR format */
bool WriteEXR(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const float* pData, bool bSaveAsHalf = false);

/**
Write image in WEBP format.
Quality is between 0 and 1. For lossy, 0 gives the smallest size and 1 the largest. For lossless, this
parameter is the amount of effort put into the compression: 0 is the fastest but gives larger files compared to the slowest, but best, 1.
*/
bool WriteWEBP(IBinaryStreamWriteInterface& Stream, uint32_t Width, uint32_t Height, uint32_t NumChannels, const void* pData, float Quality = 1.0f, bool bLossless = true);

bool WriteImage(StringView FileName, ImageWriteConfig const& Config);
bool WriteImageHDRI(StringView FileName, ImageWriteConfig const& Config);

bool WriteImage(StringView FileName, RawImage const& Image);
