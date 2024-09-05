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

#include <Hork/ECS/ECS.h>
#include <Hork/World/Modules/Render/WorldRenderView.h>

HK_NAMESPACE_BEGIN

class Actor_PlayerController;

class UIViewport : public UIWidget
{
    UI_CLASS(UIViewport, UIWidget)

public:
    RoundingDesc     Rounding;
    Color4           TintColor;
    CANVAS_COMPOSITE Composite = CANVAS_COMPOSITE_COPY;

    UIViewport();

    UIViewport& SetWorldRenderView(WorldRenderView* worldRenderView);
    UIViewport& WithRounding(RoundingDesc const& rounding);
    UIViewport& WithTint(Color4 const& tintColor);
    UIViewport& WithComposite(CANVAS_COMPOSITE composite);

    void Draw(Canvas& canvas) override;

protected:
    void OnKeyEvent(struct KeyEvent const& event) override;

    void OnMouseButtonEvent(struct MouseButtonEvent const& event) override;

    void OnMouseWheelEvent(struct MouseWheelEvent const& event) override;

    void OnMouseMoveEvent(struct MouseMoveEvent const& event) override;

    void OnGamepadButtonEvent(struct GamepadKeyEvent const& event) override;

    void OnGamepadAxisMotionEvent(struct GamepadAxisMotionEvent const& event) override;

    void OnCharEvent(struct CharEvent const& event) override;

    void OnFocusLost() override;

    void OnFocusReceive() override;

private:
    void UpdateViewSize();
    void Clear(Canvas& canvas);

    Ref<WorldRenderView>  m_WorldRenderView;
    int                   m_ViewWidth{};
    int                   m_ViewHeight{};
};

extern bool GUILockViewportScaling;

HK_NAMESPACE_END
