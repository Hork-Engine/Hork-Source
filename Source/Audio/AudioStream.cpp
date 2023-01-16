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

#include "AudioStream.h"

#include <Platform/Platform.h>
#include <Core/BaseMath.h>

#include <miniaudio/miniaudio.h>

HK_NAMESPACE_BEGIN

AudioStream::AudioStream(FileInMemory* pFileInMemory, int FrameCount, int SampleRate, int SampleBits, int Channels)
{
    ma_format format = ma_format_unknown;

    switch (SampleBits)
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
            CriticalError("AudioStream::ctor: expected 8, 16 or 32 sample bits\n");
    }

    m_Decoder = (ma_decoder*)Platform::GetHeapAllocator<HEAP_AUDIO_DATA>().Alloc(sizeof(*m_Decoder));

    ma_decoder_config config = ma_decoder_config_init(format, Channels, SampleRate);

    ma_result result = ma_decoder_init_memory(pFileInMemory->GetHeapPtr(), pFileInMemory->GetSizeInBytes(), &config, m_Decoder);
    if (result != MA_SUCCESS)
    {
        CriticalError("AudioStream::ctor: failed to initialize decoder\n");
    }

    // Capture the reference
    m_pFileInMemory = pFileInMemory;

    m_FrameCount = FrameCount;
    m_Channels = Channels;
    m_SampleBits = SampleBits;
    m_SampleStride = (SampleBits >> 3) << (Channels - 1);
}

AudioStream::~AudioStream()
{
    ma_decoder_uninit(m_Decoder);
    Platform::GetHeapAllocator<HEAP_AUDIO_DATA>().Free(m_Decoder);
}

void AudioStream::SeekToFrame(int FrameNum)
{
    ma_decoder_seek_to_pcm_frame(m_Decoder, Math::Max(0, FrameNum));
}

//static const int ma_format_sample_bits[] =
//{
//    0,  // ma_format_unknown
//    8,  // ma_format_u8
//    16, // ma_format_s16
//    24, // ma_format_s24
//    32, // ma_format_s32
//    32, // ma_format_f32
//};

int AudioStream::ReadFrames(void* pFrames, int FrameCount, size_t SizeInBytes)
{
    if (FrameCount <= 0)
    {
        return 0;
    }

    //const int stride = ( ma_format_sample_bits[m_Decoder->outputFormat] >> 3 ) << ( m_Decoder->outputChannels - 1 );

    if ((size_t)FrameCount * m_SampleStride > SizeInBytes)
    {
        FrameCount = SizeInBytes / m_SampleStride;
    }

    return ma_decoder_read_pcm_frames(m_Decoder, pFrames, FrameCount);
}

HK_NAMESPACE_END
