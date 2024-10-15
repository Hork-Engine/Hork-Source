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
#include <Hork/Core/ConsoleVar.h>
#include <Hork/Runtime/World/GameObject.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawPunctualLights("com_DrawPunctualLights"_s, "0"_s, CVAR_CHEAT);
ConsoleVar com_LightEnergyScale("com_LightEnergyScale"_s, "16"_s);

void PunctualLightComponent::SetRadius(float radius)
{
    m_Radius = Math::Max(MinRadius, radius);
    m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
}

void PunctualLightComponent::SetInnerConeAngle(float angle)
{
    m_InnerConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
    m_CosHalfInnerConeAngle = Math::Cos(Math::Radians(m_InnerConeAngle * 0.5f));
}

void PunctualLightComponent::SetOuterConeAngle(float angle)
{
    m_OuterConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
    m_CosHalfOuterConeAngle = Math::Cos(Math::Radians(m_OuterConeAngle * 0.5f));
}

void PunctualLightComponent::BeginPlay()
{
    m_Transform[0].Position = m_Transform[1].Position = GetOwner()->GetWorldPosition();
    m_Transform[0].Rotation = m_Transform[1].Rotation = GetOwner()->GetWorldRotation();

    m_RenderTransform = m_Transform[0];

    UpdateWorldBoundingBox();
}

void PunctualLightComponent::PostTransform()
{
    auto index = GetWorld()->GetTick().StateIndex;

    m_Transform[index].Position = GetOwner()->GetWorldPosition();
    m_Transform[index].Rotation = GetOwner()->GetWorldRotation();
}

void PunctualLightComponent::PreRender(PreRenderContext const& context)
{
    // TODO: Call only for dynamic light

    if (m_LastFrame == context.FrameNum)
        return;  // already called for this frame

    m_RenderTransform.Position = Math::Lerp (m_Transform[context.Prev].Position, m_Transform[context.Cur].Position, context.Frac);
    m_RenderTransform.Rotation = Math::Slerp(m_Transform[context.Prev].Rotation, m_Transform[context.Cur].Rotation, context.Frac);

    m_LastFrame = context.FrameNum;

    UpdateWorldBoundingBox();
}

void PunctualLightComponent::UpdateEffectiveColor()
{
    const float EnergyUnitScale = 1.0f / com_LightEnergyScale.GetFloat();

    float candela;

    if (m_PhotometricProfileID != 0xffff && !m_PhotometricAsMask)
    {
        candela = m_PhotometricIntensity;
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

    Color3 temperatureColor;
    temperatureColor.SetTemperature(m_Temperature);

    float scale = candela * EnergyUnitScale;

    m_EffectiveColor[0] = ColorUtils::LinearFromSRGB_Fast(m_Color[0] * temperatureColor[0]) * scale;
    m_EffectiveColor[1] = ColorUtils::LinearFromSRGB_Fast(m_Color[1] * temperatureColor[1]) * scale;
    m_EffectiveColor[2] = ColorUtils::LinearFromSRGB_Fast(m_Color[2] * temperatureColor[2]) * scale;
}

void PunctualLightComponent::UpdateWorldBoundingBox()
{
    if (m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
    {
        float ToHalfAngleRadians = 0.5f / 180.0f * Math::_PI;
        float HalfConeAngle = m_OuterConeAngle * ToHalfAngleRadians;
        float SinHalfConeAngle = Math::Sin(HalfConeAngle);

        // Compute cone OBB for voxelization
        m_WorldOrientedBoundingBox.Orient = m_RenderTransform.Rotation.ToMatrix3x3();

        Float3 spotDir = -m_WorldOrientedBoundingBox.Orient[2];

        //m_WorldOrientedBoundingBox.HalfSize.X = m_WorldOrientedBoundingBox.HalfSize.Y = tan( HalfConeAngle ) * m_Radius;
        m_WorldOrientedBoundingBox.HalfSize.X = m_WorldOrientedBoundingBox.HalfSize.Y = SinHalfConeAngle * m_Radius;
        m_WorldOrientedBoundingBox.HalfSize.Z = m_Radius * 0.5f;
        m_WorldOrientedBoundingBox.Center = m_RenderTransform.Position + spotDir * (m_WorldOrientedBoundingBox.HalfSize.Z);

        // TODO: Optimize?
        Float4x4 OBBTransform = Float4x4::sTranslation(m_WorldOrientedBoundingBox.Center) * Float4x4(m_WorldOrientedBoundingBox.Orient) * Float4x4::sScale(m_WorldOrientedBoundingBox.HalfSize);
        m_OBBTransformInverse = OBBTransform.Inversed();

        // Compute cone AABB for culling
        m_WorldBoundingBox.Clear();
        m_WorldBoundingBox.AddPoint(m_RenderTransform.Position);
        Float3 v = m_RenderTransform.Position + spotDir * m_Radius;
        Float3 vx = m_WorldOrientedBoundingBox.Orient[0] * m_WorldOrientedBoundingBox.HalfSize.X;
        Float3 vy = m_WorldOrientedBoundingBox.Orient[1] * m_WorldOrientedBoundingBox.HalfSize.X;
        m_WorldBoundingBox.AddPoint(v + vx);
        m_WorldBoundingBox.AddPoint(v - vx);
        m_WorldBoundingBox.AddPoint(v + vy);
        m_WorldBoundingBox.AddPoint(v - vy);

        // Compute cone Sphere bounds
        if (HalfConeAngle > Math::_PI / 4)
        {
            m_WorldBoundingSphere.Radius = SinHalfConeAngle * m_Radius;
            m_WorldBoundingSphere.Center = m_RenderTransform.Position + spotDir * (m_CosHalfOuterConeAngle * m_Radius);
        }
        else
        {
            m_WorldBoundingSphere.Radius = m_Radius / (2.0 * m_CosHalfOuterConeAngle);
            m_WorldBoundingSphere.Center = m_RenderTransform.Position + spotDir * m_WorldBoundingSphere.Radius;
        }
    }
    else
    {
        m_WorldBoundingSphere.Radius = m_Radius;
        m_WorldBoundingSphere.Center = m_RenderTransform.Position;
        m_WorldBoundingBox.Mins = m_WorldBoundingSphere.Center - m_Radius;
        m_WorldBoundingBox.Maxs = m_WorldBoundingSphere.Center + m_Radius;
        m_WorldOrientedBoundingBox.Center = m_WorldBoundingSphere.Center;
        m_WorldOrientedBoundingBox.HalfSize = Float3(m_WorldBoundingSphere.Radius);
        m_WorldOrientedBoundingBox.Orient.SetIdentity();

        // TODO: Optimize?
        Float4x4 OBBTransform = Float4x4::sTranslation(m_WorldOrientedBoundingBox.Center) * Float4x4::sScale(m_WorldOrientedBoundingBox.HalfSize);
        m_OBBTransformInverse = OBBTransform.Inversed();
    }
}

void PunctualLightComponent::PackLight(Float4x4 const& viewMatrix, LightParameters& parameters)
{
    UpdateEffectiveColor();

    parameters.Position = Float3(viewMatrix * m_RenderTransform.Position);
    parameters.Radius = GetRadius();
    parameters.InverseSquareRadius = m_InverseSquareRadius;
    parameters.Direction = viewMatrix.TransformAsFloat3x3(m_RenderTransform.Rotation.ZAxis());// viewMatrix.TransformAsFloat3x3(m_WorldOrientedBoundingBox.Orient[2]); // Only for photometric light
    parameters.RenderMask = ~0u;                                                   //RenderMask; // TODO
    parameters.PhotometricProfile = m_PhotometricProfileID;

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

        Float3 pos = m_RenderTransform.Position;

        if (m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
        {
            renderer.SetColor(Color4(0.5f, 0.5f, 0.5f, 1));
            renderer.DrawCone(pos, m_WorldOrientedBoundingBox.Orient, m_Radius, m_InnerConeAngle * 0.5f);
            renderer.SetColor(Color4(1, 1, 1, 1));
            renderer.DrawCone(pos, m_WorldOrientedBoundingBox.Orient, m_Radius, m_OuterConeAngle * 0.5f);
        }
        else
        {
            renderer.SetColor(Color4(1, 1, 1, 1));
            renderer.DrawSphere(pos, m_Radius);
        }
    }
}

HK_NAMESPACE_END
