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

#include <Core/Public/BaseTypes.h>

class AAudioDevice
{
public:
    AAudioDevice( int InSampleRate );
    ~AAudioDevice();

    /** Playback frequency */
    int GetSampleRate() const
    {
        return SampleRate;
    }

    /** Bits per sample (8, 16 or 32) */
    int GetSampleBits() const
    {
        return SampleBits;
    }

    /** Sample size in bytes */
    int GetSampleWidth() const
    {
        return SampleBits >> 3;
    }

    bool IsSigned8Bit() const
    {
        return bSigned8;
    }

    int GetChannels() const
    {
        return Channels;
    }

    bool IsMono() const
    {
        return Channels == 1;
    }

    bool IsStereo() const
    {
        return Channels == 2;
    }

    int GetTransferBufferSizeInFrames() const
    {
        return NumFrames;
    }

    int GetTransferBufferSizeInBytes() const
    {
        return TransferBufferSizeInBytes;
    }

    void BlockSound();

    void UnblockSound();

    /** Clear transfer buffer. It calls MapTransferBuffer() and UnmapTransferBuffer() internally. */
    void ClearBuffer();

    /** Lock transfer buffer for writing */
    uint8_t * MapTransferBuffer( int64_t * pFrameNum = nullptr );

    /** Submit changes and unlock the buffer. */
    void UnmapTransferBuffer();

private:
    void RenderAudio( uint8_t * pStream, int StreamLength );

    // Internal device id
    uint32_t AudioDeviceId;
    // Transfer buffer memory
    uint8_t * pTransferBuffer;
    // Transfer buffer size in bytes
    int TransferBufferSizeInBytes;
    // Transfer buffer size in frames * channels
    int Samples;
    // Transfer buffer size in frames
    int NumFrames;
    // Transfer buffer offset in samples
    int TransferOffset;
    // Transfer buffer previous offset in samples
    int PrevTransferOffset;
    // Wraps count
    int64_t BufferWraps;
    // Playback frequency
    int SampleRate;
    // Bits per sample (8 or 16)
    int SampleBits;
    // Channels (1 or 2)
    int Channels;
    // Is signed 8bit audio (desired is unsigned)
    bool bSigned8;
};
