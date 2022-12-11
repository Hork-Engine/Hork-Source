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

#include "AudioChannel.h"

TPoolAllocator<AudioChannel> AudioChannel::ChannelPool;
Mutex AudioChannel::PoolMutex;

AudioChannel::AudioChannel(int _StartFrame,
                           int _LoopStart,
                           int _LoopsCount,
                           AudioBuffer* _pBuffer,
                           AudioStream* _pStream,
                           bool _bVirtualizeWhenSilent,
                           const int _Volume[2],
                           Float3 const& _LocalDir,
                           bool _bSpatializedStereo,
                           bool _bPaused) :
    Stopped(false), RefCount(1)
{
    pBuffer = nullptr;
    pStream = nullptr;

    if (_pStream)
    {
        pStream = _pStream;
        pStream->AddRef();

        FrameCount = pStream->GetFrameCount();
        Channels = pStream->GetChannels();
        SampleBits = pStream->GetSampleBits();
        SampleStride = pStream->GetSampleStride();
    }
    else if (_pBuffer)
    {
        pBuffer = _pBuffer;
        pBuffer->AddRef();

        FrameCount = pBuffer->GetFrameCount();
        Channels = pBuffer->GetChannels();
        SampleBits = pBuffer->GetSampleBits();
        SampleStride = pBuffer->GetSampleStride();
    }

    HK_ASSERT(pBuffer || pStream);

    PlaybackPos.StoreRelaxed(_StartFrame);
    PlaybackPos_COMMIT = -1;
    PlaybackEnd = 0;
    LoopStart = _LoopStart;
    LoopsCount = _LoopsCount;
    Volume[0] = _Volume[0];
    Volume[1] = _Volume[1];
    Volume_COMMIT[0] = _Volume[0];
    Volume_COMMIT[1] = _Volume[1];
    LocalDir = _LocalDir;
    LocalDir_COMMIT = _LocalDir;
    bVirtualizeWhenSilent = _bVirtualizeWhenSilent;
    bVirtual = _Volume[0] == 0 && _Volume[1] == 0;
    bPaused_COMMIT = _bPaused;
    bSpatializedStereo_COMMIT = _bSpatializedStereo;
    Next = nullptr;
    Prev = nullptr;
}

AudioChannel::~AudioChannel()
{
    if (pBuffer)
    {
        pBuffer->RemoveRef();
    }

    if (pStream)
    {
        pStream->RemoveRef();
    }
}

void AudioChannel::Commit(int _Volume[2], Float3 const& _LocalDir, bool _bSpatializedStereo, bool _bPaused)
{
    SpinLockGuard guard(SpinLock);
    Volume_COMMIT[0] = _Volume[0];
    Volume_COMMIT[1] = _Volume[1];
    LocalDir_COMMIT = _LocalDir;
    bSpatializedStereo_COMMIT = _bSpatializedStereo;
    bPaused_COMMIT = _bPaused;
}

void AudioChannel::ChangePlaybackPosition(int _PlaybackPos)
{
    SpinLockGuard guard(SpinLock);
    PlaybackPos_COMMIT = _PlaybackPos;
}

void AudioChannel::FreePool()
{
    ChannelPool.Free();
}
