/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Audio/AudioChannel.h>

TPoolAllocator< SAudioChannel > SAudioChannel::ChannelPool;
AMutex SAudioChannel::PoolMutex;

SAudioChannel::SAudioChannel( int _StartFrame,
                              int _LoopStart,
                              int _LoopsCount,
                              SAudioBuffer * _pBuffer,
                              SAudioStream * _pStream,
                              bool _bVirtualizeWhenSilent,
                              const int _Volume[2],
                              Float3 const & _LocalDir,
                              bool _bSpatializedStereo,
                              bool _bPaused )
    : RefCount( 1 )
    , Stopped( false )
{
    pBuffer = nullptr;
    pStream = nullptr;

    if ( _pStream ) {
        pStream = _pStream;
        pStream->AddRef();

        FrameCount = pStream->GetFrameCount();
        Channels = pStream->GetChannels();
        SampleBits = pStream->GetSampleBits();
        SampleStride = pStream->GetSampleStride();
    } else if ( _pBuffer ) {
        pBuffer = _pBuffer;
        pBuffer->AddRef();

        FrameCount = pBuffer->GetFrameCount();
        Channels = pBuffer->GetChannels();
        SampleBits = pBuffer->GetSampleBits();
        SampleStride = pBuffer->GetSampleStride();
    }

    AN_ASSERT( pBuffer || pStream );

    PlaybackPos.StoreRelaxed( _StartFrame );
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

SAudioChannel::~SAudioChannel()
{
    if ( pBuffer ) {
        pBuffer->RemoveRef();
    }

    if ( pStream ) {
        pStream->RemoveRef();
    }
}

void SAudioChannel::Commit( int _Volume[2], Float3 const & _LocalDir, bool _bSpatializedStereo, bool _bPaused )
{
    ASpinLockGuard guard( SpinLock );
    Volume_COMMIT[0] = _Volume[0];
    Volume_COMMIT[1] = _Volume[1];
    LocalDir_COMMIT = _LocalDir;
    bSpatializedStereo_COMMIT = _bSpatializedStereo;
    bPaused_COMMIT = _bPaused;
}

void SAudioChannel::ChangePlaybackPosition( int _PlaybackPos )
{
    ASpinLockGuard guard( SpinLock );
    PlaybackPos_COMMIT = _PlaybackPos;
}

void SAudioChannel::FreePool()
{
    ChannelPool.Free();
}
