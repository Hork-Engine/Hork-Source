/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Components/CameraComponent.h>
#include <World/Public/World.h>
#include <World/Public/Widgets/WViewport.h>
#include <World/Public/Widgets/WDesktop.h>
#include <Runtime/Public/Runtime.h>
#include <GameThread/Public/EngineInstance.h>
#include <Core/Public/Image.h>

AN_CLASS_META( APlayerController )
AN_CLASS_META( ARenderingParameters )

APlayerController * APlayerController::CurrentAudioListener = nullptr;
ACommandContext * APlayerController::CurrentCommandContext = nullptr;

APlayerController::APlayerController() {
    InputComponent = CreateComponent< AInputComponent >( "PlayerControllerInput" );

    bCanEverTick = true;

    if ( !CurrentAudioListener ) {
        CurrentAudioListener = this;
    }

    if ( !CurrentCommandContext ) {
        CurrentCommandContext = &CommandContext;
    }

    CommandContext.AddCommand( "quit", { this, &APlayerController::Quit }, "Quit from application" );
}

void APlayerController::Quit( ARuntimeCommandProcessor const & _Proc ) {
    GRuntime.PostTerminateEvent();
}

void APlayerController::EndPlay() {
    Super::EndPlay();

    if ( CurrentAudioListener == this ) {
        CurrentAudioListener = nullptr;
    }

    if ( CurrentCommandContext == &CommandContext ) {
        CurrentCommandContext = nullptr;
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
        Pawn->SetupPlayerInputComponent( InputComponent );
        Pawn->SetupRuntimeCommands( CommandContext );
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
                GRenderBackend->ReadScreenPixels( 0, 0, w, h, sz, 1, p );
                FlipImageY( p, w, h, 4, w * 4 );
                static int n = 0;
                AFileStream f;
                if ( f.OpenWrite( Core::Fmt( "screenshots/%d.png", n++ ) ) ) {
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

void APlayerController::SetCurrentCommandContext() {
    CurrentCommandContext = &CommandContext;
}

ACommandContext * APlayerController::GetCurrentCommandContext() {
    return CurrentCommandContext;
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

    SVideoMode const & vidMode = GRuntime.GetVideoMode();

    camera->SetAspectRatio( ViewportAspectRatio * vidMode.AspectScale );
}
