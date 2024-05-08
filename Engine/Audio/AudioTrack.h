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

#include "AudioStream.h"

#include <Engine/Math/VectorMath.h>
#include <Engine/Core/Allocators/PoolAllocator.h>

HK_NAMESPACE_BEGIN

/**
AudioTrack

All members can be freely modified before submit to mixer thread
All ***_LOCK members are protected by spinlock
*/
struct AudioTrack final
{
    HK_FORBID_COPY(AudioTrack)

    /// Audio source. Read only
    TRef<AudioSource> pSource;

    /// Stream interface for partial audio streaming. Read only
    TRef<AudioStream> pStream;

    /// Playback position in frames.
    /// Read only for main thread. Modified by mixer thread.
    /// To change playback position from main thread PlaybackPos_LOCK is used.
    AtomicInt PlaybackPos;

    /// PlaybackPos_LOCK is used to change current playback position.
    /// Value is valid if PlaybackPos_LOCK greater or equal to zero.
    int PlaybackPos_LOCK;

    /// Playback end timestamp in frames.
    /// Only used by mixer thread (RW).
    int64_t PlaybackEnd;

    /// Loop start in frames. Read only
    int LoopStart;

    /// Repeats counter.
    /// Only used by mixer thread (RW).
    int LoopsCount;

    /// Current playing volume.
    /// Only used by mixer thread (RW).
    int Volume[2];

    /// Volume_LOCK is used to change track volume
    int Volume_LOCK[2];

    /// Direction from listener to audio source (for HRTF lookup).
    /// Only used by mixer thread (RW).
    Float3 LocalDir;

    /// LocalDir_LOCK is used to change current relative to listener direction
    Float3 LocalDir_LOCK;

    /// Should mixer virtualize the channel or stop playing. Read only
    bool bVirtualizeWhenSilent : 1;

    /// Track is playing, but mixer skip the samples from this track.
    /// Only used by mixer thread (RW).
    bool bVirtual : 1;

    /// Track is paused
    bool bPaused_LOCK : 1;

    /// If track is has stereo samples, it will be combined to mono and spatialized for 3D
    bool bSpatializedStereo_LOCK : 1;

    /// The stop signal. It's setted by mixer thread. If it's true, main thread should reject to use this track and remove it.
    AtomicBool Stopped;

    /// Reference counter
    AtomicInt RefCount;

    /// Track iterator. Used by mixer thread.
    AudioTrack* Next;
    /// Track iterator. Used by mixer thread.
    AudioTrack* Prev;

    SpinLock Lock;

    /// Frame count. Read only.
    int FrameCount;

    /// Track count. Read only.
    int Channels;

    /// Bits per sample. Read only.
    int SampleBits;

    /// Stride between frames in bytes. Read only.
    int SampleStride;

    /// Audio data. Just a wrapper to simplify access to audio buffer.
    /// For encoded audio returns nullptr.
    HK_FORCEINLINE const void* GetFrames() const
    {
        return pSource->GetFrames();
    }

    HK_FORCEINLINE int GetLoopStart() const
    {
        return LoopStart;
    }

    HK_FORCEINLINE int GetPlaybackPos() const
    {
        return PlaybackPos.Load();
    }

    HK_FORCEINLINE bool IsStopped() const
    {
        return Stopped.Load();
    }

public:
    void* operator new(size_t _SizeInBytes)
    {
        MutexGuard lockGuard(PoolMutex);
        return TrackPool.Allocate();
    }

    void operator delete(void* _Ptr)
    {
        MutexGuard lockGuard(PoolMutex);
        TrackPool.Deallocate(_Ptr);
    }

    AudioTrack(AudioSource* inSource, int inStartFrame, int inLoopStart, int inLoopsCount, bool inVirtualizeWhenSilent);

    /// Update parameters. Called from main thread.
    void SetPlaybackParameters(const int inVolume[2], Float3 const& inLocalDir, bool inSpatializedStereo, bool inPaused);

    /// Change playback position. Called from main thread.
    void SetPlaybackPosition(int inPosition);

    /// Add reference. Can be used from both main and mixer threads.
    HK_FORCEINLINE void AddRef()
    {
        RefCount.Increment();
    }

    /// Remove reference. Can be used from both main and mixer threads.
    HK_FORCEINLINE void RemoveRef()
    {
        if (RefCount.Decrement() == 0)
        {
            delete this;
        }
    }

    /// Reference count
    HK_FORCEINLINE int GetRefCount() const
    {
        return RefCount.Load();
    }

    /// This function is called by a mixer at shutdown to cleanup the memory
    static void FreePool();

private:
    static TPoolAllocator<AudioTrack> TrackPool;
    static Mutex PoolMutex;
};

HK_NAMESPACE_END
