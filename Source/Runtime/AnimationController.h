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

#include "BaseObject.h"

class ASkeletalAnimation;
class ASkinnedComponent;

/**

EAnimationPlayMode

Animation play mode

*/
enum EAnimationPlayMode {
    ANIMATION_PLAY_WRAP,
    ANIMATION_PLAY_MIRROR,
    ANIMATION_PLAY_CLAMP
};

/**

AAnimationController

Animation controller (track, state)

*/
class AAnimationController : public ABaseObject {
    AN_CLASS( AAnimationController, ABaseObject )

    friend class ASkinnedComponent;

public:
    /** Set source animation */
    void SetAnimation( ASkeletalAnimation * _Animation );

    /** Get source animation */
    ASkeletalAnimation * GetAnimation() { return Animation; }

    /** Get animation owner */
    ASkinnedComponent * GetOwner() { return Owner; }

    /** Set position on animation track */
    void SetTime( float _Time );

    /** Step time delta on animation track */
    void AddTimeDelta( float _TimeDelta );

    /** Get time */
    float GetTime() const { return TimeLine; }

    /** Set play mode */
    void SetPlayMode( EAnimationPlayMode _PlayMode );

    /** Get play mode */
    EAnimationPlayMode GetPlayMode() const { return PlayMode; }

    /** Set time quantizer */
    void SetQuantizer( float _Quantizer );

    /** Get quantizer */
    float GetQuantizer() const { return Quantizer; }

    /** Set weight for animation blending */
    void SetWeight( float _Weight );

    /** Get weight */
    float GetWeight() const { return Weight; }

    /** Set controller enabled/disabled */
    void SetEnabled( bool _Enabled );

    /** Is controller enabled */
    bool IsEnabled() const { return bEnabled; }

    AAnimationController();

private:
    TRef< ASkeletalAnimation > Animation;
    ASkinnedComponent * Owner;
    float TimeLine;
    float Quantizer;
    float Weight;
    float Blend;
    int Frame;
    int NextFrame;
    EAnimationPlayMode PlayMode;
    bool bEnabled;
};
