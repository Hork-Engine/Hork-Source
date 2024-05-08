#include "SoundSource.h"
#include "AudioModule.h"

#include <Engine/Audio/AudioMixer.h>
#include <Engine/GameApplication/GameApplication.h>

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

    auto resource = GameApplication::GetResourceManager().TryGet(inSound);
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

    bool play_now = IsSilent();
    if (play_now && m_AudioQueue.empty())
    {
        StartPlay(inSound, 0, -1);
        return;
    }

    m_AudioQueue.push(inSound);

    if (play_now)
        SelectNextSound();
}

bool SoundSource::SelectNextSound()
{
    bool was_selected = false;

    m_Track.Reset();
    m_SoundHandle = {};

    while (!m_AudioQueue.empty() && !was_selected)
    {
        SoundHandle playSound = m_AudioQueue.front();
        m_AudioQueue.pop();
        was_selected = StartPlay(playSound, 0, -1);
    }

    return was_selected;
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

    PlayOneShotData& one_shot = m_PlayOneShot.EmplaceBack();
    one_shot.Track.Attach(new AudioTrack(source, inStartFrame, -1, 0, m_bVirtualizeWhenSilent));
    one_shot.bNeedToSubmit = true;
    one_shot.VolumeScale = Math::Saturate(inVolumeScale);
}

bool SoundSource::StartPlay(SoundHandle inSound, int inStartFrame, int inLoopStart)
{
    if (!inSound.IsValid())
    {
        LOG("SoundSource::StartPlay: No sound specified\n");
        return false;
    }

    auto resource = GameApplication::GetResourceManager().TryGet(inSound);
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

    int loops_count = 0;

    if (inStartFrame >= source->GetFrameCount())
    {
        if (inLoopStart < 0)
            return false;
        inStartFrame = inLoopStart;
        loops_count++;
    }

    m_SoundHandle = inSound;

    m_Track.Attach(new AudioTrack(source, inStartFrame, inLoopStart, loops_count, m_bVirtualizeWhenSilent));
    m_bNeedToSubmit = true;
 
    return true;
}

bool SoundSource::RestartSound()
{
    SoundHandle new_sound = m_SoundHandle;

    int loop_start = m_Track ? m_Track->GetLoopStart() : -1;

    m_Track.Reset();
    m_SoundHandle = {};

    return StartPlay(new_sound, 0, loop_start);
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
    AudioDevice* device = AudioModule::Get().GetDevice();

    int frameNum = Math::Round(inTime * device->GetSampleRate());
    SetPlaybackPosition(frameNum);
}

float SoundSource::GetPlaybackTime() const
{
    AudioDevice* device = AudioModule::Get().GetDevice();
    if (!m_Track)
        return 0;
    return (float)m_Track->GetPlaybackPos() / device->GetSampleRate();
}

void SoundSource::SetEntity(ECS::EntityHandle inEntity)
{
    m_Entity = inEntity;
}

ECS::EntityHandle SoundSource::GetEntity() const
{
    return m_Entity;
}

void SoundSource::SetPositionAndRotation(Float3 const& inPosition, Quat const& inRotaiton)
{
    m_Position = inPosition;
    m_Direction = -inRotaiton.ZAxis();
}

void SoundSource::SetSoundGroup(SoundGroup* inGroup)
{
    m_Group = inGroup;
}

void SoundSource::SetTargetListener(ECS::EntityHandle inListener)
{
    m_TargetListener = inListener;
}

void SoundSource::SetListenerMask(uint32_t inMask)
{
    m_ListenerMask = inMask;
}

void SoundSource::SetSourceType(SOUND_SOURCE_TYPE inSourceType)
{
    m_SourceType = inSourceType;
}

void SoundSource::SetVirtualizeWhenSilent(bool inVirtualizeWhenSilent)
{
    m_bVirtualizeWhenSilent = inVirtualizeWhenSilent;
}

void SoundSource::SetVolume(float inVolume)
{
    m_Volume = Math::Saturate(inVolume);
}

void SoundSource::SetReferenceDistance(float inDist)
{
    m_ReferenceDistance = Math::Clamp(inDist, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX);
}

void SoundSource::SetMaxDistance(float inDist)
{
    m_MaxDistance = Math::Clamp(inDist, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX);
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
    m_bPaused = inPaused;
}

void SoundSource::SetMuted(bool inMuted)
{
    m_bMuted = inMuted;
}

bool SoundSource::IsMuted() const
{
    return m_bMuted;
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
    float max_dist = Math::Clamp(m_MaxDistance, m_ReferenceDistance, SOUND_DISTANCE_MAX);
    float falloff = FalloffDistance(max_dist);
    return max_dist + falloff;
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
                            float* pRightVol)
{
    Float3 dir = SoundPosition - ListenerPosition;
    float distance = dir.NormalizeSelf();
    float attenuation = 1;

    // Cone attenuation
    if (SourceType == SOUND_SOURCE_DIRECTIONAL && ConeInnerAngle < 360.0f)
    {
        float angle = -2.0f * Math::Degrees(std::acos(Math::Dot(SoundDirection, dir)));
        float angle_interval = ConeOuterAngle - ConeInnerAngle;

        if (angle > ConeInnerAngle)
        {
            if (angle_interval > 0.0f)
            {
                //attenuation = std::powf(1.0f - Math::Clamp(angle - ConeInnerAngle, 0.0f, angleInterval) / angleInterval, RolloffFactor);
                attenuation = 1.0f - (angle - ConeInnerAngle) / angle_interval;
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
            attenuation = 0;
        else
            attenuation *= 1.0f - distance / falloff;
    }

    // Panning
    if (Snd_HRTF || AudioModule::Get().GetDevice()->IsMono())
    {
        *pLeftVol = *pRightVol = attenuation;
    }
    else
    {
        float panning = Math::Dot(ListenerRightVec, dir);

        float left_pan = 1.0f - panning;
        float right_pan = 1.0f + panning;

        *pLeftVol = attenuation * left_pan;
        *pRightVol = attenuation * right_pan;
    }
}

void SoundSource::Spatialize(AudioListener const& inListener)
{
    m_ChanVolume[0] = 0;
    m_ChanVolume[1] = 0;

    // Cull if muted
    if (m_bMuted)
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
    if (m_SourceType == SOUND_SOURCE_BACKGROUND || m_Entity == inListener.Entity)
    {
        // Use full volume without attenuation
        m_ChanVolume[0] = m_ChanVolume[1] = ivolume;

        // Don't spatialize stereo sounds
        m_bSpatializedStereo = false;
        return;
    }

    float left_vol, right_vol;

    CalcAttenuation(m_SourceType,
                    m_Position,
                    m_Direction,
                    inListener.Position,
                    inListener.RightVec,
                    m_ReferenceDistance,
                    m_MaxDistance,
                    m_RolloffRate,
                    m_ConeInnerAngle,
                    m_ConeOuterAngle,
                    &left_vol,
                    &right_vol);

    m_ChanVolume[0] = (int)(volume * left_vol);
    m_ChanVolume[1] = (int)(volume * right_vol);

    // Should never happen, but just in case
    if (m_ChanVolume[0] < 0)
        m_ChanVolume[0] = 0;
    if (m_ChanVolume[1] < 0)
        m_ChanVolume[1] = 0;
    if (m_ChanVolume[0] > 65535)
        m_ChanVolume[0] = 65535;
    if (m_ChanVolume[1] > 65535)
        m_ChanVolume[1] = 65535;

    m_bSpatializedStereo = !AudioModule::Get().GetDevice()->IsMono();

    if (Snd_HRTF)
    {
        m_LocalDir = inListener.TransformInv * m_Position;
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
    bool paused = m_bPaused;
    bool play_even_when_paused = m_Group ? m_Group->ShouldPlayEvenWhenPaused() : false;
    if (!play_even_when_paused)
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

        int chan_vol[2];
        chan_vol[0] = m_ChanVolume[0] * it->VolumeScale;
        chan_vol[1] = m_ChanVolume[1] * it->VolumeScale;

        if (it->bNeedToSubmit && !m_bVirtualizeWhenSilent && chan_vol[0] == 0 && chan_vol[1] == 0)
        {
            it = m_PlayOneShot.Erase(it);
            continue;
        }

        it->Track->SetPlaybackParameters(chan_vol, m_LocalDir, m_bSpatializedStereo, paused);

        if (it->bNeedToSubmit)
        {
            it->bNeedToSubmit = false;
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

    m_Track->SetPlaybackParameters(m_ChanVolume, m_LocalDir, m_bSpatializedStereo, paused);

    if (m_bNeedToSubmit)
    {
        m_bNeedToSubmit = false;
        submitQueue.Add(m_Track);
    }
}

HK_NAMESPACE_END
