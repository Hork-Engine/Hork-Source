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

#include <Audio/Public/AudioDecoder.h>

#include <Core/Public/Logger.h>
#include <Core/Public/BaseMath.h>

#include <miniaudio/miniaudio.h>

static size_t Read( ma_decoder * pDecoder, void* pBufferOut, size_t bytesToRead )
{
    IBinaryStream * file = (IBinaryStream *)pDecoder->pUserData;

    file->ReadBuffer( pBufferOut, bytesToRead );
    return file->GetReadBytesCount();
}

static ma_bool32 Seek( ma_decoder * pDecoder, ma_int64 byteOffset, ma_seek_origin origin )
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

bool LoadAudioFile( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, int SampleRate, bool bForceMono, bool bForce8Bit, void ** ppFrames )
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

    byte temp[8192];

    if ( ppFrames ) {
        ma_uint64 totalFramesRead = 0;
        ma_uint64 framesCapacity = 0;
        void* pFrames = NULL;
        const int stride = ( pAudioFileInfo->SampleBits >> 3 ) * pAudioFileInfo->Channels;
        const ma_uint64 framesToReadRightNow = AN_ARRAY_SIZE( temp ) / stride;
        for ( ;; ) {
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
                pFrames = GHeapMemory.Realloc( pFrames, (size_t)newFramesBufferSize, 16, true );
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

        pAudioFileInfo->FrameCount = totalFramesRead;

        *ppFrames = pFrames;
    }
    else {
        pAudioFileInfo->FrameCount = ma_decoder_get_length_in_pcm_frames( &decoder ); // For MP3's, this will decode the entire file

        // ma_decoder_get_length_in_pcm_frames will always return 0 for Vorbis decoders.
        // This is due to a limitation with stb_vorbis in push mode which is what miniaudio
        // uses internally.
        if ( pAudioFileInfo->FrameCount == 0 ) { // stb_vorbis :(
            ma_uint64 totalFramesRead = 0;
            const int stride = (pAudioFileInfo->SampleBits >> 3) * pAudioFileInfo->Channels;
            const ma_uint64 framesToReadRightNow = AN_ARRAY_SIZE( temp ) / stride;
            for ( ;; ) {
                ma_uint64 framesJustRead = ma_decoder_read_pcm_frames( &decoder, temp, framesToReadRightNow );
                if ( framesJustRead == 0 ) {
                    break;
                }
                totalFramesRead += framesJustRead;
                // Check EOF
                if ( framesJustRead != framesToReadRightNow ) {
                    break;
                }
            }

            pAudioFileInfo->FrameCount = totalFramesRead;
        }
    }

    ma_decoder_uninit( &decoder );

    return pAudioFileInfo->FrameCount > 0;
}

bool CreateAudioBuffer( IBinaryStream & File, SAudioFileInfo * pAudioFileInfo, int SampleRate, bool bForceMono, bool bForce8Bit, TRef< SAudioBuffer > * ppBuffer )
{
    void * pFrames;

    if ( !LoadAudioFile( File, pAudioFileInfo, SampleRate, bForceMono, bForce8Bit, &pFrames ) ) {
        return false;
    }

    *ppBuffer = MakeRef< SAudioBuffer >( pAudioFileInfo->FrameCount, pAudioFileInfo->Channels, pAudioFileInfo->SampleBits, pFrames );

    return true;
}
