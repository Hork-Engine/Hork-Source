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

#include <Core/Ref.h>

HK_NAMESPACE_BEGIN

/** AudioBuffer
Designed as immutable structure, so we can use it from several threads. Owns the heap pointer. */
class AudioBuffer : public InterlockedRef
{
public:
    AudioBuffer(int frameCount, int channels, int sampleBits, void* frames)
    {
        m_Frames = frames;
        m_FrameCount = frameCount;
        m_Channels = channels;
        m_SampleBits = sampleBits;
        m_SampleStride = (m_SampleBits >> 3) << (m_Channels - 1);
    }

    ~AudioBuffer()
    {
        Platform::GetHeapAllocator<HEAP_AUDIO_DATA>().Free(m_Frames);
    }

    /** Audio data */
    HK_FORCEINLINE const void* GetFrames() const
    {
        return m_Frames;
    }

    /** Frame count */
    HK_FORCEINLINE int GetFrameCount() const
    {
        return m_FrameCount;
    }

    /** Channels count */
    HK_FORCEINLINE int GetChannels() const
    {
        return m_Channels;
    }

    /** Bits per sample */
    HK_FORCEINLINE int GetSampleBits() const
    {
        return m_SampleBits;
    }

    /** Stride between frames in bytes */
    HK_FORCEINLINE int GetSampleStride() const
    {
        return m_SampleStride;
    }

private:
    /** Audio data */
    void* m_Frames;

    /** Frame count */
    int m_FrameCount;

    /** Channels count */
    int m_Channels;

    /** Bits per sample */
    int m_SampleBits;

    /** Stride between frames in bytes */
    int m_SampleStride;
};

HK_NAMESPACE_END
