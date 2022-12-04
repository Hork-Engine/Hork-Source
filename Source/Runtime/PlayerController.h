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

#include "Controller.h"
#include "HUD.h"
#include "AudioSystem.h"
#include "WorldRenderView.h"

class AInputMappings;

class AAudioParameters : public ABaseObject
{
    HK_CLASS(AAudioParameters, ABaseObject)

public:
    //Float3 Velocity;

    //float DopplerFactor = 1;
    //float DopplerVelocity = 1;
    //float SpeedOfSound = 343.3f;
    //EAudioDistanceModel DistanceModel = AUDIO_DIST_LINEAR_CLAMPED;//AUDIO_DIST_INVERSE_CLAMPED;
    float Volume = 1.0f;

    uint32_t ListenerMask = ~0u;

    AAudioParameters() = default;
};

/**

APlayerController

Base class for players

*/
class APlayerController : public AController
{
    HK_ACTOR(APlayerController, AController)

public:
    /** Player viewport. */
    void SetViewport(int w, int h);

    /** Override listener location. If listener is not specified, pawn camera will be used. */
    void SetAudioListener(ASceneComponent* _AudioListener);

    /** Set HUD actor */
    void SetHUD(AHUD* _HUD);

    /** Set world render view */
    void SetRenderView(WorldRenderView* renderView);

    /** Set audio rendering parameters */
    void SetAudioParameters(AAudioParameters* _AudioParameters);

    /** Set input mappings for input component */
    void SetInputMappings(AInputMappings* _InputMappings);

    /** Set controller player index */
    void SetPlayerIndex(int _ControllerId);

    /** Set player controller as primary audio listener */
    void SetCurrentAudioListener();

    float GetViewportAspectRatio() const;

    int GetViewportWidth() const { return m_ViewportWidth; }
    int GetViewportHeight() const { return m_ViewportHeight; }

    /** Get current audio listener */
    ASceneComponent* GetAudioListener();

    /** Get HUD actor */
    AHUD* GetHUD() const { return m_HUD; }

    /** Get world render view */
    WorldRenderView* GetRenderView() { return m_RenderView; }

    /** Get audio rendering parameters */
    AAudioParameters* GetAudioParameters() { return m_AudioParameters; }

    /** Get input mappings of input component */
    AInputMappings* GetInputMappings();

    /** Return input component */
    AInputComponent* GetInputComponent() const { return m_InputComponent; }

    /** Get controller player index */
    int GetPlayerIndex() const;

    /** Primary audio listener */
    static APlayerController* GetCurrentAudioListener();

    virtual void UpdatePawnCamera();

protected:
    AInputComponent* m_InputComponent{};

    APlayerController() = default;
    ~APlayerController();

    void Initialize(SActorInitializer& Initializer);

    void OnPawnChanged() override;

private:
    void TogglePause();

    TRef<WorldRenderView>      m_RenderView;
    TRef<AAudioParameters>     m_AudioParameters;
    TWeakRef<ASceneComponent>  m_AudioListener;
    TWeakRef<AHUD>             m_HUD;
    float                      m_ViewportAspectRatio{};
    int                        m_ViewportWidth{};
    int                        m_ViewportHeight{};

    static APlayerController* m_CurrentAudioListener;
};
