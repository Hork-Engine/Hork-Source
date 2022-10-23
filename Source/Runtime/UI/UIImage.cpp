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

void UIImage::AdjustSize(Float2 const& size)
{
    Super::AdjustSize(size);

    if (!Texture)
        return;

    if (bAutoWidth && !bStretchedX && !bTiledX)
    {
        AdjustedSize.X = Texture->GetDimensionX() * Scale.X - Padding.Left - Padding.Right;
        if (AdjustedSize.X < 0)
            AdjustedSize.X = 0;
    }

    if (bAutoHeight && !bStretchedY && !bTiledY)
    {
        AdjustedSize.Y = Texture->GetDimensionY() * Scale.Y - Padding.Top - Padding.Bottom;
        if (AdjustedSize.Y < 0)
            AdjustedSize.Y = 0;
    }
}

void UIImage::Draw(ACanvas& canvas)
{
    if (!Texture)
        return;

    Float2 pos = Geometry.Mins;
    Float2 size;

    if (bStretchedX)
        size.X = (Geometry.Maxs.X - Geometry.Mins.X) * Scale.X;
    else
    {
        size.X = Texture->GetDimensionX() * Scale.X;
        pos.X += Offset.X;
    }

    if (bStretchedY)
        size.Y = (Geometry.Maxs.Y - Geometry.Mins.Y) * Scale.Y;
    else
    {
        size.Y = Texture->GetDimensionY() * Scale.Y;
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
    paint.ImagePattern(pos.X, pos.Y, size.X, size.Y, 0.0f, Texture, TintColor, imageFlags);

    auto prevComposite = canvas.CompositeOperation(Composite);

    canvas.BeginPath();
    canvas.RoundedRectVarying(Geometry.Mins.X, Geometry.Mins.Y, Geometry.Maxs.X - Geometry.Mins.X, Geometry.Maxs.Y - Geometry.Mins.Y,
                              Rounding.RoundingTL,
                              Rounding.RoundingTR,
                              Rounding.RoundingBR,
                              Rounding.RoundingBL);
    canvas.FillPaint(paint);
    canvas.Fill();

    canvas.CompositeOperation(prevComposite);
}
