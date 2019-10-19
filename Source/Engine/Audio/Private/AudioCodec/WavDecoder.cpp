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

#include <Engine/Audio/Public/AudioCodec/WavDecoder.h>
#include <Engine/Core/Public/Alloc.h>
#include <Engine/Core/Public/IO.h>
#include <Engine/Core/Public/EndianSwap.h>

#ifdef AN_COMPILER_MSVC
#pragma warning( disable : 4505 )
#endif

// http://audiocoding.ru/assets/meta/2008-05-22-wav-file-structure/wav_formats.txt
// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Docs/RIFFNEW.pdf
enum EWaveEncoding {
    WAVE_FORMAT_PCM       = 0x0001,
    WAVE_FORMAT_DVI_ADPCM = 0x0011
};

static bool IMAADPCMUnpack16_Mono( signed short * _PCM, int _SamplesCount, const byte * _ADPCM, int _DataLength, int _BlockAlign );
static bool IMAADPCMUnpack16_Stereo( signed short * _PCM, int _SamplesCount, int _ChannelsCount, const byte * _ADPCM, int _DataLength, int _BlockAlign );
static bool IMAADPCMUnpack16Ext_Mono( signed short * _PCM, int _IgnoreFirstNSamples, int _SamplesCount, const byte * _ADPCM, int _DataLength, int _BlockAlign );
static bool IMAADPCMUnpack16Ext_Stereo( signed short * _PCM, int _IgnoreFirstNSamples, int _SamplesCount, int _ChannelsCount, const byte * _ADPCM, int _DataLength, int _BlockAlign );

template< typename T >
static bool WaveReadHeader( FStreamBase< T > & _File, SWaveFormat & _Wave );
template< typename T >
static int WaveReadFile( FStreamBase< T > & _File, void * _Buffer, int _BufferLength, SWaveFormat *_Wave );
template< typename T >
static void WaveRewindFile( FStreamBase< T > & _File, SWaveFormat *_Wave );
template< typename T >
static int WaveSeekFile( FStreamBase< T > & _File, int _Offset, SWaveFormat *_Wave );

AN_CLASS_META( FWavAudioTrack )
AN_CLASS_META( FWavDecoder )

FWavAudioTrack::FWavAudioTrack() {
    WaveMemory = NULL;
    PCMDataOffset = 0;
    CurrentSample = 0;
    ADPCM = NULL;
    ADPCMBufferLength = 0;
}

FWavAudioTrack::~FWavAudioTrack() {
    GZoneMemory.Dealloc( ADPCM );
}

bool FWavAudioTrack::InitializeFileStream( const char * _FileName ) {
    assert( !File.IsOpened() );
    assert( WaveMemory == NULL );

    if ( !File.OpenRead( _FileName ) ) {
        return false;
    }

    if ( !WaveReadHeader( File, Wave ) ) {
        File.Close();
        return false;
    }

    if ( WaveSeekFile( File, 0, &Wave ) != 0 ) {
        File.Close();
        return false;
    }

    PCMDataOffset = 0;
    CurrentSample = 0;

    return true;
}

bool FWavAudioTrack::InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) {
    assert( !File.IsOpened() );
    assert( WaveMemory == NULL );

    Wave = *(SWaveFormat *)_EncodedData;
    WaveMemory = _EncodedData + sizeof( SWaveFormat );
    PCMDataOffset = 0;
    CurrentSample = 0;

    assert( Wave.DataSize == _EncodedDataLength - sizeof( SWaveFormat ) );

    return true;
}

void FWavAudioTrack::StreamRewind() {
    PCMDataOffset = 0;
    CurrentSample = 0;

    if ( File.IsOpened() ) {
        WaveRewindFile( File, &Wave );
    }
}

void FWavAudioTrack::StreamSeek( int _PositionInSamples ) {
    if ( WaveMemory ) {
        if ( Wave.Format == WAVE_FORMAT_PCM ) {
            int bytesPerSample = Wave.BitsPerSample >> 3;

            CurrentSample = FMath::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );

            PCMDataOffset = CurrentSample * bytesPerSample;

            //PCMDataOffset = CurrentSample * BytesPerSample;
            //if ( PCMDataOffset > Wave.DataSize ) {
            //    PCMDataOffset = Wave.DataSize;
            //}
        } else if ( Wave.Format == WAVE_FORMAT_DVI_ADPCM ) {
            CurrentSample = FMath::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );
        }

    } else if ( File.IsOpened() ) {
        if ( Wave.Format == WAVE_FORMAT_PCM ) {
            int bytesPerSample = Wave.BitsPerSample >> 3;

            CurrentSample = FMath::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );

            PCMDataOffset = CurrentSample * bytesPerSample;

            WaveSeekFile( File, PCMDataOffset, &Wave );
        } else if ( Wave.Format == WAVE_FORMAT_DVI_ADPCM ) {
            CurrentSample = FMath::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );
        }
    }
}

int FWavAudioTrack::StreamDecodePCM( short * _Buffer, int _NumShorts ) {
    if ( WaveMemory ) {
        switch ( Wave.Format ) {
            case WAVE_FORMAT_PCM: {
                int dataLength = _NumShorts * 2;
                if ( PCMDataOffset + dataLength > Wave.DataSize ) {
                    dataLength = Wave.DataSize - PCMDataOffset;
                }
                if ( dataLength > 0 ) {
                    memcpy( _Buffer, WaveMemory + PCMDataOffset, dataLength );
                    PCMDataOffset += dataLength;
                } else {
                    return 0;
                }

                int samplesCount = dataLength / (Wave.BitsPerSample >> 3);

                CurrentSample += samplesCount;

                return samplesCount;
            }
            case WAVE_FORMAT_DVI_ADPCM: {

                if ( Wave.Channels == 2 ) {
                    assert( (CurrentSample & 1) == 0 );
                    assert( (Wave.SamplesPerBlock & 1) == 0 );
                }

                int lastSample = CurrentSample + _NumShorts;
                if ( lastSample > Wave.NumSamples ) {
                    lastSample = Wave.NumSamples;
                }

                int numSamples = lastSample - CurrentSample;

                if ( numSamples > 0 ) {
                    int firstBlockIndex = CurrentSample / Wave.SamplesPerBlock;
                    int lastBlockIndex = lastSample / Wave.SamplesPerBlock;
                    int blocksCount = lastBlockIndex - firstBlockIndex + 1;
                    int samplesInsideBlock = lastSample - lastBlockIndex * Wave.SamplesPerBlock;

                    assert( samplesInsideBlock <= Wave.SamplesPerBlock );

                    if ( samplesInsideBlock == 0 ) {
                        blocksCount--;
                        samplesInsideBlock = Wave.SamplesPerBlock;
                    }

                    int samplesCount = (blocksCount - 1) * Wave.SamplesPerBlock + samplesInsideBlock;

#if 1
                    if ( Wave.Channels == 2 ) {
                        IMAADPCMUnpack16Ext_Stereo( _Buffer, samplesCount - numSamples, numSamples, Wave.Channels, WaveMemory + firstBlockIndex * Wave.BlockLength, blocksCount * Wave.BlockLength, Wave.BlockAlign );
                    } else {
                        IMAADPCMUnpack16Ext_Mono( _Buffer, samplesCount - numSamples, numSamples, WaveMemory + firstBlockIndex * Wave.BlockLength, blocksCount * Wave.BlockLength, Wave.BlockAlign );
                    }

#else
                    short * PCM = ( short * )GMainHunkMemory.HunkMemory( sizeof( short ) * SamplesCount );
                    IMAADPCMUnpack16_Mono( PCM, SamplesCount, WaveMemory + FirstBlockIndex * Wave.BlockLength, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    memcpy( _Buffer, PCM + (SamplesCount - NumSamples), NumSamples * 2 );
                    GMainHunkMemory.ClearLastHunk();
#endif

                    CurrentSample += numSamples;
                } else {
                    return 0;
                }

                return numSamples;
            }
        }

        return 0;
    } else if ( File.IsOpened() ) {

        switch ( Wave.Format ) {
            case WAVE_FORMAT_PCM: {
                int dataLength = _NumShorts * 2;
                if ( PCMDataOffset + dataLength > Wave.DataSize ) {
                    dataLength = Wave.DataSize - PCMDataOffset;
                }
                if ( dataLength > 0 ) {
                    dataLength = WaveReadFile( File, (char *)_Buffer, dataLength, &Wave );

                    PCMDataOffset += dataLength;
                } else {
                    return 0;
                }

                int samplesCount = dataLength / (Wave.BitsPerSample >> 3);

                CurrentSample += samplesCount;

                return samplesCount;
            }
            case WAVE_FORMAT_DVI_ADPCM: {

                int lastSample = CurrentSample + _NumShorts;
                if ( lastSample > Wave.NumSamples ) {
                    lastSample = Wave.NumSamples;
                }

                int numSamples = lastSample - CurrentSample;

                if ( numSamples > 0 ) {
                    int firstBlockIndex = CurrentSample / Wave.SamplesPerBlock;
                    int lastBlockIndex = lastSample / Wave.SamplesPerBlock;
                    int blocksCount = lastBlockIndex - firstBlockIndex + 1;
                    int samplesInsideBlock = lastSample - lastBlockIndex * Wave.SamplesPerBlock;

                    assert( samplesInsideBlock <= Wave.SamplesPerBlock );

                    if ( samplesInsideBlock == 0 ) {
                        blocksCount--;
                        samplesInsideBlock = Wave.SamplesPerBlock;
                    }

                    int readBytesCount = blocksCount * Wave.BlockLength;
                    if ( ADPCMBufferLength < readBytesCount ) {
                        ADPCM = ( byte * )GZoneMemory.Extend( ADPCM, ADPCMBufferLength, readBytesCount, 1, false );
                        ADPCMBufferLength = readBytesCount;
                    }

                    WaveSeekFile( File, firstBlockIndex * Wave.BlockLength, &Wave );
                    WaveReadFile( File, ADPCM, readBytesCount, &Wave );

                    int samplesCount = (blocksCount - 1) * Wave.SamplesPerBlock + samplesInsideBlock;

#if 1
                    if ( Wave.Channels == 2 ) {
                        IMAADPCMUnpack16Ext_Stereo( _Buffer, samplesCount - numSamples, numSamples, Wave.Channels, ADPCM, blocksCount * Wave.BlockLength, Wave.BlockAlign );
                    } else {
                        IMAADPCMUnpack16Ext_Mono( _Buffer, samplesCount - numSamples, numSamples, ADPCM, blocksCount * Wave.BlockLength, Wave.BlockAlign );
                    }
#else
                    short * PCM = ( short * )GMainHunkMemory.HunkMemory( sizeof( short ) * SamplesCount );
                    IMAADPCMUnpack16_Mono( PCM, SamplesCount, ADPCM, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    memcpy( _Buffer, PCM + (SamplesCount - NumSamples), NumSamples * 2 );
                    GMainHunkMemory.ClearLastHunk();
#endif

                    CurrentSample += numSamples;
                } else {
                    return 0;
                }

                return numSamples;
            }
        }
    }
    return 0;
}

FWavDecoder::FWavDecoder() {

}

IAudioStreamInterface * FWavDecoder::CreateAudioStream() {
    return CreateInstanceOf< FWavAudioTrack >();
}

bool FWavDecoder::DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    FFileStream f;
    SWaveFormat inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;

    if ( _PCM ) *_PCM = NULL;

    if ( !f.OpenRead( _FileName ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, inf ) ) {
        return false;
    }

    if ( _PCM ) {
        if ( WaveSeekFile( f, 0, &inf ) != 0 ) {
            return false;
        }

        *_PCM = (short *)GZoneMemory.Alloc( inf.DataSize, 1 );
        if ( !*_PCM ) {
            return false;
        }

        if ( WaveReadFile( f, (char *)*_PCM, inf.DataSize, &inf ) != (int)inf.DataSize ) {
            GZoneMemory.Dealloc( *_PCM );
            *_PCM = NULL;
            return false;
        }

        if ( inf.Format == WAVE_FORMAT_DVI_ADPCM ) {
            byte * encodedADPCM = (byte *)*_PCM;
            *_PCM = (short *)GZoneMemory.Alloc( inf.NumSamples * sizeof( short ), 1 );
            if ( inf.Channels == 2 )
                IMAADPCMUnpack16_Stereo( *_PCM, inf.NumSamples, inf.Channels, encodedADPCM, inf.DataSize, inf.BlockAlign );
            else
                IMAADPCMUnpack16_Mono( *_PCM, inf.NumSamples, encodedADPCM, inf.DataSize, inf.BlockAlign );
            GZoneMemory.Dealloc( encodedADPCM );
        }
    }

    *_SamplesCount = inf.NumSamples / inf.Channels;
    *_Channels = inf.Channels;
    *_SampleRate = inf.SampleRate;
    *_BitsPerSample = inf.BitsPerSample;

    return true;
}

bool FWavDecoder::DecodePCM( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    FMemoryStream f;
    SWaveFormat inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;

    if ( _PCM ) *_PCM = NULL;

    if ( !f.OpenRead( _FileName, _Data, _DataLength ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, inf ) ) {
        return false;
    }

    if ( _PCM ) {
        if ( WaveSeekFile( f, 0, &inf ) != 0 ) {
            return false;
        }

        *_PCM = (short *)GZoneMemory.Alloc( inf.DataSize, 1 );
        if ( !*_PCM ) {
            return false;
        }

        if ( WaveReadFile( f, (char *)*_PCM, inf.DataSize, &inf ) != (int)inf.DataSize ) {
            GZoneMemory.Dealloc( *_PCM );
            *_PCM = NULL;
            return false;
        }

        if ( inf.Format == WAVE_FORMAT_DVI_ADPCM ) {
            byte * encodedADPCM = (byte *)*_PCM;
            *_PCM = (short *)GZoneMemory.Alloc( inf.NumSamples * sizeof( short ), 1 );
            if ( inf.Channels == 2 )
                IMAADPCMUnpack16_Stereo( *_PCM, inf.NumSamples, inf.Channels, encodedADPCM, inf.DataSize, inf.BlockAlign );
            else
                IMAADPCMUnpack16_Mono( *_PCM, inf.NumSamples, encodedADPCM, inf.DataSize, inf.BlockAlign );
            GZoneMemory.Dealloc( encodedADPCM );
        }
    }

    *_SamplesCount = inf.NumSamples / inf.Channels;
    *_Channels = inf.Channels;
    *_SampleRate = inf.SampleRate;
    *_BitsPerSample = inf.BitsPerSample;

    return true;
}

bool FWavDecoder::ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    FFileStream f;
    SWaveFormat inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;

    if ( !f.OpenRead( _FileName ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, inf ) ) {
        return false;
    }

    if ( WaveSeekFile( f, 0, &inf ) != 0 ) {
        return false;
    }

    *_EncodedData = ( byte * )GZoneMemory.Alloc( inf.DataSize + sizeof( SWaveFormat ), 1 );
    if ( !*_EncodedData ) {
        return false;
    }

    if ( WaveReadFile( f, ( char * )*_EncodedData + sizeof( SWaveFormat ), inf.DataSize, &inf ) != (int)inf.DataSize ) {
        GZoneMemory.Dealloc( *_EncodedData );
        *_EncodedData = NULL;
        return false;
    }

    memcpy( *_EncodedData, &inf, sizeof( SWaveFormat ) );

    *_EncodedDataLength = inf.DataSize + sizeof( SWaveFormat );
    *_SamplesCount = inf.NumSamples / inf.Channels;
    *_Channels = inf.Channels;
    *_SampleRate = inf.SampleRate;
    *_BitsPerSample = inf.BitsPerSample;

    return true;
}

bool FWavDecoder::ReadEncoded( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    FMemoryStream f;
    SWaveFormat inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;

    if ( !f.OpenRead( _FileName, _Data, _DataLength ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, inf ) ) {
        return false;
    }

    if ( WaveSeekFile( f, 0, &inf ) != 0 ) {
        return false;
    }

    *_EncodedData = ( byte * )GZoneMemory.Alloc( inf.DataSize + sizeof( SWaveFormat ), 1 );
    if ( !*_EncodedData ) {
        return false;
    }

    if ( WaveReadFile( f, ( char * )*_EncodedData + sizeof( SWaveFormat ), inf.DataSize, &inf ) != (int)inf.DataSize ) {
        GZoneMemory.Dealloc( *_EncodedData );
        *_EncodedData = NULL;
        return false;
    }

    memcpy( *_EncodedData, &inf, sizeof( SWaveFormat ) );

    *_EncodedDataLength = inf.DataSize + sizeof( SWaveFormat );
    *_SamplesCount = inf.NumSamples / inf.Channels;
    *_Channels = inf.Channels;
    *_SampleRate = inf.SampleRate;
    *_BitsPerSample = inf.BitsPerSample;

    return true;
}

static const int IMAUnpackTable[ 89 ] =
{
  7,     8,     9,     10,    11,    12,    13,    14,
  16,    17,    19,    21,    23,    25,    28,    31,
  34,    37,    41,    45,    50,    55,    60,    66,
  73,    80,    88,    97,    107,   118,   130,   143,
  157,   173,   190,   209,   230,   253,   279,   307,
  337,   371,   408,   449,   494,   544,   598,   658,
  724,   796,   876,   963,   1060,  1166,  1282,  1411,
  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
  32767
};

static const int IMAIndexTable[ 8 ] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static bool IMAADPCMUnpack16_Mono( signed short * _PCM, int _SamplesCount, const byte * _ADPCM, int _DataLength, int _BlockAlign ) {
    int sampleIndex;
    int sampleValue;
    int tableIndex;
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4 ) * 2;

    sampleIndex = 0;
    while ( sampleIndex < _SamplesCount && _DataLength > 4 ) {
        sampleValue = FCore::LittleWord( *( ( short int * )_ADPCM ) );
        tableIndex = _ADPCM[ 2 ];
        if ( tableIndex > 88 ) {
            tableIndex = 88;
        }
        _ADPCM += 4;
        _DataLength -= 4;
        _PCM[ sampleIndex++ ] = ( short int )sampleValue;
        for ( int byteIndex = 0; byteIndex < BlockLength && sampleIndex < _SamplesCount && _DataLength; byteIndex++ ) {
            if ( byteIndex & 1 ) {
                delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                _DataLength--;
            } else {
                delta = *_ADPCM & 0x0f;
            }
            int v = IMAUnpackTable[ tableIndex ] >> 3;
            if ( delta & 1 ) {
                v += IMAUnpackTable[ tableIndex ] >> 2;
            }
            if ( delta & 2 ) {
                v += IMAUnpackTable[ tableIndex ] >> 1;
            }
            if ( delta & 4 ) {
                v += IMAUnpackTable[ tableIndex ];
            }
            if ( delta & 8 ) {
                sampleValue -= v;
            } else {
                sampleValue += v;
            }
            tableIndex += IMAIndexTable[ delta & 7 ];
            if ( tableIndex < 0 ) {
                tableIndex = 0;
            } else if ( tableIndex > 88 ) {
                tableIndex = 88;
            }
            if ( sampleValue > 32767 ) {
                sampleValue = 32767;
            } else if ( sampleValue < -32768 ) {
                sampleValue = -32768;
            }
            _PCM[ sampleIndex++ ] = ( short int )sampleValue;
        }
    }
    return true;
}

static bool IMAADPCMUnpack16_Stereo( signed short * _PCM, int _SamplesCount, int _ChannelsCount, const byte * _ADPCM, int _DataLength, int _BlockAlign ) {
    int sampleIndex;
    int sampleValue[2];
    int tableIndex[2];
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4*_ChannelsCount ) * 2;
    const int MinDataLength = 4 * _ChannelsCount;

    sampleIndex = 0;
    while ( sampleIndex < _SamplesCount && _DataLength > MinDataLength ) {

        for ( int channel = 0 ; channel < _ChannelsCount ; channel++ ) {
            sampleValue[channel] = FCore::LittleWord( *( ( short int * )_ADPCM ) );
            tableIndex[channel] = _ADPCM[ 2 ];
            if ( tableIndex[channel] > 88 ) {
                tableIndex[channel] = 88;
            }
            _ADPCM += 4;
            _DataLength -= 4;

            // FIXME: закомментировать?
            _PCM[ sampleIndex++ ] = ( short int )sampleValue[channel];
        }

        for ( int byteIndex = 0; byteIndex < BlockLength ; ) {
            for ( int channel = 0 ; channel < _ChannelsCount ; channel++ ) {
                for ( int chunk = 0 ; chunk < 8 ; chunk++, byteIndex++ ) {
                    if ( byteIndex & 1 ) {
                        delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                        _DataLength--;
                    } else {
                        delta = *_ADPCM & 0x0f;
                    }
                    int v = IMAUnpackTable[ tableIndex[channel] ] >> 3;
                    if ( delta & 1 ) {
                        v += IMAUnpackTable[ tableIndex[channel] ] >> 2;
                    }
                    if ( delta & 2 ) {
                        v += IMAUnpackTable[ tableIndex[channel] ] >> 1;
                    }
                    if ( delta & 4 ) {
                        v += IMAUnpackTable[ tableIndex[channel] ];
                    }
                    if ( delta & 8 ) {
                        sampleValue[channel] -= v;
                    } else {
                        sampleValue[channel] += v;
                    }
                    tableIndex[channel] += IMAIndexTable[ delta & 7 ];
                    if ( tableIndex[channel] < 0 ) {
                        tableIndex[channel] = 0;
                    } else if ( tableIndex[channel] > 88 ) {
                        tableIndex[channel] = 88;
                    }
                    if ( sampleValue[channel] > 32767 ) {
                        sampleValue[channel] = 32767;
                    } else if ( sampleValue[channel] < -32768 ) {
                        sampleValue[channel] = -32768;
                    }
                    int Index = sampleIndex + (chunk>>1)*4 + (chunk&1)*2 + channel;
                    if ( Index < _SamplesCount ) {
                        _PCM[ Index ] = ( short int )sampleValue[channel];
                    }
                }
            }

            assert( _DataLength >= 0 );

            sampleIndex += 16;
            if ( sampleIndex >= _SamplesCount ) {
                sampleIndex = _SamplesCount;
                break;
            }
        }
    }

    return true;
}

static bool IMAADPCMUnpack16Ext_Mono( signed short * _PCM, int _IgnoreFirstNSamples, int _SamplesCount, const byte * _ADPCM, int _DataLength, int _BlockAlign ) {
    int sampleIndex;
    int sampleValue;
    int tableIndex;
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4 ) * 2;

    sampleIndex = 0;
    while ( sampleIndex < _SamplesCount && _DataLength > 4 ) {
        sampleValue = FCore::LittleWord( *( ( short int * )_ADPCM ) );
        tableIndex = _ADPCM[ 2 ];
        if ( tableIndex > 88 ) {
            tableIndex = 88;
        }
        _ADPCM += 4;
        _DataLength -= 4;
        if ( _IgnoreFirstNSamples > 0 ) {
            _IgnoreFirstNSamples--;
        } else {
            _PCM[ sampleIndex++ ] = ( short int )sampleValue;
        }
        for ( int byteIndex = 0; byteIndex < BlockLength && sampleIndex < _SamplesCount && _DataLength; byteIndex++ ) {
            if ( byteIndex & 1 ) {
                delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                _DataLength--;
            } else {
                delta = *_ADPCM & 0x0f;
            }
            int v = IMAUnpackTable[ tableIndex ] >> 3;
            if ( delta & 1 ) {
                v += IMAUnpackTable[ tableIndex ] >> 2;
            }
            if ( delta & 2 ) {
                v += IMAUnpackTable[ tableIndex ] >> 1;
            }
            if ( delta & 4 ) {
                v += IMAUnpackTable[ tableIndex ];
            }
            if ( delta & 8 ) {
                sampleValue -= v;
            } else {
                sampleValue += v;
            }
            tableIndex += IMAIndexTable[ delta & 7 ];
            if ( tableIndex < 0 ) {
                tableIndex = 0;
            } else if ( tableIndex > 88 ) {
                tableIndex = 88;
            }
            if ( sampleValue > 32767 ) {
                sampleValue = 32767;
            } else if ( sampleValue < -32768 ) {
                sampleValue = -32768;
            }
            if ( _IgnoreFirstNSamples > 0 ) {
                _IgnoreFirstNSamples--;
            } else {
                _PCM[ sampleIndex++ ] = ( short int )sampleValue;
            }
        }
    }
    return true;
}

static bool IMAADPCMUnpack16Ext_Stereo( signed short * _PCM, int _IgnoreFirstNSamples, int _SamplesCount, int _ChannelsCount, const byte * _ADPCM, int _DataLength, int _BlockAlign ) {
    int sampleIndex;
    int sampleValue[2];
    int tableIndex[2];
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4*_ChannelsCount ) * 2;
    const int MinDataLength = 4 * _ChannelsCount;

    sampleIndex = 0;
    while ( sampleIndex < _SamplesCount && _DataLength > MinDataLength ) {

        for ( int channel = 0 ; channel < _ChannelsCount ; channel++ ) {
            sampleValue[channel] = FCore::LittleWord( *( ( short int * )_ADPCM ) );
            tableIndex[channel] = _ADPCM[ 2 ];
            if ( tableIndex[channel] > 88 ) {
                tableIndex[channel] = 88;
            }
            _ADPCM += 4;
            _DataLength -= 4;
            // NOTE: Если откомментировать, то слышны щелчки
            //if ( _IgnoreFirstNSamples > 0 ) {
            //    _IgnoreFirstNSamples--;
            //} else {
            //    _PCM[ SampleIndex++ ] = ( short int )SampleValue[Channel];
            //}
        }

        for ( int byteIndex = 0; byteIndex < BlockLength ; ) {
            int ignore = 0;
            int offset = 0;
            for ( int channel = 0 ; channel < _ChannelsCount ; channel++ ) {
                ignore = _IgnoreFirstNSamples;
                offset = 0;
                for ( int chunk = 0 ; chunk < 8 ; chunk++, byteIndex++ ) {
                    if ( byteIndex & 1 ) {
                        delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                        _DataLength--;
                    } else {
                        delta = *_ADPCM & 0x0f;
                    }
                    int v = IMAUnpackTable[ tableIndex[channel] ] >> 3;
                    if ( delta & 1 ) {
                        v += IMAUnpackTable[ tableIndex[channel] ] >> 2;
                    }
                    if ( delta & 2 ) {
                        v += IMAUnpackTable[ tableIndex[channel] ] >> 1;
                    }
                    if ( delta & 4 ) {
                        v += IMAUnpackTable[ tableIndex[channel] ];
                    }
                    if ( delta & 8 ) {
                        sampleValue[channel] -= v;
                    } else {
                        sampleValue[channel] += v;
                    }
                    tableIndex[channel] += IMAIndexTable[ delta & 7 ];
                    if ( tableIndex[channel] < 0 ) {
                        tableIndex[channel] = 0;
                    } else if ( tableIndex[channel] > 88 ) {
                        tableIndex[channel] = 88;
                    }
                    if ( sampleValue[channel] > 32767 ) {
                        sampleValue[channel] = 32767;
                    } else if ( sampleValue[channel] < -32768 ) {
                        sampleValue[channel] = -32768;
                    }

                    if ( ignore > 0 ) {
                        ignore -= 2;
                        offset += 2;
                    } else {
                        int Index = sampleIndex + (chunk>>1)*4 + (chunk&1)*2 + channel - offset;
                        if ( Index < _SamplesCount ) {
                            _PCM[ Index ] = ( short int )sampleValue[channel];
                        }
                    }
                }
            }
            _IgnoreFirstNSamples = ignore;

            assert( _DataLength >= 0 );

            sampleIndex += 16 - offset;
            if ( sampleIndex >= _SamplesCount ) {
                sampleIndex = _SamplesCount;
                break;
            }
        }
    }
    assert( sampleIndex <= _SamplesCount );
    //Out() << Int(SampleIndex) << _SamplesCount;
    return true;
}

// Code based on wave.c from libaudio

/*
 * Copyright 1993 Network Computing Devices, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name Network Computing Devices, Inc. not be
 * used in advertising or publicity pertaining to distribution of this
 * software without specific, written prior permission.
 *
 * THIS SOFTWARE IS PROVIDED 'AS-IS'.  NETWORK COMPUTING DEVICES, INC.,
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT
 * LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NONINFRINGEMENT.  IN NO EVENT SHALL NETWORK
 * COMPUTING DEVICES, INC., BE LIABLE FOR ANY DAMAGES WHATSOEVER, INCLUDING
 * SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES, INCLUDING LOSS OF USE, DATA,
 * OR PROFITS, EVEN IF ADVISED OF THE POSSIBILITY THEREOF, AND REGARDLESS OF
 * WHETHER IN AN ACTION IN CONTRACT, TORT OR NEGLIGENCE, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $NCDId: @(#)wave.c,v 1.26 1996/04/29 21:48:08 greg Exp $
 */


#define RIFF_RiffID			"RIFF"
#define RIFF_WaveID			"WAVE"
#define RIFF_ListID			"LIST"
#define RIFF_ListInfoID	    "INFO"
#define RIFF_InfoIcmtID		"ICMT"
#define RIFF_WaveFmtID		"fmt "
#define RIFF_WaveFmtSize	16
#define RIFF_WaveDataID		"data"

typedef uint32_t RIFF_FOURCC;

struct RiffChunk {
    RIFF_FOURCC Id;
    int32_t SizeInBytes;
};

#define CheckID(_x, _y) strncmp((const char *) (_x), (const char *) (_y), sizeof(RIFF_FOURCC))
#define PAD2(_x)	    (((_x) + 1) & ~1)

template< typename T >
static int ReadChunk( FStreamBase< T > & _File, RiffChunk * _Chunk ) {
    _File.Read( _Chunk, sizeof( RiffChunk ) );
    if ( _File.GetReadBytesCount() ) {
        _Chunk->SizeInBytes = FCore::LittleDWord( _Chunk->SizeInBytes );
    }
    return _File.GetReadBytesCount();
}

template< typename T >
static bool WaveReadHeader( FStreamBase< T > & _File, SWaveFormat & _Wave ) {
    RiffChunk       chunk;
    RIFF_FOURCC     id;
    int32_t         fileSize;
    SWaveFormat *   wave = &_Wave;

    wave->DataBase = 0;
    wave->Format = 0;

    if ( !ReadChunk( _File, &chunk ) ) {
        return false;
    }

    if ( CheckID( &chunk.Id, RIFF_RiffID ) ) {
        return false;
    }

    _File.Read( &id, sizeof( RIFF_FOURCC ) );
    if ( !_File.GetReadBytesCount() ) {
        return false;
    }

    if ( CheckID( &id, RIFF_WaveID ) ) {
        return false;
    }

    fileSize = PAD2( chunk.SizeInBytes ) - sizeof( RIFF_FOURCC );

    bool bHasFormat = false;
    bool bHasData = false;

    while ( fileSize >= ( int32_t )sizeof( RiffChunk ) ) {
        if ( bHasData && bHasFormat ) {
            break;
        }

        if ( !ReadChunk( _File, &chunk ) )
            return false;

        fileSize -= sizeof( RiffChunk ) + PAD2( chunk.SizeInBytes );

        /* LIST chunk */
        if ( !CheckID( &chunk.Id, RIFF_ListID ) ) {

            _File.Read( &id, sizeof( RIFF_FOURCC ) );
            if ( !_File.GetReadBytesCount() ) {
                return false;
            }

            /* INFO chunk */
            if ( !CheckID( &id, RIFF_ListInfoID ) ) {
                chunk.SizeInBytes -= sizeof( RIFF_FOURCC );

                while ( chunk.SizeInBytes ) {
                    RiffChunk       c;

                    if ( !ReadChunk( _File, &c ) )
                        return false;

                    /* skip unknown chunk */
                    _File.SeekCur( PAD2( c.SizeInBytes ) );

                    chunk.SizeInBytes -= sizeof( RiffChunk ) + PAD2( c.SizeInBytes );
                }
            } else {
                /* skip unknown chunk */
                _File.SeekCur( PAD2( chunk.SizeInBytes ) - sizeof( RIFF_FOURCC ) );
            }
        }
        /* wave format chunk */
        else if ( !CheckID( &chunk.Id, RIFF_WaveFmtID ) && !wave->Format ) {
            Int Dummy;

            _File >> wave->Format;
            _File >> wave->Channels;
            _File >> wave->SampleRate;
            _File >> Dummy;
            _File >> wave->BlockAlign;

            if ( wave->Format != WAVE_FORMAT_PCM && wave->Format != WAVE_FORMAT_DVI_ADPCM ) {
                return false;
            }

            _File >> wave->BitsPerSample;

            /* skip any other format specific fields */
            _File.SeekCur( PAD2( chunk.SizeInBytes - 16 ) );

            bHasFormat = true;
        }
        /* wave data chunk */
        else if ( !CheckID( &chunk.Id, RIFF_WaveDataID ) && !wave->DataBase ) {
            long endOfFile;

            wave->DataBase = _File.Tell();
            wave->DataSize = chunk.SizeInBytes;
            _File.SeekEnd( 0 );
            endOfFile = _File.Tell();

            if ( _File.SeekSet( wave->DataBase + PAD2( chunk.SizeInBytes ) ) || _File.Tell() > endOfFile ) {
                /* the seek failed, assume the size is bogus */
                _File.SeekEnd( 0 );
                wave->DataSize = _File.Tell() - wave->DataBase;
            }

            wave->DataBase -= sizeof( long );

            bHasData = true;
        } else {
            /* skip unknown chunk */
            _File.SeekCur( PAD2( chunk.SizeInBytes ) );
        }
    }

    if ( !wave->DataBase ) {
        return false;
    }

    if ( wave->Format == WAVE_FORMAT_DVI_ADPCM ) {

        if ( wave->BitsPerSample != 4 ) {   // Other bits per samples are not supported
            return false;
        }

        wave->SamplesPerBlock = ( wave->BlockAlign - 4 * wave->Channels ) * 2;
        wave->BlockLength = wave->BlockAlign;
        wave->BlocksCount = wave->DataSize / wave->BlockLength;
        wave->NumSamples = wave->SamplesPerBlock * wave->BlocksCount;
        wave->DataSize = wave->BlocksCount * wave->BlockLength;    // align data size
    } else {
        wave->NumSamples = wave->DataSize / ( wave->BitsPerSample >> 3 );

        // correct data size
        wave->DataSize = wave->NumSamples * ( wave->BitsPerSample >> 3 );
    }

    WaveRewindFile( _File, wave );

    return true;
}

template< typename T >
static int WaveReadFile( FStreamBase< T > & _File, void * _Buffer, int _BufferLength, SWaveFormat *_Wave ) {
    _File.Read( _Buffer, _BufferLength );
    return _File.GetReadBytesCount();
}

template< typename T >
static void WaveRewindFile( FStreamBase< T > & _File, SWaveFormat *_Wave ) {
    _File.SeekSet( _Wave->DataBase + sizeof( long ) );
}

template< typename T >
static int WaveSeekFile( FStreamBase< T > & _File, int _Offset, SWaveFormat *_Wave ) {
    return _File.SeekSet( _Wave->DataBase + sizeof( long ) + _Offset );
}

//int
//WaveTellFile(FWaveFormat *_Wave)
//{
//    return ftell(_Wave->fp) - _Wave->DataBase - sizeof(long);
//}
//
//int
//WaveFlushFile(FWaveFormat *_Wave)
//{
//    return fflush(_Wave->fp);
//}
