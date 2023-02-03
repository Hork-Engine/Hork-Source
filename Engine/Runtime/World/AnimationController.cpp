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

#include "AnimationController.h"
#include "SkinnedComponent.h"

#include <Engine/Runtime/Animation.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(AnimationController)

AnimationController::AnimationController()
{
    m_Owner = nullptr;
    m_TimeLine = 0.0f;
    m_Quantizer = 0.0f;
    m_Weight = 1.0f;
    m_Blend = 0.0f;
    m_Frame = 0;
    m_NextFrame = 0;
    m_PlayMode = ANIMATION_PLAY_CLAMP;
    m_bEnabled = true;
}

void AnimationController::SetAnimation(SkeletalAnimation* _Animation)
{
    m_Animation = _Animation;

    if (m_Owner)
    {
        m_Owner->m_bUpdateRelativeTransforms = true;
        m_Owner->m_bUpdateBounds = true;
    }
}

void AnimationController::SetTime(float _Time)
{
    m_TimeLine = _Time;

    if (m_Owner)
    {
        m_Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::AddTimeDelta(float _TimeDelta)
{
    m_TimeLine += _TimeDelta;

    if (m_Owner)
    {
        m_Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::SetPlayMode(ANIMATION_PLAY_MODE _PlayMode)
{
    m_PlayMode = _PlayMode;

    if (m_Owner)
    {
        m_Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::SetQuantizer(float _Quantizer)
{
    m_Quantizer = Math::Min(_Quantizer, 1.0f);

    if (m_Owner)
    {
        m_Owner->m_bUpdateControllers = true;
    }
}

void AnimationController::SetWeight(float _Weight)
{
    m_Weight = _Weight;

    if (m_Owner)
    {
        m_Owner->m_bUpdateRelativeTransforms = true;
    }
}

void AnimationController::SetEnabled(bool _Enabled)
{
    m_bEnabled = _Enabled;

    if (m_Owner)
    {
        m_Owner->m_bUpdateRelativeTransforms = true;
        m_Owner->m_bUpdateBounds = true;
    }
}

HK_NAMESPACE_END
