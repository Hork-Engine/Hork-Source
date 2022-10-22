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

#include "UIDesktop.h"
#include "UIConsole.h"

enum UI_CURSOR_MODE
{
    UI_CURSOR_MODE_AUTO,
    UI_CURSOR_MODE_FORCE_ENABLED,
    UI_CURSOR_MODE_FORCE_DISABLED
};

class UIManager
{
public:
    UI_CURSOR_MODE CursorMode = UI_CURSOR_MODE_AUTO;

    Float2 CursorPosition;
    bool   bCursorVisible = true;

    /** Allow to drop down the console */
    bool bAllowConsole = true;

    TRef<UIBrush> ConsoleBackground;

    TWeakRef<UIWidget> HoveredWidget;

    UIManager(RenderCore::IGenericWindow* mainWindow);
    virtual ~UIManager();

    UICursor* ArrowCursor() const;

    void AddDesktop(UIDesktop* desktop);
    void RemoveDesktop(UIDesktop* desktop);
    void SetActiveDesktop(UIDesktop* desktop);
    UIDesktop* GetActiveDesktop() { return m_ActiveDesktop; }

    void Update(float timeStep);

    void Draw(ACanvas& cv);

    void GenerateKeyEvents(struct SKeyEvent const& event, double timeStamp, ACommandContext& commandCtx, ACommandProcessor& commandProcessor);

    void GenerateMouseButtonEvents(struct SMouseButtonEvent const& event, double timeStamp);

    void GenerateMouseWheelEvents(struct SMouseWheelEvent const& event, double timeStamp);

    void GenerateMouseMoveEvents(struct SMouseMoveEvent const& event, double timeStamp);

    void GenerateJoystickButtonEvents(struct SJoystickButtonEvent const& event, double timeStamp);

    void GenerateJoystickAxisEvents(struct SJoystickAxisEvent const& event, double timeStamp);

    void GenerateCharEvents(struct SCharEvent const& event, double timeStamp);

private:
    void DrawCursor(ACanvas& cv);

    TRef<RenderCore::IGenericWindow> m_MainWindow;
    UIConsole                        m_Console;
    TVector<TRef<UIDesktop>>         m_Desktops;
    TRef<UIDesktop>                  m_ActiveDesktop;
    TRef<UICursor>                   m_Cursor;
    mutable TRef<UICursor>           m_ArrowCursor;
};

extern UIManager* GUIManager;
