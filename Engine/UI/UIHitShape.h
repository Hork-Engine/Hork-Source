/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#include <Engine/Core/Containers/BitMask.h>
#include <Engine/Math/VectorMath.h>

HK_NAMESPACE_BEGIN

class UIHitShape : public UIObject
{
    UI_CLASS(UIHitShape, UIObject)

public:
    // x, y in normalized space 0..1
    virtual bool IsOverlap(struct UIWidgetGeometry const& geometry, float x, float y) const
    {
        return false;
    }
};

class UIHitPolygon : public UIHitShape
{
    UI_CLASS(UIHitPolygon, UIHitShape)

public:
    TVector<Float2> Vertices;

    UIHitPolygon() = default;
    UIHitPolygon(TVector<Float2> vertices) :
        Vertices(std::move(vertices))
    {}

    // x, y in normalized space 0..1
    bool IsOverlap(struct UIWidgetGeometry const& geometry, float x, float y) const override;
};

class UIHitRect : public UIHitShape
{
    UI_CLASS(UIHitRect, UIHitShape)

public:
    Float2 Mins;
    Float2 Maxs;

    UIHitRect(Float2 const& mins, Float2 const& maxs) :
        Mins(mins),
        Maxs(maxs)
    {}

    // x, y in normalized space 0..1
    bool IsOverlap(struct UIWidgetGeometry const& geometry, float x, float y) const override;
};

class UIHitImage : public UIHitShape
{
    UI_CLASS(UIHitImage, UIHitShape)

private:
    TBitMask<> m_BitMask;
    uint32_t   m_Width{};
    uint32_t   m_Height{};

public:
    void SetImage(uint8_t const* data, uint32_t w, uint32_t h, uint32_t bpp, uint32_t rowPitch, uint32_t alphaChan);

    // x, y in normalized space 0..1
    bool IsOverlap(struct UIWidgetGeometry const& geometry, float x, float y) const override;
};

HK_NAMESPACE_END
