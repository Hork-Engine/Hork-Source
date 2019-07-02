#pragma once

#include <Engine/GameEngine/Public/AudioDecoderInterface.h>

extern "C" {

struct mpg123_handle_struct;

}

class ANGIE_API FMp3AudioTrack : public IAudioStreamInterface {
    AN_CLASS( FMp3AudioTrack, IAudioStreamInterface )

public:
    bool InitializeFileStream( const char * _FileName ) override;

    bool InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) override;

    void StreamRewind() override;

    void StreamSeek( int _PositionInSamples ) override;

    int StreamDecodePCM( short * _Buffer, int _NumShorts ) override;

protected:
    FMp3AudioTrack();
    ~FMp3AudioTrack();

private:
    mpg123_handle_struct * Handle;
    int BlockSize;
    int NumChannels;
};

class ANGIE_API FMp3Decoder : public IAudioDecoderInterface {
    AN_CLASS( FMp3Decoder, IAudioDecoderInterface )

public:
    IAudioStreamInterface * CreateAudioStream() override;

    bool DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) override;
    bool ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) override;

protected:
    FMp3Decoder();
};
