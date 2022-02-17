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

#include "WScroll.h"
#include "WDesktop.h"
#include "FrameLoop.h"

HK_CLASS_META(WScroll)

WScroll::WScroll()
{
    bAutoScrollH          = true;
    bAutoScrollV          = true;
    bShowButtons          = false;
    bUpdateGeometry       = true;
    ScrollbarSize         = 12;
    ButtonWidth           = 0.0f;
    SliderRounding        = 0.0f;
    BackgroundColor       = Color4(0.05f, 0.05f, 0.05f);
    ButtonColor           = Color4(1, 0, 1, 1);
    SliderBackgroundColor = Color4(0.4f, 0.4f, 0.4f);
    SliderColor           = Color4(1, 1, 1, 1);
    Action                = A_NONE;
    DragCursor            = 0.0f;
    UpdateMargin();
}

WScroll::~WScroll()
{
}

WScroll& WScroll::SetContentWidget(WWidget* _Content)
{
    if (_Content == this || IsSame(_Content, Content))
    {
        return *this;
    }

    if (Content && Content->GetParent() == this)
    {
        Content->Unparent();
    }

    Content = _Content;

    if (Content)
    {
        Content->SetParent(this);
    }

    bUpdateGeometry = true;
    UpdateMargin();

    return *this;
}

WScroll& WScroll::SetContentWidget(WWidget& _Content)
{
    return SetContentWidget(&_Content);
}

WWidget* WScroll::GetContentWidget()
{
    return Content;
}

void WScroll::UpdateMargin()
{
    Float2 contentSize = Content ? Content->GetCurrentSize() : Float2(0.0f);

    Float2 viewSize = GetCurrentSize();

    if (!bAutoScrollH)
    {
        viewSize.Y -= ScrollbarSize;
    }
    if (!bAutoScrollV)
    {
        viewSize.X -= ScrollbarSize;
    }

    Float4 newMargin(0.0f);

    if (bAutoScrollH)
    {
        if (contentSize.X > viewSize.X)
        {
            newMargin.W = ScrollbarSize;
            viewSize.Y -= ScrollbarSize;
        }
    }
    else
    {
        newMargin.W = ScrollbarSize;
    }

    if (bAutoScrollV)
    {
        if (contentSize.Y > viewSize.Y)
        {
            newMargin.Z = ScrollbarSize;
            viewSize.X -= ScrollbarSize;
            if (bAutoScrollH)
            {
                if (contentSize.X > viewSize.X)
                {
                    newMargin.W = ScrollbarSize;
                }
            }
        }
    }
    else
    {
        newMargin.Z = ScrollbarSize;
    }

    if (GetMargin() != newMargin)
    {
        SetMargin(newMargin);
        bUpdateGeometry = true;
    }
}

WScroll& WScroll::SetAutoScrollH(bool _AutoScroll)
{
    if (bAutoScrollH != _AutoScroll)
    {
        bAutoScrollH = _AutoScroll;

        UpdateMargin();
    }
    return *this;
}

WScroll& WScroll::SetAutoScrollV(bool _AutoScroll)
{
    if (bAutoScrollV != _AutoScroll)
    {
        bAutoScrollV = _AutoScroll;

        UpdateMargin();
    }
    return *this;
}

WScroll& WScroll::SetScrollbarSize(float _Size)
{
    ScrollbarSize = _Size;
    if (ScrollbarSize < 0.0f)
    {
        ScrollbarSize = 0.0f;
    }
    UpdateMargin();
    return *this;
}

WScroll& WScroll::SetButtonWidth(float _Width)
{
    ButtonWidth = _Width;
    if (ButtonWidth < 0.0f)
    {
        ButtonWidth = 0.0f;
    }
    bUpdateGeometry = true;
    return *this;
}

WScroll& WScroll::SetShowButtons(bool _ShowButtons)
{
    bShowButtons = _ShowButtons;
    MarkTransformDirty();
    return *this;
}

WScroll& WScroll::SetSliderRounding(float _Rounding)
{
    SliderRounding = _Rounding;
    return *this;
}

WScroll& WScroll::SetBackgroundColor(Color4 const& _Color)
{
    BackgroundColor = _Color;
    return *this;
}

WScroll& WScroll::SetButtonColor(Color4 const& _Color)
{
    ButtonColor = _Color;
    return *this;
}

WScroll& WScroll::SetSliderBackgroundColor(Color4 const& _Color)
{
    SliderBackgroundColor = _Color;
    return *this;
}

WScroll& WScroll::SetSliderColor(Color4 const& _Color)
{
    SliderColor = _Color;
    return *this;
}

void WScroll::UpdateScrollbarGeometry()
{
    bUpdateGeometry = false;

    Float2 mins, maxs;

    GetDesktopRect(mins, maxs, false);

    Float4 const& margin = GetMargin();

    Platform::ZeroMem(&Geometry, sizeof(Geometry));

    Geometry.bDrawHScrollbar = margin.W > 0.0f;
    Geometry.bDrawVScrollbar = margin.Z > 0.0f;

    Geometry.ContentSize     = Content ? Content->GetCurrentSize() : Float2(0.0f);
    Geometry.ContentPosition = Content ? Content->GetPosition() : Float2(0.0f);
    Geometry.ViewSize        = GetAvailableSize();

    const float SliderMargin = 2;

    if (Geometry.bDrawHScrollbar)
    {
        Geometry.HScrollbarMins.X = mins.X;
        Geometry.HScrollbarMins.Y = maxs.Y - margin.W;

        Geometry.HScrollbarMaxs.X = maxs.X - margin.Z;
        Geometry.HScrollbarMaxs.Y = maxs.Y;

        if (bShowButtons)
        {
            Float2 ButtonSize(ButtonWidth, ScrollbarSize);

            Geometry.LeftButtonMins = Geometry.HScrollbarMins;
            Geometry.LeftButtonMaxs = Geometry.HScrollbarMins + ButtonSize;

            Geometry.RightButtonMins = Geometry.HScrollbarMaxs - ButtonSize;
            Geometry.RightButtonMaxs = Geometry.HScrollbarMaxs;

            Geometry.HSliderBgMins.X = Geometry.LeftButtonMaxs.X;
            Geometry.HSliderBgMins.Y = Geometry.LeftButtonMins.Y;

            Geometry.HSliderBgMaxs.X = Geometry.RightButtonMins.X;
            Geometry.HSliderBgMaxs.Y = Geometry.RightButtonMaxs.Y;
        }
        else
        {
            Geometry.HSliderBgMins = Geometry.HScrollbarMins;
            Geometry.HSliderBgMaxs = Geometry.HScrollbarMaxs;
        }

        float SliderPos  = 0.0f;
        float SliderSize = 0.0f;

        if (Geometry.ViewSize.X > 0)
        {
            if (Geometry.ViewSize.X >= Geometry.ContentSize.X || Geometry.ContentSize.X <= 0.0f)
            {
                Geometry.ContentPosition.X = 0;
                SliderSize                 = 1;
            }
            else
            {
                float MinPos = -Geometry.ContentSize.X + Geometry.ViewSize.X;

                Geometry.ContentPosition.X = Math::Clamp(Geometry.ContentPosition.X, MinPos, 0.0f);

                SliderPos  = -Geometry.ContentPosition.X / Geometry.ContentSize.X;
                SliderSize = Geometry.ViewSize.X / Geometry.ContentSize.X;
            }
        }

        float SliderBgSize = Math::Max(0.0f, Geometry.HSliderBgMaxs.X - Geometry.HSliderBgMins.X);

        SliderPos *= SliderBgSize;
        SliderSize *= SliderBgSize;

        Geometry.HSliderMins.X = Geometry.HSliderBgMins.X + SliderPos;
        Geometry.HSliderMins.Y = Geometry.HSliderBgMins.Y + SliderMargin;

        Geometry.HSliderMaxs.X = Geometry.HSliderMins.X + SliderSize;
        Geometry.HSliderMaxs.Y = Geometry.HSliderBgMaxs.Y - SliderMargin;
    }

    if (Geometry.bDrawVScrollbar)
    {
        Geometry.VScrollbarMins.X = maxs.X - margin.Z;
        Geometry.VScrollbarMins.Y = mins.Y;

        Geometry.VScrollbarMaxs.X = maxs.X;
        Geometry.VScrollbarMaxs.Y = maxs.Y - margin.W;

        if (bShowButtons)
        {
            Float2 ButtonSize(ScrollbarSize, ButtonWidth);

            Geometry.UpButtonMins = Geometry.VScrollbarMins;
            Geometry.UpButtonMaxs = Geometry.VScrollbarMins + ButtonSize;

            Geometry.DownButtonMins = Geometry.VScrollbarMaxs - ButtonSize;
            Geometry.DownButtonMaxs = Geometry.VScrollbarMaxs;

            Geometry.VSliderBgMins.X = Geometry.UpButtonMins.X;
            Geometry.VSliderBgMins.Y = Geometry.UpButtonMaxs.Y;

            Geometry.VSliderBgMaxs.X = Geometry.DownButtonMaxs.X;
            Geometry.VSliderBgMaxs.Y = Geometry.DownButtonMins.Y;
        }
        else
        {
            Geometry.VSliderBgMins = Geometry.VScrollbarMins;
            Geometry.VSliderBgMaxs = Geometry.VScrollbarMaxs;
        }

        float SliderPos  = 0.0f;
        float SliderSize = 0.0f;

        if (Geometry.ViewSize.Y > 0)
        {
            if (Geometry.ViewSize.Y >= Geometry.ContentSize.Y || Geometry.ContentSize.Y <= 0.0f)
            {
                Geometry.ContentPosition.Y = 0;
                SliderSize                 = 1;
            }
            else
            {
                float MinPos = -Geometry.ContentSize.Y + Geometry.ViewSize.Y;

                Geometry.ContentPosition.Y = Math::Clamp(Geometry.ContentPosition.Y, MinPos, 0.0f);

                SliderPos  = -Geometry.ContentPosition.Y / Geometry.ContentSize.Y;
                SliderSize = Geometry.ViewSize.Y / Geometry.ContentSize.Y;
            }
        }

        float SliderBgSize = Math::Max(0.0f, Geometry.VSliderBgMaxs.Y - Geometry.VSliderBgMins.Y);

        SliderPos *= SliderBgSize;
        SliderSize *= SliderBgSize;

        Geometry.VSliderMins.X = Geometry.VSliderBgMins.X + SliderMargin;
        Geometry.VSliderMins.Y = Geometry.VSliderBgMins.Y + SliderPos;

        Geometry.VSliderMaxs.X = Geometry.VSliderBgMaxs.X - SliderMargin;
        Geometry.VSliderMaxs.Y = Geometry.VSliderMins.Y + SliderSize;
    }
}

void WScroll::UpdateScrollbarGeometryIfDirty()
{
    if (bUpdateGeometry)
    {
        UpdateScrollbarGeometry();
    }
}

SScrollbarGeometry const& WScroll::GetScrollbarGeometry() const
{
    const_cast<WScroll*>(this)->UpdateScrollbarGeometryIfDirty();

    return Geometry;
}

void WScroll::OnTransformDirty()
{
    Super::OnTransformDirty();

    //UpdateMargin();
    bUpdateGeometry = true;
}

void WScroll::MoveHSlider(float Vec)
{
    if (!Content)
    {
        return;
    }

    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    float SliderBarSize = geometry.HSliderBgMaxs.X - geometry.HSliderBgMins.X;

    float ContentPos = -Vec * geometry.ContentSize.X / SliderBarSize;
    ContentPos       = Math::Min(ContentPos, 0.0f);

    float MinPos = -geometry.ContentSize.X + geometry.ViewSize.X;
    ContentPos   = Math::Max(ContentPos, MinPos);

    Float2 pos = geometry.ContentPosition;
    pos.X      = ContentPos;

    Content->SetPosition(pos);

    bUpdateGeometry = true;
}

void WScroll::MoveVSlider(float Vec)
{
    if (!Content)
    {
        return;
    }

    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    float SliderBarSize = geometry.VSliderBgMaxs.Y - geometry.VSliderBgMins.Y;

    float ContentPos = -Vec * geometry.ContentSize.Y / SliderBarSize;
    ContentPos       = Math::Min(ContentPos, 0.0f);

    float MinPos = -geometry.ContentSize.Y + geometry.ViewSize.Y;
    ContentPos   = Math::Max(ContentPos, MinPos);

    Float2 pos = geometry.ContentPosition;
    pos.Y      = ContentPos;

    Content->SetPosition(pos);

    bUpdateGeometry = true;
}

void WScroll::ScrollDelta(Float2 const& _Delta)
{
    if (!Content)
    {
        return;
    }

    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    SetScrollPosition(geometry.ContentPosition + _Delta);
}

void WScroll::SetScrollPosition(Float2 const& _Position)
{
    if (!Content)
    {
        return;
    }

    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    Float2 ContentPos = _Position;

    ContentPos.X = Math::Max(ContentPos.X, -geometry.ContentSize.X + geometry.ViewSize.X);
    ContentPos.Y = Math::Max(ContentPos.Y, -geometry.ContentSize.Y + geometry.ViewSize.Y);
    ContentPos.X = Math::Min(ContentPos.X, 0.0f);
    ContentPos.Y = Math::Min(ContentPos.Y, 0.0f);

    if (geometry.ContentPosition.X != ContentPos.X || geometry.ContentPosition.Y != ContentPos.Y)
    {
        Content->SetPosition(ContentPos);

        bUpdateGeometry = true;
    }
}

Float2 WScroll::GetScrollPosition() const
{
    if (!Content)
    {
        return Float2(0.0f);
    }

    SScrollbarGeometry const& geometry = GetScrollbarGeometry();
    return geometry.ContentPosition;
}

HK_FORCEINLINE bool InRect(Float2 const& _Mins, Float2 const& _Maxs, Float2 const& _Position)
{
    return _Position.X >= _Mins.X && _Position.X < _Maxs.X && _Position.Y >= _Mins.Y && _Position.Y < _Maxs.Y;
}

void WScroll::OnMouseButtonEvent(SMouseButtonEvent const& _Event, double _TimeStamp)
{
    Action = A_NONE;

    if (_Event.Action != IA_PRESS)
    {

        if (Content)
        {
            Content->SetFocus();
        }

        return;
    }

    if (!Content)
    {
        return;
    }

    Float2 const& CursorPos = GetDesktop()->GetCursorPosition();

    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    if (geometry.bDrawHScrollbar && geometry.ContentSize.X > geometry.ViewSize.X)
    {

        if (InRect(geometry.LeftButtonMins, geometry.LeftButtonMaxs, CursorPos))
        {
            Action = A_SCROLL_LEFT;
            return;
        }

        if (InRect(geometry.RightButtonMins, geometry.RightButtonMaxs, CursorPos))
        {
            Action = A_SCROLL_RIGHT;
            return;
        }

        if (InRect(geometry.HSliderMins, geometry.HSliderMaxs, CursorPos))
        {
            Action = A_SCROLL_HSLIDER;

            float SliderBarSize = geometry.HSliderBgMaxs.X - geometry.HSliderBgMins.X;

            DragCursor = CursorPos.X + geometry.ContentPosition.X / geometry.ContentSize.X * SliderBarSize;
            return;
        }

        if (InRect(geometry.HSliderBgMins, geometry.HSliderBgMaxs, CursorPos))
        {
            float CursorLocalOffset = CursorPos.X - geometry.HSliderBgMins.X;

            float SliderSize = geometry.HSliderMaxs.X - geometry.HSliderMins.X;

            float Vec = CursorLocalOffset - SliderSize * 0.5f;

            MoveHSlider(Vec);
            return;
        }
    }

    if (geometry.bDrawVScrollbar && geometry.ContentSize.Y > geometry.ViewSize.Y)
    {

        if (InRect(geometry.UpButtonMins, geometry.UpButtonMaxs, CursorPos))
        {
            Action = A_SCROLL_UP;
            return;
        }

        if (InRect(geometry.DownButtonMins, geometry.DownButtonMaxs, CursorPos))
        {
            Action = A_SCROLL_DOWN;
            return;
        }

        if (InRect(geometry.VSliderMins, geometry.VSliderMaxs, CursorPos))
        {
            Action = A_SCROLL_VSLIDER;

            float SliderBarSize = geometry.VSliderBgMaxs.Y - geometry.VSliderBgMins.Y;

            DragCursor = CursorPos.Y + geometry.ContentPosition.Y / geometry.ContentSize.Y * SliderBarSize;
            return;
        }

        if (InRect(geometry.VSliderBgMins, geometry.VSliderBgMaxs, CursorPos))
        {
            float CursorLocalOffset = CursorPos.Y - geometry.VSliderBgMins.Y;

            float SliderSize = geometry.VSliderMaxs.Y - geometry.VSliderMins.Y;

            float Vec = CursorLocalOffset - SliderSize * 0.5f;

            MoveVSlider(Vec);
            return;
        }
    }
}

void WScroll::OnMouseMoveEvent(struct SMouseMoveEvent const& _Event, double _TimeStamp)
{

    if (Action == A_SCROLL_HSLIDER)
    {
        Float2 CursorPos = GetDesktop()->GetCursorPosition();

        float Vec = CursorPos.X - DragCursor;

        MoveHSlider(Vec);
    }

    else if (Action == A_SCROLL_VSLIDER)
    {
        Float2 CursorPos = GetDesktop()->GetCursorPosition();

        float Vec = CursorPos.Y - DragCursor;

        MoveVSlider(Vec);
    }
}

void WScroll::UpdateScrolling(float _TimeStep)
{
    const float ScrollSpeed = _TimeStep;

    switch (Action)
    {
        case A_SCROLL_LEFT:
            ScrollDelta(Float2(ScrollSpeed, 0.0f));
            break;
        case A_SCROLL_RIGHT:
            ScrollDelta(Float2(-ScrollSpeed, 0.0f));
            break;
        case A_SCROLL_UP:
            ScrollDelta(Float2(0.0f, ScrollSpeed));
            break;
        case A_SCROLL_DOWN:
            ScrollDelta(Float2(0.0f, -ScrollSpeed));
            break;
        default:
            // keep up to date
            ScrollDelta(Float2(0.0f));
            break;
    }
}

void WScroll::Update(float _TimeStep)
{
    UpdateScrolling(_TimeStep);
    UpdateMargin();
}

void WScroll::OnFocusReceive()
{
    //    if ( Content ) {
    //        Content->SetFocus();
    //    }
}

void WScroll::OnDrawEvent(ACanvas& _Canvas)
{
    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    Update(1); // TODO: move it to Tick()

    DrawDecorates(_Canvas);

    if (BackgroundColor.GetAlpha() > 0.0f)
    {
        Float2 bgMins, bgMaxs;
        GetDesktopRect(bgMins, bgMaxs, true);
        _Canvas.DrawRectFilled(bgMins, bgMaxs, BackgroundColor);
    }

    if (geometry.bDrawHScrollbar)
    {
        if (bShowButtons)
        {
            // Left button
            if (geometry.LeftButtonMaxs.X > geometry.LeftButtonMins.X && geometry.LeftButtonMaxs.Y > geometry.LeftButtonMins.Y)
            {
                _Canvas.DrawRect(geometry.LeftButtonMins, geometry.LeftButtonMaxs, ButtonColor);
            }

            // Right button
            if (geometry.RightButtonMaxs.X > geometry.RightButtonMins.X && geometry.RightButtonMaxs.Y > geometry.RightButtonMins.Y)
            {
                _Canvas.DrawRect(geometry.RightButtonMins, geometry.RightButtonMaxs, ButtonColor);
            }
        }

        // Draw slider background
        if (geometry.HSliderBgMaxs.X > geometry.HSliderBgMins.X && geometry.HSliderBgMaxs.Y > geometry.HSliderBgMins.Y)
        {
            _Canvas.DrawRectFilled(geometry.HSliderBgMins, geometry.HSliderBgMaxs, SliderBackgroundColor);
        }

        // Draw slider
        if (geometry.HSliderMaxs.X > geometry.HSliderMins.X && geometry.HSliderMaxs.Y > geometry.HSliderMins.Y)
        {
            _Canvas.DrawRectFilled(geometry.HSliderMins, geometry.HSliderMaxs, SliderColor, SliderRounding);
        }
    }

    if (geometry.bDrawVScrollbar)
    {
        if (bShowButtons)
        {
            // Up button
            if (geometry.UpButtonMaxs.X > geometry.UpButtonMins.X && geometry.UpButtonMaxs.Y > geometry.UpButtonMins.Y)
            {
                _Canvas.DrawRect(geometry.UpButtonMins, geometry.UpButtonMaxs, ButtonColor);
            }

            // Down button
            if (geometry.DownButtonMaxs.X > geometry.DownButtonMins.X && geometry.DownButtonMaxs.Y > geometry.DownButtonMins.Y)
            {
                _Canvas.DrawRect(geometry.DownButtonMins, geometry.DownButtonMaxs, ButtonColor);
            }
        }

        // Draw slider background
        if (geometry.VSliderBgMaxs.X > geometry.VSliderBgMins.X && geometry.VSliderBgMaxs.Y > geometry.VSliderBgMins.Y)
        {
            _Canvas.DrawRectFilled(geometry.VSliderBgMins, geometry.VSliderBgMaxs, SliderBackgroundColor);
        }

        // Draw slider
        if (geometry.VSliderMaxs.X > geometry.VSliderMins.X && geometry.VSliderMaxs.Y > geometry.VSliderMins.Y)
        {
            _Canvas.DrawRectFilled(geometry.VSliderMins, geometry.VSliderMaxs, SliderColor, SliderRounding);
        }
    }
}

void WScroll::ScrollHome()
{
    SetScrollPosition(Float2(0.0f, 0.0f));
}

void WScroll::ScrollEnd()
{
    if (!Content)
    {
        return;
    }

    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    Float2 ContentPos(0.0f, -geometry.ContentSize.Y + geometry.ViewSize.Y);

    SetScrollPosition(ContentPos);
}

bool WScroll::IsVerticalScrollAllowed() const
{
    SScrollbarGeometry const& geometry = GetScrollbarGeometry();

    return geometry.ContentSize.Y > geometry.ViewSize.Y;
}
