/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

class FAudioClip;
class FActor;
class FSceneComponent;
class FPlayerController;

class FAudioControlCallback : public FBaseObject {
    AN_CLASS( FAudioControlCallback, FBaseObject )

public:
    float VolumeScale = 1.0f;

    // TODO: ...

protected:
    FAudioControlCallback() {}
};

class FAudioGroup : public FBaseObject {
    AN_CLASS( FAudioGroup, FBaseObject )

public:
    float Volume = 1;

    //SetPaused, etc

protected:
    FAudioGroup() {}
};

enum EAudioLocation {
    AUDIO_STAY_AT_SPAWN_LOCATION,
    AUDIO_STAY_BACKGROUND,
    AUDIO_FOLLOW_INSIGATOR
};

enum EAudioChannelPriority {
    AUDIO_CHANNEL_PRIORITY_ONESHOT = 0,
    AUDIO_CHANNEL_PRIORITY_AMBIENT = 1,
    AUDIO_CHANNEL_PRIORITY_MUSIC   = 2,
    AUDIO_CHANNEL_PRIORITY_DIALOGUE= 3

    //AUDIO_CHANNEL_PRIORITY_MAX = 16
};

enum EAudioDistanceModel {
    AUDIO_DM_Inverse          = 0,
    AUDIO_DM_InverseClamped   = 1, // default
    AUDIO_DM_Linear           = 2,
    AUDIO_DM_LinearClamped    = 3,
    AUDIO_DM_Exponent         = 4,
    AUDIO_DM_ExponentClamped  = 5
};

#define AUDIO_MIN_REF_DISTANCE      0.1f
#define AUDIO_DEFAULT_REF_DISTANCE  1.0f
#define AUDIO_DEFAULT_MAX_DISTANCE  100.0f
#define AUDIO_DEFAULT_ROLLOFF_RATE  1.0f
#define AUDIO_MAX_DISTANCE          1000.0f

struct FSoundAttenuationParameters {

    // Distance attenuation parameter
    // Can be from AUDIO_MIN_REF_DISTANCE to AUDIO_MAX_DISTANCE
    float        ReferenceDistance = AUDIO_DEFAULT_REF_DISTANCE;

    // Distance attenuation parameter
    // Can be from ReferenceDistance to AUDIO_MAX_DISTANCE
    float        MaxDistance = AUDIO_DEFAULT_MAX_DISTANCE;

    // Distance attenuation parameter
    // Gain rolloff factor
    float        RolloffRate = AUDIO_DEFAULT_ROLLOFF_RATE;
};

struct FSoundSpawnParameters {

    // Sound location behavior
    EAudioLocation Location = AUDIO_STAY_AT_SPAWN_LOCATION;

    // Priority to play the sound
    int         Priority = AUDIO_CHANNEL_PRIORITY_ONESHOT;

    // Play the sound even when game is paused
    bool        bPlayEvenWhenPaused = false;

    // Virtualize sound when silent
    bool        bVirtualizeWhenSilent = false;

    // Calc position based velocity to affect the sound
    bool        bUseVelocity = false;

    // Use velocity from physical body
    bool        bUsePhysicalVelocity = false;

    // Audio group
    FAudioGroup * Group = nullptr;

    // Sound attenuation
    FSoundAttenuationParameters Attenuation;

    // Sound volume
    float       Volume = 1;

    // Sound pitch
    float       Pitch = 1;

    // Play audio with offset (in seconds)
    float       PlayOffset = 0;

    bool        bLooping = false;

    bool        bStopWhenInstigatorDead = false;

    bool        bDirectional = false;

    // Directional sound inner cone angle in degrees. [0-360]
    float       ConeInnerAngle = 360;

    // Directional sound outer cone angle in degrees. [0-360]
    float       ConeOuterAngle = 360;

    // Direction of sound porpagation
    Float3      Direction = Float3::Zero();

    float       LifeSpan = 0;

    FAudioControlCallback * ControlCallback = nullptr; // Reserved for future
};

class ANGIE_API FAudioSystem {
    AN_SINGLETON( FAudioSystem )

public:
    void Initialize();
    void Deinitialize();

    void PurgeChannels();

    void RegisterDecoder( const char * _Extension, IAudioDecoderInterface * _Interface );

    void UnregisterDecoder( const char * _Extension );

    void UnregisterDecoders();

    IAudioDecoderInterface * FindDecoder( const char * _FileName );

    bool DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, /* optional */ short ** _PCM );

    bool ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength );

    void PlaySound( FAudioClip * _AudioClip, FActor * _Instigator, FSoundSpawnParameters const * _SpawnParameters = nullptr );

    void PlaySoundAt( FAudioClip * _AudioClip, Float3 const & _SpawnPosition, FActor * _Instigator, FSoundSpawnParameters const * _SpawnParameters = nullptr );

    void PlaySound( FAudioClip * _AudioClip, FSceneComponent * _Instigator, FSoundSpawnParameters const * _SpawnParameters = nullptr );

    void PlaySoundAt( FAudioClip * _AudioClip, Float3 const & _SpawnPosition, FSceneComponent * _Instigator, FSoundSpawnParameters const * _SpawnParameters = nullptr );

    //FAudioPlayer * CreateAudioPlayer();

    //FMusicChannel * GetMusicChannel( EMusicChannelName _Name );

    void EnableHRTF( int _Index );
    void EnableDefaultHRTF();
    void DisableHRTF();

    int GetNumHRTFs() const;
    const char * GetHRTF( int _Index ) const;

    Float3 const & GetListenerPosition() const;

    void Update( FPlayerController * _Controller, float _TimeStep );

    int GetNumActiveChannels() const;

private:
    ~FAudioSystem();

    bool bInitialized;

    struct Entry {
        char Extension[16];
        IAudioDecoderInterface * Decoder;
    };

    TPodArray< Entry > Decoders;
};

extern FAudioSystem & GAudioSystem;
