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

#include "UIBrush.h"
#include "UIWidget.h"

HK_NAMESPACE_BEGIN

static void DrawBoxGradient(CanvasPaint& paint, Float2 const& mins, Float2 const& maxs, UIBoxGradient* brush)
{
    Float2 boxMins = mins + brush->BoxOffsetTopLeft;
    Float2 boxMaxs = maxs + brush->BoxOffsetBottomRight;
    paint.BoxGradient(boxMins, boxMaxs.X - boxMins.X, boxMaxs.Y - boxMins.Y, brush->CornerRadius, brush->Feather, brush->InnerColor, brush->OuterColor);
}

static void DrawLinearGradient(CanvasPaint& paint, Float2 const& mins, Float2 const& maxs, UILinearGradient* brush)
{
    Float2 sp = mins + brush->StartPoint;
    Float2 ep = mins + brush->EndPoint;
    paint.LinearGradient(sp, ep, brush->InnerColor, brush->OuterColor);
}

static void DrawRadialGradient(CanvasPaint& paint, Float2 const& mins, Float2 const& maxs, UIRadialGradient* brush)
{
    Float2 center = (mins + maxs) * 0.5f + brush->CenterOffset;
    Float2 size   = maxs - mins;
    float  radius = Math::Max(size.X, size.Y);
    paint.RadialGradient(center, radius * brush->InnerRadius, radius * brush->OuterRadius, brush->InnerColor, brush->OuterColor);
}

static void DrawSolid(CanvasPaint& paint, Float2 const& mins, Float2 const& maxs, UISolidBrush* brush)
{
    paint.Solid(brush->Color);
}

static void DrawImage(CanvasPaint& paint, Float2 const& mins, Float2 const& maxs, UIImageBrush* brush)
{
    if (brush->TexView)
    {
        CANVAS_IMAGE_FLAGS imageFlags = CANVAS_IMAGE_REPEATX | CANVAS_IMAGE_REPEATY;

        if (brush->bPremultipliedAlpha)
            imageFlags |= CANVAS_IMAGE_PREMULTIPLIED;

        if (brush->bNearestFilter)
            imageFlags |= CANVAS_IMAGE_NEAREST;

        if (brush->bFlipY)
            imageFlags |= CANVAS_IMAGE_FLIPY;

        if (brush->bStretch)
        {
            paint.ImagePattern(mins, maxs.X - mins.X, maxs.Y - mins.Y, 0.0f, brush->TexView, brush->TintColor, imageFlags);
        }
        else
        {
            paint.ImagePattern({mins.X + brush->Offset.X, mins.Y + brush->Offset.Y}, brush->TexView->GetWidth() * brush->Scale.X, brush->TexView->GetHeight() * brush->Scale.Y, 0.0f, brush->TexView, brush->TintColor, imageFlags);
        }
    }
    else
        paint.Solid(brush->TintColor);
}

void DrawBrush(Canvas& canvas, Float2 const& mins, Float2 const& maxs, TArrayView<Float2> vertices, UIBrush* brush)
{
    if (brush->Type == UIBrush::CUSTOM)
    {
        static_cast<UICustomBrush*>(brush)->Draw(canvas, mins, maxs, vertices);
        return;
    }

    if (vertices.Size() > 1)
    {
        canvas.BeginPath();
        canvas.MoveTo(vertices[0]);
        for (int i = 1; i < vertices.Size(); ++i)
            canvas.LineTo(vertices[i]);
    }
    else
    {
        canvas.BeginPath();
        canvas.RoundedRectVarying(mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y,
                                  brush->Rounding.RoundingTL,
                                  brush->Rounding.RoundingTR,
                                  brush->Rounding.RoundingBR,
                                  brush->Rounding.RoundingBL);
    }

    auto prevComposite = canvas.CompositeOperation(brush->Composite);

    CanvasPaint paint;
    switch (brush->Type)
    {
        case UIBrush::BOX_GRADIENT:
            DrawBoxGradient(paint, mins, maxs, static_cast<UIBoxGradient*>(brush));
            break;
        case UIBrush::LINEAR_GRADIENT:
            DrawLinearGradient(paint, mins, maxs, static_cast<UILinearGradient*>(brush));
            break;
        case UIBrush::RADIAL_GRADIENT:
            DrawRadialGradient(paint, mins, maxs, static_cast<UIRadialGradient*>(brush));
            break;
        case UIBrush::SOLID:
            DrawSolid(paint, mins, maxs, static_cast<UISolidBrush*>(brush));
            break;
        case UIBrush::IMAGE:
            DrawImage(paint, mins, maxs, static_cast<UIImageBrush*>(brush));
            break;
    }

    if (brush->bFilled)
    {
        canvas.FillPaint(paint);
        canvas.Fill();
    }
    else
    {
        canvas.StrokeWidth(brush->StrokeWidth);
        canvas.StrokePaint(paint);
        canvas.Stroke();
    }

    canvas.CompositeOperation(prevComposite);
}

HK_NAMESPACE_END
