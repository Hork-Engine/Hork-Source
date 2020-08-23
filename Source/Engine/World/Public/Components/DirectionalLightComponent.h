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

#include "LightComponent.h"

class ADirectionalLightComponent : public ALightComponent {
    AN_COMPONENT( ADirectionalLightComponent, ALightComponent )

    friend class ARenderWorld;

public:
    bool bCastShadow = true;

    void SetIlluminance( float IlluminanceInLux );
    float GetIlluminance() const;

    /** Set temperature of the light in Kelvin */
    void SetTemperature( float _Temperature );

    /** Get temperature of the light in Kelvin */
    float GetTemperature() const;

    void SetColor( Float3 const & _Color );
    void SetColor( float _R, float _G, float _B );
    Float3 const & GetColor() const;

    /** Set light direction in local space */
    void SetDirection( Float3 const & _Direction );

    /** Get light direction in local space */
    Float3 GetDirection() const;

    /** Set light direction in world space */
    void SetWorldDirection( Float3 const & _Direction );

    /** Get light direction in world space */
    Float3 GetWorldDirection() const;

    void SetShadowMaxDistance( float MaxDistance ) { ShadowMaxDistance = MaxDistance; }
    int GetShadowMaxDistance() const { return ShadowMaxDistance; }

    void SetShadowCascadeOffset( float Offset ) { ShadowCascadeOffset = Offset; }
    int GetShadowCascadeOffset() const { return ShadowCascadeOffset; }

    void SetMaxShadowCascades( int _MaxShadowCascades );
    int GetMaxShadowCascades() const;

    Float4 const & GetEffectiveColor() const;

    void AddShadowmapCascades( struct SRenderView * View, int * pFirstCascade, int * pNumCascades );

    ADirectionalLightComponent * GetNext() { return Next; }
    ADirectionalLightComponent * GetPrev() { return Prev; }

protected:
    ADirectionalLightComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    void OnTransformDirty() override;
    void DrawDebug( ADebugRenderer * InRenderer ) override;

private:
    float IlluminanceInLux;
    float Temperature;
    Float3 Color;
    mutable Float4 EffectiveColor;
    float ShadowMaxDistance;
    float ShadowCascadeOffset;
    int MaxShadowCascades;
    ADirectionalLightComponent * Next;
    ADirectionalLightComponent * Prev;
};
