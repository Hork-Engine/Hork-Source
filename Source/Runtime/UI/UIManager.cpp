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

#include "UIManager.h"
#include <Runtime/FrameLoop.h>

static AConsoleVar ui_SimulateCursorBallistics("ui_SimulateCursorBallistics"s, "1"s);

UIManager* GUIManager = nullptr;

UIManager::UIManager(RenderCore::IGenericWindow* mainWindow) :
    m_MainWindow(mainWindow)
{
    GUIManager = this;

    m_Console.ReadStoryLines();
}

UIManager::~UIManager()
{
    m_Console.WriteStoryLines();
}

UICursor* UIManager::ArrowCursor() const
{
    if (!m_ArrowCursor)
        m_ArrowCursor = UINew(UIDefaultCursor);

    return m_ArrowCursor;
}

void UIManager::AddDesktop(UIDesktop* desktop)
{
    if (m_Desktops.IndexOf(TRef<UIDesktop>(desktop)) != Core::NPOS)
        return;

    m_Desktops.Add() = desktop;

    if (!m_ActiveDesktop)
        m_ActiveDesktop = desktop;
}

void UIManager::RemoveDesktop(UIDesktop* desktop)
{
    auto index = m_Desktops.IndexOf(TRef<UIDesktop>(desktop));
    if (index == Core::NPOS)
        return;

    m_Desktops.Remove(index);

    if (m_ActiveDesktop == desktop)
    {
        if (!m_Desktops.IsEmpty())
            m_ActiveDesktop = m_Desktops[Math::Max((int)index - 1, 0)];
        else
            m_ActiveDesktop.Reset();
    }
}

void UIManager::SetActiveDesktop(UIDesktop* desktop)
{
    m_ActiveDesktop = desktop;
}

void UIManager::Update(float timeStep)
{
    SVideoMode const& videoMode = m_MainWindow->GetVideoMode();

    m_Console.SetFullscreen(m_ActiveDesktop ? false : true);
    m_Console.Update(timeStep);

    switch (CursorMode)
    {
        case UI_CURSOR_MODE_AUTO:
            if (!videoMode.bFullscreen && m_Console.IsActive())
            {
                Platform::SetCursorEnabled(true);
            }
            else
            {
                Platform::SetCursorEnabled(false);
            }
            break;
        case UI_CURSOR_MODE_FORCE_ENABLED:
            Platform::SetCursorEnabled(true);
            break;
        case UI_CURSOR_MODE_FORCE_DISABLED:
            Platform::SetCursorEnabled(false);
            break;
        default:
            HK_ASSERT(0);
    }

    if (m_ActiveDesktop)
    {
        m_ActiveDesktop->UpdateGeometry(videoMode.FramebufferWidth, videoMode.FramebufferHeight);

        auto widget = m_ActiveDesktop->Trace(CursorPosition.X, CursorPosition.Y);
        if (HoveredWidget && (widget == nullptr || HoveredWidget != widget))
        {
            HoveredWidget->ForwardHoverEvent(false);
        }

        if (HoveredWidget != widget)
        {
            HoveredWidget = widget;

            if (widget)
            {
                widget->ForwardHoverEvent(true);
                m_Cursor = widget->Cursor;
            }
            else
            {
                m_Cursor = ArrowCursor();
            }
        }
    }
}

void UIManager::GenerateKeyEvents(SKeyEvent const& event, double timeStamp, ACommandContext& commandCtx, ACommandProcessor& commandProcessor)
{
    if (m_Console.IsActive() || bAllowConsole)
    {
        m_Console.OnKeyEvent(event, commandCtx, commandProcessor);

        if (!m_Console.IsActive() && event.Key == KEY_GRAVE_ACCENT)
        {
            // Console just closed
            return;
        }
    }

    if (m_Console.IsActive() && event.Action != IA_RELEASE)
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateKeyEvents(event, timeStamp);
}

void UIManager::GenerateMouseButtonEvents(SMouseButtonEvent const& event, double timeStamp)
{
    if (m_Console.IsActive() && event.Action != IA_RELEASE)
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateMouseButtonEvents(event, timeStamp);
}

void UIManager::GenerateMouseWheelEvents(SMouseWheelEvent const& event, double timeStamp)
{
    m_Console.OnMouseWheelEvent(event);
    if (m_Console.IsActive())
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateMouseWheelEvents(event, timeStamp);
}

void UIManager::GenerateMouseMoveEvents(SMouseMoveEvent const& event, double timeStamp)
{
    SVideoMode const& videoMode = m_MainWindow->GetVideoMode();

    if (Platform::IsCursorEnabled())
    {
        int    x, y;

        Platform::GetCursorPosition(x, y);

        CursorPosition.X = Math::Clamp(x, 0, videoMode.FramebufferWidth - 1);
        CursorPosition.Y = Math::Clamp(y, 0, videoMode.FramebufferHeight - 1);
    }
    else
    {
        // Simulate ballistics
        if (ui_SimulateCursorBallistics)
        {
            CursorPosition.X += event.X / videoMode.RefreshRate * videoMode.DPI_X;
            CursorPosition.Y -= event.Y / videoMode.RefreshRate * videoMode.DPI_Y;
        }
        else
        {
            CursorPosition.X += event.X;
            CursorPosition.Y -= event.Y;
        }
        CursorPosition = Math::Clamp(CursorPosition, Float2(0.0f), Float2(videoMode.FramebufferWidth - 1, videoMode.FramebufferHeight - 1));
    }

    if (m_Console.IsActive())
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateMouseMoveEvents(event, timeStamp);
}

void UIManager::GenerateJoystickButtonEvents(SJoystickButtonEvent const& event, double timeStamp)
{
    if (m_Console.IsActive() && event.Action != IA_RELEASE)
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateJoystickButtonEvents(event, timeStamp);
}

void UIManager::GenerateJoystickAxisEvents(SJoystickAxisEvent const& event, double timeStamp)
{
    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateJoystickAxisEvents(event, timeStamp);
}

void UIManager::GenerateCharEvents(SCharEvent const& event, double timeStamp)
{
    m_Console.OnCharEvent(event);
    if (m_Console.IsActive())
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateCharEvents(event, timeStamp);
}

void UIManager::Draw(ACanvas& cv)
{
    if (m_ActiveDesktop)
        m_ActiveDesktop->Draw(cv);


    if (!Platform::IsCursorEnabled())
    {
        DrawCursor(cv);
    }

    m_Console.Draw(cv, ConsoleBackground);
}

void UIManager::DrawCursor(ACanvas& cv)
{
    if (!bCursorVisible)
        return;

    if (m_Cursor)
    {
        cv.ResetScissor();
        m_Cursor->Draw(cv, CursorPosition);
    }
}
