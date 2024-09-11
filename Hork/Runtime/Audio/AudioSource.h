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

#pragma once

#include <Hork/Core/Ref.h>
#include <Hork/Core/HeapBlob.h>
#include <Hork/Core/BinaryStream.h>

HK_NAMESPACE_BEGIN

class AudioSource final : public InterlockedRef
{
public:
                                AudioSource(int inFrameCount, int inSampleRate, int inSampleBits, int inChannels, HeapBlob blob);
                                AudioSource(int inFrameCount, int inSampleRate, int inSampleBits, int inChannels, const void* inFrames);

    const void*                 GetHeapPtr() const { return m_Blob.GetData(); }

    size_t                      GetSizeInBytes() const { return m_Blob.Size(); }

    /// Audio data
    HK_FORCEINLINE const void*  GetFrames() const { return m_Frames; }

    /// Frame count
    HK_FORCEINLINE int          GetFrameCount() const { return m_FrameCount; }

    /// Channels count
    HK_FORCEINLINE int          GetChannels() const { return m_Channels; }

    /// Is mono
    HK_FORCEINLINE bool         IsMono() const { return m_Channels == 1; }

    /// Is stereo
    HK_FORCEINLINE bool         IsStereo() const { return m_Channels == 2; }

    /// Bits per sample
    HK_FORCEINLINE int          GetSampleBits() const { return m_SampleBits; }

    /// Stride between frames in bytes
    HK_FORCEINLINE int          GetSampleStride() const { return m_SampleStride; }

    HK_FORCEINLINE int          GetSampleRate() const { return m_SampleRate; }

    HK_FORCEINLINE float        GetDurationInSeconds() const { return static_cast<float>(m_FrameCount) / m_SampleRate; }

    HK_FORCEINLINE bool         IsEncoded() const { return m_bIsEncoded; }

private:
    HeapBlob                    m_Blob;

    bool                        m_bIsEncoded;

    /// Pointer to first frame
    void*                       m_Frames;

    /// Frame count
    int                         m_FrameCount;

    /// Channels count
    int                         m_Channels;

    /// Bits per sample
    int                         m_SampleBits;

    /// Stride between frames in bytes
    int                         m_SampleStride;

    int                         m_SampleRate;
};

struct AudioFileInfo
{
    int FrameCount;
    int Channels;
    int SampleBits;
};

struct AudioResample
{
    int  SampleRate;
    bool bForceMono;
    bool bForce8Bit;
};

bool DecodeAudio(IBinaryStreamReadInterface& inStream, AudioResample const& inResample, Ref<AudioSource>& outSource);
bool ReadAudioInfo(IBinaryStreamReadInterface& inStream, AudioResample const& inResample, AudioFileInfo* outInfo);

HK_NAMESPACE_END
