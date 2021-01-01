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

#include <World/Public/Audio/AudioDecoderInterface.h>

class AMiniaudioTrack : public IAudioStreamInterface
{
    AN_CLASS( AMiniaudioTrack, IAudioStreamInterface )

public:
    bool InitializeFileStream( const char * _FileName ) override;

    bool InitializeMemoryStream( const byte * FileInMemory, size_t FileInMemorySize ) override;

    void StreamSeek( int _PositionInSamples ) override;

    int StreamDecodePCM( short * _Buffer, int _NumShorts ) override;

protected:
    AMiniaudioTrack();
    ~AMiniaudioTrack();

private:
    void PurgeStream();

    void * Handle;
    AFileStream File;
    AMemoryStream Memory;
    bool bValid;
};

class AMiniaudioDecoder : public IAudioDecoderInterface
{
    AN_CLASS( AMiniaudioDecoder, IAudioDecoderInterface )

public:
    IAudioStreamInterface * CreateAudioStream() override;

    /** Open audio file and read info */
    bool GetAudioFileInfo( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo ) override;

    /** Open media file and read PCM frames to heap memory */
    bool LoadFromFile( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, short ** _PCM ) override;

protected:
    AMiniaudioDecoder();
};
