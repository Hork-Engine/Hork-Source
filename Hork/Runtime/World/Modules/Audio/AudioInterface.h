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

#include <Hork/Runtime/World/WorldInterface.h>
#include <Hork/Runtime/Resources/Resource_Sound.h>
#include <Hork/Runtime/World/GameObject.h>

#include <Hork/Audio/AudioMixer.h>

#include "Components/AudioListenerComponent.h"

HK_NAMESPACE_BEGIN

/// Audio distance attenuation model. Not used now, reserved for future.
enum class AudioDistanceModel
{
    Inverse          = 0,
    InverseClamped   = 1, // default
    Linear           = 2,
    LinearClamped    = 3,
    Exponent         = 4,
    ExponentClamped  = 5
};

/// Priority to play the sound.
/// NOTE: Not used now. Reserved for future to pick a free channel.
enum class AudioChannelPriority : uint8_t
{
    OneShot  = 0,
    Ambient  = 1,
    Music    = 2,
    Dialogue = 3,
    Max      = 255
};

struct AudioListener
{
    /// Entity
    GameObjectHandle        Entity;

    /// World transfrom inversed
    Float3x4                TransformInv;

    /// World position
    Float3                  Position;

    /// View right vector
    Float3                  RightVec;

    /// Volume factor
    float                   VolumeScale = 1.0f;

    /// Listener mask
    uint32_t                Mask = ~0u;
};

class SoundGroup : public RefCounted
{
public:
    /// Scale volume for all sounds in group
    void                    SetVolume(float inVolume) { m_Volume = Math::Saturate(inVolume); }

    /// Scale volume for all sounds in group
    float                   GetVolume() const { return m_Volume; }

    /// Pause/unpause all sounds in group
    void                    SetPaused(bool inPaused) { m_IsPaused = inPaused; }

    /// Is group paused
    bool                    IsPaused() const { return m_IsPaused; }

    /// Play sounds even when game is paused
    void                    SetPlayEvenWhenPaused(bool inPlayEvenWhenPaused) { m_PlayEvenWhenPaused = inPlayEvenWhenPaused; }

    /// Play sounds even when game is paused
    bool                    ShouldPlayEvenWhenPaused() const { return m_PlayEvenWhenPaused; }

private:
    /// Scale volume for all sounds in group
    float                   m_Volume = 1;

    /// Pause all sounds in group
    bool                    m_IsPaused = false;

    /// Play sounds even when game is paused
    bool                    m_PlayEvenWhenPaused = false;
};

class AudioInterface : public WorldInterfaceBase
{
public:
    float                   MasterVolume = 1;

                            AudioInterface();

    void                    SetListener(Handle32<AudioListenerComponent> inListener);

    Handle32<AudioListenerComponent> GetListener() const { return m_ListenerComponent; }

    /// Plays a sound at a given position in world space.
    void                    PlaySoundAt(SoundHandle inSound, Float3 const& inPosition, SoundGroup* inGroup = nullptr, float inVolume = 1.0f, int inStartFrame = 0);

    /// Plays a sound at background.
    void                    PlaySoundBackground(SoundHandle inSound, SoundGroup* inGroup = nullptr, float inVolume = 1.0f, int inStartFrame = 0);

protected:
    virtual void            Initialize() override;
    virtual void            Deinitialize() override;

private:
    void                    UpdateOneShotSound();
    void                    Update();
    //void                    DrawDebug(DebugRenderer& renderer);

    Handle32<AudioListenerComponent> m_ListenerComponent;
    AudioListener           m_Listener;
    AudioMixerSubmitQueue   m_SubmitQueue;

    struct OneShotSound
    {
        Ref<AudioTrack>     Track;
        Ref<SoundGroup>     Group;
        Float3              Position;
        float               Volume;
        bool                IsBackground;
        bool                NeedToSubmit;

        void                Spatialize(AudioListener const& inListener, int outChanVolume[2], Float3& outLocalDir, bool& outSpatializedStereo);
    };
    Vector<OneShotSound>    m_OneShotSound;
};

HK_NAMESPACE_END
