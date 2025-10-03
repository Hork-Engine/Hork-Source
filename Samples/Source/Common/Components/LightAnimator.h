/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>

HK_NAMESPACE_BEGIN

HK_FORCEINLINE float Quantize(float frac, float quantizer)
{
    return quantizer > 0.0f ? Math::Floor(frac * quantizer) / quantizer : frac;
}

class LightAnimator : public Component
{
    Handle32<PunctualLightComponent> m_Light;

public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    // Quake styled light anims
    enum AnimationType
    {
        Flicker1,
        SlowStrongPulse,
        Candle,
        FastStrobe,
        GentlePulse,
        Flicker2,
        Candle2,
        Candle3,
        SlowStrobe,
        FluorescentFlicker,
        SlowPulse,
        CustomSequence
    };

    AnimationType Type = Flicker1;
    String Sequence;
    float TimeOffset = 0;

    void BeginPlay()
    {
        m_Light = GetOwner()->GetComponentHandle<PunctualLightComponent>();
    }

    void Update()
    {
        if (auto lightComponent = GetWorld()->GetComponent(m_Light))
        {
            StringView sequence;
            switch (Type)
            {
            case Flicker1:
                sequence = "mmnmmommommnonmmonqnmmo";
                break;
            case SlowStrongPulse:
                sequence = "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba";
                break;
            case Candle:
                sequence = "mmmmmaaaaammmmmaaaaaabcdefgabcdefg";
                break;
            case FastStrobe:
                sequence = "mamamamamama";
                break;
            case GentlePulse:
                sequence = "jklmnopqrstuvwxyzyxwvutsrqponmlkj";
                break;
            case Flicker2:
                sequence = "nmonqnmomnmomomno";
                break;
            case Candle2:
                sequence = "mmmaaaabcdefgmmmmaaaammmaamm";
                break;
            case Candle3:
                sequence = "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa";
                break;
            case SlowStrobe:
                sequence = "aaaaaaaazzzzzzzz";
                break;
            case FluorescentFlicker:
                sequence = "mmamammmmammamamaaamammma";
                break;
            case SlowPulse:
                sequence = "abcdefghijklmnopqrrqponmlkjihgfedcba";
                break;
            case CustomSequence:
                sequence = Sequence;
                break;
            default:
                sequence = "m";
                break;
            }

            float speed = 10;
            float brightness = GetBrightness(sequence, TimeOffset + GetWorld()->GetTick().FrameTime * speed, 0);
            lightComponent->SetColor(Color3(brightness));
        }        
    }

private:
    // Converts string to brightness: 'a' = no light, 'z' = double bright
    float GetBrightness(StringView sequence, float position, float quantizer = 0)
    {
        int frameCount = sequence.Size();
        if (frameCount > 0)
        {
            int keyframe = Math::Floor(position);
            int nextframe = keyframe + 1;

            float lerp = position - keyframe;

            keyframe %= frameCount;
            nextframe %= frameCount;

            float a = (Math::Clamp(sequence[keyframe], 'a', 'z') - 'a') / 26.0f;
            float b = (Math::Clamp(sequence[nextframe], 'a', 'z') - 'a') / 26.0f;

            return Math::Lerp(a, b, Quantize(lerp, quantizer)) * 2;
        }

        return 1.0f;
    }
};

HK_NAMESPACE_END
