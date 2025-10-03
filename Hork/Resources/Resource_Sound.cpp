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

#include "Resource_Sound.h"

#include <Hork/Core/Logger.h>
#include <Hork/Audio/AudioDecoder.h>

HK_NAMESPACE_BEGIN

int SoundResource::s_DecoderSampleRate;
bool SoundResource::s_IsStereo;

void SoundResource::SetDecoderProperties(int sampleRate, bool stereo)
{
    s_DecoderSampleRate = sampleRate;
    s_IsStereo = stereo;
}

SoundResource::~SoundResource()
{}

UniqueRef<SoundResource> SoundResource::sLoad(IBinaryStreamReadInterface& stream)
{
    UniqueRef<SoundResource> resource = MakeUnique<SoundResource>();
    if (!resource->Read(stream))
        return {};
    return resource;
}

bool SoundResource::Read(IBinaryStreamReadInterface& stream)
{
    HK_ASSERT_(s_DecoderSampleRate != 0, "The audio decoder properties must be set! Use SoundResource::SetDecoderProperties");

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

    if (!cfg_encoded)
    {
        if (!DecodeAudio(stream, resample, m_Source))
        {
            LOG("Failed to decode audio {}\n", stream.GetName());
            return false;
        }
    }
    else
    {
        AudioFileInfo info;
        if (!ReadAudioInfo(stream, resample, &info))
        {
            LOG("Failed to read audio {}\n", stream.GetName());
            return false;
        }

        m_Source = MakeRef<AudioSource>(info.FrameCount, s_DecoderSampleRate, info.SampleBits, info.Channels, stream.AsBlob());
    }
    return true;
}

Ref<AudioSource> SoundResource::GetSource()
{
    return m_Source;
}

HK_NAMESPACE_END
