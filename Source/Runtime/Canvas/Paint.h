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

#include <Runtime/Texture.h>
#include "Transform2D.h"

/**
Paints

Supported four types of paints: linear gradient, box gradient, radial gradient and image pattern.
These can be used as paints for strokes and fills.
*/
struct CanvasPaint
{
    Transform2D           Xform;
    float                 Extent[2];
    float                 Radius{};
    float                 Feather{};
    Color4                InnerColor;
    Color4                OuterColor;
    TextureView*         pTextureView{};
    CANVAS_IMAGE_FLAGS    ImageFlags = CANVAS_IMAGE_DEFAULT;

    /** Creates and returns a linear gradient.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& LinearGradient(Float2 const& startPoint, Float2 const& endPoint, Color4 const& innerColor, Color4 const& outerColor);

    /** Creates and returns a radial gradient.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& RadialGradient(Float2 const& center, float innerRadius, float outerRadius, Color4 const& innerColor, Color4 const& outerColor);

    /** Creates and returns a box gradient. Box gradient is a feathered rounded rectangle, it is useful for rendering
    drop shadows or highlights for boxes. Feather defines how blurry the border of the rectangle is.
    Parameter innerColor specifies the inner color and outerColor the outer color of the gradient.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& BoxGradient(Float2 const& boxTopLeft, float w, float h, float cornerRadius, float feather, Color4 const& innerColor, Color4 const& outerColor);

    /** Creates and returns an image patter. Angle is a rotation around the top-left corner.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& ImagePattern(Float2 const& posTopLeft, float w, float h, float angleInRadians, TextureView* texture, Color4 const& tintColor, CANVAS_IMAGE_FLAGS imageFlags = CANVAS_IMAGE_DEFAULT);

    CanvasPaint& Solid(Color4 const& color);
};
