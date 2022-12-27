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
#include <Runtime/Canvas/Canvas.h>
#include <Containers/ArrayView.h>

HK_NAMESPACE_BEGIN

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

    UIBrush& WithComposite(CANVAS_COMPOSITE composite)
    {
        Composite = composite;
        return *this;
    }

    UIBrush& WithRounding(RoundingDesc const& rounding)
    {
        Rounding = rounding;
        return *this;
    }

    UIBrush& WithStrokeWidth(float strokeWidth)
    {
        StrokeWidth = strokeWidth;
        return *this;
    }

    UIBrush& WithFilled(bool filled)
    {
        bFilled = filled;
        return *this;
    }
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

    UIBoxGradient& WithBoxOffsetTopLeft(Float2 const& boxOffsetTopLeft)
    {
        BoxOffsetTopLeft = boxOffsetTopLeft;
        return *this;
    }

    UIBoxGradient& WithBoxOffsetBottomRight(Float2 const& boxOffsetBottomRight)
    {
        BoxOffsetBottomRight = boxOffsetBottomRight;
        return *this;
    }

    UIBoxGradient& WithCornerRadius(float cornerRadius)
    {
        CornerRadius = cornerRadius;
        return *this;
    }

    UIBoxGradient& WithFeather(float feather)
    {
        Feather = feather;
        return *this;
    }

    UIBoxGradient& WithInnerColor(Color4 const& innerColor)
    {
        InnerColor = innerColor;
        return *this;
    }

    UIBoxGradient& WithOuterColor(Color4 const& outerColor)
    {
        OuterColor = outerColor;
        return *this;
    }
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

    UILinearGradient& WithStartPoint(Float2 const& startPoint)
    {
        StartPoint = startPoint;
        return *this;
    }

    UILinearGradient& WithEndPoint(Float2 const& endPoint)
    {
        EndPoint = endPoint;
        return *this;
    }

    UILinearGradient& WithInnerColor(Color4 const& innerColor)
    {
        InnerColor = innerColor;
        return *this;
    }

    UILinearGradient& WithOuterColor(Color4 const& outerColor)
    {
        OuterColor = outerColor;
        return *this;
    }
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

    UIRadialGradient& WithCenterOffset(Float2 const& centerOffset)
    {
        CenterOffset = centerOffset;
        return *this;
    }

    UIRadialGradient& WithInnerRadius(float innerRadius)
    {
        InnerRadius = innerRadius;
        return *this;
    }

    UIRadialGradient& WithOuterRadius(float outerRadius)
    {
        OuterRadius = outerRadius;
        return *this;
    }

    UIRadialGradient& WithInnerColor(Color4 const& innerColor)
    {
        InnerColor = innerColor;
        return *this;
    }

    UIRadialGradient& WithOuterColor(Color4 const& outerColor)
    {
        OuterColor = outerColor;
        return *this;
    }
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
    TRef<TextureView> TexView;
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

    UIImageBrush(Texture* texture) :
        UIBrush(IMAGE),
        TexView(texture ? texture->GetView() : nullptr)
    {}

    UIImageBrush(TextureView* texture) :
        UIBrush(IMAGE),
        TexView(texture)
    {}

    UIImageBrush& WithTexture(Texture* texture)
    {
        TexView = texture ? texture->GetView() : nullptr;
        return *this;
    }

    UIImageBrush& WithTexture(TextureView* textureView)
    {
        TexView = textureView;
        return *this;
    }

    UIImageBrush& WithTintColor(Color4 const& tintColor)
    {
        TintColor = tintColor;
        return *this;
    }

    UIImageBrush& WithOffset(Float2 const& offset)
    {
        Offset = offset;
        return *this;
    }

    UIImageBrush& WithScale(Float2 const& scale)
    {
        Scale = scale;
        return *this;
    }

    UIImageBrush& WithPremultipliedAlpha(bool premultipliedAlpha)
    {
        bPremultipliedAlpha = premultipliedAlpha;
        return *this;
    }

    UIImageBrush& WithNearestFilter(bool nearestFilter)
    {
        bNearestFilter = nearestFilter;
        return *this;
    }

    UIImageBrush& WithFlipY(bool flipY)
    {
        bFlipY = flipY;
        return *this;
    }

    UIImageBrush& WithStretch(bool stretch)
    {
        bStretch = stretch;
        return *this;
    }
};

class UICustomBrush : public UIBrush
{
    UI_CLASS(UICustomBrush, UIBrush)

public:
    UICustomBrush() :
        UIBrush(CUSTOM)
    {}

    virtual void Draw(Canvas& canvas, Float2 const& mins, Float2 const& maxs, TArrayView<Float2> vertices) {}
};

void DrawBrush(Canvas& canvas, Float2 const& mins, Float2 const& maxs, TArrayView<Float2> vertices, UIBrush* brush);

HK_NAMESPACE_END
