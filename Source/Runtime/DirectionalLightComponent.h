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
#include <Core/IntrusiveLinkedListMacro.h>

class DirectionalLightComponent : public LightComponent
{
    HK_COMPONENT(DirectionalLightComponent, LightComponent)

public:
    TLink<DirectionalLightComponent> Link;

    void  SetIlluminance(float illuminanceInLux);
    float GetIlluminance() const;

    /** Set temperature of the light in Kelvin */
    void SetTemperature(float temperature);

    /** Get temperature of the light in Kelvin */
    float GetTemperature() const;

    void          SetColor(Float3 const& color);
    Float3 const& GetColor() const;

    /** Set light direction in local space */
    void SetDirection(Float3 const& direction);

    /** Get light direction in local space */
    Float3 GetDirection() const;

    /** Set light direction in world space */
    void SetWorldDirection(Float3 const& _Direction);

    /** Get light direction in world space */
    Float3 GetWorldDirection() const;

    /** Allow mesh to cast shadows on the world */
    void SetCastShadow(bool bCastShadow) { m_bCastShadow = bCastShadow; }

    /** Is cast shadows enabled */
    bool IsCastShadow() const { return m_bCastShadow; }

    void SetShadowMaxDistance(float maxDistance) { m_ShadowMaxDistance = maxDistance; }
    float GetShadowMaxDistance() const { return m_ShadowMaxDistance; }

    void SetShadowCascadeResolution(int resolution) { m_ShadowCascadeResolution = Math::ToClosestPowerOfTwo(resolution); }
    int GetShadowCascadeResolution() const { return m_ShadowCascadeResolution; }

    void SetShadowCascadeOffset(float offset) { m_ShadowCascadeOffset = offset; }
    float GetShadowCascadeOffset() const { return m_ShadowCascadeOffset; }

    void SetShadowCascadeSplitLambda(float splitLambda) { m_ShadowCascadeSplitLambda = splitLambda; }
    float GetShadowCascadeSplitLambda() const { return m_ShadowCascadeSplitLambda; }

    void SetMaxShadowCascades(int maxShadowCascades);
    int  GetMaxShadowCascades() const;

    Float4 const& GetEffectiveColor() const;

    void AddShadowmapCascades(class StreamedMemoryGPU* StreamedMemory, struct RenderViewData* View, size_t* ViewProjStreamHandle, int* pFirstCascade, int* pNumCascades);

protected:
    DirectionalLightComponent();

    void OnCreateAvatar() override;
    void InitializeComponent() override;
    void DeinitializeComponent() override;
    void OnTransformDirty() override;
    void DrawDebug(DebugRenderer* InRenderer) override;

private:
    float          m_IlluminanceInLux;
    float          m_Temperature;
    Float3         m_Color;
    mutable Float4 m_EffectiveColor;
    bool           m_bCastShadow;
    float          m_ShadowMaxDistance;
    float          m_ShadowCascadeOffset;
    int            m_MaxShadowCascades;
    int            m_ShadowCascadeResolution;
    float          m_ShadowCascadeSplitLambda;
};
