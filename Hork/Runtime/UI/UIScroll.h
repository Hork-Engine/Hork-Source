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

#pragma once

#include "UIWidget.h"
#include "UIDecorator.h"

HK_NAMESPACE_BEGIN

class UIScroll : public UIWidget
{
    UI_CLASS(UIScroll, UIWidget)

public:
    UIScroll(UIWidget* contentWidget);

    UIScroll& WithButtons(bool bButtons)
    {
        m_bWithButtons = bButtons;
        return *this;
    }

    UIScroll& WithButtonDecorator(UIDecorator* decorator)
    {
        for (int i = 0; i < 4; i++)
            m_ButtonDecorator[i] = decorator;
        return *this;
    }

    UIScroll& WithButtonDecoratorLeft(UIDecorator* decorator)
    {
        m_ButtonDecorator[0] = decorator;
        return *this;
    }

    UIScroll& WithButtonDecoratorRight(UIDecorator* decorator)
    {
        m_ButtonDecorator[1] = decorator;
        return *this;
    }

    UIScroll& WithButtonDecoratorUp(UIDecorator* decorator)
    {
        m_ButtonDecorator[2] = decorator;
        return *this;
    }

    UIScroll& WithButtonDecoratorDown(UIDecorator* decorator)
    {
        m_ButtonDecorator[3] = decorator;
        return *this;
    }

    UIScroll& WithButtonSize(float size)
    {
        m_ButtonSize = Math::Max(size, 0.0f);
        return *this;
    }

    UIScroll& WithSliderBrush(UIBrush* brush)
    {
        m_SliderBrush = brush;
        return *this;
    }

    UIScroll& WithScrollbarBrush(UIBrush* brush)
    {
        m_ScrollbarBrush = brush;
        return *this;
    }

    UIScroll& WithScrollbarWidth(float width)
    {
        m_ScrollbarWidth = Math::Max(width, 0.0f);
        return *this;
    }

    UIScroll& WithScrollbarPadding(float padding)
    {
        m_ScrollbarPadding = Math::Max(padding, 0.0f);
        return *this;
    }

    UIScroll& WithSliderPadding(float padding)
    {
        m_SliderPadding = padding;
        return *this;
    }

    UIScroll& WithAutoHScroll(bool bAutoScroll)
    {
        m_bAutoHScroll = bAutoScroll;
        return *this;
    }

    UIScroll& WithAutoVScroll(bool bAutoScroll)
    {
        m_bAutoVScroll = bAutoScroll;
        return *this;
    }

    UIWidget* GetContentWidget()
    {
        return m_ContentWidget;
    }

    void ScrollHome();

    void ScrollEnd();

    void ScrollDelta(Float2 const& delta);

    void SetScrollPosition(Float2 const& position);

    Float2 GetScrollPosition() const
    {
        return m_ScrollPosition;
    }

    Float2 const& GetViewSize() const
    {
        return m_ViewSize;
    }

    bool CanScroll() const;

protected:
    void OnMouseWheelEvent(struct MouseWheelEvent const& event) override;
    void OnMouseButtonEvent(struct MouseButtonEvent const& event) override;
    void OnMouseMoveEvent(struct MouseMoveEvent const& event) override;
    void Draw(Canvas& canvas) override;

private:
    enum DRAW
    {
        DRAW_INACTIVE,
        DRAW_ACTIVE,
        DRAW_HOVERED,
        DRAW_DISABLED
    };

    DRAW GetDrawType(int buttonNum) const;

    void MoveHSlider(float dir);
    void MoveVSlider(float dir);

    void DrawButton(Canvas& canvas, int buttonNum);

    void DoMeasureLayout(Float2 const& size);
    void DoArrangeChildren();

    Ref<UIWidget>    m_ContentWidget;
    Ref<UIDecorator> m_ButtonDecorator[4];
    Ref<UIBrush>     m_SliderBrush; // FIXME: replace with Decorator?
    Ref<UIBrush>     m_ScrollbarBrush; // FIXME: replace with Decorator?
    float             m_ScrollbarWidth   = 12;
    float             m_ScrollbarPadding = 0;
    float             m_SliderPadding    = 2;
    Float2            m_VerticalScrollbarMins;
    Float2            m_VerticalScrollbarMaxs;
    Float2            m_HorizontalScrollbarMins;
    Float2            m_HorizontalScrollbarMaxs;
    Float2            m_ButtonMins[4];
    Float2            m_ButtonMaxs[4];
    float             m_ButtonSize = 20;
    Float2            m_HorizontalSliderMins;
    Float2            m_HorizontalSliderMaxs;
    Float2            m_VerticalSliderMins;
    Float2            m_VerticalSliderMaxs;
    Float2            m_ScrollPosition;
    Float2            m_ContentSize;
    Float2            m_ViewSize;
    int               m_PressButton = -1;
    float             m_DragCursor{};
    bool              m_bWithButtons{true};
    bool              m_bAutoHScroll{true};
    bool              m_bAutoVScroll{true};
    bool              m_bDrawHScroll{false};
    bool              m_bDrawVScroll{false};

    enum STATE
    {
        STATE_IDLE,
        STATE_MOVE_HSLIDER,
        STATE_MOVE_VSLIDER,
        STATE_PRESS_BUTTON_LEFT,
        STATE_PRESS_BUTTON_RIGHT,
        STATE_PRESS_BUTTON_UP,
        STATE_PRESS_BUTTON_DOWN,
    };

    STATE m_State{STATE_IDLE};
};

HK_NAMESPACE_END
