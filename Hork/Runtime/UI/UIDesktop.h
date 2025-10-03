/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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
#include "UIShortcut.h"

HK_NAMESPACE_BEGIN

class UIDesktop : public UIObject
{
    UI_CLASS(UIDesktop, UIObject)

    friend class UIManager;
    friend class UIWidget;

public:
    UIDesktop();
    virtual ~UIDesktop();

    void SetWallpaper(UIBrush* brush)
    {
        m_Wallpaper = brush;
    }

    void SetShortcuts(UIShortcutContainer* shortcutContainer);

    void SetDragWidget(UIWidget* widget);

    void SetFocusWidget(UIWidget* widget);

    void SetFullscreenWidget(UIWidget* widget);

    void OpenPopupWidget(UIWidget* widget, Float2 const& position);

    void ClosePopupWidget();

    void AddWidget(UIWidget* widget);

    void RemoveWidget(UIWidget* widget);

    UIWidget* GetFocusWidget() const { return m_FocusWidget; }

    UIWidget* Trace(float x, float y);

private:
    void UpdateGeometry(float w, float h);

    void Draw(Canvas& cv);

    void GenerateKeyEvents(struct KeyEvent const& event);

    void GenerateMouseButtonEvents(struct MouseButtonEvent const& event);

    void GenerateMouseWheelEvents(struct MouseWheelEvent const& event);

    void GenerateMouseMoveEvents(struct MouseMoveEvent const& event);

    void GeneratenGamepadButtonEvents(GamepadKeyEvent const& event);

    void GenerateGamepadAxisMotionEvents(GamepadAxisMotionEvent const& event);

    void GenerateCharEvents(struct CharEvent const& event);

    void StartDragging(UIWidget* widget);

    void CancelDragging();

    UIWidget* GetExclusive();

    bool HandleDraggingWidget();

    UIWidgetGeometry          m_Geometry;
    Vector<UIWidget*>        m_Widgets;
    WeakRef<UIWidget>        m_FocusWidget;
    Ref<UIWidget>            m_MouseFocusWidget;
    Ref<UIWidget>            m_Popup;
    Ref<UIWidget>            m_PendingDrag;
    Ref<UIWidget>            m_FullscreenWidget;
    Ref<UIWidget>            m_DraggingWidget;
    Ref<UIWidget>            m_MouseClickWidget;
    Ref<UIShortcutContainer> m_ShortcutContainer;
    Ref<UIBrush>             m_Wallpaper;
    uint64_t                  m_MouseClickTime{};
    Float2                    m_MouseClickPos{};
    Float2                    m_DraggingCursor{};
    Float2                    m_DraggingWidgetPos{};
};

HK_NAMESPACE_END
