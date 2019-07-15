/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/GameEngine/Public/Codec/OggVorbisDecoder.h>
#include <Engine/Core/Public/Alloc.h>

#ifdef AN_COMPILER_MSVC
#pragma warning( push )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4457 )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4701 )
#endif

#ifdef AN_COMPILER_GCC
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

#include <stb_vorbis.c>

#ifdef AN_COMPILER_MSVC
#pragma warning( pop )
#endif

AN_CLASS_META_NO_ATTRIBS( FOggVorbisAudioTrack )
AN_CLASS_META_NO_ATTRIBS( FOggVorbisDecoder )

FOggVorbisAudioTrack::FOggVorbisAudioTrack() {
    Vorbis = NULL;
}

FOggVorbisAudioTrack::~FOggVorbisAudioTrack() {
    stb_vorbis_close( Vorbis );
}

bool FOggVorbisAudioTrack::InitializeFileStream( const char * _FileName ) {
    assert( Vorbis == NULL );

    Vorbis = stb_vorbis_open_filename( _FileName, NULL, NULL );
    return Vorbis != NULL;
}

bool FOggVorbisAudioTrack::InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) {
    assert( Vorbis == NULL );

    Vorbis = stb_vorbis_open_memory( _EncodedData, _EncodedDataLength, NULL, NULL );
    return Vorbis !=NULL;
}

void FOggVorbisAudioTrack::StreamRewind() {
    stb_vorbis_seek_start( Vorbis );
}

void FOggVorbisAudioTrack::StreamSeek( int _PositionInSamples ) {
    stb_vorbis_seek( Vorbis, _PositionInSamples );
}

int FOggVorbisAudioTrack::StreamDecodePCM( short * _Buffer, int _NumShorts ) {
    int TotalSamples = 0;
    int ReadSamples = 0;

    while ( TotalSamples < _NumShorts ) {
        ReadSamples = stb_vorbis_get_samples_short_interleaved( Vorbis, Vorbis->channels, _Buffer + TotalSamples, _NumShorts - TotalSamples ) * Vorbis->channels;
        if ( ReadSamples > 0 ) {
            TotalSamples += ReadSamples;
        } else {
            break;
        }
    }
    return TotalSamples;
}

FOggVorbisDecoder::FOggVorbisDecoder() {

}

IAudioStreamInterface * FOggVorbisDecoder::CreateAudioStream() {
    return CreateInstanceOf< FOggVorbisAudioTrack >();
}

bool FOggVorbisDecoder::DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    bool bResult = false;
    int NumSamples = 0;
    int NumChannels = 0;
    int SampleRate = 0;

    if ( _PCM ) {
        *_PCM = NULL;
        // FIXME: stb_vorbis_decode_filename uses malloc
        NumSamples = stb_vorbis_decode_filename( _FileName, &NumChannels, &SampleRate, _PCM );
        if ( NumSamples > 0 && NumChannels > 0 ) {
            NumSamples /= NumChannels; // FIXME: right?
            bResult = true;
        }
    } else {
        stb_vorbis * TmpVorbis = stb_vorbis_open_filename( _FileName, NULL, NULL );
        if ( TmpVorbis ) {
            NumChannels = TmpVorbis->channels;
            SampleRate = TmpVorbis->sample_rate;
            NumSamples = stb_vorbis_stream_length_in_samples( TmpVorbis );
            stb_vorbis_close( TmpVorbis );
            bResult = true;
        }
    }

    *_SamplesCount = NumSamples;
    *_Channels = NumChannels;
    *_SampleRate = SampleRate;
    *_BitsPerSample = 16;

    return bResult;
}

bool FOggVorbisDecoder::ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;

    // TODO: replace FILE by FFileStream?
    FILE * f = fopen( _FileName, "rb" );
    if ( !f ) {
        return false;
    }

    stb_vorbis * TmpVorbis = stb_vorbis_open_file( f, TRUE, NULL, NULL );
    if ( !TmpVorbis ) {
        return false;
    }

    *_SamplesCount = stb_vorbis_stream_length_in_samples( TmpVorbis );
    *_Channels = TmpVorbis->channels;
    *_SampleRate = TmpVorbis->sample_rate;
    *_BitsPerSample = 16;
            
    fseek( f, 0, SEEK_END );
    *_EncodedDataLength = ftell( f );
    fseek( f, 0, SEEK_SET );
    if ( *_EncodedDataLength > 0 ) {
        *_EncodedData = ( byte * )GMainMemoryZone.Alloc( *_EncodedDataLength, 1 );
        fread( *_EncodedData, 1, *_EncodedDataLength, f );
    }

    stb_vorbis_close( TmpVorbis );
    return true;
}
