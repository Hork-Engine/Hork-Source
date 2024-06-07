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

#include "AudioDevice.h"

#include <Engine/Core/Platform.h>
#include <Engine/Core/Logger.h>
#include <Engine/Core/Memory.h>
#include <Engine/Core/BaseMath.h>

#include <SDL/SDL.h>

HK_NAMESPACE_BEGIN

AudioDevice::AudioDevice(int sampleRate)
{
    m_TransferBuffer = nullptr;

    const char* driver = NULL;

    ApplicationArguments const& args = CoreApplication::Args();

    int n = args.Find("-AudioDrv");
    if (n != -1 && n + 1 < args.Count())
    {
        driver = args.At(n + 1);
        SDL_setenv("SDL_AUDIODRIVER", driver, SDL_TRUE);
    }

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        CoreApplication::TerminateWithError("Failed to init audio system: {}\n", SDL_GetError());

    int numdrivers = SDL_GetNumAudioDrivers();
    if (numdrivers > 0)
    {
        LOG("Available audio drivers:\n");
        for (int i = 0; i < numdrivers; i++)
            LOG("\t{}\n", SDL_GetAudioDriver(i));
    }

    int numdevs = SDL_GetNumAudioDevices(SDL_FALSE);
    if (numdevs > 0)
    {
        LOG("Available audio devices:\n");
        for (int i = 0; i < numdevs; i++)
            LOG("\t{}\n", SDL_GetAudioDeviceName(i, SDL_FALSE));
    }

    SDL_AudioSpec desired = {};
    SDL_AudioSpec obtained = {};

    desired.freq = sampleRate;
    desired.channels = 2;

    // Choose audio buffer size in sample FRAMES (total samples divided by channel count)
    if (desired.freq <= 11025)
        desired.samples = 256;
    else if (desired.freq <= 22050)
        desired.samples = 512;
    else if (desired.freq <= 44100)
        desired.samples = 1024;
    else if (desired.freq <= 56000)
        desired.samples = 2048;
    else
        desired.samples = 4096;

    desired.callback = [](void* userdata, uint8_t* stream, int len)
    {
        ((AudioDevice*)userdata)->RenderAudio(stream, len);
    };
    desired.userdata = this;
    desired.format = AUDIO_F32SYS;

    int allowedChanges = 0;

    // Try to minimize format convertions
    allowedChanges |= SDL_AUDIO_ALLOW_FREQUENCY_CHANGE;
    allowedChanges |= SDL_AUDIO_ALLOW_FORMAT_CHANGE;
    allowedChanges |= SDL_AUDIO_ALLOW_CHANNELS_CHANGE;

    m_AudioDeviceId = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desired, &obtained, allowedChanges);
    if (m_AudioDeviceId == 0)
        CoreApplication::TerminateWithError("Failed to open audio device: {}\n", SDL_GetError());

    SDL_AudioFormat supportedFormats[] = {AUDIO_F32SYS, AUDIO_S16SYS, AUDIO_U8, AUDIO_S8};
    bool formatSupported = false;
    for (int i = 0; i < HK_ARRAY_SIZE(supportedFormats); i++)
    {
        if (obtained.format == supportedFormats[i])
        {
            formatSupported = true;
            break;
        }
    }

    if (!formatSupported)
    {
        SDL_CloseAudioDevice(m_AudioDeviceId);

        allowedChanges &= ~SDL_AUDIO_ALLOW_FORMAT_CHANGE;
        m_AudioDeviceId = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desired, &obtained, allowedChanges);
        if (m_AudioDeviceId == 0)
            CoreApplication::TerminateWithError("Failed to open audio device: {}\n", SDL_GetError());
    }

    m_SampleBits = obtained.format & 0xFF; // extract first byte which is sample bits
    m_bSigned8 = (obtained.format == AUDIO_S8);
    m_SampleRate = obtained.freq;
    m_Channels = obtained.channels;
    m_Samples = Math::ToGreaterPowerOfTwo(obtained.samples * obtained.channels * 10);
    m_NumFrames = m_Samples >> (m_Channels - 1);
    m_TransferBufferSizeInBytes = m_Samples * (m_SampleBits / 8);
    m_TransferBuffer = (uint8_t*)Core::GetHeapAllocator<HEAP_AUDIO_DATA>().Alloc(m_TransferBufferSizeInBytes);
    Core::Memset(m_TransferBuffer, m_SampleBits == 8 && !m_bSigned8 ? 0x80 : 0, m_TransferBufferSizeInBytes);
    m_TransferOffset = 0;
    m_PrevTransferOffset = 0;
    m_BufferWraps = 0;

    SDL_PauseAudioDevice(m_AudioDeviceId, 0);

    LOG("Initialized audio : {} Hz, {} samples, {} channels\n", m_SampleRate, obtained.samples, m_Channels);

    const char* audioDriver = SDL_GetCurrentAudioDriver();
    const char* audioDevice = SDL_GetAudioDeviceName(0, SDL_FALSE);

    LOG("Using audio driver: {}\n", audioDriver ? audioDriver : "Unknown");
    LOG("Using playback device: {}\n", audioDevice ? audioDevice : "Unknown");
    LOG("Audio buffer size: {} bytes\n", m_TransferBufferSizeInBytes);
}

AudioDevice::~AudioDevice()
{
    SDL_CloseAudioDevice(m_AudioDeviceId);

    Core::GetHeapAllocator<HEAP_AUDIO_DATA>().Free(m_TransferBuffer);
}

void AudioDevice::SetMixerCallback(std::function<void(uint8_t* transferBuffer, int transferBufferSizeInFrames, int FrameNum, int MinFramesToRender)> MixerCallback)
{
    SDL_LockAudioDevice(m_AudioDeviceId);

    m_MixerCallback = MixerCallback;

    SDL_UnlockAudioDevice(m_AudioDeviceId);
}

void AudioDevice::RenderAudio(uint8_t* pStream, int StreamLength)
{
    int sampleWidth = m_SampleBits / 8;

    if (!m_TransferBuffer)
    {
        // Should never happen
        Core::ZeroMem(pStream, StreamLength);
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

        m_MixerCallback(m_TransferBuffer, m_NumFrames, frameNum, StreamLength / sampleWidth);
    }

    int offset = m_TransferOffset * sampleWidth;
    if (offset >= m_TransferBufferSizeInBytes)
    {
        m_TransferOffset = offset = 0;
    }

    int len1 = StreamLength;
    int len2 = 0;

    if (offset + len1 > m_TransferBufferSizeInBytes)
    {
        len1 = m_TransferBufferSizeInBytes - offset;
        len2 = StreamLength - len1;
    }

    Core::Memcpy(pStream, m_TransferBuffer + offset, len1);
    //Core::ZeroMem( transferBuffer + offset, len1 );

    if (len2 > 0)
    {
        Core::Memcpy(pStream + len1, m_TransferBuffer, len2);
        m_TransferOffset = len2 / sampleWidth;
    }
    else
    {
        m_TransferOffset += len1 / sampleWidth;
    }
}

uint8_t* AudioDevice::MapTransferBuffer(int64_t* frameNum)
{
    SDL_LockAudioDevice(m_AudioDeviceId);

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
    SDL_UnlockAudioDevice(m_AudioDeviceId);
}

void AudioDevice::BlockSound()
{
    SDL_PauseAudioDevice(m_AudioDeviceId, 1);
}

void AudioDevice::UnblockSound()
{
    SDL_PauseAudioDevice(m_AudioDeviceId, 0);
}

void AudioDevice::ClearBuffer()
{
    MapTransferBuffer();
    Core::Memset(m_TransferBuffer, m_SampleBits == 8 && !m_bSigned8 ? 0x80 : 0, m_TransferBufferSizeInBytes);
    UnmapTransferBuffer();
}

HK_NAMESPACE_END
