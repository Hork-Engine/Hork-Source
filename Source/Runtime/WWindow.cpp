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
    BorderRounding.RoundingTL = 8;
    BorderRounding.RoundingTR = 8;
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

WWindow& WWindow::SetCaptionFontSize(float _FontSize)
{
    FontSize = _FontSize;
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

WWindow& WWindow::SetRounding(RoundingDesc const& _Rounding)
{
    BorderRounding = _Rounding;
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

    SetDragShape(vertices);
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

Float2 WWindow::GetTextPositionWithAlignment(TextBounds const& textBounds) const
{
    Float2 pos;

    const float width  = GetCurrentSize().X;
    const float height = CaptionHeight;

    Float2 size(textBounds.Width(), textBounds.Height());

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

AFont* WWindow::GetFont() const
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

        if (windowShape.IsEmpty())
        {
            RoundingDesc backgroundRounding;

            if (!IsMaximized())
            {
                backgroundRounding.RoundingBL = BorderRounding.RoundingBL;
                backgroundRounding.RoundingBR = BorderRounding.RoundingBR;
            }

            _Canvas.DrawRectFilled(mins + Float2(0, CaptionHeight), maxs, BgColor, backgroundRounding);
        }
        else
        {
            _Canvas.DrawPolyFilled(windowShape.ToPtr(), windowShape.Size(), BgColor);
        }
    }

    Super::OnDrawEvent(_Canvas);

    // Draw border
    if (bWindowBorder)
    {
        AWidgetShape const& windowShape = GetShape();

        if (windowShape.IsEmpty())
        {
            if (IsMaximized())
            {
                _Canvas.DrawRect(mins, maxs, BorderColor, BorderThickness);
            }
            else
            {
                _Canvas.DrawRect(mins, maxs, BorderColor, BorderThickness, BorderRounding);
            }
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

        RoundingDesc captionRounding;
        if (!IsMaximized())
        {
            captionRounding.RoundingTL = BorderRounding.RoundingTL;
            captionRounding.RoundingTR = BorderRounding.RoundingTR;
        }

        WWidget* focus = GetDesktop()->GetFocusWidget();
        for (; focus && focus != this; focus = focus->GetParent()) {}

        // Draw caption
        if (focus)
        {
            _Canvas.DrawRectFilled(mins, mins + captionSize, CaptionColor, captionRounding);
        }
        else
        {
            _Canvas.DrawRectFilled(mins, mins + captionSize, CaptionColorNotActive, captionRounding);
        }

        // Draw caption border
        if (bCaptionBorder)
        {
            _Canvas.DrawRect(mins, mins + captionSize, BorderColor, BorderThickness, captionRounding);
        }

        // Draw caption text
        if (!CaptionText.IsEmpty())
        {
            AFont* font = GetFont();
            _Canvas.FontFace(font);
            _Canvas.FontSize(FontSize);
            TextBounds textBounds;
            _Canvas.GetTextBoxBounds(0, 0, width, CaptionText, textBounds);
            Float2 offset = GetTextPositionWithAlignment(textBounds);
            _Canvas.IntersectScissor(mins, mins + captionSize);
            if (bWordWrap)
                _Canvas.DrawTextWrapUTF8(mins + offset, TextColor, CaptionText, width);
            else
                _Canvas.DrawTextUTF8(mins + offset, TextColor, CaptionText);
        }
    }
}
