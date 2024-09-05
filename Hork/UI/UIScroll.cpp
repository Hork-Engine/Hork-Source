/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Geometry/BV/BvIntersect.h>
#include <Hork/GameApplication/GameApplication.h>

#include "UIScroll.h"
#include "UIManager.h"

HK_NAMESPACE_BEGIN

UIScroll::UIScroll(UIWidget* contentWidget) :
    m_ContentWidget(contentWidget ? contentWidget : UINew(UIWidget))
{
    struct ScrollLayout : UIBaseLayout
    {
        UIScroll* Self;

        ScrollLayout(UIScroll* self) :
            Self(self)
        {}

        Float2 MeasureLayout(UIWidget*, bool, bool, Float2 const& size) override
        {
            Self->DoMeasureLayout(size);
            return Self->m_MeasuredSize;
        }

        void ArrangeChildren(UIWidget*, bool, bool) override
        {
            Self->DoArrangeChildren();
        }
    };

    Layout = UINew(ScrollLayout, this);

    m_ContentWidget->Position = Float2(0);

    AddWidget(m_ContentWidget);
}

void UIScroll::DoMeasureLayout(Float2 const& size)
{
    float scrollBarSizeWithPad = m_ScrollbarWidth + m_ScrollbarPadding;

    Padding.Left   = 0;
    Padding.Top    = 0;
    Padding.Right  = 0;
    Padding.Bottom = 0;

    Float2 paddedSize;
    paddedSize.X = size.X - scrollBarSizeWithPad;
    paddedSize.Y = size.Y - scrollBarSizeWithPad;
    paddedSize.X = Math::Max(0.0f, paddedSize.X);
    paddedSize.Y = Math::Max(0.0f, paddedSize.Y);

    if (m_ContentWidget->Visibility == UI_WIDGET_VISIBILITY_VISIBLE)
    {
        m_ContentWidget->MeasureLayout(true, true, {paddedSize.X, paddedSize.Y});
    }

    m_ContentSize  = m_ContentWidget->m_MeasuredSize;
    m_MeasuredSize = size;

    if (!m_bAutoHScroll)
        Padding.Right = scrollBarSizeWithPad;

    if (!m_bAutoVScroll)
        Padding.Bottom = scrollBarSizeWithPad;

    m_ViewSize.X = size.X - Padding.Right;
    m_ViewSize.Y = size.Y - Padding.Bottom;

    Float2 contentSizeWithScroll = m_ContentSize - m_ScrollPosition;

    if (m_bAutoHScroll)
    {
        if (contentSizeWithScroll.X > m_ViewSize.X)
        {
            Padding.Bottom = scrollBarSizeWithPad;
            m_ViewSize.Y -= scrollBarSizeWithPad;
        }
    }
    else
    {
        Padding.Bottom = scrollBarSizeWithPad;
    }

    if (m_bAutoVScroll)
    {
        if (contentSizeWithScroll.Y > m_ViewSize.Y)
        {
            Padding.Right = scrollBarSizeWithPad;
            m_ViewSize.X -= scrollBarSizeWithPad;
            if (m_bAutoHScroll)
            {
                if (contentSizeWithScroll.X > m_ViewSize.X)
                {
                    Padding.Bottom = scrollBarSizeWithPad;
                }
            }
        }
    }
    else
    {
        Padding.Right = scrollBarSizeWithPad;
    }

    m_ViewSize.X = Math::Max(0.0f, m_ViewSize.X);
    m_ViewSize.Y = Math::Max(0.0f, m_ViewSize.Y);

    //m_ScrollPosition.X = Math::Max(m_ScrollPosition.X, -m_ContentSize.X + m_ViewSize.X);
    //m_ScrollPosition.Y = Math::Max(m_ScrollPosition.Y, -m_ContentSize.Y + m_ViewSize.Y);
    m_ScrollPosition.X = Math::Min(m_ScrollPosition.X, 0.0f);
    m_ScrollPosition.Y = Math::Min(m_ScrollPosition.Y, 0.0f);

    Float2 horizontalBarMins;
    Float2 horizontalBarMaxs;

    Float2 verticalBarMins;
    Float2 verticalBarMaxs;

    m_bDrawHScroll = Padding.Bottom > m_ScrollbarPadding;
    m_bDrawVScroll = Padding.Right > m_ScrollbarPadding;

    if (m_bDrawHScroll)
    {
        horizontalBarMins.X = 0;
        horizontalBarMins.Y = m_ViewSize.Y + m_ScrollbarPadding;

        horizontalBarMaxs.X = m_ViewSize.X;
        horizontalBarMaxs.Y = horizontalBarMins.Y + m_ScrollbarWidth;

        if (m_bWithButtons)
        {
            Float2 horizontalButtonSize = Float2(m_ButtonSize, m_ScrollbarWidth);

            // left
            m_ButtonMins[0] = horizontalBarMins;
            m_ButtonMaxs[0] = m_ButtonMins[0] + horizontalButtonSize;

            // right
            m_ButtonMaxs[1] = horizontalBarMaxs;
            m_ButtonMins[1] = m_ButtonMaxs[1] - horizontalButtonSize;

            horizontalBarMins.X += m_ButtonSize;
            horizontalBarMaxs.X -= m_ButtonSize;
        }
    }

    if (m_bDrawVScroll)
    {
        verticalBarMins.X = m_ViewSize.X + m_ScrollbarPadding;
        verticalBarMins.Y = 0;

        verticalBarMaxs.X = verticalBarMins.X + m_ScrollbarWidth;
        verticalBarMaxs.Y = m_ViewSize.Y;

        if (m_bWithButtons)
        {
            Float2 verticalButtonSize = Float2(m_ScrollbarWidth, m_ButtonSize);

            // up
            m_ButtonMins[2] = verticalBarMins;
            m_ButtonMaxs[2] = m_ButtonMins[2] + verticalButtonSize;

            // down
            m_ButtonMaxs[3] = verticalBarMaxs;
            m_ButtonMins[3] = m_ButtonMaxs[3] - verticalButtonSize;

            verticalBarMins.Y += m_ButtonSize;
            verticalBarMaxs.Y -= m_ButtonSize;
        }
    }    

    m_HorizontalScrollbarMins = horizontalBarMins;
    m_HorizontalScrollbarMaxs = horizontalBarMaxs;

    m_VerticalScrollbarMins = verticalBarMins;
    m_VerticalScrollbarMaxs = verticalBarMaxs;

    float hsliderPos, hsliderWidth;
    float vsliderPos, vsliderWidth;

    if (contentSizeWithScroll.X > 0.0f)
    {
        hsliderPos   = -m_ScrollPosition.X / m_ContentSize.X;
        hsliderWidth = Math::Min(1.0f, m_ViewSize.X / m_ContentSize.X);
    }
    else
    {
        hsliderPos = 0;
        hsliderWidth = 1;
    }

    if (contentSizeWithScroll.Y > 0.0f)
    {
        vsliderPos   = -m_ScrollPosition.Y / m_ContentSize.Y;
        vsliderWidth = Math::Min(1.0f, m_ViewSize.Y / m_ContentSize.Y);
    }
    else
    {
        vsliderPos   = 0;
        vsliderWidth = 1;
    }

    float hscale = Math::Max(0.0f, m_HorizontalScrollbarMaxs.X - m_HorizontalScrollbarMins.X);
    hsliderPos *= hscale;
    hsliderWidth *= hscale;

    float vscale = Math::Max(0.0f, m_VerticalScrollbarMaxs.Y - m_VerticalScrollbarMins.Y);
    vsliderPos *= vscale;
    vsliderWidth *= vscale;

    const int MIN_SLIDER_WIDTH = 10;

    if (hsliderWidth < MIN_SLIDER_WIDTH)
        hsliderWidth = MIN_SLIDER_WIDTH;

    if (vsliderWidth < MIN_SLIDER_WIDTH)
        vsliderWidth = MIN_SLIDER_WIDTH;

    m_HorizontalSliderMins.X = horizontalBarMins.X + hsliderPos;
    m_HorizontalSliderMins.Y = horizontalBarMins.Y;

    m_HorizontalSliderMaxs.X = m_HorizontalSliderMins.X + hsliderWidth;
    m_HorizontalSliderMaxs.Y = horizontalBarMaxs.Y;

    m_VerticalSliderMins.X = verticalBarMins.X;
    m_VerticalSliderMins.Y = verticalBarMins.Y + vsliderPos;

    m_VerticalSliderMaxs.X = verticalBarMaxs.X;
    m_VerticalSliderMaxs.Y = m_VerticalSliderMins.Y + vsliderWidth;
}

void UIScroll::DoArrangeChildren()
{
    if (m_ContentWidget->Visibility == UI_WIDGET_VISIBILITY_VISIBLE)
    {
        m_ContentWidget->m_Geometry.Mins.X = m_Geometry.PaddedMins.X + m_ContentWidget->Position.X;
        m_ContentWidget->m_Geometry.Mins.Y = m_Geometry.PaddedMins.Y + m_ContentWidget->Position.Y;

        m_ContentWidget->m_Geometry.Maxs = m_ContentWidget->m_Geometry.Mins + m_ContentWidget->m_MeasuredSize;

        if ((m_ContentWidget->m_Geometry.Mins.X < m_Geometry.PaddedMaxs.X) && (m_ContentWidget->m_Geometry.Mins.Y < m_Geometry.PaddedMaxs.Y))
        {
            m_ContentWidget->ArrangeChildren(true, true);
        }
    }

    m_VerticalScrollbarMins += m_Geometry.Mins;
    m_VerticalScrollbarMaxs += m_Geometry.Mins;
    m_HorizontalScrollbarMins += m_Geometry.Mins;
    m_HorizontalScrollbarMaxs += m_Geometry.Mins;
    m_HorizontalSliderMins += m_Geometry.Mins;
    m_HorizontalSliderMaxs += m_Geometry.Mins;
    m_VerticalSliderMins += m_Geometry.Mins;
    m_VerticalSliderMaxs += m_Geometry.Mins;
    if (m_bWithButtons)
    {
        for (int i = 0; i < 4; i++)
        {
            m_ButtonMins[i] += m_Geometry.Mins;
            m_ButtonMaxs[i] += m_Geometry.Mins;
        }
    }   
}

void UIScroll::ScrollHome()
{
    SetScrollPosition({0.0f, 0.0f});
}

void UIScroll::ScrollEnd()
{
    Float2 ContentPos(0.0f, -m_ContentSize.Y + m_ViewSize.Y);

    ContentPos.X = Math::Min(ContentPos.X, 0.0f);
    ContentPos.Y = Math::Min(ContentPos.Y, 0.0f);

    SetScrollPosition(ContentPos);
}

void UIScroll::ScrollDelta(Float2 const& delta)
{
    m_ScrollPosition += delta; 
    if (Math::Abs(delta.X) > 0.0f)
        m_ScrollPosition.X = Math::Max(m_ScrollPosition.X, -m_ContentSize.X + m_ViewSize.X);
    if (Math::Abs(delta.Y) > 0.0f)
        m_ScrollPosition.Y = Math::Max(m_ScrollPosition.Y, -m_ContentSize.Y + m_ViewSize.Y);
    m_ScrollPosition.X = Math::Min(m_ScrollPosition.X, 0.0f);
    m_ScrollPosition.Y = Math::Min(m_ScrollPosition.Y, 0.0f);

    m_ContentWidget->Position = m_ScrollPosition;
}

void UIScroll::SetScrollPosition(Float2 const& position)
{
    m_ScrollPosition.X = Math::Min(position.X, 0.0f);
    m_ScrollPosition.Y = Math::Min(position.Y, 0.0f);

    m_ContentWidget->Position = m_ScrollPosition;
}

void UIScroll::MoveHSlider(float dir)
{
    float size = m_HorizontalScrollbarMaxs.X - m_HorizontalScrollbarMins.X;

    if (size > 0.0f)
    {
        if (m_ContentSize.X > m_ViewSize.X)
            m_ScrollPosition.X = m_ContentWidget->Position.X = Math::Max(Math::Min(-dir * m_ContentSize.X / size, 0.0f), -m_ContentSize.X + m_ViewSize.X);
        else
            m_ScrollPosition.X = m_ContentWidget->Position.X = 0;
    }
}

void UIScroll::MoveVSlider(float dir)
{
    float size = m_VerticalScrollbarMaxs.Y - m_VerticalScrollbarMins.Y;

    if (size > 0.0f)
    {
        if (m_ContentSize.Y > m_ViewSize.Y)
            m_ScrollPosition.Y = m_ContentWidget->Position.Y = Math::Max(Math::Min(-dir * m_ContentSize.Y / size, 0.0f), -m_ContentSize.Y + m_ViewSize.Y);
        else
            m_ScrollPosition.Y = m_ContentWidget->Position.Y = 0;
    }
}

bool UIScroll::CanScroll() const
{
    return m_ContentSize.Y > m_ViewSize.Y;
}

void UIScroll::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    if (IsDisabled() || event.Button != VirtualKey::MouseLeftBtn)
        return;

    if (event.Action == InputAction::Pressed)
    {
        if (BvPointInRect(m_VerticalSliderMins, m_VerticalSliderMaxs, GUIManager->CursorPosition))
        {
            float size   = m_VerticalScrollbarMaxs.Y - m_VerticalScrollbarMins.Y;
            m_DragCursor = GUIManager->CursorPosition.Y + m_ScrollPosition.Y / m_ContentSize.Y * size;

            m_State = STATE_MOVE_VSLIDER;
        }
        else if (BvPointInRect(m_VerticalScrollbarMins, m_VerticalScrollbarMaxs, GUIManager->CursorPosition))
        {
            float local = GUIManager->CursorPosition.Y - m_VerticalScrollbarMins.Y;
            float sliderSize = m_VerticalSliderMaxs.Y - m_VerticalSliderMins.Y;

            MoveVSlider(local - sliderSize * 0.5f);
        }
        else if (BvPointInRect(m_HorizontalSliderMins, m_HorizontalSliderMaxs, GUIManager->CursorPosition))
        {
            float size   = m_HorizontalScrollbarMaxs.X - m_HorizontalScrollbarMins.X;
            m_DragCursor = GUIManager->CursorPosition.X + m_ScrollPosition.X / m_ContentSize.X * size;

            m_State = STATE_MOVE_HSLIDER;
        }
        else if (BvPointInRect(m_HorizontalScrollbarMins, m_HorizontalScrollbarMaxs, GUIManager->CursorPosition))
        {
            float local = GUIManager->CursorPosition.X - m_HorizontalScrollbarMins.X;
            float sliderSize = m_HorizontalSliderMaxs.X - m_HorizontalSliderMins.X;

            MoveHSlider(local - sliderSize * 0.5f);
        }
        else
        {
            if (m_bWithButtons)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (BvPointInRect(m_ButtonMins[i], m_ButtonMaxs[i], GUIManager->CursorPosition))
                    {
                        m_PressButton = i;
                        m_State       = (STATE)(STATE_PRESS_BUTTON_LEFT + i);
                        break;
                    }
                }
            }
        }        
    }
    else if (event.Action == InputAction::Released)
    {
        m_PressButton = -1;
        m_State       = STATE_IDLE;

        UIDesktop* desktop = GetDesktop();
        if (desktop)
            desktop->SetFocusWidget(m_ContentWidget);
    }
}

void UIScroll::OnMouseWheelEvent(MouseWheelEvent const& event)
{
    if (event.WheelY < 0)
    {
        ScrollDelta(Float2(0.0f, -20));
    }
    else if (event.WheelY > 0)
    {
        ScrollDelta(Float2(0.0f, 20));
    }
}

void UIScroll::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    switch (m_State)
    {
        case STATE_MOVE_HSLIDER:
            MoveHSlider(GUIManager->CursorPosition.X - m_DragCursor);
            break;
        case STATE_MOVE_VSLIDER:
            MoveVSlider(GUIManager->CursorPosition.Y - m_DragCursor);
            break;
        default:
            break;
    }
}

UIScroll::DRAW UIScroll::GetDrawType(int buttonNum) const
{
    if (IsDisabled())
        return DRAW_DISABLED;

    bool hovered = BvPointInRect(m_ButtonMins[buttonNum], m_ButtonMaxs[buttonNum], GUIManager->CursorPosition);

    return m_PressButton == buttonNum ? DRAW_ACTIVE : (hovered ? DRAW_HOVERED : DRAW_INACTIVE);
}

void UIScroll::DrawButton(Canvas& canvas, int buttonNum)
{
    UIWidgetGeometry buttonGeometry;

    if (m_ButtonDecorator[buttonNum])
    {
        DRAW drawType = GetDrawType(buttonNum);

        buttonGeometry.Mins = m_ButtonMins[buttonNum];
        buttonGeometry.Maxs = m_ButtonMaxs[buttonNum];

        switch (drawType)
        {
            case DRAW_INACTIVE:
                m_ButtonDecorator[buttonNum]->DrawInactive(canvas, buttonGeometry);
                break;
            case DRAW_ACTIVE:
                m_ButtonDecorator[buttonNum]->DrawActive(canvas, buttonGeometry);
                break;
            case DRAW_HOVERED:
                m_ButtonDecorator[buttonNum]->DrawHovered(canvas, buttonGeometry);
                break;
            case DRAW_DISABLED:
                m_ButtonDecorator[buttonNum]->DrawDisabled(canvas, buttonGeometry);
                break;
        }
    }
}

void UIScroll::Draw(Canvas& canvas)
{
    double delta = static_cast<double>(GameApplication::sGetFrameLoop().SysFrameDuration()) * 0.000001;

    const float ScrollSpeed = static_cast<float>(delta) * 200;

    switch (m_State)
    {
        case STATE_PRESS_BUTTON_LEFT:
            if (BvPointInRect(m_ButtonMins[0], m_ButtonMaxs[0], GUIManager->CursorPosition))
                ScrollDelta(Float2(ScrollSpeed, 0.0f));
            break;
        case STATE_PRESS_BUTTON_RIGHT:
            if (BvPointInRect(m_ButtonMins[1], m_ButtonMaxs[1], GUIManager->CursorPosition))
                ScrollDelta(Float2(-ScrollSpeed, 0.0f));
            break;
        case STATE_PRESS_BUTTON_UP:
            if (BvPointInRect(m_ButtonMins[2], m_ButtonMaxs[2], GUIManager->CursorPosition))
                ScrollDelta(Float2(0.0f, ScrollSpeed));
            break;
        case STATE_PRESS_BUTTON_DOWN:
            if (BvPointInRect(m_ButtonMins[3], m_ButtonMaxs[3], GUIManager->CursorPosition))
                ScrollDelta(Float2(0.0f, -ScrollSpeed));
            break;
        default:
            break;
    }

    if (m_bDrawHScroll || m_bDrawVScroll)
    {
        if (!m_SliderBrush)
            m_SliderBrush = GUIManager->DefaultSliderBrush();

        if (!m_ScrollbarBrush)
            m_ScrollbarBrush = GUIManager->DefaultScrollbarBrush();

        if (m_bDrawHScroll)
        {
            Hk::DrawBrush(canvas, m_HorizontalScrollbarMins, m_HorizontalScrollbarMaxs, {}, m_ScrollbarBrush);

            Float2 sliderMins = m_HorizontalSliderMins;
            Float2 sliderMaxs = m_HorizontalSliderMaxs;

            sliderMins.Y += m_SliderPadding;
            sliderMaxs.Y -= m_SliderPadding;

            Hk::DrawBrush(canvas, sliderMins, sliderMaxs, {}, m_SliderBrush);

            if (m_bWithButtons)
            {
                DrawButton(canvas, 0);
                DrawButton(canvas, 1);
            }
        }

        if (m_bDrawVScroll)
        {
            Hk::DrawBrush(canvas, m_VerticalScrollbarMins, m_VerticalScrollbarMaxs, {}, m_ScrollbarBrush);

            Float2 sliderMins = m_VerticalSliderMins;
            Float2 sliderMaxs = m_VerticalSliderMaxs;

            sliderMins.X += m_SliderPadding;
            sliderMaxs.X -= m_SliderPadding;

            Hk::DrawBrush(canvas, sliderMins, sliderMaxs, {}, m_SliderBrush);

            if (m_bWithButtons)
            {
                DrawButton(canvas, 2);
                DrawButton(canvas, 3);
            }
        }
    }
}

HK_NAMESPACE_END
