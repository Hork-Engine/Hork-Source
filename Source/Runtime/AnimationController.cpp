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

#include "AnimationController.h"
#include "SkinnedComponent.h"
#include "Animation.h"

HK_CLASS_META(AnimationController)

AnimationController::AnimationController()
{
    Owner     = nullptr;
    TimeLine  = 0.0f;
    Quantizer = 0.0f;
    Weight    = 1.0f;
    Blend     = 0.0f;
    Frame     = 0;
    NextFrame = 0;
    PlayMode  = ANIMATION_PLAY_CLAMP;
    bEnabled  = true;
}

void AnimationController::SetAnimation(SkeletalAnimation* _Animation)
{
    Animation = _Animation;

    if (Owner)
    {
        Owner->m_bUpdateRelativeTransforms = true;
        Owner->m_bUpdateBounds = true;
    }
}

void AnimationController::SetTime(float _Time)
{
    TimeLine = _Time;

    if (Owner)
    {
        Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::AddTimeDelta(float _TimeDelta)
{
    TimeLine += _TimeDelta;

    if (Owner)
    {
        Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::SetPlayMode(ANIMATION_PLAY_MODE _PlayMode)
{
    PlayMode = _PlayMode;

    if (Owner)
    {
        Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::SetQuantizer(float _Quantizer)
{
    Quantizer = Math::Min(_Quantizer, 1.0f);

    if (Owner)
    {
        Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::SetWeight(float _Weight)
{
    Weight = _Weight;

    if (Owner)
    {
        Owner->m_bUpdateRelativeTransforms = true;
    }
}

void AnimationController::SetEnabled(bool _Enabled)
{
    bEnabled = _Enabled;

    if (Owner)
    {
        Owner->m_bUpdateRelativeTransforms = true;
        Owner->m_bUpdateBounds = true;
    }
}
