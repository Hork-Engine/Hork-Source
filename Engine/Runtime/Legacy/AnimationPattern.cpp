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

#include "AnimationPattern.h"

HK_NAMESPACE_BEGIN

HK_CLASS_META(AnimationPattern)

HK_FORCEINLINE float Quantize(float _Lerp, float _Quantizer)
{
    return _Quantizer > 0.0f ? Math::Floor(_Lerp * _Quantizer) / _Quantizer : _Lerp;
}

float AnimationPattern::Calculate(float _Time)
{
    int numFrames = Pattern.Length();

    if (numFrames > 0)
    {
        float t = _Time * Speed;

        int keyFrame  = Math::Floor(t);
        int nextFrame = keyFrame + 1;

        float lerp = t - keyFrame;

        keyFrame %= numFrames;
        nextFrame %= numFrames;

        float a = (Math::Clamp(Pattern[keyFrame], 'a', 'z') - 'a') / 26.0f;
        float b = (Math::Clamp(Pattern[nextFrame], 'a', 'z') - 'a') / 26.0f;

        return Math::Lerp(a, b, Quantize(lerp, Quantizer));
    }

    return 1.0f;
}

HK_NAMESPACE_END
#endif
