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
#include "TerrainView.h"
#include "Engine.h"

HK_CLASS_META(APlayerController)
HK_CLASS_META(ARenderingParameters)

APlayerController* APlayerController::CurrentAudioListener = nullptr;

APlayerController::APlayerController()
{}

APlayerController::~APlayerController()
{
    if (CurrentAudioListener == this)
    {
        CurrentAudioListener = nullptr;
    }
}

void APlayerController::Initialize(SActorInitializer& Initializer)
{
    Super::Initialize(Initializer);

    InputComponent = CreateComponent<AInputComponent>("PlayerControllerInput");

    if (!CurrentAudioListener)
    {
        CurrentAudioListener = this;
    }
}

void APlayerController::OnPawnChanged()
{
    InputComponent->UnbindAll();

    InputComponent->BindAction("Pause", IA_PRESS, this, &APlayerController::TogglePause, true);

    if (Pawn)
    {
        Pawn->SetupInputComponent(InputComponent);
        Pawn->SetupRuntimeCommands();
    }

    if (HUD)
    {
        HUD->OwnerPawn = Pawn;
    }

    UpdatePawnCamera();
}

void APlayerController::SetAudioListener(ASceneComponent* _AudioListener)
{
    AudioListener = _AudioListener;
}

void APlayerController::SetHUD(AHUD* _HUD)
{
    if (IsSame(HUD, _HUD))
    {
        return;
    }

    if (_HUD && _HUD->OwnerPlayer)
    {
        _HUD->OwnerPlayer->SetHUD(nullptr);
    }

    if (HUD)
    {
        HUD->OwnerPlayer = nullptr;
        HUD->OwnerPawn   = nullptr;
    }

    HUD = _HUD;

    if (HUD)
    {
        HUD->OwnerPlayer = this;
        HUD->OwnerPawn   = Pawn;
    }
}

void APlayerController::SetRenderingParameters(ARenderingParameters* _RP)
{
    RenderingParameters = _RP;
}

void APlayerController::SetAudioParameters(AAudioParameters* _AudioParameters)
{
    AudioParameters = _AudioParameters;
}

void APlayerController::SetInputMappings(AInputMappings* _InputMappings)
{
    InputComponent->SetInputMappings(_InputMappings);
}

AInputMappings* APlayerController::GetInputMappings()
{
    return InputComponent->GetInputMappings();
}

void APlayerController::SetPlayerIndex(int _ControllerId)
{
    InputComponent->ControllerId = _ControllerId;
}

int APlayerController::GetPlayerIndex() const
{
    return InputComponent->ControllerId;
}

void APlayerController::TogglePause()
{
    GetWorld()->SetPaused(!GetWorld()->IsPaused());
}

ASceneComponent* APlayerController::GetAudioListener()
{
    if (AudioListener)
    {
        return AudioListener;
    }

    if (Pawn)
    {
        return Pawn->GetPawnCamera();
    }

    return nullptr;
}

void APlayerController::SetCurrentAudioListener()
{
    CurrentAudioListener = this;
}

APlayerController* APlayerController::GetCurrentAudioListener()
{
    return CurrentAudioListener;
}

float APlayerController::GetViewportAspectRatio() const
{
    return ViewportAspectRatio;
}

void APlayerController::SetViewport(int x, int y, int w, int h)
{
    ViewportX = x;
    ViewportY = y;

    if (ViewportWidth != w || ViewportHeight != h)
    {
        ViewportWidth = w;
        ViewportHeight = h;

        if (w > 0 && h > 0)
            ViewportAspectRatio = (float)w / h;
        else
            ViewportAspectRatio = 1;

        UpdatePawnCamera();
    }
}

Float2 APlayerController::GetLocalCursorPosition() const
{
    Float2 pos = GUIManager->CursorPosition;
    pos.X -= ViewportX;
    pos.Y -= ViewportY;
    return pos;
}

Float2 APlayerController::GetNormalizedCursorPosition() const
{
    if (ViewportWidth > 0 && ViewportHeight > 0)
    {
        Float2 pos = GetLocalCursorPosition();
        pos.X /= ViewportWidth;
        pos.Y /= ViewportHeight;
        pos.X = Math::Saturate(pos.X);
        pos.Y = Math::Saturate(pos.Y);
        return pos;
    }

    return Float2::Zero();
}

void APlayerController::UpdatePawnCamera()
{
    if (!Pawn)
    {
        return;
    }

    ACameraComponent* camera = Pawn->GetPawnCamera();
    if (!camera)
    {
        return;
    }

    SVideoMode const& vidMode = GEngine->GetVideoMode();

    camera->SetAspectRatio(ViewportAspectRatio * vidMode.AspectScale);
}

ARenderingParameters::ARenderingParameters()
{
    static Half data[16][16][16][4];
    static bool dataInit = false;
    if (!dataInit)
    {
        for (int z = 0; z < 16; z++)
        {
            for (int y = 0; y < 16; y++)
            {
                for (int x = 0; x < 16; x++)
                {
                    data[z][y][x][0] = (float)z / 15.0f * 255.0f;
                    data[z][y][x][1] = (float)y / 15.0f * 255.0f;
                    data[z][y][x][2] = (float)x / 15.0f * 255.0f;
                    data[z][y][x][3] = 255;
                }
            }
        }
        dataInit = true;
    }
    
    CurrentColorGradingLUT = ATexture::Create3D(TEXTURE_FORMAT_RGBA16_FLOAT, 1, 16, 16, 16); // FIXME: bgra
    CurrentColorGradingLUT->WriteTextureData3D(0, 0, 0, 16, 16, 16, 0, data);

    const float initialExposure[2] = {30.0f / 255.0f, 30.0f / 255.0f};

    CurrentExposure = ATexture::Create2D(TEXTURE_FORMAT_RG32_FLOAT, 1, 1, 1);
    CurrentExposure->WriteTextureData2D(0, 0, 1, 1, 0, initialExposure);

    SetColorGradingDefaults();
}

ARenderingParameters::~ARenderingParameters()
{
    for (auto it : TerrainViews)
    {
        it.second->RemoveRef();
    }
}

void ARenderingParameters::SetColorGradingEnabled(bool _ColorGradingEnabled)
{
    bColorGradingEnabled = _ColorGradingEnabled;
}

void ARenderingParameters::SetColorGradingLUT(ATexture* Texture)
{
    ColorGradingLUT = Texture;
}

void ARenderingParameters::SetColorGradingGrain(Float3 const& _ColorGradingGrain)
{
    ColorGradingGrain = _ColorGradingGrain;
}

void ARenderingParameters::SetColorGradingGamma(Float3 const& _ColorGradingGamma)
{
    ColorGradingGamma = _ColorGradingGamma;
}

void ARenderingParameters::SetColorGradingLift(Float3 const& _ColorGradingLift)
{
    ColorGradingLift = _ColorGradingLift;
}

void ARenderingParameters::SetColorGradingPresaturation(Float3 const& _ColorGradingPresaturation)
{
    ColorGradingPresaturation = _ColorGradingPresaturation;
}

void ARenderingParameters::SetColorGradingTemperature(float _ColorGradingTemperature)
{
    ColorGradingTemperature = _ColorGradingTemperature;

    Color4 color;
    color.SetTemperature(ColorGradingTemperature);

    ColorGradingTemperatureScale.X = color.R;
    ColorGradingTemperatureScale.Y = color.G;
    ColorGradingTemperatureScale.Z = color.B;
}

void ARenderingParameters::SetColorGradingTemperatureStrength(Float3 const& _ColorGradingTemperatureStrength)
{
    ColorGradingTemperatureStrength = _ColorGradingTemperatureStrength;
}

void ARenderingParameters::SetColorGradingBrightnessNormalization(float _ColorGradingBrightnessNormalization)
{
    ColorGradingBrightnessNormalization = _ColorGradingBrightnessNormalization;
}

void ARenderingParameters::SetColorGradingAdaptationSpeed(float _ColorGradingAdaptationSpeed)
{
    ColorGradingAdaptationSpeed = _ColorGradingAdaptationSpeed;
}

void ARenderingParameters::SetColorGradingDefaults()
{
    bColorGradingEnabled                = false;
    ColorGradingLUT                     = NULL;
    ColorGradingGrain                   = Float3(0.5f);
    ColorGradingGamma                   = Float3(0.5f);
    ColorGradingLift                    = Float3(0.5f);
    ColorGradingPresaturation           = Float3(1.0f);
    ColorGradingTemperatureStrength     = Float3(0.0f);
    ColorGradingBrightnessNormalization = 0.0f;
    ColorGradingAdaptationSpeed         = 2;
    ColorGradingTemperature             = 6500.0f;

    Color4 color;
    color.SetTemperature(ColorGradingTemperature);
    ColorGradingTemperatureScale.X = color.R;
    ColorGradingTemperatureScale.Y = color.G;
    ColorGradingTemperatureScale.Z = color.B;
}
