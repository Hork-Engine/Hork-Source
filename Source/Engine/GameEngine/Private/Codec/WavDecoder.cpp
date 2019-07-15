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

#include <Engine/GameEngine/Public/Codec/WavDecoder.h>
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
static bool WaveReadHeader( FStreamBase< T > & _File, FWaveFormat & _Wave );
template< typename T >
static int WaveReadFile( FStreamBase< T > & _File, void * _Buffer, int _BufferLength, FWaveFormat *_Wave );
template< typename T >
static void WaveRewindFile( FStreamBase< T > & _File, FWaveFormat *_Wave );
template< typename T >
static int WaveSeekFile( FStreamBase< T > & _File, int _Offset, FWaveFormat *_Wave );

AN_CLASS_META_NO_ATTRIBS( FWavAudioTrack )
AN_CLASS_META_NO_ATTRIBS( FWavDecoder )

FWavAudioTrack::FWavAudioTrack() {
    WaveMemory = NULL;
    PCMDataOffset = 0;
    CurrentSample = 0;
    ADPCM = NULL;
    ADPCMBufferLength = 0;
}

FWavAudioTrack::~FWavAudioTrack() {
    GMainMemoryZone.Dealloc( ADPCM );
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

    Wave = *(FWaveFormat *)_EncodedData;
    WaveMemory = _EncodedData + sizeof( FWaveFormat );
    PCMDataOffset = 0;
    CurrentSample = 0;

    assert( Wave.DataSize == _EncodedDataLength - sizeof( FWaveFormat ) );

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
            int BytesPerSample = Wave.BitsPerSample >> 3;

            CurrentSample = FMath::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );

            PCMDataOffset = CurrentSample * BytesPerSample;

            //PCMDataOffset = CurrentSample * BytesPerSample;
            //if ( PCMDataOffset > Wave.DataSize ) {
            //    PCMDataOffset = Wave.DataSize;
            //}
        } else if ( Wave.Format == WAVE_FORMAT_DVI_ADPCM ) {
            CurrentSample = FMath::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );
        }

    } else if ( File.IsOpened() ) {
        if ( Wave.Format == WAVE_FORMAT_PCM ) {
            int BytesPerSample = Wave.BitsPerSample >> 3;

            CurrentSample = FMath::Min( _PositionInSamples * Wave.Channels, (int)Wave.NumSamples );

            PCMDataOffset = CurrentSample * BytesPerSample;

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
                int DataLength = _NumShorts * 2;
                if ( PCMDataOffset + DataLength > Wave.DataSize ) {
                    DataLength = Wave.DataSize - PCMDataOffset;
                }
                if ( DataLength > 0 ) {
                    memcpy( _Buffer, WaveMemory + PCMDataOffset, DataLength );
                    PCMDataOffset += DataLength;
                } else {
                    return 0;
                }

                int SamplesCount = DataLength / (Wave.BitsPerSample >> 3);

                CurrentSample += SamplesCount;

                return SamplesCount;
            }
            case WAVE_FORMAT_DVI_ADPCM: {

                if ( Wave.Channels == 2 ) {
                    assert( (CurrentSample & 1) == 0 );
                    assert( (Wave.SamplesPerBlock & 1) == 0 );
                }

                int LastSample = CurrentSample + _NumShorts;
                if ( LastSample > Wave.NumSamples ) {
                    LastSample = Wave.NumSamples;
                }

                int NumSamples = LastSample - CurrentSample;

                if ( NumSamples > 0 ) {
                    int FirstBlockIndex = CurrentSample / Wave.SamplesPerBlock;
                    int LastBlockIndex = LastSample / Wave.SamplesPerBlock;
                    int BlocksCount = LastBlockIndex - FirstBlockIndex + 1;
                    int SamplesInsideBlock = LastSample - LastBlockIndex * Wave.SamplesPerBlock;

                    assert( SamplesInsideBlock <= Wave.SamplesPerBlock );

                    if ( SamplesInsideBlock == 0 ) {
                        BlocksCount--;
                        SamplesInsideBlock = Wave.SamplesPerBlock;
                    }

                    int SamplesCount = (BlocksCount - 1) * Wave.SamplesPerBlock + SamplesInsideBlock;

#if 1
                    if ( Wave.Channels == 2 ) {
                        IMAADPCMUnpack16Ext_Stereo( _Buffer, SamplesCount - NumSamples, NumSamples, Wave.Channels, WaveMemory + FirstBlockIndex * Wave.BlockLength, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    } else {
                        IMAADPCMUnpack16Ext_Mono( _Buffer, SamplesCount - NumSamples, NumSamples, WaveMemory + FirstBlockIndex * Wave.BlockLength, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    }

#else
                    short * PCM = ( short * )GMainHunkMemory.HunkMemory( sizeof( short ) * SamplesCount );
                    IMAADPCMUnpack16_Mono( PCM, SamplesCount, WaveMemory + FirstBlockIndex * Wave.BlockLength, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    memcpy( _Buffer, PCM + (SamplesCount - NumSamples), NumSamples * 2 );
                    GMainHunkMemory.ClearLastHunk();
#endif

                    CurrentSample += NumSamples;
                } else {
                    return 0;
                }

                return NumSamples;
            }
        }

        return 0;
    } else if ( File.IsOpened() ) {

        switch ( Wave.Format ) {
            case WAVE_FORMAT_PCM: {
                int DataLength = _NumShorts * 2;
                if ( PCMDataOffset + DataLength > Wave.DataSize ) {
                    DataLength = Wave.DataSize - PCMDataOffset;
                }
                if ( DataLength > 0 ) {
                    DataLength = WaveReadFile( File, (char *)_Buffer, DataLength, &Wave );

                    PCMDataOffset += DataLength;
                } else {
                    return 0;
                }

                int SamplesCount = DataLength / (Wave.BitsPerSample >> 3);

                CurrentSample += SamplesCount;

                return SamplesCount;
            }
            case WAVE_FORMAT_DVI_ADPCM: {

                int LastSample = CurrentSample + _NumShorts;
                if ( LastSample > Wave.NumSamples ) {
                    LastSample = Wave.NumSamples;
                }

                int NumSamples = LastSample - CurrentSample;

                if ( NumSamples > 0 ) {
                    int FirstBlockIndex = CurrentSample / Wave.SamplesPerBlock;
                    int LastBlockIndex = LastSample / Wave.SamplesPerBlock;
                    int BlocksCount = LastBlockIndex - FirstBlockIndex + 1;
                    int SamplesInsideBlock = LastSample - LastBlockIndex * Wave.SamplesPerBlock;

                    assert( SamplesInsideBlock <= Wave.SamplesPerBlock );

                    if ( SamplesInsideBlock == 0 ) {
                        BlocksCount--;
                        SamplesInsideBlock = Wave.SamplesPerBlock;
                    }

                    int ReadBytesCount = BlocksCount * Wave.BlockLength;
                    if ( ADPCMBufferLength < ReadBytesCount ) {
                        ADPCM = ( byte * )GMainMemoryZone.Extend( ADPCM, ADPCMBufferLength, ReadBytesCount, 1, false );
                        ADPCMBufferLength = ReadBytesCount;
                    }

                    WaveSeekFile( File, FirstBlockIndex * Wave.BlockLength, &Wave );
                    WaveReadFile( File, ADPCM, ReadBytesCount, &Wave );

                    int SamplesCount = (BlocksCount - 1) * Wave.SamplesPerBlock + SamplesInsideBlock;

#if 1
                    if ( Wave.Channels == 2 ) {
                        IMAADPCMUnpack16Ext_Stereo( _Buffer, SamplesCount - NumSamples, NumSamples, Wave.Channels, ADPCM, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    } else {
                        IMAADPCMUnpack16Ext_Mono( _Buffer, SamplesCount - NumSamples, NumSamples, ADPCM, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    }
#else
                    short * PCM = ( short * )GMainHunkMemory.HunkMemory( sizeof( short ) * SamplesCount );
                    IMAADPCMUnpack16_Mono( PCM, SamplesCount, ADPCM, BlocksCount * Wave.BlockLength, Wave.BlockAlign );
                    memcpy( _Buffer, PCM + (SamplesCount - NumSamples), NumSamples * 2 );
                    GMainHunkMemory.ClearLastHunk();
#endif

                    CurrentSample += NumSamples;
                } else {
                    return 0;
                }

                return NumSamples;
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
    FWaveFormat Inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;

    if ( _PCM ) *_PCM = NULL;

    if ( !f.OpenRead( _FileName ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, Inf ) ) {
        return false;
    }

    if ( _PCM ) {
        if ( WaveSeekFile( f, 0, &Inf ) != 0 ) {
            return false;
        }

        *_PCM = (short *)GMainMemoryZone.Alloc( Inf.DataSize, 1 );
        if ( !*_PCM ) {
            return false;
        }

        if ( WaveReadFile( f, (char *)*_PCM, Inf.DataSize, &Inf ) != (int)Inf.DataSize ) {
            GMainMemoryZone.Dealloc( *_PCM );
            *_PCM = NULL;
            return false;
        }

        if ( Inf.Format == WAVE_FORMAT_DVI_ADPCM ) {
            byte * EncodedADPCM = (byte *)*_PCM;
            *_PCM = (short *)GMainMemoryZone.Alloc( Inf.NumSamples * sizeof( short ), 1 );
            if ( Inf.Channels == 2 )
                IMAADPCMUnpack16_Stereo( *_PCM, Inf.NumSamples, Inf.Channels, EncodedADPCM, Inf.DataSize, Inf.BlockAlign );
            else
                IMAADPCMUnpack16_Mono( *_PCM, Inf.NumSamples, EncodedADPCM, Inf.DataSize, Inf.BlockAlign );
            GMainMemoryZone.Dealloc( EncodedADPCM );
        }
    }

    *_SamplesCount = Inf.NumSamples / Inf.Channels;
    *_Channels = Inf.Channels;
    *_SampleRate = Inf.SampleRate;
    *_BitsPerSample = Inf.BitsPerSample;

    return true;
}

bool FWavDecoder::DecodePCM( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    FMemoryStream f;
    FWaveFormat Inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;

    if ( _PCM ) *_PCM = NULL;

    if ( !f.OpenRead( _FileName, _Data, _DataLength ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, Inf ) ) {
        return false;
    }

    if ( _PCM ) {
        if ( WaveSeekFile( f, 0, &Inf ) != 0 ) {
            return false;
        }

        *_PCM = (short *)GMainMemoryZone.Alloc( Inf.DataSize, 1 );
        if ( !*_PCM ) {
            return false;
        }

        if ( WaveReadFile( f, (char *)*_PCM, Inf.DataSize, &Inf ) != (int)Inf.DataSize ) {
            GMainMemoryZone.Dealloc( *_PCM );
            *_PCM = NULL;
            return false;
        }

        if ( Inf.Format == WAVE_FORMAT_DVI_ADPCM ) {
            byte * EncodedADPCM = (byte *)*_PCM;
            *_PCM = (short *)GMainMemoryZone.Alloc( Inf.NumSamples * sizeof( short ), 1 );
            if ( Inf.Channels == 2 )
                IMAADPCMUnpack16_Stereo( *_PCM, Inf.NumSamples, Inf.Channels, EncodedADPCM, Inf.DataSize, Inf.BlockAlign );
            else
                IMAADPCMUnpack16_Mono( *_PCM, Inf.NumSamples, EncodedADPCM, Inf.DataSize, Inf.BlockAlign );
            GMainMemoryZone.Dealloc( EncodedADPCM );
        }
    }

    *_SamplesCount = Inf.NumSamples / Inf.Channels;
    *_Channels = Inf.Channels;
    *_SampleRate = Inf.SampleRate;
    *_BitsPerSample = Inf.BitsPerSample;

    return true;
}

bool FWavDecoder::ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    FFileStream f;
    FWaveFormat Inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;

    if ( !f.OpenRead( _FileName ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, Inf ) ) {
        return false;
    }

    if ( WaveSeekFile( f, 0, &Inf ) != 0 ) {
        return false;
    }

    *_EncodedData = ( byte * )GMainMemoryZone.Alloc( Inf.DataSize + sizeof( FWaveFormat ), 1 );
    if ( !*_EncodedData ) {
        return false;
    }

    if ( WaveReadFile( f, ( char * )*_EncodedData + sizeof( FWaveFormat ), Inf.DataSize, &Inf ) != (int)Inf.DataSize ) {
        GMainMemoryZone.Dealloc( *_EncodedData );
        *_EncodedData = NULL;
        return false;
    }

    memcpy( *_EncodedData, &Inf, sizeof( FWaveFormat ) );

    *_EncodedDataLength = Inf.DataSize + sizeof( FWaveFormat );
    *_SamplesCount = Inf.NumSamples / Inf.Channels;
    *_Channels = Inf.Channels;
    *_SampleRate = Inf.SampleRate;
    *_BitsPerSample = Inf.BitsPerSample;

    return true;
}

bool FWavDecoder::ReadEncoded( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    FMemoryStream f;
    FWaveFormat Inf;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;

    if ( !f.OpenRead( _FileName, _Data, _DataLength ) ) {
        return false;
    }

    if ( !WaveReadHeader( f, Inf ) ) {
        return false;
    }

    if ( WaveSeekFile( f, 0, &Inf ) != 0 ) {
        return false;
    }

    *_EncodedData = ( byte * )GMainMemoryZone.Alloc( Inf.DataSize + sizeof( FWaveFormat ), 1 );
    if ( !*_EncodedData ) {
        return false;
    }

    if ( WaveReadFile( f, ( char * )*_EncodedData + sizeof( FWaveFormat ), Inf.DataSize, &Inf ) != (int)Inf.DataSize ) {
        GMainMemoryZone.Dealloc( *_EncodedData );
        *_EncodedData = NULL;
        return false;
    }

    memcpy( *_EncodedData, &Inf, sizeof( FWaveFormat ) );

    *_EncodedDataLength = Inf.DataSize + sizeof( FWaveFormat );
    *_SamplesCount = Inf.NumSamples / Inf.Channels;
    *_Channels = Inf.Channels;
    *_SampleRate = Inf.SampleRate;
    *_BitsPerSample = Inf.BitsPerSample;

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
    int SampleIndex;
    int SampleValue;
    int TableIndex;
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4 ) * 2;

    SampleIndex = 0;
    while ( SampleIndex < _SamplesCount && _DataLength > 4 ) {
        SampleValue = FCore::LittleWord( *( ( short int * )_ADPCM ) );
        TableIndex = _ADPCM[ 2 ];
        if ( TableIndex > 88 ) {
            TableIndex = 88;
        }
        _ADPCM += 4;
        _DataLength -= 4;
        _PCM[ SampleIndex++ ] = ( short int )SampleValue;
        for ( int ByteIndex = 0; ByteIndex < BlockLength && SampleIndex < _SamplesCount && _DataLength; ByteIndex++ ) {
            if ( ByteIndex & 1 ) {
                delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                _DataLength--;
            } else {
                delta = *_ADPCM & 0x0f;
            }
            int v = IMAUnpackTable[ TableIndex ] >> 3;
            if ( delta & 1 ) {
                v += IMAUnpackTable[ TableIndex ] >> 2;
            }
            if ( delta & 2 ) {
                v += IMAUnpackTable[ TableIndex ] >> 1;
            }
            if ( delta & 4 ) {
                v += IMAUnpackTable[ TableIndex ];
            }
            if ( delta & 8 ) {
                SampleValue -= v;
            } else {
                SampleValue += v;
            }
            TableIndex += IMAIndexTable[ delta & 7 ];
            if ( TableIndex < 0 ) {
                TableIndex = 0;
            } else if ( TableIndex > 88 ) {
                TableIndex = 88;
            }
            if ( SampleValue > 32767 ) {
                SampleValue = 32767;
            } else if ( SampleValue < -32768 ) {
                SampleValue = -32768;
            }
            _PCM[ SampleIndex++ ] = ( short int )SampleValue;
        }
    }
    return true;
}

static bool IMAADPCMUnpack16_Stereo( signed short * _PCM, int _SamplesCount, int _ChannelsCount, const byte * _ADPCM, int _DataLength, int _BlockAlign ) {
    int SampleIndex;
    int SampleValue[2];
    int TableIndex[2];
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4*_ChannelsCount ) * 2;
    const int MinDataLength = 4 * _ChannelsCount;

    SampleIndex = 0;
    while ( SampleIndex < _SamplesCount && _DataLength > MinDataLength ) {

        for ( int Channel = 0 ; Channel < _ChannelsCount ; Channel++ ) {
            SampleValue[Channel] = FCore::LittleWord( *( ( short int * )_ADPCM ) );
            TableIndex[Channel] = _ADPCM[ 2 ];
            if ( TableIndex[Channel] > 88 ) {
                TableIndex[Channel] = 88;
            }
            _ADPCM += 4;
            _DataLength -= 4;

            // FIXME: закомментировать?
            _PCM[ SampleIndex++ ] = ( short int )SampleValue[Channel];
        }

        for ( int ByteIndex = 0; ByteIndex < BlockLength ; ) {
            for ( int Channel = 0 ; Channel < _ChannelsCount ; Channel++ ) {
                for ( int Chunk = 0 ; Chunk < 8 ; Chunk++, ByteIndex++ ) {
                    if ( ByteIndex & 1 ) {
                        delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                        _DataLength--;
                    } else {
                        delta = *_ADPCM & 0x0f;
                    }
                    int v = IMAUnpackTable[ TableIndex[Channel] ] >> 3;
                    if ( delta & 1 ) {
                        v += IMAUnpackTable[ TableIndex[Channel] ] >> 2;
                    }
                    if ( delta & 2 ) {
                        v += IMAUnpackTable[ TableIndex[Channel] ] >> 1;
                    }
                    if ( delta & 4 ) {
                        v += IMAUnpackTable[ TableIndex[Channel] ];
                    }
                    if ( delta & 8 ) {
                        SampleValue[Channel] -= v;
                    } else {
                        SampleValue[Channel] += v;
                    }
                    TableIndex[Channel] += IMAIndexTable[ delta & 7 ];
                    if ( TableIndex[Channel] < 0 ) {
                        TableIndex[Channel] = 0;
                    } else if ( TableIndex[Channel] > 88 ) {
                        TableIndex[Channel] = 88;
                    }
                    if ( SampleValue[Channel] > 32767 ) {
                        SampleValue[Channel] = 32767;
                    } else if ( SampleValue[Channel] < -32768 ) {
                        SampleValue[Channel] = -32768;
                    }
                    int Index = SampleIndex + (Chunk>>1)*4 + (Chunk&1)*2 + Channel;
                    if ( Index < _SamplesCount ) {
                        _PCM[ Index ] = ( short int )SampleValue[Channel];
                    }
                }
            }

            assert( _DataLength >= 0 );

            SampleIndex += 16;
            if ( SampleIndex >= _SamplesCount ) {
                SampleIndex = _SamplesCount;
                break;
            }
        }
    }

    return true;
}

static bool IMAADPCMUnpack16Ext_Mono( signed short * _PCM, int _IgnoreFirstNSamples, int _SamplesCount, const byte * _ADPCM, int _DataLength, int _BlockAlign ) {
    int SampleIndex;
    int SampleValue;
    int TableIndex;
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4 ) * 2;

    SampleIndex = 0;
    while ( SampleIndex < _SamplesCount && _DataLength > 4 ) {
        SampleValue = FCore::LittleWord( *( ( short int * )_ADPCM ) );
        TableIndex = _ADPCM[ 2 ];
        if ( TableIndex > 88 ) {
            TableIndex = 88;
        }
        _ADPCM += 4;
        _DataLength -= 4;
        if ( _IgnoreFirstNSamples > 0 ) {
            _IgnoreFirstNSamples--;
        } else {
            _PCM[ SampleIndex++ ] = ( short int )SampleValue;
        }
        for ( int ByteIndex = 0; ByteIndex < BlockLength && SampleIndex < _SamplesCount && _DataLength; ByteIndex++ ) {
            if ( ByteIndex & 1 ) {
                delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                _DataLength--;
            } else {
                delta = *_ADPCM & 0x0f;
            }
            int v = IMAUnpackTable[ TableIndex ] >> 3;
            if ( delta & 1 ) {
                v += IMAUnpackTable[ TableIndex ] >> 2;
            }
            if ( delta & 2 ) {
                v += IMAUnpackTable[ TableIndex ] >> 1;
            }
            if ( delta & 4 ) {
                v += IMAUnpackTable[ TableIndex ];
            }
            if ( delta & 8 ) {
                SampleValue -= v;
            } else {
                SampleValue += v;
            }
            TableIndex += IMAIndexTable[ delta & 7 ];
            if ( TableIndex < 0 ) {
                TableIndex = 0;
            } else if ( TableIndex > 88 ) {
                TableIndex = 88;
            }
            if ( SampleValue > 32767 ) {
                SampleValue = 32767;
            } else if ( SampleValue < -32768 ) {
                SampleValue = -32768;
            }
            if ( _IgnoreFirstNSamples > 0 ) {
                _IgnoreFirstNSamples--;
            } else {
                _PCM[ SampleIndex++ ] = ( short int )SampleValue;
            }
        }
    }
    return true;
}

static bool IMAADPCMUnpack16Ext_Stereo( signed short * _PCM, int _IgnoreFirstNSamples, int _SamplesCount, int _ChannelsCount, const byte * _ADPCM, int _DataLength, int _BlockAlign ) {
    int SampleIndex;
    int SampleValue[2];
    int TableIndex[2];
    byte delta;

    if ( _SamplesCount < 4 || !_PCM || !_ADPCM || _BlockAlign < 5 || _BlockAlign > _DataLength ) {
        return false;
    }

    const int BlockLength = ( _BlockAlign - 4*_ChannelsCount ) * 2;
    const int MinDataLength = 4 * _ChannelsCount;

    SampleIndex = 0;
    while ( SampleIndex < _SamplesCount && _DataLength > MinDataLength ) {

        for ( int Channel = 0 ; Channel < _ChannelsCount ; Channel++ ) {
            SampleValue[Channel] = FCore::LittleWord( *( ( short int * )_ADPCM ) );
            TableIndex[Channel] = _ADPCM[ 2 ];
            if ( TableIndex[Channel] > 88 ) {
                TableIndex[Channel] = 88;
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

        for ( int ByteIndex = 0; ByteIndex < BlockLength ; ) {
            int Ignore = 0;
            int Offset = 0;
            for ( int Channel = 0 ; Channel < _ChannelsCount ; Channel++ ) {
                Ignore = _IgnoreFirstNSamples;
                Offset = 0;
                for ( int Chunk = 0 ; Chunk < 8 ; Chunk++, ByteIndex++ ) {
                    if ( ByteIndex & 1 ) {
                        delta = ( *( _ADPCM++ ) >> 4 ) & 0x0f;
                        _DataLength--;
                    } else {
                        delta = *_ADPCM & 0x0f;
                    }
                    int v = IMAUnpackTable[ TableIndex[Channel] ] >> 3;
                    if ( delta & 1 ) {
                        v += IMAUnpackTable[ TableIndex[Channel] ] >> 2;
                    }
                    if ( delta & 2 ) {
                        v += IMAUnpackTable[ TableIndex[Channel] ] >> 1;
                    }
                    if ( delta & 4 ) {
                        v += IMAUnpackTable[ TableIndex[Channel] ];
                    }
                    if ( delta & 8 ) {
                        SampleValue[Channel] -= v;
                    } else {
                        SampleValue[Channel] += v;
                    }
                    TableIndex[Channel] += IMAIndexTable[ delta & 7 ];
                    if ( TableIndex[Channel] < 0 ) {
                        TableIndex[Channel] = 0;
                    } else if ( TableIndex[Channel] > 88 ) {
                        TableIndex[Channel] = 88;
                    }
                    if ( SampleValue[Channel] > 32767 ) {
                        SampleValue[Channel] = 32767;
                    } else if ( SampleValue[Channel] < -32768 ) {
                        SampleValue[Channel] = -32768;
                    }

                    if ( Ignore > 0 ) {
                        Ignore -= 2;
                        Offset += 2;
                    } else {
                        int Index = SampleIndex + (Chunk>>1)*4 + (Chunk&1)*2 + Channel - Offset;
                        if ( Index < _SamplesCount ) {
                            _PCM[ Index ] = ( short int )SampleValue[Channel];
                        }
                    }
                }
            }
            _IgnoreFirstNSamples = Ignore;

            assert( _DataLength >= 0 );

            SampleIndex += 16 - Offset;
            if ( SampleIndex >= _SamplesCount ) {
                SampleIndex = _SamplesCount;
                break;
            }
        }
    }
    assert( SampleIndex <= _SamplesCount );
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
    int32_t ByteLength;
};

#define CheckID(_x, _y) strncmp((const char *) (_x), (const char *) (_y), sizeof(RIFF_FOURCC))
#define PAD2(_x)	    (((_x) + 1) & ~1)

template< typename T >
static int ReadChunk( FStreamBase< T > & _File, RiffChunk * _Chunk ) {
    _File.Read( _Chunk, sizeof( RiffChunk ) );
    if ( _File.GetReadBytesCount() ) {
        _Chunk->ByteLength = FCore::LittleDWord( _Chunk->ByteLength );
    }
    return _File.GetReadBytesCount();
}

template< typename T >
static bool WaveReadHeader( FStreamBase< T > & _File, FWaveFormat & _Wave ) {
    RiffChunk       Chunk;
    RIFF_FOURCC     Id;
    int32_t         FileSize;
    FWaveFormat *   Wave = &_Wave;

    Wave->DataBase = 0;
    Wave->Format = 0;

    if ( !ReadChunk( _File, &Chunk ) ) {
        return false;
    }

    if ( CheckID( &Chunk.Id, RIFF_RiffID ) ) {
        return false;
    }

    _File.Read( &Id, sizeof( RIFF_FOURCC ) );
    if ( !_File.GetReadBytesCount() ) {
        return false;
    }

    if ( CheckID( &Id, RIFF_WaveID ) ) {
        return false;
    }

    FileSize = PAD2( Chunk.ByteLength ) - sizeof( RIFF_FOURCC );

    bool bHasFormat = false;
    bool bHasData = false;

    while ( FileSize >= ( int32_t )sizeof( RiffChunk ) ) {
        if ( bHasData && bHasFormat ) {
            break;
        }

        if ( !ReadChunk( _File, &Chunk ) )
            return false;

        FileSize -= sizeof( RiffChunk ) + PAD2( Chunk.ByteLength );

        /* LIST chunk */
        if ( !CheckID( &Chunk.Id, RIFF_ListID ) ) {

            _File.Read( &Id, sizeof( RIFF_FOURCC ) );
            if ( !_File.GetReadBytesCount() ) {
                return false;
            }

            /* INFO chunk */
            if ( !CheckID( &Id, RIFF_ListInfoID ) ) {
                Chunk.ByteLength -= sizeof( RIFF_FOURCC );

                while ( Chunk.ByteLength ) {
                    RiffChunk       c;

                    if ( !ReadChunk( _File, &c ) )
                        return false;

                    /* skip unknown chunk */
                    _File.SeekCur( PAD2( c.ByteLength ) );

                    Chunk.ByteLength -= sizeof( RiffChunk ) + PAD2( c.ByteLength );
                }
            } else {
                /* skip unknown chunk */
                _File.SeekCur( PAD2( Chunk.ByteLength ) - sizeof( RIFF_FOURCC ) );
            }
        }
        /* wave format chunk */
        else if ( !CheckID( &Chunk.Id, RIFF_WaveFmtID ) && !Wave->Format ) {
            Int Dummy;

            _File >> Wave->Format;
            _File >> Wave->Channels;
            _File >> Wave->SampleRate;
            _File >> Dummy;
            _File >> Wave->BlockAlign;

            if ( Wave->Format != WAVE_FORMAT_PCM && Wave->Format != WAVE_FORMAT_DVI_ADPCM ) {
                return false;
            }

            _File >> Wave->BitsPerSample;

            /* skip any other format specific fields */
            _File.SeekCur( PAD2( Chunk.ByteLength - 16 ) );

            bHasFormat = true;
        }
        /* wave data chunk */
        else if ( !CheckID( &Chunk.Id, RIFF_WaveDataID ) && !Wave->DataBase ) {
            long endOfFile;

            Wave->DataBase = _File.Tell();
            Wave->DataSize = Chunk.ByteLength;
            _File.SeekEnd( 0 );
            endOfFile = _File.Tell();

            if ( _File.SeekSet( Wave->DataBase + PAD2( Chunk.ByteLength ) ) || _File.Tell() > endOfFile ) {
                /* the seek failed, assume the size is bogus */
                _File.SeekEnd( 0 );
                Wave->DataSize = _File.Tell() - Wave->DataBase;
            }

            Wave->DataBase -= sizeof( long );

            bHasData = true;
        } else {
            /* skip unknown chunk */
            _File.SeekCur( PAD2( Chunk.ByteLength ) );
        }
    }

    if ( !Wave->DataBase ) {
        return false;
    }

    if ( Wave->Format == WAVE_FORMAT_DVI_ADPCM ) {

        if ( Wave->BitsPerSample != 4 ) {   // Other bits per samples are not supported
            return false;
        }

        Wave->SamplesPerBlock = ( Wave->BlockAlign - 4 * Wave->Channels ) * 2;
        Wave->BlockLength = Wave->BlockAlign;
        Wave->BlocksCount = Wave->DataSize / Wave->BlockLength;
        Wave->NumSamples = Wave->SamplesPerBlock * Wave->BlocksCount;
        Wave->DataSize = Wave->BlocksCount * Wave->BlockLength;    // align data size
    } else {
        Wave->NumSamples = Wave->DataSize / ( Wave->BitsPerSample >> 3 );

        // correct data size
        Wave->DataSize = Wave->NumSamples * ( Wave->BitsPerSample >> 3 );
    }

    WaveRewindFile( _File, Wave );

    return true;
}

template< typename T >
static int WaveReadFile( FStreamBase< T > & _File, void * _Buffer, int _BufferLength, FWaveFormat *_Wave ) {
    _File.Read( _Buffer, _BufferLength );
    return _File.GetReadBytesCount();
}

template< typename T >
static void WaveRewindFile( FStreamBase< T > & _File, FWaveFormat *_Wave ) {
    _File.SeekSet( _Wave->DataBase + sizeof( long ) );
}

template< typename T >
static int WaveSeekFile( FStreamBase< T > & _File, int _Offset, FWaveFormat *_Wave ) {
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
