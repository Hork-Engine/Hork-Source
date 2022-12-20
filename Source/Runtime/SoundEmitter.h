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

#include "SoundResource.h"
#include "World.h"

#include <Audio/AudioChannel.h>
#include <Containers/PodQueue.h>

class SoundGroup : public BaseObject
{
    HK_CLASS(SoundGroup, BaseObject)

public:
    /** Scale volume for all sounds in group */
    void SetVolume(float _Volume)
    {
        Volume = Math::Saturate(_Volume);
    }

    /** Scale volume for all sounds in group */
    float GetVolume() const
    {
        return Volume;
    }

    /** Pause/unpause all sounds in group */
    void SetPaused(bool _bGroupIsPaused)
    {
        bGroupIsPaused = _bGroupIsPaused;
    }

    /** Is group paused */
    bool IsPaused() const
    {
        return bGroupIsPaused;
    }

    /** Play sounds even when game is paused */
    void SetPlayEvenWhenPaused(bool _bPlayEvenWhenPaused)
    {
        bPlayEvenWhenPaused = _bPlayEvenWhenPaused;
    }

    /** Play sounds even when game is paused */
    bool ShouldPlayEvenWhenPaused() const
    {
        return bPlayEvenWhenPaused;
    }

    SoundGroup() = default;

protected:
    /** Scale volume for all sounds in group */
    float Volume = 1;

    /** Pause all sounds in group */
    bool bGroupIsPaused = false;

    /** Play sounds even when game is paused */
    bool bPlayEvenWhenPaused = false;
};

enum SOUND_EMITTER_TYPE
{
    /** Spatial sound emitter */
    SOUND_EMITTER_POINT,

    /** Spatial sound emitter with direction */
    SOUND_EMITTER_DIRECTIONAL,

    /** Background sound (usually music or speach) */
    SOUND_EMITTER_BACKGROUND
};

/** Audio distance attenuation model. Not used now, reserved for future. */
enum AUDIO_DISTANCE_MODEL
{
    AUDIO_DIST_INVERSE          = 0,
    AUDIO_DIST_INVERSE_CLAMPED  = 1, // default
    AUDIO_DIST_LINEAR           = 2,
    AUDIO_DIST_LINEAR_CLAMPED   = 3,
    AUDIO_DIST_EXPONENT         = 4,
    AUDIO_DIST_EXPONENT_CLAMPED = 5
};

/** Priority to play the sound.
NOTE: Not used now. Reserved for future to pick a free channel. */
enum AUDIO_CHANNEL_PRIORITY : uint8_t
{
    AUDIO_CHANNEL_PRIORITY_ONESHOT  = 0,
    AUDIO_CHANNEL_PRIORITY_AMBIENT  = 1,
    AUDIO_CHANNEL_PRIORITY_MUSIC    = 2,
    AUDIO_CHANNEL_PRIORITY_DIALOGUE = 3,

    AUDIO_CHANNEL_PRIORITY_MAX = 255
};

constexpr float SOUND_DISTANCE_MIN         = 0.1f;
constexpr float SOUND_DISTANCE_MAX         = 1000.0f;
constexpr float SOUND_DISTANCE_DEFAULT     = 100.0f;
constexpr float SOUND_REF_DISTANCE_DEFAULT = 1.0f;
constexpr float SOUND_ROLLOFF_RATE_DEFAULT = 1.0f;

struct SoundAttenuationParameters
{
    /** Distance attenuation parameter
    Can be from SOUND_DISTANCE_MIN to SOUND_DISTANCE_MAX */
    float ReferenceDistance = SOUND_REF_DISTANCE_DEFAULT;

    /** Distance attenuation parameter
    Can be from ReferenceDistance to SOUND_DISTANCE_MAX */
    float Distance = SOUND_DISTANCE_DEFAULT;

    /** Distance attenuation parameter
    Gain rolloff factor */
    float RolloffRate = SOUND_ROLLOFF_RATE_DEFAULT;
};

struct SoundSpawnInfo
{
    /** Audio source type */
    SOUND_EMITTER_TYPE EmitterType = SOUND_EMITTER_POINT;

    /** Priority to play the sound.
    NOTE: Not used now. Reserved for future to pick a free channel. */
    int Priority = AUDIO_CHANNEL_PRIORITY_ONESHOT;

    /** Virtualize sound when silent */
    bool bVirtualizeWhenSilent = false;

    /** Dynamic sources that follows the instigator (e.g. projectiles) */
    bool bFollowInstigator = false;

    /** If audio client is not specified, audio will be hearable for all listeners */
    AActor* AudioClient = nullptr;

    /** With listener mask you can filter listeners for the sound */
    uint32_t ListenerMask = ~0u;

    /** Sound group */
    SoundGroup* Group = nullptr;

    /** Sound attenuation */
    SoundAttenuationParameters Attenuation;

    /** Sound volume */
    float Volume = 1;

    /** Play audio with offset (in seconds) */
    int StartFrame = 0;

    /** Stop playing if instigator dead */
    bool bStopWhenInstigatorDead = false;

    /** Directional sound inner cone angle in degrees. [0-360] */
    float ConeInnerAngle = 360;

    /** Directional sound outer cone angle in degrees. [0-360] */
    float ConeOuterAngle = 360;

    /** Direction of sound propagation */
    Float3 Direction;
};

class SoundOneShot
{
    HK_FORBID_COPY(SoundOneShot)

public:
    int                   Priority; // AUDIO_CHANNEL_PRIORITY
    SOUND_EMITTER_TYPE     EmitterType;
    uint64_t              AudioClient;
    uint32_t              ListenerMask;
    TRef<World>          World;
    TRef<SoundGroup>     Group;
    TRef<SceneComponent> Instigator;
    TRef<SoundResource>  Resource;
    uint64_t              InstigatorId;
    int                   ResourceRevision;
    Float3                SoundPosition;
    Float3                SoundDirection;
    float                 Volume;
    int                   ChanVolume[2];
    Float3                LocalDir;
    float                 ReferenceDistance;
    float                 MaxDistance;
    float                 RolloffRate;
    float                 ConeInnerAngle;
    float                 ConeOuterAngle;
    int                   UpdateFrame;
    bool                  bStopWhenInstigatorDead : 1;
    bool                  bVirtualizeWhenSilent : 1;
    bool                  bFollowInstigator : 1;
    bool                  bSpatializedStereo : 1;

    AudioChannel* Channel;

    SoundOneShot* Next;
    SoundOneShot* Prev;

    SoundOneShot()
    {
        Priority         = 0;
        EmitterType      = SOUND_EMITTER_POINT;
        AudioClient      = 0;
        ListenerMask     = 0;
        InstigatorId     = 0;
        ResourceRevision = 0;
        SoundPosition.Clear();
        SoundDirection.Clear();
        Volume        = 0;
        ChanVolume[0] = 0;
        ChanVolume[1] = 0;
        LocalDir.Clear();
        ReferenceDistance       = 0;
        MaxDistance             = 0;
        RolloffRate             = 0;
        ConeInnerAngle          = 0;
        ConeOuterAngle          = 0;
        UpdateFrame             = 0;
        bStopWhenInstigatorDead = false;
        bVirtualizeWhenSilent   = false;
        bFollowInstigator       = false;
        bSpatializedStereo      = false;
        Next                    = nullptr;
        Prev                    = nullptr;
        Channel                 = nullptr;
    }

    virtual ~SoundOneShot() {}

    void Spatialize();

    bool IsPaused() const;
};

class SoundEmitter : public SceneComponent
{
    HK_COMPONENT(SoundEmitter, SceneComponent)

public:
    /** Start playing sound. This function cancels any sound that is already being played by the emitter. */
    void PlaySound(SoundResource* SoundResource, int StartFrame = 0, int LoopStart = -1);

    /** Play one shot. Does not cancel sounds that are already being played by PlayOneShot and PlaySound.
    This function creates a separate channel for sound playback. */
    void PlayOneShot(SoundResource* SoundResource, float VolumeScale, bool bFixedPosition, int StartFrame = 0);

    /** Plays a sound at a given position in world space. */
    static void PlaySoundAt(World* World, SoundResource* SoundResource, SoundGroup* SoundGroup, Float3 const& Position, float Volume = 1.0f, int StartFrame = 0);

    /** Plays a sound at background. */
    static void PlaySoundBackground(World* World, SoundResource* SoundResource, SoundGroup* SoundGroup = nullptr, float Volume = 1.0f, int StartFrame = 0);

    /** Play a custom sound. Use it if you want full control over one shot sounds. */
    static void SpawnSound(SoundResource* SoundResource, Float3 const& SpawnPosition, World* World, SceneComponent* Instigator, SoundSpawnInfo const* SpawnInfo);

    /** Clears all one shot sounds. */
    static void ClearOneShotSounds();

    /** Stops playing any sound from this emitter. */
    void ClearSound();

    /** Add sound to queue */
    void AddToQueue(SoundResource* SoundResource);

    /** Clear sound queue */
    void ClearQueue();

    /** We can control the volume by groups of sound emitters */
    void SetSoundGroup(SoundGroup* SoundGroup);

    /** We can control the volume by groups of sound emitters */
    SoundGroup* GetSoundGroup() const { return Group; }

    /** If audio client is not specified, audio will be hearable for all listeners */
    void SetAudioClient(AActor* AudioClient);

    /** If audio client is not specified, audio will be hearable for all listeners */
    AActor* GetAudioClient() const { return Client; }

    /** With listener mask you can filter listeners for the sound */
    void SetListenerMask(uint32_t Mask);

    /** With listener mask you can filter listeners for the sound */
    uint32_t GetListenerMask() const { return ListenerMask; }

    /** Set emitter type. See SOUND_EMITTER_TYPE */
    void SetEmitterType(SOUND_EMITTER_TYPE EmitterType);

    /** Get emitter type. See SOUND_EMITTER_TYPE */
    SOUND_EMITTER_TYPE GetEmitterType() const { return EmitterType; }

    /** Virtualize sound when silent. Looped sounds has this by default. */
    void SetVirtualizeWhenSilent(bool bVirtualizeWhenSilent);

    /** Virtualize sound when silent. Looped sounds has this by default. */
    bool ShouldVirtualizeWhenSilent() const { return bVirtualizeWhenSilent; }

    /** Audio volume scale */
    void SetVolume(float Volume);

    /** Audio volume scale */
    float GetVolume() const { return Volume; }

    /** Distance attenuation parameter
    Can be from SOUND_DISTANCE_MIN to SOUND_DISTANCE_MAX */
    void SetReferenceDistance(float Dist);

    /** Distance attenuation parameter
    Can be from SOUND_DISTANCE_MIN to SOUND_DISTANCE_MAX */
    float GetReferenceDistance() const { return ReferenceDistance; }

    /** Distance attenuation parameter
    Can be from ReferenceDistance to SOUND_DISTANCE_MAX */
    void SetMaxDistance(float Dist);

    /** Distance attenuation parameter
    Can be from ReferenceDistance to SOUND_DISTANCE_MAX */
    float GetMaxDistance() const { return MaxDistance; }

    /** Distance attenuation parameter
    Gain rolloff factor */
    void SetRolloffRate(float Rolloff);

    /** Distance attenuation parameter
    Gain rolloff factor */
    float GetRollofRate() const { return RolloffRate; }

    /** Directional sound inner cone angle in degrees. [0-360] */
    void SetConeInnerAngle(float Angle);

    /** Directional sound inner cone angle in degrees. [0-360] */
    float GetConeInnerAngle() const { return ConeInnerAngle; }

    /** Directional sound outer cone angle in degrees. [0-360] */
    void SetConeOuterAngle(float Angle);

    /** Directional sound outer cone angle in degrees. [0-360] */
    float GetConeOuterAngle() const { return ConeOuterAngle; }

    /** Pause/Unpause the emitter */
    void SetPaused(bool bPaused);

    /** Set playback position in frames */
    void SetPlaybackPosition(int FrameNum);

    /** Get playback position in frames */
    int GetPlaybackPosition() const;

    /** Set playback position in seconds */
    void SetPlaybackTime(float Time);

    /** Get playback position in seconds */
    float GetPlaybackTime() const;

    /** Set audio volume to 0 */
    void SetMuted(bool bMuted);

    /** Restore audio volume */
    bool IsMuted() const;

    /** Is sound is virtualized (culled) */
    //bool IsVirtual() const;

    /** Return true if no sound plays */
    bool IsSilent() const;

    bool IsPaused() const;

    /** Next sound emitter from global list */
    SoundEmitter* GetNext() { return Next; }

    /** Prev sound emitter from global list */
    SoundEmitter* GetPrev() { return Prev; }

    /** Global sound emitters list */
    static SoundEmitter* GetSoundEmitters() { return SoundEmitters; }

    /** Global list of one shot sounds */
    static SoundOneShot* GetOneShots() { return OneShots; }

    /** Internal. Called by Audio System to update the sounds. */
    static void UpdateSounds();

    SoundEmitter();

protected:
    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnTransformDirty() override;

    void BeginPlay() override;

private:
    void Update();

    /** Select next sound from queue */
    bool SelectNextSound();

    bool StartPlay(SoundResource* SoundResource, int StartFrame, int LoopStart);

    bool RestartSound();

    void Spatialize();

    static void UpdateSound(SoundOneShot* Sound);
    static void FreeSound(SoundOneShot* Sound);

    using Queue = TPodQueue<SoundResource*, 1, true>;

    Queue AudioQueue;

    TRef<SoundGroup>    Group;
    TRef<AActor>        Client;
    uint32_t            ListenerMask;
    SOUND_EMITTER_TYPE  EmitterType;
    TRef<SoundResource> Resource;
    int                 ResourceRevision;
    AudioChannel*       Channel;
    float               Volume;
    float               ReferenceDistance;
    float               MaxDistance;
    float               RolloffRate;
    float               ConeInnerAngle;
    float               ConeOuterAngle;
    int                 ChanVolume[2];
    Float3              LocalDir;
    bool                bSpatializedStereo;
    bool                bEmitterPaused : 1;
    bool                bVirtualizeWhenSilent : 1;
    bool                bMuted : 1;

    SoundEmitter* Next = nullptr;
    SoundEmitter* Prev = nullptr;

    static SoundEmitter* SoundEmitters;
    static SoundEmitter* SoundEmittersTail;

    static SoundOneShot* OneShots;
    static SoundOneShot* OneShotsTail;
};
