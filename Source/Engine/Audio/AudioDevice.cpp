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

#include "AudioDevice.h"

#include <Runtime/Public/Runtime.h>
#include <Core/Public/CriticalError.h>

#include <SDL.h>

AAudioDevice::AAudioDevice( int InSampleRate )
{
    pTransferBuffer = nullptr;

    const char * driver = NULL;

    int n = GRuntime.CheckArg( "-AudioDrv" );
    if ( n != -1 ) {
        driver = GRuntime.GetArgv()[n];
        SDL_setenv( "SDL_AUDIODRIVER", driver, SDL_TRUE );
    }

    if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 ) {
        CriticalError( "Failed to init audio system: %s\n", SDL_GetError() );
    }

    int numdrivers = SDL_GetNumAudioDrivers();
    if ( numdrivers > 0 ) {
        GLogger.Printf( "Available audio drivers:\n" );
        for ( int i = 0 ; i < numdrivers ; i++ ) {
            GLogger.Printf( "\t%s\n", SDL_GetAudioDriver( i ) );
        }
    }

    int numdevs = SDL_GetNumAudioDevices( SDL_FALSE );
    if ( numdevs > 0  ) {
        GLogger.Printf( "Available audio devices:\n" );
        for ( int i = 0 ; i < numdrivers ; i++ ) {
            GLogger.Printf( "\t%s\n", SDL_GetAudioDeviceName( i, SDL_FALSE ) );
        }
    }

    SDL_AudioSpec desired = {};
    SDL_AudioSpec obtained = {};

    desired.freq = InSampleRate;
    desired.channels = 2;

    // Choose audio buffer size in sample FRAMES (total samples divided by channel count)
    if ( desired.freq <= 11025 )
        desired.samples = 256;
    else if ( desired.freq <= 22050 )
        desired.samples = 512;
    else if ( desired.freq <= 44100 )
        desired.samples = 1024;
    else if ( desired.freq <= 56000 )
        desired.samples = 2048;
    else
        desired.samples = 4096;

    desired.callback = []( void *userdata, uint8_t * stream, int len )
    {
        ((AAudioDevice *)userdata)->RenderAudio( stream, len );
    };
    desired.userdata = this;

    int allowedChanges = 0;

    // Try to minimize format convertions
    allowedChanges |= SDL_AUDIO_ALLOW_FREQUENCY_CHANGE;
    allowedChanges |= SDL_AUDIO_ALLOW_FORMAT_CHANGE;
    allowedChanges |= SDL_AUDIO_ALLOW_CHANNELS_CHANGE;

    //SDL_AudioFormat formats[] = { AUDIO_F32SYS, AUDIO_S16SYS, AUDIO_U8, AUDIO_S8 };
    SDL_AudioFormat formats[] = { AUDIO_S16SYS };

    // Try to search appropriate native format
    for ( int i = 0 ; i < AN_ARRAY_SIZE( formats ) ; ) {
        desired.format = formats[i];

        AudioDeviceId = SDL_OpenAudioDevice( NULL, SDL_FALSE, &desired, &obtained, allowedChanges );
        if ( AudioDeviceId == 0 ) {
            if ( !driver ) {
                // Reinit audio subsystem with dummy driver
                SDL_QuitSubSystem( SDL_INIT_AUDIO );
                driver = "dummy";
                SDL_setenv( "SDL_AUDIODRIVER", driver, SDL_TRUE );
                if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 ) {
                    CriticalError( "Failed to init audio system: %s\n", SDL_GetError() );
                }
                continue;
            }
            // Should not happen
            CriticalError( "Failed to open audio device: %s\n", SDL_GetError() );
        }

        if ( obtained.format == desired.format ) {
            // Ok
            break;
        }

        // Try another format
        SDL_CloseAudioDevice( AudioDeviceId );
        AudioDeviceId = 0;
        i++;
    }

    // If appropriate format was not found force default AUDIO_S16SYS
    if ( !AudioDeviceId ) {
        allowedChanges &= ~SDL_AUDIO_ALLOW_FORMAT_CHANGE;
        desired.format = AUDIO_S16SYS;
        AudioDeviceId = SDL_OpenAudioDevice( NULL, SDL_FALSE, &desired, &obtained, allowedChanges );
        if ( AudioDeviceId == 0 ) {
            // Should not happen
            CriticalError( "Failed to open audio device: %s\n", SDL_GetError() );
        }
    }

    SampleBits = obtained.format & 0xFF; // extract first byte which is sample bits
    bSigned8 = (obtained.format == AUDIO_S8);
    SampleRate = obtained.freq;
    Channels = obtained.channels;
    Samples = Math::ToGreaterPowerOfTwo( obtained.samples * obtained.channels * 10 );
    NumFrames = Samples >> ( Channels - 1 );
    TransferBufferSizeInBytes = Samples * (SampleBits / 8);
    pTransferBuffer = (uint8_t *)GHeapMemory.Alloc( TransferBufferSizeInBytes );
    Core::MemsetSSE( pTransferBuffer, SampleBits == 8 && !bSigned8 ? 0x80 : 0, TransferBufferSizeInBytes );
    TransferOffset = 0;
    PrevTransferOffset = 0;
    BufferWraps = 0;

    SDL_PauseAudioDevice( AudioDeviceId, 0 );

    GLogger.Printf( "Initialized audio : %d Hz, %d samples, %d channels\n", SampleRate, obtained.samples, Channels );

    const char * audioDriver = SDL_GetCurrentAudioDriver();
    const char * audioDevice = SDL_GetAudioDeviceName( 0, SDL_FALSE );

    GLogger.Printf( "Using audio driver: %s\n", audioDriver ? audioDriver : "Unknown" );
    GLogger.Printf( "Using playback device: %s\n", audioDevice ? audioDevice : "Unknown" );
    GLogger.Printf( "Audio buffer size: %d bytes\n", TransferBufferSizeInBytes );
}

AAudioDevice::~AAudioDevice()
{
    SDL_CloseAudioDevice( AudioDeviceId );

    GHeapMemory.Free( pTransferBuffer );
}

void AAudioDevice::RenderAudio( uint8_t * pStream, int StreamLength )
{
    if ( !pTransferBuffer ) {
        // Should never happen
        Core::ZeroMem( pStream, StreamLength );
        return;
    }

    int sampleWidth = SampleBits / 8;

    int offset = TransferOffset * sampleWidth;
    if ( offset >= TransferBufferSizeInBytes ) {
        TransferOffset = offset = 0;
    }

    int len1 = StreamLength;
    int len2 = 0;

    if ( offset + len1 > TransferBufferSizeInBytes ) {
        len1 = TransferBufferSizeInBytes - offset;
        len2 = StreamLength - len1;
    }

    Core::Memcpy( pStream, pTransferBuffer + offset, len1 );
    //Core::ZeroMem( pTransferBuffer + offset, len1 );

    if ( len2 > 0 ) {
        Core::Memcpy( pStream + len1, pTransferBuffer, len2 );
        TransferOffset = len2 / sampleWidth;
    }
    else {
        TransferOffset += len1 / sampleWidth;
    }
}

uint8_t * AAudioDevice::MapTransferBuffer( int64_t * pFrameNum )
{
    SDL_LockAudioDevice( AudioDeviceId );

    if ( pFrameNum ) {
        if ( TransferOffset < PrevTransferOffset ) {
            BufferWraps++;
        }
        PrevTransferOffset = TransferOffset;

        *pFrameNum = BufferWraps * NumFrames + ( TransferOffset >> (Channels - 1) );
    }

    return pTransferBuffer;
}

void AAudioDevice::UnmapTransferBuffer()
{
    SDL_UnlockAudioDevice( AudioDeviceId );
}

void AAudioDevice::BlockSound()
{
    SDL_PauseAudioDevice( AudioDeviceId, 1 );
}

void AAudioDevice::UnblockSound()
{
    SDL_PauseAudioDevice( AudioDeviceId, 0 );
}

void AAudioDevice::ClearBuffer()
{
    MapTransferBuffer();
    Core::MemsetSSE( pTransferBuffer, SampleBits == 8 && !bSigned8 ? 0x80 : 0, TransferBufferSizeInBytes );
    UnmapTransferBuffer();
}
