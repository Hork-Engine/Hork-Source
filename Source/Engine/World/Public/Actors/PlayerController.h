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

#include "Pawn.h"
#include "HUD.h"
#include <Engine/World/Public/Components/CameraComponent.h>
#include <Engine/Audio/Public/AudioSystem.h>

class FInputMappings;

class ANGIE_API FRenderingParameters final : public FBaseObject{
    AN_CLASS( FRenderingParameters, FBaseObject )

public:
    Float3 BackgroundColor = Float3( 0.3f, 0.3f, 0.8f );
    bool bClearBackground = true;
    bool bWireframe;
    bool bDrawDebug;
    int RenderingMask = ~0;

private:
    FRenderingParameters() {}
    ~FRenderingParameters() {}
};

class ANGIE_API FAudioParameters final : public FBaseObject {
    AN_CLASS( FAudioParameters, FBaseObject )

public:
    Float3 Velocity;

    Float DopplerFactor = 1;
    Float DopplerVelocity = 1;
    Float SpeedOfSound = 343.3f;
    EAudioDistanceModel DistanceModel = AUDIO_DM_InverseClamped;
    Float Volume = 1.0f;

private:
    FAudioParameters() {}
    ~FAudioParameters() {}
};

/*

FPlayerController

Base class for players

*/
class ANGIE_API FPlayerController : public FActor {
    AN_ACTOR( FPlayerController, FActor )

public:

    void SetPawn( FPawn * _Pawn );

    FPawn * GetPawn() const { return Pawn; }

    void SetViewCamera( FCameraComponent * _Camera );

    FCameraComponent * GetViewCamera() const { return CameraComponent; }

    void SetAudioListener( FSceneComponent * _AudioListener );

    FSceneComponent * GetAudioListener() { if ( AudioListener ) return AudioListener; return CameraComponent; }

    void SetHUD( FHUD * _HUD );

    FHUD * GetHUD() const { return HUD; }

    void SetRenderingParameters( FRenderingParameters * _RP );

    FRenderingParameters * GetRenderingParameters() { return RenderingParameters; }

    void SetAudioParameters( FAudioParameters * _AudioParameters );

    FAudioParameters * GetAudioParameters() { return AudioParameters; }

    // Forward setting input mappings to input component
    void SetInputMappings( FInputMappings * _InputMappings );

    FInputMappings * GetInputMappings();

    // Get input component
    FInputComponent * GetInputComponent() const { return InputComponent; }

    void SetPlayerIndex( int _ControllerId );

    int GetPlayerIndex() const;

    void AddViewActor( FViewActor * _ViewActor );

    void SetCurrentAudioListener();

    static FPlayerController * GetCurrentAudioListener();

    void VisitViewActors();

protected:

    FPlayerController();

    void EndPlay() override;
    void Tick( float _TimeStep ) override;

private:

    void TogglePause();
    void TakeScreenshot();
    void ToggleWireframe();
    void ToggleDebugDraw();

    FInputComponent * InputComponent;
    TRef< FPawn > Pawn;
    TRef< FCameraComponent > CameraComponent;
    TRef< FSceneComponent > AudioListener;
    TRef< FRenderingParameters > RenderingParameters;
    TRef< FAudioParameters > AudioParameters;
    TPodArray< FViewActor * > ViewActors;
    TRef< FHUD > HUD;

    static FPlayerController * CurrentAudioListener;
};
