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

#include "AnalyticLightComponent.h"
#include "World.h"
#include "DebugRenderer.h"

#include <Core/ConsoleVar.h>

static const float  DEFAULT_LUMENS      = 3000.0f;
static const float  DEFAULT_TEMPERATURE = 6590.0f;
static const Float3 DEFAULT_COLOR(1.0f);

AConsoleVar com_LightEnergyScale("com_LightEnergyScale"s, "16"s);

HK_CLASS_META(AAnalyticLightComponent)

AAnalyticLightComponent::AAnalyticLightComponent()
{
    Lumens         = DEFAULT_LUMENS;
    Temperature    = DEFAULT_TEMPERATURE;
    Color          = DEFAULT_COLOR;
    EffectiveColor = Float3(0.0f);
}

void AAnalyticLightComponent::SetPhotometricProfile(APhotometricProfile* Profile)
{
    PhotometricProfile   = Profile;
    bEffectiveColorDirty = true;
}

void AAnalyticLightComponent::SetPhotometricAsMask(bool _bPhotometricAsMask)
{
    bPhotometricAsMask   = _bPhotometricAsMask;
    bEffectiveColorDirty = true;
}

void AAnalyticLightComponent::SetLuminousIntensityScale(float IntensityScale)
{
    LuminousIntensityScale = IntensityScale;
    bEffectiveColorDirty   = true;
}

void AAnalyticLightComponent::SetLumens(float _Lumens)
{
    Lumens               = Math::Max(0.0f, _Lumens);
    bEffectiveColorDirty = true;
}

float AAnalyticLightComponent::GetLumens() const
{
    return Lumens;
}

void AAnalyticLightComponent::SetTemperature(float _Temperature)
{
    Temperature          = _Temperature;
    bEffectiveColorDirty = true;
}

float AAnalyticLightComponent::GetTemperature() const
{
    return Temperature;
}

void AAnalyticLightComponent::SetColor(Float3 const& _Color)
{
    Color                = _Color;
    bEffectiveColorDirty = true;
}

void AAnalyticLightComponent::SetColor(float _R, float _G, float _B)
{
    Color.X              = _R;
    Color.Y              = _G;
    Color.Z              = _B;
    bEffectiveColorDirty = true;
}

Float3 const& AAnalyticLightComponent::GetColor() const
{
    return Color;
}

Float3 const& AAnalyticLightComponent::GetEffectiveColor(float CosHalfConeAngle) const
{
    if (bEffectiveColorDirty || com_LightEnergyScale.IsModified())
    {
        const float EnergyUnitScale = 1.0f / com_LightEnergyScale.GetFloat();

        float candela;

        if (PhotometricProfile && !bPhotometricAsMask)
        {
            candela = LuminousIntensityScale * PhotometricProfile->GetIntensity();
        }
        else
        {
            const float LumensToCandela = 1.0f / Math::_2PI / (1.0f - CosHalfConeAngle);

            candela = Lumens * LumensToCandela;
        }

        // Animate light intensity
        candela *= GetAnimationBrightness();

        Color4 temperatureColor;
        temperatureColor.SetTemperature(GetTemperature());

        float finalScale  = candela * EnergyUnitScale;
        EffectiveColor[0] = Color[0] * temperatureColor[0] * finalScale;
        EffectiveColor[1] = Color[1] * temperatureColor[1] * finalScale;
        EffectiveColor[2] = Color[2] * temperatureColor[2] * finalScale;

        bEffectiveColorDirty = false;
    }
    return EffectiveColor;
}

void AAnalyticLightComponent::SetDirection(Float3 const& _Direction)
{
    Float3x3 orientation;

    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis(orientation[0], orientation[1]);

    Quat rotation;
    rotation.FromMatrix(orientation);
    SetRotation(rotation);
}

Float3 AAnalyticLightComponent::GetDirection() const
{
    return GetForwardVector();
}

void AAnalyticLightComponent::SetWorldDirection(Float3 const& _Direction)
{
    Float3x3 orientation;
    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis(orientation[0], orientation[1]);

    Quat rotation;
    rotation.FromMatrix(orientation);
    SetWorldRotation(rotation);
}

Float3 AAnalyticLightComponent::GetWorldDirection() const
{
    return GetWorldForwardVector();
}

void AAnalyticLightComponent::PackLight(Float4x4 const& InViewMatrix, SLightParameters& Light)
{
}
