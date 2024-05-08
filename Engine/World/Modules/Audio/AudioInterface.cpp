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
#include "AudioModule.h"

#include <Engine/Core/Logger.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>

#include <Engine/Audio/AudioDevice.h>
#include <Engine/Audio/AudioMixer.h>

#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

AudioInterface::AudioInterface(ECS::World* world) :
    m_World(world)
{}

void AudioInterface::SetListener(ECS::EntityHandle inEntity)
{
    m_Listener = inEntity;
}

/*

TODO: Add these parameters?

/// If target listener is not specified, audio will be hearable for all listeners
ECS::EntityHandle TargetListener;

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
    one_shot.bNeedToSubmit = true;
    one_shot.Volume = Math::Saturate(inVolume);
    one_shot.Group = inGroup;
    one_shot.Position = inPosition;
    one_shot.bIsBackground = false;
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
    one_shot.bNeedToSubmit = true;
    one_shot.Volume = Math::Saturate(inVolume);
    one_shot.Group = inGroup;
    one_shot.bIsBackground = true;
    m_OneShotSound.Add(one_shot); // TODO: Thread-safe
}

void CalcAttenuation(SOUND_SOURCE_TYPE SourceType,
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

    if (bIsBackground)
    {
        // Use full volume without attenuation
        outChanVolume[0] = outChanVolume[1] = ivolume;

        // Don't spatialize stereo sounds
        outSpatializedStereo = false;
        return;
    }

    float left_vol, right_vol;

    CalcAttenuation(bIsBackground ? SOUND_SOURCE_BACKGROUND : SOUND_SOURCE_POINT,
                    Position,
                    Float3::AxisX(),
                    inListener.Position,
                    inListener.RightVec,
                    SOUND_REF_DISTANCE_DEFAULT,
                    SOUND_DISTANCE_DEFAULT,
                    SOUND_ROLLOFF_RATE_DEFAULT,
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

    outSpatializedStereo = !AudioModule::Get().GetDevice()->IsMono();

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

void AudioInterface::UpdateOneShotSound(AudioMixerSubmitQueue& submitQueue, AudioListener const& inListener)
{
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
            paused = paused || bPaused;
        if (it->Group)
            paused = paused || it->Group->IsPaused();

        int chan_vol[2];
        Float3 local_dir;
        bool spatialized_stereo;
        it->Spatialize(inListener, chan_vol, local_dir, spatialized_stereo);

        if (it->bNeedToSubmit && chan_vol[0] == 0 && chan_vol[1] == 0)
        {
            it = m_OneShotSound.Erase(it);
            continue;
        }

        it->Track->SetPlaybackParameters(chan_vol, local_dir, spatialized_stereo, paused);

        if (it->bNeedToSubmit)
        {
            it->bNeedToSubmit = false;
            submitQueue.Add(it->Track);
        }

        it++;
    }
}

HK_NAMESPACE_END
