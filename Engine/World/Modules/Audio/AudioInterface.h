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

#pragma once

#include <Engine/World/Common/BaseModule.h>
#include "SoundSource.h"

HK_NAMESPACE_BEGIN

class AudioInterface
{
    HK_FORBID_COPY(AudioInterface)

public:
    float MasterVolume = 1;
    bool bPaused = false;

    explicit AudioInterface(ECS::World* world);

    void SetListener(ECS::EntityHandle inEntity);

    ECS::EntityHandle GetListener() const { return m_Listener; }

    /// Plays a sound at a given position in world space.
    void PlaySoundAt(SoundHandle inSound, Float3 const& inPosition, SoundGroup* inGroup = nullptr, float inVolume = 1.0f, int inStartFrame = 0);

    /// Plays a sound at background.
    void PlaySoundBackground(SoundHandle inSound, SoundGroup* inGroup = nullptr, float inVolume = 1.0f, int inStartFrame = 0);

    void UpdateOneShotSound(AudioMixerSubmitQueue& submitQueue, AudioListener const& inListener);

private:
    ECS::World* m_World;
    ECS::EntityHandle m_Listener;

    struct OneShotSound
    {
        TRef<AudioTrack> Track;
        TRef<SoundGroup> Group;
        Float3 Position;
        float Volume;
        bool bIsBackground;
        bool bNeedToSubmit;

        void Spatialize(AudioListener const& inListener, int outChanVolume[2], Float3& outLocalDir, bool& outSpatializedStereo);
    };
    TVector<OneShotSound> m_OneShotSound;
};

HK_NAMESPACE_END
