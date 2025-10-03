/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Image/Gif.h>
#include <Hork/Core/Logger.h>

#include <giflib/gif_lib.h>

HK_NAMESPACE_BEGIN

GifImage::GifImage(GifImage&& Rhs) noexcept :
    m_Width(Rhs.m_Width),
    m_Height(Rhs.m_Height),
    m_BackgroundColor(Rhs.m_BackgroundColor),
    m_ColorMap(std::move(Rhs.m_ColorMap)),
    m_FrameData(std::move(Rhs.m_FrameData)),
    m_Frames(std::move(Rhs.m_Frames)),
    m_Duration(Rhs.m_Duration)
{
    Rhs.m_Width = 0;
    Rhs.m_Height = 0;
    Rhs.m_BackgroundColor = 0;
    Rhs.m_Duration = 0;
}

GifImage& GifImage::operator=(GifImage&& Rhs) noexcept
{
    Reset();

    m_Width = Rhs.m_Width;
    m_Height = Rhs.m_Height;
    m_BackgroundColor = Rhs.m_BackgroundColor;
    m_ColorMap = std::move(Rhs.m_ColorMap);
    m_FrameData = std::move(Rhs.m_FrameData);
    m_Frames = std::move(Rhs.m_Frames);
    m_Duration = Rhs.m_Duration;

    Rhs.m_Width = 0;
    Rhs.m_Height = 0;
    Rhs.m_BackgroundColor = 0;
    Rhs.m_Duration = 0;

    return *this;
}

void GifImage::Reset()
{
    m_Width = 0;
    m_Height = 0;
    m_BackgroundColor = 0;
    m_Duration = 0;
    m_ColorMap.Reset();
    m_FrameData.Reset();
    m_Frames.Clear();
}

namespace
{

    int GetBytesPerPixel(GifImage::DecodeFormat format)
    {
        switch (format)
        {
        case GifImage::DecodeFormat::RGB8:
        case GifImage::DecodeFormat::BGR8:
            return 3;
        case GifImage::DecodeFormat::RGBA8:
        case GifImage::DecodeFormat::BGRA8:
            return 4;
        }

        HK_ASSERT(0);
        return 4;
    }

    bool IsBGR(GifImage::DecodeFormat format)
    {
        switch (format)
        {
        case GifImage::DecodeFormat::RGB8:
        case GifImage::DecodeFormat::RGBA8:
            return false;
        }
        return true;
    }

}

void GifImage::StartDecode(DecodeContext& context, DecodeFormat format) const
{
    context.FrameIndex = 0;
    context.Format = format;

    int bpp = GetBytesPerPixel(format);

    size_t requiredFrameSize = (size_t)m_Width * m_Height * bpp;
    if (context.Data.Size() < requiredFrameSize)
        context.Data.Reset(requiredFrameSize);
}

bool GifImage::DecodeNextFrame(DecodeContext& context) const
{
    if (context.FrameIndex >= m_Frames.Size())
        return false;

    //LOG("DECODE FRAME {} from {}\n", context.FrameIndex, m_Frames.Size());

    Frame const& frame = m_Frames[context.FrameIndex];

    int bpp = GetBytesPerPixel(context.Format);
    bool bgr = IsBGR(context.Format);

    if (context.Data.Size() < m_Width * m_Height * bpp)
        return false;

    if (context.FrameIndex == 0)
    {
        Color color = ((Color const*)m_ColorMap.GetData())[m_BackgroundColor];

        if (bgr)
            std::swap(color.R, color.B);

        uint8_t* data = (uint8_t*)context.Data.GetData();
        for (uint32_t y = 0; y < m_Height; ++y)
        {
            for (uint32_t x = 0; x < m_Width; ++x)
            {
                uint8_t* dst = data + x * bpp;

                dst[0] = color.R;
                dst[1] = color.G;
                dst[2] = color.B;

                if (bpp == 4)
                    dst[3] = 255;
            }
            data += m_Width * bpp;
        }
    }

    const int R = bgr ? 2 : 0;
    const int B = bgr ? 0 : 2;

    uint8_t* data = (uint8_t*)context.Data.GetData();
    data += frame.Top * m_Width * bpp + frame.Left * bpp;
    for (uint32_t y = 0; y < frame.Height; ++y)
    {
        for (uint32_t x = 0; x < frame.Width; ++x)
        {
            int index = frame.ColorIndex[y * frame.Width + x];
            if (index != frame.TransparentColor)
            {
                auto& color = frame.ColorMap[index];

                uint8_t* dst = data + x * bpp;

                dst[R] = color.R;
                dst[1] = color.G;
                dst[B] = color.B;
            }
        }
        data += m_Width * bpp;
    }

    context.FrameIndex++;

    return true;
}

uint32_t GifImage::FindFrame(float timeStamp) const
{
    if (m_Frames.IsEmpty())
        return 0;

    for (uint32_t index = 0; index < m_Frames.Size(); ++index)
    {
        if (timeStamp <= m_Frames[index].TimeStamp)
            return index;
    }

    return m_Frames.Size() - 1;
}

float GifImage::GetTimeStamp(uint32_t frameIndex) const
{
    if (frameIndex >= m_Frames.Size())
        return m_Duration;

    return m_Frames[frameIndex].TimeStamp;
}

GifImage CreateGif(IBinaryStreamReadInterface& stream)
{
    auto read = [](GifFileType *file, GifByteType *data, int size) -> int
        {
            return ((IBinaryStreamReadInterface*)file->UserData)->Read(data, size);
        };

    struct Deleter
    {
        void operator()(GifFileType* gif) const
        {
            DGifCloseFile(gif, nullptr);
        };
    };

    int error = 0;
    std::unique_ptr<GifFileType, Deleter> gif(DGifOpen(&stream, read, &error));
    if (!gif)
        return {};

    if (DGifSlurp(gif.get()) != GIF_OK)
        return {};

    GifImage image;

    image.m_Width = gif->SWidth;
    image.m_Height = gif->SHeight;

    image.m_BackgroundColor = gif->SBackGroundColor;
    image.m_Frames.Resize(gif->ImageCount);

    int totalColorsCount = 0;
    size_t totalPixelCount = 0;

    if (gif->SColorMap)
        totalColorsCount += gif->SColorMap->ColorCount;

    const float FrameRate = 1.0f / 10; // default frame rate
    float timeStamp = 0;

    for (int i = 0; i < gif->ImageCount; ++i)
    {
        auto& frame = image.m_Frames[i];

        frame.Left = gif->SavedImages[i].ImageDesc.Left;
        frame.Top = gif->SavedImages[i].ImageDesc.Top;
        frame.Width = gif->SavedImages[i].ImageDesc.Width;
        frame.Height = gif->SavedImages[i].ImageDesc.Height;

        if ((frame.Left + frame.Width > image.m_Width) || (frame.Top + frame.Height > image.m_Height))
            return {};

        frame.TimeStamp = timeStamp;

        GraphicsControlBlock gcb;
        if (DGifSavedExtensionToGCB(gif.get(), i, &gcb) == GIF_OK)
        {
            frame.TransparentColor = gcb.TransparentColor;
            timeStamp += static_cast<float>(gcb.DelayTime) * 0.01f;

            // TODO: Take into account DisposalMode?
        }
        else
        {
            frame.TransparentColor = NO_TRANSPARENT_COLOR;
            timeStamp += FrameRate;
        }

        totalPixelCount += (size_t)gif->SavedImages[i].ImageDesc.Width * gif->SavedImages[i].ImageDesc.Height;

        auto colormap = gif->SavedImages[i].ImageDesc.ColorMap;
        if (!colormap || colormap == gif->SColorMap)
        {
            // Frame has default color map
            frame.ColorMap = (GifImage::Color*)0;
            continue;
        }

        bool hasColormap = false;
        for (int j = 0; j < i; ++j)
        {
            if (colormap == gif->SavedImages[j].ImageDesc.ColorMap)
            {
                frame.ColorMap = image.m_Frames[j].ColorMap;
                hasColormap = true;
                break;
            }
        }

        if (!hasColormap)
        {
            frame.ColorMap = (GifImage::Color*)0 + totalColorsCount;
            totalColorsCount += colormap->ColorCount;
        }
    }

    if (!totalColorsCount)
        return {};

    image.m_Duration = timeStamp;

    image.m_ColorMap.Reset(totalColorsCount * sizeof(GifImage::Color));
    image.m_FrameData.Reset(totalPixelCount);

    GifImage::Color* destColorMap = (GifImage::Color*)image.m_ColorMap.GetData();
    uint8_t* destPixels = (uint8_t*)image.m_FrameData.GetData();

    if (gif->SColorMap)
    {
        Core::Memcpy(destColorMap, gif->SColorMap->Colors, gif->SColorMap->ColorCount*sizeof(GifImage::Color));
        destColorMap += gif->SColorMap->ColorCount;
    }

    for (int i = 0; i < gif->ImageCount; ++i)
    {
        auto& frame = image.m_Frames[i];

        frame.ColorIndex = destPixels;

        size_t pixelCount = (size_t)gif->SavedImages[i].ImageDesc.Width * gif->SavedImages[i].ImageDesc.Height;
        Core::Memcpy(destPixels, gif->SavedImages[i].RasterBits, pixelCount);

        destPixels += pixelCount;

        auto colormap = gif->SavedImages[i].ImageDesc.ColorMap;
        if (!colormap || colormap == gif->SColorMap)
        {
            frame.ColorMap = (GifImage::Color*)image.m_ColorMap.GetData();
            continue;
        }

        bool hasColormap = false;
        for (int j = 0; j < i; ++j)
        {
            if (colormap == gif->SavedImages[j].ImageDesc.ColorMap)
            {
                frame.ColorMap = image.m_Frames[j].ColorMap;
                hasColormap = true;
                break;
            }
        }

        if (!hasColormap)
        {
            frame.ColorMap = destColorMap;

            Core::Memcpy(destColorMap, colormap->Colors, colormap->ColorCount*sizeof(GifImage::Color));
            destColorMap += colormap->ColorCount;
        }
    }

    return image;
}

HK_NAMESPACE_END
