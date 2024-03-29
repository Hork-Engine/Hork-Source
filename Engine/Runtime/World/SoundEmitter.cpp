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

#include "SoundEmitter.h"
#include <Engine/Runtime/Engine.h>
#include <Engine/Audio/AudioMixer.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>

HK_NAMESPACE_BEGIN

SoundEmitter* SoundEmitter::m_SoundEmitters;
SoundEmitter* SoundEmitter::m_SoundEmittersTail;

SoundOneShot* SoundEmitter::m_OneShots = nullptr;
SoundOneShot* SoundEmitter::m_OneShotsTail = nullptr;

HK_CLASS_META(SoundGroup)
HK_CLASS_META(SoundEmitter)

SoundEmitter::SoundEmitter()
{
    m_ListenerMask = ~0u;
    m_EmitterType = SOUND_EMITTER_POINT;
    m_Volume = 1.0f;
    m_ChanVolume[0] = 0;
    m_ChanVolume[1] = 0;
    m_ReferenceDistance = SOUND_REF_DISTANCE_DEFAULT;
    m_MaxDistance = SOUND_DISTANCE_DEFAULT;
    m_RolloffRate = SOUND_ROLLOFF_RATE_DEFAULT;
    m_ConeInnerAngle = 360;
    m_ConeOuterAngle = 360;
    m_bEmitterPaused = false;
    m_bVirtualizeWhenSilent = false;
    m_bSpatializedStereo = false;
    m_ResourceRevision = 0;
    m_Channel = nullptr;
}

void SoundEmitter::InitializeComponent()
{
    Super::InitializeComponent();
}

void SoundEmitter::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    INTRUSIVE_REMOVE(this, m_Next, m_Prev, m_SoundEmitters, m_SoundEmittersTail);

    ClearSound();
}

void SoundEmitter::OnTransformDirty()
{
    Super::OnTransformDirty();
}

void SoundEmitter::BeginPlay()
{
    INTRUSIVE_ADD(this, m_Next, m_Prev, m_SoundEmitters, m_SoundEmittersTail);

    Spatialize();

    if (IsSilent())
    {
        SelectNextSound();
    }
}

HK_FORCEINLINE float FalloffDistance(float MaxDistance)
{
    return MaxDistance * 1.3f;
}

void SoundEmitter::PlaySound(SoundResource* SoundResource, int StartFrame, int _LoopStart)
{
    if (!IsInitialized())
    {
        LOG("SoundEmitter::PlaySound: not initialized\n");
        return;
    }

    ClearSound();

    if (m_Client && m_Client->IsPendingKill())
    {
        // Client is dead
        return;
    }

    bool bLooped                     = _LoopStart >= 0;
    bool bShouldVirtualizeWhenSilent = m_bVirtualizeWhenSilent || bLooped;

    if (m_EmitterType != SOUND_EMITTER_BACKGROUND && !bShouldVirtualizeWhenSilent)
    {
        float maxDist = Math::Clamp(m_MaxDistance, m_ReferenceDistance, SOUND_DISTANCE_MAX);
        float falloff  = FalloffDistance(maxDist);
        float cullDist = maxDist + falloff;

        AudioListener const& listener = GEngine->GetAudioSystem()->GetListener();

        if (listener.Position.DistSqr(GetWorldPosition()) >= cullDist * cullDist)
        {
            // Sound is too far from listener
            return;
        }
    }

    Spatialize();

    if (!bShouldVirtualizeWhenSilent && m_ChanVolume[0] == 0 && m_ChanVolume[1] == 0)
    {
        // Don't even start
        return;
    }

    StartPlay(SoundResource, StartFrame, _LoopStart);
}

void SoundEmitter::PlayOneShot(SoundResource* SoundResource, float VolumeScale, bool bFixedPosition, int StartFrame)
{
    if (!IsInitialized())
    {
        LOG("SoundEmitter::PlayOneShot: not initialized\n");
        return;
    }

    SoundSpawnInfo spawnInfo;
    spawnInfo.EmitterType = m_EmitterType;
    spawnInfo.Priority = AUDIO_CHANNEL_PRIORITY_ONESHOT;
    spawnInfo.bVirtualizeWhenSilent = m_bVirtualizeWhenSilent;
    spawnInfo.bFollowInstigator = !bFixedPosition;
    spawnInfo.AudioClient = m_Client;
    spawnInfo.ListenerMask = m_ListenerMask;
    spawnInfo.Group = m_Group;
    spawnInfo.Attenuation.ReferenceDistance = m_ReferenceDistance;
    spawnInfo.Attenuation.Distance = m_MaxDistance;
    spawnInfo.Attenuation.RolloffRate = m_RolloffRate;
    spawnInfo.Volume = m_Volume * VolumeScale;
    spawnInfo.StartFrame = StartFrame;
    spawnInfo.bStopWhenInstigatorDead = false; //true;
    spawnInfo.ConeInnerAngle = m_ConeInnerAngle;
    spawnInfo.ConeOuterAngle = m_ConeOuterAngle;
    spawnInfo.Direction = GetWorldForwardVector();

    SpawnSound(SoundResource, GetWorldPosition(), GetWorld(), this, &spawnInfo);
}

void SoundEmitter::PlaySoundAt(World* World, SoundResource* SoundResource, SoundGroup* SoundGroup, Float3 const& Position, float Volume, int StartFrame)
{
    SoundSpawnInfo spawnInfo;

    spawnInfo.EmitterType = SOUND_EMITTER_POINT;
    spawnInfo.Priority    = AUDIO_CHANNEL_PRIORITY_ONESHOT;
    //    spawnInfo.bVirtualizeWhenSilent = bVirtualizeWhenSilent;
    //    spawnInfo.AudioClient = Client;
    //    spawnInfo.ListenerMask = ListenerMask;
    spawnInfo.Group = SoundGroup;
    //    spawnInfo.Attenuation.ReferenceDistance = ReferenceDistance;
    //    spawnInfo.Attenuation.Distance = MaxDistance;
    //    spawnInfo.Attenuation.RolloffRate = RolloffRate;
    spawnInfo.Volume     = Volume;
    spawnInfo.StartFrame = StartFrame;

    SpawnSound(SoundResource, Position, World, nullptr, &spawnInfo);
}

void SoundEmitter::PlaySoundBackground(World* World, SoundResource* SoundResource, SoundGroup* SoundGroup, float Volume, int StartFrame)
{
    SoundSpawnInfo spawnInfo;

    spawnInfo.EmitterType = SOUND_EMITTER_BACKGROUND;
    spawnInfo.Priority    = AUDIO_CHANNEL_PRIORITY_ONESHOT;
    //    spawnInfo.bVirtualizeWhenSilent = bVirtualizeWhenSilent;
    //    spawnInfo.AudioClient = Client;
    //    spawnInfo.ListenerMask = ListenerMask;
    spawnInfo.Group      = SoundGroup;
    spawnInfo.Volume     = Volume;
    spawnInfo.StartFrame = StartFrame;

    SpawnSound(SoundResource, Float3(0.0f), World, nullptr, &spawnInfo);
}

bool SoundEmitter::StartPlay(SoundResource* SoundResource, int StartFrame, int LoopStart)
{
    if (!SoundResource)
    {
        LOG("SoundEmitter::StartPlay: No sound specified\n");
        return false;
    }

    if (SoundResource->GetFrameCount() == 0)
    {
        LOG("SoundEmitter::StartPlay: Sound has no frames\n");
        return false;
    }

    if (LoopStart >= SoundResource->GetFrameCount())
    {
        LoopStart = 0;
    }

    if (StartFrame < 0)
    {
        StartFrame = 0;
    }

    int loopsCount = 0;

    if (StartFrame >= SoundResource->GetFrameCount())
    {
        if (LoopStart < 0)
        {
            return false;
        }

        StartFrame = LoopStart;
        loopsCount++;
    }

    TRef<AudioStream> streamInterface;

    // Initialize audio stream instance
    if (SoundResource->GetStreamType() != SOUND_STREAM_DISABLED)
    {
        if (!SoundResource->CreateStreamInstance(&streamInterface))
        {
            LOG("SoundEmitter::StartPlay: Couldn't create audio stream instance\n");
            return false;
        }
    }
    else
    {
        if (!SoundResource->GetAudioBuffer())
        {
            LOG("SoundEmitter::StartPlay: Resource has no audio buffer\n");
            return false;
        }
    }

    m_Resource = SoundResource;
    m_ResourceRevision = m_Resource->GetRevision();

    m_Channel = new AudioChannel(StartFrame,
                                 LoopStart,
                                 loopsCount,
                                 SoundResource->GetAudioBuffer(),
                                 streamInterface,
                                 m_bVirtualizeWhenSilent,
                                 m_ChanVolume,
                                 m_LocalDir,
                                 m_bSpatializedStereo,
                                 IsPaused());

    GEngine->GetAudioSystem()->GetMixer()->SubmitChannel(m_Channel);

    return true;
}

bool SoundEmitter::RestartSound()
{
    TRef<SoundResource> newSound = m_Resource;

    int loopStart = m_Channel ? m_Channel->GetLoopStart() : -1;

    if (m_Channel)
    {
        m_Channel->RemoveRef();
        m_Channel = nullptr;
    }

    m_Resource.Reset();

    return StartPlay(newSound.GetObject(), 0, loopStart);
}

void SoundEmitter::ClearSound()
{
    if (m_Channel)
    {
        m_Channel->RemoveRef();
        m_Channel = nullptr;
    }

    m_Resource.Reset();

    ClearQueue();
}

void SoundEmitter::AddToQueue(SoundResource* SoundResource)
{
    if (!SoundResource)
    {
        LOG("SoundEmitter::AddToQueue: No sound specified\n");
        return;
    }

    if (SoundResource->GetFrameCount() == 0)
    {
        LOG("SoundEmitter::AddToQueue: Sound has no frames\n");
        return;
    }

    bool bPlayNow = IsInitialized() && IsSilent();

    if (bPlayNow && AudioQueue.IsEmpty())
    {
        StartPlay(SoundResource, 0, -1);
        return;
    }

    *AudioQueue.Push() = SoundResource;
    SoundResource->AddRef();

    if (bPlayNow)
    {
        SelectNextSound();
    }
}

bool SoundEmitter::SelectNextSound()
{
    bool bSelected = false;

    if (m_Channel)
    {
        m_Channel->RemoveRef();
        m_Channel = nullptr;
    }

    m_Resource.Reset();

    while (!AudioQueue.IsEmpty() && !bSelected)
    {
        SoundResource* playSound = *AudioQueue.Pop();

        bSelected = StartPlay(playSound, 0, -1);

        playSound->RemoveRef();
    }

    return bSelected;
}

void SoundEmitter::ClearQueue()
{
    while (!AudioQueue.IsEmpty())
    {
        SoundResource* sound = *AudioQueue.Pop();
        sound->RemoveRef();
    }
}

bool SoundEmitter::IsPaused() const
{
    bool paused = m_bEmitterPaused;

    bool bPlayEvenWhenPaused = m_Group ? m_Group->ShouldPlayEvenWhenPaused() : false;

    if (!bPlayEvenWhenPaused)
    {
        paused = paused || GetWorld()->IsPaused();
    }

    if (m_Group)
    {
        paused = paused || m_Group->IsPaused();
    }

    return paused;
}

void SoundEmitter::Update()
{
    if (!m_Resource)
    {
        // silent
        return;
    }

    // Check if the audio clip has been modified
    if (m_ResourceRevision != m_Resource->GetRevision())
    {
        if (!RestartSound())
        {
            // couldn't restart
            return;
        }
    }

    // Select next sound from queue if playback position is reached the end
    if (m_Channel->GetPlaybackPos() >= m_Channel->FrameCount)
    {
        if (!SelectNextSound())
        {
            return;
        }
    }

    if (m_Channel->IsStopped())
    {
        ClearSound();
        return;
    }

    bool paused = IsPaused();

    if (!paused)
    {
        Spatialize();
    }

    m_Channel->Commit(m_ChanVolume, m_LocalDir, m_bSpatializedStereo, paused);
}

static void CalcAttenuation(SOUND_EMITTER_TYPE EmitterType,
                            Float3 const&     SoundPosition,
                            Float3 const&     SoundDirection,
                            Float3 const&     ListenerPosition,
                            Float3 const&     ListenerRightVec,
                            float             ReferenceDistance,
                            float             MaxDistance,
                            float             RolloffRate,
                            float             ConeInnerAngle,
                            float             ConeOuterAngle,
                            float*            pLeftVol,
                            float*            pRightVol)
{
    Float3 dir         = SoundPosition - ListenerPosition;
    float  distance    = dir.NormalizeSelf();
    float  attenuation = 1;

    // Cone attenuation
    if (EmitterType == SOUND_EMITTER_DIRECTIONAL && ConeInnerAngle < 360.0f)
    {
        float angle         = -2.0f * Math::Degrees(std::acos(Math::Dot(SoundDirection, dir)));
        float angleInterval = ConeOuterAngle - ConeInnerAngle;

        if (angle > ConeInnerAngle)
        {
            if (angleInterval > 0.0f)
            {
                //attenuation = std::powf( 1.0f - Math::Clamp( angle - ConeInnerAngle, 0.0f, angleInterval ) / angleInterval, RolloffFactor );
                attenuation = 1.0f - (angle - ConeInnerAngle) / angleInterval;
            }
            else
            {
                attenuation = 0;
            }
        }
    }

    // Calc clamped distance
    float d = Math::Clamp(distance, ReferenceDistance, MaxDistance);

    // Linear distance clamped model
    //attenuation *= ( 1.0f - RolloffRate * (d - ReferenceDistance) / (MaxDistance - ReferenceDistance) );

    // Inverse distance clamped model
    attenuation *= ReferenceDistance / (ReferenceDistance + RolloffRate * (d - ReferenceDistance));

    // Falloff
    distance -= MaxDistance;
    if (distance > 0)
    {
        const float falloff = FalloffDistance(MaxDistance);
        if (distance >= falloff)
        {
            attenuation = 0;
        }
        else
        {
            attenuation *= 1.0f - distance / falloff;
        }
    }

    // Panning
    if (Snd_HRTF || GEngine->GetAudioSystem()->GetPlaybackDevice()->IsMono())
    {
        *pLeftVol = *pRightVol = attenuation;
    }
    else
    {
        float panning = Math::Dot(ListenerRightVec, dir);

        float leftPan  = 1.0f - panning;
        float rightPan = 1.0f + panning;

        *pLeftVol  = attenuation * leftPan;
        *pRightVol = attenuation * rightPan;
    }
}

void SoundEmitter::Spatialize()
{
    AudioListener const& listener = GEngine->GetAudioSystem()->GetListener();

    m_ChanVolume[0] = 0;
    m_ChanVolume[1] = 0;

    // Cull if muted
    if (m_bMuted)
    {
        return;
    }

    // Cull by client
    if (m_Client && listener.Id != m_Client->Id)
    {
        return;
    }

    // Cull by mask
    if ((m_ListenerMask & listener.Mask) == 0)
    {
        return;
    }

    float volume = m_Volume;
    volume *= listener.VolumeScale;
    volume *= GetWorld()->GetAudioVolume();
    if (m_Group)
    {
        volume *= m_Group->GetVolume();
    }

    // Cull by volume
    if (volume < 0.0001f)
    {
        return;
    }

    // Don't be too lound
    if (volume > 1.0f)
    {
        volume = 1.0f;
    }

    const float VolumeFtoI = 65535;

    volume *= VolumeFtoI;

    // If the sound is played from the listener, consider it as background
    if (m_EmitterType == SOUND_EMITTER_BACKGROUND || GetOwnerActor()->Id == listener.Id)
    {
        // Use full volume without attenuation
        m_ChanVolume[0] = m_ChanVolume[1] = (int)volume;

        // Don't spatialize stereo sounds
        m_bSpatializedStereo = false;
        return;
    }

    Float3 soundPosition = GetWorldPosition();

    float leftVol, rightVol;

    CalcAttenuation(m_EmitterType, soundPosition, GetWorldForwardVector(), listener.Position, listener.RightVec, m_ReferenceDistance, m_MaxDistance, m_RolloffRate, m_ConeInnerAngle, m_ConeOuterAngle, &leftVol, &rightVol);

    m_ChanVolume[0] = (int)(volume * leftVol);
    m_ChanVolume[1] = (int)(volume * rightVol);

    // Should never happen, but just in case
    if (m_ChanVolume[0] < 0)
        m_ChanVolume[0] = 0;
    if (m_ChanVolume[1] < 0)
        m_ChanVolume[1] = 0;
    if (m_ChanVolume[0] > 65535)
        m_ChanVolume[0] = 65535;
    if (m_ChanVolume[1] > 65535)
        m_ChanVolume[1] = 65535;

    m_bSpatializedStereo = !GEngine->GetAudioSystem()->GetPlaybackDevice()->IsMono();

    if (Snd_HRTF)
    {
        m_LocalDir = listener.TransformInv * soundPosition;
        float d = m_LocalDir.NormalizeSelf();
        if (d < 0.0001f)
        {
            // Sound has same position as listener
            m_LocalDir = Float3(0, 1, 0);
        }
    }
}

void SoundEmitter::SetSoundGroup(SoundGroup* SoundGroup)
{
    m_Group = SoundGroup;
}

void SoundEmitter::SetAudioClient(Actor* AudioClient)
{
    m_Client = AudioClient;
}

void SoundEmitter::SetListenerMask(uint32_t Mask)
{
    m_ListenerMask = Mask;
}

void SoundEmitter::SetEmitterType(SOUND_EMITTER_TYPE _EmitterType)
{
    m_EmitterType = _EmitterType;
}

void SoundEmitter::SetVirtualizeWhenSilent(bool _bVirtualizeWhenSilent)
{
    m_bVirtualizeWhenSilent = _bVirtualizeWhenSilent;
}

void SoundEmitter::SetVolume(float _Volume)
{
    m_Volume = Math::Saturate(_Volume);
}

void SoundEmitter::SetReferenceDistance(float Dist)
{
    m_ReferenceDistance = Math::Clamp(Dist, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX);
}

void SoundEmitter::SetMaxDistance(float Dist)
{
    m_MaxDistance = Math::Clamp(Dist, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX);
}

void SoundEmitter::SetRolloffRate(float Rolloff)
{
    m_RolloffRate = Math::Clamp(Rolloff, 0.0f, 1.0f);
}

void SoundEmitter::SetConeInnerAngle(float Angle)
{
    m_ConeInnerAngle = Math::Clamp(Angle, 0.0f, 360.0f);
}

void SoundEmitter::SetConeOuterAngle(float Angle)
{
    m_ConeOuterAngle = Math::Clamp(Angle, 0.0f, 360.0f);
}

void SoundEmitter::SetPaused(bool _bPaused)
{
    m_bEmitterPaused = _bPaused;
}

void SoundEmitter::SetPlaybackPosition(int FrameNum)
{
    if (!m_Channel)
    {
        return;
    }

    if (m_Channel->GetPlaybackPos() == FrameNum)
    {
        return;
    }

    m_Channel->ChangePlaybackPosition(Math::Clamp(FrameNum, 0, m_Channel->FrameCount));
}

int SoundEmitter::GetPlaybackPosition() const
{
    return m_Channel->GetPlaybackPos();
}

void SoundEmitter::SetPlaybackTime(float Time)
{
    AudioDevice* device = GEngine->GetAudioSystem()->GetPlaybackDevice();

    int frameNum = Math::Round(Time * device->GetSampleRate());

    SetPlaybackPosition(frameNum);
}

float SoundEmitter::GetPlaybackTime() const
{
    AudioDevice* device = GEngine->GetAudioSystem()->GetPlaybackDevice();

    return (float)m_Channel->GetPlaybackPos() / device->GetSampleRate();
}

void SoundEmitter::SetMuted(bool _bMuted)
{
    m_bMuted = _bMuted;
}

bool SoundEmitter::IsMuted() const
{
    return m_bMuted;
}

//bool SoundEmitter::IsVirtual() const
//{
//    return m_Channel->bVirtual; // TODO: Needs synchronization
//}

bool SoundEmitter::IsSilent() const
{
    return !m_Resource.GetObject();
}

void SoundEmitter::SpawnSound(SoundResource* SoundResource, Float3 const& SpawnPosition, World* World, SceneComponent* Instigator, SoundSpawnInfo const* SpawnInfo)
{
    static SoundSpawnInfo DefaultSoundSpawnInfo;

    if (!SpawnInfo)
    {
        SpawnInfo = &DefaultSoundSpawnInfo;
    }

    if (!SoundResource)
    {
        LOG("SoundEmitter::SpawnSound: No sound specified\n");
        return;
    }

    if (SoundResource->GetFrameCount() == 0)
    {
        LOG("SoundEmitter::SpawnSound: Sound has no frames\n");
        return;
    }

    int startFrame = SpawnInfo->StartFrame;

    if (startFrame < 0)
    {
        startFrame = 0;
    }

    if (startFrame >= SoundResource->GetFrameCount())
    {
        return;
    }

    if (SpawnInfo->AudioClient && SpawnInfo->AudioClient->IsPendingKill())
    {
        return;
    }

    SoundAttenuationParameters const& atten = SpawnInfo->Attenuation;

    float refDist = Math::Clamp(atten.ReferenceDistance, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX);
    float maxDist = Math::Clamp(atten.Distance, refDist, SOUND_DISTANCE_MAX);
    float falloff = FalloffDistance(maxDist);

    if (SpawnInfo->EmitterType != SOUND_EMITTER_BACKGROUND && !SpawnInfo->bVirtualizeWhenSilent)
    {
        AudioListener const& listener = GEngine->GetAudioSystem()->GetListener();
        float                 cullDist = maxDist + falloff;

        if (listener.Position.DistSqr(SpawnPosition) >= cullDist * cullDist)
        {
            // Sound is too far from listener
            return;
        }
    }

    TRef<AudioStream> streamInterface;

    // Initialize audio stream instance
    if (SoundResource->GetStreamType() != SOUND_STREAM_DISABLED)
    {
        if (!SoundResource->CreateStreamInstance(&streamInterface))
        {
            LOG("SoundEmitter::SpawnSound: Couldn't create audio stream instance\n");
            return;
        }
    }
    else
    {
        if (!SoundResource->GetAudioBuffer())
        {
            LOG("SoundEmitter::SpawnSound: Resource has no audio buffer\n");
            return;
        }
    }

    auto& pool = GEngine->GetAudioSystem()->GetOneShotPool();

    SoundOneShot* sound           = new (pool.Allocate()) SoundOneShot;
    sound->Volume                  = Math::Saturate(SpawnInfo->Volume);
    sound->ReferenceDistance       = refDist;
    sound->MaxDistance             = maxDist;
    sound->RolloffRate             = Math::Saturate(atten.RolloffRate);
    sound->bStopWhenInstigatorDead = Instigator && SpawnInfo->bStopWhenInstigatorDead;
    sound->EmitterType             = SpawnInfo->EmitterType;
    sound->Resource                = SoundResource;
    sound->ResourceRevision        = SoundResource->GetRevision();
    sound->Priority                = SpawnInfo->Priority;
    sound->bFollowInstigator       = SpawnInfo->bFollowInstigator;
    if (SpawnInfo->EmitterType == SOUND_EMITTER_DIRECTIONAL)
    {
        sound->ConeInnerAngle = Math::Clamp(SpawnInfo->ConeInnerAngle, 0.0f, 360.0f);
        sound->ConeOuterAngle = Math::Clamp(SpawnInfo->ConeOuterAngle, SpawnInfo->ConeInnerAngle, 360.0f);

        if (SpawnInfo->bFollowInstigator)
        {
            sound->SoundDirection = Instigator ? Instigator->GetWorldForwardVector() : SpawnInfo->Direction;
        }
        else
        {
            sound->SoundDirection = SpawnInfo->Direction;
        }
    }
    sound->AudioClient           = SpawnInfo->AudioClient ? SpawnInfo->AudioClient->Id : 0;
    sound->ListenerMask          = SpawnInfo->ListenerMask;
    sound->Group                 = SpawnInfo->Group;
    sound->Instigator            = Instigator;
    sound->InstigatorId          = Instigator ? Instigator->GetOwnerActor()->Id : 0;
    sound->pWorld                = World;
    sound->SoundPosition         = SpawnPosition;
    sound->bVirtualizeWhenSilent = SpawnInfo->bVirtualizeWhenSilent;
    sound->Spatialize();

    if (!sound->bVirtualizeWhenSilent && sound->ChanVolume[0] == 0 && sound->ChanVolume[1] == 0)
    {
        // Don't even start
        FreeSound(sound);
        return;
    }

    INTRUSIVE_ADD(sound, Next, Prev, m_OneShots, m_OneShotsTail);

    sound->Channel = new AudioChannel(startFrame,
                                       -1,
                                       0,
                                       SoundResource->GetAudioBuffer(),
                                       streamInterface.GetObject(),
                                       sound->bVirtualizeWhenSilent,
                                       sound->ChanVolume,
                                       sound->LocalDir,
                                       sound->bSpatializedStereo,
                                       sound->IsPaused());

    GEngine->GetAudioSystem()->GetMixer()->SubmitChannel(sound->Channel);
}

void SoundEmitter::ClearOneShotSounds()
{
    SoundOneShot* next;

    for (SoundOneShot* sound = m_OneShots; sound; sound = next)
    {
        next = sound->Next;

        FreeSound(sound);
    }

    HK_ASSERT(m_OneShots == nullptr);
}

void SoundEmitter::FreeSound(SoundOneShot* Sound)
{
    if (Sound->Channel)
    {
        Sound->Channel->RemoveRef();
    }

    INTRUSIVE_REMOVE(Sound, Next, Prev, m_OneShots, m_OneShotsTail);

    Sound->~SoundOneShot();

    auto& pool = GEngine->GetAudioSystem()->GetOneShotPool();
    pool.Deallocate(Sound);
}

void SoundEmitter::UpdateSound(SoundOneShot* Sound)
{
    AudioChannel* chan = Sound->Channel;

    // Check if the instigator is still alive
    if (Sound->bStopWhenInstigatorDead && Sound->Instigator && Sound->Instigator->IsPendingKill())
    {
        FreeSound(Sound);
        return;
    }

    // Check if the audio clip has been modified
    if (Sound->ResourceRevision != Sound->Resource->GetRevision())
    {
        FreeSound(Sound);
        return;
    }

    // Free the channel if the sound stops
    if (chan->GetPlaybackPos() >= chan->FrameCount || chan->IsStopped())
    {
        FreeSound(Sound);
        return;
    }

    // Update position, direction, velocity
    if (Sound->bFollowInstigator && Sound->Instigator && !Sound->Instigator->IsPendingKill())
    {
        Sound->SoundPosition = Sound->Instigator->GetWorldPosition();

        if (Sound->EmitterType == SOUND_EMITTER_DIRECTIONAL)
        {
            Sound->SoundDirection = Sound->Instigator->GetWorldForwardVector();
        }
    }

    bool paused = Sound->IsPaused();

    if (!paused)
    {
        Sound->Spatialize();
    }

    chan->Commit(Sound->ChanVolume, Sound->LocalDir, Sound->bSpatializedStereo, paused);
}

void SoundEmitter::UpdateSounds()
{
    SoundOneShot* next;

    for (SoundOneShot* sound = m_OneShots; sound; sound = next)
    {
        next = sound->Next;
        UpdateSound(sound);
    }

    for (SoundEmitter* e = m_SoundEmitters; e; e = e->GetNext())
    {
        e->Update();
    }
}

void SoundOneShot::Spatialize()
{
    AudioListener const& listener = GEngine->GetAudioSystem()->GetListener();

    ChanVolume[0] = 0;
    ChanVolume[1] = 0;

    // Cull by client
    if (AudioClient && listener.Id != AudioClient)
    {
        return;
    }

    // Cull by mask
    if ((ListenerMask & listener.Mask) == 0)
    {
        return;
    }

    float volume = Volume;
    volume *= listener.VolumeScale;
    if (pWorld)
    {
        volume *= pWorld->GetAudioVolume();
    }
    if (Group)
    {
        volume *= Group->GetVolume();
    }

    // Cull by volume
    if (volume < 0.0001f)
    {
        return;
    }

    // Don't be too lound
    if (volume > 1.0f)
    {
        volume = 1.0f;
    }

    const float VolumeFtoI = 65535;

    volume *= VolumeFtoI;

    // If the sound is played from the listener, consider it as background
    if (EmitterType == SOUND_EMITTER_BACKGROUND || (bFollowInstigator && InstigatorId == listener.Id))
    {
        // Use full volume without attenuation
        ChanVolume[0] = ChanVolume[1] = (int)volume;

        // Don't spatialize stereo sounds
        bSpatializedStereo = false;
        return;
    }

    float leftVol, rightVol;

    CalcAttenuation(EmitterType, SoundPosition, SoundDirection, listener.Position, listener.RightVec, ReferenceDistance, MaxDistance, RolloffRate, ConeInnerAngle, ConeOuterAngle, &leftVol, &rightVol);

    ChanVolume[0] = (int)(volume * leftVol);
    ChanVolume[1] = (int)(volume * rightVol);

    // Should never happen, but just in case
    if (ChanVolume[0] < 0)
        ChanVolume[0] = 0;
    if (ChanVolume[1] < 0)
        ChanVolume[1] = 0;
    if (ChanVolume[0] > 65535)
        ChanVolume[0] = 65535;
    if (ChanVolume[1] > 65535)
        ChanVolume[1] = 65535;

    bSpatializedStereo = !GEngine->GetAudioSystem()->GetPlaybackDevice()->IsMono();

    if (Snd_HRTF)
    {
        LocalDir = listener.TransformInv * SoundPosition;
        float d  = LocalDir.NormalizeSelf();
        if (d < 0.0001f)
        {
            // Sound has same position as listener
            LocalDir = Float3(0, 1, 0);
        }
    }
}

bool SoundOneShot::IsPaused() const
{
    bool bPlayEvenWhenPaused = Group ? Group->ShouldPlayEvenWhenPaused() : false;

    bool paused = false;
    if (pWorld && !bPlayEvenWhenPaused)
    {
        paused = pWorld->IsPaused();
    }
    if (Group)
    {
        paused = paused || Group->IsPaused();
    }

    return paused;
}

HK_NAMESPACE_END
