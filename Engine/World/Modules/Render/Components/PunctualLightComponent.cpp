/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "PunctualLightComponent.h"
#include <Engine/Core/ConsoleVar.h>
#include <Engine/World/GameObject.h>
#include <Engine/World/DebugRenderer.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawPunctualLights("com_DrawPunctualLights"s, "0"s, CVAR_CHEAT);
ConsoleVar com_LightEnergyScale("com_LightEnergyScale"s, "16"s);

void PunctualLightComponent::UpdateEffectiveColor()
{
    const float EnergyUnitScale = 1.0f / com_LightEnergyScale.GetFloat();

    float candela;

    if (m_PhotometricProfileId && !m_bPhotometricAsMask)
    {
        candela = m_LuminousIntensityScale; // * PhotometricProfile->GetIntensity(); // TODO
    }
    else
    {
        float cosHalfConeAngle;
        if (m_InnerConeAngle < MaxConeAngle)
            cosHalfConeAngle = Math::Min(m_CosHalfOuterConeAngle, 0.9999f);
        else
            cosHalfConeAngle = -1.0f;

        const float lumensToCandela = 1.0f / Math::_2PI / (1.0f - cosHalfConeAngle);

        candela = m_Lumens * lumensToCandela;
    }

    // Animate light intensity
    //candela *= GetAnimationBrightness();

    Color4 temperatureColor;
    temperatureColor.SetTemperature(m_Temperature);

    float scale = candela * EnergyUnitScale;

    m_EffectiveColor[0] = m_Color[0] * temperatureColor[0] * scale;
    m_EffectiveColor[1] = m_Color[1] * temperatureColor[1] * scale;
    m_EffectiveColor[2] = m_Color[2] * temperatureColor[2] * scale;
}

void PunctualLightComponent::UpdateBoundingBox()
{
    if (m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
    {
        const float ToHalfAngleRadians = 0.5f / 180.0f * Math::_PI;
        const float HalfConeAngle = m_OuterConeAngle * ToHalfAngleRadians;
        const Float3 WorldPos = GetOwner()->GetWorldPosition();
        const float SinHalfConeAngle = Math::Sin(HalfConeAngle);

        // Compute cone OBB for voxelization
        m_OBBWorldBounds.Orient = GetOwner()->GetWorldRotation().ToMatrix3x3();

        const Float3 SpotDir = -m_OBBWorldBounds.Orient[2];

        //m_OBBWorldBounds.HalfSize.X = m_OBBWorldBounds.HalfSize.Y = tan( HalfConeAngle ) * m_Radius;
        m_OBBWorldBounds.HalfSize.X = m_OBBWorldBounds.HalfSize.Y = SinHalfConeAngle * m_Radius;
        m_OBBWorldBounds.HalfSize.Z = m_Radius * 0.5f;
        m_OBBWorldBounds.Center = WorldPos + SpotDir * (m_OBBWorldBounds.HalfSize.Z);

        // TODO: Optimize?
        Float4x4 OBBTransform = Float4x4::Translation(m_OBBWorldBounds.Center) * Float4x4(m_OBBWorldBounds.Orient) * Float4x4::Scale(m_OBBWorldBounds.HalfSize);
        m_OBBTransformInverse = OBBTransform.Inversed();

        // Compute cone AABB for culling
        m_AABBWorldBounds.Clear();
        m_AABBWorldBounds.AddPoint(WorldPos);
        Float3 v = WorldPos + SpotDir * m_Radius;
        Float3 vx = m_OBBWorldBounds.Orient[0] * m_OBBWorldBounds.HalfSize.X;
        Float3 vy = m_OBBWorldBounds.Orient[1] * m_OBBWorldBounds.HalfSize.X;
        m_AABBWorldBounds.AddPoint(v + vx);
        m_AABBWorldBounds.AddPoint(v - vx);
        m_AABBWorldBounds.AddPoint(v + vy);
        m_AABBWorldBounds.AddPoint(v - vy);

        // Compute cone Sphere bounds
        if (HalfConeAngle > Math::_PI / 4)
        {
            m_SphereWorldBounds.Radius = SinHalfConeAngle * m_Radius;
            m_SphereWorldBounds.Center = WorldPos + SpotDir * (m_CosHalfOuterConeAngle * m_Radius);
        }
        else
        {
            m_SphereWorldBounds.Radius = m_Radius / (2.0 * m_CosHalfOuterConeAngle);
            m_SphereWorldBounds.Center = WorldPos + SpotDir * m_SphereWorldBounds.Radius;
        }
    }
    else
    {
        m_SphereWorldBounds.Radius = m_Radius;
        m_SphereWorldBounds.Center = GetOwner()->GetWorldPosition();
        m_AABBWorldBounds.Mins = m_SphereWorldBounds.Center - m_Radius;
        m_AABBWorldBounds.Maxs = m_SphereWorldBounds.Center + m_Radius;
        m_OBBWorldBounds.Center = m_SphereWorldBounds.Center;
        m_OBBWorldBounds.HalfSize = Float3(m_SphereWorldBounds.Radius);
        m_OBBWorldBounds.Orient.SetIdentity();

        // TODO: Optimize?
        Float4x4 OBBTransform = Float4x4::Translation(m_OBBWorldBounds.Center) * Float4x4::Scale(m_OBBWorldBounds.HalfSize);
        m_OBBTransformInverse = OBBTransform.Inversed();
    }
}

void PunctualLightComponent::PackLight(Float4x4 const& viewMatrix, LightParameters& parameters)
{
    //PhotometricProfile* profile = GetPhotometricProfile();

    UpdateBoundingBox();
    UpdateEffectiveColor();

    auto& worldPosition = GetOwner()->GetWorldPosition();

    parameters.Position = Float3(viewMatrix * worldPosition);
    parameters.Radius = GetRadius();
    parameters.InverseSquareRadius = m_InverseSquareRadius;
    parameters.Direction = viewMatrix.TransformAsFloat3x3(m_OBBWorldBounds.Orient[2]);// viewMatrix.TransformAsFloat3x3(-GetWorldDirection()); // Only for photometric light
    parameters.RenderMask = ~0u;                                                   //RenderMask; // TODO
    parameters.PhotometricProfile = 0xffffffff;//profile ? profile->GetPhotometricProfileIndex() : 0xffffffff; // TODO

    if (m_InnerConeAngle < MaxConeAngle)
    {
        parameters.CosHalfOuterConeAngle = m_CosHalfOuterConeAngle;
        parameters.CosHalfInnerConeAngle = m_CosHalfInnerConeAngle;
        parameters.SpotExponent = m_SpotExponent;
        parameters.Color = m_EffectiveColor;
        parameters.LightType = CLUSTER_LIGHT_SPOT;
    }
    else
    {   
        parameters.CosHalfOuterConeAngle = 0;
        parameters.CosHalfInnerConeAngle = 0;
        parameters.SpotExponent = 0;
        parameters.Color = m_EffectiveColor;
        parameters.LightType = CLUSTER_LIGHT_POINT;  
    }
}

void PunctualLightComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawPunctualLights)
    {
        //if (m_Primitive->VisPass != renderer.GetVisPass()) // TODO
        //    return;

        renderer.SetDepthTest(false);

        Float3 pos = GetOwner()->GetWorldPosition();

        if (m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
        {
            Float3x3 orient = GetOwner()->GetWorldRotation().ToMatrix3x3();

            renderer.SetColor(Color4(0.5f, 0.5f, 0.5f, 1));
            renderer.DrawCone(pos, orient, m_Radius, Math::Radians(m_InnerConeAngle) * 0.5f);
            renderer.SetColor(Color4(1, 1, 1, 1));
            renderer.DrawCone(pos, orient, m_Radius, Math::Radians(m_OuterConeAngle) * 0.5f);
        }
        else
        {
            renderer.SetColor(Color4(1, 1, 1, 1));
            renderer.DrawSphere(pos, m_Radius);
        }
    }
}

HK_FORCEINLINE float Quantize(float frac, float quantizer)
{
    return quantizer > 0.0f ? Math::Floor(frac * quantizer) / quantizer : frac;
}

// Converts string to brightness
float SamplePattern(StringView pattern, float position, float quantizer = 0)
{
    int frameCount = pattern.Size();
    if (frameCount > 0)
    {
        int keyframe = Math::Floor(position);
        int nextframe = keyframe + 1;

        float lerp = position - keyframe;

        keyframe %= frameCount;
        nextframe %= frameCount;

        float a = (Math::Clamp(pattern[keyframe], 'a', 'z') - 'a') / 26.0f;
        float b = (Math::Clamp(pattern[nextframe], 'a', 'z') - 'a') / 26.0f;

        return Math::Lerp(a, b, Quantize(lerp, quantizer));
    }

    return 1.0f;
}

HK_NAMESPACE_END
