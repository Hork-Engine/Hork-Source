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

#include "Cinematic.h"

#include <Hork/Core/Logger.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>
#include <Hork/Runtime/Audio/AudioDevice.h>

#define PLM_MALLOC(sz) Hk::Core::GetHeapAllocator<Hk::HEAP_IMAGE>().Alloc(sz, 0)
#define PLM_FREE(p) Hk::Core::GetHeapAllocator<Hk::HEAP_IMAGE>().Free(p)
#define PLM_REALLOC(p, sz) Hk::Core::GetHeapAllocator<Hk::HEAP_IMAGE>().Realloc(p, sz, 0)

extern "C"
{

struct plm_io_callbacks
{
    void (*seek)(void* stream, long offset, int origin);
    long (*tell)(void* stream);
    size_t (*read)(void* data, size_t numBytes, void* stream);
};

}

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg/pl_mpeg.h>

extern "C"
{
    plm_buffer_t *plm_buffer_create_with_callbacks(plm_io_callbacks const* callbacks, void* stream)
    {
        plm_buffer_t *self = plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
        self->fh = stream;
        self->io = *callbacks;
        self->mode = PLM_BUFFER_MODE_FILE;
        self->discard_read_bytes = TRUE;

        self->io.seek(self->fh, 0, SEEK_END);
        self->total_size = self->io.tell(self->fh);
        self->io.seek(self->fh, 0, SEEK_SET);

        plm_buffer_set_load_callback(self, plm_buffer_load_file_callback, NULL);
        return self;
    }

    plm_t* plm_create_with_callbacks(plm_io_callbacks const* callbacks, void* stream)
    {
        plm_buffer_t *buffer = plm_buffer_create_with_callbacks(callbacks, stream);
        if (!buffer) {
            return NULL;
        }
        return plm_create_with_buffer(buffer, TRUE);
    }
}

HK_NAMESPACE_BEGIN

struct Cinematic::Frame
{
    plm_frame_t*  data;
};

Cinematic::Cinematic(StringView resourceName)
{
    m_Texture = GameApplication::sGetResourceManager().CreateResource<TextureResource>(resourceName);
}

Cinematic::~Cinematic()
{
    Close();
}

bool Cinematic::Open(StringView filename, CinematicFlags flags)
{
    Close();

    m_File = GameApplication::sGetResourceManager().OpenFile(filename);
    if (!m_File)
    {
        LOG("Cinematic::Open: Couldn't open {}\n", filename);
        return false;
    }

    plm_io_callbacks callbacks;

    callbacks.seek = [](void* stream, long offset, int origin) {
        switch (origin)
        {
            case SEEK_SET:
                ((File*)stream)->SeekSet(offset);
                break;
            case SEEK_CUR:
                ((File*)stream)->SeekCur(offset);
                break;
            case SEEK_END:
                ((File*)stream)->SeekEnd(offset);
                break;
        }
    };
    callbacks.tell = [](void* stream) -> long {
        return ((File*)stream)->GetOffset();
    };
    callbacks.read = [](void* data, size_t numBytes, void* stream) -> size_t {
        return ((File*)stream)->Read(data, numBytes);
    };

    m_pImpl = plm_create_with_callbacks(&callbacks, &m_File);
    if (!m_pImpl)
    {
        LOG("Cinematic::Open: Couldn't open {}\n", filename);
        return false;
    }

    if (!plm_probe(m_pImpl, 5000 * 1024))
    {
        LOG("Cinematic::Open: No MPEG video or audio streams found in {}\n", filename);
        plm_destroy(m_pImpl);
        m_pImpl = nullptr;
        return false;
    }

    m_FrameRate = plm_get_framerate(m_pImpl);
    m_SampleRate = plm_get_samplerate(m_pImpl);
    m_Duration = plm_get_duration(m_pImpl);
    m_Width = plm_get_width(m_pImpl);
    m_Height = plm_get_height(m_pImpl);

    LOG("Cinematic opened:\n"
        "  file: {}\n"
        "  framerate: {}\n"
        "  samplerate: {}\n"
        "  duration: {}\n", filename, m_FrameRate, m_SampleRate, m_Duration);

    plm_set_video_decode_callback(
        m_pImpl, [](plm_t* self, plm_frame_t* frame, void* user)
        {
                Frame f;
                f.data = frame;
                ((Cinematic*)user)->OnVideoDecode(f);
        },
        this);
    plm_set_video_enabled(m_pImpl, true);

    bool audioEnabled = !(flags & CinematicFlags::NoAudio) && plm_get_num_audio_streams(m_pImpl) > 0;
    plm_set_audio_enabled(m_pImpl, audioEnabled);

    if (audioEnabled)
    {
        plm_set_audio_decode_callback(
            m_pImpl, [](plm_t* self, plm_samples_t* samples, void* user)
            {
                ((Cinematic*)user)->OnAudioDecode(samples->count, samples->interleaved);
            },
            this);

        plm_set_audio_stream(m_pImpl, 0);

        int audioSamples = 4096;

        // Adjust the audio lead time according to the audio_spec buffer size
        plm_set_audio_lead_time(m_pImpl, (double)audioSamples / m_SampleRate);
    }

    TextureResource* texture = GameApplication::sGetResourceManager().TryGet(m_Texture);
    HK_ASSERT(texture);

    if (!texture->GetTextureGPU() || texture->GetWidth() != m_Width || texture->GetHeight() != m_Height)
        texture->Allocate2D(TEXTURE_FORMAT_SBGRA8_UNORM, 1, m_Width, m_Height);

    if (audioEnabled)
    {
        AudioDevice* audio = GameApplication::sGetAudioDevice();

        AudioStreamDesc streamDesc{};
        streamDesc.Format = AudioTransferFormat::FLOAT32;
        streamDesc.NumChannels = 2;
        streamDesc.SampleRate = m_SampleRate;

        m_AudioStream = audio->CreateStream(streamDesc);
        m_AudioStream->SetVolume(m_Volume);
        m_AudioStream->UnblockSound();
    }

    return true;
}

void Cinematic::Close()
{
    if (m_pImpl)
    {
        plm_destroy(m_pImpl);
        m_pImpl = nullptr;
    }

    if (m_Texture)
    {
        TextureResource* texture = GameApplication::sGetResourceManager().TryGet(m_Texture);
        HK_ASSERT(texture);

        texture->SetTextureGPU(nullptr);
    }

    m_AudioStream.Reset();

    m_File.Close();

    m_FrameRate = 0;
    m_SampleRate = 0;
    m_Duration = 0;
    m_Width = 0;
    m_Height = 0;
}

bool Cinematic::IsOpened() const
{
    return m_pImpl != nullptr;
}

uint32_t Cinematic::GetWidth() const
{
    return m_Width;
}

uint32_t Cinematic::GetHeight() const
{
    return m_Height;
}

void Cinematic::SetVolume(float volume)
{
    m_Volume = Math::Saturate(volume);

    if (m_AudioStream)
        m_AudioStream->SetVolume(m_Volume);
}

float Cinematic::GetVolume() const
{
    return m_Volume;
}

void Cinematic::SetLoop(bool loop)
{
    if (m_pImpl)
        plm_set_loop(m_pImpl, loop);
}

bool Cinematic::GetLoop() const
{
    return m_pImpl ? !!plm_get_loop(m_pImpl) : false;
}

void Cinematic::Rewind()
{
    m_SeekTo = 0;
}

void Cinematic::Seek(float ratio)
{
    SeekSeconds(m_Duration * ratio);
}

void Cinematic::SeekSeconds(float seconds)
{
    if (m_pImpl)
        m_SeekTo = Math::Clamp(seconds, 0.0f, static_cast<float>(m_Duration));
}

void Cinematic::Tick(float timeStep)
{
    if (!m_pImpl)
        return;

    if (m_SeekTo != -1)
    {
        if (m_SeekTo == 0)
            plm_rewind(m_pImpl);
        else
            plm_seek(m_pImpl, m_SeekTo, false);

        if (m_AudioStream)
            m_AudioStream->Clear();

        m_SeekTo = -1;
    }

    plm_decode(m_pImpl, timeStep);
}

bool Cinematic::IsEnded() const
{
    return m_pImpl ? !!plm_has_ended(m_pImpl) : true;
}

double Cinematic::GetTime() const
{
    return m_pImpl ? plm_get_time(m_pImpl) : 0;
}

double Cinematic::GetDuration() const
{
    return m_Duration;
}

double Cinematic::GetFrameRate() const
{
    return m_FrameRate;
}

int Cinematic::GetSampleRate() const
{
    return m_SampleRate;
}

TextureHandle Cinematic::GetTextureHandle() const
{
    return m_Texture;
}

void Cinematic::OnVideoDecode(Frame& frame)
{
    uint32_t width = frame.data->width;
    uint32_t height = frame.data->height;

    HK_ASSERT(width == m_Width);
    HK_ASSERT(height == m_Height);

    size_t size = width * height * 4;
    if (m_Blob.Size() < size)
    {
        m_Blob.Reset(size);

        // Set all pixels to white, alpha to 1.0f
        memset(m_Blob.GetData(), 0xff, size);
    }

    plm_frame_to_bgra(frame.data, (uint8_t*)m_Blob.GetData(), frame.data->width * 4);

    TextureResource* texture = GameApplication::sGetResourceManager().TryGet(m_Texture);
    texture->WriteData2D(0, 0, width, height, 0, m_Blob.GetData());

    E_OnImageUpdate.Invoke((uint8_t*)m_Blob.GetData(), width, height);
}

void Cinematic::OnAudioDecode(uint32_t count, float const* interleaved)
{
    HK_ASSERT(m_AudioStream);

    int size = sizeof(float) * count * 2;
    m_AudioStream->QueueAudio(interleaved, size);
}

HK_NAMESPACE_END
