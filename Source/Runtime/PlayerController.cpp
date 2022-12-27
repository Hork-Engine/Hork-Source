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

#include "PlayerController.h"
#include "InputComponent.h"
#include "CameraComponent.h"
#include "World.h"
#include "WorldRenderView.h"
#include "Engine.h"

HK_NAMESPACE_BEGIN

HK_CLASS_META(APlayerController)

APlayerController* APlayerController::m_CurrentAudioListener = nullptr;

APlayerController::~APlayerController()
{
    if (m_CurrentAudioListener == this)
    {
        m_CurrentAudioListener = nullptr;
    }
}

void APlayerController::Initialize(ActorInitializer& Initializer)
{
    Super::Initialize(Initializer);

    m_InputComponent = CreateComponent<InputComponent>("PlayerControllerInput");

    if (!m_CurrentAudioListener)
    {
        m_CurrentAudioListener = this;
    }
}

void APlayerController::OnPawnChanged()
{
    m_InputComponent->UnbindAll();

    m_InputComponent->BindAction("Pause", IA_PRESS, this, &APlayerController::TogglePause, true);

    if (m_Pawn)
    {
        m_Pawn->SetupInputComponent(m_InputComponent);
        m_Pawn->SetupRuntimeCommands();
    }

    if (m_HUD)
    {
        m_HUD->m_OwnerPawn = m_Pawn;
    }

    UpdatePawnCamera();
}

void APlayerController::SetAudioListener(SceneComponent* _AudioListener)
{
    m_AudioListener = _AudioListener;
}

void APlayerController::SetHUD(AHUD* _HUD)
{
    if (m_HUD == _HUD)
    {
        return;
    }

    if (_HUD && _HUD->m_OwnerPlayer)
    {
        _HUD->m_OwnerPlayer->SetHUD(nullptr);
    }

    if (m_HUD)
    {
        m_HUD->m_OwnerPlayer = nullptr;
        m_HUD->m_OwnerPawn   = nullptr;
    }

    m_HUD = _HUD;

    if (m_HUD)
    {
        m_HUD->m_OwnerPlayer = this;
        m_HUD->m_OwnerPawn   = m_Pawn;
    }
}

void APlayerController::SetRenderView(WorldRenderView* renderView)
{
    m_RenderView = renderView;
}

void APlayerController::SetAudioParameters(AudioParameters* _AudioParameters)
{
    m_AudioParameters = _AudioParameters;
}

void APlayerController::SetInputMappings(InputMappings* _InputMappings)
{
    m_InputComponent->SetInputMappings(_InputMappings);
}

InputMappings* APlayerController::GetInputMappings()
{
    return m_InputComponent->GetInputMappings();
}

void APlayerController::SetPlayerIndex(int _ControllerId)
{
    m_InputComponent->ControllerId = _ControllerId;
}

int APlayerController::GetPlayerIndex() const
{
    return m_InputComponent->ControllerId;
}

void APlayerController::TogglePause()
{
    GetWorld()->SetPaused(!GetWorld()->IsPaused());
}

SceneComponent* APlayerController::GetAudioListener()
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

void APlayerController::SetCurrentAudioListener()
{
    m_CurrentAudioListener = this;
}

APlayerController* APlayerController::GetCurrentAudioListener()
{
    return m_CurrentAudioListener;
}

float APlayerController::GetViewportAspectRatio() const
{
    return m_ViewportAspectRatio;
}

void APlayerController::SetViewport(int w, int h)
{
    if (m_RenderView)
    {
        m_RenderView->SetViewport(w, h);
    }

    if (m_ViewportWidth != w || m_ViewportHeight != h)
    {
        m_ViewportWidth  = w;
        m_ViewportHeight = h;

        if (w > 0 && h > 0)
            m_ViewportAspectRatio = (float)w / h;
        else
            m_ViewportAspectRatio = 1;

        UpdatePawnCamera();
    }
}

void APlayerController::UpdatePawnCamera()
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
