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

#include "UIDecorator.h"
#include "UIWidget.h"

void UIBrushDecorator::DrawInactive(ACanvas& canvas, UIWidgetGeometry const& geometry)
{
    if (InactiveBrush)
        DrawBrush(canvas, geometry.Mins, geometry.Maxs, {}, InactiveBrush);
}

void UIBrushDecorator::DrawActive(ACanvas& canvas, UIWidgetGeometry const& geometry)
{
    if (ActiveBrush)
        DrawBrush(canvas, geometry.Mins, geometry.Maxs, {}, ActiveBrush);
}

void UIBrushDecorator::DrawHovered(ACanvas& canvas, UIWidgetGeometry const& geometry)
{
    if (HoverBrush)
        DrawBrush(canvas, geometry.Mins, geometry.Maxs, {}, HoverBrush);
}

void UIBrushDecorator::DrawSelected(ACanvas& canvas, UIWidgetGeometry const& geometry)
{
    if (SelectedBrush)
        DrawBrush(canvas, geometry.Mins, geometry.Maxs, {}, SelectedBrush);
}

void UIBrushDecorator::DrawDisabled(ACanvas& canvas, UIWidgetGeometry const& geometry)
{
    if (DisabledBrush)
        DrawBrush(canvas, geometry.Mins, geometry.Maxs, {}, DisabledBrush);
}
