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

#include "AudioInterface.h"

#include <Engine/Core/Logger.h>

#include <Engine/Audio/AudioDevice.h>

#include <Engine/GameApplication/GameApplication.h>

#include "Components/SoundSource.h"
#include "Components/AudioListenerComponent.h"

HK_NAMESPACE_BEGIN

//ConsoleVar Snd_RefreshRate("Snd_RefreshRate"s, "16"s); // TODO
ConsoleVar Snd_MasterVolume("Snd_MasterVolume"s, "1"s);

AudioInterface::AudioInterface()
{}

void AudioInterface::Initialize()
{
    TickFunction f;
    f.Desc.Name.FromString("UpdateAudio");
    f.Desc.TickEvenWhenPaused = true;
    f.Group = TickGroup::PostTransform;
    f.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
    f.Delegate.Bind(this, &AudioInterface::Update);
    RegisterTickFunction(f);
}

void AudioInterface::Deinitialize()
{
}

void AudioInterface::SetListener(Handle32<AudioListenerComponent> inListener)
{
    m_ListenerComponent = inListener;
}

/*

TODO: Add these parameters?

/// If target listener is not specified, audio will be hearable for all listeners
Handle32<AudioListenerComponent> TargetListener;

/// With listener mask you can filter listeners for the sound
uint32_t ListenerMask = ~0u;

/// Distance attenuation parameter
/// Can be from SOUND_DISTANCE_MIN to SOUND_DISTANCE_MAX
float ReferenceDistance = SOUND_REF_DISTANCE_DEFAULT;

/// Distance attenuation parameter
/// Can be from ReferenceDistance to SOUND_DISTANCE_MAX
float Distance = SOUND_DISTANCE_DEFAULT;

/// Distance attenuation parameter
/// Gain rolloff factor
float RolloffRate = SOUND_ROLLOFF_RATE_DEFAULT;

*/

void AudioInterface::PlaySoundAt(SoundHandle inSound, Float3 const& inPosition, SoundGroup* inGroup, float inVolume, int inStartFrame)
{
    if (inVolume <= 0.0001f)
        return;

    if (!inSound.IsValid())
    {
        LOG("SoundSource::StartPlay: No sound specified\n");
        return;
    }

    auto resource = GameApplication::GetResourceManager().TryGet(inSound);
    if (!resource)
    {
        LOG("SoundSource::StartPlay: Sound is not loaded\n");
        return;
    }

    auto source = resource->GetSource();
    if (!source)
    {
        LOG("SoundSource::StartPlay: Resource has no audio\n");
        return;
    }

    if (source->GetFrameCount() == 0)
    {
        LOG("SoundSource::StartPlay: Sound has no frames\n");
        return;
    }

    if (inStartFrame < 0)
        inStartFrame = 0;

    if (inStartFrame >= source->GetFrameCount())
        return;

    OneShotSound one_shot;
    one_shot.Track.Attach(new AudioTrack(source, inStartFrame, -1, 0, false));
    one_shot.NeedToSubmit = true;
    one_shot.Volume = Math::Saturate(inVolume);
    one_shot.Group = inGroup;
    one_shot.Position = inPosition;
    one_shot.IsBackground = false;
    m_OneShotSound.Add(one_shot);// TODO: Thread-safe
}

void AudioInterface::PlaySoundBackground(SoundHandle inSound, SoundGroup* inGroup, float inVolume, int inStartFrame)
{
    if (inVolume <= 0.0001f)
        return;

    if (!inSound.IsValid())
    {
        LOG("SoundSource::StartPlay: No sound specified\n");
        return;
    }

    auto resource = GameApplication::GetResourceManager().TryGet(inSound);
    if (!resource)
    {
        LOG("SoundSource::StartPlay: Sound is not loaded\n");
        return;
    }

    auto source = resource->GetSource();
    if (!source)
    {
        LOG("SoundSource::StartPlay: Resource has no audio\n");
        return;
    }

    if (source->GetFrameCount() == 0)
    {
        LOG("SoundSource::StartPlay: Sound has no frames\n");
        return;
    }

    if (inStartFrame < 0)
        inStartFrame = 0;

    if (inStartFrame >= source->GetFrameCount())
        return;

    OneShotSound one_shot;
    one_shot.Track.Attach(new AudioTrack(source, inStartFrame, -1, 0, false));
    one_shot.NeedToSubmit = true;
    one_shot.Volume = Math::Saturate(inVolume);
    one_shot.Group = inGroup;
    one_shot.IsBackground = true;
    m_OneShotSound.Add(one_shot); // TODO: Thread-safe
}

void CalcAttenuation(SoundSourceType SourceType,
                     Float3 const& SoundPosition,
                     Float3 const& SoundDirection,
                     Float3 const& ListenerPosition,
                     Float3 const& ListenerRightVec,
                     float ReferenceDistance,
                     float MaxDistance,
                     float RolloffRate,
                     float ConeInnerAngle,
                     float ConeOuterAngle,
                     float* pLeftVol,
                     float* pRightVol);

 void AudioInterface::OneShotSound::Spatialize(AudioListener const& inListener, int outChanVolume[2], Float3& outLocalDir, bool& outSpatializedStereo)
{
    outChanVolume[0] = 0;
    outChanVolume[1] = 0;
    outSpatializedStereo = false;

    // Filter by target listener
    //if (m_TargetListener && inListener.Entity != m_TargetListener)
    //    return;

    // Cull by mask
    //if ((m_ListenerMask & inListener.Mask) == 0)
    //    return;

    float volume = Volume;
    volume *= inListener.VolumeScale;
    if (Group)
        volume *= Group->GetVolume();

    // Don't be too lound
    if (volume > 1.0f)
        volume = 1.0f;

    const float VolumeFtoI = 65535;

    volume *= VolumeFtoI;
    int ivolume = (int)volume;

    // Cull by volume
    if (ivolume == 0)
        return;

    if (IsBackground)
    {
        // Use full volume without attenuation
        outChanVolume[0] = outChanVolume[1] = ivolume;

        // Don't spatialize stereo sounds
        outSpatializedStereo = false;
        return;
    }

    float left_vol, right_vol;

    const float ReferenceDistance = 1;
    const float MaxDistance = 100.0f;
    const float RolloffRate = 1;

    CalcAttenuation(IsBackground ? SoundSourceType::Background : SoundSourceType::Point,
                    Position,
                    Float3::AxisX(),
                    inListener.Position,
                    inListener.RightVec,
                    ReferenceDistance,
                    MaxDistance,
                    RolloffRate,
                    0,
                    0,
                    &left_vol,
                    &right_vol);

    outChanVolume[0] = (int)(volume * left_vol);
    outChanVolume[1] = (int)(volume * right_vol);

    // Should never happen, but just in case
    if (outChanVolume[0] < 0)
        outChanVolume[0] = 0;
    if (outChanVolume[1] < 0)
        outChanVolume[1] = 0;
    if (outChanVolume[0] > 65535)
        outChanVolume[0] = 65535;
    if (outChanVolume[1] > 65535)
        outChanVolume[1] = 65535;

    outSpatializedStereo = !GameApplication::GetAudioDevice()->IsMono();

    if (Snd_HRTF)
    {
        outLocalDir = inListener.TransformInv * Position;
        float d = outLocalDir.NormalizeSelf();
        if (d < 0.0001f)
        {
            // Sound has same position as listener
            outLocalDir = Float3(0, 1, 0);
        }
    }
}

void AudioInterface::UpdateOneShotSound()
{
    bool isPaused = GetWorld()->GetTick().IsPaused;

    for (auto it = m_OneShotSound.begin(); it != m_OneShotSound.end();)
    {
        if (it->Track->GetPlaybackPos() >= it->Track->FrameCount || it->Track->IsStopped())
        {
            it = m_OneShotSound.Erase(it);
            continue;
        }

        bool paused = false;
        bool play_even_when_paused = it->Group ? it->Group->ShouldPlayEvenWhenPaused() : false;
        if (!play_even_when_paused)
            paused = paused || isPaused;
        if (it->Group)
            paused = paused || it->Group->IsPaused();

        int chan_vol[2];
        Float3 local_dir;
        bool spatialized_stereo;
        it->Spatialize(m_Listener, chan_vol, local_dir, spatialized_stereo);

        if (it->NeedToSubmit && chan_vol[0] == 0 && chan_vol[1] == 0)
        {
            it = m_OneShotSound.Erase(it);
            continue;
        }

        it->Track->SetPlaybackParameters(chan_vol, local_dir, spatialized_stereo, paused);

        if (it->NeedToSubmit)
        {
            it->NeedToSubmit = false;
            m_SubmitQueue.Add(it->Track);
        }

        it++;
    }
}

void AudioInterface::Update()
{
    auto& audioListenerManager = GetWorld()->GetComponentManager<AudioListenerComponent>();

    if (auto listenerComponent = audioListenerManager.GetComponent(m_ListenerComponent))
    {
        auto owner = listenerComponent->GetOwner();

        m_Listener.Entity = owner->GetHandle();
        m_Listener.TransformInv.Compose(owner->GetWorldPosition(), owner->GetWorldRotation().ToMatrix3x3());
        m_Listener.TransformInv.InverseSelf();
        m_Listener.Position = owner->GetWorldPosition();
        m_Listener.RightVec = owner->GetWorldRotation().XAxis();
        m_Listener.VolumeScale = Math::Saturate(listenerComponent->Volume * MasterVolume * Snd_MasterVolume.GetFloat());
        m_Listener.Mask = listenerComponent->ListenerMask;
    }
    else
    {
        m_Listener.Entity = {};
        m_Listener.TransformInv.SetIdentity();
        m_Listener.Position.Clear();
        m_Listener.RightVec = Float3(1, 0, 0);
        m_Listener.VolumeScale = Math::Saturate(MasterVolume * Snd_MasterVolume.GetFloat());
        m_Listener.Mask = ~0u;
    }

    auto& soundSourceManager = GetWorld()->GetComponentManager<SoundSource>();

    struct Visitor
    {
        AudioListener& m_Listener;
        AudioMixerSubmitQueue& m_SubmitQueue;
        bool m_IsPaused;

        Visitor(AudioListener& listener, AudioMixerSubmitQueue& submitQueue, bool isPaused) :
            m_Listener(listener), m_SubmitQueue(submitQueue), m_IsPaused(isPaused)
        {
        }

        HK_FORCEINLINE void Visit(SoundSource& soundSource)
        {
            soundSource.Spatialize(m_Listener);
            soundSource.UpdateTrack(m_SubmitQueue, m_IsPaused);
        }
    };

    Visitor visitor(m_Listener, m_SubmitQueue, GetWorld()->GetTick().IsPaused);
    soundSourceManager.IterateComponents(visitor);

    UpdateOneShotSound();

    GameApplication::GetAudioMixer()->SubmitTracks(m_SubmitQueue);
}

HK_NAMESPACE_END
