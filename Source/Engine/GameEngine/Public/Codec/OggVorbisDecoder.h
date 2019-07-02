#pragma once

#include <Engine/GameEngine/Public/AudioDecoderInterface.h>

class ANGIE_API FOggVorbisAudioTrack : public IAudioStreamInterface {
    AN_CLASS( FOggVorbisAudioTrack, IAudioStreamInterface )

public:
    bool InitializeFileStream( const char * _FileName ) override;

    bool InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) override;

    void StreamRewind() override;

    void StreamSeek( int _PositionInSamples ) override;

    int StreamDecodePCM( short * _Buffer, int _NumShorts ) override;

protected:
    FOggVorbisAudioTrack();
    ~FOggVorbisAudioTrack();

private:
    struct stb_vorbis * Vorbis;
};

class ANGIE_API FOggVorbisDecoder : public IAudioDecoderInterface {
    AN_CLASS( FOggVorbisDecoder, IAudioDecoderInterface )

public:
    IAudioStreamInterface * CreateAudioStream() override;

    bool DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) override;
    bool ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) override;

protected:
    FOggVorbisDecoder();
};
