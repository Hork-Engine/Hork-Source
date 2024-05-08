#pragma once

#include <Engine/Math/Quat.h>
#include <Engine/ECS/ECS.h>
#include <Engine/Audio/AudioTrack.h>
#include <Engine/World/Resources/Resource_Sound.h>

#include <EASTL/queue.h>

HK_NAMESPACE_BEGIN

struct AudioListener
{
    /// Entity
    ECS::EntityHandle Entity = 0;

    /// World transfrom inversed
    Float3x4 TransformInv;

    /// World position
    Float3 Position;

    /// View right vector
    Float3 RightVec;

    /// Volume factor
    float VolumeScale = 1.0f;

    /// Listener mask
    uint32_t Mask = ~0u;
};

class SoundGroup : public RefCounted
{
public:
    /// Scale volume for all sounds in group
    void SetVolume(float inVolume)
    {
        m_Volume = Math::Saturate(inVolume);
    }

    /// Scale volume for all sounds in group
    float GetVolume() const
    {
        return m_Volume;
    }

    /// Pause/unpause all sounds in group
    void SetPaused(bool inPaused)
    {
        m_bPaused = inPaused;
    }

    /// Is group paused
    bool IsPaused() const
    {
        return m_bPaused;
    }

    /// Play sounds even when game is paused
    void SetPlayEvenWhenPaused(bool inPlayEvenWhenPaused)
    {
        m_bPlayEvenWhenPaused = inPlayEvenWhenPaused;
    }

    /// Play sounds even when game is paused
    bool ShouldPlayEvenWhenPaused() const
    {
        return m_bPlayEvenWhenPaused;
    }

private:
    /// Scale volume for all sounds in group
    float m_Volume = 1;

    /// Pause all sounds in group
    bool m_bPaused = false;

    /// Play sounds even when game is paused
    bool m_bPlayEvenWhenPaused = false;
};

enum SOUND_SOURCE_TYPE
{
    /// Point sound source
    SOUND_SOURCE_POINT,

    /// Cone sound source
    SOUND_SOURCE_DIRECTIONAL,

    /// Background sound (usually music or speach)
    SOUND_SOURCE_BACKGROUND
};

/// Audio distance attenuation model. Not used now, reserved for future.
enum AUDIO_DISTANCE_MODEL
{
    AUDIO_DIST_INVERSE          = 0,
    AUDIO_DIST_INVERSE_CLAMPED  = 1, // default
    AUDIO_DIST_LINEAR           = 2,
    AUDIO_DIST_LINEAR_CLAMPED   = 3,
    AUDIO_DIST_EXPONENT         = 4,
    AUDIO_DIST_EXPONENT_CLAMPED = 5
};

/// Priority to play the sound.
/// NOTE: Not used now. Reserved for future to pick a free channel.
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

class SoundSource
{
public:
    SoundSource() = default;
    SoundSource(SoundSource const&) = delete;
    SoundSource& operator=(SoundSource const&) = delete;
    SoundSource(SoundSource&&) = default;
    SoundSource& operator=(SoundSource&&) = default;

    /// Start playing sound. This function cancels any sound that is already being played by the source.
    void PlaySound(SoundHandle inSound, int inStartFrame = 0, int inLoopStart = -1);
    
    /// Play one shot. Does not cancel sounds that are already being played by PlayOneShot and PlaySound.
    /// This function creates a separate track for sound playback.
    void PlayOneShot(SoundHandle inSound, float inVolumeScale, int inStartFrame = 0);

    /// Stops playing any sound from this source.
    void ClearSound();

    /// Add sound to queue
    void AddToQueue(SoundHandle inSound);

    /// Clear sound queue
    void ClearQueue();

    /// Set playback position in frames
    void SetPlaybackPosition(int inFrameNum);

    /// Get playback position in frames
    int GetPlaybackPosition() const;

    /// Set playback position in seconds
    void SetPlaybackTime(float inTime);

    /// Get playback position in seconds
    float GetPlaybackTime() const;

    /// Reload and restart current sound
    bool RestartSound();

    /// Select next sound from queue
    bool SelectNextSound();

    /// Assign an entity to the source
    void SetEntity(ECS::EntityHandle inEntity);

    /// Returns assigned entity
    ECS::EntityHandle GetEntity() const;

    /// Set source position and rotation
    void SetPositionAndRotation(Float3 const& inPosition, Quat const& inRotaiton);

    /// We can control the volume by groups of sound sources
    void SetSoundGroup(SoundGroup* inGroup);

    /// We can control the volume by groups of sound sources
    SoundGroup* GetSoundGroup() const { return m_Group; }

    /// If target listener is not specified, audio will be hearable for all listeners
    void SetTargetListener(ECS::EntityHandle inListener);

    /// Returns target listener. If target listener is not specified, audio will be hearable for all listeners
    ECS::EntityHandle GetTargetListener() const { return m_TargetListener; }

    /// With listener mask you can filter listeners for the sound
    void SetListenerMask(uint32_t inMask);

    /// With listener mask you can filter listeners for the sound
    uint32_t GetListenerMask() const { return m_ListenerMask; }

    /// Set source type. See SOUND_SOURCE_TYPE
    void SetSourceType(SOUND_SOURCE_TYPE inSourceType);

    /// Get source type. See SOUND_SOURCE_TYPE
    SOUND_SOURCE_TYPE GetSourceType() const { return m_SourceType; }

    /// Virtualize sound when silent. Looped sounds has this by default.
    void SetVirtualizeWhenSilent(bool inVirtualizeWhenSilent);

    /// Virtualize sound when silent. Looped sounds has this by default.
    bool ShouldVirtualizeWhenSilent() const { return m_bVirtualizeWhenSilent; }

    /// Audio volume scale
    void SetVolume(float inVolume);

    /// Audio volume scale
    float GetVolume() const { return m_Volume; }

    /// Distance attenuation parameter
    /// Can be from SOUND_DISTANCE_MIN to SOUND_DISTANCE_MAX
    void SetReferenceDistance(float inDist);

    /// Distance attenuation parameter
    /// Can be from SOUND_DISTANCE_MIN to SOUND_DISTANCE_MAX
    float GetReferenceDistance() const { return m_ReferenceDistance; }

    /// Distance attenuation parameter
    /// Can be from ReferenceDistance to SOUND_DISTANCE_MAX
    void SetMaxDistance(float inDist);

    /// Distance attenuation parameter
    /// Can be from ReferenceDistance to SOUND_DISTANCE_MAX
    float GetMaxDistance() const { return m_MaxDistance; }

    /// Distance at which sound can be heard
    float GetCullDistance() const;

    /// Distance attenuation parameter
    /// Gain rolloff factor
    void SetRolloffRate(float inRolloff);

    /// Distance attenuation parameter
    /// Gain rolloff factor
    float GetRollofRate() const { return m_RolloffRate; }

    /// Directional sound inner cone angle in degrees. [0-360]
    void SetConeInnerAngle(float inAngle);

    /// Directional sound inner cone angle in degrees. [0-360]
    float GetConeInnerAngle() const { return m_ConeInnerAngle; }

    /// Directional sound outer cone angle in degrees. [0-360]
    void SetConeOuterAngle(float inAngle);

    /// Directional sound outer cone angle in degrees. [0-360]
    float GetConeOuterAngle() const { return m_ConeOuterAngle; }

    /// Pause/Unpause
    void SetPaused(bool inPaused);

    /// Set audio volume to 0
    void SetMuted(bool inMuted);

    /// Restore audio volume
    bool IsMuted() const;

    /// Return true if no sound plays
    bool IsSilent() const;

    void Spatialize(AudioListener const& inListener);

    void UpdateTrack(class AudioMixerSubmitQueue& submitQueue, bool inPaused);

private:
    bool StartPlay(SoundHandle inSound, int inStartFrame, int inLoopStart);

    using Queue = eastl::queue<SoundHandle, eastl::deque<SoundHandle, Allocators::HeapMemoryAllocator<HEAP_VECTOR>, DEQUE_DEFAULT_SUBARRAY_SIZE(SoundHandle)>>;//TPodQueue<SoundHandle, 1, true>;

    ECS::EntityHandle   m_Entity;
    Float3              m_Position;
    Float3              m_Direction;
    Queue               m_AudioQueue;
    TRef<SoundGroup>    m_Group;
    ECS::EntityHandle   m_TargetListener;
    uint32_t            m_ListenerMask = ~0u;
    SOUND_SOURCE_TYPE   m_SourceType = SOUND_SOURCE_POINT;
    SoundHandle         m_SoundHandle;
    TRef<AudioTrack>    m_Track;
    float               m_Volume = 1.0f;
    float               m_ReferenceDistance = SOUND_REF_DISTANCE_DEFAULT;
    float               m_MaxDistance = SOUND_DISTANCE_DEFAULT;
    float               m_RolloffRate = SOUND_ROLLOFF_RATE_DEFAULT;
    float               m_ConeInnerAngle = 360;
    float               m_ConeOuterAngle = 360;
    int                 m_ChanVolume[2] = {0, 0};
    Float3              m_LocalDir;
    bool                m_bSpatializedStereo = false;
    bool                m_bPaused = false;
    bool                m_bVirtualizeWhenSilent = false;
    bool                m_bMuted = false;
    bool                m_bNeedToSubmit = false;

    struct PlayOneShotData
    {
        TRef<AudioTrack> Track;
        bool bNeedToSubmit;
        float VolumeScale;
    };
    TVector<PlayOneShotData> m_PlayOneShot;
};

HK_NAMESPACE_END
