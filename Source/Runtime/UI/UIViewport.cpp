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
#include "UIManager.h"
#include <Runtime/InputComponent.h>
#include <Runtime/PlayerController.h>
#include <Runtime/FrameLoop.h>
#include <Runtime/Engine.h>

HK_NAMESPACE_BEGIN

bool GUILockViewportScaling = false;

UIViewport::UIViewport(APlayerController* playerController) :
    m_PlayerController(playerController)
{}

UIViewport& UIViewport::WithRounding(RoundingDesc const& rounding)
{
    Rounding = rounding;
    return *this;
}

UIViewport& UIViewport::WithTint(Color4 const& tintColor)
{
    TintColor = tintColor;
    return *this;
}

UIViewport& UIViewport::WithComposite(CANVAS_COMPOSITE composite)
{
    Composite = composite;
    return *this;
}

UIViewport& UIViewport::SetPlayerController(APlayerController* playerController)
{
    m_PlayerController = playerController;

    // FIXME: Unpress buttons?

    return *this;
}

void UIViewport::OnKeyEvent(KeyEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    InputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetButtonState({ID_KEYBOARD, uint16_t(event.Key)}, event.Action, event.ModMask, timeStamp);
}

void UIViewport::OnMouseButtonEvent(MouseButtonEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    InputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetButtonState({ID_MOUSE, uint16_t(event.Button)}, event.Action, event.ModMask, timeStamp);
}

void UIViewport::OnMouseWheelEvent(MouseWheelEvent const& event, double timeStamp)
{
}

void UIViewport::OnMouseMoveEvent(MouseMoveEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    InputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetMouseAxisState(event.X, event.Y);

    UpdateViewSize();

    Float2 const& pos  = m_Geometry.Mins;
    Float2 const& size = m_Geometry.Maxs - m_Geometry.Mins;

    inputComponent->SetCursorPosition((GUIManager->CursorPosition - pos) / size * Float2(m_ViewWidth, m_ViewHeight));
}

void UIViewport::OnJoystickButtonEvent(JoystickButtonEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    InputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->SetButtonState({uint16_t(ID_JOYSTICK_1 + event.Joystick), uint16_t(event.Button)}, event.Action, 0, timeStamp);
}

void UIViewport::OnJoystickAxisEvent(JoystickAxisEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    InputComponent::SetJoystickAxisState(event.Joystick, event.Axis, event.Value);
}

void UIViewport::OnCharEvent(CharEvent const& event, double timeStamp)
{
    if (!m_PlayerController)
    {
        return;
    }

    InputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->NotifyUnicodeCharacter(event.UnicodeCharacter, event.ModMask, timeStamp);
}

void UIViewport::OnFocusLost()
{
    if (!m_PlayerController)
    {
        return;
    }

    InputComponent* inputComponent = m_PlayerController->GetInputComponent();

    inputComponent->UnpressButtons();
}

void UIViewport::OnFocusReceive()
{
}

void UIViewport::UpdateViewSize() // TODO: PostGeometryUpdate?
{
    if (!GUILockViewportScaling)
    {
        int x = m_Geometry.Mins.X;
        int y = m_Geometry.Mins.Y;

        m_ViewWidth  = Math::Max(0.0f, m_Geometry.Maxs.X - x);
        m_ViewHeight = Math::Max(0.0f, m_Geometry.Maxs.Y - y);
    }
}

void UIViewport::Draw(Canvas& canvas)
{
    if (m_PlayerController)
    {
        UpdateViewSize();

        m_PlayerController->SetViewport(m_ViewWidth, m_ViewHeight);

        Float2 const& pos  = m_Geometry.Mins;
        Float2 const& size = m_Geometry.Maxs - m_Geometry.Mins;

        AActor* pawn = m_PlayerController->GetPawn();
        if (pawn && size.X >= 1 && size.Y >= 1)
        {
            WorldRenderView* pView = m_PlayerController->GetRenderView();

            pView->SetCamera(pawn->GetPawnCamera());
            pView->SetCullingCamera(pawn->GetPawnCamera());

            GEngine->GetFrameLoop()->RegisterView(pView);

            DrawTextureDesc desc;
            desc.pTextureView = pView->GetTextureView();
            desc.X            = pos.X;
            desc.Y            = pos.Y;
            desc.W            = size.X;
            desc.H            = size.Y;
            desc.Rounding     = Rounding;
            desc.Angle        = 0;
            desc.TintColor    = TintColor;
            desc.Composite    = Composite;
            desc.bFlipY       = true;

            canvas.DrawTexture(desc);
        }

        AHUD* hud = m_PlayerController->GetHUD();
        if (hud)
            hud->Draw(canvas, pos.X, pos.Y, size.X, size.Y);
    }
}

HK_NAMESPACE_END
