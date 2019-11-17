/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include "SceneComponent.h"

#include <Engine/Resource/Public/AnimationPattern.h>

class ABaseLightComponent : public ASceneComponent {
    AN_COMPONENT( ABaseLightComponent, ASceneComponent )

public:
    enum { RENDERING_GROUP_DEFAULT = 1 };

    /** Rendering group to filter lights during rendering */
    int RenderingGroup;

    /** Only directional light supports shadow casting yet */
    bool bCastShadow;

    void SetColor( Float3 const & _Color );
    void SetColor( float _R, float _G, float _B );
    Float3 const & GetColor() const;

    Float4 const & GetEffectiveColor() const;

    // Set temperature of the light in Kelvin
    void SetTemperature( float _Temperature );
    float GetTemperature() const;

    void SetLumens( float _Lumens );
    float GetLumens() const;

    void SetAmbientIntensity( float _Intensity );
    float GetAmbientIntensity() const;

    void SetEnabled( bool _Enabled );
    bool IsEnabled() const { return bEnabled; }

    void SetAnimation( const char * _Pattern, float _Speed = 1.0f, float _Quantizer = 0.0f );
    void SetAnimation( AAnimationPattern * _Animation );
    AAnimationPattern * GetAnimation() { return Animation; }

protected:
    ABaseLightComponent();

    void TickComponent( float _TimeStep ) override;

private:
    Float3         Color;
    mutable Float4 EffectiveColor;         // Composed from Temperature, Lumens, Color and AmbientIntensity
    mutable bool   bEffectiveColorDirty;
    bool           bEnabled;
    float          Temperature;
    float          Lumens;
    TRef< AAnimationPattern >Animation;
    float          AnimTime;
    float          AnimationBrightness;
};
