/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Core/Ref.h>

/** Immutable structure that holds heap pointer and size in bytes. Owns the heap pointer. */
struct FileInMemory : InterlockedRef
{
private:
    void* pHeapPtr;

    size_t SizeInBytes;

public:
    FileInMemory(void* _pHeapPtr, size_t _SizeInBytes)
    {
        pHeapPtr = _pHeapPtr;
        SizeInBytes = _SizeInBytes;
    }

    ~FileInMemory()
    {
        Platform::GetHeapAllocator<HEAP_AUDIO_DATA>().Free(pHeapPtr);
    }

    const void* GetHeapPtr() const
    {
        return pHeapPtr;
    }

    size_t GetSizeInBytes() const
    {
        return SizeInBytes;
    }
};

/** AudioStream
Designed as immutable structure, so we can use it from several threads.
NOTE: SeekToFrame and ReadFrames are not thread safe without your own synchronization. */
class AudioStream : public InterlockedRef
{
public:
    AudioStream(FileInMemory* pFileInMemory, int FrameCount, int SampleRate, int SampleBits, int Channels);

    ~AudioStream();

    /** Frame count */
    HK_FORCEINLINE int GetFrameCount() const
    {
        return m_FrameCount;
    }

    /** Channels count */
    HK_FORCEINLINE int GetChannels() const
    {
        return m_Channels;
    }

    /** Bits per sample */
    HK_FORCEINLINE int GetSampleBits() const
    {
        return m_SampleBits;
    }

    /** Stride between frames in bytes */
    HK_FORCEINLINE int GetSampleStride() const
    {
        return m_SampleStride;
    }

    /** Seeks to a PCM frame based on it's absolute index. */
    void SeekToFrame(int FrameNum);

    /** Reads PCM frames from the stream. */
    int ReadFrames(void* pFrames, int FrameCount, size_t SizeInBytes);

private:
    /** Audio decoder */
    struct ma_decoder* m_Decoder;

    /** Audio source */
    TRef<FileInMemory> m_pFileInMemory;

    /** Frame count */
    int m_FrameCount;

    /** Channels count */
    int m_Channels;

    /** Bits per sample */
    int m_SampleBits;

    /** Stride between frames in bytes */
    int m_SampleStride;
};
