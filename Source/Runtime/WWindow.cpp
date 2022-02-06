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

#include "WWindow.h"
#include "WDesktop.h"
#include "ResourceManager.h"

HK_CLASS_META(WWindow)

WWindow::WWindow()
{
    CaptionHeight           = 24;
    TextColor               = Color4::White();
    TextOffset              = Float2(0.0f);
    TextHorizontalAlignment = WIDGET_ALIGNMENT_CENTER;
    TextVerticalAlignment   = WIDGET_ALIGNMENT_CENTER;
    CaptionColor            = Color4(0.1f, 0.4f, 0.8f);
    CaptionColorNotActive   = Color4(0.15f, 0.15f, 0.15f);
    BorderColor             = Color4(1, 1, 1, 0.5f);
    RoundingCorners         = CORNER_ROUND_TOP;
    BorderRounding          = 8;
    BorderThickness         = 2;
    bWindowBorder           = true;
    bCaptionBorder          = true;
    BgColor                 = Color4(0); // transparent by default
    bWordWrap               = false;
    UpdateDragShape();
    UpdateMargin();
}

WWindow::~WWindow()
{
}

WWindow& WWindow::SetCaptionText(const char* _CaptionText)
{
    CaptionText = _CaptionText;
    return *this;
}

WWindow& WWindow::SetCaptionHeight(float _CaptionHeight)
{
    CaptionHeight = _CaptionHeight;
    UpdateDragShape();
    UpdateMargin();
    return *this;
}

WWindow& WWindow::SetCaptionFont(AFont* _Font)
{
    Font = _Font;
    return *this;
}

WWindow& WWindow::SetTextColor(Color4 const& _Color)
{
    TextColor = _Color;
    return *this;
}

WWindow& WWindow::SetTextHorizontalAlignment(EWidgetAlignment _Alignment)
{
    TextHorizontalAlignment = _Alignment;
    return *this;
}

WWindow& WWindow::SetTextVerticalAlignment(EWidgetAlignment _Alignment)
{
    TextVerticalAlignment = _Alignment;
    return *this;
}

WWindow& WWindow::SetWordWrap(bool _WordWrap)
{
    bWordWrap = _WordWrap;
    return *this;
}

WWindow& WWindow::SetTextOffset(Float2 const& _Offset)
{
    TextOffset = _Offset;
    return *this;
}

WWindow& WWindow::SetCaptionColor(Color4 const& _Color)
{
    CaptionColor = _Color;
    return *this;
}

WWindow& WWindow::SetCaptionColorNotActive(Color4 const& _Color)
{
    CaptionColorNotActive = _Color;
    return *this;
}

WWindow& WWindow::SetBorderColor(Color4 const& _Color)
{
    BorderColor = _Color;
    return *this;
}

WWindow& WWindow::SetBorderThickness(float _Thickness)
{
    BorderThickness = _Thickness;
    UpdateMargin();
    return *this;
}

WWindow& WWindow::SetBackgroundColor(Color4 const& _Color)
{
    BgColor = _Color;
    return *this;
}

WWindow& WWindow::SetRounding(float _Rounding)
{
    BorderRounding = _Rounding;
    return *this;
}

WWindow& WWindow::SetRoundingCorners(EDrawCornerFlags _RoundingCorners)
{
    RoundingCorners = _RoundingCorners;
    return *this;
}

void WWindow::UpdateDragShape()
{
    Float2 vertices[4];

    float w = GetCurrentSize().X;

    vertices[0] = Float2(0, 0);
    vertices[1] = Float2(w, 0);
    vertices[2] = Float2(w, CaptionHeight);
    vertices[3] = Float2(0, CaptionHeight);

    SetDragShape(vertices, 4);
}

void WWindow::UpdateMargin()
{
    Float4 margin;
    margin.X = BorderThickness;
    margin.Y = CaptionHeight;
    margin.Z = BorderThickness;
    margin.W = BorderThickness;
    SetMargin(margin);
}

void WWindow::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateDragShape();
}

Float2 WWindow::GetTextPositionWithAlignment(ACanvas& _Canvas) const
{
    Float2 pos;

    const float width  = GetCurrentSize().X;
    const float height = CaptionHeight;

    AFont const* font = GetFont();
    Float2       size = font->CalcTextSizeA(font->GetFontSize(), width, bWordWrap ? width : 0.0f, CaptionText.Begin(), CaptionText.End());

    if (TextHorizontalAlignment == WIDGET_ALIGNMENT_LEFT)
    {
        pos.X = 0;
    }
    else if (TextHorizontalAlignment == WIDGET_ALIGNMENT_RIGHT)
    {
        pos.X = width - size.X;
    }
    else if (TextHorizontalAlignment == WIDGET_ALIGNMENT_CENTER)
    {
        float center = width * 0.5f;
        pos.X        = center - size.X * 0.5f;
    }
    else
    {
        pos.X = TextOffset.X;
    }

    if (TextVerticalAlignment == WIDGET_ALIGNMENT_TOP)
    {
        pos.Y = 0;
    }
    else if (TextVerticalAlignment == WIDGET_ALIGNMENT_BOTTOM)
    {
        pos.Y = height - size.Y;
    }
    else if (TextVerticalAlignment == WIDGET_ALIGNMENT_CENTER)
    {
        float center = height * 0.5f;
        pos.Y        = center - size.Y * 0.5f;
    }
    else
    {
        pos.Y = TextOffset.Y;
    }

    // WIDGET_ALIGNMENT_STRETCH is not handled for text

    return pos;
}

AFont const* WWindow::GetFont() const
{
    return Font ? Font : ACanvas::GetDefaultFont();
}

void WWindow::OnDrawEvent(ACanvas& _Canvas)
{
    Float2 mins, maxs;

    GetDesktopRect(mins, maxs, false);

    if (!BgColor.IsTransparent())
    {

        AWidgetShape const& windowShape = GetShape();

        int bgCorners = 0;
        if (!IsMaximized())
        {
            if (RoundingCorners & CORNER_ROUND_BOTTOM_LEFT)
            {
                bgCorners |= CORNER_ROUND_BOTTOM_LEFT;
            }
            if (RoundingCorners & CORNER_ROUND_BOTTOM_RIGHT)
            {
                bgCorners |= CORNER_ROUND_BOTTOM_RIGHT;
            }
        }

        if (windowShape.IsEmpty())
        {

            _Canvas.DrawRectFilled(mins + Float2(0, CaptionHeight), maxs, BgColor, BorderRounding, bgCorners);
        }
        else
        {

            // TODO: Draw triangulated concave polygon

            _Canvas.DrawRectFilled(mins + Float2(0, CaptionHeight), maxs, BgColor, BorderRounding, bgCorners);
        }
    }

    Super::OnDrawEvent(_Canvas);

    // Draw border
    if (bWindowBorder)
    {

        AWidgetShape const& windowShape = GetShape();

        if (windowShape.IsEmpty())
        {
            _Canvas.DrawRect(mins, maxs, BorderColor, BorderRounding, !IsMaximized() ? RoundingCorners : 0, BorderThickness);
        }
        else
        {
            _Canvas.DrawPolyline(windowShape.ToPtr(), windowShape.Size(), BorderColor, false, BorderThickness);
        }
    }

    // Draw caption
    if (CaptionHeight > 0.0f)
    {
        float  width       = GetCurrentSize().X;
        Float2 captionSize = Float2(width, CaptionHeight);

        int captionCorners = 0;
        if (!IsMaximized())
        {
            if (RoundingCorners & CORNER_ROUND_TOP_LEFT)
            {
                captionCorners |= CORNER_ROUND_TOP_LEFT;
            }
            if (RoundingCorners & CORNER_ROUND_TOP_RIGHT)
            {
                captionCorners |= CORNER_ROUND_TOP_RIGHT;
            }
        }

        WWidget* focus = GetDesktop()->GetFocusWidget();
        for (; focus && focus != this; focus = focus->GetParent()) {}

        // Draw caption
        if (focus)
        {
            _Canvas.DrawRectFilled(mins, mins + captionSize, CaptionColor, BorderRounding, captionCorners);
        }
        else
        {
            _Canvas.DrawRectFilled(mins, mins + captionSize, CaptionColorNotActive, BorderRounding, captionCorners);
        }

        // Draw caption border
        if (bCaptionBorder)
        {
            _Canvas.DrawRect(mins, mins + captionSize, BorderColor, BorderRounding, captionCorners, BorderThickness);
        }

        // Draw caption text
        if (!CaptionText.IsEmpty())
        {
            AFont const* font = GetFont();
            _Canvas.PushFont(font);
            _Canvas.PushClipRect(mins, mins + captionSize, true);
            _Canvas.DrawTextUTF8(font->GetFontSize(), mins + GetTextPositionWithAlignment(_Canvas), TextColor, CaptionText.Begin(), CaptionText.End(), bWordWrap ? width : 0.0f);
            _Canvas.PopClipRect();
            _Canvas.PopFont();
        }
    }
}
