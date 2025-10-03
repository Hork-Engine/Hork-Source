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

#include "AudioDevice.h"
#include "AudioStream.h"

#include <Hork/Core/CoreApplication.h>

#include <SDL3/SDL.h>

HK_NAMESPACE_BEGIN

AudioDevice::AudioDevice()
{
    ApplicationArguments const& args = CoreApplication::sArgs();

    m_TransferBuffer = nullptr;

    const char* driver = NULL;
    int n = args.Find("-AudioDrv");
    if (n != -1 && n + 1 < args.Count())
    {
        driver = args.At(n + 1);
        SDL_setenv("SDL_AUDIO_DRIVER", driver, SDL_TRUE);
    }

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        CoreApplication::sTerminateWithError("Failed to init audio system: {}\n", SDL_GetError());

    int numdrivers = SDL_GetNumAudioDrivers();
    if (numdrivers > 0)
    {
        LOG("Available audio drivers:\n");
        for (int i = 0; i < numdrivers; i++)
            LOG("\t{}\n", SDL_GetAudioDriver(i));
    }

    int numdevs = 0;
    if (SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&numdevs))
    {
        LOG("Available audio devices:\n");
        for (int i = 0; i < numdevs; ++i)
        {
            SDL_AudioDeviceID instanceID = devices[i];
            LOG("\t{}\n", SDL_GetAudioDeviceName(instanceID));
        }
        SDL_free(devices);
    }

    SDL_AudioSpec spec = {};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = 48000;

    m_TransferFormat = AudioTransferFormat::FLOAT32;

    auto callback = [](void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
    {
        if (additional_amount > 0)
        {
            uint8_t *data = (uint8_t*)HkStackAlloc(additional_amount);
            if (data)
            {
                ((AudioDevice*)userdata)->RenderAudio(data, additional_amount);

                SDL_PutAudioStreamData(stream, data, additional_amount);
            }
        }
    };
    
    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, this);
    
    m_AudioStream = stream;
    if (!m_AudioStream)
        CoreApplication::sTerminateWithError("Failed to open audio device: {}\n", SDL_GetError());

    m_DeviceID = SDL_GetAudioStreamDevice(stream);

    SDL_AudioSpec deviceSpec;
    int sampleFrames;
    SDL_GetAudioDeviceFormat(m_DeviceID, &deviceSpec, &sampleFrames);

    if (spec.channels != deviceSpec.channels || spec.freq != deviceSpec.freq)
    {
        // Recreate audio stream with obtained format

        if (deviceSpec.format == SDL_AUDIO_S16)
        {
            spec.format = deviceSpec.format;
            m_TransferFormat = AudioTransferFormat::INT16;
        }

        spec.channels = deviceSpec.channels;
        spec.freq = deviceSpec.freq;

        SDL_DestroyAudioStream(stream);

        stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, this);

        m_AudioStream = stream;
        if (!m_AudioStream)
            CoreApplication::sTerminateWithError("Failed to open audio device: {}\n", SDL_GetError());

        m_DeviceID = SDL_GetAudioStreamDevice(stream);
    }

    m_SampleRate = spec.freq;
    m_Channels = spec.channels;
    m_Samples = Math::ToGreaterPowerOfTwo(sampleFrames * spec.channels * 10);
    m_NumFrames = m_Samples >> (m_Channels - 1);
    m_TransferBufferSizeInBytes = m_Samples * (m_TransferFormat == AudioTransferFormat::FLOAT32 ? sizeof(float) : sizeof(int16_t));
    m_TransferBuffer = (uint8_t*)Core::GetHeapAllocator<HEAP_AUDIO_DATA>().Alloc(m_TransferBufferSizeInBytes);
    Core::ZeroMem(m_TransferBuffer, m_TransferBufferSizeInBytes);
    m_TransferOffset = 0;
    m_PrevTransferOffset = 0;
    m_BufferWraps = 0;

    SDL_ResumeAudioDevice(m_DeviceID);

    const char* audioDriver = SDL_GetCurrentAudioDriver();
    const char* audioDevice = SDL_GetAudioDeviceName(m_DeviceID);

    LOG("Initialized audio : {} Hz, {} samples, {} channels\n", m_SampleRate, sampleFrames, m_Channels);
    LOG("Using audio driver: {}\n", audioDriver ? audioDriver : "Unknown");
    LOG("Using playback device: {}\n", audioDevice ? audioDevice : "Unknown");
    LOG("Audio buffer size: {} bytes\n", m_TransferBufferSizeInBytes);
}

AudioDevice::~AudioDevice()
{
    SDL_DestroyAudioStream((SDL_AudioStream*)m_AudioStream);

    Core::GetHeapAllocator<HEAP_AUDIO_DATA>().Free(m_TransferBuffer);
}

void AudioDevice::SetMixerCallback(std::function<void(uint8_t* transferBuffer, int transferBufferSizeInFrames, int FrameNum, int MinFramesToRender)> MixerCallback)
{
    SDL_LockAudioStream((SDL_AudioStream*)m_AudioStream);

    m_MixerCallback = MixerCallback;

    SDL_UnlockAudioStream((SDL_AudioStream*)m_AudioStream);
}

void AudioDevice::RenderAudio(uint8_t* stream, int streamLength)
{
    int sampleWidth = m_TransferFormat == AudioTransferFormat::FLOAT32 ? sizeof(float) : sizeof(int16_t);

    if (!m_TransferBuffer)
    {
        // Should never happen
        Core::ZeroMem(stream, streamLength);
        return;
    }

    if (m_MixerCallback)
    {
        if (m_TransferOffset < m_PrevTransferOffset)
        {
            m_BufferWraps++;
        }
        m_PrevTransferOffset = m_TransferOffset;

        int frameNum = m_BufferWraps * m_NumFrames + (m_TransferOffset >> (m_Channels - 1));

        m_MixerCallback(m_TransferBuffer, m_NumFrames, frameNum, streamLength / sampleWidth);
    }

    int offset = m_TransferOffset * sampleWidth;
    if (offset >= m_TransferBufferSizeInBytes)
    {
        m_TransferOffset = offset = 0;
    }

    int len1 = streamLength;
    int len2 = 0;

    if (offset + len1 > m_TransferBufferSizeInBytes)
    {
        len1 = m_TransferBufferSizeInBytes - offset;
        len2 = streamLength - len1;
    }

    Core::Memcpy(stream, m_TransferBuffer + offset, len1);

    if (len2 > 0)
    {
        Core::Memcpy(stream + len1, m_TransferBuffer, len2);
        m_TransferOffset = len2 / sampleWidth;
    }
    else
    {
        m_TransferOffset += len1 / sampleWidth;
    }
}

uint8_t* AudioDevice::MapTransferBuffer(int64_t* frameNum)
{
    SDL_LockAudioStream((SDL_AudioStream*)m_AudioStream);

    if (frameNum)
    {
        if (m_TransferOffset < m_PrevTransferOffset)
        {
            m_BufferWraps++;
        }
        m_PrevTransferOffset = m_TransferOffset;

        *frameNum = m_BufferWraps * m_NumFrames + (m_TransferOffset >> (m_Channels - 1));
    }

    return m_TransferBuffer;
}

void AudioDevice::UnmapTransferBuffer()
{
    SDL_UnlockAudioStream((SDL_AudioStream*)m_AudioStream);
}

void AudioDevice::BlockSound()
{
    SDL_PauseAudioDevice(m_DeviceID);
}

void AudioDevice::UnblockSound()
{
    SDL_ResumeAudioDevice(m_DeviceID);
}

void AudioDevice::ClearBuffer()
{
    MapTransferBuffer();
    Core::ZeroMem(m_TransferBuffer, m_TransferBufferSizeInBytes);
    UnmapTransferBuffer();
}

Ref<AudioStream> AudioDevice::CreateStream(AudioStreamDesc const& desc)
{
    const SDL_AudioSpec spec = {desc.Format == AudioTransferFormat::FLOAT32 ? SDL_AUDIO_F32 : SDL_AUDIO_S16, desc.NumChannels, desc.SampleRate};
    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream)
        return {};

    Ref<AudioStream> result = Ref<AudioStream>::sCreate(new AudioStream);
    result->m_AudioStream = stream;
    return result;
}

HK_NAMESPACE_END
