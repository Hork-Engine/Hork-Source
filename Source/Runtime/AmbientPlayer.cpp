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
#include "Engine.h"

HK_CLASS_META(AAmbientPlayer)

AAmbientPlayer::AAmbientPlayer()
{
}

void AAmbientPlayer::Initialize(SActorInitializer& Initializer)
{
    Initializer.bCanEverTick = true;
}

void AAmbientPlayer::PreInitializeComponents()
{
    Super::PreInitializeComponents();

    ALevel* level = GetLevel();

    int ambientCount = level->AmbientSounds.Size();
    AmbientSound.Resize(ambientCount);
    for (int i = 0; i < ambientCount; i++)
    {
        AmbientSound[i] = CreateComponent<ASoundEmitter>("Ambient");
        AmbientSound[i]->SetEmitterType(SOUND_EMITTER_BACKGROUND);
        AmbientSound[i]->SetVirtualizeWhenSilent(true);
        AmbientSound[i]->SetVolume(0.0f);
    }
}

void AAmbientPlayer::BeginPlay()
{
    Super::BeginPlay();

    ALevel* level = GetLevel();

    int ambientCount = level->AmbientSounds.Size();

    for (int i = 0; i < ambientCount; i++)
    {
        AmbientSound[i]->PlaySound(level->AmbientSounds[i], 0, 0);
    }
}

void AAmbientPlayer::Tick(float _TimeStep)
{
    Super::Tick(_TimeStep);

    UpdateAmbientVolume(_TimeStep);
}

void AAmbientPlayer::UpdateAmbientVolume(float _TimeStep)
{
    ALevel* level = GetLevel();

    int leaf = level->FindLeaf(GEngine->GetAudioSystem()->GetListener().Position);

    if (leaf < 0)
    {
        int ambientCount = AmbientSound.Size();

        for (int i = 0; i < ambientCount; i++)
        {
            AmbientSound[i]->SetVolume(0.0f);
        }
        return;
    }

    int audioAreaNum = level->Leafs[leaf].AudioArea;

    SAudioArea const* audioArea = &level->AudioAreas[audioAreaNum];

    const float scale = 0.1f;
    const float step  = _TimeStep * scale;
    for (int i = 0; i < MAX_AMBIENT_SOUNDS_IN_AREA; i++)
    {
        uint16_t soundIndex = audioArea->AmbientSound[i];
        float    volume     = (float)audioArea->AmbientVolume[i] / 255.0f * scale;

        ASoundEmitter* emitter = AmbientSound[soundIndex];

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
