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

#include "SceneComponent.h"
#include "AnimationPattern.h"

HK_NAMESPACE_BEGIN

class LightComponent : public SceneComponent
{
    HK_COMPONENT(LightComponent, SceneComponent)

public:
    virtual void SetEnabled(bool _Enabled);

    bool IsEnabled() const { return m_bEnabled; }

    /** Allow light to cast the shadows. */
    void SetCastShadow(bool bCastShadow) { m_bCastShadow = bCastShadow; }

    /** Is the shadow casting allowed for this light source. */
    bool IsCastShadow() const { return m_bCastShadow; }

    /** Set temperature of the light in Kelvin */
    void SetTemperature(float temperature);

    /** Get temperature of the light in Kelvin */
    float GetTemperature() const;

    void SetColor(Float3 const& color);
    Float3 const& GetColor() const;

    void SetAnimation(const char* _Pattern, float _Speed = 1.0f, float _Quantizer = 0.0f);

    void SetAnimation(AnimationPattern* _Animation);

    AnimationPattern* GetAnimation() { return m_Animation; }

    void SetAnimationTime(float _Time);

    float GetAnimationTime() const { return m_AnimTime; }

protected:
    LightComponent();

    void TickComponent(float _TimeStep) override;

    float GetAnimationBrightness() const { return m_AnimationBrightness; }

    mutable bool           m_bEffectiveColorDirty = true;
    bool                   m_bEnabled = true;
    bool                   m_bCastShadow = false;
    float                  m_Temperature = 6590.0f;
    Float3                 m_Color = Float3(1.0f);
    TRef<AnimationPattern> m_Animation;
    float                  m_AnimTime = 0.0f;
    float                  m_AnimationBrightness = 1.0f;
};

HK_NAMESPACE_END
