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

#include "UIViewport.h"
#include <Runtime/InputComponent.h>
#include <Runtime/PlayerController.h>
#include <Runtime/FrameLoop.h>

bool GUILockViewportScaling = false;

UIViewport::UIViewport(APlayerController* playerController) :
    m_PlayerController(playerController)
{}

UIViewport& UIViewport::SetRounding(RoundingDesc const& rounding)
{
    m_Rounding = rounding;
    return *this;
}

UIViewport& UIViewport::SetTint(Color4 const& tintColor)
{
    m_TintColor = tintColor;
    return *this;
}

UIViewport& UIViewport::SetComposite(CANVAS_COMPOSITE composite)
{
    m_Composite = composite;
    return *this;
}

UIViewport& UIViewport::SetPlayerController(APlayerController* playerController)
{
    m_PlayerController = playerController;

    // FIXME: Unpress buttons?

    return *this;
}

void UIViewport::OnKeyEvent(SKeyEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetButtonState({ID_KEYBOARD, uint16_t(event.Key)}, event.Action, event.ModMask, timeStamp);
}

void UIViewport::OnMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetButtonState({ID_MOUSE, uint16_t(event.Button)}, event.Action, event.ModMask, timeStamp);
}

void UIViewport::OnMouseWheelEvent(SMouseWheelEvent const& event, double timeStamp)
{
}

void UIViewport::OnMouseMoveEvent(SMouseMoveEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetMouseAxisState(event.X, event.Y);
}

void UIViewport::OnJoystickButtonEvent(SJoystickButtonEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetButtonState({uint16_t(ID_JOYSTICK_1 + event.Joystick), uint16_t(event.Button)}, event.Action, 0, timeStamp);
}

void UIViewport::OnJoystickAxisEvent(SJoystickAxisEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    AInputComponent::SetJoystickAxisState(event.Joystick, event.Axis, event.Value);
}

void UIViewport::OnCharEvent(SCharEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->NotifyUnicodeCharacter(event.UnicodeCharacter, event.ModMask, timeStamp);
}

void UIViewport::OnFocusLost()
{
    if (!m_PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->UnpressButtons();
}

void UIViewport::OnFocusReceive()
{
}

void UIViewport::Draw(ACanvas& canvas)
{
    if (m_PlayerController)
    {
        int x = Geometry.Mins.X;
        int y = Geometry.Mins.Y;

        if (!GUILockViewportScaling)
        {
            m_ViewWidth = Math::Max(0.0f, Geometry.Maxs.X - x);
            m_ViewHeight = Math::Max(0.0f, Geometry.Maxs.Y - y);
        }

        m_PlayerController->SetViewport(x, y, m_ViewWidth, m_ViewHeight);

        Float2 const& pos  = Geometry.Mins;
        Float2 const& size = Geometry.Maxs - Geometry.Mins;

        AActor* pawn = m_PlayerController->GetPawn();
        if (pawn)
        {
            DrawViewportDesc desc;

            desc.pCamera          = pawn->GetPawnCamera();
            desc.pRenderingParams = m_PlayerController->GetRenderingParameters();
            desc.X                = pos.X;
            desc.Y                = pos.Y;
            desc.W                = size.X;
            desc.H                = size.Y;
            desc.TextureResolutionX = m_ViewWidth;
            desc.TextureResolutionY = m_ViewHeight;
            desc.Rounding         = m_Rounding;
            desc.Angle            = 0;
            desc.TintColor        = m_TintColor;
            desc.Composite        = m_Composite;

            canvas.DrawViewport(desc);
        }

        AHUD* hud = m_PlayerController->GetHUD();
        if (hud)
        {
            //canvas.Push();
            //canvas.IntersectScissor(Geometry.Mins, Geometry.Maxs);
            hud->Draw(&canvas, pos.X, pos.Y, size.X, size.Y);
            //canvas.Pop();
        }
    }
}
