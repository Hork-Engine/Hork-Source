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

#include <Hork/Image/Gif.h>
#include <Hork/Resources/Resource_Texture.h>

HK_NAMESPACE_BEGIN

class GifPlayer final : public Noncopyable
{
public:
    explicit        GifPlayer(StringView resourceName);
                    ~GifPlayer();

    bool            Open(StringView filename);
    void            Close();

    bool            IsOpened() const;

    uint32_t        GetWidth() const;
    uint32_t        GetHeight() const;

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
    float           GetTime() const;

    /// Get the video duration in seconds.
    float           GetDuration() const;

    TextureHandle   GetTextureHandle() const;

private:
    TextureHandle   m_Texture;
    GifImage        m_Image;
    GifImage::DecodeContext m_DecContext;
    float           m_Time = 0;
    bool            m_Loop = false;
    bool            m_IsEnded = false;
};

HK_NAMESPACE_END
