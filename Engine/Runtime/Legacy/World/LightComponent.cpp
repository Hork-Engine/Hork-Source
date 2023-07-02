/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#if 0
#include "LightComponent.h"

HK_NAMESPACE_BEGIN

HK_BEGIN_CLASS_META(LightComponent)
HK_PROPERTY(bEnabled, SetEnabled, IsEnabled, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bCastShadow, SetCastShadow, IsCastShadow, HK_PROPERTY_DEFAULT)
HK_PROPERTY(Temperature, SetTemperature, GetTemperature, HK_PROPERTY_DEFAULT)
HK_PROPERTY(Color, SetColor, GetColor, HK_PROPERTY_DEFAULT)
HK_PROPERTY(AnimTime, SetAnimationTime, GetAnimationTime, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

LightComponent::LightComponent()
{
    m_bCanEverTick = true;
}

void LightComponent::SetEnabled(bool _Enabled)
{
    m_bEnabled = _Enabled;
}

void LightComponent::SetTemperature(float _Temperature)
{
    m_Temperature = _Temperature;
    m_bEffectiveColorDirty = true;
}

float LightComponent::GetTemperature() const
{
    return m_Temperature;
}

void LightComponent::SetColor(Float3 const& color)
{
    m_Color = color;
    m_bEffectiveColorDirty = true;
}

Float3 const& LightComponent::GetColor() const
{
    return m_Color;
}

void LightComponent::SetAnimation(const char* _Pattern, float _Speed, float _Quantizer)
{
    AnimationPattern* anim = NewObj<AnimationPattern>();
    anim->Pattern           = _Pattern;
    anim->Speed             = _Speed;
    anim->Quantizer         = _Quantizer;
    SetAnimation(anim);
}

void LightComponent::SetAnimation(AnimationPattern* _Animation)
{
    if (m_Animation == _Animation)
    {
        return;
    }

    m_Animation = _Animation;
    m_AnimationBrightness = 1;

    if (m_Animation)
    {
        m_AnimationBrightness = m_Animation->Calculate(m_AnimTime);
    }

    m_bEffectiveColorDirty = true;
}

void LightComponent::SetAnimationTime(float _Time)
{
    m_AnimTime = _Time;

    if (m_Animation)
    {
        m_AnimationBrightness = m_Animation->Calculate(m_AnimTime);
    }
}

void LightComponent::TickComponent(float _TimeStep)
{
    if (!m_bEnabled || !m_Animation)
    {
        return;
    }

    // FIXME: Update light animation only if light is visible?

    m_AnimationBrightness = m_Animation->Calculate(m_AnimTime);
    m_AnimTime += _TimeStep;
    m_bEffectiveColorDirty = true;
}

HK_NAMESPACE_END
#endif
