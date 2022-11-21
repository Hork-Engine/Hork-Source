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
#include "UIShortcut.h"

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

    void Draw(ACanvas& cv);

    void GenerateKeyEvents(struct SKeyEvent const& event, double timeStamp);

    void GenerateMouseButtonEvents(struct SMouseButtonEvent const& event, double timeStamp);

    void GenerateMouseWheelEvents(struct SMouseWheelEvent const& event, double timeStamp);

    void GenerateMouseMoveEvents(struct SMouseMoveEvent const& event, double timeStamp);

    void GenerateJoystickButtonEvents(struct SJoystickButtonEvent const& event, double timeStamp);

    void GenerateJoystickAxisEvents(struct SJoystickAxisEvent const& event, double timeStamp);

    void GenerateCharEvents(struct SCharEvent const& event, double timeStamp);

    void StartDragging(UIWidget* widget);

    void CancelDragging();

    UIWidget* GetExclusive();

    bool HandleDraggingWidget();

    UIWidgetGeometry          m_Geometry;
    TVector<UIWidget*>        m_Widgets;
    TWeakRef<UIWidget>        m_FocusWidget;
    TRef<UIWidget>            m_MouseFocusWidget;
    TRef<UIWidget>            m_Popup;
    TRef<UIWidget>            m_PendingDrag;
    TRef<UIWidget>            m_FullscreenWidget;
    TRef<UIWidget>            m_DraggingWidget;
    TRef<UIWidget>            m_MouseClickWidget;
    TRef<UIShortcutContainer> m_ShortcutContainer;
    TRef<UIBrush>             m_Wallpaper;
    uint64_t                  m_MouseClickTime{};
    Float2                    m_MouseClickPos{};
    Float2                    m_DraggingCursor{};
    Float2                    m_DraggingWidgetPos{};
};
