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

#include "BaseModule.h"

#include <Engine/Core/Allocators/PoolAllocator.h>
#include <Engine/Math/VectorMath.h>

HK_NAMESPACE_BEGIN

class AudioDevice;
class AudioMixer;
class Actor_PlayerController;

struct AudioListener
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

class AudioModule : public BaseModule<AudioModule>
{
    friend class BaseModule<AudioModule>;

public:
    AudioDevice* GetPlaybackDevice() const
    {
        return m_pPlaybackDevice;
    }

    AudioMixer* GetMixer() const
    {
        return m_pMixer.RawPtr();
    }

    //TPoolAllocator<SoundOneShot, 128>& GetOneShotPool()
    //{
    //    return m_OneShotPool;
    //}

    AudioListener const& GetListener() const
    {
        return m_Listener;
    }

    void Update(Actor_PlayerController* _Controller, float timeStep);

private:
    AudioModule();
    ~AudioModule();

    TRef<AudioDevice>                 m_pPlaybackDevice;
    TUniqueRef<AudioMixer>            m_pMixer;
//    TPoolAllocator<SoundOneShot, 128> m_OneShotPool;
    AudioListener                     m_Listener;
};

HK_NAMESPACE_END
