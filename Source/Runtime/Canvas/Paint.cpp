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

#include "Paint.h"

CanvasPaint& CanvasPaint::LinearGradient(Float2 const& startPoint, Float2 const& endPoint, Color4 const& innerColor, Color4 const& outerColor)
{
    const float large = 1e5;

    Float2 dir = endPoint - startPoint;

    float d = dir.Length();
    if (d > 0.0001f)
    {
        dir /= d;
    }
    else
    {
        dir.X = 0;
        dir.Y = 1;
    }

    Xform[0][0] = dir.Y;
    Xform[0][1] = -dir.X;
    Xform[1][0] = dir.X;
    Xform[1][1] = dir.Y;
    Xform[2] = startPoint - dir * large;

    Extent[0] = large;
    Extent[1] = large + d * 0.5f;

    Radius = 0.0f;

    Feather = Math::Max(1.0f, d);

    InnerColor = innerColor;
    OuterColor = outerColor;

    pTextureView = nullptr;
    ImageFlags = CANVAS_IMAGE_DEFAULT;

    return *this;
}

CanvasPaint& CanvasPaint::RadialGradient(Float2 const& center, float innerRadius, float outerRadius, Color4 const& innerColor, Color4 const& outerColor)
{
    Xform = Transform2D::Translation(center);

    Extent[0] = Extent[1] = Radius = (innerRadius + outerRadius) * 0.5f;

    Feather = Math::Max(1.0f, outerRadius - innerRadius);

    InnerColor = innerColor;
    OuterColor = outerColor;

    pTextureView = nullptr;
    ImageFlags = CANVAS_IMAGE_DEFAULT;

    return *this;
}

CanvasPaint& CanvasPaint::BoxGradient(Float2 const& boxTopLeft, float w, float h, float cornerRadius, float feather, Color4 const& innerColor, Color4 const& outerColor)
{
    Xform = Transform2D::Translation({boxTopLeft.X + w * 0.5f, boxTopLeft.Y + h * 0.5f});

    Extent[0] = w * 0.5f;
    Extent[1] = h * 0.5f;

    Radius = cornerRadius;

    Feather = Math::Max(1.0f, feather);

    InnerColor = innerColor;
    OuterColor = outerColor;

    pTextureView = nullptr;
    ImageFlags = CANVAS_IMAGE_DEFAULT;

    return *this;
}

CanvasPaint& CanvasPaint::ImagePattern(Float2 const& posTopLeft, float w, float h, float angleInRadians, TextureView* textureView, Color4 const& tintColor, CANVAS_IMAGE_FLAGS imageFlags)
{
    if (angleInRadians != 0.0f)
    {
        Xform = Transform2D::Rotation(angleInRadians);

        Xform[2] = posTopLeft;
    }
    else
        Xform = Transform2D::Translation(posTopLeft);    

    Extent[0] = w;
    Extent[1] = h;

    pTextureView = textureView;
    ImageFlags = imageFlags;

    InnerColor = OuterColor = tintColor;

    return *this;
}

CanvasPaint& CanvasPaint::Solid(Color4 const& color)
{
    Xform.SetIdentity();

    Extent[0] = 0;
    Extent[1] = 0;

    Radius  = 0;
    Feather = 1;

    InnerColor = OuterColor = color;

    pTextureView = nullptr;
    ImageFlags = CANVAS_IMAGE_DEFAULT;

    return *this;
}
