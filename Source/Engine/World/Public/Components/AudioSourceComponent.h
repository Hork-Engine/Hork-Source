/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#pragma once

#include <World/Public/Audio/AudioSystem.h>
#include <World/Public/Audio/AudioClip.h>
#include <World/Public/Actors/Pawn.h>

class AAudioSourceComponent : public ASceneComponent
{
    AN_COMPONENT( AAudioSourceComponent, ASceneComponent )

public:
    /** Audio source type/behavior */
    EAudioSourceType SourceType = AUDIO_SOURCE_STATIC;

    /** Priority to play the sound */
    int         Priority = AUDIO_CHANNEL_PRIORITY_ONESHOT;

    /** Play the sound even when game is paused */
    bool        bPlayEvenWhenPaused = false;

    /** Virtualize sound when silent */
    bool        bVirtualizeWhenSilent = false;

    /** Calc position based velocity to affect the sound */
    bool        bUseVelocity = false;

    /** If audio client is not specified, audio will be hearable for all listeners */
    TRef< APawn > AudioClient;

    /** Audio group */
    TRef< AAudioGroup > AudioGroup;

    /** Distance attenuation parameter
    Can be from AUDIO_MIN_REF_DISTANCE to AUDIO_MAX_DISTANCE */
    float       ReferenceDistance = AUDIO_DEFAULT_REF_DISTANCE;

    /** Distance attenuation parameter
    Can be from ReferenceDistance to AUDIO_MAX_DISTANCE */
    float       MaxDistance = AUDIO_DEFAULT_MAX_DISTANCE;

    /** Distance attenuation parameter
    Gain rolloff factor */
    float       RolloffRate = AUDIO_DEFAULT_ROLLOFF_RATE;

    /** Sound pitch */
    float       Pitch = 1;

    /** Play audio with offset (in seconds) */
    float       PlayOffset = 0;

    bool        bLooping = false;

    bool        bDirectional = false;

    /** Directional sound inner cone angle in degrees. [0-360] */
    float       ConeInnerAngle = 360;

    /** Directional sound outer cone angle in degrees. [0-360] */
    float       ConeOuterAngle = 360;

    /** Direction of sound propagation */
    Float3      Direction = Float3::Zero();

    float       AudioLifeSpan = 0;

    TRef< AAudioClip > AudioClip;

    void SetVolume( float Volume )
    {
        AudioControl->VolumeScale = Volume;
    }

    float GetVolume() const
    {
        return AudioControl->VolumeScale;
    }

protected:
    AAudioSourceComponent();

    void OnCreateAvatar() override;

    void BeginPlay() override;

private:
    TRef< AAudioControlCallback > AudioControl;
};
