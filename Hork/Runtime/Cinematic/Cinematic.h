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

#pragma once

#include <Hork/Core/Delegate.h>
#include <Hork/Audio/AudioStream.h>
#include <Hork/Resources/Resource_Texture.h>

struct plm_t;

HK_NAMESPACE_BEGIN

enum class CinematicFlags
{
    Default,
    NoAudio
};

HK_FLAG_ENUM_OPERATORS(CinematicFlags)

class Cinematic final : public Noncopyable
{
public:
    Delegate<void(uint8_t const*, uint32_t, uint32_t)> E_OnImageUpdate;

    explicit        Cinematic(StringView resourceName);
                    ~Cinematic();

    bool            Open(StringView filename, CinematicFlags flags = CinematicFlags::Default);
    void            Close();

    bool            IsOpened() const;

    uint32_t        GetWidth() const;
    uint32_t        GetHeight() const;

    void            SetVolume(float volume);
    float           GetVolume() const;

    /// Set looping.
    void            SetLoop(bool loop);

    /// Get looping.
    bool            GetLoop() const;

    /// Rewind all buffers back to the beginning.
    void            Rewind();

    void            Seek(float ratio);

    void            SeekSeconds(float seconds);

    void            Tick(float timeStep);

    /// Get whether the file has ended. If looping is enabled, this will always
    /// return false.
    bool            IsEnded() const;

    /// Get the current internal time in seconds.
    double          GetTime() const;

    /// Get the video duration in seconds.
    double          GetDuration() const;

    /// Get the framerate of the video stream in frames per second.
    double          GetFrameRate() const;

    /// Get the samplerate of the audio stream in samples per second.
    int             GetSampleRate() const;

    TextureHandle   GetTextureHandle() const;

private:
    struct Frame;

    void            OnVideoDecode(Frame& frame);
    void            OnAudioDecode(uint32_t count, float const* interleaved);

    plm_t*          m_pImpl = nullptr;
    File            m_File;
    double          m_FrameRate = 0;
    int             m_SampleRate = 0;
    double          m_Duration = 0;
    float           m_Volume = 1.0f;
    uint32_t        m_Width = 0;
    uint32_t        m_Height = 0;
    double          m_SeekTo = -1;
    HeapBlob        m_Blob;
    TextureHandle   m_Texture;
    Ref<AudioStream>m_AudioStream;
};

HK_NAMESPACE_END
