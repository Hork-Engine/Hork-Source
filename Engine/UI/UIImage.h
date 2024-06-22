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

#include "UIWidget.h"

HK_NAMESPACE_BEGIN

class UIImage : public UIWidget
{
    UI_CLASS(UIImage, UIWidget)

public:
    TextureHandle     TexHandle;
    uint32_t          TexWidth{32};
    uint32_t          TexHeight{32};
    Color4            TintColor;
    RoundingDesc      Rounding;
    CANVAS_COMPOSITE  Composite = CANVAS_COMPOSITE_SOURCE_OVER;
    Float2            Offset;
    Float2            Scale = Float2(1, 1);

    union
    {
        uint32_t      FlagBits = 0;

        struct
        {
            bool      PremultipliedAlpha : 1;
            bool      NearestFilter : 1;
            bool      StretchedX : 1;
            bool      StretchedY : 1;
            bool      TiledX : 1;
            bool      TiledY : 1;
            bool      FlipY : 1;
        } Flags;
    };
    
    UIImage& WithTexture(TextureHandle texture)
    {
        TexHandle = texture;
        return *this;
    }

    UIImage& WithTextureSize(uint32_t width, uint32_t height)
    {
        TexWidth = width;
        TexHeight = height;
        return *this;
    }

    UIImage& WithTintColor(Color4 const& tintColor)
    {
        TintColor = tintColor;
        return *this;
    }

    UIImage& WithRounding(RoundingDesc const& rounding)
    {
        Rounding = rounding;
        return *this;
    }

    UIImage& WithComposite(CANVAS_COMPOSITE composite)
    {
        Composite = composite;
        return *this;
    }

    UIImage& WithOffset(Float2 const& offset)
    {
        Offset = offset;
        return *this;
    }

    UIImage& WithScale(Float2 const& scale)
    {
        Scale = scale;
        return *this;
    }

    UIImage& WithPremultipliedAlpha(bool premultipliedAlpha)
    {
        Flags.PremultipliedAlpha = premultipliedAlpha;
        return *this;
    }

    UIImage& WithNearestFilter(bool nearestFilter)
    {
        Flags.NearestFilter = nearestFilter;
        return *this;
    }

    UIImage& WithStretchedX(bool stretchedX)
    {
        Flags.StretchedX = stretchedX;
        return *this;
    }

    UIImage& WithStretchedY(bool stretchedY)
    {
        Flags.StretchedY = stretchedY;
        return *this;
    }

    UIImage& WithTiledX(bool tiledX)
    {
        Flags.TiledX = tiledX;
        return *this;
    }

    UIImage& WithTiledY(bool tiledY)
    {
        Flags.TiledY = tiledY;
        return *this;
    }

    UIImage& WithFlipY(bool flipY)
    {
        Flags.FlipY = flipY;
        return *this;
    }

protected:
    void AdjustSize(Float2 const& size) override;

    void Draw(Canvas& canvas) override;
};

HK_NAMESPACE_END
