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
    float GetTemperature() const;

    void SetColor( Float3 const & _Color );
    void SetColor( float _R, float _G, float _B );
    Float3 const & GetColor() const;

    void SetDirection( Float3 const & _Direction );
    Float3 GetDirection() const;

    void SetWorldDirection( Float3 const & _Direction );
    Float3 GetWorldDirection() const;

    void SetMaxShadowCascades( int _MaxShadowCascades );
    int GetMaxShadowCascades() const;

    Float4 const & GetEffectiveColor() const;

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
    int MaxShadowCascades;
//    mutable Float4x4 LightViewMatrix;
//    mutable Float4x4 ShadowMatrix;
//    mutable bool bShadowMatrixDirty;
    ADirectionalLightComponent * Next;
    ADirectionalLightComponent * Prev;
};
