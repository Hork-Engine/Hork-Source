#pragma once

#include <Engine/GameEngine/Public/AudioDecoderInterface.h>

struct FWaveFormat {
    Short       Format;
    Short       Channels;
    Short       BitsPerSample;
    Int         SampleRate;
    UInt        NumSamples;
    UInt        DataSize;
    UInt        DataBase;

    // ADPCM
    Short       BlockAlign;
    int         SamplesPerBlock;
    int         BlockLength;
    int         BlocksCount;
};

class ANGIE_API FWavAudioTrack : public IAudioStreamInterface {
    AN_CLASS( FWavAudioTrack, IAudioStreamInterface )

public:
    bool InitializeFileStream( const char * _FileName ) override;

    bool InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) override;

    void StreamRewind() override;

    void StreamSeek( int _PositionInSamples ) override;

    int StreamDecodePCM( short * _Buffer, int _NumShorts ) override;

protected:
    FWavAudioTrack();
    ~FWavAudioTrack();

private:
    FFileStream File;
    FWaveFormat Wave;
    const byte * WaveMemory;
    int CurrentSample;
    int PCMDataOffset;
    byte * ADPCM;
    int ADPCMBufferLength;
};

class ANGIE_API FWavDecoder : public IAudioDecoderInterface {
    AN_CLASS( FWavDecoder, IAudioDecoderInterface )

public:
    IAudioStreamInterface * CreateAudioStream() override;

    bool DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) override;
    bool DecodePCM( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) override;

    bool ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) override;
    bool ReadEncoded( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) override;

protected:
    FWavDecoder();
};
