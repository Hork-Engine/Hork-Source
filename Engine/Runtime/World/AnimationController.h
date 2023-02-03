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

#pragma once

#include <Engine/Runtime/BaseObject.h>

HK_NAMESPACE_BEGIN

class SkeletalAnimation;
class SkinnedComponent;

/**

Animation play mode

*/
enum ANIMATION_PLAY_MODE
{
    ANIMATION_PLAY_WRAP,
    ANIMATION_PLAY_MIRROR,
    ANIMATION_PLAY_CLAMP
};

/**

AnimationController

Animation controller (track, state)

*/
class AnimationController : public BaseObject
{
    HK_CLASS(AnimationController, BaseObject)

    friend class SkinnedComponent;

public:
    /** Set source animation */
    void SetAnimation(SkeletalAnimation* _Animation);

    /** Get source animation */
    SkeletalAnimation* GetAnimation() { return m_Animation; }

    /** Get animation owner */
    SkinnedComponent* GetOwner() { return m_Owner; }

    /** Set position on animation track */
    void SetTime(float _Time);

    /** Step time delta on animation track */
    void AddTimeDelta(float _TimeDelta);

    /** Get time */
    float GetTime() const { return m_TimeLine; }

    /** Set play mode */
    void SetPlayMode(ANIMATION_PLAY_MODE _PlayMode);

    /** Get play mode */
    ANIMATION_PLAY_MODE GetPlayMode() const { return m_PlayMode; }

    /** Set time quantizer */
    void SetQuantizer(float _Quantizer);

    /** Get quantizer */
    float GetQuantizer() const { return m_Quantizer; }

    /** Set weight for animation blending */
    void SetWeight(float _Weight);

    /** Get weight */
    float GetWeight() const { return m_Weight; }

    /** Set controller enabled/disabled */
    void SetEnabled(bool _Enabled);

    /** Is controller enabled */
    bool IsEnabled() const { return m_bEnabled; }

    AnimationController();

private:
    TRef<SkeletalAnimation> m_Animation;
    SkinnedComponent* m_Owner;
    float m_TimeLine;
    float m_Quantizer;
    float m_Weight;
    float m_Blend;
    int m_Frame;
    int m_NextFrame;
    ANIMATION_PLAY_MODE m_PlayMode;
    bool m_bEnabled;
};

HK_NAMESPACE_END
