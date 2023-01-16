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

#include "UIWindow.h"
#include "UILabel.h"

#include <Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

UIWindow::UIWindow(UIWidget* caption, UIWidget* central) :
    m_Caption(caption ? caption : UINew(UIWidget)),
    m_Central(central ? central : UINew(UIWidget))
{
    Layout = UINew(WindowLayout, this);

    Padding = UIPadding(1);

    AddWidget(m_Caption);
    AddWidget(m_Central);
}

UIWindow::WindowLayout::WindowLayout(UIWindow* self) :
    Self(self)
{
}

Float2 UIWindow::WindowLayout::MeasureLayout(UIWidget* self, bool, bool, Float2 const& size)
{
    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    UIWidget* widgets[] = {Self->m_Caption, Self->m_Central};
    float   height[]  = {Self->CaptionHeight, Math::Max(0.0f, paddedSize.Y - Self->CaptionHeight)};

    for (int i = 0; i < 2; ++i)
    {
        UIWidget* w = widgets[i];

        if (w->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        w->MeasureLayout(false, false, {paddedSize.X, height[i]});
    }

    return paddedSize;
}

void UIWindow::WindowLayout::ArrangeChildren(UIWidget* self, bool, bool)
{
    UIWidgetGeometry& geometry = self->m_Geometry;

    float x = geometry.PaddedMins.X;
    float y = geometry.PaddedMins.Y;

    UIWidget* widgets[] = {Self->m_Caption, Self->m_Central};

    for (int i = 0; i < 2; ++i)
    {
        UIWidget* w = widgets[i];

        if (w->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        w->m_Geometry.Mins.X = x;
        w->m_Geometry.Mins.Y = y;

        w->m_Geometry.Maxs = w->m_Geometry.Mins + w->m_MeasuredSize;

        if ((w->m_Geometry.Mins.X >= geometry.PaddedMaxs.X) || (w->m_Geometry.Mins.Y >= geometry.PaddedMaxs.Y))
            continue;

        w->ArrangeChildren(false, false);

        y = w->m_Geometry.Maxs.Y;
    }
}

UIWindow::WINDOW_AREA UIWindow::HitTestWindowArea(float x, float y) const
{
    const Float2 resizeBoxSize = Float2(8, 8);
    const float  borderHitWidth = 4;
    Float2       boxMins;
    Float2       boxMaxs;

    boxMins = m_Geometry.Mins;
    boxMaxs = boxMins + resizeBoxSize;
    if (BvPointInRect(boxMins, boxMaxs, x, y))
        return WA_TOP_LEFT;

    boxMins = Float2(m_Geometry.Maxs.X - resizeBoxSize.X, m_Geometry.Mins.Y);
    boxMaxs = boxMins + resizeBoxSize;
    if (BvPointInRect(boxMins, boxMaxs, x, y))
        return WA_TOP_RIGHT;

    boxMins = Float2(m_Geometry.Mins.X, m_Geometry.Maxs.Y - resizeBoxSize.Y);
    boxMaxs = Float2(m_Geometry.Mins.X + resizeBoxSize.X, m_Geometry.Maxs.Y);
    if (BvPointInRect(boxMins, boxMaxs, x, y))
        return WA_BOTTOM_LEFT;

    boxMins = m_Geometry.Maxs - resizeBoxSize;
    boxMaxs = m_Geometry.Maxs;
    if (BvPointInRect(boxMins, boxMaxs, x, y))
        return WA_BOTTOM_RIGHT;

    if (x < borderHitWidth)
        return WA_LEFT;

    if (y < borderHitWidth)
        return WA_TOP;

    if (x > m_Geometry.Maxs.X - borderHitWidth)
        return WA_RIGHT;

    if (y > m_Geometry.Maxs.Y - borderHitWidth)
        return WA_BOTTOM;

    if (CaptionHitTest(x, y))
        return WA_CAPTION;

    if (!HitTest(x, y))
        return WA_NONE;

    if (BvPointInRect(m_Geometry.PaddedMins, m_Geometry.PaddedMaxs, x, y))
        return WA_CLIENT;

    return WA_BACK;
}

bool UIWindow::CaptionHitTest(float x, float y) const
{
    if (!BvPointInRect(m_Geometry.Mins, m_Geometry.Maxs, x, y))
        return false;

    if (CaptionHitShape)
    {
        return CaptionHitShape->IsOverlap(m_Geometry, x, y);
    }

    if (m_Caption)
        return BvPointInRect(m_Caption->m_Geometry.Mins, m_Caption->m_Geometry.Maxs, x, y);
    
    return false;
}

void UIWindow::Draw(Canvas& canvas)
{
    if (bDropShadow && WindowState != WS_MAXIMIZED)
    {
        CanvasPaint shadowPaint;
        float       cornerRadius = 3;

        canvas.Push(CANVAS_PUSH_FLAG_RESET);

        if (m_Parent)
            canvas.Scissor(m_Parent->m_Geometry.PaddedMins, m_Parent->m_Geometry.PaddedMaxs);

        RoundingDesc rounding(8,8,8,8);

        // Drop shadow
        float x = m_Geometry.Mins.X;
        float y = m_Geometry.Mins.Y;
        float w = m_Geometry.Maxs.X - m_Geometry.Mins.X;
        float h = m_Geometry.Maxs.Y - m_Geometry.Mins.Y;
        shadowPaint.BoxGradient({x, y + 2}, w, h, cornerRadius * 2, 10, MakeColorU8(0, 0, 0, 255), MakeColorU8(0, 0, 0, 0));
        canvas.BeginPath();
        canvas.Rect(x - 10, y - 10, w + 20, h + 30);
        canvas.RoundedRectVarying(x, y, w, h, rounding.RoundingTL, rounding.RoundingTR, rounding.RoundingBR, rounding.RoundingBL);
        canvas.PathWinding(CANVAS_PATH_WINDING_HOLE);
        canvas.FillPaint(shadowPaint);
        canvas.Fill();

        canvas.Pop();
    }
}

UIWindow* UIMakeWindow(StringView captionText, UIWidget* centralWidget)
{
    // TODO: Different caption for Active/Not active window

    auto label = UINew(UILabel)
                     .WithText(UINew(UIText, captionText)
                                   .WithFontSize(16)
                                   .WithWordWrap(true)
                                   .WithAlignment(TEXT_ALIGNMENT_HCENTER | TEXT_ALIGNMENT_VCENTER))
                     .WithBackground(UINew(UILinearGradient)
                                         .WithStartPoint({0, -5})
                                         .WithEndPoint({0, 10})
                                         .WithInnerColor({0.25f, 0.25f, 0.25f})
                                         .WithOuterColor({0.16f, 0.16f, 0.16f})
                                         .WithFilled(true)
                                         .WithRounding({8, 8, 0, 0}))
                     .WithNoInput(true);

    return UINew(UIWindow, label, centralWidget)
        .WithBackground(UINew(UISolidBrush, Color4(0.15f, 0.15f, 0.15f))
                            .WithFilled(true)
                            .WithRounding({8, 8, 0, 0}))
        .WithForeground(UINew(UISolidBrush, Color4(0.1f, 0.1f, 0.1f))
                            .WithFilled(false)
                            .WithRounding({8, 8, 0, 0}))
        .WithPadding(UIPadding(0));
}

HK_NAMESPACE_END
