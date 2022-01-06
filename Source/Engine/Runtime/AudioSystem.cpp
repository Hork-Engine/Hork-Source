/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include "AudioSystem.h"
#include "PlayerController.h"

#include <Platform/Logger.h>
#include <Core/IntrusiveLinkedListMacro.h>

#include <Audio/AudioDevice.h>
#include <Audio/AudioMixer.h>

AConsoleVar Snd_MasterVolume( _CTS("Snd_MasterVolume"), _CTS("1") );
AConsoleVar Snd_RefreshRate( _CTS("Snd_RefreshRate"), _CTS("16") );

AAudioSystem & GAudioSystem = AAudioSystem::Inst();

AAudioSystem::AAudioSystem()
{
}

AAudioSystem::~AAudioSystem()
{
}

void AAudioSystem::Initialize()
{
    GLogger.Printf( "Initializing audio system...\n" );

    pPlaybackDevice = MakeUnique< AAudioDevice >( 44100 );
    pMixer = MakeUnique< AAudioMixer >( pPlaybackDevice.GetObject() );
    bMono = pPlaybackDevice->IsMono();

    pMixer->StartAsync();
}

void AAudioSystem::Deinitialize()
{
    GLogger.Printf( "Deinitializing audio system...\n" );

    pMixer.Reset();
    pPlaybackDevice.Reset();
    OneShotPool.Free();
}

void AAudioSystem::Update( APlayerController * _Controller, float _TimeStep )
{
    ASceneComponent * audioListener = _Controller ? _Controller->GetAudioListener() : nullptr;
    AAudioParameters * audioParameters = _Controller ? _Controller->GetAudioParameters() : nullptr;

    if ( audioListener ) {
        Listener.Position = audioListener->GetWorldPosition();
        Listener.RightVec = audioListener->GetWorldRightVector();

        Listener.TransformInv.Compose( Listener.Position, audioListener->GetWorldRotation().ToMatrix3x3() );
        // We can optimize Inverse like for viewmatrix
        Listener.TransformInv.InverseSelf();

        Listener.Id = audioListener->GetOwnerActor()->Id;

    }
    else {
        Listener.Position.Clear();
        Listener.RightVec = Float3(1,0,0);

        Listener.TransformInv.SetIdentity();

        Listener.Id = 0;
    }

    if ( audioParameters ) {
        Listener.VolumeScale = Math::Saturate( audioParameters->Volume * Snd_MasterVolume.GetFloat() );
        Listener.Mask = audioParameters->ListenerMask;
    }
    else {
        // set defaults
        Listener.VolumeScale = Math::Saturate( Snd_MasterVolume.GetFloat() );
        Listener.Mask = ~0u;
    }

    static double time = 0;

    time += _TimeStep;
    if ( time > 1.0f / Snd_RefreshRate.GetFloat() ) {
        time = 0;

        //GLogger.Printf("Update\n");
        ASoundEmitter::UpdateSounds();
    }

//    pMixer->Update();
}
