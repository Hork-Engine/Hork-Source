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

#pragma once

#include <Engine/Core/IO.h>
#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

class GifImage final
{
public:
    enum DecodeFormat
    {
        RGB8,
        BGR8,
        RGBA8,
        BGRA8
    };

    struct DecodeContext
    {
        HeapBlob     Data;
        uint32_t     FrameIndex = 0;
        DecodeFormat Format = DecodeFormat::BGRA8;
    };

                    GifImage() = default;

                    GifImage(GifImage const& Rhs) = delete;
    GifImage&       operator=(GifImage const& Rhs) = delete;

                    GifImage(GifImage&& Rhs) noexcept;
    GifImage&       operator=(GifImage&& Rhs) noexcept;

    operator        bool() const { return !m_Frames.IsEmpty(); }

    void            Reset();

    uint32_t        GetWidth() const { return m_Width; }
    uint32_t        GetHeight() const { return m_Height; }
    uint32_t        GetFrameCount() const { return m_Frames.Size(); }
    uint32_t        FindFrame(float timeStamp) const;
    float           GetTimeStamp(uint32_t frameIndex) const;
    float           GetDuration() const { return m_Duration; }

    void            StartDecode(DecodeContext& context, DecodeFormat format = DecodeFormat::BGRA8) const;
    bool            DecodeNextFrame(DecodeContext& context) const;

private:
    struct Color
    {
        uint8_t     R;
        uint8_t     G;
        uint8_t     B;
    };

    struct Frame
    {
        uint32_t    Left;
        uint32_t    Top;
        uint32_t    Width;
        uint32_t    Height;
        int         TransparentColor;
        float       TimeStamp;

        uint8_t*    ColorIndex;
        Color*      ColorMap;
    };

    uint32_t        m_Width = 0;
    uint32_t        m_Height = 0;
    uint32_t        m_BackgroundColor = 0;
    HeapBlob        m_ColorMap;
    HeapBlob        m_FrameData;
    Vector<Frame>   m_Frames;
    float           m_Duration = 0;

    friend GifImage CreateGif(IBinaryStreamReadInterface& stream);
};

GifImage CreateGif(IBinaryStreamReadInterface& stream);

HK_NAMESPACE_END
