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

#include "UIHitShape.h"
#include "UIWidget.h"

#include <Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

bool UIHitPolygon::IsOverlap(UIWidgetGeometry const& geometry, float x, float y) const
{
    x -= geometry.Mins.X;
    y -= geometry.Mins.Y;
    x /= geometry.Maxs.X - geometry.Mins.X;
    y /= geometry.Maxs.Y - geometry.Mins.Y;

    return BvPointInPoly2D(Vertices.ToPtr(), Vertices.Size(), x, y);
}

bool UIHitRect::IsOverlap(UIWidgetGeometry const& geometry, float x, float y) const
{
    x -= geometry.Mins.X;
    y -= geometry.Mins.Y;

    return BvPointInRect(Mins, Maxs, x, y);
}

void UIHitImage::SetImage(uint8_t const* data, uint32_t w, uint32_t h, uint32_t bpp, uint32_t rowPitch, uint32_t alphaChan)
{
    int bitNum = 0;
    uint8_t const* end = data + rowPitch * h;
    uint32_t count = w * bpp;

    m_BitMask.Clear();
    m_BitMask.Resize(w * h);

    while (data < end)
    {
        for (uint32_t i = alphaChan; i < count; i += bpp, bitNum++)
        {
            if (data[i])
                m_BitMask.Mark(bitNum);
            
        }
        data += rowPitch;
    }
}

bool UIHitImage::IsOverlap(UIWidgetGeometry const& geometry, float x, float y) const
{
    x -= geometry.Mins.X;
    y -= geometry.Mins.Y;
    x /= geometry.Maxs.X - geometry.Mins.X;
    y /= geometry.Maxs.Y - geometry.Mins.Y;

    uint32_t px = x * m_Width;
    uint32_t py = y * m_Height;
    if (px >= m_Width || py >= m_Height)
        return false;
    return m_BitMask.IsMarked(py * m_Width + px);
}

HK_NAMESPACE_END
