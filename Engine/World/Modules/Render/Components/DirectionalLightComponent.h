#pragma once

#include <Engine/Renderer/RenderDefs.h>
#include <Engine/World/Component.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

class DirectionalLightComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;
    // Public

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

    void SetIlluminance(float illuminanceInLux)
    {
        m_IlluminanceInLux = illuminanceInLux;
    }

    float GetIlluminance() const
    {
        return m_IlluminanceInLux;
    }

    void SetShadowMaxDistance(float maxDistance)
    {
        m_ShadowMaxDistance = maxDistance;
    }

    float GetShadowMaxDistance() const
    {
        return m_ShadowMaxDistance;
    }

    void SetShadowCascadeResolution(int resolution)
    {
        m_ShadowCascadeResolution = Math::ToClosestPowerOfTwo(resolution);
    }

    int GetShadowCascadeResolution() const
    {
        return m_ShadowCascadeResolution;
    }

    void SetShadowCascadeOffset(float offset)
    {
        m_ShadowCascadeOffset = offset;
    }

    float GetShadowCascadeOffset() const
    {
        return m_ShadowCascadeOffset;
    }

    void SetShadowCascadeSplitLambda(float splitLambda)
    {
        m_ShadowCascadeSplitLambda = splitLambda;
    }

    float GetShadowCascadeSplitLambda() const
    {
        return m_ShadowCascadeSplitLambda;
    }

    void SetMaxShadowCascades(int maxShadowCascades)
    {
        m_MaxShadowCascades = Math::Clamp(maxShadowCascades, 1, MAX_SHADOW_CASCADES);
    }

    int GetMaxShadowCascades() const
    {
        return m_MaxShadowCascades;
    }

    void SetCastShadow(bool castShadow)
    {
        m_CastShadow = castShadow;
    }

    bool IsCastShadow() const
    {
        return m_CastShadow;
    }

    void UpdateEffectiveColor()
    {
        const float EnergyUnitScale = 1.0f / 100.0f / 100.0f;
        float energy = m_IlluminanceInLux * EnergyUnitScale; // * GetAnimationBrightness();

        Color4 temperatureColor;
        temperatureColor.SetTemperature(m_Temperature);

        m_EffectiveColor[0] = m_Color[0] * temperatureColor[0] * energy;
        m_EffectiveColor[1] = m_Color[1] * temperatureColor[1] * energy;
        m_EffectiveColor[2] = m_Color[2] * temperatureColor[2] * energy;
    }

    void DrawDebug(DebugRenderer& renderer);

    // Private

    Float3         m_Color = Float3(1.0f);
    float          m_Temperature = 6590.0f;
    float          m_IlluminanceInLux = 110000.0f;
    Float4         m_EffectiveColor;
    bool           m_CastShadow = true;

    // TODO: move to CascadeShadowComponent?
    float          m_ShadowMaxDistance = 128;
    float          m_ShadowCascadeOffset = 3;
    int            m_MaxShadowCascades = 4;
    int            m_ShadowCascadeResolution = 1024;
    float          m_ShadowCascadeSplitLambda = 0.5f;    
};

HK_NAMESPACE_END
