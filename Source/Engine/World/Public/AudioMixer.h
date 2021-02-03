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

#include <Audio/AudioDevice.h>

#include <Core/Public/CoreMath.h>
#include <Core/Public/Ref.h>

#include <Runtime/Public/RuntimeVariable.h>

struct SAudioChannel
{
    /** Playback position in frames */
    int PlaybackPos;

    /** Playback end timestamp in frames */
    int64_t PlaybackEnd;

    /** Loop start in frames */
    int LoopStart;

    /** Repeats counter */
    int LoopsCount;

    /** Current playing volume */
    int CurVol[2];

    /** Desired volume */
    int NewVol[2];

    /** Direction from listener to audio source (for HRTF lookup) */
    Float3 CurDir;

    /** Desired direction from listener to audio source (for HRTF lookup) */
    Float3 NewDir;

    /** Channel is playing, but mixer skip the samples from this channel */
    bool bVirtual : 1;

    /** Channel is paused */
    bool bPaused : 1;

    /** If channel is has stereo samples, it will be combined to mono and spatialized for 3D */
    bool bSpatializedStereo : 1;

    /** Stream interface for partial audio streaming */
    class IAudioStream * StreamInterface;

    /** Audio data */
    const void * pRawSamples;

    /** Frame count */
    int FrameCount;

    /** Channels count */
    int Ch;

    /** Bits per sample */
    int SampleBits;

    /** Stride between frames in bytes */
    int SampleStride;

    SAudioChannel * Next;
    SAudioChannel * Prev;

    static SAudioChannel * Channels;
    static SAudioChannel * ChannelsTail;
};

class AAudioMixer
{
public:
    AAudioMixer( AAudioDevice * _Device );
    virtual ~AAudioMixer();

    int64_t GetRenderFrame() const
    {
        return RenderFrame;
    }

    /** Get current active channels */
    int GetNumActiveChannels() const
    {
        return NumActiveChannels;
    }

    /** Get number of not active (virtual) channels */
    int GetNumVirtualChannels() const
    {
        return NumVirtualChannels;
    }

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

    void RenderChannels( int64_t EndFrame );
    void RenderChannel( SAudioChannel * Chan, int64_t EndFrame );
    void RenderStream( SAudioChannel * Chan, int64_t EndFrame );
    void RenderFramesHRTF( SAudioChannel * Chan, int FrameCount, SSamplePair * pBuffer );
    void RenderFrames( SAudioChannel * Chan, const void * pFrames, int FrameCount, SSamplePair * pBuffer );
    void WriteToTransferBuffer( int const * pSamples, int64_t EndFrame );
    void MakeVolumeRamp( const int CurVol[2], const int NewVol[2], int FrameCount, int Scale );

    TUniqueRef< class AAudioHRTF > Hrtf;
    TUniqueRef< class AFreeverb > ReverbFilter;

    alignas(16) SSamplePair RenderBuffer[2048];
    const int RenderBufferSize = AN_ARRAY_SIZE( RenderBuffer );

    AAudioDevice * pDevice;
    uint8_t * pTransferBuffer;
    void * pTempFrames;
    int VolumeRampL[1024];
    int VolumeRampR[1024];
    int VolumeRampSize;
    int64_t RenderFrame;
    float * pFramesF32 = nullptr;
    SSamplePair * StreamF32 = nullptr;
    int NumActiveChannels;
    int NumVirtualChannels;
};

extern ARuntimeVariable Snd_HRTF;
