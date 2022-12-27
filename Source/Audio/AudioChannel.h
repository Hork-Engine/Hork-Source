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

#pragma once

#include "AudioBuffer.h"
#include "AudioStream.h"

#include <Geometry/VectorMath.h>
#include <Platform/Memory/PoolAllocator.h>

HK_NAMESPACE_BEGIN

/**
AudioChannel

All members can be freely modified before submit to mixer thread
All ***_COMMIT members are protected by spinlock
*/
struct AudioChannel final
{
    HK_FORBID_COPY(AudioChannel)

    /** Audio buffer. Read only */
    AudioBuffer* pBuffer;

    /** Stream interface for partial audio streaming. Read only */
    AudioStream* pStream;

    /** Playback position in frames.
    Read only for main thread. Modified by mixer thread.
    To change playback position from main thread PlaybackPos_COMMIT is used. */
    AtomicInt PlaybackPos;

    /** PlaybackPos_COMMIT is used to change current playback position.
    Value is valid if PlaybackPos_COMMIT greater or equal to zero. */
    int PlaybackPos_COMMIT;

    /** Playback end timestamp in frames.
    Only used by mixer thread (RW). */
    int64_t PlaybackEnd;

    /** Loop start in frames. Read only */
    int LoopStart;

    /** Repeats counter.
    Only used by mixer thread (RW). */
    int LoopsCount;

    /** Current playing volume.
    Only used by mixer thread (RW). */
    int Volume[2];

    /** Volume_COMMIT is used to change current channel volume */
    int Volume_COMMIT[2];

    /** Direction from listener to audio source (for HRTF lookup).
    Only used by mixer thread (RW). */
    Float3 LocalDir;

    /** LocalDir_COMMIT is used to change current relative to listener direction */
    Float3 LocalDir_COMMIT;

    /** Should mixer virtualize the channel or stop playing. Read only */
    bool bVirtualizeWhenSilent : 1;

    /** Channel is playing, but mixer skip the samples from this channel.
    Only used by mixer thread (RW). */
    bool bVirtual : 1;

    /** Channel is paused */
    bool bPaused_COMMIT : 1;

    /** If channel is has stereo samples, it will be combined to mono and spatialized for 3D */
    bool bSpatializedStereo_COMMIT : 1;

    /** The stop signal. It's setted by mixer thread. If it's true, main thread should reject to use this channel and remove it. */
    AtomicBool Stopped;

    /** Reference counter */
    AtomicInt RefCount;

    /** Channel iterator. Used by mixer thread. */
    AudioChannel* Next;
    /** Channel iterator. Used by mixer thread. */
    AudioChannel* Prev;

    SpinLock SpinLock;

    /** Frame count. Read only. */
    int FrameCount;

    /** Channels count. Read only. */
    int Channels;

    /** Bits per sample. Read only. */
    int SampleBits;

    /** Stride between frames in bytes. Read only. */
    int SampleStride;

    /** Audio data. Just a wrapper to simplify access to audio buffer */
    HK_FORCEINLINE const void* GetFrames() const
    {
        return pBuffer->GetFrames();
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
        MutexGurad lockGuard(PoolMutex);
        return ChannelPool.Allocate();
    }

    void operator delete(void* _Ptr)
    {
        MutexGurad lockGuard(PoolMutex);
        ChannelPool.Deallocate(_Ptr);
    }

    AudioChannel(int _StartFrame,
                 int _LoopStart,
                 int _LoopsCount,
                 AudioBuffer* _pBuffer,
                 AudioStream* _pStream,
                 bool _bVirtualizeWhenSilent,
                 const int _Volume[2],
                 Float3 const& _LocalDir,
                 bool _bSpatializedStereo,
                 bool _bPaused);

    ~AudioChannel();

    /** Commit spatial data. Called from main thread. */
    void Commit(int _Volume[2], Float3 const& _LocalDir, bool _bSpatializedStereo, bool _bPaused);

    /** Commit playback position. Called from main thread. */
    void ChangePlaybackPosition(int _PlaybackPos);

    /** Add reference. Can be used from both main and mixer threads. */
    HK_FORCEINLINE void AddRef()
    {
        RefCount.Increment();
    }

    /** Remove reference. Can be used from both main and mixer threads. */
    HK_FORCEINLINE void RemoveRef()
    {
        if (RefCount.Decrement() == 0)
        {
            delete this;
        }
    }

    /** Reference count */
    HK_FORCEINLINE int GetRefCount() const
    {
        return RefCount.Load();
    }

    /** This function is called by a mixer at shutdown to cleanup the memory */
    static void FreePool();

private:
    static TPoolAllocator<AudioChannel> ChannelPool;
    static Mutex PoolMutex;
};

HK_NAMESPACE_END
