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

#include "AudioSource.h"

HK_NAMESPACE_BEGIN

AudioSource::AudioSource(int inFrameCount, int inSampleRate, int inSampleBits, int inChannels, HeapBlob blob) :
    m_Blob(std::move(blob))
{
    m_FrameCount = inFrameCount;
    m_Channels = inChannels;
    m_SampleBits = inSampleBits;
    m_SampleStride = (m_SampleBits >> 3) << (m_Channels - 1);
    m_SampleRate = inSampleRate;
    m_bIsEncoded = true;
    m_Frames = nullptr;
}

AudioSource::AudioSource(int inFrameCount, int inSampleRate, int inSampleBits, int inChannels, const void* inFrames)
{
    m_FrameCount = inFrameCount;
    m_Channels = inChannels;
    m_SampleBits = inSampleBits;
    m_SampleStride = (m_SampleBits >> 3) << (m_Channels - 1);
    m_SampleRate = inSampleRate;
    m_Blob.Reset(m_FrameCount * m_SampleStride, inFrames);
    m_Frames = m_Blob.GetData();
    m_bIsEncoded = false;
}

HK_NAMESPACE_END
