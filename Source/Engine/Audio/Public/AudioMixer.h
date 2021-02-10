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

#pragma once

#include "AudioDevice.h"
#include "AudioChannel.h"

#include <Core/Public/PodArray.h>

#include <Runtime/Public/RuntimeVariable.h>

class AAudioMixer
{
    AN_FORBID_COPY( AAudioMixer )

public:
    AAudioMixer( AAudioDevice * _Device );
    virtual ~AAudioMixer();

    /** Make channel visible for mixer thread */
    void SubmitChannel( SAudioChannel * Channel );

    /** Get current active channels */
    int GetNumActiveChannels() const
    {
        return NumActiveChannels.Load();
    }

    /** Get number of not active (virtual) channels */
    int GetNumVirtualChannels() const
    {
        return TotalChannels.Load() - NumActiveChannels.Load();
    }

    /** Get total count of channels */
    int GetTotalChannels() const
    {
        return TotalChannels.Load();
    }

    /** Start async mixing */
    void StartAsync();

    /** Stop async mixing */
    void StopAsync();

    /** Perform mixing in main thread */
    void Update();

private:
    struct SSamplePair
    {
        union
        {
            int32_t Chan[2];
            float Chanf[2];
        };
    };

    void UpdateAsync( uint8_t * pTransferBuffer, int TransferBufferSizeInFrames, int FrameNum, int MinFramesToRender );

    // This fuction adds pending channels to list
    void AddPendingChannels();
    void RejectChannel( SAudioChannel * Channel );
    void RenderChannels( int64_t EndFrame );
    void RenderChannel( SAudioChannel * Chan, int64_t EndFrame );
    void RenderStream( SAudioChannel * Chan, int64_t EndFrame );
    void RenderFramesHRTF( SAudioChannel * Chan, int FrameCount, SSamplePair * pBuffer );
    void RenderFrames( SAudioChannel * Chan, const void * pFrames, int FrameCount, SSamplePair * pBuffer );
    void WriteToTransferBuffer( int const * pSamples, int64_t EndFrame );
    void MakeVolumeRamp( const int CurVol[2], const int NewVol[2], int FrameCount, int Scale );
    void ReadFramesF32( SAudioChannel * Chan, int FramesToRead, int HistoryExtraFrames, float * pFrames );

    TUniqueRef< class AAudioHRTF > Hrtf;
    TUniqueRef< class AFreeverb > ReverbFilter;

    alignas(16) SSamplePair RenderBuffer[2048];
    const int RenderBufferSize = AN_ARRAY_SIZE( RenderBuffer );

    AAudioDevice * pDevice;
    uint8_t * pTransferBuffer;
    bool bAsync;
    int64_t RenderFrame;
    AAtomicInt NumActiveChannels;
    AAtomicInt TotalChannels;

    SAudioChannel * Channels;
    SAudioChannel * ChannelsTail;
    SAudioChannel * PendingList;
    SAudioChannel * PendingListTail;

    ASpinLock SubmitLock;

    // For current mixing channel
    int NewVol[2];
    Float3 NewDir;
    bool bSpatializedChannel;
    bool bChannelPaused;
    int PlaybackPos;
    int VolumeRampL[1024];
    int VolumeRampR[1024];
    int VolumeRampSize;

    TPodArrayHeap< uint8_t > TempFrames;
    TPodArrayHeap< float > FramesF32;
    TPodArrayHeap< SSamplePair > StreamF32;
};

extern ARuntimeVariable Snd_HRTF;
