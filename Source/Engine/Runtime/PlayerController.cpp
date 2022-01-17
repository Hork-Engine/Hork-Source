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

#include "PlayerController.h"
#include "InputComponent.h"
#include "CameraComponent.h"
#include "World.h"
#include "WViewport.h"
#include "WDesktop.h"
#include "Engine.h"
#include <Core/Image.h>

AN_CLASS_META( APlayerController )
AN_CLASS_META( ARenderingParameters )

APlayerController * APlayerController::CurrentAudioListener = nullptr;

APlayerController::APlayerController() {
    InputComponent = CreateComponent< AInputComponent >( "PlayerControllerInput" );

    bCanEverTick = true;

    if ( !CurrentAudioListener ) {
        CurrentAudioListener = this;
    }
}

APlayerController::~APlayerController()
{
    if ( CurrentAudioListener == this ) {
        CurrentAudioListener = nullptr;
    }
}

void APlayerController::OnPawnChanged()
{
    InputComponent->UnbindAll();

    InputComponent->BindAction( "Pause", IA_PRESS, this, &APlayerController::TogglePause, true );
    InputComponent->BindAction( "TakeScreenshot", IA_PRESS, this, &APlayerController::TakeScreenshot, true );
    InputComponent->BindAction( "ToggleWireframe", IA_PRESS, this, &APlayerController::ToggleWireframe, true );
    InputComponent->BindAction( "ToggleDebugDraw", IA_PRESS, this, &APlayerController::ToggleDebugDraw, true );

    if ( Pawn ) {
        Pawn->SetupInputComponent( InputComponent );
        Pawn->SetupRuntimeCommands();
    }

    if ( HUD ) {
        HUD->OwnerPawn = Pawn;
    }

    UpdatePawnCamera();
}

void APlayerController::SetAudioListener( ASceneComponent * _AudioListener ) {
    AudioListener = _AudioListener;
}

void APlayerController::SetHUD( AHUD * _HUD ) {
    if ( IsSame( HUD, _HUD ) ) {
        return;
    }

    if ( _HUD && _HUD->OwnerPlayer ) {
        _HUD->OwnerPlayer->SetHUD( nullptr );
    }

    if ( HUD ) {
        HUD->OwnerPlayer = nullptr;
        HUD->OwnerPawn = nullptr;
    }

    HUD = _HUD;

    if ( HUD ) {
        HUD->OwnerPlayer = this;
        HUD->OwnerPawn = Pawn;
    }
}

void APlayerController::SetRenderingParameters( ARenderingParameters * _RP ) {
    RenderingParameters = _RP;
}

void APlayerController::SetAudioParameters( AAudioParameters * _AudioParameters ) {
    AudioParameters = _AudioParameters;
}

void APlayerController::SetInputMappings( AInputMappings * _InputMappings ) {
    InputComponent->SetInputMappings( _InputMappings );
}

AInputMappings * APlayerController::GetInputMappings() {
    return InputComponent->GetInputMappings();
}

void APlayerController::SetPlayerIndex( int _ControllerId ) {
    InputComponent->ControllerId = _ControllerId;
}

int APlayerController::GetPlayerIndex() const {
    return InputComponent->ControllerId;
}

void APlayerController::TogglePause() {
    GetWorld()->SetPaused( !GetWorld()->IsPaused() );
}

void APlayerController::TakeScreenshot() {
    if ( Viewport ) {
        WDesktop * desktop = Viewport->GetDesktop();

        if ( desktop ) {
            int w = desktop->GetWidth();
            int h = desktop->GetHeight();
            size_t sz = w*h*4;
            if ( sz > 0 ) {
                void * p = GHunkMemory.Alloc( sz );
                GEngine->ReadScreenPixels( 0, 0, w, h, sz, p );
                FlipImageY( p, w, h, 4, w * 4 );
                static int n = 0;
                AFileStream f;
                if ( f.OpenWrite( Platform::Fmt( "screenshots/%d.png", n++ ) ) ) {
                    WritePNG( f, w, h, 4, p, w*4 );
                }
                GHunkMemory.ClearLastHunk();
            }
        }
    }
}

void APlayerController::ToggleWireframe() {
    if ( RenderingParameters ) {
        RenderingParameters->bWireframe ^= 1;
    }
}

void APlayerController::ToggleDebugDraw() {
    if ( RenderingParameters ) {
        RenderingParameters->bDrawDebug ^= 1;
    }
}

ASceneComponent * APlayerController::GetAudioListener() {
    if ( AudioListener )
    {
        return AudioListener;
    }

    if ( Pawn )
    {
        return Pawn->GetPawnCamera();
    }

    return nullptr;
}

void APlayerController::SetCurrentAudioListener() {
    CurrentAudioListener = this;
}

APlayerController * APlayerController::GetCurrentAudioListener() {
    return CurrentAudioListener;
}

float APlayerController::GetViewportAspectRatio() const {
    return ViewportAspectRatio;
}

Float2 APlayerController::GetLocalCursorPosition() const {
    return Viewport ? Viewport->GetLocalCursorPosition()
                    : Float2::Zero();
}

Float2 APlayerController::GetNormalizedCursorPosition() const {
    if ( Viewport ) {
        Float2 size = Viewport->GetAvailableSize();
        if ( size.X > 0 && size.Y > 0 )
        {
            Float2 pos = GetLocalCursorPosition();
            pos.X /= size.X;
            pos.Y /= size.Y;
            pos.X = Math::Saturate( pos.X );
            pos.Y = Math::Saturate( pos.Y );
            return pos;
        }
    }
    return Float2::Zero();
}

void APlayerController::OnViewportUpdate() {
    if ( Viewport )
    {
        Float2 size = Viewport->GetAvailableSize();
        if ( size.X > 0 && size.Y > 0 ) {
            ViewportAspectRatio =  size.X / size.Y;
        }

        ViewportWidth = size.X;
        ViewportHeight = size.Y;
    }
    else
    {
        // Set default
        ViewportAspectRatio = 1.0f;
        ViewportWidth = 512;
        ViewportHeight = 512;
    }

    UpdatePawnCamera();
}

void APlayerController::UpdatePawnCamera() {
    if ( !Pawn )
    {
        return;
    }

    ACameraComponent * camera = Pawn->GetPawnCamera();
    if ( !camera )
    {
        return;
    }

    SVideoMode const & vidMode = GEngine->GetVideoMode();

    camera->SetAspectRatio( ViewportAspectRatio * vidMode.AspectScale );
}




ARenderingParameters::ARenderingParameters() {
    static uint16_t data[16][16][16][4];
    static bool     dataInit = false;
    if ( !dataInit ) {
        for ( int z = 0; z < 16; z++ ) {
            for ( int y = 0; y < 16; y++ ) {
                for ( int x = 0; x < 16; x++ ) {
                    data[z][y][x][0] = Math::FloatToHalf( (float)z / 15.0f * 255.0f );
                    data[z][y][x][1] = Math::FloatToHalf( (float)y / 15.0f * 255.0f );
                    data[z][y][x][2] = Math::FloatToHalf( (float)x / 15.0f * 255.0f );
                    data[z][y][x][3] = 255;
                }
            }
        }
        dataInit = true;
    }

    CurrentColorGradingLUT = CreateInstanceOf< ATexture >( STexture3D{}, TEXTURE_PF_BGRA16F, 1, 16, 16, 16 );
    CurrentColorGradingLUT->WriteTextureData3D( 0, 0, 0, 16, 16, 16, 0, data );

    const float initialExposure[2] = { 30.0f / 255.0f, 30.0f / 255.0f };
    CurrentExposure = CreateInstanceOf< ATexture >( STexture2D{}, TEXTURE_PF_RG32F, 1, 1, 1 );
    CurrentExposure->WriteTextureData2D( 0, 0, 1, 1, 0, initialExposure );

    LightTexture = CreateInstanceOf< ATexture >();
    DepthTexture = CreateInstanceOf< ATexture >();

    SetColorGradingDefaults();
}

ARenderingParameters::~ARenderingParameters() {
    for ( auto it : TerrainViews ) {
        it.second->RemoveRef();
    }
}

void ARenderingParameters::SetColorGradingEnabled( bool _ColorGradingEnabled ) {
    bColorGradingEnabled = _ColorGradingEnabled;
}

void ARenderingParameters::SetColorGradingLUT( ATexture * Texture ) {
    ColorGradingLUT = Texture;
}

void ARenderingParameters::SetColorGradingGrain( Float3 const & _ColorGradingGrain ) {
    ColorGradingGrain = _ColorGradingGrain;
}

void ARenderingParameters::SetColorGradingGamma( Float3 const & _ColorGradingGamma ) {
    ColorGradingGamma = _ColorGradingGamma;
}

void ARenderingParameters::SetColorGradingLift( Float3 const & _ColorGradingLift ) {
    ColorGradingLift = _ColorGradingLift;
}

void ARenderingParameters::SetColorGradingPresaturation( Float3 const & _ColorGradingPresaturation ) {
    ColorGradingPresaturation = _ColorGradingPresaturation;
}

void ARenderingParameters::SetColorGradingTemperature( float _ColorGradingTemperature ) {
    ColorGradingTemperature = _ColorGradingTemperature;

    Color4 color;
    color.SetTemperature( ColorGradingTemperature );

    ColorGradingTemperatureScale.X = color.R;
    ColorGradingTemperatureScale.Y = color.G;
    ColorGradingTemperatureScale.Z = color.B;
}

void ARenderingParameters::SetColorGradingTemperatureStrength( Float3 const & _ColorGradingTemperatureStrength ) {
    ColorGradingTemperatureStrength = _ColorGradingTemperatureStrength;
}

void ARenderingParameters::SetColorGradingBrightnessNormalization( float _ColorGradingBrightnessNormalization ) {
    ColorGradingBrightnessNormalization = _ColorGradingBrightnessNormalization;
}

void ARenderingParameters::SetColorGradingAdaptationSpeed( float _ColorGradingAdaptationSpeed ) {
    ColorGradingAdaptationSpeed = _ColorGradingAdaptationSpeed;
}

void ARenderingParameters::SetColorGradingDefaults() {
    bColorGradingEnabled = false;
    ColorGradingLUT = NULL;
    ColorGradingGrain = Float3( 0.5f );
    ColorGradingGamma = Float3( 0.5f );
    ColorGradingLift = Float3( 0.5f );
    ColorGradingPresaturation = Float3( 1.0f );
    ColorGradingTemperatureStrength = Float3( 0.0f );
    ColorGradingBrightnessNormalization = 0.0f;
    ColorGradingAdaptationSpeed = 2;
    ColorGradingTemperature = 6500.0f;

    Color4 color;
    color.SetTemperature( ColorGradingTemperature );
    ColorGradingTemperatureScale.X = color.R;
    ColorGradingTemperatureScale.Y = color.G;
    ColorGradingTemperatureScale.Z = color.B;
}
