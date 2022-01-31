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

/** SAudioBuffer
Designed as immutable structure, so we can use it from several threads. Owns the heap pointer. */
struct SAudioBuffer : SInterlockedRef
{
private:
    /** Audio data */
    void * pFramesHeapPtr;

    /** Frame count */
    int FrameCount;

    /** Channels count */
    int Channels;

    /** Bits per sample */
    int SampleBits;

    /** Stride between frames in bytes */
    int SampleStride;

public:
    SAudioBuffer( int _FrameCount, int _Channels, int _SampleBits, void * _pFramesHeapPtr )
    {
        pFramesHeapPtr = _pFramesHeapPtr;
        FrameCount = _FrameCount;
        Channels = _Channels;
        SampleBits = _SampleBits;
        SampleStride = ( SampleBits >> 3 ) << ( Channels - 1 );
    }

    ~SAudioBuffer()
    {
        GHeapMemory.Free( pFramesHeapPtr );
    }

    /** Audio data */
    AN_FORCEINLINE const void * GetFrames() const
    {
        return pFramesHeapPtr;
    }

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
};
