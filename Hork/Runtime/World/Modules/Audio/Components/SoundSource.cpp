/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "SoundSource.h"

#include <Hork/Audio/AudioMixer.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

extern ConsoleVar Snd_HRTF;

void SoundSource::ClearSound()
{
    m_Track.Reset();
    m_SoundHandle = {};
    m_ChanVolume[0] = 0;
    m_ChanVolume[1] = 0;

    ClearQueue();
}

void SoundSource::AddToQueue(SoundHandle inSound)
{
    if (!inSound.IsValid())
    {
        LOG("SoundComponent::AddToQueue: No sound specified\n");
        return;
    }

    auto resource = GameApplication::sGetResourceManager().TryGet(inSound);
    if (!resource)
    {
        LOG("SoundComponent::AddToQueue: Sound is not loaded\n");
        return;
    }

    auto source = resource->GetSource();
    if (source->GetFrameCount() == 0)
    {
        LOG("SoundComponent::AddToQueue: Sound has no frames\n");
        return;
    }

    bool playNow = IsSilent();
    if (playNow && m_AudioQueue.empty())
    {
        StartPlay(inSound, 0, -1);
        return;
    }

    m_AudioQueue.push(inSound);

    if (playNow)
        SelectNextSound();
}

bool SoundSource::SelectNextSound()
{
    bool wasSelected = false;

    m_Track.Reset();
    m_SoundHandle = {};

    while (!m_AudioQueue.empty() && !wasSelected)
    {
        SoundHandle playSound = m_AudioQueue.front();
        m_AudioQueue.pop();
        wasSelected = StartPlay(playSound, 0, -1);
    }

    return wasSelected;
}

void SoundSource::ClearQueue()
{
    while (!m_AudioQueue.empty())
        m_AudioQueue.pop();
}

void SoundSource::PlaySound(SoundHandle inSound, int inStartFrame, int inLoopStart)
{
    ClearSound();
    StartPlay(inSound, inStartFrame, inLoopStart);
}

void SoundSource::PlayOneShot(SoundHandle inSound, float inVolumeScale, int inStartFrame)
{
    if (inVolumeScale <= 0.0001f)
        return;

    if (!inSound.IsValid())
    {
        LOG("SoundSource::StartPlay: No sound specified\n");
        return;
    }

    auto resource = GameApplication::sGetResourceManager().TryGet(inSound);
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

    PlayOneShotData& oneShot = m_PlayOneShot.EmplaceBack();
    oneShot.Track.Attach(new AudioTrack(source, inStartFrame, -1, 0, m_VirtualizeWhenSilent));
    oneShot.NeedToSubmit = true;
    oneShot.VolumeScale = Math::Saturate(inVolumeScale);
}

bool SoundSource::StartPlay(SoundHandle inSound, int inStartFrame, int inLoopStart)
{
    if (!inSound.IsValid())
    {
        LOG("SoundSource::StartPlay: No sound specified\n");
        return false;
    }

    auto resource = GameApplication::sGetResourceManager().TryGet(inSound);
    if (!resource)
    {
        LOG("SoundSource::StartPlay: Sound is not loaded\n");
        return false;
    }

    auto source = resource->GetSource();
    if (!source)
    {
        LOG("SoundSource::StartPlay: Resource has no audio\n");
        return false;
    }

    if (source->GetFrameCount() == 0)
    {
        LOG("SoundSource::StartPlay: Sound has no frames\n");
        return false;
    }

    if (inLoopStart >= source->GetFrameCount())
        inLoopStart = 0;

    if (inStartFrame < 0)
        inStartFrame = 0;

    int loopsCount = 0;

    if (inStartFrame >= source->GetFrameCount())
    {
        if (inLoopStart < 0)
            return false;
        inStartFrame = inLoopStart;
        loopsCount++;
    }

    m_SoundHandle = inSound;

    m_Track.Attach(new AudioTrack(source, inStartFrame, inLoopStart, loopsCount, m_VirtualizeWhenSilent));
    m_NeedToSubmit = true;
 
    return true;
}

bool SoundSource::RestartSound()
{
    SoundHandle newSound = m_SoundHandle;

    int loop_start = m_Track ? m_Track->GetLoopStart() : -1;

    m_Track.Reset();
    m_SoundHandle = {};

    return StartPlay(newSound, 0, loop_start);
}

void SoundSource::SetPlaybackPosition(int inFrameNum)
{
    if (!m_Track)
        return;
    if (m_Track->GetPlaybackPos() == inFrameNum)
        return;

    m_Track->SetPlaybackPosition(Math::Clamp(inFrameNum, 0, m_Track->FrameCount));
}

int SoundSource::GetPlaybackPosition() const
{
    if (!m_Track)
        return 0;
    return m_Track->GetPlaybackPos();
}

void SoundSource::SetPlaybackTime(float inTime)
{
    AudioDevice* device = GameApplication::sGetAudioDevice();

    int frameNum = Math::Round(inTime * device->GetSampleRate());
    SetPlaybackPosition(frameNum);
}

float SoundSource::GetPlaybackTime() const
{
    AudioDevice* device = GameApplication::sGetAudioDevice();
    if (!m_Track)
        return 0;
    return (float)m_Track->GetPlaybackPos() / device->GetSampleRate();
}

void SoundSource::SetSoundGroup(SoundGroup* inGroup)
{
    m_Group = inGroup;
}

void SoundSource::SetTargetListener(GameObjectHandle inListener)
{
    m_TargetListener = inListener;
}

void SoundSource::SetListenerMask(uint32_t inMask)
{
    m_ListenerMask = inMask;
}

void SoundSource::SetSourceType(SoundSourceType inSourceType)
{
    m_SourceType = inSourceType;
}

void SoundSource::SetVirtualizeWhenSilent(bool inVirtualizeWhenSilent)
{
    m_VirtualizeWhenSilent = inVirtualizeWhenSilent;
}

void SoundSource::SetVolume(float inVolume)
{
    m_Volume = Math::Saturate(inVolume);
}

void SoundSource::SetReferenceDistance(float inDist)
{
    m_ReferenceDistance = Math::Clamp(inDist, MinSoundDistance, MaxSoundDsitance);
}

void SoundSource::SetMaxDistance(float inDist)
{
    m_MaxDistance = Math::Clamp(inDist, MinSoundDistance, MaxSoundDsitance);
}

void SoundSource::SetRolloffRate(float inRolloff)
{
    m_RolloffRate = Math::Clamp(inRolloff, 0.0f, 1.0f);
}

void SoundSource::SetConeInnerAngle(float inAngle)
{
    m_ConeInnerAngle = Math::Clamp(inAngle, 0.0f, 360.0f);
}

void SoundSource::SetConeOuterAngle(float inAngle)
{
    m_ConeOuterAngle = Math::Clamp(inAngle, 0.0f, 360.0f);
}

void SoundSource::SetPaused(bool inPaused)
{
    m_IsPaused = inPaused;
}

void SoundSource::SetMuted(bool inMuted)
{
    m_IsMuted = inMuted;
}

bool SoundSource::IsMuted() const
{
    return m_IsMuted;
}

bool SoundSource::IsSilent() const
{
    return !m_SoundHandle.IsValid();
}

HK_FORCEINLINE float FalloffDistance(float inMaxDistance)
{
    return inMaxDistance * 1.3f;
}

float SoundSource::GetCullDistance() const
{
    float maxDist = Math::Clamp(m_MaxDistance, m_ReferenceDistance, MaxSoundDsitance);
    float falloff = FalloffDistance(maxDist);
    return maxDist + falloff;
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
                     float* pRightVol)
{
    Float3 dir = SoundPosition - ListenerPosition;
    float distance = dir.NormalizeSelf();
    float attenuation = 1;

    // Cone attenuation
    if (SourceType == SoundSourceType::Directional && ConeInnerAngle < 360.0f)
    {
        float angle = -2.0f * Math::Degrees(std::acos(Math::Dot(SoundDirection, dir)));
        float angleInterval = ConeOuterAngle - ConeInnerAngle;

        if (angle > ConeInnerAngle)
        {
            if (angleInterval > 0.0f)
            {
                //attenuation = std::powf(1.0f - Math::Clamp(angle - ConeInnerAngle, 0.0f, angleInterval) / angleInterval, RolloffFactor);
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
    //attenuation *= (1.0f - RolloffRate * (d - ReferenceDistance) / (MaxDistance - ReferenceDistance));

    // Inverse distance clamped model
    attenuation *= ReferenceDistance / (ReferenceDistance + RolloffRate * (d - ReferenceDistance));

    // Falloff
    distance -= MaxDistance;
    if (distance > 0)
    {
        const float falloff = FalloffDistance(MaxDistance);
        if (distance >= falloff)
            attenuation = 0;
        else
            attenuation *= 1.0f - distance / falloff;
    }

    // Panning
    if (Snd_HRTF || GameApplication::sGetAudioDevice()->IsMono())
    {
        *pLeftVol = *pRightVol = attenuation;
    }
    else
    {
        float panning = Math::Dot(ListenerRightVec, dir);

        float leftPan = 1.0f - panning;
        float rightPan = 1.0f + panning;

        *pLeftVol = attenuation * leftPan;
        *pRightVol = attenuation * rightPan;
    }
}

void SoundSource::Spatialize(AudioListener const& inListener)
{
    m_ChanVolume[0] = 0;
    m_ChanVolume[1] = 0;

    // Cull if muted
    if (m_IsMuted)
        return;

    // Filter by target listener
    if (m_TargetListener && inListener.Entity != m_TargetListener)
        return;

    // Cull by mask
    if ((m_ListenerMask & inListener.Mask) == 0)
        return;

    float volume = m_Volume;
    volume *= inListener.VolumeScale;
    if (m_Group)
        volume *= m_Group->GetVolume();

    // Don't be too lound
    if (volume > 1.0f)
        volume = 1.0f;

    const float VolumeFtoI = 65535;

    volume *= VolumeFtoI;
    int ivolume = (int)volume;

    // Cull by volume
    if (ivolume == 0)
        return;

    // If the sound is played from the listener, consider it as background
    if (m_SourceType == SoundSourceType::Background || GetOwner()->GetHandle() == inListener.Entity)
    {
        // Use full volume without attenuation
        m_ChanVolume[0] = m_ChanVolume[1] = ivolume;

        // Don't spatialize stereo sounds
        m_SpatializedStereo = false;
        return;
    }

    float leftVol, rightVol;

    auto* owner = GetOwner();
    Float3 position = owner->GetWorldPosition();
    Float3 dir = owner->GetWorldDirection();

    CalcAttenuation(m_SourceType,
                    position,
                    dir,
                    inListener.Position,
                    inListener.RightVec,
                    m_ReferenceDistance,
                    m_MaxDistance,
                    m_RolloffRate,
                    m_ConeInnerAngle,
                    m_ConeOuterAngle,
                    &leftVol,
                    &rightVol);

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

    m_SpatializedStereo = !GameApplication::sGetAudioDevice()->IsMono();

    if (Snd_HRTF)
    {
        m_LocalDir = inListener.TransformInv * position;
        float d = m_LocalDir.NormalizeSelf();
        if (d < 0.0001f)
        {
            // Sound has same position as listener
            m_LocalDir = Float3(0, 1, 0);
        }
    }
}

void SoundSource::UpdateTrack(AudioMixerSubmitQueue& submitQueue, bool inPaused)
{
    bool paused = m_IsPaused;
    bool playEvenWhenPaused = m_Group ? m_Group->ShouldPlayEvenWhenPaused() : false;
    if (!playEvenWhenPaused)
        paused = paused || inPaused;
    if (m_Group)
        paused = paused || m_Group->IsPaused();

    for (auto it = m_PlayOneShot.begin(); it != m_PlayOneShot.end(); )
    {
        if (it->Track->GetPlaybackPos() >= it->Track->FrameCount || it->Track->IsStopped())
        {
            it = m_PlayOneShot.Erase(it);
            continue;
        }

        int chanVol[2];
        chanVol[0] = m_ChanVolume[0] * it->VolumeScale;
        chanVol[1] = m_ChanVolume[1] * it->VolumeScale;

        if (it->NeedToSubmit && !m_VirtualizeWhenSilent && chanVol[0] == 0 && chanVol[1] == 0)
        {
            it = m_PlayOneShot.Erase(it);
            continue;
        }

        it->Track->SetPlaybackParameters(chanVol, m_LocalDir, m_SpatializedStereo, paused);

        if (it->NeedToSubmit)
        {
            it->NeedToSubmit = false;
            submitQueue.Add(it->Track);
        }

        it++;
    }

    if (!m_SoundHandle.IsValid())
    {
        // silent
        return;
    }

    HK_ASSERT(m_Track);

    // Select next sound from queue if playback position is reached the end
    if (m_Track->GetPlaybackPos() >= m_Track->FrameCount)
    {
        if (!SelectNextSound())
            return;
    }

    if (m_Track->IsStopped())
    {
        ClearSound();
        return;
    }

    m_Track->SetPlaybackParameters(m_ChanVolume, m_LocalDir, m_SpatializedStereo, paused);

    if (m_NeedToSubmit)
    {
        m_NeedToSubmit = false;
        submitQueue.Add(m_Track);
    }
}

HK_NAMESPACE_END
