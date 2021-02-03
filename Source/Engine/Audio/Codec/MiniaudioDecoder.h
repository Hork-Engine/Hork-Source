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

#include <Audio/AudioDecoder.h>
#include <Core/Public/IO.h>

class AMiniaudioTrack : public IAudioStream
{
public:
    AMiniaudioTrack();
    ~AMiniaudioTrack();

    bool InitializeFileStream( const char * FileName, int SampleRate, int SampleBits, int Channels ) override;

    bool InitializeMemoryStream( const char * FileName, const byte * FileInMemory, size_t FileInMemorySize, int SampleRate, int SampleBits, int Channels ) override;

    void SeekToFrame( int FrameNum ) override;

    int ReadFrames( void * pFrames, int FrameCount, size_t SizeInBytes ) override;

private:
    void PurgeStream();

    void * Handle;
    AFileStream File;
    AMemoryStream Memory;
    int SampleBits;
    int Channels;
    bool bValid;
};

class AMiniaudioDecoder : public IAudioDecoder
{
public:
    AMiniaudioDecoder();

    void CreateAudioStream( TRef< IAudioStream > * ppInterface ) override;

    /** Open media file and read PCM frames to heap memory */
    bool LoadFromFile( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, int SampleRate, bool bForceMono, bool bForce8Bit, void ** pFrames = nullptr ) override;
};
