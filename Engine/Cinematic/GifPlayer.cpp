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

#include "GifPlayer.h"

#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

GifPlayer::GifPlayer(StringView resourceName)
{
    m_Texture = GameApplication::GetResourceManager().CreateResource<TextureResource>(resourceName);
}

GifPlayer::~GifPlayer()
{
    Close();
}

bool GifPlayer::Open(StringView filename)
{
    Close();

    File file = GameApplication::GetResourceManager().OpenFile(filename);
    if (!file)
        return false;

    m_Image = CreateGif(file);
    if (!m_Image)
        return false;

    m_Time = 0;
    m_Image.StartDecode(m_DecContext);

    TextureResource* texture = GameApplication::GetResourceManager().TryGet(m_Texture);
    HK_ASSERT(texture);

    if (!texture->GetTextureGPU() || texture->GetWidth() != m_Image.GetWidth() || texture->GetHeight() != m_Image.GetHeight())
        texture->Allocate2D(TEXTURE_FORMAT_SBGRA8_UNORM, 1, m_Image.GetWidth(), m_Image.GetHeight());

    return true;
}

void GifPlayer::Close()
{
    m_Image.Reset();

    if (m_Texture)
    {
        TextureResource* texture = GameApplication::GetResourceManager().TryGet(m_Texture);
        HK_ASSERT(texture);

        texture->SetTextureGPU(nullptr);
    }

    m_Time = 0;
    m_Loop = false;
    m_IsEnded = false;
}

bool GifPlayer::IsOpened() const
{
    return m_Image;
}

uint32_t GifPlayer::GetWidth() const
{
    return m_Image.GetWidth();
}

uint32_t GifPlayer::GetHeight() const
{
    return m_Image.GetHeight();
}

void GifPlayer::SetLoop(bool loop)
{
    m_Loop = loop;
}

bool GifPlayer::GetLoop() const
{
    return m_Loop;
}

void GifPlayer::Rewind()
{
    m_Time = 0;
    m_Image.StartDecode(m_DecContext);
    m_IsEnded = false;
}

void GifPlayer::Seek(float ratio)
{
    SeekSeconds(ratio * m_Image.GetDuration());
}

void GifPlayer::SeekSeconds(float seconds)
{
    float newTime = Math::Clamp(seconds, 0.0f, m_Image.GetDuration());
    if (newTime < m_Time)
        m_Image.StartDecode(m_DecContext);

    m_Time = newTime;
    m_IsEnded = false;
}

void GifPlayer::Tick(float timeStep)
{
    if (!m_Image)
        return;

    float duration = m_Image.GetDuration();

    float targetTime = m_Time + timeStep;
    if (targetTime >= duration)
    {
        if (m_Loop)
        {
            targetTime = Math::FMod(targetTime, duration);
            m_Image.StartDecode(m_DecContext);
        }
        else
        {
            targetTime = duration;
            m_IsEnded = true;
        }
    }

    bool decoded;
    bool updateTexture = false;

    do
    {
        decoded = false;
        if (m_Image.GetTimeStamp(m_DecContext.FrameIndex) < targetTime)
        {
            if (m_Image.DecodeNextFrame(m_DecContext))
            {
                decoded = true;
                updateTexture = true;
            }
        }
    } while (decoded);

    m_Time = targetTime;

    if (updateTexture)
    {
        TextureResource* texture = GameApplication::GetResourceManager().TryGet(m_Texture);
        texture->WriteData2D(0, 0, GetWidth(), GetHeight(), 0, m_DecContext.Data.GetData());
    }
}

bool GifPlayer::IsEnded() const
{
    if (m_Loop)
        return false;

    return m_IsEnded;
}

float GifPlayer::GetTime() const
{
    return m_Time;
}

float GifPlayer::GetDuration() const
{
    return m_Image.GetDuration();
}

TextureHandle GifPlayer::GetTextureHandle() const
{
    return m_Texture;
}

HK_NAMESPACE_END
