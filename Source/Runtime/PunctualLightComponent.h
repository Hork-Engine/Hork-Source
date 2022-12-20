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

#pragma once

#include "LightComponent.h"
#include "Level.h"
#include "PhotometricProfile.h"

class PunctualLightComponent : public LightComponent
{
    HK_COMPONENT(PunctualLightComponent, LightComponent)

public:
    static constexpr float MinRadius = 0.01f;
    static constexpr float MinConeAngle = 1.0f;
    static constexpr float MaxConeAngle = 180.0f;

    void SetEnabled(bool bEnabled) override;

    void SetRadius(float radius);
    float GetRadius() const { return m_Radius; }

    void SetInnerConeAngle(float angle);
    float GetInnerConeAngle() const;

    void SetOuterConeAngle(float angle);
    float GetOuterConeAngle() const;

    void SetSpotExponent(float exponent);
    float GetSpotExponent() const;

    void SetLumens(float lumens);
    float GetLumens() const;

    /** Set photometric profile for the light source */
    void SetPhotometricProfile(PhotometricProfile* profile);
    PhotometricProfile* GetPhotometricProfile() const { return m_PhotometricProfile; }

    /** If true, photometric profile will be used as mask to modify luminous intensity of the light source.
    If false, luminous intensity will be taken from photometric profile */
    void SetPhotometricAsMask(bool bPhotometricAsMask);
    bool IsPhotometricAsMask() const { return m_bPhotometricAsMask; }

    /** Luminous intensity scale for photometric profile */
    void SetLuminousIntensityScale(float intensityScale);
    float GetLuminousIntensityScale() const { return m_LuminousIntensityScale; }

    void SetVisibilityGroup(VISIBILITY_GROUP visibilityGroup);
    VISIBILITY_GROUP GetVisibilityGroup() const;
    
    Float3 const& GetEffectiveColor(float cosHalfConeAngle) const;

    BvAxisAlignedBox const& GetWorldBounds() const { return m_AABBWorldBounds; }

    BvSphere const& GetSphereWorldBounds() const { return m_SphereWorldBounds; }

    Float4x4 const& GetOBBTransformInverse() const { return m_OBBTransformInverse; }

    void PackLight(Float4x4 const& viewMatrix, struct LightParameters& light);

private:
    PunctualLightComponent();
    ~PunctualLightComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnTransformDirty() override;

    void DrawDebug(DebugRenderer* InRenderer) override;

    void UpdateWorldBounds();

    BvSphere         m_SphereWorldBounds;
    BvOrientedBox    m_OBBWorldBounds;

    BvAxisAlignedBox m_AABBWorldBounds;
    Float4x4         m_OBBTransformInverse;
    PrimitiveDef*    m_Primitive;

    float                    m_Radius = 15;
    float                    m_InverseSquareRadius;
    float                    m_InnerConeAngle = 180;
    float                    m_OuterConeAngle = 180;
    float                    m_CosHalfInnerConeAngle;
    float                    m_CosHalfOuterConeAngle;
    float                    m_SpotExponent = 1.0f;
    TRef<PhotometricProfile> m_PhotometricProfile;
    float                    m_Lumens = 3000.0f;
    float                    m_LuminousIntensityScale = 1.0f;
    mutable Float3           m_EffectiveColor; // Composed from Temperature, Lumens, Color
    bool                     m_bPhotometricAsMask = false;
};
