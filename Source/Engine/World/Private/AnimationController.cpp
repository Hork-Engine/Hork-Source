/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <World/Public/AnimationController.h>
#include <World/Public/Components/SkinnedComponent.h>

#include <World/Public/Resource/Animation.h>

AN_CLASS_META( AAnimationController )

AAnimationController::AAnimationController() {
    Owner = nullptr;
    TimeLine = 0.0f;
    Quantizer = 0.0f;
    Weight = 1.0f;
    Blend = 0.0f;
    Frame = 0;
    NextFrame = 0;
    PlayMode = ANIMATION_PLAY_CLAMP;
    bEnabled = true;
}

void AAnimationController::SetAnimation( ASkeletalAnimation * _Animation ) {
    Animation = _Animation;

    if ( Owner ) {
        Owner->bUpdateRelativeTransforms = true;
        Owner->bUpdateBounds = true;
    }
}

void AAnimationController::SetTime( float _Time ) {
    TimeLine = _Time;

    if ( Owner ) {
        Owner->bUpdateControllers = true;
    }
}

void AAnimationController::AddTimeDelta( float _TimeDelta ) {
    TimeLine += _TimeDelta;

    if ( Owner ) {
        Owner->bUpdateControllers = true;
    }
}

void AAnimationController::SetPlayMode( EAnimationPlayMode _PlayMode ) {
    PlayMode = _PlayMode;

    if ( Owner ) {
        Owner->bUpdateControllers = true;
    }
}

void AAnimationController::SetQuantizer( float _Quantizer ) {
    Quantizer = Math::Min( _Quantizer, 1.0f );

    if ( Owner ) {
        Owner->bUpdateControllers = true;
    }
}

void AAnimationController::SetWeight( float _Weight ) {
    Weight = _Weight;

    if ( Owner ) {
        Owner->bUpdateRelativeTransforms = true;
    }
}

void AAnimationController::SetEnabled( bool _Enabled ) {
    bEnabled = _Enabled;

    if ( Owner ) {
        Owner->bUpdateRelativeTransforms = true;
        Owner->bUpdateBounds = true;
    }
}
