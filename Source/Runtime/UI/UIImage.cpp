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

#include "UIImage.h"

HK_NAMESPACE_BEGIN

void UIImage::AdjustSize(Float2 const& size)
{
    Super::AdjustSize(size);

    if (!TexView)
        return;

    if (bAutoWidth && !bStretchedX && !bTiledX)
    {
        m_AdjustedSize.X = TexView->GetWidth() * Scale.X - Padding.Left - Padding.Right;
        if (m_AdjustedSize.X < 0)
            m_AdjustedSize.X = 0;
    }

    if (bAutoHeight && !bStretchedY && !bTiledY)
    {
        m_AdjustedSize.Y = TexView->GetHeight() * Scale.Y - Padding.Top - Padding.Bottom;
        if (m_AdjustedSize.Y < 0)
            m_AdjustedSize.Y = 0;
    }
}

void UIImage::Draw(Canvas& canvas)
{
    if (!TexView)
        return;

    Float2 pos = m_Geometry.Mins;
    Float2 size;

    if (bStretchedX)
        size.X = (m_Geometry.Maxs.X - m_Geometry.Mins.X) * Scale.X;
    else
    {
        size.X = TexView->GetWidth() * Scale.X;
        pos.X += Offset.X;
    }

    if (bStretchedY)
        size.Y = (m_Geometry.Maxs.Y - m_Geometry.Mins.Y) * Scale.Y;
    else
    {
        size.Y = TexView->GetHeight() * Scale.Y;
        pos.Y += Offset.Y;
    }

    CANVAS_IMAGE_FLAGS imageFlags = CANVAS_IMAGE_DEFAULT;

    if (bTiledX && !bStretchedX)
        imageFlags |= CANVAS_IMAGE_REPEATX;

    if (bTiledY && !bStretchedY)
        imageFlags |= CANVAS_IMAGE_REPEATY;

    if (bFlipY)
        imageFlags |= CANVAS_IMAGE_FLIPY;

    if (bPremultipliedAlpha)
        imageFlags |= CANVAS_IMAGE_PREMULTIPLIED;

    if (bNearestFilter)
        imageFlags |= CANVAS_IMAGE_NEAREST;

    CanvasPaint paint;
    paint.ImagePattern(pos, size.X, size.Y, 0.0f, TexView, TintColor, imageFlags);

    auto prevComposite = canvas.CompositeOperation(Composite);

    canvas.BeginPath();
    canvas.RoundedRectVarying(m_Geometry.Mins.X, m_Geometry.Mins.Y, m_Geometry.Maxs.X - m_Geometry.Mins.X, m_Geometry.Maxs.Y - m_Geometry.Mins.Y,
                              Rounding.RoundingTL,
                              Rounding.RoundingTR,
                              Rounding.RoundingBR,
                              Rounding.RoundingBL);
    canvas.FillPaint(paint);
    canvas.Fill();

    canvas.CompositeOperation(prevComposite);
}

HK_NAMESPACE_END
