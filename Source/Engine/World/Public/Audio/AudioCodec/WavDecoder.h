/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

struct SWaveFormat {
    int16_t       Format;
    int16_t       Channels;
    int16_t       BitsPerSample;
    int32_t         SampleRate;
    uint32_t        NumSamples;
    uint32_t        DataSize;
    uint32_t        DataBase;

    // ADPCM
    int16_t       BlockAlign;
    int         SamplesPerBlock;
    int         BlockLength;
    int         BlocksCount;
};

class ANGIE_API AWavAudioTrack : public IAudioStreamInterface {
    AN_CLASS( AWavAudioTrack, IAudioStreamInterface )

public:
    bool InitializeFileStream( const char * _FileName ) override;

    bool InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) override;

    void StreamRewind() override;

    void StreamSeek( int _PositionInSamples ) override;

    int StreamDecodePCM( short * _Buffer, int _NumShorts ) override;

protected:
    AWavAudioTrack();
    ~AWavAudioTrack();

private:
    AFileStream File;
    SWaveFormat Wave;
    const byte * WaveMemory;
    int CurrentSample;
    int PCMDataOffset;
    byte * ADPCM;
    int ADPCMBufferLength;
};

class ANGIE_API AWavDecoder : public IAudioDecoderInterface {
    AN_CLASS( AWavDecoder, IAudioDecoderInterface )

public:
    IAudioStreamInterface * CreateAudioStream() override;

    bool DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) override;
    bool DecodePCM( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) override;

    bool ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) override;
    bool ReadEncoded( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) override;

protected:
    AWavDecoder();
};
