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

#include <World/Public/Audio/AudioCodec/WavDecoder.h>
#include <Core/Public/Alloc.h>
#include <Core/Public/IO.h>
#include <Core/Public/Logger.h>

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

static bool ReadWaveHeader( IStreamBase & _File, SWaveFormat & _Wave );
static int WaveRead( IStreamBase & _File, void * _Buffer, int _SizeInBytes, SWaveFormat *_Wave );
static void WaveRewind( IStreamBase & _File, SWaveFormat *_Wave );
static int WaveSeek( IStreamBase & _File, int _Offset, SWaveFormat *_Wave );

AN_CLASS_META( AWavAudioTrack )
AN_CLASS_META( AWavDecoder )

AWavAudioTrack::AWavAudioTrack() {
    WaveMemory = NULL;
    PCMDataOffset = 0;
    CurrentSample = 0;
    ADPCM = NULL;
    ADPCMBufferLength = 0;
}

AWavAudioTrack::~AWavAudioTrack() {
    GZoneMemory.Dealloc( ADPCM );
}

bool AWavAudioTrack::InitializeFileStream( const char * _FileName ) {
    AN_ASSERT( !File.IsOpened() );
    AN_ASSERT( WaveMemory == NULL );

    if ( !File.OpenRead( _FileName ) ) {
        return false;
    }

    if ( !ReadWaveHeader( File, Wave ) ) {
        File.Close();
        return false;
    }

    if ( WaveSeek( File, 0, &Wave ) != 0 ) {
        File.Close();
        return false;
    }

    PCMDataOffset = 0;
    CurrentSample = 0;

    return true;
}

bool AWavAudioTrack::InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) {
    AN_ASSERT( !File.IsOpened() );
    AN_ASSERT( WaveMemory == NULL );

    Wave = *(SWaveFormat *)_EncodedData;
    WaveMemory = _EncodedData + sizeof( SWaveFormat );
    PCMDataOffset = 0;
    CurrentSample = 0;

    AN_ASSERT( Wave.DataSize == _EncodedDataLength - sizeof( SWaveFormat ) );

    return true;
}

void AWavAudioTrack::StreamRewind() {
    PCMDataOffset = 0;
    CurrentSample = 0;

    if ( File.IsOpened() ) {
        WaveRewind( File, &Wave );
    }
}

void AWavAudioTrack::StreamSeek( int _PositionInSamples ) {
    if ( WaveMemory ) {
        if ( Wave.Format == WAVE_FORMAT_PCM ) {
            int bytesPerSample = Wave.BitsPerSample >> 3;

            CurrentSample = Math::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );

            PCMDataOffset = CurrentSample * bytesPerSample;

            //PCMDataOffset = CurrentSample * BytesPerSample;
            //if ( PCMDataOffset > Wave.DataSize ) {
            //    PCMDataOffset = Wave.DataSize;
            //}
        } else if ( Wave.Format == WAVE_FORMAT_DVI_ADPCM ) {
            CurrentSample = Math::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );
        }

    } else if ( File.IsOpened() ) {
        if ( Wave.Format == WAVE_FORMAT_PCM ) {
            int bytesPerSample = Wave.BitsPerSample >> 3;

            CurrentSample = Math::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );

            PCMDataOffset = CurrentSample * bytesPerSample;

            WaveSeek( File, PCMDataOffset, &Wave );
        } else if ( Wave.Format == WAVE_FORMAT_DVI_ADPCM ) {
            CurrentSample = Math::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );
        }
    }
}

int AWavAudioTrack::StreamDecodePCM( short * _Buffer, int _NumShorts ) {
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
                    AN_ASSERT( (CurrentSample & 1) == 0 );
                    AN_ASSERT( (Wave.SamplesPerBlock & 1) == 0 );
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

                    AN_ASSERT( samplesInsideBlock <= Wave.SamplesPerBlock );

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
                    dataLength = WaveRead( File, (char *)_Buffer, dataLength, &Wave );

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

                    AN_ASSERT( samplesInsideBlock <= Wave.SamplesPerBlock );

                    if ( samplesInsideBlock == 0 ) {
                        blocksCount--;
                        samplesInsideBlock = Wave.SamplesPerBlock;
                    }

                    int readBytesCount = blocksCount * Wave.BlockLength;
                    if ( ADPCMBufferLength < readBytesCount ) {
                        ADPCM = ( byte * )GZoneMemory.Extend( ADPCM, ADPCMBufferLength, readBytesCount, 1, false );
                        ADPCMBufferLength = readBytesCount;
                    }

                    WaveSeek( File, firstBlockIndex * Wave.BlockLength, &Wave );
                    WaveRead( File, ADPCM, readBytesCount, &Wave );

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

AWavDecoder::AWavDecoder() {

}

IAudioStreamInterface * AWavDecoder::CreateAudioStream() {
    return CreateInstanceOf< AWavAudioTrack >();
}

bool AWavDecoder::DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    AFileStream f;
    SWaveFormat inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;

    if ( _PCM ) *_PCM = NULL;

    if ( !f.OpenRead( _FileName ) ) {
        return false;
    }

    if ( !ReadWaveHeader( f, inf ) ) {
        return false;
    }

    if ( _PCM ) {
        if ( WaveSeek( f, 0, &inf ) != 0 ) {
            return false;
        }

        *_PCM = (short *)GZoneMemory.Alloc( inf.DataSize, 1 );
        if ( !*_PCM ) {
            return false;
        }

        if ( WaveRead( f, (char *)*_PCM, inf.DataSize, &inf ) != (int)inf.DataSize ) {
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

bool AWavDecoder::DecodePCM( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    AMemoryStream f;
    SWaveFormat inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;

    if ( _PCM ) *_PCM = NULL;

    if ( !f.OpenRead( _FileName, _Data, _DataLength ) ) {
        return false;
    }

    if ( !ReadWaveHeader( f, inf ) ) {
        return false;
    }

    if ( _PCM ) {
        if ( WaveSeek( f, 0, &inf ) != 0 ) {
            return false;
        }

        *_PCM = (short *)GZoneMemory.Alloc( inf.DataSize, 1 );
        if ( !*_PCM ) {
            return false;
        }

        if ( WaveRead( f, (char *)*_PCM, inf.DataSize, &inf ) != (int)inf.DataSize ) {
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

bool AWavDecoder::ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    AFileStream f;
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

    if ( !ReadWaveHeader( f, inf ) ) {
        return false;
    }

    if ( WaveSeek( f, 0, &inf ) != 0 ) {
        return false;
    }

    *_EncodedData = ( byte * )GZoneMemory.Alloc( inf.DataSize + sizeof( SWaveFormat ), 1 );
    if ( !*_EncodedData ) {
        return false;
    }

    if ( WaveRead( f, ( char * )*_EncodedData + sizeof( SWaveFormat ), inf.DataSize, &inf ) != (int)inf.DataSize ) {
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

bool AWavDecoder::ReadEncoded( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    AMemoryStream f;
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

    if ( !ReadWaveHeader( f, inf ) ) {
        return false;
    }

    if ( WaveSeek( f, 0, &inf ) != 0 ) {
        return false;
    }

    *_EncodedData = ( byte * )GZoneMemory.Alloc( inf.DataSize + sizeof( SWaveFormat ), 1 );
    if ( !*_EncodedData ) {
        return false;
    }

    if ( WaveRead( f, ( char * )*_EncodedData + sizeof( SWaveFormat ), inf.DataSize, &inf ) != (int)inf.DataSize ) {
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
        sampleValue = Core::LittleWord( *( ( short int * )_ADPCM ) );
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
            sampleValue[channel] = Core::LittleWord( *( ( short int * )_ADPCM ) );
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

            AN_ASSERT( _DataLength >= 0 );

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
        sampleValue = Core::LittleWord( *( ( short int * )_ADPCM ) );
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
            sampleValue[channel] = Core::LittleWord( *( ( short int * )_ADPCM ) );
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

            AN_ASSERT( _DataLength >= 0 );

            sampleIndex += 16 - offset;
            if ( sampleIndex >= _SamplesCount ) {
                sampleIndex = _SamplesCount;
                break;
            }
        }
    }
    AN_ASSERT( sampleIndex <= _SamplesCount );

    return true;
}

static bool ReadWaveHeader( IStreamBase & InFile, SWaveFormat & Wave ) {
    struct SRiffChunk {
        uint32_t Id;
        int32_t SizeInBytes;
    };

    SRiffChunk chunk;
    uint32_t type;
    int32_t paddedSize;

    chunk.Id = InFile.ReadUInt32();
    chunk.SizeInBytes = InFile.ReadInt32();

    paddedSize = Align( chunk.SizeInBytes, 2 );

    if ( memcmp( &chunk.Id, "RIFF", 4 ) ) {
        GLogger.Printf( "AWavAudioTrack: Unexpected chunk id (expected RIFF)\n" );
        return false;
    }

    type = InFile.ReadUInt32();

    if ( memcmp( &type, "WAVE", 4 ) ) {
        GLogger.Printf( "AWavAudioTrack: Expected WAVE list\n" );
        return false;
    }

    int64_t curSize = paddedSize;

    curSize -= sizeof( uint32_t );

    memset( &Wave, 0, sizeof( Wave ) );

    while ( curSize >= sizeof( SRiffChunk ) ) {

        chunk.Id = InFile.ReadUInt32();
        chunk.SizeInBytes = InFile.ReadInt32();

        paddedSize = Align( chunk.SizeInBytes, 2 );

        curSize -= sizeof( SRiffChunk );
        curSize -= paddedSize;

        long offset = InFile.Tell();

        if ( !memcmp( &chunk.Id, "fmt ", 4 ) && !Wave.Format ) {
            Wave.Format = InFile.ReadInt16();
            Wave.Channels = InFile.ReadInt16();
            Wave.SampleRate = InFile.ReadInt32();
            InFile.ReadInt32(); // byte rate
            Wave.BlockAlign = InFile.ReadInt16();
            Wave.BitsPerSample = InFile.ReadInt16();
        }
        else if ( !memcmp( &chunk.Id, "data", 4 ) && !Wave.DataBase ) {
            Wave.DataBase = offset;
            Wave.DataSize = chunk.SizeInBytes;
        }

        if ( Wave.Format && Wave.DataBase ) {
            break;
        }

        // Move to next chunk
        InFile.SeekSet( offset + paddedSize );
    }

    const uint32_t minDataSize = 4;
    if ( Wave.DataBase < minDataSize ) {
        GLogger.Printf( "AWavAudioTrack: Audio data was not found\n" );
        return false;
    }

    InFile.SeekEnd( 0 );
    long fileLen = InFile.Tell();

    if ( Wave.DataBase + Wave.DataSize > fileLen ) {
        GLogger.Printf( "AWavAudioTrack: Audio size is bogus\n" );
        return false;
    }

    if ( Wave.Format == WAVE_FORMAT_DVI_ADPCM ) {

        if ( Wave.BitsPerSample != 4 ) {
            GLogger.Printf( "AWavAudioTrack: Expected 4 bits per sample for DVI ADPCM format\n" );
            return false;
        }

        Wave.SamplesPerBlock = ( Wave.BlockAlign - 4 * Wave.Channels ) * 2;
        Wave.BlockLength = Wave.BlockAlign;
        Wave.BlocksCount = Wave.DataSize / Wave.BlockLength;
        Wave.NumSamples = Wave.SamplesPerBlock * Wave.BlocksCount;
        Wave.DataSize = Wave.BlocksCount * Wave.BlockLength;    // align data size

    } else if ( Wave.Format == WAVE_FORMAT_PCM ){
        Wave.NumSamples = Wave.DataSize / ( Wave.BitsPerSample >> 3 );

        // correct data size
        Wave.DataSize = Wave.NumSamples * ( Wave.BitsPerSample >> 3 );

    } else {
        GLogger.Printf( "AWavAudioTrack: Unexpected audio format (only PCM, DVI ADPCM supported)\n" );
        return false;
    }

    WaveRewind( InFile, &Wave );

    return true;
}

static int WaveRead( IStreamBase & _File, void * _Buffer, int _SizeInBytes, SWaveFormat * _Wave ) {
    _File.ReadBuffer( _Buffer, _SizeInBytes );
    return _File.GetReadBytesCount();
}

static void WaveRewind( IStreamBase & _File, SWaveFormat * _Wave ) {
    _File.SeekSet( _Wave->DataBase );
}

static int WaveSeek( IStreamBase & _File, int _Offset, SWaveFormat * _Wave ) {
    return _File.SeekSet( _Wave->DataBase + _Offset );
}
