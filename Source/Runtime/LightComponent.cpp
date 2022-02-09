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

#include "LightComponent.h"

HK_BEGIN_CLASS_META(ALightComponent)
HK_PROPERTY(bEnabled, SetEnabled, IsEnabled, HK_PROPERTY_DEFAULT)
HK_PROPERTY(AnimTime, SetAnimationTime, GetAnimationTime, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

ALightComponent::ALightComponent()
{
    bCanEverTick = true;
}

void ALightComponent::SetEnabled(bool _Enabled)
{
    bEnabled = _Enabled;
}

void ALightComponent::SetAnimation(const char* _Pattern, float _Speed, float _Quantizer)
{
    AAnimationPattern* anim = CreateInstanceOf<AAnimationPattern>();
    anim->Pattern           = _Pattern;
    anim->Speed             = _Speed;
    anim->Quantizer         = _Quantizer;
    SetAnimation(anim);
}

void ALightComponent::SetAnimation(AAnimationPattern* _Animation)
{
    if (IsSame(Animation, _Animation))
    {
        return;
    }

    Animation           = _Animation;
    AnimationBrightness = 1;

    if (Animation)
    {
        AnimationBrightness = Animation->Calculate(AnimTime);
    }

    bEffectiveColorDirty = true;
}

void ALightComponent::SetAnimationTime(float _Time)
{
    AnimTime = _Time;

    if (Animation)
    {
        AnimationBrightness = Animation->Calculate(AnimTime);
    }
}

void ALightComponent::TickComponent(float _TimeStep)
{
    if (!bEnabled || !Animation)
    {
        return;
    }

    // FIXME: Update light animation only if light is visible?

    AnimationBrightness = Animation->Calculate(AnimTime);
    AnimTime += _TimeStep;
    bEffectiveColorDirty = true;
}
