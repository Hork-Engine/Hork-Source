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

#pragma once

#include "UIWidget.h"

HK_NAMESPACE_BEGIN

class Actor_PlayerController;

class UIViewport : public UIWidget
{
    UI_CLASS(UIViewport, UIWidget)

public:
    RoundingDesc     Rounding;
    Color4           TintColor;
    CANVAS_COMPOSITE Composite = CANVAS_COMPOSITE_COPY;

    UIViewport(Actor_PlayerController* playerController = nullptr);

    UIViewport& SetPlayerController(Actor_PlayerController* playerController);
    UIViewport& WithRounding(RoundingDesc const& rounding);
    UIViewport& WithTint(Color4 const& tintColor);
    UIViewport& WithComposite(CANVAS_COMPOSITE composite);

    void Draw(Canvas& canvas) override;

protected:
    void OnKeyEvent(struct KeyEvent const& event, double timeStamp) override;

    void OnMouseButtonEvent(struct MouseButtonEvent const& event, double timeStamp) override;

    void OnMouseWheelEvent(struct MouseWheelEvent const& event, double timeStamp) override;

    void OnMouseMoveEvent(struct MouseMoveEvent const& event, double timeStamp) override;

    void OnJoystickButtonEvent(struct JoystickButtonEvent const& event, double timeStamp) override;

    void OnJoystickAxisEvent(struct JoystickAxisEvent const& event, double timeStamp) override;

    void OnCharEvent(struct CharEvent const& event, double timeStamp) override;

    void OnFocusLost() override;

    void OnFocusReceive() override;

private:
    void UpdateViewSize();

    TRef<Actor_PlayerController> m_PlayerController;
    int                     m_ViewWidth{};
    int                     m_ViewHeight{};
};

extern bool GUILockViewportScaling;

HK_NAMESPACE_END
