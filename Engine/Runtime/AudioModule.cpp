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

#include "AudioModule.h"

#include <Engine/Core/Logger.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>

#include <Engine/Audio/AudioDevice.h>
#include <Engine/Audio/AudioMixer.h>

HK_NAMESPACE_BEGIN

ConsoleVar Snd_MasterVolume("Snd_MasterVolume"s, "1"s);
ConsoleVar Snd_RefreshRate("Snd_RefreshRate"s, "16"s);

AudioModule::AudioModule()
{
    LOG("Initializing audio system...\n");

    m_pPlaybackDevice = MakeRef<AudioDevice>(44100);

    m_pMixer = MakeUnique<AudioMixer>(m_pPlaybackDevice);
    m_pMixer->StartAsync();
}

AudioModule::~AudioModule()
{
    LOG("Deinitializing audio system...\n");
}

void AudioModule::Update(Actor_PlayerController* _Controller, float timeStep)
{
#if 0
    SceneComponent*  audioListener = _Controller ? _Controller->GetAudioListener() : nullptr;
    AudioParameters* audioParameters = _Controller ? _Controller->GetAudioParameters() : nullptr;

    if (audioListener)
    {
        m_Listener.Position = audioListener->GetWorldPosition();
        m_Listener.RightVec = audioListener->GetWorldRightVector();

        m_Listener.TransformInv.Compose(m_Listener.Position, audioListener->GetWorldRotation().ToMatrix3x3());
        // We can optimize Inverse like for viewmatrix
        m_Listener.TransformInv.InverseSelf();

        m_Listener.Id = audioListener->GetOwnerActor()->Id;
    }
    else
    {
        m_Listener.Position.Clear();
        m_Listener.RightVec = Float3(1, 0, 0);

        m_Listener.TransformInv.SetIdentity();

        m_Listener.Id = 0;
    }

    if (audioParameters)
    {
        m_Listener.VolumeScale = Math::Saturate(audioParameters->Volume * Snd_MasterVolume.GetFloat());
        m_Listener.Mask        = audioParameters->ListenerMask;
    }
    else
    {
        // set defaults
        m_Listener.VolumeScale = Math::Saturate(Snd_MasterVolume.GetFloat());
        m_Listener.Mask        = ~0u;
    }

    static double time = 0;

    time += timeStep;
    if (time > 1.0f / Snd_RefreshRate.GetFloat())
    {
        time = 0;

        SoundEmitter::UpdateSounds();
    }

    if (!m_pMixer->IsAsync())
    {
        m_pMixer->Update();
    }
#endif
}

HK_NAMESPACE_END
