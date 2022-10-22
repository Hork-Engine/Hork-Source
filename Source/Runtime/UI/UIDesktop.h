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

public:
    TRef<UIBrush> Background;

    UIDesktop();
    virtual ~UIDesktop();

    void SetDragWidget(UIWidget* widget);

    void SetShortcuts(UIShortcutContainer* shortcutContainer);

    void SetFullscreenWidget(UIWidget* widget);

    void AddWidget(UIWidget* widget);

    void RemoveWidget(UIWidget* widget);

    UIWidget* Trace(float x, float y);

    void UpdateGeometry(float w, float h);

//    void SetTopWidget(UIWidget* widget);

    void SetFocusWidget(UIWidget* widget);

    void ExecuteEvents();

    void Draw(ACanvas& cv);

    void GenerateKeyEvents(struct SKeyEvent const& event, double timeStamp);

    void GenerateMouseButtonEvents(struct SMouseButtonEvent const& event, double timeStamp);

    void GenerateMouseWheelEvents(struct SMouseWheelEvent const& event, double timeStamp);

    void GenerateMouseMoveEvents(struct SMouseMoveEvent const& event, double timeStamp);

    void GenerateJoystickButtonEvents(struct SJoystickButtonEvent const& event, double timeStamp);

    void GenerateJoystickAxisEvents(struct SJoystickAxisEvent const& event, double timeStamp);

    void GenerateCharEvents(struct SCharEvent const& event, double timeStamp);

    void CancelDragging();


//private:

    UIWidget* GetExclusive();

    bool HandleDraggingWidget();

//private:
    void StartDragWidget(UIWidget* widget);

    UIWidgetGeometry      m_Geometry;
    TVector<UIWidget*>    m_Widgets;

    TRef<UIWidget>      m_MouseFocusWidget;
    TWeakRef<UIWidget>    m_FocusWidget;

    TRef<UIShortcutContainer> m_ShortcutContainer;

    TRef<UIWidget> m_PendingDrag;


        //TRef<WMenuPopup>           m_Popup;
    TRef<UIWidget>                 m_FullscreenWidget;
    TRef<UIWidget>              m_DraggingWidget;
    TRef<UIWidget>              m_MouseClickWidget;
    uint64_t                       m_MouseClickTime{};
    Float2                         m_MouseClickPos{};
    Float2                         m_DraggingCursor{};
    Float2                         m_DraggingWidgetPos{};
};
