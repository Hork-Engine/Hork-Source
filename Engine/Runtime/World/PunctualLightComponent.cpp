/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#include "World.h"

#include <Engine/Runtime/DebugRenderer.h>

#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_LightEnergyScale("com_LightEnergyScale"s, "16"s);
ConsoleVar com_DrawPunctualLights("com_DrawPunctualLights"s, "0"s, CVAR_CHEAT);

HK_BEGIN_CLASS_META(PunctualLightComponent)
HK_PROPERTY(Radius, SetRadius, GetRadius, HK_PROPERTY_DEFAULT)
HK_PROPERTY(InnerConeAngle, SetInnerConeAngle, GetInnerConeAngle, HK_PROPERTY_DEFAULT)
HK_PROPERTY(OuterConeAngle, SetOuterConeAngle, GetOuterConeAngle, HK_PROPERTY_DEFAULT)
HK_PROPERTY(SpotExponent, SetSpotExponent, GetSpotExponent, HK_PROPERTY_DEFAULT)
HK_PROPERTY(Lumens, SetLumens, GetLumens, HK_PROPERTY_DEFAULT)
//HK_PROPERTY(PhotometricProfile, SetPhotometricProfile, GetPhotometricProfile, HK_PROPERTY_DEFAULT) // TODO
HK_PROPERTY(PhotometricAsMask, SetPhotometricAsMask, IsPhotometricAsMask, HK_PROPERTY_DEFAULT)
HK_PROPERTY(LuminousIntensityScale, SetLuminousIntensityScale, GetLuminousIntensityScale, HK_PROPERTY_DEFAULT)
//HK_PROPERTY(VisibilityGroup, SetVisibilityGroup, GetVisibilityGroup, HK_PROPERTY_DEFAULT) // TODO
HK_END_CLASS_META()

PunctualLightComponent::PunctualLightComponent()
{
    m_AABBWorldBounds.Clear();
    m_OBBTransformInverse.Clear();

    m_Primitive = VisibilitySystem::AllocatePrimitive();
    m_Primitive->Owner = this;
    m_Primitive->Type = VSD_PRIMITIVE_SPHERE;
    m_Primitive->VisGroup = VISIBILITY_GROUP_DEFAULT;
    m_Primitive->QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;

    m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
    m_CosHalfInnerConeAngle = Math::Cos(Math::Radians(m_InnerConeAngle * 0.5f));
    m_CosHalfOuterConeAngle = Math::Cos(Math::Radians(m_OuterConeAngle * 0.5f));

    UpdateWorldBounds();
}

PunctualLightComponent::~PunctualLightComponent()
{
    VisibilitySystem::DeallocatePrimitive(m_Primitive);
}

void PunctualLightComponent::InitializeComponent()
{
    Super::InitializeComponent();

    GetWorld()->VisibilitySystem.AddPrimitive(m_Primitive);
}

void PunctualLightComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetWorld()->VisibilitySystem.RemovePrimitive(m_Primitive);
}

void PunctualLightComponent::SetVisibilityGroup(VISIBILITY_GROUP VisibilityGroup)
{
    m_Primitive->SetVisibilityGroup(VisibilityGroup);
}

VISIBILITY_GROUP PunctualLightComponent::GetVisibilityGroup() const
{
    return m_Primitive->GetVisibilityGroup();
}

void PunctualLightComponent::SetEnabled(bool _Enabled)
{
    Super::SetEnabled(_Enabled);

    if (_Enabled)
    {
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    }
    else
    {
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

void PunctualLightComponent::SetLumens(float _Lumens)
{
    m_Lumens = Math::Max(0.0f, _Lumens);
    m_bEffectiveColorDirty = true;
}

float PunctualLightComponent::GetLumens() const
{
    return m_Lumens;
}

void PunctualLightComponent::SetRadius(float radius)
{
    m_Radius = Math::Max(MinRadius, radius);
    m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);

    UpdateWorldBounds();
}

void PunctualLightComponent::SetInnerConeAngle(float angle)
{
    m_InnerConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
    m_CosHalfInnerConeAngle = Math::Cos(Math::Radians(m_InnerConeAngle * 0.5f));
}

float PunctualLightComponent::GetInnerConeAngle() const
{
    return m_InnerConeAngle;
}

void PunctualLightComponent::SetOuterConeAngle(float angle)
{
    m_OuterConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
    m_CosHalfOuterConeAngle = Math::Cos(Math::Radians(m_OuterConeAngle * 0.5f));

    UpdateWorldBounds();
}

float PunctualLightComponent::GetOuterConeAngle() const
{
    return m_OuterConeAngle;
}

void PunctualLightComponent::SetSpotExponent(float exponent)
{
    m_SpotExponent = exponent;
}

float PunctualLightComponent::GetSpotExponent() const
{
    return m_SpotExponent;
}

void PunctualLightComponent::SetPhotometricProfile(PhotometricProfile* Profile)
{
    m_PhotometricProfile = Profile;
    m_bEffectiveColorDirty = true;
}

void PunctualLightComponent::SetPhotometricAsMask(bool bPhotometricAsMask)
{
    m_bPhotometricAsMask = bPhotometricAsMask;
    m_bEffectiveColorDirty = true;
}

void PunctualLightComponent::SetLuminousIntensityScale(float IntensityScale)
{
    m_LuminousIntensityScale = IntensityScale;
    m_bEffectiveColorDirty = true;
}

Float3 const& PunctualLightComponent::GetEffectiveColor(float CosHalfConeAngle) const
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

        float finalScale = candela * EnergyUnitScale;
        m_EffectiveColor[0] = m_Color[0] * temperatureColor[0] * finalScale;
        m_EffectiveColor[1] = m_Color[1] * temperatureColor[1] * finalScale;
        m_EffectiveColor[2] = m_Color[2] * temperatureColor[2] * finalScale;

        m_bEffectiveColorDirty = false;
    }
    return m_EffectiveColor;
}

void PunctualLightComponent::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void PunctualLightComponent::UpdateWorldBounds()
{
    if (m_InnerConeAngle < MaxConeAngle)
    {
        const float ToHalfAngleRadians = 0.5f / 180.0f * Math::_PI;
        const float HalfConeAngle = m_OuterConeAngle * ToHalfAngleRadians;
        const Float3 WorldPos = GetWorldPosition();
        const float SinHalfConeAngle = Math::Sin(HalfConeAngle);

        // Compute cone OBB for voxelization
        m_OBBWorldBounds.Orient = GetWorldRotation().ToMatrix3x3();

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
        m_SphereWorldBounds.Center = GetWorldPosition();
        m_AABBWorldBounds.Mins = m_SphereWorldBounds.Center - m_Radius;
        m_AABBWorldBounds.Maxs = m_SphereWorldBounds.Center + m_Radius;
        m_OBBWorldBounds.Center = m_SphereWorldBounds.Center;
        m_OBBWorldBounds.HalfSize = Float3(m_SphereWorldBounds.Radius);
        m_OBBWorldBounds.Orient.SetIdentity();

        // TODO: Optimize?
        Float4x4 OBBTransform = Float4x4::Translation(m_OBBWorldBounds.Center) * Float4x4::Scale(m_OBBWorldBounds.HalfSize);
        m_OBBTransformInverse = OBBTransform.Inversed();
    }

    m_Primitive->Sphere = m_SphereWorldBounds;

    if (IsInitialized())
    {
        GetWorld()->VisibilitySystem.MarkPrimitive(m_Primitive);
    }
}

void PunctualLightComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawPunctualLights)
    {
        if (m_Primitive->VisPass == InRenderer->GetVisPass())
        {
            Float3 pos = GetWorldPosition();
            if (m_InnerConeAngle < MaxConeAngle)
            {
                Float3x3 orient = GetWorldRotation().ToMatrix3x3();
                InRenderer->SetDepthTest(false);
                InRenderer->SetColor(Color4(0.5f, 0.5f, 0.5f, 1));
                InRenderer->DrawCone(pos, orient, m_Radius, Math::Radians(m_InnerConeAngle) * 0.5f);
                InRenderer->SetColor(Color4(1, 1, 1, 1));
                InRenderer->DrawCone(pos, orient, m_Radius, Math::Radians(m_OuterConeAngle) * 0.5f);
            }
            else
            {
                InRenderer->SetDepthTest(false);
                InRenderer->SetColor(Color4(1, 1, 1, 1));
                InRenderer->DrawSphere(pos, m_Radius);
            }
        }
    }
}

void PunctualLightComponent::PackLight(Float4x4 const& InViewMatrix, LightParameters& Light)
{
    PhotometricProfile* profile = GetPhotometricProfile();

    Light.Position = Float3(InViewMatrix * GetWorldPosition());
    Light.Radius = GetRadius();
    Light.InverseSquareRadius = m_InverseSquareRadius;
    Light.Direction = InViewMatrix.TransformAsFloat3x3(-GetWorldDirection()); // Only for photometric light
    Light.RenderMask = ~0u;                                                   //RenderMask; // TODO
    Light.PhotometricProfile = profile ? profile->GetPhotometricProfileIndex() : 0xffffffff;

    if (m_InnerConeAngle < MaxConeAngle)
    {
        Light.CosHalfOuterConeAngle = m_CosHalfOuterConeAngle;
        Light.CosHalfInnerConeAngle = m_CosHalfInnerConeAngle;
        Light.SpotExponent = m_SpotExponent;
        Light.Color = GetEffectiveColor(Math::Min(m_CosHalfOuterConeAngle, 0.9999f));
        Light.LightType = CLUSTER_LIGHT_SPOT;
    }
    else
    {   
        Light.CosHalfOuterConeAngle = 0;
        Light.CosHalfInnerConeAngle = 0;
        Light.SpotExponent = 0;
        Light.Color = GetEffectiveColor(-1.0f);
        Light.LightType = CLUSTER_LIGHT_POINT;  
    }
}

HK_NAMESPACE_END
