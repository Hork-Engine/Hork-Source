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

#include <World/Public/Base/BaseObject.h>

class IAudioStreamInterface : public ABaseObject {
    AN_CLASS( IAudioStreamInterface, ABaseObject )

public:
    /** Initialize and use file as source */
    virtual bool InitializeFileStream( const char * _FileName );

    /** Initialize and use encoded data as source */
    virtual bool InitializeMemoryStream( const byte * FileInMemory, size_t FileInMemorySize );

    /** Seek to first sample */
    void StreamRewind() { StreamSeek( 0 ); }

    /** Seek to sample */
    virtual void StreamSeek( int _PositionInSamples );

    /** Return total samples (samples * channels) */
    virtual int StreamDecodePCM( short * _Buffer, int _NumShorts );

protected:
    IAudioStreamInterface() {}
};

struct SAudioFileInfo
{
    int SamplesCount;
    int Channels;
    int SampleRate;
    int BitsPerSample;
};

class IAudioDecoderInterface : public ABaseObject
{
    AN_CLASS( IAudioDecoderInterface, ABaseObject )

public:
    virtual IAudioStreamInterface * CreateAudioStream();

    /** Open audio file and read info */
    virtual bool GetAudioFileInfo( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo );

    /** Open media file and read PCM frames to heap memory */
    virtual bool LoadFromFile( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, short ** _PCM );

protected:
    IAudioDecoderInterface() {}
};
