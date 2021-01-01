/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <World/Public/Audio/AudioClip.h>
#include <World/Public/Audio/AudioSystem.h>

#include <Core/Public/Alloc.h>
#include <Core/Public/Logger.h>

#include "AudioSystemLocal.h"

#define DEFAULT_BUFFER_SIZE ( 1024 * 32 )

static int ResourceSerialIdGen = 0;

AN_CLASS_META( AAudioClip )

AAudioClip::AAudioClip()
{
    BufferSize = DEFAULT_BUFFER_SIZE;
    SerialId = ++ResourceSerialIdGen;

    Core::ZeroMem( &AudioFileInfo, sizeof( AudioFileInfo ) );
}

AAudioClip::~AAudioClip()
{
    Purge();
}

int AAudioClip::GetFrequency() const
{
    return AudioFileInfo.SampleRate;
}

int AAudioClip::GetBitsPerSample() const
{
    return AudioFileInfo.BitsPerSample;
}

int AAudioClip::GetChannels() const
{
    return AudioFileInfo.Channels;
}

int AAudioClip::GetSamplesCount() const
{
    return AudioFileInfo.SamplesCount;
}

float AAudioClip::GetDurationInSecounds() const
{
    return DurationInSeconds;
}

ESoundStreamType AAudioClip::GetStreamType() const
{
    return CurStreamType;
}

void AAudioClip::SetBufferSize( int _BufferSize )
{
    BufferSize = Math::Clamp< int >( _BufferSize, AUDIO_MIN_PCM_BUFFER_SIZE, AUDIO_MAX_PCM_BUFFER_SIZE );
}

int AAudioClip::GetBufferSize() const
{
    return BufferSize;
}

void AAudioClip::LoadInternalResource( const char * _Path )
{
    Purge();

    // TODO: ...
}

bool AAudioClip::LoadResource( AString const & _Path )
{
    Purge();

    AN_ASSERT( BufferHandle == 0 );

    FileName = _Path;

    Decoder = GAudioSystem.FindAudioDecoder( _Path.CStr() );
    if ( !Decoder ) {
        return false;
    }

    CurStreamType = StreamType;

    switch ( CurStreamType ) {
    case SOUND_STREAM_DISABLED:
    {
        AFileStream f;
        short * PCM;

        if ( !f.OpenRead( _Path ) ) {
            return false;
        }

        if ( !Decoder->LoadFromFile( f, &AudioFileInfo, &PCM ) ) {
            return false;
        }

        AN_ASSERT( AudioFileInfo.SamplesCount > 0 );

        SAudioBufferUpload upload = {};
        upload.SamplesCount = AudioFileInfo.SamplesCount;
        upload.BitsPerSample = AudioFileInfo.BitsPerSample;
        upload.Frequency = AudioFileInfo.SampleRate;
        upload.PCM = PCM;
        upload.bStereo = AudioFileInfo.Channels == 2;
        BufferHandle = CreateAudioBuffer( &upload );

        GHeapMemory.Free( PCM );
        break;
    }
    case SOUND_STREAM_FILE:
    {
        AFileStream f;
        if ( !f.OpenRead( _Path ) ) {
            return false;
        }
        if ( !Decoder->GetAudioFileInfo( f, &AudioFileInfo ) ) {
            return false;
        }
        break;
    }
    case SOUND_STREAM_MEMORY:
    {
        AFileStream f;

        if ( !f.OpenRead( _Path ) ) {
            return false;
        }

        if ( !Decoder->GetAudioFileInfo( f, &AudioFileInfo ) ) {
            return false;
        }

        f.SeekEnd( 0 );

        FileInMemorySize = f.Tell();
        FileInMemory = (byte *)GHeapMemory.Alloc( FileInMemorySize );

        f.Rewind();
        f.ReadBuffer( FileInMemory, FileInMemorySize );

        break;
    }
    default:
        AN_ASSERT(0);
        return false;
    }

    bLoaded = true;
    DurationInSeconds = (float)AudioFileInfo.SamplesCount / AudioFileInfo.SampleRate;

    return true;
}

bool AAudioClip::InitializeFromData( const char * _Path, IAudioDecoderInterface * _Decoder, const byte * _Data, size_t _DataLength )
{
    Purge();

    AN_ASSERT( BufferHandle == 0 );

    FileName = _Path;

    Decoder = _Decoder;
    if ( !Decoder ) {
        return false;
    }

    CurStreamType = StreamType;
    if ( CurStreamType == SOUND_STREAM_FILE ) {
        CurStreamType = SOUND_STREAM_MEMORY;

        GLogger.Printf( "Using MemoryStreamed instead of FileStreamed becouse file data is already in memory\n" );
    }

    switch ( CurStreamType ) {
    case SOUND_STREAM_DISABLED:
    {
        AMemoryStream f;
        short * PCM;

        if ( !f.OpenRead( _Path, _Data, _DataLength ) ) {
            return false;
        }

        if ( !Decoder->LoadFromFile( f, &AudioFileInfo, &PCM ) ) {
            return false;
        }

        AN_ASSERT( AudioFileInfo.SamplesCount > 0 );

        SAudioBufferUpload upload = {};
        upload.SamplesCount = AudioFileInfo.SamplesCount;
        upload.BitsPerSample = AudioFileInfo.BitsPerSample;
        upload.Frequency = AudioFileInfo.SampleRate;
        upload.PCM = PCM;
        upload.bStereo = AudioFileInfo.Channels == 2;
        BufferHandle = CreateAudioBuffer( &upload );

        GHeapMemory.Free( PCM );
        break;
    }
    case SOUND_STREAM_MEMORY:
    {
        AMemoryStream f;

        if ( !f.OpenRead( _Path, _Data, _DataLength ) ) {
            return false;
        }

        if ( !Decoder->GetAudioFileInfo( f, &AudioFileInfo ) ) {
            return false;
        }

        f.SeekEnd( 0 );

        FileInMemorySize = f.Tell();
        FileInMemory = (byte *)GHeapMemory.Alloc( FileInMemorySize );

        f.Rewind();
        f.ReadBuffer( FileInMemory, FileInMemorySize );

        break;
    }
    default:
        AN_ASSERT(0);
        return false;
    }

    bLoaded = true;
    DurationInSeconds = (float)AudioFileInfo.SamplesCount / AudioFileInfo.SampleRate;

    return true;
}

IAudioStreamInterface * AAudioClip::CreateAudioStreamInstance()
{
    IAudioStreamInterface * streamInterface;

    if ( CurStreamType == SOUND_STREAM_DISABLED || !Decoder ) {
        return nullptr;
    }

    streamInterface = Decoder->CreateAudioStream();

    bool bCreateResult = false;

    if ( streamInterface ) {
        if ( CurStreamType == SOUND_STREAM_FILE ) {
            bCreateResult = streamInterface->InitializeFileStream( GetFileName().CStr() );
        } else {
            bCreateResult = streamInterface->InitializeMemoryStream( GetFileInMemory(), GetFileInMemorySize() );
        }
    }

    if ( !bCreateResult ) {
        return nullptr;
    }

    return streamInterface;
}

void AAudioClip::Purge()
{
    if ( BufferHandle ) {
        DeleteAudioBuffer( BufferHandle );
        BufferHandle = 0;
    }

    GHeapMemory.Free( FileInMemory );
    FileInMemory = nullptr;
    FileInMemorySize = 0;

    bLoaded = false;

    DurationInSeconds = 0;

    Decoder = nullptr;

    // Mark resource was changed
    SerialId = ++ResourceSerialIdGen;
}
