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

HK_NAMESPACE_BEGIN

enum class AudioTransferFormat : uint8_t
{
    INT16,
    FLOAT32
};

class AudioStream;

struct AudioStreamDesc
{
    AudioTransferFormat Format;
    int                 NumChannels;
    int                 SampleRate;
};

class AudioDevice final : public RefCounted
{
public:
                        AudioDevice();
                        ~AudioDevice();

    /// Playback frequency
    int                 GetSampleRate() const { return m_SampleRate; }

    int                 GetChannels() const { return m_Channels; }

    bool                IsMono() const { return m_Channels == 1; }

    bool                IsStereo() const { return m_Channels == 2; }

    /// Transfer buffer format (See AudioTransferFormat)
    AudioTransferFormat GetTransferFormat() const { return m_TransferFormat; }

    int                 GetTransferBufferSizeInFrames() const { return m_NumFrames; }

    int                 GetTransferBufferSizeInBytes() const { return m_TransferBufferSizeInBytes; }

    /// Pause device
    void                BlockSound();

    /// Resume device
    void                UnblockSound();

    /// Clear transfer buffer. It calls MapTransferBuffer() and UnmapTransferBuffer() internally.
    void                ClearBuffer();

    /// Lock transfer buffer for writing
    uint8_t*            MapTransferBuffer(int64_t* frameNum = nullptr);

    /// Submit changes and unlock the buffer.
    void                UnmapTransferBuffer();

    /// Pass MixerCallback for async mixing
    void                SetMixerCallback(std::function<void(uint8_t*, int, int, int)> MixerCallback);

    Ref<AudioStream>    CreateStream(AudioStreamDesc const& desc);

private:
    void                RenderAudio(uint8_t* pStream, int StreamLength);

    // Internal audio stream
    void*               m_AudioStream;
    // Internal device id
    uint32_t            m_DeviceID;
    // Transfer buffer format
    AudioTransferFormat m_TransferFormat;
    // Transfer buffer memory
    uint8_t*            m_TransferBuffer;
    // Transfer buffer size in bytes
    int                 m_TransferBufferSizeInBytes;
    // Transfer buffer size in frames * channels
    int                 m_Samples;
    // Transfer buffer size in frames
    int                 m_NumFrames;
    // Transfer buffer offset in samples
    int                 m_TransferOffset;
    // Transfer buffer previous offset in samples
    int                 m_PrevTransferOffset;
    // Wraps count
    int64_t             m_BufferWraps;
    // Playback frequency
    int                 m_SampleRate;
    // Channels (1 or 2)
    int                 m_Channels;
    // Callback for async mixing
    std::function<void(uint8_t*, int, int, int)> m_MixerCallback;
};

HK_NAMESPACE_END
