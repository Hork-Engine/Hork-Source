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

#include "WDesktop.h"
#include "InputDefs.h"
#include "FrameLoop.h"
#include <Geometry/BV/BvIntersect.h>

#define DOUBLECLICKTIME_MSEC 250
#define DOUBLECLICKHALFSIZE  4.0f

HK_CLASS_META(WDesktop)

WDesktop::WDesktop() :
    FocusWidget(nullptr), MouseClickTime(0), MouseClickPos(0.0f), DraggingCursor(0.0f), DraggingWidgetPos(0.0f), CursorPosition(0.0f), Cursor(DRAW_CURSOR_ARROW), bCursorVisible(true), bDrawBackground(false)
{
    Root          = CreateInstanceOf<WWidget>();
    Root->Desktop = this;
    Root->SetMargin(0, 0, 0, 0);
}

WDesktop::~WDesktop()
{
    if (FocusWidget)
    {
        FocusWidget->bFocus = false;
        FocusWidget         = nullptr;
    }
    DraggingWidget   = nullptr;
    MouseClickWidget = nullptr;
    MouseFocusWidget = nullptr;

    ClosePopupMenu();

    Root = nullptr;
}

WDesktop& WDesktop::AddWidget(WWidget* _Widget)
{
    _Widget->SetParent(Root);
    if (_Widget->bSetFocusOnAddToDesktop)
    {
        _Widget->SetFocus();
    }
    return *this;
}

WDesktop& WDesktop::RemoveWidget(WWidget* _Widget)
{
    if (IsSame(_Widget->GetParent(), Root))
    {
        _Widget->Unparent();
    }
    return *this;
}

WDesktop& WDesktop::RemoveWidgets()
{
    Root->RemoveWidgets();
    return *this;
}

WDesktop& WDesktop::SetSize(float _Width, float _Height)
{
    return SetSize(Float2(_Width, _Height));
}

WDesktop& WDesktop::SetSize(Float2 const& _Size)
{
    Float2 sz = Root->GetSize();
    if (sz == _Size)
    {
        return *this;
    }
    Root->SetSize(_Size);
    return *this;
}

float WDesktop::GetWidth() const
{
    return Root->GetWidth();
}

float WDesktop::GetHeight() const
{
    return Root->GetHeight();
}

WDesktop& WDesktop::SetCursorVisible(bool _Visible)
{
    bCursorVisible = _Visible;
    return *this;
}

void WDesktop::OpenPopupMenu(WMenuPopup* _PopupMenu)
{
    OpenPopupMenuAt(_PopupMenu, CursorPosition);
}

void WDesktop::OpenPopupMenuAt(WMenuPopup* _PopupMenu, Float2 const& Position)
{
    CancelDragging();
    ClosePopupMenu();

    Popup = _PopupMenu;
    if (Popup)
    {
        AddWidget(Popup->Self);

        //Popup->Self->SetMaxSize( Root->GetWidth(), Math::Max( 1.0f, (Root->GetHeight()*0.9f - Position.Y) ) );
        Popup->Self->SetMaxSize(Root->GetWidth(), Math::Max(1.0f, (Root->GetHeight() - Position.Y)));

        Popup->Self->SetPosition(Position);
        Popup->Self->SetVisible();
        Popup->Self->SetFocus();
        Popup->Self->BringOnTop();
    }
}

void WDesktop::ClosePopupMenu()
{
    if (Popup)
    {
        RemoveWidget(Popup->Self);
        Popup->Self->SetInvisible();
        Popup = nullptr;
    }
}

HK_FORCEINLINE bool InRect(Float2 const& _Mins, Float2 const& _Maxs, Float2 const& _Position)
{
    return _Position.X >= _Mins.X && _Position.X < _Maxs.X && _Position.Y >= _Mins.Y && _Position.Y < _Maxs.Y;
}

HK_FORCEINLINE void ApplyMargins(Float2& _Mins, Float2& _Maxs, Float4 const& _Margins)
{
    _Mins.X += _Margins.X;
    _Mins.Y += _Margins.Y;
    _Maxs.X -= _Margins.Z;
    _Maxs.Y -= _Margins.W;
}

WWidget* WDesktop::GetWidgetUnderCursor_r(WWidget* _Widget, Float2 const& _ClipMins, Float2 const& _ClipMaxs, Float2 const& _Position)
{
    if (!_Widget->IsVisible())
    {
        return nullptr;
    }

    Float2 rectMins, rectMaxs;
    Float2 mins, maxs;

    _Widget->GetDesktopRect(rectMins, rectMaxs, false);

    mins.X = Math::Max(rectMins.X, _ClipMins.X);
    mins.Y = Math::Max(rectMins.Y, _ClipMins.Y);

    maxs.X = Math::Min(rectMaxs.X, _ClipMaxs.X);
    maxs.Y = Math::Min(rectMaxs.Y, _ClipMaxs.Y);

    if (mins.X >= maxs.X || mins.Y >= maxs.Y)
    {
        return nullptr; // invalid rect
    }

    // check in rect
    if (!InRect(mins, maxs, _Position))
    {
        return nullptr;
    }

    // check in shape
    if (_Widget->GetShape().Size() >= 3)
    {
        Float2 localPosition = _Position;
        _Widget->FromDesktopToWidget(localPosition);

        AWidgetShape const& shape = _Widget->GetShape();
        if (!BvPointInPoly2D(shape.ToPtr(), shape.Size(), localPosition))
        {
            return nullptr;
        }
    }

    ApplyMargins(rectMins, rectMaxs, _Widget->GetMargin());

    mins.X = Math::Max(rectMins.X, _ClipMins.X);
    mins.Y = Math::Max(rectMins.Y, _ClipMins.Y);

    maxs.X = Math::Min(rectMaxs.X, _ClipMaxs.X);
    maxs.Y = Math::Min(rectMaxs.Y, _ClipMaxs.Y);

    if (mins.X >= maxs.X || mins.Y >= maxs.Y)
    {
        return _Widget;
    }

    // from top to bottom level
    for (int i = _Widget->Childs.Size() - 1; i >= 0; i--)
    {
        WWidget* hoveredWidget = GetWidgetUnderCursor_r(_Widget->Childs[i], mins, maxs, _Position);
        if (hoveredWidget)
        {
            //            if ( hoveredWidget->GetStyle() & WIDGET_STYLE_TRANSPARENT ) {
            //                continue;
            //            }
            return hoveredWidget;
        }
    }

    if (_Widget->GetStyle() & WIDGET_STYLE_TRANSPARENT)
    {
        return nullptr; //_Widget->GetParent();
    }

    return _Widget;
}

WWidget* WDesktop::GetWidgetUnderCursor(Float2 const& _Position)
{
    int      numChilds = Root->Childs.Size();
    WWidget* widget    = nullptr;

    if (!numChilds)
    {
        return nullptr;
    }

    Float2 mins, maxs;
    Root->GetDesktopRect(mins, maxs, true);

    if (InRect(mins, maxs, _Position))
    {
        // Обходим начиная с верхнего уровня
        for (int i = numChilds - 1; i >= 0; i--)
        {
            widget = GetWidgetUnderCursor_r(Root->Childs[i], mins, maxs, _Position);
            if (widget)
            {
                break;
            }
        }
    }

    return widget;
}

// Получает текущий фокус и проверяет, является ли окно, либо его предки эксклюзивными.
// Возвращается ближайшее найденное эксклюзивное окно. В противном случае возвращается nullptr.
WWidget* WDesktop::GetExclusive()
{
    WWidget* exclusive = FocusWidget;

    // Проверить exclusive
    while (exclusive)
    {
        if (HK_HASFLAG(exclusive->GetStyle(), WIDGET_STYLE_EXCLUSIVE) && exclusive->IsVisible())
        {
            break;
        }
        exclusive = exclusive->GetParent();
    }

    return exclusive;
}

void WDesktop::ForEachWidget(bool _TopFirst, TCallback<bool(WWidget* _Widget, void* _Arg)> const& _Callback, void* _Arg)
{
    if (_TopFirst)
    {
        for (int i = Root->Childs.Size() - 1; i >= 0; i--)
        {
            if (!ForEachWidget_r(_TopFirst, Root->Childs[i], _Callback, _Arg))
            {
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < Root->Childs.Size(); i++)
        {
            if (!ForEachWidget_r(_TopFirst, Root->Childs[i], _Callback, _Arg))
            {
                break;
            }
        }
    }
}

bool WDesktop::ForEachWidget_r(bool _TopFirst, WWidget* _Widget, TCallback<bool(WWidget* _Widget, void* _Arg)> const& _Callback, void* _Arg)
{
    if (_TopFirst)
    {
        for (int i = _Widget->Childs.Size() - 1; i >= 0; i--)
        {
            if (!ForEachWidget_r(_TopFirst, _Widget->Childs[i], _Callback, _Arg))
            {
                return false;
            }
        }

        if (!_Callback(_Widget, _Arg))
        {
            return false;
        }
    }
    else
    {
        if (!_Callback(_Widget, _Arg))
        {
            return false;
        }
        for (int i = 0; i < _Widget->Childs.Size(); i++)
        {
            if (!ForEachWidget_r(_TopFirst, _Widget->Childs[i], _Callback, _Arg))
            {
                return false;
            }
        }
    }

    return true;
}

void WDesktop::CancelDragging()
{
    if (DraggingWidget)
    {
        Float2 mins, maxs;

        // get parent layout area
        DraggingWidget->GetLayoutRect(mins, maxs);

        Float2 newWidgetPos = DraggingWidgetPos;

        newWidgetPos -= mins;

        DraggingWidget->OnDragEvent(newWidgetPos);
        DraggingWidget->SetPosition(newWidgetPos);

        DraggingWidget = nullptr;
    }
}

void WDesktop::SetFocusWidget(WWidget* _Focus)
{
    if (IsSame(_Focus, FocusWidget))
    {
        return;
    }

    if (_Focus && (_Focus->GetStyle() & WIDGET_STYLE_NO_INPUTS))
    {
        return;
    }

    if (FocusWidget)
    {
        FocusWidget->bFocus = false;
        FocusWidget->OnFocusLost();
    }

    FocusWidget      = _Focus;
    MouseFocusWidget = _Focus; // TODO: Check

    if (FocusWidget)
    {
        FocusWidget->bFocus = true;
        FocusWidget->OnFocusReceive();
    }
}

void WDesktop::GenerateKeyEvents(SKeyEvent const& _Event, double _TimeStamp)
{
    if (DraggingWidget)
    {
        if (_Event.Key == KEY_ESCAPE && _Event.Action == IA_PRESS)
        {
            CancelDragging();
        }
        return;
    }

    if (Popup)
    {
        if (_Event.Action == IA_PRESS || _Event.Action == IA_REPEAT)
        {
            if (_Event.Key == KEY_ESCAPE)
            {
                ClosePopupMenu();
            }
            else if (_Event.Key == KEY_DOWN)
            {
                Popup->SelectNextItem();
            }
            else if (_Event.Key == KEY_UP)
            {
                Popup->SelectPrevItem();
            }
            else if (_Event.Key == KEY_RIGHT)
            {
                Popup->SelectNextSubMenu();
            }
            else if (_Event.Key == KEY_LEFT)
            {
                Popup->SelectPrevSubMenu();
            }
            else if (_Event.Key == KEY_HOME)
            {
                Popup->SelectFirstItem();
            }
            else if (_Event.Key == KEY_END)
            {
                Popup->SelectLastItem();
            }
        }
        return;
    }

    if (E_OnKeyEvent.HasCallbacks())
    {
        bool bOverride = false;

        E_OnKeyEvent.Dispatch(_Event, _TimeStamp, bOverride);
        if (bOverride)
        {
            return;
        }
    }

    bool bPassFocusWidgetEvent = FocusWidget && FocusWidget->IsVisible() && !FocusWidget->IsDisabled();

    if (ShortcutContainer && _Event.Action == IA_PRESS)
    {
        if (!bPassFocusWidgetEvent || FocusWidget->IsShortcutsAllowed())
        {
            for (SShortcutInfo const& shortcut : ShortcutContainer->GetShortcuts())
            {
                if (shortcut.Key == _Event.Key && shortcut.ModMask == _Event.ModMask)
                {
                    shortcut.Binding();
                    return;
                }
            }
        }
    }

    if (bPassFocusWidgetEvent)
    {
        FocusWidget->ForwardKeyEvent(_Event, _TimeStamp);
    }
}

void WDesktop::GenerateMouseButtonEvents(struct SMouseButtonEvent const& _Event, double _TimeStamp)
{
    WWidget*  widget         = nullptr;
    const int DraggingButton = 0;

    MouseFocusWidget = nullptr;

    if (DraggingWidget)
    {
        if (_Event.Button == DraggingButton && _Event.Action == IA_RELEASE)
        {
            // Stop dragging
            DraggingWidget = nullptr;
        }
        // Ignore when dragging
        return;
    }

    if (_Event.Action == IA_PRESS)
    {

        if (Popup)
        {
            Float2 mins, maxs;

            Root->GetDesktopRect(mins, maxs, true);

            widget = GetWidgetUnderCursor_r(Popup->Self, mins, maxs, CursorPosition);
            if (widget == nullptr)
            {
                ClosePopupMenu();
            }
        }
        if (!widget)
        {
            widget = GetExclusive();
            if (widget)
            {
                Float2 mins, maxs;

                Root->GetDesktopRect(mins, maxs, true);

                widget = GetWidgetUnderCursor_r(widget, mins, maxs, CursorPosition);
                if (widget == nullptr)
                {
                    return;
                }
            }
            else
            {
                widget = GetWidgetUnderCursor(CursorPosition);
            }
        }
        while (widget && (widget->GetStyle() & WIDGET_STYLE_NO_INPUTS))
        {
            widget = widget->GetParent();
        }
        if (widget && widget->IsVisible())
        {

            widget->SetFocus();
            widget->BringOnTop();

            uint64_t newMouseTimeMsec = _TimeStamp * 1000.0;
            uint64_t clickTime        = newMouseTimeMsec - MouseClickTime;
            if (IsSame(MouseClickWidget, widget) && clickTime < DOUBLECLICKTIME_MSEC && CursorPosition.X > MouseClickPos.X - DOUBLECLICKHALFSIZE && CursorPosition.X < MouseClickPos.X + DOUBLECLICKHALFSIZE && CursorPosition.Y > MouseClickPos.Y - DOUBLECLICKHALFSIZE && CursorPosition.Y < MouseClickPos.Y + DOUBLECLICKHALFSIZE)
            {

                if (!widget->IsDisabled())
                {

                    if (_Event.Button == 0)
                    {
                        if (widget->GetStyle() & WIDGET_STYLE_RESIZABLE)
                        {
                            Float2 localPosition = CursorPosition;
                            widget->FromDesktopToWidget(localPosition);

                            auto const& dragShape = widget->GetDragShape();

                            if (BvPointInPoly2D(dragShape.ToPtr(), dragShape.Size(), localPosition))
                            {
                                if (widget->IsMaximized())
                                {
                                    widget->SetNormal();
                                }
                                else
                                {
                                    widget->SetMaximized();
                                }
                            }
                        }
                    }

                    MouseFocusWidget = widget;

                    widget->ForwardMouseButtonEvent(_Event, _TimeStamp);
                    widget->ForwardDblClickEvent(_Event.Button, MouseClickPos, clickTime);
                }

                MouseClickTime   = 0;
                MouseClickWidget = nullptr;
                return;
            }

            MouseClickTime   = newMouseTimeMsec;
            MouseClickWidget = widget;
            MouseClickPos.X  = CursorPosition.X;
            MouseClickPos.Y  = CursorPosition.Y;

            AWidgetShape const& dragShape = widget->GetDragShape();

            Float2 localPosition = CursorPosition;
            widget->FromDesktopToWidget(localPosition);

            // Check dragging
            if (_Event.Button == DraggingButton && BvPointInPoly2D(dragShape.ToPtr(), dragShape.Size(), localPosition))
            {
                // dragging...

                DraggingWidget    = widget;
                DraggingCursor    = CursorPosition;
                DraggingWidgetPos = widget->GetDesktopPosition();
                return;
            }
        }
    }
    else
    {
        widget = FocusWidget;
    }

    MouseFocusWidget = widget;

    if (widget && widget->IsVisible() && !widget->IsDisabled())
    {
        widget->ForwardMouseButtonEvent(_Event, _TimeStamp);
    }
}

void WDesktop::GenerateMouseWheelEvents(struct SMouseWheelEvent const& _Event, double _TimeStamp)
{
    WWidget* widget;

    if (DraggingWidget)
    {
        // Ignore when dragging
        return;
    }

    if (Popup)
    {
        Float2 mins, maxs;

        Root->GetDesktopRect(mins, maxs, true);

        widget = GetWidgetUnderCursor_r(Popup->Self, mins, maxs, CursorPosition);
    }
    else
    {
        widget = GetExclusive();
        if (widget)
        {
            Float2 mins, maxs;

            Root->GetDesktopRect(mins, maxs, true);

            widget = GetWidgetUnderCursor_r(widget, mins, maxs, CursorPosition);
        }
        else
        {
            widget = GetWidgetUnderCursor(CursorPosition);
        }
    }
    while (widget && (widget->GetStyle() & WIDGET_STYLE_NO_INPUTS))
    {
        widget = widget->GetParent();
    }
    if (widget && widget->IsVisible())
    {
        widget->SetFocus();
        widget->BringOnTop();

        if (!widget->IsDisabled())
        {
            widget->ForwardMouseWheelEvent(_Event, _TimeStamp);
        }
    }
}

bool WDesktop::HandleDraggingWidget()
{
    if (!DraggingWidget)
    {
        return false;
    }

    Float2 mins, maxs;

    // get parent layout area
    DraggingWidget->GetLayoutRect(mins, maxs);

    if (DraggingWidget->GetStyle() & WIDGET_STYLE_RESIZABLE)
    {

        if (DraggingWidget->IsMaximized())
        {
            DraggingWidget->SetNormal();

            const Float2 parentSize      = maxs - mins;
            const Float2 cursor          = Math::Clamp(CursorPosition - mins, Float2(0.0f), parentSize);
            const float  widgetWidth     = DraggingWidget->GetCurrentSize().X;
            const float  widgetHalfWidth = widgetWidth * 0.5f;

            Float2 newWidgetPos;
            if (cursor.X < parentSize.X * 0.5f)
            {
                newWidgetPos.X = cursor.X - Math::Min(cursor.X, widgetHalfWidth);
            }
            else
            {
                newWidgetPos.X = cursor.X + Math::Min(parentSize.X - cursor.X, widgetHalfWidth) - widgetWidth;
            }
            newWidgetPos.Y = 0;

            DraggingWidget->OnDragEvent(newWidgetPos);
            DraggingWidget->SetPosition(newWidgetPos);

            DraggingCursor    = CursorPosition;
            DraggingWidgetPos = DraggingWidget->GetDesktopPosition();

            return true;
        }
    }

    // deflate client area
    mins += 1;
    maxs -= 1;

    // clamp cursor position
    Float2 clampedCursorPos = Math::Clamp(CursorPosition, mins, maxs);

    Float2 draggingVector = clampedCursorPos - DraggingCursor;

    // get widget new position
    Float2 newWidgetPos = DraggingWidgetPos + draggingVector;

    newWidgetPos -= mins;

    DraggingWidget->OnDragEvent(newWidgetPos);
    DraggingWidget->SetPosition(newWidgetPos);

    return true;
}

void WDesktop::GenerateMouseMoveEvents(struct SMouseMoveEvent const& _Event, double _TimeStamp)
{

    if (HandleDraggingWidget())
    {
        return;
    }

    WWidget* widget;

    if (!MouseFocusWidget)
    {
        if (Popup)
        {
            Float2 mins, maxs;

            Root->GetDesktopRect(mins, maxs, true);

            widget = GetWidgetUnderCursor_r(Popup->Self, mins, maxs, CursorPosition);
        }
        else
        {
            widget = GetExclusive();
            if (widget)
            {
                Float2 mins, maxs;

                Root->GetDesktopRect(mins, maxs, true);

                widget = GetWidgetUnderCursor_r(widget, mins, maxs, CursorPosition);
            }
            else
            {
                widget = GetWidgetUnderCursor(CursorPosition);
            }
        }
        while (widget && (widget->GetStyle() & WIDGET_STYLE_NO_INPUTS))
        {
            widget = widget->GetParent();
        }
    }
    else
    {
        widget = MouseFocusWidget;
    }

    if (widget)
    {

        if (!widget->IsDisabled())
        {
            widget->ForwardMouseMoveEvent(_Event, _TimeStamp);
        }
    }
}

void WDesktop::GenerateJoystickButtonEvents(SJoystickButtonEvent const& _Event, double _TimeStamp)
{
    if (DraggingWidget)
    {
        // Don't allow joystick buttons when dragging
        return;
    }

    if (Popup)
    {
        // TODO: Use joystick/gamepad buttons to control popup menu
#if 0
        if ( _Event.Action == IE_Press || _Event.Action ==  IE_Repeat ) {
            if ( _Event.Key == KEY_ESCAPE ) {
                ClosePopupMenu();
            } else if ( _Event.Key == KEY_DOWN ) {
                Popup->SelectNextItem();
            } else if ( _Event.Key == KEY_UP ) {
                Popup->SelectPrevItem();
            } else if ( _Event.Key == KEY_RIGHT ) {
                Popup->SelectNextSubMenu();
            } else if ( _Event.Key == KEY_LEFT ) {
                Popup->SelectPrevSubMenu();
            } else if ( _Event.Key == KEY_HOME ) {
                Popup->SelectFirstItem();
            } else if ( _Event.Key == KEY_END ) {
                Popup->SelectLastItem();
            }
        }
#endif
        return;
    }

    if (FocusWidget && FocusWidget->IsVisible() && !FocusWidget->IsDisabled())
    {
        FocusWidget->ForwardJoystickButtonEvent(_Event, _TimeStamp);
    }
}

void WDesktop::GenerateJoystickAxisEvents(SJoystickAxisEvent const& _Event, double _TimeStamp)
{
    if (DraggingWidget)
    {
        // Don't allow joystick buttons when dragging
        return;
    }

    if (Popup)
    {
        return;
    }

    if (FocusWidget && FocusWidget->IsVisible() && !FocusWidget->IsDisabled())
    {
        FocusWidget->ForwardJoystickAxisEvent(_Event, _TimeStamp);
    }
}

void WDesktop::GenerateCharEvents(struct SCharEvent const& _Event, double _TimeStamp)
{
    if (DraggingWidget)
    {
        // Ignore when dragging
        return;
    }

    if (FocusWidget && FocusWidget->IsVisible() && !FocusWidget->IsDisabled())
    {
        FocusWidget->ForwardCharEvent(_Event, _TimeStamp);
    }
}

void WDesktop::GenerateWindowHoverEvents()
{
    WWidget* w = GetWidgetUnderCursor(CursorPosition);

    if (LastHoveredWidget && (w == nullptr || LastHoveredWidget != w))
    {
        LastHoveredWidget->OnWindowHovered(false);
    }

    if (LastHoveredWidget != w)
    {
        LastHoveredWidget = w;

        if (!w)
        {
            return;
        }

        LastHoveredWidget->OnWindowHovered(true);
    }
}

void WDesktop::GenerateDrawEvents(ACanvas& _Canvas)
{
    Float2 mins, maxs;
    Root->GetDesktopRect(mins, maxs, false);

    _Canvas.PushClipRect(mins, maxs);

    if (bDrawBackground)
    {
        OnDrawBackground(_Canvas);
    }

    // draw childs recursive
    for (WWidget* child : Root->Childs)
    {
        child->Draw_r(_Canvas, mins, maxs);
    }

    for (auto it = Tooltips.begin(); it != Tooltips.end();)
    {
        auto& tooltip = (*it);
        if (tooltip.IsExpired())
        {
            it = Tooltips.Erase(it);
        }
        else
        {
            tooltip->Draw_r(_Canvas, mins, maxs);
            it++;
        }
    }

    //_Canvas.DrawCircleFilled( CursorPosition, 5.0f, Color4(1,0,0) );

    _Canvas.PopClipRect();
}

void WDesktop::MarkTransformDirty()
{
    Root->MarkTransformDirty();
}

void WDesktop::OnDrawBackground(ACanvas& _Canvas)
{
    _Canvas.DrawRectFilled(_Canvas.GetClipMins(), _Canvas.GetClipMaxs(), Color4(0.03f, 0.03f, 0.03f, 1.0f));
    //_Canvas.DrawRectFilled( _Canvas.GetClipMins(), _Canvas.GetClipMaxs(), Color4::Black() );
}

void WDesktop::DrawCursor(ACanvas& _Canvas)
{
    _Canvas.DrawCursor(Cursor, CursorPosition, Color4::White(), Color4(0, 0, 0, 1), Color4(0, 0, 0, 0.3f));
}

void WDesktop::SetShortcuts(AShortcutContainer* _ShortcutContainer)
{
    ShortcutContainer = _ShortcutContainer;
}

void WDesktop::AddTooltip(WWidget* TooltipWidget)
{
    if (std::find(Tooltips.begin(), Tooltips.end(), TooltipWidget)
         != Tooltips.end())
    {
        return;
    }

    Tooltips.EmplaceBack(TooltipWidget);
}

void WDesktop::RemoveTooltip(WWidget* TooltipWidget)
{
    auto it = std::find(Tooltips.begin(), Tooltips.end(), TooltipWidget);
    if (it != Tooltips.end())
    {
        Tooltips.Erase(it);
    }
}
