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

#include "UIDesktop.h"
#include "UIManager.h"
#include "UIWindow.h"
#include "UIDockContainer.h"

#include <Engine/Runtime/FrameLoop.h>
#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/Core/Platform.h>

#define DOUBLECLICKTIME_MSEC 250
#define DOUBLECLICKHALFSIZE  4.0f

HK_NAMESPACE_BEGIN

int UIVisibilityFrame = 0;

UIDesktop::UIDesktop()
{
}

UIDesktop::~UIDesktop()
{
    for (UIWidget* widget : m_Widgets)
        widget->RemoveRef();
}

void UIDesktop::AddWidget(UIWidget* widget)
{
    if (widget->m_Desktop)
        return;

    // Check if already exists
    if (m_Widgets.IndexOf(widget) == Core::NPOS)
    {
        m_Widgets.Add(widget);
        widget->AddRef();
        widget->m_Desktop = this;

        if (widget->ShouldSetFocusOnAddToDesktop())
        {
            SetFocusWidget(widget);
        }
    }
}

void UIDesktop::RemoveWidget(UIWidget* widget)
{
    auto index = m_Widgets.IndexOf(widget);
    if (index != Core::NPOS)
    {
        m_Widgets.Remove(index);

        widget->m_Desktop = nullptr;
        widget->RemoveRef();
    }
}

void UIDesktop::SetDragWidget(UIWidget* widget)
{
    m_PendingDrag = widget;
}

void UIDesktop::SetShortcuts(UIShortcutContainer* shortcutContainer)
{
    m_ShortcutContainer = shortcutContainer;
}

void UIDesktop::SetFullscreenWidget(UIWidget* widget)
{
    if (m_FullscreenWidget == widget)
        return;

    if (widget && widget->GetDesktop() != this)
        return;

    m_FullscreenWidget = widget;

    if (m_FullscreenWidget)
        m_FullscreenWidget->Visibility = UI_WIDGET_VISIBILITY_VISIBLE;
}

UIWidget* UIDesktop::Trace(float x, float y)
{
    if (m_FullscreenWidget)
    {
        return m_FullscreenWidget->Trace(x, y);
    }

    for (int i = (int)m_Widgets.Size() - 1; i >= 0; --i)
    {
        UIWidget* widget = m_Widgets[i]->Trace(x, y);
        if (widget)
        {
            return widget;
        }
    }
    return {};
}

void UIDesktop::UpdateGeometry(float w, float h)
{
    UIVisibilityFrame++;

    m_Geometry.Mins.X     = 0;
    m_Geometry.Mins.Y     = 0;
    m_Geometry.Maxs.X     = w;
    m_Geometry.Maxs.Y     = h;
    m_Geometry.PaddedMins = m_Geometry.Mins;
    m_Geometry.PaddedMaxs = m_Geometry.Maxs;

    if (m_Geometry.IsTiny())
    {
        return;
    }

    Float2 desktopSize = m_Geometry.PaddedMaxs - m_Geometry.PaddedMins;

    if (m_FullscreenWidget)
    {
        m_FullscreenWidget->MeasureLayout(false, false, desktopSize);

        m_FullscreenWidget->m_Geometry.Mins = m_Geometry.PaddedMins;
        m_FullscreenWidget->m_Geometry.Maxs = m_Geometry.PaddedMaxs;

        m_FullscreenWidget->ArrangeChildren(false, false);
    }
    else
    {
        for (UIWidget* widget : m_Widgets)
        {
            if (widget->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
                continue;

            UIWindow* window = dynamic_cast<UIWindow*>(widget);
            if (window && window->WindowState == UIWindow::WS_MAXIMIZED)
            {
                widget->MeasureLayout(true, true, desktopSize);
            }
            else
            {
                widget->MeasureLayout(true, true, widget->Size);
            }
        }

        for (UIWidget* widget : m_Widgets)
        {
            if (widget->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
                continue;

            UIWindow* window = dynamic_cast<UIWindow*>(widget);
            if (window && window->WindowState == UIWindow::WS_MAXIMIZED)
            {
                widget->m_Geometry.Mins = m_Geometry.PaddedMins;
                widget->m_Geometry.Maxs = m_Geometry.PaddedMaxs;
            }
            else
            {
                widget->m_Geometry.Mins = m_Geometry.PaddedMins + widget->Position;
                widget->m_Geometry.Maxs = widget->m_Geometry.Mins + widget->m_MeasuredSize;
            }

            if ((widget->m_Geometry.Mins.X >= m_Geometry.PaddedMaxs.X) || (widget->m_Geometry.Mins.Y >= m_Geometry.PaddedMaxs.Y))
                continue;

            widget->ArrangeChildren(true, true);
        }
    }

    if (m_PendingDrag)
    {
        StartDragging(m_PendingDrag);
        m_PendingDrag.Reset();
    }
}

void UIDesktop::SetFocusWidget(UIWidget* widget)
{
    if (widget == m_FocusWidget.RawPtr())
    {
        return;
    }

    if (widget && widget->bNoInput)
    {
        return;
    }

    if (m_FocusWidget)
    {
        //MouseButtonEvent event;
        //event.ModMask = 0;
        //event.Action  = IA_RELEASE;

        //for (int i = 0; i < 8; i++)
        //{
        //    event.Button = MOUSE_BUTTON_1 + i;
        //    m_FocusWidget->ForwardMouseButtonEvent(event, 0);
        //}

        // FIXME: Unpress all keys?

        //m_FocusWidget->bFocus = false;
        m_FocusWidget->ForwardFocusEvent(false);
    }

    m_FocusWidget      = widget;
    m_MouseFocusWidget = widget; // TODO: Check

    if (m_FocusWidget)
    {
        //m_FocusWidget->bFocus = true;
        m_FocusWidget->ForwardFocusEvent(true);
    }
}

void UIDesktop::Draw(Canvas& cv)
{
    cv.Push(CANVAS_PUSH_FLAG_RESET);

    cv.Scissor(m_Geometry.Mins, m_Geometry.Maxs);

    if (m_Wallpaper)
        DrawBrush(cv, m_Geometry.Mins, m_Geometry.Maxs, {}, m_Wallpaper);

    if (m_FullscreenWidget)
    {
        m_FullscreenWidget->Draw(cv, m_Geometry.Mins, m_Geometry.Maxs, 1.0f);
    }
    else
    {
        for (UIWidget* widget : m_Widgets)
        {
            widget->Draw(cv, m_Geometry.Mins, m_Geometry.Maxs, 1.0f);
        }
    }

    cv.Pop();
}

UIWidget* UIDesktop::GetExclusive()
{
    UIWidget* exclusive = m_FocusWidget;

    while (exclusive)
    {
        if (exclusive->bExclusive && exclusive->IsVisible())
            break;
        exclusive = exclusive->GetParent();
    }

    return exclusive;
}

void UIDesktop::GenerateKeyEvents(KeyEvent const& event)
{
    if (m_DraggingWidget)
    {
        if (event.Key == KEY_ESCAPE && event.Action == IA_PRESS)
        {
            CancelDragging();
        }
        return;
    }

    #if 0
    if (m_Popup)
    {
        if (event.Action == IA_PRESS || event.Action == IA_REPEAT)
        {
            if (event.Key == KEY_ESCAPE)
            {
                ClosePopupMenu();
            }
            else if (event.Key == KEY_DOWN)
            {
                Popup->SelectNextItem();
            }
            else if (event.Key == KEY_UP)
            {
                Popup->SelectPrevItem();
            }
            else if (event.Key == KEY_RIGHT)
            {
                Popup->SelectNextSubMenu();
            }
            else if (event.Key == KEY_LEFT)
            {
                Popup->SelectPrevSubMenu();
            }
            else if (event.Key == KEY_HOME)
            {
                Popup->SelectFirstItem();
            }
            else if (event.Key == KEY_END)
            {
                Popup->SelectLastItem();
            }
        }
        return;
    }
    #endif

    #if 0
    if (E_OnKeyEvent.HasCallbacks())
    {
        bool bOverride = false;

        E_OnKeyEvent.Dispatch(event, bOverride);
        if (bOverride)
        {
            return;
        }
    }
    #endif

    bool bPassFocusWidgetEvent = m_FocusWidget && m_FocusWidget->IsVisible() && !m_FocusWidget->IsDisabled();

    if (m_ShortcutContainer && event.Action == IA_PRESS)
    {
        if (!bPassFocusWidgetEvent || m_FocusWidget->bShortcutsAllowed)
        {
            for (UIShortcutInfo const& shortcut : m_ShortcutContainer->GetShortcuts())
            {
                if (shortcut.Key == event.Key && shortcut.ModMask == event.ModMask)
                {
                    shortcut.Binding();
                    return;
                }
            }
        }
    }

    if (bPassFocusWidgetEvent)
    {
        m_FocusWidget->ForwardKeyEvent(event);
    }
}

void UIDesktop::GenerateMouseButtonEvents(struct MouseButtonEvent const& event)
{
    UIWidget* widget = nullptr;
    const int DraggingButton = 0;

    m_MouseFocusWidget.Reset();

    if (m_DraggingWidget)
    {
        if (event.Button == DraggingButton && event.Action == IA_RELEASE)
        {
            // Stop dragging

            UIDockWidget* dockWidget = dynamic_cast<UIDockWidget*>(m_DraggingWidget.RawPtr());
            if (dockWidget)
            {
                UIDockContainer* dockContainer = dockWidget->GetContainer();
                if (dockContainer)
                {
                    if (dockContainer->AttachWidgetAt(dockWidget, GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y))
                    {
                        RemoveWidget(dockWidget);
                    }
                    dockContainer->bDrawPlacement = false;
                    dockContainer->DragWidget.Reset();
                }
            }

            m_DraggingWidget.Reset();
        }
        // Ignore when dragging
        return;
    }

    if (event.Action == IA_PRESS)
    {
        if (m_Popup)
        {
            widget = m_Popup->Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
            if (!widget)
            {
                ClosePopupWidget();
            }
        }
        if (!widget)
        {
            widget = GetExclusive();
            if (widget)
            {
                widget = widget->Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
                if (!widget)
                    return;
            }
            else
            {
                widget = Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
            }
        }
        while (widget && widget->bNoInput)
            widget = widget->GetParent();

        if (widget && widget->IsVisible())
        {
            SetFocusWidget(widget);
            
            widget->BringOnTop();

            uint64_t newMouseTimeMsec = Core::SysMilliseconds();
            uint64_t clickTime        = newMouseTimeMsec - m_MouseClickTime;
            if (m_MouseClickWidget == widget && clickTime < DOUBLECLICKTIME_MSEC && GUIManager->CursorPosition.X > m_MouseClickPos.X - DOUBLECLICKHALFSIZE && GUIManager->CursorPosition.X < m_MouseClickPos.X + DOUBLECLICKHALFSIZE && GUIManager->CursorPosition.Y > m_MouseClickPos.Y - DOUBLECLICKHALFSIZE && GUIManager->CursorPosition.Y < m_MouseClickPos.Y + DOUBLECLICKHALFSIZE)
            {
                if (!widget->IsDisabled())
                {
                    if (event.Button == 0 && !widget->GetParent())
                    {
                        UIWindow* window = dynamic_cast<UIWindow*>(widget);

                        if (window && window->bResizable)
                        {
                            if (window->CaptionHitTest(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y))
                            {
                                if (window->IsMaximized())
                                    window->SetNormal();
                                else
                                    window->SetMaximized();
                            }
                        }
                    }

                    m_MouseFocusWidget = widget;

                    widget->ForwardMouseButtonEvent(event);
                    widget->ForwardDblClickEvent(event.Button, m_MouseClickPos, clickTime);
                }

                m_MouseClickTime = 0;
                m_MouseClickWidget.Reset();
                return;
            }

            m_MouseClickTime   = newMouseTimeMsec;
            m_MouseClickWidget = widget;
            m_MouseClickPos.X  = GUIManager->CursorPosition.X;
            m_MouseClickPos.Y  = GUIManager->CursorPosition.Y;

            // Check dragging
            if (event.Button == DraggingButton)
            {
                if (widget->bAllowDrag)
                {
                    StartDragging(widget);
                    return;
                }

                UIWindow* window = dynamic_cast<UIWindow*>(widget);
                if (window && window->CaptionHitTest(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y))
                {
                    StartDragging(widget);
                    return;
                }
            }
        }
    }
    else
    {
        widget = m_FocusWidget;
    }

    m_MouseFocusWidget = widget;

    if (widget && widget->IsVisible() && !widget->IsDisabled())
    {
        widget->ForwardMouseButtonEvent(event);
    }
}

void UIDesktop::GenerateMouseWheelEvents(MouseWheelEvent const& event)
{
    UIWidget* widget;

    if (m_DraggingWidget)
    {
        // Ignore when dragging
        return;
    }

    if (m_Popup)
    {
        widget = m_Popup->Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
    }
    else
    {
        widget = GetExclusive();
        if (widget)
        {
            widget = widget->Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
        }
        else
        {
            widget = Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
        }
    }
    while (widget && widget->bNoInput)
    {
        widget = widget->GetParent();
    }
    if (widget && widget->IsVisible())
    {
        SetFocusWidget(widget);
        widget->BringOnTop();

        if (!widget->IsDisabled())
        {
            widget->ForwardMouseWheelEvent(event);
        }
    }
}

void UIDesktop::GenerateMouseMoveEvents(MouseMoveEvent const& event)
{
    if (HandleDraggingWidget())
    {
        return;
    }

    UIWidget* widget;

    if (!m_MouseFocusWidget)
    {
        if (m_Popup)
        {
            widget = m_Popup->Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
        }
        else
        {
            widget = GetExclusive();
            if (widget)
            {
                widget = widget->Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
            }
            else
            {
                widget = Trace(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
            }
        }
        while (widget && widget->bNoInput)
        {
            widget = widget->GetParent();
        }
    }
    else
    {
        widget = m_MouseFocusWidget;
    }

    if (widget)
    {
        if (!widget->IsDisabled())
        {
            widget->ForwardMouseMoveEvent(event);
        }
    }
}

void UIDesktop::GenerateJoystickButtonEvents(JoystickButtonEvent const& event)
{
    if (m_DraggingWidget)
    {
        // Don't allow joystick buttons when dragging
        return;
    }

    #if 0
    if (m_Popup)
    {
        // TODO: Use joystick/gamepad buttons to control popup menu
#if 0
        if ( event.Action == IE_Press || event.Action ==  IE_Repeat ) {
            if ( event.Key == KEY_ESCAPE ) {
                ClosePopupMenu();
            } else if ( event.Key == KEY_DOWN ) {
                m_Popup->SelectNextItem();
            } else if ( event.Key == KEY_UP ) {
                m_Popup->SelectPrevItem();
            } else if ( event.Key == KEY_RIGHT ) {
                m_Popup->SelectNextSubMenu();
            } else if ( event.Key == KEY_LEFT ) {
                m_Popup->SelectPrevSubMenu();
            } else if ( event.Key == KEY_HOME ) {
                m_Popup->SelectFirstItem();
            } else if ( event.Key == KEY_END ) {
                m_Popup->SelectLastItem();
            }
        }
#endif
        return;
    }
    #endif

    if (m_FocusWidget && m_FocusWidget->IsVisible() && !m_FocusWidget->IsDisabled())
    {
        m_FocusWidget->ForwardJoystickButtonEvent(event);
    }
}

void UIDesktop::GenerateJoystickAxisEvents(JoystickAxisEvent const& event)
{
    if (m_DraggingWidget)
    {
        // Don't allow joystick buttons when dragging
        return;
    }
    #if 0
    if (m_Popup)
    {
        return;
    }
    #endif
    if (m_FocusWidget && m_FocusWidget->IsVisible() && !m_FocusWidget->IsDisabled())
    {
        m_FocusWidget->ForwardJoystickAxisEvent(event);
    }
}

void UIDesktop::GenerateCharEvents(CharEvent const& event)
{
    if (m_DraggingWidget)
    {
        // Ignore when dragging
        return;
    }

    if (m_FocusWidget && m_FocusWidget->IsVisible() && !m_FocusWidget->IsDisabled())
    {
        m_FocusWidget->ForwardCharEvent(event);
    }
}

bool UIDesktop::HandleDraggingWidget()
{
    if (!m_DraggingWidget)
    {
        return false;
    }

    Float2 mins = m_Geometry.PaddedMins;
    Float2 maxs = m_Geometry.PaddedMaxs;

    UIWidget* parent = m_DraggingWidget->GetParent();
    if (parent)
    {
        mins = parent->m_Geometry.PaddedMins;
        maxs = parent->m_Geometry.PaddedMaxs;
    }

    UIWindow* window = dynamic_cast<UIWindow*>(m_DraggingWidget.RawPtr());
    if (window && window->bResizable && !window->GetParent())
    {
        if (window->IsMaximized())
        {
            window->SetNormal();

            const Float2 parentSize = maxs - mins;
            const Float2 cursor     = Math::Clamp(GUIManager->CursorPosition - mins, Float2(0.0f), parentSize);

            const float widgetWidth = window->Size.X;
            const float widgetHalfWidth = widgetWidth * 0.5f;

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

            newWidgetPos = newWidgetPos.Floor();

            window->ForwardDragEvent(newWidgetPos);
            window->Position = newWidgetPos;

            m_DraggingCursor    = GUIManager->CursorPosition;
            m_DraggingWidgetPos = mins + newWidgetPos;

            return true;
        }
    }

    if (maxs.X - mins.X > 2 && maxs.Y - mins.Y > 2)
    {
        // clamp cursor position
        Float2 clampedCursorPos = Math::Clamp(GUIManager->CursorPosition, mins + 1, maxs - 1);

        Float2 draggingVector = clampedCursorPos - m_DraggingCursor;

        // get widget new position
        Float2 newWidgetPos = m_DraggingWidgetPos + draggingVector;

        newWidgetPos -= mins;

        newWidgetPos = newWidgetPos.Floor();

        m_DraggingWidget->ForwardDragEvent(newWidgetPos);
        m_DraggingWidget->Position = newWidgetPos;
    }

    return true;
}

void UIDesktop::CancelDragging()
{
    if (m_DraggingWidget)
    {
        Float2 mins = m_DraggingWidget->GetParent() ? m_DraggingWidget->GetParent()->m_Geometry.PaddedMins : m_Geometry.PaddedMins;
        
        Float2 newWidgetPos = m_DraggingWidgetPos;

        newWidgetPos -= mins;

        newWidgetPos = newWidgetPos.Floor();

        m_DraggingWidget->ForwardDragEvent(newWidgetPos);
        m_DraggingWidget->Position = newWidgetPos;

        m_DraggingWidget = nullptr;
    }
}

void UIDesktop::StartDragging(UIWidget* widget)
{
    m_DraggingWidget    = widget;
    m_DraggingCursor    = GUIManager->CursorPosition;
    m_DraggingWidgetPos = widget->m_Geometry.Mins;

    UIDockWidget* dockWidget = dynamic_cast<UIDockWidget*>(widget);
    if (dockWidget)
    {
        dockWidget->Size = dockWidget->m_DockSize;

        UIDockContainer* dockContainer = dockWidget->GetContainer();
        if (dockContainer)
        {
            dockContainer->DetachWidget(dockWidget);

            dockContainer->bDrawPlacement = true;
            dockContainer->DragWidget     = dockWidget;
        }

        //SetFocusWidget(dockWidget);

        AddWidget(dockWidget);
    }

    HandleDraggingWidget();
}

//void UIDesktop::ReleaseDragWidget()
//{
//    if (m_BeginDragWidget)
//    {
//        UIDockWidget* dockWidget = dynamic_cast<UIDockWidget*>(m_DragWidget.RawPtr());
//        if (dockWidget)
//        {
//            UIDockContainer* dockContainer = dockWidget->GetContainer();
//            if (dockContainer)
//            {
//                dockContainer->AttachDragWidget(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
//            }
//        }
//    }
//    m_DragWidget.Reset();
//}

void UIDesktop::OpenPopupWidget(UIWidget* widget, Float2 const& position)
{
    ClosePopupWidget();

    if (widget)
    {
        m_Popup             = widget;
        m_Popup->Visibility = UI_WIDGET_VISIBILITY_VISIBLE;
        m_Popup->Position   = position;

        //m_Popup->MaxSize = {Root->GetWidth(), Math::Max(1.0f, (Root->GetHeight() - Position.Y))};

        AddWidget(m_Popup);

        SetFocusWidget(m_Popup);
        m_Popup->BringOnTop();
    }
}

void UIDesktop::ClosePopupWidget()
{
    if (m_Popup)
    {
        RemoveWidget(m_Popup);

        m_Popup->Visibility = UI_WIDGET_VISIBILITY_INVISIBLE;
        m_Popup.Reset();
    }
}

HK_NAMESPACE_END
