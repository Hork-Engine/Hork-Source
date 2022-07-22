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

class ARawImage
{
public:
    ARawImage() = default;

    ARawImage(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, void* pData = nullptr)
    {
        Reset(Width, Height, Format, pData);
    }

    ARawImage(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, Float4 const& Color)
    {
        Reset(Width, Height, Format);
        Clear(Color);
    }

    virtual ~ARawImage()
    {
        Reset();
    }

    ARawImage(ARawImage const& Rhs) = delete;
    ARawImage& operator=(ARawImage const& Rhs) = delete;

    ARawImage(ARawImage&& Rhs) noexcept :
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

    ARawImage& operator=(ARawImage&& Rhs) noexcept
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

    ARawImage Clone() const;

    void Reset(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, void const* pData = nullptr);

    void Reset();

    void SetExternalData(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, void* pData)
    {
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

    void Swap(ARawImage& Rhs)
    {
        ARawImage temp = std::move(Rhs);
        Rhs            = std::move(*this);
        *this          = std::move(temp);
    }

    /** Flip image horizontally. */
    void FlipX();

    /** Flip image vertically. */
    void FlipY();

    /** Swap Red and Green channels. */
    void SwapRGB();

private:
    void*            m_pData{};
    uint32_t         m_Width{};
    uint32_t         m_Height{};
    RAW_IMAGE_FORMAT m_Format{RAW_IMAGE_FORMAT_UNDEFINED};
};

ARawImage CreateRawImage(IBinaryStreamReadInterface& Stream, RAW_IMAGE_FORMAT Format = RAW_IMAGE_FORMAT_UNDEFINED);
ARawImage CreateRawImage(AStringView FileName, RAW_IMAGE_FORMAT Format = RAW_IMAGE_FORMAT_UNDEFINED);

ARawImage CreateEmptyRawImage(uint32_t Width, uint32_t Height, RAW_IMAGE_FORMAT Format, Float4 const& Color);

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
