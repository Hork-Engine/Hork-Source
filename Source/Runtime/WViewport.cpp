/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include "WViewport.h"
#include "InputComponent.h"
#include "PlayerController.h"
#include "FrameLoop.h"

AN_CLASS_META(WViewport)

WViewport::WViewport()
{
}

WViewport::~WViewport()
{
}

void WViewport::OnTransformDirty()
{
    Super::OnTransformDirty();

    if (PlayerController)
    {
        PlayerController->OnViewportUpdate();
    }
}

WViewport& WViewport::SetPlayerController(APlayerController* _PlayerController)
{
    // Unset old viewport
    if (PlayerController)
    {
        TRef<APlayerController> playerController(PlayerController); // Keep reference
        if (playerController->Viewport)
        {
            playerController->Viewport->PlayerController.Reset();
            playerController->Viewport.Reset();
        }
        if (!_PlayerController)
        {
            playerController->OnViewportUpdate();
        }
    }

    // Set new viewport
    PlayerController = _PlayerController;
    if (PlayerController)
    {
        PlayerController->Viewport = this;
        PlayerController->OnViewportUpdate();
    }

    // FIXME: Unpress buttons?

    return *this;
}

void WViewport::OnKeyEvent(struct SKeyEvent const& _Event, double _TimeStamp)
{
    if (!PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = PlayerController->GetInputComponent();

    inputComponent->SetButtonState({ID_KEYBOARD, uint16_t(_Event.Key)}, _Event.Action, _Event.ModMask, _TimeStamp);
}

void WViewport::OnMouseButtonEvent(struct SMouseButtonEvent const& _Event, double _TimeStamp)
{
    if (!PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = PlayerController->GetInputComponent();

    inputComponent->SetButtonState({ID_MOUSE, uint16_t(_Event.Button)}, _Event.Action, _Event.ModMask, _TimeStamp);
}

void WViewport::OnMouseWheelEvent(struct SMouseWheelEvent const& _Event, double _TimeStamp)
{
}

void WViewport::OnMouseMoveEvent(struct SMouseMoveEvent const& _Event, double _TimeStamp)
{
    if (!PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = PlayerController->GetInputComponent();

    inputComponent->SetMouseAxisState(_Event.X, _Event.Y);
}

void WViewport::OnJoystickButtonEvent(struct SJoystickButtonEvent const& _Event, double _TimeStamp)
{
    if (!PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = PlayerController->GetInputComponent();

    inputComponent->SetButtonState({uint16_t(ID_JOYSTICK_1 + _Event.Joystick), uint16_t(_Event.Button)}, _Event.Action, 0, _TimeStamp);
}

void WViewport::OnJoystickAxisEvent(struct SJoystickAxisEvent const& _Event, double _TimeStamp)
{
    if (!PlayerController)
    {
        return;
    }

    AInputComponent::SetJoystickAxisState(_Event.Joystick, _Event.Axis, _Event.Value);
}

void WViewport::OnCharEvent(struct SCharEvent const& _Event, double _TimeStamp)
{
    if (!PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = PlayerController->GetInputComponent();

    inputComponent->NotifyUnicodeCharacter(_Event.UnicodeCharacter, _Event.ModMask, _TimeStamp);
}

void WViewport::OnFocusLost()
{
    if (!PlayerController)
    {
        return;
    }

    AInputComponent* inputComponent = PlayerController->GetInputComponent();

    inputComponent->UnpressButtons();
}

void WViewport::OnFocusReceive()
{
}

void WViewport::OnDrawEvent(ACanvas& InCanvas)
{
    Super::OnDrawEvent(InCanvas);

    if (PlayerController)
    {
        Float2 mins, maxs;
        GetDesktopRect(mins, maxs, false);

        Float2 const& pos  = mins;
        Float2 const& size = maxs - mins;

        AActor* pawn = PlayerController->GetPawn();
        if (pawn)
        {
            InCanvas.DrawViewport(pawn->GetPawnCamera(), PlayerController->GetRenderingParameters(),
                                  pos.X, pos.Y, size.X, size.Y, Color4::White(), 0, -1, COLOR_BLENDING_DISABLED);
        }

        AHUD* hud = PlayerController->GetHUD();
        if (hud)
        {
            InCanvas.PushClipRect(mins, maxs, true);
            hud->Draw(&InCanvas, pos.X, pos.Y, size.X, size.Y);
            InCanvas.PopClipRect();
        }
    }

    DrawDecorates(InCanvas);
}
