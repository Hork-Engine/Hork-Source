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

#include <Engine/World/Public/Actors/PlayerController.h>
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/World.h>
#include <Engine/Runtime/Public/Runtime.h>

AN_CLASS_META_NO_ATTRIBS( FPlayerController )
AN_CLASS_META_NO_ATTRIBS( FRenderingParameters )

FPlayerController * FPlayerController::CurrentAudioListener = nullptr;

FPlayerController::FPlayerController() {
    InputComponent = CreateComponent< FInputComponent >( "PlayerControllerInput" );

    bCanEverTick = true;

    if ( !CurrentAudioListener ) {
        CurrentAudioListener = this;
    }
}

void FPlayerController::EndPlay() {
    Super::EndPlay();

    for ( int i = 0 ; i < ViewActors.Size() ; i++ ) {
        FViewActor * viewer = ViewActors[i];
        viewer->RemoveRef();
    }

    if ( CurrentAudioListener == this ) {
        CurrentAudioListener = nullptr;
    }
}

void FPlayerController::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    if ( Pawn && Pawn->IsPendingKill() ) {
        SetPawn( nullptr );
    }

    if ( HUD && HUD->IsPendingKill() ) {
        SetHUD( nullptr );
    }

    if ( CameraComponent && CameraComponent->IsPendingKill() ) {
        SetViewCamera( nullptr );
    }

    if ( AudioListener && AudioListener->IsPendingKill() ) {
        AudioListener = nullptr;
    }
}

void FPlayerController::SetPawn( FPawn * _Pawn ) {

    InputComponent->UnbindAll();

    InputComponent->BindAction( "Pause", IE_Press, this, &FPlayerController::TogglePause, true );
    InputComponent->BindAction( "TakeScreenshot", IE_Press, this, &FPlayerController::TakeScreenshot, true );
    InputComponent->BindAction( "ToggleWireframe", IE_Press, this, &FPlayerController::ToggleWireframe, true );
    InputComponent->BindAction( "ToggleDebugDraw", IE_Press, this, &FPlayerController::ToggleDebugDraw, true );

    Pawn = _Pawn;

    if ( Pawn ) {
        Pawn->SetupPlayerInputComponent( InputComponent );
    }

    if ( HUD ) {
        HUD->OwnerPawn = Pawn;
    }
}

void FPlayerController::SetViewCamera( FCameraComponent * _Camera ) {
    CameraComponent = _Camera;
}

void FPlayerController::SetAudioListener( FSceneComponent * _AudioListener ) {
    AudioListener = _AudioListener;
}

void FPlayerController::SetHUD( FHUD * _HUD ) {
    if ( HUD == _HUD ) {
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

void FPlayerController::SetRenderingParameters( FRenderingParameters * _RP ) {
    RenderingParameters = _RP;
}

void FPlayerController::SetInputMappings( FInputMappings * _InputMappings ) {
    InputComponent->SetInputMappings( _InputMappings );
}

FInputMappings * FPlayerController::GetInputMappings() {
    return InputComponent->GetInputMappings();
}

void FPlayerController::AddViewActor( FViewActor * _ViewActor ) {
    ViewActors.Append( _ViewActor );
    _ViewActor->AddRef();
}

void FPlayerController::VisitViewActors() {
    for ( int i = 0 ; i < ViewActors.Size() ; ) {
        FViewActor * viewer = ViewActors[i];

        if ( viewer->IsPendingKill() ) {
            viewer->RemoveRef();
            ViewActors.Remove( i );
            continue;
        }

        if ( CameraComponent ) {
            viewer->OnView( CameraComponent );
        }

        i++;
    }
}

void FPlayerController::SetPlayerIndex( int _ControllerId ) {
    InputComponent->ControllerId = _ControllerId;
}

int FPlayerController::GetPlayerIndex() const {
    return InputComponent->ControllerId;
}

void FPlayerController::TogglePause() {
    GetWorld()->SetPaused( !GetWorld()->IsPaused() );
}

void FPlayerController::TakeScreenshot() {
    /*

    TODO: something like this?

    struct FScreenshotParameters {
        int Width;          // Screenshot custom width or zero to use display width
        int Height;         // Screenshot custom height or zero to use display height
        bool bJpeg;         // Use JPEG compression instead of PNG
        int Number;         // Custom screenshot number. Set to zero to generate unique number.
    };

    FScreenshotParameters screenshotParams;
    memset( &screenshotParams, 0, sizeof( screenshotParams ) );

    GRuntime.TakeScreenshot( screenshotParams );

    Screenshot path example: /Screenshots/Year-Month-Day/Display1/ShotNNNN.png
    Where NNNN is screenshot number.

    */
}

void FPlayerController::ToggleWireframe() {
    if ( RenderingParameters ) {
        RenderingParameters->bWireframe ^= 1;
    }
}

void FPlayerController::ToggleDebugDraw() {
    if ( RenderingParameters ) {
        RenderingParameters->bDrawDebug ^= 1;
    }
}

void FPlayerController::SetCurrentAudioListener() {
    CurrentAudioListener = this;
}

FPlayerController * FPlayerController::GetCurrentAudioListener() {
    return CurrentAudioListener;
}
