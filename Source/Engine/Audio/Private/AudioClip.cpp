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

#include <Engine/Audio/Public/AudioClip.h>
#include <Engine/Audio/Public/AudioSystem.h>

#include <Engine/Core/Public/Alloc.h>
#include <Engine/Core/Public/Logger.h>

#include "AudioSystemLocal.h"

#define DEFAULT_BUFFER_SIZE ( 1024 * 32 )

static int ResourceSerialIdGen = 0;

AN_CLASS_META( FAudioClip )

FAudioClip::FAudioClip() {
    BufferSize = DEFAULT_BUFFER_SIZE;
    SerialId = ++ResourceSerialIdGen;
    CurStreamType = StreamType = SST_NonStreamed;
}

FAudioClip::~FAudioClip() {
    Purge();
}

int FAudioClip::GetFrequency() const {
    return Frequency;
}

int FAudioClip::GetBitsPerSample() const {
    return BitsPerSample;
}

int FAudioClip::GetChannels() const {
    return Channels;
}

int FAudioClip::GetSamplesCount() const {
    return SamplesCount;
}

float FAudioClip::GetDurationInSecounds() const {
    return DurationInSeconds;
}

ESoundStreamType FAudioClip::GetStreamType() const {
    return CurStreamType;
}

void FAudioClip::SetBufferSize( int _BufferSize ) {
    BufferSize = _BufferSize;
    BufferSize.Clamp( AUDIO_MIN_PCM_BUFFER_SIZE, AUDIO_MAX_PCM_BUFFER_SIZE );
}

int FAudioClip::GetBufferSize() const {
    return BufferSize;
}

static int GetAppropriateFormat( short _Channels, short _BitsPerSample ) {
    bool bStereo = ( _Channels > 1 );
    switch ( _BitsPerSample ) {
    case 16:
        return ( bStereo ) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    case 8:
        return ( bStereo ) ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
    }
    return -1;
}

void FAudioClip::InitializeInternalResource( const char * _InternalResourceName ) {
    Purge();

    // TODO: ...

    //SetName( _InternalResourceName );
}

bool FAudioClip::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
    Purge();

    FileName = _Path;

    // Mark resource was changed
    SerialId = ++ResourceSerialIdGen;

    Decoder = GAudioSystem.FindDecoder( _Path );
    if ( Decoder ) {

        CurStreamType = StreamType;

        switch ( CurStreamType ) {
        case SST_NonStreamed:
            {
                short * PCM;
                Decoder->DecodePCM( _Path, &SamplesCount, &Channels, &Frequency, &BitsPerSample, &PCM );

                if ( SamplesCount > 0 ) {
                    Format = GetAppropriateFormat( Channels, BitsPerSample );
                    if ( !BufferId ) {
                        // create buffer if not exists
                        BufferId = AL_CreateBuffer();
                    }
                    if ( BitsPerSample == 16 ) {
                        AL_UploadBuffer( BufferId, Format, PCM, SamplesCount * Channels * sizeof( short ), Frequency );
                    } else {
                        AL_UploadBuffer( BufferId, Format, PCM, SamplesCount * Channels * sizeof( byte ), Frequency );
                    }
                    GMainMemoryZone.Dealloc( PCM );

                    bLoaded = true;
                } else {
                    // delete buffer if exists
                    if ( BufferId ) {
                        AL_DeleteBuffer( BufferId );
                        BufferId = 0;
                    }
                }
            }
            break;
        case SST_FileStreamed:
            {
                if ( BufferId ) {
                    AL_DeleteBuffer( BufferId );
                    BufferId = 0;
                }

                bool bDecodeResult = Decoder->DecodePCM( _Path, &SamplesCount, &Channels, &Frequency, &BitsPerSample, NULL );
                if ( bDecodeResult ) {
                    Format = GetAppropriateFormat( Channels, BitsPerSample );
                    bLoaded = true;
                }
            }
            break;
        case SST_MemoryStreamed:
            {
                if ( BufferId ) {
                    AL_DeleteBuffer( BufferId );
                    BufferId = 0;
                }

                bool bResult = Decoder->ReadEncoded( _Path, &SamplesCount, &Channels, &Frequency, &BitsPerSample, &EncodedData, &EncodedDataLength );
                if ( bResult ) {
                    Format = GetAppropriateFormat( Channels, BitsPerSample );
                    bLoaded = true;
                }
            }
            break;
        }
    }

    if ( !bLoaded ) {

        if ( _CreateDefultObjectIfFails ) {
            InitializeDefaultObject();
            return true;
        }

        return false;
    }

    DurationInSeconds = (float)SamplesCount / Frequency;

    return true;
}


bool FAudioClip::InitializeFromData( const char * _Path, IAudioDecoderInterface * _Decoder, const byte * _Data, size_t _DataLength ) {
    Purge();

    FileName = _Path;

    // Mark resource was changed
    SerialId = ++ResourceSerialIdGen;

    Decoder = _Decoder;
    if ( Decoder ) {

        CurStreamType = StreamType;
        if ( CurStreamType == SST_FileStreamed ) {
            CurStreamType = SST_MemoryStreamed;

            GLogger.Printf( "Using MemoryStreamed instead of FileStreamed because file data is already in memory\n" );
        }

        switch ( CurStreamType ) {
        case SST_NonStreamed:
            {
                short * PCM;
                Decoder->DecodePCM( _Path, _Data, _DataLength, &SamplesCount, &Channels, &Frequency, &BitsPerSample, &PCM );

                if ( SamplesCount > 0 ) {
                    Format = GetAppropriateFormat( Channels, BitsPerSample );
                    if ( !BufferId ) {
                        // create buffer if not exists
                        BufferId = AL_CreateBuffer();
                    }
                    if ( BitsPerSample == 16 ) {
                        AL_UploadBuffer( BufferId, Format, PCM, SamplesCount * Channels * sizeof( short ), Frequency );
                    } else {
                        AL_UploadBuffer( BufferId, Format, PCM, SamplesCount * Channels * sizeof( byte ), Frequency );
                    }
                    GMainMemoryZone.Dealloc( PCM );

                    bLoaded = true;
                } else {
                    // delete buffer if exists
                    if ( BufferId ) {
                        AL_DeleteBuffer( BufferId );
                        BufferId = 0;
                    }
                }
            }
            break;
        case SST_MemoryStreamed:
            {
                if ( BufferId ) {
                    AL_DeleteBuffer( BufferId );
                    BufferId = 0;
                }

                bool bResult = Decoder->ReadEncoded( _Path, _Data, _DataLength, &SamplesCount, &Channels, &Frequency, &BitsPerSample, &EncodedData, &EncodedDataLength );
                if ( bResult ) {
                    Format = GetAppropriateFormat( Channels, BitsPerSample );
                    bLoaded = true;
                }
            }
            break;
        default:
            AN_Assert(0);
            break;
        }
    }

    if ( !bLoaded ) {

//        if ( _CreateDefultObjectIfFails ) {
//            InitializeDefaultObject();
//            return true;
//        }

        return false;
    }

    DurationInSeconds = (float)SamplesCount / Frequency;

    return true;
}

IAudioStreamInterface * FAudioClip::CreateAudioStreamInstance() {
    IAudioStreamInterface * streamInterface;

    if ( CurStreamType == SST_NonStreamed ) {
        return nullptr;
    }

    streamInterface = Decoder->CreateAudioStream();

    bool bCreateResult = false;

    if ( streamInterface ) {
        if ( CurStreamType == SST_FileStreamed ) {
            bCreateResult = streamInterface->InitializeFileStream( GetFileName().ToConstChar() );
        } else {
            bCreateResult = streamInterface->InitializeMemoryStream( GetEncodedData(), GetEncodedDataLength() );
        }
    }

    if ( !bCreateResult ) {
        return nullptr;
    }

    return streamInterface;
}

void FAudioClip::Purge() {
    if ( BufferId ) {
        AL_DeleteBuffer( BufferId );
        BufferId = 0;
    }

    GMainMemoryZone.Dealloc( EncodedData );
    EncodedData = NULL;

    EncodedDataLength = 0;

    bLoaded = false;

    DurationInSeconds = 0;

    Decoder = nullptr;

    // Пометить, что ресурс изменен
    SerialId = ++ResourceSerialIdGen;
}
