/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "AmbientPlayer.h"
#include "SoundEmitter.h"

#include <Runtime/Engine.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(Actor_AmbientPlayer)

Actor_AmbientPlayer::Actor_AmbientPlayer()
{
}

void Actor_AmbientPlayer::Initialize(ActorInitializer& Initializer)
{
    Initializer.bCanEverTick = true;
}

void Actor_AmbientPlayer::PreInitializeComponents()
{
    Super::PreInitializeComponents();

    Level* level = GetLevel();
    LevelAudio* audio = level->Audio;
    if (audio)
    {
        int ambientCount = audio->AmbientSounds.Size();
        AmbientSound.Resize(ambientCount);
        for (int i = 0; i < ambientCount; i++)
        {
            AmbientSound[i] = CreateComponent<SoundEmitter>("Ambient");
            AmbientSound[i]->SetEmitterType(SOUND_EMITTER_BACKGROUND);
            AmbientSound[i]->SetVirtualizeWhenSilent(true);
            AmbientSound[i]->SetVolume(0.0f);
        }
    }
}

void Actor_AmbientPlayer::BeginPlay()
{
    Super::BeginPlay();

    Level* level = GetLevel();

    LevelAudio* audio = level->Audio;
    if (audio)
    {
        int ambientCount = audio->AmbientSounds.Size();

        for (int i = 0; i < ambientCount; i++)
        {
            AmbientSound[i]->PlaySound(audio->AmbientSounds[i], 0, 0);
        }
    }
}

void Actor_AmbientPlayer::Tick(float _TimeStep)
{
    Super::Tick(_TimeStep);

    UpdateAmbientVolume(_TimeStep);
}

void Actor_AmbientPlayer::UpdateAmbientVolume(float _TimeStep)
{
    Level* level = GetLevel();
    LevelAudio* audio = level->Audio;

    if (!audio)
        return;
    
    VisibilityLevel* visibility = level->Visibility;

    int leaf = visibility->FindLeaf(GEngine->GetAudioSystem()->GetListener().Position);

    if (leaf < 0)
    {
        int ambientCount = AmbientSound.Size();

        for (int i = 0; i < ambientCount; i++)
        {
            AmbientSound[i]->SetVolume(0.0f);
        }
        return;
    }

    int audioAreaNum = visibility->GetLeafs()[leaf].AudioArea;

    AudioArea const* audioArea = &audio->AudioAreas[audioAreaNum];

    const float scale = 0.1f;
    const float step  = _TimeStep * scale;
    for (int i = 0; i < MAX_AMBIENT_SOUNDS_IN_AREA; i++)
    {
        uint16_t soundIndex = audioArea->AmbientSound[i];
        float    volume     = (float)audioArea->AmbientVolume[i] / 255.0f * scale;

        if (soundIndex >= AmbientSound.Size())
            continue;

        SoundEmitter* emitter = AmbientSound[soundIndex];

        float vol = emitter->GetVolume();

        if (vol < volume)
        {
            vol += step;

            if (vol > volume)
            {
                vol = volume;
            }
        }
        else if (vol > volume)
        {
            vol -= step;

            if (vol < 0)
            {
                vol = 0;
            }
        }

        emitter->SetVolume(vol);
    }
}

HK_NAMESPACE_END
