/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "Sound.h"

#include <Hork/Audio/AudioDecoder.h>

HK_NAMESPACE_BEGIN

int Sound::s_DecoderSampleRate;
bool Sound::s_IsStereo;

void Sound::SetDecoderProperties(int sampleRate, bool stereo)
{
    s_DecoderSampleRate = sampleRate;
    s_IsStereo = stereo;
}

UniqueRef<SoundData> Sound::BeginAsyncLoad(IBinaryStreamReadInterface& stream)
{
    HK_ASSERT_(s_DecoderSampleRate != 0, "The audio decoder properties must be set! Use Sound::SetDecoderProperties");

    // TODO: Audio config file:
    // {
    //      AudioFiles
    //      [
    //          {
    //              Name "/path/to/file"
    //              Force8Bit "false"
    //              ForceMono "false"
    //              Encoded "false"
    //          }
    //      ]
    // }
    //
    // Or:
    // "/path/to/file" Force8Bit ForceMono Encoded

    bool cfg_force_8bit = false;
    bool cfg_force_mono = false;
    bool cfg_encoded = false;

    AudioResample resample;
    resample.SampleRate = s_DecoderSampleRate;
    resample.bForceMono = cfg_force_mono || !s_IsStereo;
    resample.bForce8Bit = cfg_force_8bit;

    Ref<AudioSource> source;

    if (!cfg_encoded)
    {
        if (!DecodeAudio(stream, resample, source))
        {
            LOG("Failed to decode audio {}\n", stream.GetName());
            return {};
        }
    }
    else
    {
        AudioFileInfo info;
        if (!ReadAudioInfo(stream, resample, &info))
        {
            LOG("Failed to read audio {}\n", stream.GetName());
            return {};
        }

        source = MakeRef<AudioSource>(info.FrameCount, s_DecoderSampleRate, info.SampleBits, info.Channels, stream.AsBlob());
    }

    UniqueRef<SoundData> data = MakeUnique<SoundData>();
    data->Source = std::move(source);

    return data;
}

void Sound::InitFromData(UniqueRef<SoundData> data)
{
    if (!data)
        return;

    m_Source = std::move(data->Source);

    m_IsPurged = false;
}

void Sound::Load(IBinaryStreamReadInterface& stream)
{
    auto tempData = BeginAsyncLoad(stream);
    if (tempData)
        InitFromData(std::move(tempData));
}

void Sound::Write(IBinaryStreamWriteInterface& stream)
{
    // TODO
}

void Sound::Purge()
{
    m_Source.Reset();
   
    m_IsPurged = true;
}

Ref<AudioSource> Sound::GetSource()
{
    return m_Source;
}

HK_NAMESPACE_END
