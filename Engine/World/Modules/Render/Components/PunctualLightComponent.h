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

#pragma once

#include <Engine/Geometry/BV/BvSphere.h>
#include <Engine/Geometry/BV/BvOrientedBox.h>

#include <Engine/World/Component.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

class PunctualLightComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    static constexpr float MinRadius = 0.01f;
    static constexpr float MinConeAngle = 1.0f;
    static constexpr float MaxConeAngle = 180.0f;

    // Public
    
    void SetLumens(float lumens)
    {
        m_Lumens = Math::Max(0.0f, lumens);
    }

    float GetLumens() const
    {
        return m_Lumens;
    }

    void SetTemperature(float temperature)
    {
        m_Temperature = temperature;
    }

    float GetTemperature() const
    {
        return m_Temperature;
    }

    void SetColor(Float3 const& color)
    {
        m_Color = color;
    }

    Float3 const& GetColor() const
    {
        return m_Color;
    }

    void SetRadius(float radius)
    {
        m_Radius = Math::Max(MinRadius, radius);
        m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
    }

    float GetRadius() const
    {
        return m_Radius;
    }

    void SetInnerConeAngle(float angle)
    {
        m_InnerConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
        m_CosHalfInnerConeAngle = Math::Cos(Math::Radians(m_InnerConeAngle * 0.5f));
    }

    float GetInnerConeAngle() const
    {
        return m_InnerConeAngle;
    }

    void SetOuterConeAngle(float angle)
    {
        m_OuterConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
        m_CosHalfOuterConeAngle = Math::Cos(Math::Radians(m_OuterConeAngle * 0.5f));
    }

    float GetOuterConeAngle() const
    {
        return m_OuterConeAngle;
    }

    void SetSpotExponent(float exponent)
    {
        m_SpotExponent = exponent;
    }

    float GetSpotExponent() const
    {
        return m_SpotExponent;
    }

    void SetPhotometric(uint16_t id)
    {
        m_PhotometricProfileId = id;
    }

    uint16_t GetPhotometric() const
    {
        return m_PhotometricProfileId;
    }

    void SetPhotometricAsMask(bool bPhotometricAsMask)
    {
        m_bPhotometricAsMask = bPhotometricAsMask;
    }

    bool IsPhotometricAsMask() const
    {
        return m_bPhotometricAsMask;
    }

    /** Luminous intensity scale for photometric profile */
    void SetLuminousIntensityScale(float intensityScale)
    {
        m_LuminousIntensityScale = intensityScale;
    }

    float GetLuminousIntensityScale() const
    {
        return m_LuminousIntensityScale;
    }

    void SetCastShadow(bool castShadow)
    {
        m_CastShadow = castShadow;
    }

    bool IsCastShadow() const
    {
        return m_CastShadow;
    }

    // Internal

    void PackLight(Float4x4 const& viewMatrix, struct LightParameters& parameters);

    void UpdateEffectiveColor();
    void UpdateBoundingBox();

    void DrawDebug(DebugRenderer& renderer);

    // TODO: Make private:

    BvSphere         m_SphereWorldBounds;
    BvOrientedBox    m_OBBWorldBounds;

    BvAxisAlignedBox m_AABBWorldBounds;
    Float4x4         m_OBBTransformInverse;
    uint32_t         m_PrimID;
    bool             m_CastShadow = false;

    Float3           m_Color = Float3(1.0f);
    float            m_Temperature = 6590.0f;
    float            m_Lumens = 3000.0f;
    float            m_LuminousIntensityScale = 1;
    Float3           m_EffectiveColor; // Composed from Temperature, Lumens, Color
    uint16_t         m_PhotometricProfileId = 0;
    bool             m_bPhotometricAsMask = false;
    float            m_Radius = 15;
    float            m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
    float            m_InnerConeAngle = 180;
    float            m_OuterConeAngle = 180;
    float            m_CosHalfInnerConeAngle = 0;
    float            m_CosHalfOuterConeAngle = 0;
    float            m_SpotExponent = 1.0f;
};

HK_NAMESPACE_END
