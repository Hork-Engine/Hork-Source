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

#include "AudioTrack.h"

HK_NAMESPACE_BEGIN

PoolAllocator<AudioTrack> AudioTrack::TrackPool;
Mutex AudioTrack::PoolMutex;

AudioTrack::AudioTrack(AudioSource* inSource, int inStartFrame, int inLoopStart, int inLoopsCount, bool inVirtualizeWhenSilent) :
    Stopped(false), RefCount(1)
{
    pSource = inSource;
    if (inSource->IsEncoded())
        pDecoder = MakeRef<AudioDecoder>(inSource);

    FrameCount = inSource->GetFrameCount();
    Channels = inSource->GetChannels();
    SampleBits = inSource->GetSampleBits();
    SampleStride = inSource->GetSampleStride();
    PlaybackPos.StoreRelaxed(inStartFrame);
    PlaybackPos_LOCK = -1;
    PlaybackEnd = 0;
    LoopStart = inLoopStart;
    LoopsCount = inLoopsCount;
    Volume[0] = 0;
    Volume[1] = 0;
    Volume_LOCK[0] = 0;
    Volume_LOCK[1] = 0;
    bVirtualizeWhenSilent = inVirtualizeWhenSilent;
    bVirtual = false;
    bPaused_LOCK = false;
    bSpatializedStereo_LOCK = false;
    Next = nullptr;
    Prev = nullptr;
}

void AudioTrack::SetPlaybackParameters(const int inVolume[2], Float3 const& inLocalDir, bool inSpatializedStereo, bool inPaused)
{
    SpinLockGuard guard(Lock);
    Volume_LOCK[0] = inVolume[0];
    Volume_LOCK[1] = inVolume[1];
    LocalDir_LOCK = inLocalDir;
    bSpatializedStereo_LOCK = inSpatializedStereo;
    bPaused_LOCK = inPaused;
}

void AudioTrack::SetPlaybackPosition(int inPosition)
{
    SpinLockGuard guard(Lock);
    PlaybackPos_LOCK = inPosition;
}

void AudioTrack::FreePool()
{
    TrackPool.Free();
}

HK_NAMESPACE_END
