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

#include "MiniaudioDecoder.h"

#include <Core/Public/Logger.h>
#include <Core/Public/BaseMath.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.h"

#define MINIAUDIO_IMPLEMENTATION
//#define MA_PREFER_SSE2
#include "miniaudio.h"

// The stb_vorbis implementation must come after the implementation of miniaudio.
#ifdef AN_COMPILER_MSVC
#pragma warning( push )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4457 )
#pragma warning( disable : 4701 )
#endif

#undef STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.h"

#ifdef AN_COMPILER_MSVC
#pragma warning( pop )
#endif

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

bool AMiniaudioTrack::InitializeFileStream( const char * FileName, int InSampleRate, int InSampleBits, int InChannels )
{
    PurgeStream();

    ma_format format;
    switch ( InSampleBits ) {
    case 8:
        format = ma_format_u8;
        break;
    case 16:
        format = ma_format_s16;
        break;
    case 32:
        format = ma_format_f32;
        break;
    default:
        GLogger.Printf( "AMiniaudioTrack::InitializeFileStream: expected 8, 16 or 32 sample bits\n" );
        return false;
    }

    if ( !File.OpenRead( FileName ) ) {
        GLogger.Printf( "Failed to open %s\n", FileName );
        return false;
    }

    ma_decoder_config config = ma_decoder_config_init( format, InChannels, InSampleRate );

    ma_result result = ma_decoder_init( Read, Seek, &File, &config, (ma_decoder *)Handle );
    if ( result != MA_SUCCESS ) {
        GLogger.Printf( "AMiniaudioTrack::InitializeFileStream: failed on %s\n", File.GetFileName() );
        return false;
    }

    SampleBits = InSampleBits;
    Channels = InChannels;

    bValid = true;

    return true;
}

bool AMiniaudioTrack::InitializeMemoryStream( const char * FileName, const byte * FileInMemory, size_t FileInMemorySize, int InSampleRate, int InSampleBits, int InChannels )
{
    PurgeStream();

    ma_format format;
    switch ( InSampleBits ) {
    case 8:
        format = ma_format_u8;
        break;
    case 16:
        format = ma_format_s16;
        break;
    case 32:
        format = ma_format_f32;
        break;
    default:
        GLogger.Printf( "AMiniaudioTrack::InitializeMemoryStream: expected 8, 16 or 32 sample bits\n" );
        return false;
    }

    if ( !Memory.OpenRead( FileName, FileInMemory, FileInMemorySize ) ) {
        GLogger.Printf( "AMiniaudioTrack::InitializeMemoryStream: failed on %s\n", Memory.GetFileName() );
        return false;
    }

    ma_decoder_config config = ma_decoder_config_init( format, InChannels, InSampleRate );

    ma_result result = ma_decoder_init( Read, Seek, &Memory, &config, (ma_decoder *)Handle );
    if ( result != MA_SUCCESS ) {
        GLogger.Printf( "AMiniaudioTrack::InitializeMemoryStream: failed on %s\n", Memory.GetFileName() );
        return false;
    }

    SampleBits = InSampleBits;
    Channels = InChannels;

    bValid = true;

    return true;
}

void AMiniaudioTrack::SeekToFrame( int FrameNum )
{
    if ( bValid ) {
        ma_decoder_seek_to_pcm_frame( (ma_decoder *)Handle, Math::Max( 0, FrameNum ) );
    }
}

int AMiniaudioTrack::ReadFrames( void * pFrames, int FrameCount, size_t SizeInBytes )
{
    if ( !bValid || FrameCount <= 0 ) {
        return 0;
    }

    int stride = ( SampleBits >> 3 ) << ( Channels - 1 );

    if ( (size_t)FrameCount * stride > SizeInBytes ) {
        FrameCount = SizeInBytes / stride;
    }

    return ma_decoder_read_pcm_frames( (ma_decoder *)Handle, pFrames, FrameCount );
}

AMiniaudioDecoder::AMiniaudioDecoder()
{
}

void AMiniaudioDecoder::CreateAudioStream( TRef< IAudioStream > * ppInterface )
{
    *ppInterface = MakeRef< AMiniaudioTrack >();
}

bool AMiniaudioDecoder::LoadFromFile( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, int SampleRate, bool bForceMono, bool bForce8Bit, void ** ppFrames )
{
    Core::ZeroMem( pAudioFileInfo, sizeof( *pAudioFileInfo ) );
    if ( ppFrames ) {
        *ppFrames = nullptr;
    }

    ma_decoder_config config = ma_decoder_config_init( bForce8Bit ? ma_format_u8 : ma_format_s16, bForceMono ? 1 : 0, SampleRate );

    ma_decoder decoder;
    ma_result result = ma_decoder_init( Read, Seek, &File, &config, &decoder );
    if ( result != MA_SUCCESS ) {
        GLogger.Printf( "AMiniaudioDecoder::LoadFromFile: failed on %s\n", File.GetFileName() );
        return false;
    }

    pAudioFileInfo->Channels = decoder.outputChannels;
    pAudioFileInfo->SampleBits = bForce8Bit ? 8 : 16;

    if ( ppFrames ) {
        ma_uint64 totalFramesRead = 0;
        ma_uint64 framesCapacity = 0;
        void* pFrames = NULL;
        byte temp[8192];
        int stride = ( pAudioFileInfo->SampleBits >> 3 ) * pAudioFileInfo->Channels;
        for ( ;; ) {
            ma_uint64 framesToReadRightNow = AN_ARRAY_SIZE( temp ) / stride;
            ma_uint64 framesJustRead = ma_decoder_read_pcm_frames( &decoder, temp, framesToReadRightNow );
            if ( framesJustRead == 0 ) {
                break;
            }

            ma_uint64 currentFrameCount = totalFramesRead + framesJustRead;

            // resize dest buffer
            if ( framesCapacity < currentFrameCount ) {
                ma_uint64 newFramesCap = framesCapacity * 2;
                if ( newFramesCap < currentFrameCount ) {
                    newFramesCap = currentFrameCount;
                }
                ma_uint64 newFramesBufferSize = newFramesCap * stride;
                if ( newFramesBufferSize > MA_SIZE_MAX ) {
                    break;
                }
                pFrames = GHeapMemory.Realloc( pFrames, (size_t)newFramesBufferSize, true );
                framesCapacity = newFramesCap;
            }

            // Copy frames
            Core::Memcpy( (ma_int8 *)pFrames + totalFramesRead*stride, temp, (size_t)(framesJustRead*stride) );
            totalFramesRead += framesJustRead;

            // Check EOF
            if ( framesJustRead != framesToReadRightNow ) {
                // EOF reached
                break;
            }
        }

        pAudioFileInfo->NumFrames = totalFramesRead;

        *ppFrames = pFrames;
    }
    else {
        pAudioFileInfo->NumFrames = ma_decoder_get_length_in_pcm_frames( &decoder ); // For MP3's, this will decode the entire file
    }

    ma_decoder_uninit( &decoder );

    return pAudioFileInfo->NumFrames > 0;
}
