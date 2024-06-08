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

#include "AudioStream.h"
#include <Engine/Core/CoreApplication.h>
#include <miniaudio/miniaudio.h>

HK_NAMESPACE_BEGIN

AudioStream::AudioStream(AudioSource* inSource) :
    m_Source(inSource)
{
    if (m_Source->IsEncoded())
    {
        ma_format format = ma_format_unknown;

        switch (m_Source->GetSampleBits())
        {
            case 8:
                format = ma_format_u8;
                break;
            case 16:
                format = ma_format_s16;
                break;
            case 32:
                format = ma_format_f32;
                break;
            default:
                // Shouldn't happen
                CoreApplication::TerminateWithError("AudioStream: expected 8, 16 or 32 sample bits\n");
        }

        m_Decoder = (ma_decoder*)Core::GetHeapAllocator<HEAP_AUDIO_DATA>().Alloc(sizeof(*m_Decoder));

        ma_decoder_config config = ma_decoder_config_init(format, m_Source->GetChannels(), m_Source->GetSampleRate());

        ma_result result = ma_decoder_init_memory(inSource->GetHeapPtr(), inSource->GetSizeInBytes(), &config, m_Decoder);
        if (result != MA_SUCCESS)
            CoreApplication::TerminateWithError("AudioStream: failed to initialize decoder\n");
    }
}

AudioStream::~AudioStream()
{
    if (m_Decoder)
    {
        ma_decoder_uninit(m_Decoder);
        Core::GetHeapAllocator<HEAP_AUDIO_DATA>().Free(m_Decoder);
    }
}

void AudioStream::SeekToFrame(int inFrameNum)
{
    m_FrameIndex = Math::Clamp(inFrameNum, 0, m_Source->GetFrameCount());
    if (m_Decoder)
        ma_decoder_seek_to_pcm_frame(m_Decoder, m_FrameIndex);
}

int AudioStream::ReadFrames(void* outFrames, int inFrameCount, size_t inSizeInBytes)
{
    if (inFrameCount <= 0)
        return 0;

    auto sample_stride = m_Source->GetSampleStride();
    if ((size_t)inFrameCount * sample_stride > inSizeInBytes)
        inFrameCount = inSizeInBytes / sample_stride;

    int frames_read;
    if (m_Decoder)
        frames_read = ma_decoder_read_pcm_frames(m_Decoder, outFrames, inFrameCount);
    else
    {
        frames_read = inFrameCount;
        if (m_FrameIndex + frames_read > m_Source->GetFrameCount())
            frames_read = m_Source->GetFrameCount() - m_FrameIndex;
        Core::Memcpy(outFrames, reinterpret_cast<const uint8_t*>(m_Source->GetFrames()) + m_FrameIndex * sample_stride, frames_read * sample_stride);
    }

    m_FrameIndex += frames_read;
    HK_ASSERT(m_FrameIndex <= m_Source->GetFrameCount());

    return frames_read;
}

HK_NAMESPACE_END
