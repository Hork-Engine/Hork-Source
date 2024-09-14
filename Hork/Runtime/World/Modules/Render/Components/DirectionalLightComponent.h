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

#include <Hork/Renderer/RenderDefs.h>
#include <Hork/Runtime/World/Component.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

class DirectionalLightComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    // Public

    void                        SetTemperature(float temperature) { m_Temperature = temperature; }

    float                       GetTemperature() const { return m_Temperature; }

    void                        SetColor(Float3 const& color) { m_Color = color; }

    Float3 const&               GetColor() const { return m_Color; }

    void                        SetIlluminance(float illuminanceInLux) { m_IlluminanceInLux = illuminanceInLux; }

    float                       GetIlluminance() const { return m_IlluminanceInLux; }

    void                        SetShadowMaxDistance(float maxDistance) { m_ShadowMaxDistance = maxDistance; }

    float                       GetShadowMaxDistance() const { return m_ShadowMaxDistance; }

    void                        SetShadowCascadeResolution(int resolution) { m_ShadowCascadeResolution = Math::ToClosestPowerOfTwo(resolution); }

    int                         GetShadowCascadeResolution() const { return m_ShadowCascadeResolution; }

    void                        SetShadowCascadeOffset(float offset) { m_ShadowCascadeOffset = offset; }

    float                       GetShadowCascadeOffset() const { return m_ShadowCascadeOffset; }

    void                        SetShadowCascadeSplitLambda(float splitLambda) { m_ShadowCascadeSplitLambda = splitLambda; }

    float                       GetShadowCascadeSplitLambda() const { return m_ShadowCascadeSplitLambda; }

    void                        SetMaxShadowCascades(int maxShadowCascades) { m_MaxShadowCascades = Math::Clamp(maxShadowCascades, 1, MAX_SHADOW_CASCADES); }

    int                         GetMaxShadowCascades() const { return m_MaxShadowCascades; }

    void                        SetCastShadow(bool castShadow) { m_CastShadow = castShadow; }

    bool                        IsCastShadow() const { return m_CastShadow; }

    Float4 const&               GetEffectiveColor() const { return m_EffectiveColor; }

    void                        UpdateEffectiveColor();

    void                        DrawDebug(DebugRenderer& renderer);

private:
    Float3                      m_Color = Float3(1.0f);
    float                       m_Temperature = 6590.0f;
    float                       m_IlluminanceInLux = 110000.0f;
    Float4                      m_EffectiveColor;
    bool                        m_CastShadow = true;
    float                       m_ShadowMaxDistance = 128;
    float                       m_ShadowCascadeOffset = 3;
    int                         m_MaxShadowCascades = 4;
    int                         m_ShadowCascadeResolution = 1024;
    float                       m_ShadowCascadeSplitLambda = 0.5f;    
};

HK_NAMESPACE_END
