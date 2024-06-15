/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Engine/World/Modules/Audio/AudioInterface.h>

#include <Engine/Math/Quat.h>
#include <Engine/Audio/AudioTrack.h>
#include <Engine/World/Resources/Resource_Sound.h>

#include <EASTL/queue.h>

HK_NAMESPACE_BEGIN

enum class SoundSourceType
{
    /// Point sound source
    Point,

    /// Cone sound source
    Directional,

    /// Background sound (usually music or speach)
    Background
};

class SoundSource final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    static constexpr float  MinSoundDistance = 0.1f;
    static constexpr float  MaxSoundDsitance = 1000.0f;

    /// Start playing sound. This function cancels any sound that is already being played by the source.
    void                    PlaySound(SoundHandle inSound, int inStartFrame = 0, int inLoopStart = -1);
    
    /// Play one shot. Does not cancel sounds that are already being played by PlayOneShot and PlaySound.
    /// This function creates a separate track for sound playback.
    void                    PlayOneShot(SoundHandle inSound, float inVolumeScale, int inStartFrame = 0);

    /// Stops playing any sound from this source.
    void                    ClearSound();

    /// Add sound to queue
    void                    AddToQueue(SoundHandle inSound);

    /// Clear sound queue
    void                    ClearQueue();

    /// Set playback position in frames
    void                    SetPlaybackPosition(int inFrameNum);

    /// Get playback position in frames
    int                     GetPlaybackPosition() const;

    /// Set playback position in seconds
    void                    SetPlaybackTime(float inTime);

    /// Get playback position in seconds
    float                   GetPlaybackTime() const;

    /// Reload and restart current sound
    bool                    RestartSound();

    /// Select next sound from queue
    bool                    SelectNextSound();

    /// We can control the volume by groups of sound sources
    void                    SetSoundGroup(SoundGroup* inGroup);

    /// We can control the volume by groups of sound sources
    SoundGroup*             GetSoundGroup() const { return m_Group; }

    /// If target listener is not specified, audio will be hearable for all listeners
    void                    SetTargetListener(GameObjectHandle inListener);

    /// Returns target listener. If target listener is not specified, audio will be hearable for all listeners
    GameObjectHandle        GetTargetListener() const { return m_TargetListener; }

    /// With listener mask you can filter listeners for the sound
    void                    SetListenerMask(uint32_t inMask);

    /// With listener mask you can filter listeners for the sound
    uint32_t                GetListenerMask() const { return m_ListenerMask; }

    /// Set source type. See SoundSourceType
    void                    SetSourceType(SoundSourceType inSourceType);

    /// Get source type. See SoundSourceType
    SoundSourceType         GetSourceType() const { return m_SourceType; }

    /// Virtualize sound when silent. Looped sounds has this by default.
    void                    SetVirtualizeWhenSilent(bool inVirtualizeWhenSilent);

    /// Virtualize sound when silent. Looped sounds has this by default.
    bool                    ShouldVirtualizeWhenSilent() const { return m_VirtualizeWhenSilent; }

    /// Audio volume scale
    void                    SetVolume(float inVolume);

    /// Audio volume scale
    float                   GetVolume() const { return m_Volume; }

    /// Distance attenuation parameter
    /// Can be from MinSoundDistance to MaxSoundDsitance
    void                    SetReferenceDistance(float inDist);

    /// Distance attenuation parameter
    /// Can be from MinSoundDistance to MaxSoundDsitance
    float                   GetReferenceDistance() const { return m_ReferenceDistance; }

    /// Distance attenuation parameter
    /// Can be from ReferenceDistance to MaxSoundDsitance
    void                    SetMaxDistance(float inDist);

    /// Distance attenuation parameter
    /// Can be from ReferenceDistance to MaxSoundDsitance
    float                   GetMaxDistance() const { return m_MaxDistance; }

    /// Distance at which sound can be heard
    float                   GetCullDistance() const;

    /// Distance attenuation parameter
    /// Gain rolloff factor
    void                    SetRolloffRate(float inRolloff);

    /// Distance attenuation parameter
    /// Gain rolloff factor
    float                   GetRollofRate() const { return m_RolloffRate; }

    /// Directional sound inner cone angle in degrees. [0-360]
    void                    SetConeInnerAngle(float inAngle);

    /// Directional sound inner cone angle in degrees. [0-360]
    float                   GetConeInnerAngle() const { return m_ConeInnerAngle; }

    /// Directional sound outer cone angle in degrees. [0-360]
    void                    SetConeOuterAngle(float inAngle);

    /// Directional sound outer cone angle in degrees. [0-360]
    float                   GetConeOuterAngle() const { return m_ConeOuterAngle; }

    /// Pause/Unpause
    void                    SetPaused(bool inPaused);

    /// Set audio volume to 0
    void                    SetMuted(bool inMuted);

    /// Restore audio volume
    bool                    IsMuted() const;

    /// Return true if no sound plays
    bool                    IsSilent() const;

    void                    Spatialize(AudioListener const& inListener);

    void                    UpdateTrack(class AudioMixerSubmitQueue& submitQueue, bool inPaused);

private:
    bool                    StartPlay(SoundHandle inSound, int inStartFrame, int inLoopStart);

    using Queue = eastl::queue<SoundHandle, eastl::deque<SoundHandle, Allocators::HeapMemoryAllocator<HEAP_VECTOR>, DEQUE_DEFAULT_SUBARRAY_SIZE(SoundHandle)>>;//PodQueue<SoundHandle, 1, true>;

    Queue                   m_AudioQueue;
    Ref<SoundGroup>        m_Group;
    GameObjectHandle        m_TargetListener;
    uint32_t                m_ListenerMask = ~0u;
    SoundSourceType         m_SourceType = SoundSourceType::Point;
    SoundHandle             m_SoundHandle;
    Ref<AudioTrack>        m_Track;
    float                   m_Volume = 1.0f;
    float                   m_ReferenceDistance = 1;
    float                   m_MaxDistance = 100.0f;
    float                   m_RolloffRate = 1;
    float                   m_ConeInnerAngle = 360;
    float                   m_ConeOuterAngle = 360;
    int                     m_ChanVolume[2] = {0, 0};
    Float3                  m_LocalDir;
    bool                    m_SpatializedStereo = false;
    bool                    m_IsPaused = false;
    bool                    m_IsMuted = false;
    bool                    m_VirtualizeWhenSilent = false;
    bool                    m_NeedToSubmit = false;

    struct PlayOneShotData
    {
        Ref<AudioTrack>    Track;
        float               VolumeScale;
        bool                NeedToSubmit;
    };
    Vector<PlayOneShotData> m_PlayOneShot;
};

HK_NAMESPACE_END
