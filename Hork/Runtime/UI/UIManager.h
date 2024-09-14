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

#include "UIDesktop.h"
#include "UIConsole.h"

HK_NAMESPACE_BEGIN

enum UI_CURSOR_MODE
{
    UI_CURSOR_MODE_AUTO,
    UI_CURSOR_MODE_FORCE_ENABLED,
    UI_CURSOR_MODE_FORCE_DISABLED
};

class UIManager final
{
public:
    UI_CURSOR_MODE CursorMode = UI_CURSOR_MODE_AUTO;

    Float2 CursorPosition;
    bool   bCursorVisible = true;

    /// Allow to drop down the console
    bool bAllowConsole = true;

    Ref<UIBrush> ConsoleBackground;

    WeakRef<UIWidget> HoveredWidget;

    UIManager(RHI::IGenericWindow* mainWindow);
    ~UIManager();

    RHI::IGenericWindow* GetGenericWindow()
    {
        return m_MainWindow;
    }

    UICursor* ArrowCursor() const;
    UICursor* TextInputCursor() const;

    UIBrush* DefaultSliderBrush() const;
    UIBrush* DefaultScrollbarBrush() const;

    void AddDesktop(UIDesktop* desktop);
    void RemoveDesktop(UIDesktop* desktop);
    void SetActiveDesktop(UIDesktop* desktop);
    UIDesktop* GetActiveDesktop() { return m_ActiveDesktop; }

    void UpConsole();
    void DownConsole();

    void OpenPopupWidget(UIWidget* widget);
    void OpenPopupWidget(UIWidget* widget, Float2 const& position);
    void ClosePopupWidget();

    void SetInsertMode(bool bInsertMode);
    bool IsInsertMode() const;

    void Tick(float timeStep);

    void Draw(Canvas& cv);

    void GenerateKeyEvents(struct KeyEvent const& event, CommandContext& commandCtx, CommandProcessor& commandProcessor);

    void GenerateMouseButtonEvents(struct MouseButtonEvent const& event);

    void GenerateMouseWheelEvents(struct MouseWheelEvent const& event);

    void GenerateMouseMoveEvents(struct MouseMoveEvent const& event);

    void GeneratenGamepadButtonEvents(struct GamepadKeyEvent const& event);

    void GenerateGamepadAxisMotionEvents(struct GamepadAxisMotionEvent const& event);

    void GenerateCharEvents(struct CharEvent const& event);

private:
    void DrawCursor(Canvas& cv);

    Ref<RHI::IGenericWindow> m_MainWindow;
    UIConsole                        m_Console;
    Vector<Ref<UIDesktop>>         m_Desktops;
    Ref<UIDesktop>                  m_ActiveDesktop;
    Ref<UICursor>                   m_Cursor;
    mutable Ref<UICursor>           m_ArrowCursor;
    mutable Ref<UICursor>           m_TextInputCursor;
    mutable Ref<UIBrush>            m_SliderBrush;
    mutable Ref<UIBrush>            m_ScrollbarBrush;
    bool                             m_bInsertMode{};
    WeakRef<UIWidget>               m_TooltipWidget;
    float                            m_TooltipTime{};
    Float2                           m_TooltipPosition;
};

extern UIManager* GUIManager;

HK_NAMESPACE_END
