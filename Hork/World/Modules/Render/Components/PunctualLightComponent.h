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

#include <Hork/Geometry/BV/BvSphere.h>
#include <Hork/Geometry/BV/BvOrientedBox.h>
#include <Hork/Math/Quat.h>

#include <Hork/World/Component.h>
#include <Hork/World/TickFunction.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

class PunctualLightComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    static constexpr float      MinRadius = 0.01f;
    static constexpr float      MinConeAngle = 1.0f;
    static constexpr float      MaxConeAngle = 180.0f;

    // Public
    
    void                        SetLumens(float lumens) { m_Lumens = Math::Max(0.0f, lumens); }

    float                       GetLumens() const { return m_Lumens; }

    void                        SetTemperature(float temperature) { m_Temperature = temperature; }

    float                       GetTemperature() const { return m_Temperature; }

    void                        SetColor(Float3 const& color) { m_Color = color; }

    Float3 const&               GetColor() const { return m_Color; }

    void                        SetRadius(float radius);

    float                       GetRadius() const { return m_Radius; }

    void                        SetInnerConeAngle(float angle);

    float                       GetInnerConeAngle() const { return m_InnerConeAngle; }

    void                        SetOuterConeAngle(float angle);

    float                       GetOuterConeAngle() const { return m_OuterConeAngle; }

    void                        SetSpotExponent(float exponent) { m_SpotExponent = exponent; }

    float                       GetSpotExponent() const { return m_SpotExponent; }

    void                        SetPhotometric(uint16_t id) { m_PhotometricProfileID = id; }

    uint16_t                    GetPhotometric() const { return m_PhotometricProfileID; }

    void                        SetPhotometricAsMask(bool photometricAsMask) { m_PhotometricAsMask = photometricAsMask; }

    bool                        IsPhotometricAsMask() const { return m_PhotometricAsMask; }

    /// Luminous intensity scale for photometric profile
    void                        SetPhotometricIntensity(float intensity) { m_PhotometricIntensity = intensity; }

    float                       GetPhotometricIntensity() const { return m_PhotometricIntensity; }

    void                        SetCastShadow(bool castShadow) { m_CastShadow = castShadow; }

    bool                        IsCastShadow() const { return m_CastShadow; }

    BvSphere                    GetWorldBoundingSphere() const { return m_WorldBoundingSphere; }

    BvAxisAlignedBox const&     GetWorldBoundingBox() const { return m_WorldBoundingBox; };

    BvOrientedBox  const&       GetWorldOrientedBoundingBox() const { return m_WorldOrientedBoundingBox; }

    Float4x4 const&             GetOBBTransformInverse() const { return m_OBBTransformInverse; }

    /// Force update for world bonding box
    void                        UpdateWorldBoundingBox();

    Float3 const&               GetRenderPosition() const { return m_RenderTransform.Position; }

    // Internal

    void                        BeginPlay();
    void                        PostTransform();

    void                        PreRender(struct PreRenderContext const& context);

    void                        PackLight(Float4x4 const& viewMatrix, struct LightParameters& parameters);

    void                        UpdateEffectiveColor();

    void                        DrawDebug(DebugRenderer& renderer);

private:
    struct LightTransform
    {
        Float3 Position;
        Quat Rotation;
    };

    LightTransform              m_Transform[2];
    LightTransform              m_RenderTransform;
    uint32_t                    m_LastFrame{0};

    BvSphere                    m_WorldBoundingSphere;
    BvAxisAlignedBox            m_WorldBoundingBox;
    BvOrientedBox               m_WorldOrientedBoundingBox;

    Float4x4                    m_OBBTransformInverse;
    uint32_t                    m_PrimID;
    bool                        m_CastShadow = false;

    Float3                      m_Color = Float3(1.0f);
    float                       m_Temperature = 6590.0f;
    float                       m_Lumens = 3000.0f;
    float                       m_PhotometricIntensity = 1;
    Float3                      m_EffectiveColor; // Composed from Temperature, Lumens, Color
    uint16_t                    m_PhotometricProfileID = 0xffff;
    bool                        m_PhotometricAsMask = false;
    float                       m_Radius = 15;
    float                       m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
    float                       m_InnerConeAngle = 180;
    float                       m_OuterConeAngle = 180;
    float                       m_CosHalfInnerConeAngle = 0;
    float                       m_CosHalfOuterConeAngle = 0;
    float                       m_SpotExponent = 1.0f;
};

namespace TickGroup_PostTransform
{
    template <>
    HK_INLINE void InitializeTickFunction<PunctualLightComponent>(TickFunctionDesc& desc)
    {
        desc.TickEvenWhenPaused = true;
    }
}

HK_NAMESPACE_END
