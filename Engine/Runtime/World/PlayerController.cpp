/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "PlayerController.h"
#include "InputComponent.h"
#include "CameraComponent.h"
#include "World.h"

#include <Engine/Runtime/Engine.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(Actor_PlayerController)

Actor_PlayerController* Actor_PlayerController::m_CurrentAudioListener = nullptr;

Actor_PlayerController::~Actor_PlayerController()
{
    if (m_CurrentAudioListener == this)
    {
        m_CurrentAudioListener = nullptr;
    }
}

void Actor_PlayerController::Initialize(ActorInitializer& Initializer)
{
    Super::Initialize(Initializer);

    m_InputComponent = CreateComponent<InputComponent>("PlayerControllerInput");

    if (!m_CurrentAudioListener)
    {
        m_CurrentAudioListener = this;
    }
}

void Actor_PlayerController::OnPawnChanged()
{
    m_InputComponent->UnbindAll();

    m_InputComponent->BindAction("Pause", IA_PRESS, this, &Actor_PlayerController::TogglePause, true);

    if (m_Pawn)
    {
        m_Pawn->SetupInputComponent(m_InputComponent);
        m_Pawn->SetupRuntimeCommands();
    }

    UpdatePawnCamera();
}

void Actor_PlayerController::SetAudioListener(SceneComponent* _AudioListener)
{
    m_AudioListener = _AudioListener;
}

void Actor_PlayerController::SetHUD(Actor_HUD* _HUD)
{
    if (m_HUD == _HUD)
    {
        return;
    }

    if (m_HUD)
    {
        m_HUD->OnControllerDetached();
    }

    if (_HUD)
    {
        Actor_PlayerController* controller = _HUD->GetController();
        if (controller)
            controller->m_HUD.Reset();
    }

    m_HUD = _HUD;

    if (m_HUD)
        m_HUD->OnControllerAttached(this);
}

void Actor_PlayerController::SetRenderView(WorldRenderView* renderView)
{
    m_RenderView = renderView;
}

void Actor_PlayerController::SetAudioParameters(AudioParameters* _AudioParameters)
{
    m_AudioParameters = _AudioParameters;
}

void Actor_PlayerController::SetInputMappings(InputMappings* _InputMappings)
{
    m_InputComponent->SetInputMappings(_InputMappings);
}

InputMappings* Actor_PlayerController::GetInputMappings()
{
    return m_InputComponent->GetInputMappings();
}

void Actor_PlayerController::SetPlayerIndex(int _ControllerId)
{
    m_InputComponent->ControllerId = _ControllerId;
}

int Actor_PlayerController::GetPlayerIndex() const
{
    return m_InputComponent->ControllerId;
}

void Actor_PlayerController::TogglePause()
{
    GetWorld()->SetPaused(!GetWorld()->IsPaused());
}

SceneComponent* Actor_PlayerController::GetAudioListener()
{
    if (m_AudioListener)
    {
        return m_AudioListener;
    }

    if (m_Pawn)
    {
        return m_Pawn->GetPawnCamera();
    }

    return nullptr;
}

void Actor_PlayerController::SetCurrentAudioListener()
{
    m_CurrentAudioListener = this;
}

Actor_PlayerController* Actor_PlayerController::GetCurrentAudioListener()
{
    return m_CurrentAudioListener;
}

float Actor_PlayerController::GetViewportAspectRatio() const
{
    return m_ViewportAspectRatio;
}

void Actor_PlayerController::SetViewport(int w, int h)
{
    if (m_RenderView)
    {
        m_RenderView->SetViewport(w, h);
    }

    if (m_ViewportWidth != w || m_ViewportHeight != h)
    {
        m_ViewportWidth = w;
        m_ViewportHeight = h;

        if (w > 0 && h > 0)
            m_ViewportAspectRatio = (float)w / h;
        else
            m_ViewportAspectRatio = 1;

        UpdatePawnCamera();
    }
}

void Actor_PlayerController::UpdatePawnCamera()
{
    if (!m_Pawn)
    {
        return;
    }

    CameraComponent* camera = m_Pawn->GetPawnCamera();
    if (!camera)
    {
        return;
    }

    DisplayVideoMode const& vidMode = GEngine->GetVideoMode();

    camera->SetAspectRatio(m_ViewportAspectRatio * vidMode.AspectScale);
}

HK_NAMESPACE_END
