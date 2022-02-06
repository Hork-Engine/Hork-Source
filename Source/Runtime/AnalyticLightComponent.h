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

#include "PunctualLightComponent.h"
#include "PhotometricProfile.h"

class AAnalyticLightComponent : public APunctualLightComponent
{
    HK_COMPONENT(AAnalyticLightComponent, APunctualLightComponent)

public:
    /** Allow mesh to cast shadows on the world */
    void SetCastShadow(bool _CastShadow) { bCastShadow = _CastShadow; }

    /** Is cast shadows enabled */
    bool IsCastShadow() const { return bCastShadow; }

    /** Set photometric profile for the light source */
    void                 SetPhotometricProfile(APhotometricProfile* Profile);
    APhotometricProfile* GetPhotometricProfile() const { return PhotometricProfile; }

    /** If true, photometric profile will be used as mask to modify luminous intensity of the light source.
    If false, luminous intensity will be taken from photometric profile */
    void SetPhotometricAsMask(bool bPhotometricAsMask);
    bool IsPhotometricAsMask() const { return bPhotometricAsMask; }

    /** Luminous intensity scale for photometric profile */
    void  SetLuminousIntensityScale(float IntensityScale);
    float GetLuminousIntensityScale() const { return LuminousIntensityScale; }

    void  SetLumens(float Lumens);
    float GetLumens() const;

    /** Set temperature of the light in Kelvin */
    void  SetTemperature(float _Temperature);
    float GetTemperature() const;

    void          SetColor(Float3 const& _Color);
    void          SetColor(float _R, float _G, float _B);
    Float3 const& GetColor() const;

    Float3 const& GetEffectiveColor(float CosHalfConeAngle) const;

    void   SetDirection(Float3 const& _Direction);
    Float3 GetDirection() const;

    void   SetWorldDirection(Float3 const& _Direction);
    Float3 GetWorldDirection() const;

    /** Internal */
    virtual void PackLight(Float4x4 const& InViewMatrix, struct SLightParameters& Light);

protected:
    AAnalyticLightComponent();

private:
    TRef<APhotometricProfile> PhotometricProfile;
    float                     Lumens;
    float                     LuminousIntensityScale = 1.0f;
    float                     Temperature;
    Float3                    Color;
    mutable Float3            EffectiveColor; // Composed from Temperature, Lumens, Color
    bool                      bPhotometricAsMask = false;
    bool                      bCastShadow        = false;
};
