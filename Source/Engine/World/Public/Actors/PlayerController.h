/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#pragma once

#include "Controller.h"
#include "HUD.h"
#include <Engine/World/Public/Components/CameraComponent.h>
#include <Engine/World/Public/CommandContext.h>
#include <Engine/Audio/Public/AudioSystem.h>

class AInputMappings;

class ANGIE_API ARenderingParameters final : public ABaseObject {
    AN_CLASS( ARenderingParameters, ABaseObject )

public:
    AColor4 BackgroundColor = AColor4( 0.3f, 0.3f, 0.8f );
    bool bClearBackground;
    bool bWireframe;
    bool bDrawDebug;
    int RenderingMask = ~0;

    // TODO:
    //TRef< FTexture3D > ColorGradingLUT;

private:
    ARenderingParameters() {}
    ~ARenderingParameters() {}
};

class ANGIE_API AAudioParameters final : public ABaseObject {
    AN_CLASS( AAudioParameters, ABaseObject )

public:
    Float3 Velocity;

    float DopplerFactor = 1;
    float DopplerVelocity = 1;
    float SpeedOfSound = 343.3f;
    EAudioDistanceModel DistanceModel = AUDIO_DIST_INVERSE_CLAMPED;
    float Volume = 1.0f;

private:
    AAudioParameters() {}
    ~AAudioParameters() {}
};

/*

APlayerController

Base class for players

*/
class ANGIE_API APlayerController : public AController {
    AN_ACTOR( APlayerController, AController )

public:
    ACommandContext CommandContext;

    void SetPawn( APawn * _Pawn );

    void SetViewCamera( ACameraComponent * _Camera );

    void SetViewport( Float2 const & _Position, Float2 const & _Size );

    void SetAudioListener( ASceneComponent * _AudioListener );

    void SetHUD( AHUD * _HUD );

    void SetRenderingParameters( ARenderingParameters * _RP );

    void SetAudioParameters( AAudioParameters * _AudioParameters );

    // Forward setting input mappings to input component
    void SetInputMappings( AInputMappings * _InputMappings );

    void SetPlayerIndex( int _ControllerId );

    void SetCurrentAudioListener();

    void SetCurrentCommandContext();

    void SetActive( bool _Active );

    //void AddViewActor( FViewActor * _ViewActor );

    APawn * GetPawn() const { return Pawn; }    

    ACameraComponent * GetViewCamera() const { return CameraComponent; }    

    //SViewport const & GetViewport() const { return Viewport; }
    Float2 const & GetViewportPosition() const { return ViewportPosition; }

    Float2 const & GetViewportSize() const { return ViewportSize; }

    ASceneComponent * GetAudioListener() { return AudioListener ? AudioListener.GetObject() : CameraComponent; }

    AHUD * GetHUD() const { return HUD; }

    ARenderingParameters * GetRenderingParameters() { return RenderingParameters; }

    AAudioParameters * GetAudioParameters() { return AudioParameters; }

    AInputMappings * GetInputMappings();

    AInputComponent * GetInputComponent() const { return InputComponent; }

    int GetPlayerIndex() const;

    bool IsActive() const;

    static APlayerController * GetCurrentAudioListener();
    static ACommandContext * GetCurrentCommandContext();

    //void VisitViewActors();

    Float2 GetNormalizedCursorPos() const override;

protected:

    APlayerController();

    void EndPlay() override;
    void Tick( float _TimeStep ) override;

private:
    void Quit( ARuntimeCommandProcessor const & _Proc );

    void TogglePause();
    void TakeScreenshot();
    void ToggleWireframe();
    void ToggleDebugDraw();

    TRef< ARenderingParameters > RenderingParameters;
    TRef< AAudioParameters > AudioParameters;
    AInputComponent * InputComponent;
    TWeakRef< ACameraComponent > CameraComponent;
    TWeakRef< ASceneComponent > AudioListener;
    TRef< APawn > Pawn;
    TWeakRef< AHUD > HUD;
    //TPodArray< FViewActor * > ViewActors;
    Float2 ViewportPosition;
    Float2 ViewportSize;

    static APlayerController * CurrentAudioListener;
    static ACommandContext * CurrentCommandContext;
};
