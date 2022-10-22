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

#include "UIObject.h"
#include <Runtime/Canvas.h>

class UIBrush : public UIObject
{
    UI_CLASS(UIBrush, UIObject)

public:
    enum TYPE
    {
        UNDEFINED,
        BOX_GRADIENT,
        LINEAR_GRADIENT,
        RADIAL_GRADIENT,
        SOLID,
        IMAGE,
        CUSTOM
    };

    const TYPE Type;

    CANVAS_COMPOSITE Composite = CANVAS_COMPOSITE_SOURCE_OVER;
    RoundingDesc     Rounding;
    float            StrokeWidth = 1;
    bool             bFilled = true;

    UIBrush(TYPE type = UNDEFINED) :
        Type(type)
    {}
};

class UIBoxGradient : public UIBrush
{
    UI_CLASS(UIBoxGradient, UIBrush)

public:
    Float2       BoxOffsetTopLeft;
    Float2       BoxOffsetBottomRight;
    float        CornerRadius = 0;
    float        Feather      = 4;
    Color4       InnerColor;
    Color4       OuterColor;

    UIBoxGradient() :
        UIBrush(BOX_GRADIENT)
    {}
};

class UILinearGradient : public UIBrush
{
    UI_CLASS(UILinearGradient, UIBrush)

public:
    Float2       StartPoint;
    Float2       EndPoint;
    Color4       InnerColor;
    Color4       OuterColor;

    UILinearGradient() :
        UIBrush(LINEAR_GRADIENT)
    {}
};

class UIRadialGradient : public UIBrush
{
    UI_CLASS(UIRadialGradient, UIBrush)

public:
    Float2       CenterOffset;
    float        InnerRadius = 1;
    float        OuterRadius = 1;
    Color4       InnerColor;
    Color4       OuterColor;

    UIRadialGradient() :
        UIBrush(RADIAL_GRADIENT)
    {}
};

class UISolidBrush : public UIBrush
{
    UI_CLASS(UISolidBrush, UIBrush)

public:
    Color4 Color;

    UISolidBrush() :
        UIBrush(SOLID)
    {}

    UISolidBrush(Color4 const& color) :
        UIBrush(SOLID),
        Color(color)
    {}
};

class UIImageBrush : public UIBrush
{
    UI_CLASS(UIImageBrush, UIBrush)

public:
    TRef<ATexture> Texture;
    Color4         TintColor;
    Float2         Offset;
    Float2         Scale               = Float2(1,1);
    bool           bPremultipliedAlpha = false;
    bool           bNearestFilter      = false;
    bool           bFlipY              = false;
    bool           bStretch            = false;

    UIImageBrush() :
        UIBrush(IMAGE)
    {}
    UIImageBrush(ATexture* texture) :
        UIBrush(IMAGE),
        Texture(texture)
    {}
};

class UICustomBrush : public UIBrush
{
    UI_CLASS(UICustomBrush, UIBrush)

public:
    UICustomBrush() :
        UIBrush(CUSTOM)
    {}

    virtual void Draw(ACanvas& canvas, Float2 const& mins, Float2 const& maxs) {}
};

void DrawBrush(ACanvas& canvas, struct UIWidgetGeometry const& geometry, UIBrush* brush);
