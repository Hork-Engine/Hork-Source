/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Resource/AnimationPattern.h>

class ALightComponent : public ASceneComponent{
    AN_COMPONENT( ALightComponent, ASceneComponent )

public:
    /** Only directional light supports shadow casting yet */
    bool bCastShadow;

    void SetColor( Float3 const & _Color );

    void SetColor( float _R, float _G, float _B );

    Float3 const & GetColor() const;

    /** Set temperature of the light in Kelvin */
    void SetTemperature( float _Temperature );

    float GetTemperature() const;

    virtual void SetEnabled( bool _Enabled );

    bool IsEnabled() const { return bEnabled; }

    void SetAnimation( const char * _Pattern, float _Speed = 1.0f, float _Quantizer = 0.0f );

    void SetAnimation( AAnimationPattern * _Animation );

    AAnimationPattern * GetAnimation() { return Animation; }

protected:
    ALightComponent();

    void TickComponent( float _TimeStep ) override;

    float GetAnimationBrightness() const { return AnimationBrightness; }

    mutable bool   bEffectiveColorDirty;

private:
    bool           bEnabled;
    Float3         Color;
    float          Temperature;
    TRef< AAnimationPattern >Animation;
    float          AnimTime;
    float          AnimationBrightness;
};

class APunctualLightComponent : public ALightComponent {
    AN_COMPONENT( APunctualLightComponent, ALightComponent )

public:
    /** Internal. Used by Light Voxelizer. */
    int ListIndex;

    Float4 const & GetEffectiveColor( float CosHalfConeAngle ) const;

    void SetLumens( float Lumens );
    float GetLumens() const;

    BvAxisAlignedBox const & GetWorldBounds() const { return AABBWorldBounds; }

    Float4x4 const & GetOBBTransformInverse() const { return OBBTransformInverse; }

    virtual void PackLight( Float4x4 const & InViewMatrix, struct SClusterLight & Light );

protected:
    APunctualLightComponent();

    BvAxisAlignedBox AABBWorldBounds;
    Float4x4 OBBTransformInverse;

private:    
    mutable Float4 EffectiveColor;         // Composed from Temperature, Lumens, Color and AmbientIntensity   
    float Lumens;
};
