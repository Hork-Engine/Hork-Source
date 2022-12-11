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

#pragma once

#include "UIWidget.h"

class UIWindow : public UIWidget
{
    UI_CLASS(UIWindow, UIWidget)

public:
    enum WINDOW_AREA
    {
        WA_NONE,
        WA_BACK,
        WA_CLIENT,
        WA_CAPTION,
        WA_LEFT,
        WA_TOP,
        WA_RIGHT,
        WA_BOTTOM,
        WA_TOP_LEFT,
        WA_TOP_RIGHT,
        WA_BOTTOM_LEFT,
        WA_BOTTOM_RIGHT
    };

    enum WINDOW_STATE
    {
        WS_NORMAL,
        WS_MAXIMIZED
    };

    float            CaptionHeight = 24;
    TRef<UIHitShape> CaptionHitShape;
    WINDOW_STATE     WindowState = WS_NORMAL;
    bool             bResizable{true};
    bool             bDropShadow{true};

    UIWindow(UIWidget* caption, UIWidget* central);

    UIWindow& WithCaptionHeight(float captionHeight)
    {
        CaptionHeight = captionHeight;
        return *this;
    }

    UIWindow& WithCaptionHitShape(UIHitShape* hitShape)
    {
        CaptionHitShape = hitShape;
        return *this;
    }

    UIWindow& WithWindowState(WINDOW_STATE windowState)
    {
        WindowState = windowState;
        return *this;
    }

    UIWindow& WithResizable(bool resizable)
    {
        bResizable = resizable;
        return *this;
    }

    UIWindow& WithDropShadow(bool dropShadow)
    {
        bDropShadow = dropShadow;
        return *this;
    }

    void SetNormal()
    {
        WindowState = WS_NORMAL;
    }

    void SetMaximized()
    {
        WindowState = WS_MAXIMIZED;
    }

    bool IsMaximized() const
    {
        return WindowState == WS_MAXIMIZED;
    }

    UIWidget* GetCaptionWidget() { return m_Caption; }
    UIWidget* GetCentralWidget() { return m_Central; }

    WINDOW_AREA HitTestWindowArea(float x, float y) const;

    bool CaptionHitTest(float x, float y) const;

    void Draw(Canvas& canvas) override;

private:
    struct WindowLayout : UIBaseLayout
    {
        UIWindow* Self;

        WindowLayout(UIWindow* self);

        Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

        void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;
    };

    TRef<UIWidget> m_Caption;
    TRef<UIWidget> m_Central;
};


UIWindow* UIMakeWindow(StringView captionText, UIWidget* centralWidget);
