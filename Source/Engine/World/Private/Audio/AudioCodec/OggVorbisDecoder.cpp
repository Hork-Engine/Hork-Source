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

#include <World/Public/Audio/AudioCodec/OggVorbisDecoder.h>
#include <Core/Public/Alloc.h>

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

AN_CLASS_META( AOggVorbisAudioTrack )
AN_CLASS_META( AOggVorbisDecoder )

AOggVorbisAudioTrack::AOggVorbisAudioTrack() {
    Vorbis = NULL;
}

AOggVorbisAudioTrack::~AOggVorbisAudioTrack() {
    stb_vorbis_close( Vorbis );
}

bool AOggVorbisAudioTrack::InitializeFileStream( const char * _FileName ) {
    AN_ASSERT( Vorbis == NULL );

    Vorbis = stb_vorbis_open_filename( _FileName, NULL, NULL );
    return Vorbis != NULL;
}

bool AOggVorbisAudioTrack::InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) {
    AN_ASSERT( Vorbis == NULL );

    Vorbis = stb_vorbis_open_memory( _EncodedData, _EncodedDataLength, NULL, NULL );
    return Vorbis != NULL;
}

void AOggVorbisAudioTrack::StreamSeek( int _PositionInSamples ) {
    if ( _PositionInSamples <= 0 ) {
        stb_vorbis_seek_start( Vorbis );
    }
    stb_vorbis_seek( Vorbis, _PositionInSamples );
}

int AOggVorbisAudioTrack::StreamDecodePCM( short * _Buffer, int _NumShorts ) {
    int totalSamples = 0;
    int readSamples = 0;

    while ( totalSamples < _NumShorts ) {
        readSamples = stb_vorbis_get_samples_short_interleaved( Vorbis, Vorbis->channels, _Buffer + totalSamples, _NumShorts - totalSamples ) * Vorbis->channels;
        if ( readSamples > 0 ) {
            totalSamples += readSamples;
        } else {
            break;
        }
    }
    return totalSamples;
}

AOggVorbisDecoder::AOggVorbisDecoder() {

}

IAudioStreamInterface * AOggVorbisDecoder::CreateAudioStream() {
    return CreateInstanceOf< AOggVorbisAudioTrack >();
}

bool AOggVorbisDecoder::DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    bool bResult = false;
    int numSamples = 0;
    int numChannels = 0;
    int sampleRate = 0;

    if ( _PCM ) {
        *_PCM = NULL;
        // FIXME: stb_vorbis_decode_filename uses malloc
        numSamples = stb_vorbis_decode_filename( _FileName, &numChannels, &sampleRate, _PCM );
        if ( numSamples > 0 && numChannels > 0 ) {
            numSamples /= numChannels; // FIXME: right?
            bResult = true;
        }
    } else {
        stb_vorbis * tmpVorbis = stb_vorbis_open_filename( _FileName, NULL, NULL );
        if ( tmpVorbis ) {
            numChannels = tmpVorbis->channels;
            sampleRate = tmpVorbis->sample_rate;
            numSamples = stb_vorbis_stream_length_in_samples( tmpVorbis );
            stb_vorbis_close( tmpVorbis );
            bResult = true;
        }
    }

    *_SamplesCount = numSamples;
    *_Channels = numChannels;
    *_SampleRate = sampleRate;
    *_BitsPerSample = 16;

    return bResult;
}

bool AOggVorbisDecoder::ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;

    // TODO: replace FILE by AFileStream?
    FILE * f = fopen( _FileName, "rb" );
    if ( !f ) {
        return false;
    }

    stb_vorbis * tmpVorbis = stb_vorbis_open_file( f, FALSE, NULL, NULL );
    if ( !tmpVorbis ) {
        fclose( f );
        return false;
    }

    *_SamplesCount = stb_vorbis_stream_length_in_samples( tmpVorbis );
    *_Channels = tmpVorbis->channels;
    *_SampleRate = tmpVorbis->sample_rate;
    *_BitsPerSample = 16;
            
    fseek( f, 0, SEEK_END );
    *_EncodedDataLength = ftell( f );
    fseek( f, 0, SEEK_SET );
    if ( *_EncodedDataLength > 0 ) {
        *_EncodedData = ( byte * )GZoneMemory.Alloc( *_EncodedDataLength );
        fread( *_EncodedData, 1, *_EncodedDataLength, f );
    }

    stb_vorbis_close( tmpVorbis );
    fclose( f );
    return true;
}
