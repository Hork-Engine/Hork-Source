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

#include "UIManager.h"
#include <Engine/GameApplication/FrameLoop.h>
#include <Engine/Core/Profiler.h>
#include <Engine/Core/Platform.h>

HK_NAMESPACE_BEGIN

static ConsoleVar ui_SimulateCursorBallistics("ui_SimulateCursorBallistics"s, "1"s);

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

UICursor* UIManager::TextInputCursor() const
{
    if (!m_TextInputCursor)
        m_TextInputCursor = UINew(UIDefaultCursor).WithDrawCursor(DRAW_CURSOR_TEXT_INPUT);

    return m_TextInputCursor;
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

void UIManager::SetInsertMode(bool bInsertMode)
{
    m_bInsertMode = bInsertMode;
}

bool UIManager::IsInsertMode() const
{
    return m_bInsertMode;
}

static Float2 CalcTooltipPosition(UIWidget* widget)
{
    UIWidget* tooltipWidget = widget->Tooltip;
    if (!tooltipWidget)
        return {};

    UIWidgetGeometry const& geometry = widget->m_Geometry;

    Float2 tooltipSize = tooltipWidget->MeasureLayout(true, true, tooltipWidget->Size);

    const float padding = 2;

    switch (widget->TooltipPosition)
    {
        case UI_TOOLTIP_POSITION_AT_CURSOR:
            return GUIManager->CursorPosition;

        case UI_TOOLTIP_POSITION_LEFT_TOP_BOUNDARY:
            return Float2(geometry.Mins.X - tooltipSize.X - padding, geometry.Mins.Y);

        case UI_TOOLTIP_POSITION_LEFT_CENTER_BOUNDARY:
            return Float2(geometry.Mins.X - tooltipSize.X - padding, (geometry.Mins.Y + geometry.Maxs.Y - tooltipSize.Y) * 0.5f);

        case UI_TOOLTIP_POSITION_LEFT_BOTTOM_BOUNDARY:
            return Float2(geometry.Mins.X - tooltipSize.X - padding, geometry.Maxs.Y - tooltipSize.Y);

        case UI_TOOLTIP_POSITION_RIGHT_TOP_BOUNDARY:
            return Float2(geometry.Maxs.X + padding, geometry.Mins.Y);

        case UI_TOOLTIP_POSITION_RIGHT_CENTER_BOUNDARY:
            return Float2(geometry.Maxs.X + padding, (geometry.Mins.Y + geometry.Maxs.Y - tooltipSize.Y) * 0.5f);

        case UI_TOOLTIP_POSITION_RIGHT_BOTTOM_BOUNDARY:
            return Float2(geometry.Maxs.X + padding, geometry.Maxs.Y - tooltipSize.Y);

        case UI_TOOLTIP_POSITION_TOP_LEFT_BOUNDARY:
            return Float2(geometry.Mins.X, geometry.Mins.Y - tooltipSize.Y - padding);

        case UI_TOOLTIP_POSITION_TOP_CENTER_BOUNDARY:
            return Float2((geometry.Mins.X + geometry.Maxs.X - tooltipSize.X) * 0.5f, geometry.Mins.Y - tooltipSize.Y - padding);

        case UI_TOOLTIP_POSITION_TOP_RIGHT_BOUNDARY:
            return Float2(geometry.Maxs.X - tooltipSize.X, geometry.Mins.Y - tooltipSize.Y - padding);

        case UI_TOOLTIP_POSITION_BOTTOM_LEFT_BOUNDARY:
            return Float2(geometry.Mins.X, geometry.Maxs.Y + padding);

        case UI_TOOLTIP_POSITION_BOTTOM_CENTER_BOUNDARY:
            return Float2((geometry.Mins.X + geometry.Maxs.X - tooltipSize.X) * 0.5f, geometry.Maxs.Y + padding);

        case UI_TOOLTIP_POSITION_BOTTOM_RIGHT_BOUNDARY:
            return Float2(geometry.Maxs.X - tooltipSize.X, geometry.Maxs.Y + padding);

        case UI_TOOLTIP_POSITION_TOP_LEFT_CORNER:
            return Float2(geometry.Mins.X - tooltipSize.X - padding, geometry.Mins.Y - tooltipSize.Y - padding);

        case UI_TOOLTIP_POSITION_TOP_RIGHT_CORNER:
            return Float2(geometry.Maxs.X + padding, geometry.Mins.Y - tooltipSize.Y - padding);

        case UI_TOOLTIP_POSITION_BOTTOM_LEFT_CORNER:
            return Float2(geometry.Mins.X - tooltipSize.X - padding, geometry.Maxs.Y + padding);

        case UI_TOOLTIP_POSITION_BOTTOM_RIGHT_CORNER:
            return Float2(geometry.Maxs.X + padding, geometry.Maxs.Y + padding);
    }
    return {};
}

void UIManager::Tick(float timeStep)
{
    HK_PROFILER_EVENT("Tick UI");

    DisplayVideoMode const& videoMode = m_MainWindow->GetVideoMode();

    m_Console.SetFullscreen(m_ActiveDesktop ? false : true);
    m_Console.Update(timeStep);

    switch (CursorMode)
    {
        case UI_CURSOR_MODE_AUTO:
            if (!videoMode.bFullscreen && m_Console.IsActive())
            {
                Core::SetCursorEnabled(true);
            }
            else
            {
                Core::SetCursorEnabled(false);
            }
            break;
        case UI_CURSOR_MODE_FORCE_ENABLED:
            Core::SetCursorEnabled(true);
            break;
        case UI_CURSOR_MODE_FORCE_DISABLED:
            Core::SetCursorEnabled(false);
            break;
        default:
            HK_ASSERT(0);
    }

    if (m_ActiveDesktop)
    {
        Float2 desktopSize(videoMode.FramebufferWidth, videoMode.FramebufferHeight);

        m_ActiveDesktop->UpdateGeometry(desktopSize.X, desktopSize.Y);

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

                m_TooltipWidget = widget->Tooltip;
                m_TooltipTime = widget->TooltipTime;
                m_TooltipPosition = CalcTooltipPosition(widget);
            }
            else
            {
                m_Cursor = ArrowCursor();
                m_TooltipWidget.Reset();
            }
        }

        if (m_TooltipWidget)
        {
            m_TooltipTime -= timeStep;
            if (m_TooltipTime < 0)
            {
                // Update geometry for Tooltip widget

                Float2 size = m_TooltipWidget->MeasureLayout(true, true, m_TooltipWidget->Size);

                for (int i = 0; i < 2; i++)
                {
                    if (m_TooltipPosition[i] + size[i] > desktopSize[i])
                    {
                        m_TooltipPosition[i] = desktopSize[i] - size[i];
                        if (m_TooltipPosition[i] < 0)
                            m_TooltipPosition[i] = 0;
                    }
                }

                m_TooltipWidget->m_Geometry.Mins = m_TooltipPosition;
                m_TooltipWidget->m_Geometry.Maxs = m_TooltipPosition + size;

                m_TooltipWidget->ArrangeChildren(true, true);
            }
        }
    }
}

void UIManager::GenerateKeyEvents(KeyEvent const& event, CommandContext& commandCtx, CommandProcessor& commandProcessor)
{
    if (bAllowConsole)
    {
        if (event.Action == IA_PRESS)
        {
            if (event.Key == KEY_GRAVE_ACCENT)
            {
                m_Console.Toggle();
                return;
            }
            if (event.Key == KEY_ESCAPE)
            {
                if (m_Console.IsActive())
                {
                    m_Console.Up();
                    return;
                }
            }
        }

        if (m_Console.IsActive())
        {
            m_Console.OnKeyEvent(event, commandCtx, commandProcessor);
        }
    }
    else
    {
        m_Console.Up();
    }

    if (m_Console.IsActive() && event.Action != IA_RELEASE)
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateKeyEvents(event);
}

void UIManager::GenerateMouseButtonEvents(MouseButtonEvent const& event)
{
    if (m_Console.IsActive() && event.Action != IA_RELEASE)
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateMouseButtonEvents(event);
}

void UIManager::GenerateMouseWheelEvents(MouseWheelEvent const& event)
{
    if (m_Console.IsActive())
    {
        m_Console.OnMouseWheelEvent(event);
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateMouseWheelEvents(event);
}

void UIManager::GenerateMouseMoveEvents(MouseMoveEvent const& event)
{
    DisplayVideoMode const& videoMode = m_MainWindow->GetVideoMode();

    if (Core::IsCursorEnabled())
    {
        int    x, y;

        Core::GetCursorPosition(x, y);

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
        m_ActiveDesktop->GenerateMouseMoveEvents(event);
}

void UIManager::GenerateJoystickButtonEvents(JoystickButtonEvent const& event)
{
    if (m_Console.IsActive() && event.Action != IA_RELEASE)
    {
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateJoystickButtonEvents(event);
}

void UIManager::GenerateJoystickAxisEvents(JoystickAxisEvent const& event)
{
    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateJoystickAxisEvents(event);
}

void UIManager::GenerateCharEvents(CharEvent const& event)
{
    if (event.UnicodeCharacter == '`')
        return;

    if (m_Console.IsActive())
    {
        m_Console.OnCharEvent(event);
        return;
    }

    if (m_ActiveDesktop)
        m_ActiveDesktop->GenerateCharEvents(event);
}

void UIManager::Draw(Canvas& cv)
{
    DisplayVideoMode const& videoMode = m_MainWindow->GetVideoMode();

    if (m_ActiveDesktop)
        m_ActiveDesktop->Draw(cv);

    if (m_TooltipWidget && m_TooltipTime < 0)
    {
        Float2 clipMins(0.0f);
        Float2 clipMaxs(videoMode.FramebufferWidth, videoMode.FramebufferHeight);

        m_TooltipWidget->Draw(cv, clipMins, clipMaxs, 1.0f);
    }

    if (!Core::IsCursorEnabled())
    {
        DrawCursor(cv);
    }

    m_Console.Draw(cv, ConsoleBackground, videoMode.FramebufferWidth, videoMode.FramebufferHeight);
}

void UIManager::DrawCursor(Canvas& cv)
{
    if (!bCursorVisible)
        return;

    if (m_Cursor)
    {
        cv.ResetScissor();
        m_Cursor->Draw(cv, CursorPosition);
    }
}

UIBrush* UIManager::DefaultSliderBrush() const
{
    if (!m_SliderBrush)
        m_SliderBrush = UINew(UISolidBrush, Color4(0.5f, 0.5f, 0.5f, 1.0f)).WithRounding({4, 4, 4, 4});

    return m_SliderBrush;
}

UIBrush* UIManager::DefaultScrollbarBrush() const
{
    if (!m_ScrollbarBrush)
        m_ScrollbarBrush = UINew(UIBoxGradient)
                               .WithBoxOffsetTopLeft({1, 1})
                               .WithBoxOffsetBottomRight({0, 0})
                               .WithCornerRadius(3)
                               .WithFeather(4)
                               .WithInnerColor(Color4(0.2f, 0.2f, 0.2f))
                               .WithOuterColor(Color4(0.4f, 0.4f, 0.4f))
                               .WithRounding({3,3,3,3});

    return m_ScrollbarBrush;
}

void UIManager::UpConsole()
{
    m_Console.Up();
}

void UIManager::DownConsole()
{
    m_Console.Down();
}

void UIManager::OpenPopupWidget(UIWidget* widget)
{
    OpenPopupWidget(widget, CursorPosition);
}

void UIManager::OpenPopupWidget(UIWidget* widget, Float2 const& position)
{
    if (!m_ActiveDesktop)
        return;

    m_ActiveDesktop->OpenPopupWidget(widget, position);
}

void UIManager::ClosePopupWidget()
{
    if (!m_ActiveDesktop)
        return;

    m_ActiveDesktop->ClosePopupWidget();
}

HK_NAMESPACE_END
