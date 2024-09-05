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

#include <SDL3/SDL.h>

HK_NAMESPACE_BEGIN

AudioStream::~AudioStream()
{
    SDL_DestroyAudioStream((SDL_AudioStream*)m_AudioStream);
}

void AudioStream::Clear()
{
    SDL_ClearAudioStream((SDL_AudioStream*)m_AudioStream);
}

void AudioStream::QueueAudio(void const* data, size_t size)
{
    SDL_PutAudioStreamData((SDL_AudioStream*)m_AudioStream, data, size);
}

void AudioStream::BlockSound()
{
    SDL_PauseAudioStreamDevice((SDL_AudioStream*)m_AudioStream);
}

void AudioStream::UnblockSound()
{
    SDL_ResumeAudioStreamDevice((SDL_AudioStream*)m_AudioStream);
}

HK_NAMESPACE_END
