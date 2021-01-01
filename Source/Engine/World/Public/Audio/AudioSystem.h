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

#pragma once

#include "AudioDecoderInterface.h"

class AAudioClip;
class AActor;
class APawn;
class ASceneComponent;
class APlayerController;

class AAudioControlCallback : public ABaseObject
{
    AN_CLASS( AAudioControlCallback, ABaseObject )

public:
    float VolumeScale = 1.0f;

    // TODO: ...

protected:
    AAudioControlCallback() {}
};

class AAudioGroup : public ABaseObject
{
    AN_CLASS( AAudioGroup, ABaseObject )

public:
    float Volume = 1;

    //SetPaused, etc

protected:
    AAudioGroup() {}
};

enum EAudioSourceType
{
    AUDIO_SOURCE_STATIC,
    AUDIO_SOURCE_BACKGROUND,
    AUDIO_SOURCE_FOLLOW_INSIGATOR
};

enum EAudioChannelPriority
{
    AUDIO_CHANNEL_PRIORITY_ONESHOT = 0,
    AUDIO_CHANNEL_PRIORITY_AMBIENT = 1,
    AUDIO_CHANNEL_PRIORITY_MUSIC   = 2,
    AUDIO_CHANNEL_PRIORITY_DIALOGUE= 3

    //AUDIO_CHANNEL_PRIORITY_MAX = 16
};

enum EAudioDistanceModel
{
    AUDIO_DIST_INVERSE           = 0,
    AUDIO_DIST_INVERSE_CLAMPED   = 1, // default
    AUDIO_DIST_LINEAR            = 2,
    AUDIO_DIST_LINEAR_CLAMPED    = 3,
    AUDIO_DIST_EXPONENT          = 4,
    AUDIO_DIST_EXPONENT_CLAMPED  = 5
};

constexpr float AUDIO_MIN_REF_DISTANCE      = 0.1f;
constexpr float AUDIO_DEFAULT_REF_DISTANCE  = 1.0f;
constexpr float AUDIO_DEFAULT_MAX_DISTANCE  = 100.0f;
constexpr float AUDIO_DEFAULT_ROLLOFF_RATE  = 1.0f;
constexpr float AUDIO_MAX_DISTANCE          = 1000.0f;

struct SSoundAttenuationParameters
{
    /** Distance attenuation parameter
    Can be from AUDIO_MIN_REF_DISTANCE to AUDIO_MAX_DISTANCE */
    float        ReferenceDistance = AUDIO_DEFAULT_REF_DISTANCE;

    /** Distance attenuation parameter
    Can be from ReferenceDistance to AUDIO_MAX_DISTANCE */
    float        MaxDistance = AUDIO_DEFAULT_MAX_DISTANCE;

    /** Distance attenuation parameter
    Gain rolloff factor */
    float        RolloffRate = AUDIO_DEFAULT_ROLLOFF_RATE;
};

struct SSoundSpawnParameters
{
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

    /** Use velocity from physical body */
    bool        bUsePhysicalVelocity = false;

    /** If audio client is not specified, audio will be hearable for all listeners */
    APawn *     AudioClient = nullptr;

    /** Audio group */
    AAudioGroup * Group = nullptr;

    /** Sound attenuation */
    SSoundAttenuationParameters Attenuation;

    /** Sound volume */
    float       Volume = 1;

    /** Sound pitch */
    float       Pitch = 1;

    /** Play audio with offset (in seconds) */
    float       PlayOffset = 0;

    bool        bLooping = false;

    bool        bStopWhenInstigatorDead = false;

    bool        bDirectional = false;

    /** Directional sound inner cone angle in degrees. [0-360] */
    float       ConeInnerAngle = 360;

    /** Directional sound outer cone angle in degrees. [0-360] */
    float       ConeOuterAngle = 360;

    /** Direction of sound propagation */
    Float3      Direction = Float3::Zero();

    float       LifeSpan = 0;

    AAudioControlCallback * ControlCallback = nullptr; // Reserved for future
};

class AAudioSystem
{
    AN_SINGLETON( AAudioSystem )

public:
    /** Initialize audio system */
    void Initialize();

    /** Deinitialize audio system */
    void Deinitialize();

    /** Update audio system */
    void Update( APlayerController * _Controller, float _TimeStep );

    /** Set up head related transfer function */
    void EnableHRTF( int _Index );

    /** Set default head related transfer function */
    void EnableDefaultHRTF();

    /** Disable head related transfer function */
    void DisableHRTF();

    /** Get head related transfer function count */
    int GetNumHRTFs() const;

    /** Get head related transfer function name */
    const char * GetHRTF( int _Index ) const;

    /** Purge audio channels */
    void PurgeChannels();

    /** Add audio format and decoder */
    void AddAudioDecoder( const char * _Extension, IAudioDecoderInterface * _Interface );

    /** Remove audio format and decoder */
    void RemoveAudioDecoder( const char * _Extension );

    /** Remove all audio decoders */
    void RemoveAudioDecoders();

    /** Find audio decoder by file name extension */
    IAudioDecoderInterface * FindAudioDecoder( const char * _FileName );

    /** Play audio clip */
    void PlaySound( AAudioClip * _AudioClip, AActor * _Instigator, SSoundSpawnParameters const * _SpawnParameters = nullptr );

    /** Play audio clip at spawn position */
    void PlaySoundAt( AAudioClip * _AudioClip, Float3 const & _SpawnPosition, AActor * _Instigator, SSoundSpawnParameters const * _SpawnParameters = nullptr );

    /** Play audio clip */
    void PlaySound( AAudioClip * _AudioClip, ASceneComponent * _Instigator, SSoundSpawnParameters const * _SpawnParameters = nullptr );

    /** Play audio clip at spawn position */
    void PlaySoundAt( AAudioClip * _AudioClip, Float3 const & _SpawnPosition, ASceneComponent * _Instigator, SSoundSpawnParameters const * _SpawnParameters = nullptr );

    /** Get current active channels */
    int GetNumActiveChannels() const;

    /** Get current audio listener position */
    Float3 const & GetListenerPosition() const;

private:
    ~AAudioSystem();

    struct SAudioDecoderDef
    {
        char Extension[16];
        IAudioDecoderInterface * Interface;
    };

    TPodArray< SAudioDecoderDef > Decoders;
    bool bInitialized = false;
};

extern AAudioSystem & GAudioSystem;
