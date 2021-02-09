/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Core/Public/Ref.h>
#include <Core/Public/BinaryStream.h>

#include "AudioBuffer.h"

class IAudioStream : public ARefCounted
{
public:
    /** Initialize stream from file */
    virtual bool InitializeFileStream( const char * FileName, int SampleRate, int SampleBits, int Channels ) = 0;

    /** Initialize stream from memory */
    virtual bool InitializeMemoryStream( const char * FileName, const byte * FileInMemory, size_t FileInMemorySize, int SampleRate, int SampleBits, int Channels ) = 0;

    /** Seek to frame */
    virtual void SeekToFrame( int FrameNum ) = 0;

    /** Return total frames */
    virtual int ReadFrames( void * pFrames, int FrameCount, size_t SizeInBytes ) = 0;
};

struct SAudioFileInfo
{
    int FrameCount;
    int Channels;
    int SampleBits;
};

class IAudioDecoder : public ARefCounted
{
public:
    virtual void CreateAudioStream( TRef< IAudioStream > * ppInterface ) = 0;

    /** Open audio file and read PCM frames to heap memory */
    virtual bool LoadFromFile( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, int SampleRate, bool bForceMono, bool bForce8Bit, void ** ppFrames = nullptr ) = 0;

    bool CreateBuffer( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, int SampleRate, bool bForceMono, bool bForce8Bit, TRef< SAudioBuffer > * ppBuffer );
};
