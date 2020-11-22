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

#include <World/Public/Audio/AudioCodec/MiniaudioDecoder.h>

//#include <Core/Public/Alloc.h>
//#include <Core/Public/Logger.h>
//#include <Core/Public/IO.h>
//#include <Runtime/Public/Runtime.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// The stb_vorbis implementation must come after the implementation of miniaudio.
#ifdef AN_COMPILER_MSVC
#pragma warning( push )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4457 )
#pragma warning( disable : 4701 )
#endif

//#ifdef AN_COMPILER_GCC
//#pragma GCC diagnostic ignored "-Wunused-value"
//#endif

#undef STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.h"

#ifdef AN_COMPILER_MSVC
#pragma warning( pop )
#endif


AN_CLASS_META( AMiniaudioTrack )
AN_CLASS_META( AMiniaudioDecoder )

AMiniaudioTrack::AMiniaudioTrack()
{
    Handle = (ma_decoder *)GZoneMemory.Alloc( sizeof( ma_decoder ) );
    bValid = false;
}

AMiniaudioTrack::~AMiniaudioTrack()
{
    PurgeStream();

    GZoneMemory.Free( Handle );
}

static size_t Read( ma_decoder * pDecoder, void* pBufferOut, size_t bytesToRead )
{
    IBinaryStream * file = (IBinaryStream *)pDecoder->pUserData;

    file->ReadBuffer( pBufferOut, bytesToRead );
    return file->GetReadBytesCount();
}

static ma_bool32 Seek( ma_decoder* pDecoder, int byteOffset, ma_seek_origin origin )
{
    IBinaryStream * file = (IBinaryStream *)pDecoder->pUserData;
    bool result = false;

    switch ( origin )
    {
    case ma_seek_origin_start:
        result = file->SeekSet( byteOffset );
        break;
    case ma_seek_origin_current:
        result = file->SeekCur( byteOffset );
        break;
    case ma_seek_origin_end:  // Not used by decoders.
        result = file->SeekEnd( byteOffset );
        break;
    }

    return result;
}

void AMiniaudioTrack::PurgeStream()
{
    if ( bValid ) {
        ma_decoder_uninit( (ma_decoder *)Handle );
        bValid = false;
    }

    File.Close();
    Memory.Close();
}

bool AMiniaudioTrack::InitializeFileStream( const char * _FileName )
{
    PurgeStream();

    if ( !File.OpenRead( _FileName ) ) {
        GLogger.Printf( "Failed to open %s\n", _FileName );
        return false;
    }

    ma_decoder_config config = ma_decoder_config_init( ma_format_s16, 0, 0 );

    ma_result result = ma_decoder_init( Read, Seek, &File, &config, (ma_decoder *)Handle );
    if ( result != MA_SUCCESS ) {
        GLogger.Printf( "AMiniaudioTrack::InitializeFileStream: failed on %s\n", File.GetFileName() );
        return false;
    }

    bValid = true;

    return true;
}

bool AMiniaudioTrack::InitializeMemoryStream( const byte * FileInMemory, size_t FileInMemorySize )
{
    PurgeStream();

    if ( !Memory.OpenRead( "AudioData", FileInMemory, FileInMemorySize ) ) {
        GLogger.Printf( "AMiniaudioTrack::InitializeMemoryStream: failed on %s\n", Memory.GetFileName() );
        return false;
    }

    ma_decoder_config config = ma_decoder_config_init( ma_format_s16, 0, 0 );

    ma_result result = ma_decoder_init( Read, Seek, &Memory, &config, (ma_decoder *)Handle );
    if ( result != MA_SUCCESS ) {
        GLogger.Printf( "AMiniaudioTrack::InitializeMemoryStream: failed on %s\n", Memory.GetFileName() );
        return false;
    }

    bValid = true;

    return true;
}

void AMiniaudioTrack::StreamSeek( int _PositionInSamples )
{
    if ( bValid ) {
        ma_decoder_seek_to_pcm_frame( (ma_decoder *)Handle, Math::Max( 0, _PositionInSamples ) );
    }
}

int AMiniaudioTrack::StreamDecodePCM( short * _Buffer, int _NumShorts )
{
    return bValid ? ma_decoder_read_pcm_frames( (ma_decoder *)Handle, _Buffer, _NumShorts>>1 )<<1 : 0;
}

AMiniaudioDecoder::AMiniaudioDecoder()
{
}

IAudioStreamInterface * AMiniaudioDecoder::CreateAudioStream()
{
    return CreateInstanceOf< AMiniaudioTrack >();
}

bool AMiniaudioDecoder::GetAudioFileInfo( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo )
{
    Core::ZeroMem( pAudioFileInfo, sizeof( *pAudioFileInfo ) );

    ma_decoder_config config = ma_decoder_config_init( ma_format_s16, 0, 0 );

    ma_decoder decoder;
    ma_result result = ma_decoder_init( Read, Seek, &File, &config, &decoder );
    if ( result != MA_SUCCESS ) {
        GLogger.Printf( "AMiniaudioDecoder::GetAudioFileInfo: failed on %s\n", File.GetFileName() );
        return false;
    }

    pAudioFileInfo->SamplesCount = ma_decoder_get_length_in_pcm_frames( &decoder ); // For MP3's, this will decode the entire file
    pAudioFileInfo->Channels = decoder.outputChannels;
    pAudioFileInfo->SampleRate = decoder.outputSampleRate;
    pAudioFileInfo->BitsPerSample = 16;

    ma_decoder_uninit( &decoder );

    return true;
}

bool AMiniaudioDecoder::LoadFromFile( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, short ** _PCM )
{
    Core::ZeroMem( pAudioFileInfo, sizeof( *pAudioFileInfo ) );
    *_PCM = nullptr;

    ma_decoder_config config = ma_decoder_config_init( ma_format_s16, 0, 0 );

    ma_decoder decoder;
    ma_result result = ma_decoder_init( Read, Seek, &File, &config, &decoder );
    if ( result != MA_SUCCESS ) {
        GLogger.Printf( "AMiniaudioDecoder::GetAudioFileInfo: failed on %s\n", File.GetFileName() );
        return false;
    }

    pAudioFileInfo->Channels = decoder.outputChannels;
    pAudioFileInfo->SampleRate = decoder.outputSampleRate;
    pAudioFileInfo->BitsPerSample = 16;

    ma_uint64 totalFramesRead = 0;
    ma_uint64 framesCapacity = 0;
    ma_int16* pFrames = NULL;
    ma_int16 temp[4096];
    for ( ;;) {
        ma_uint64 framesToReadRightNow = AN_ARRAY_SIZE( temp ) / pAudioFileInfo->Channels;
        ma_uint64 framesJustRead = ma_decoder_read_pcm_frames( &decoder, temp, framesToReadRightNow );
        if ( framesJustRead == 0 ) {
            break;
        }
        if ( framesCapacity < totalFramesRead + framesJustRead ) {
            ma_uint64 newFramesBufferSize;
            ma_uint64 oldFramesBufferSize;
            ma_uint64 newFramesCap;
            ma_int16* pNewFrames;
            newFramesCap = framesCapacity * 2;
            if ( newFramesCap < totalFramesRead + framesJustRead ) {
                newFramesCap = totalFramesRead + framesJustRead;
            }
            oldFramesBufferSize = framesCapacity * pAudioFileInfo->Channels * sizeof( ma_int16 );
            newFramesBufferSize = newFramesCap   * pAudioFileInfo->Channels * sizeof( ma_int16 );
            if ( newFramesBufferSize > MA_SIZE_MAX ) {
                break;
            }
            pNewFrames = (ma_int16 *)GHeapMemory.Realloc( pFrames, (size_t)newFramesBufferSize, true );
            if ( pNewFrames == NULL ) {
                ma_free( pFrames, &decoder.allocationCallbacks );
                break;
            }
            pFrames = pNewFrames;
            framesCapacity = newFramesCap;
        }
        Core::Memcpy( pFrames + totalFramesRead*pAudioFileInfo->Channels, temp, (size_t)(framesJustRead*pAudioFileInfo->Channels*sizeof( ma_int16 )) );
        totalFramesRead += framesJustRead;
        if ( framesJustRead != framesToReadRightNow ) {
            break;
        }
    }

    ma_decoder_uninit( &decoder );

    pAudioFileInfo->SamplesCount = totalFramesRead;    

    *_PCM = pFrames;

    return true;
}
