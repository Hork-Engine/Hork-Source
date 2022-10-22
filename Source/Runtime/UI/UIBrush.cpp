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

static void DrawBoxGradient(CanvasPaint& paint, UIWidgetGeometry const& geometry, UIBoxGradient* brush)
{
    Float2 boxMins = geometry.Mins + brush->BoxOffsetTopLeft;
    Float2 boxMaxs = geometry.Maxs + brush->BoxOffsetBottomRight;
    paint.BoxGradient(boxMins.X, boxMins.Y, boxMaxs.X - boxMins.X, boxMaxs.Y - boxMins.Y, brush->CornerRadius, brush->Feather, brush->InnerColor, brush->OuterColor);
}

static void DrawLinearGradient(CanvasPaint& paint, UIWidgetGeometry const& geometry, UILinearGradient* brush)
{
    Float2 sp = geometry.Mins + brush->StartPoint;
    Float2 ep = geometry.Mins + brush->EndPoint;
    paint.LinearGradient(sp.X, sp.Y, ep.X, ep.Y, brush->InnerColor, brush->OuterColor);
}

static void DrawRadialGradient(CanvasPaint& paint, UIWidgetGeometry const& geometry, UIRadialGradient* brush)
{
    Float2 center = (geometry.Mins + geometry.Maxs) * 0.5f + brush->CenterOffset;
    Float2 size   = geometry.Maxs - geometry.Mins;
    float  radius = Math::Max(size.X, size.Y);
    paint.RadialGradient(center.X, center.Y, radius * brush->InnerRadius, radius * brush->OuterRadius, brush->InnerColor, brush->OuterColor);
}

static void DrawSolid(CanvasPaint& paint, UIWidgetGeometry const& geometry, UISolidBrush* brush)
{
    paint.Solid(brush->Color);
}

static void DrawImage(CanvasPaint& paint, UIWidgetGeometry const& geometry, UIImageBrush* brush)
{
    if (brush->Texture)
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
            paint.ImagePattern(geometry.Mins.X, geometry.Mins.Y, geometry.Maxs.X - geometry.Mins.X, geometry.Maxs.Y - geometry.Mins.Y, 0.0f, brush->Texture, brush->TintColor, imageFlags);
        }
        else
        {
            paint.ImagePattern(geometry.Mins.X + brush->Offset.X, geometry.Mins.Y + brush->Offset.Y, brush->Texture->GetDimensionX() * brush->Scale.X, brush->Texture->GetDimensionY() * brush->Scale.Y, 0.0f, brush->Texture, brush->TintColor, imageFlags);
        }
    }
    else
        paint.Solid(brush->TintColor);
}

void DrawBrush(ACanvas& canvas, UIWidgetGeometry const& geometry, UIBrush* brush)
{
    if (brush->Type == UIBrush::CUSTOM)
    {
        static_cast<UICustomBrush*>(brush)->Draw(canvas, geometry.Mins, geometry.Maxs);
        return;
    }

    //if (!Shape.IsEmpty())
    //{
    //    if (Shape.Size() < 2)
    //        return;

    //    canvas.BeginPath();
    //    canvas.MoveTo(Shape[0]);
    //    for (int i = 1; i < Shape.Size(); ++i)
    //    {
    //        canvas.LineTo(Shape[i]);
    //    }
    //}
    //else
    {
        canvas.BeginPath();
        canvas.RoundedRectVarying(geometry.Mins.X, geometry.Mins.Y, geometry.Maxs.X - geometry.Mins.X, geometry.Maxs.Y - geometry.Mins.Y,
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
            DrawBoxGradient(paint, geometry, static_cast<UIBoxGradient*>(brush));
            break;
        case UIBrush::LINEAR_GRADIENT:
            DrawLinearGradient(paint, geometry, static_cast<UILinearGradient*>(brush));
            break;
        case UIBrush::RADIAL_GRADIENT:
            DrawRadialGradient(paint, geometry, static_cast<UIRadialGradient*>(brush));
            break;
        case UIBrush::SOLID:
            DrawSolid(paint, geometry, static_cast<UISolidBrush*>(brush));
            break;
        case UIBrush::IMAGE:
            DrawImage(paint, geometry, static_cast<UIImageBrush*>(brush));
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
