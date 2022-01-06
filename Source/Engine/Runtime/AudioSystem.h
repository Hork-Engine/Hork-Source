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

#pragma once

#include "SoundEmitter.h"

class AAudioDevice;
class AAudioMixer;
class APlayerController;

struct SAudioListener
{
    /** Actor ID */
    uint64_t Id = 0;

    /** World transfrom inversed */
    Float3x4 TransformInv;

    /** World position */
    Float3 Position;

    /** View right vector */
    Float3 RightVec;

    /** Volume factor */
    float VolumeScale = 1.0f;

    /** Listener mask */
    uint32_t Mask = ~0u;
};

class AAudioSystem
{
    AN_SINGLETON( AAudioSystem )

public:
    /** Initialize audio system */
    void Initialize();

    /** Deinitialize audio system */
    void Deinitialize();

    AAudioDevice * GetPlaybackDevice() const
    {
        return pPlaybackDevice.GetObject();
    }

    AAudioMixer * GetMixer() const
    {
        return pMixer.GetObject();
    }

    TPoolAllocator< ASoundOneShot, 128 > & GetOneShotPool()
    {
        return OneShotPool;
    }

    bool IsMono() const
    {
        return bMono;
    }

    ///** Get current audio listener position */
    //Float3 const & GetListenerPosition() const
    //{
    //    return ListenerPosition;
    //}

    SAudioListener const & GetListener() const
    {
        return Listener;
    }

    void Update( APlayerController * _Controller, float _TimeStep );

private:
    ~AAudioSystem();

    TUniqueRef< AAudioDevice > pPlaybackDevice;
    TUniqueRef< AAudioMixer > pMixer;
    TPoolAllocator< ASoundOneShot, 128 > OneShotPool;
    SAudioListener Listener;
    bool bMono;
};

extern AAudioSystem & GAudioSystem;
