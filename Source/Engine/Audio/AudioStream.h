/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
struct SFileInMemory : SInterlockedRef
{
private:
    void * pHeapPtr;

    size_t SizeInBytes;

public:
    SFileInMemory( void * _pHeapPtr, size_t _SizeInBytes )
    {
        pHeapPtr = _pHeapPtr;
        SizeInBytes = _SizeInBytes;
    }

    ~SFileInMemory()
    {
        GHeapMemory.Free( pHeapPtr );
    }

    const void * GetHeapPtr() const
    {
        return pHeapPtr;
    }

    size_t GetSizeInBytes() const
    {
        return SizeInBytes;
    }
};

/** SAudioStream
Designed as immutable structure, so we can use it from several threads.
NOTE: SeekToFrame and ReadFrames are not thread safe without your own synchronization. */
struct SAudioStream : SInterlockedRef
{
private:
    /** Audio decoder */
    struct ma_decoder * Decoder;

    /** Audio source */
    TRef< SFileInMemory > pFileInMemory;

    /** Frame count */
    int FrameCount;

    /** Channels count */
    int Channels;

    /** Bits per sample */
    int SampleBits;

    /** Stride between frames in bytes */
    int SampleStride;

public:
    SAudioStream( SFileInMemory * pFileInMemory, int FrameCount, int SampleRate, int SampleBits, int Channels );

    ~SAudioStream();

    /** Frame count */
    AN_FORCEINLINE int GetFrameCount() const
    {
        return FrameCount;
    }

    /** Channels count */
    AN_FORCEINLINE int GetChannels() const
    {
        return Channels;
    }

    /** Bits per sample */
    AN_FORCEINLINE int GetSampleBits() const
    {
        return SampleBits;
    }

    /** Stride between frames in bytes */
    AN_FORCEINLINE int GetSampleStride() const
    {
        return SampleStride;
    }

    /** Seeks to a PCM frame based on it's absolute index. */
    void SeekToFrame( int FrameNum );

    /** Reads PCM frames from the stream. */
    int ReadFrames( void * pFrames, int FrameCount, size_t SizeInBytes );
};
