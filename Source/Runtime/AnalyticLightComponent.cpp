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

ConsoleVar com_LightEnergyScale("com_LightEnergyScale"s, "16"s);

HK_CLASS_META(AnalyticLightComponent)

AnalyticLightComponent::AnalyticLightComponent()
{
    m_Lumens = DEFAULT_LUMENS;
    m_Temperature = DEFAULT_TEMPERATURE;
    m_Color = DEFAULT_COLOR;
    m_EffectiveColor = Float3(0.0f);
}

void AnalyticLightComponent::SetPhotometricProfile(PhotometricProfile* Profile)
{
    m_PhotometricProfile = Profile;
    m_bEffectiveColorDirty = true;
}

void AnalyticLightComponent::SetPhotometricAsMask(bool bPhotometricAsMask)
{
    m_bPhotometricAsMask = bPhotometricAsMask;
    m_bEffectiveColorDirty = true;
}

void AnalyticLightComponent::SetLuminousIntensityScale(float IntensityScale)
{
    m_LuminousIntensityScale = IntensityScale;
    m_bEffectiveColorDirty = true;
}

void AnalyticLightComponent::SetLumens(float _Lumens)
{
    m_Lumens = Math::Max(0.0f, _Lumens);
    m_bEffectiveColorDirty = true;
}

float AnalyticLightComponent::GetLumens() const
{
    return m_Lumens;
}

void AnalyticLightComponent::SetTemperature(float _Temperature)
{
    m_Temperature = _Temperature;
    m_bEffectiveColorDirty = true;
}

float AnalyticLightComponent::GetTemperature() const
{
    return m_Temperature;
}

void AnalyticLightComponent::SetColor(Float3 const& _Color)
{
    m_Color = _Color;
    m_bEffectiveColorDirty = true;
}

void AnalyticLightComponent::SetColor(float _R, float _G, float _B)
{
    m_Color.X = _R;
    m_Color.Y = _G;
    m_Color.Z = _B;
    m_bEffectiveColorDirty = true;
}

Float3 const& AnalyticLightComponent::GetColor() const
{
    return m_Color;
}

Float3 const& AnalyticLightComponent::GetEffectiveColor(float CosHalfConeAngle) const
{
    if (m_bEffectiveColorDirty || com_LightEnergyScale.IsModified())
    {
        const float EnergyUnitScale = 1.0f / com_LightEnergyScale.GetFloat();

        float candela;

        if (m_PhotometricProfile && !m_bPhotometricAsMask)
        {
            candela = m_LuminousIntensityScale * m_PhotometricProfile->GetIntensity();
        }
        else
        {
            const float LumensToCandela = 1.0f / Math::_2PI / (1.0f - CosHalfConeAngle);

            candela = m_Lumens * LumensToCandela;
        }

        // Animate light intensity
        candela *= GetAnimationBrightness();

        Color4 temperatureColor;
        temperatureColor.SetTemperature(GetTemperature());

        float finalScale  = candela * EnergyUnitScale;
        m_EffectiveColor[0] = m_Color[0] * temperatureColor[0] * finalScale;
        m_EffectiveColor[1] = m_Color[1] * temperatureColor[1] * finalScale;
        m_EffectiveColor[2] = m_Color[2] * temperatureColor[2] * finalScale;

        m_bEffectiveColorDirty = false;
    }
    return m_EffectiveColor;
}

void AnalyticLightComponent::SetDirection(Float3 const& _Direction)
{
    Float3x3 orientation;

    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis(orientation[0], orientation[1]);

    Quat rotation;
    rotation.FromMatrix(orientation);
    SetRotation(rotation);
}

Float3 AnalyticLightComponent::GetDirection() const
{
    return GetForwardVector();
}

void AnalyticLightComponent::SetWorldDirection(Float3 const& _Direction)
{
    Float3x3 orientation;
    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis(orientation[0], orientation[1]);

    Quat rotation;
    rotation.FromMatrix(orientation);
    SetWorldRotation(rotation);
}

Float3 AnalyticLightComponent::GetWorldDirection() const
{
    return GetWorldForwardVector();
}

void AnalyticLightComponent::PackLight(Float4x4 const& InViewMatrix, LightParameters& Light)
{
}
