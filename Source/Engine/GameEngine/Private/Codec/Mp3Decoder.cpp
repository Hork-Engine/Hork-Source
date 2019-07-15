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

#include <Engine/GameEngine/Public/Codec/Mp3Decoder.h>

#include "ThirdParty/libmpg123/mpg123.h"

#include <Engine/Core/Public/Alloc.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IO.h>
#include <Engine/Runtime/Public/Runtime.h>

static void * LibMpg = NULL;

static int ( *mpg_init )( );
static void ( *mpg_exit )( );
static mpg123_handle *( *mpg_new )( const char* decoder, int *error );
static void ( *mpg_delete )( mpg123_handle *mh );
static const char* ( *mpg_plain_strerror )( int errcode );
static const char* ( *mpg_strerror )( mpg123_handle *mh );
static int ( *mpg_errcode )( mpg123_handle *mh );
static int ( *mpg_open )( mpg123_handle *mh, const char *path );
static int ( *mpg_open_fd )( mpg123_handle *mh, int fd );
static int ( *mpg_open_handle )( mpg123_handle *mh, void *iohandle );
static int ( *mpg_close )( mpg123_handle *mh );
static int ( *mpg_read )( mpg123_handle *mh, unsigned char *outmemory, size_t outmemsize, size_t *done );
static off_t ( *mpg_tell )( mpg123_handle *mh );
static off_t ( *mpg_seek )( mpg123_handle *mh, off_t sampleoff, int whence );
static int ( *mpg_getformat )( mpg123_handle *mh, long *rate, int *channels, int *encoding );
static int ( *mpg_format_none )( mpg123_handle *mh );
//static int (*mpg_format_all)(mpg123_handle *mh);
static int ( *mpg_format )( mpg123_handle *mh, long rate, int channels, int encodings );
static size_t ( *mpg_outblock )( mpg123_handle *mh );
static off_t ( *mpg_length )( mpg123_handle *mh );
static int ( *mpg_replace_reader_handle )( mpg123_handle *mh, ssize_t ( *r_read ) ( void *, void *, size_t ), off_t ( *r_lseek )( void *, off_t, int ), void ( *cleanup )( void* ) );

static bool LoadLibMpg123() {
    if ( LibMpg ) {
        return true;
    }

    LibMpg = GRuntime.LoadDynamicLib( "libmpg123-0" );
    if ( !LibMpg ) {
        GLogger.Printf( "Failed to open codec library\n" );
        return false;
    }

    bool bError = false;

    #define GET_PROC_ADDRESS( Proc ) { \
        if ( !GRuntime.GetProcAddress( LibMpg, &Proc, FString::Fmt( "mpg123%s", AN_STRINGIFY( Proc )+3 ) ) ) { \
            GLogger.Printf( "Failed to load %s\n", AN_STRINGIFY(Proc) ); \
            bError = true; \
        } \
    }

    GET_PROC_ADDRESS( mpg_init );
    GET_PROC_ADDRESS( mpg_exit );
    GET_PROC_ADDRESS( mpg_new );
    GET_PROC_ADDRESS( mpg_delete );
    GET_PROC_ADDRESS( mpg_plain_strerror );
    GET_PROC_ADDRESS( mpg_strerror );
    GET_PROC_ADDRESS( mpg_errcode );
    GET_PROC_ADDRESS( mpg_open );
    GET_PROC_ADDRESS( mpg_open_fd );
    GET_PROC_ADDRESS( mpg_open_handle );
    GET_PROC_ADDRESS( mpg_close );
    GET_PROC_ADDRESS( mpg_read );
    GET_PROC_ADDRESS( mpg_tell );
    GET_PROC_ADDRESS( mpg_seek );
    GET_PROC_ADDRESS( mpg_getformat );
    GET_PROC_ADDRESS( mpg_format_none );
    GET_PROC_ADDRESS( mpg_format );
    GET_PROC_ADDRESS( mpg_outblock );
    GET_PROC_ADDRESS( mpg_length );
    GET_PROC_ADDRESS( mpg_replace_reader_handle );
    
    if ( bError ) {
        
        GRuntime.UnloadDynamicLib( LibMpg );
        LibMpg = NULL;

        return false;
    }

    int Result = MPG123_OK;
    Result = mpg_init();
	if ( Result != MPG123_OK ) {
        GLogger.Printf( "Failed to initialize mp3 decoder: %s\n", mpg_plain_strerror( Result ) );

        GRuntime.UnloadDynamicLib( LibMpg );
        LibMpg = NULL;

        return false;
	}

    return true;
}

void UnloadLibMpg123() {
    if ( LibMpg ) {
        mpg_exit();

        GRuntime.UnloadDynamicLib( LibMpg );
        LibMpg = NULL;
    }
}

AN_CLASS_META_NO_ATTRIBS( FMp3AudioTrack )
AN_CLASS_META_NO_ATTRIBS( FMp3Decoder )

FMp3AudioTrack::FMp3AudioTrack() {
    LoadLibMpg123();

    Handle = NULL;
}

FMp3AudioTrack::~FMp3AudioTrack() {
    if ( Handle ) {
        mpg_close( Handle );
        mpg_delete( Handle );
    }
}

bool FMp3AudioTrack::InitializeFileStream( const char * _FileName ) {
    int Result = MPG123_OK;
    long SampleRate = 0;
    int Encoding = 0;

    assert( Handle == NULL );    

    if ( !LibMpg ) {
        return false;
    }

    if ( ( Handle = mpg_new( NULL, &Result ) ) == NULL ) {
        GLogger.Printf( "Failed to create mp3 handle: %s\n", mpg_strerror( Handle ) );
        return false;
    }

    if ( mpg_open( Handle, _FileName ) != MPG123_OK ) {
        GLogger.Printf( "Failed to open file %s : %s\n", _FileName, mpg_strerror( Handle ) );
        mpg_delete( Handle );
        Handle = NULL;
        return false;
    }    

    if ( mpg_getformat( Handle, &SampleRate, &NumChannels, &Encoding ) != MPG123_OK ) {
        GLogger.Printf( "Failed to get file format: %s\n", mpg_strerror( Handle ) );
        mpg_close( Handle );
        mpg_delete( Handle );
        Handle = NULL;
        return false;
    }

    BlockSize = mpg_outblock( Handle );

    return true;
}

static ssize_t ReadFS( void * _File, void * _Buffer, size_t _BufferLength ) {
    FFileStream * File = static_cast< FFileStream * >( _File );

    File->Read( _Buffer, _BufferLength );
    return File->GetReadBytesCount();
}

static off_t SeekFS( void * _File, off_t _Offset, int _Origin ) {
    FFileStream * File = static_cast< FFileStream * >( _File );
    int Result = -1;

    switch ( _Origin ) {
	case SEEK_CUR: Result = File->SeekCur( _Offset ); break;
	case SEEK_END: Result = File->SeekEnd( _Offset ); break;
	case SEEK_SET: Result = File->SeekSet( _Offset ); break;
    }

    return ( Result == 0 ) ? File->Tell() : -1;
}

static ssize_t ReadMem( void * _File, void * _Buffer, size_t _BufferLength ) {
    FMemoryStream * File = static_cast< FMemoryStream * >( _File );

    File->Read( _Buffer, _BufferLength );
    return File->GetReadBytesCount();
}

static off_t SeekMem( void * _File, off_t _Offset, int _Origin ) {
    FMemoryStream * File = static_cast< FMemoryStream * >( _File );
    int Result = -1;

    switch ( _Origin ) {
    case SEEK_CUR: Result = File->SeekCur( _Offset ); break;
    case SEEK_END: Result = File->SeekEnd( _Offset ); break;
    case SEEK_SET: Result = File->SeekSet( _Offset ); break;
    }

    return ( Result == 0 ) ? File->Tell() : -1;
}

bool FMp3AudioTrack::InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) {
    int Result = MPG123_OK;
    long SampleRate = 0;
    int Encoding = 0;

    assert( Handle == NULL );    

    if ( !LibMpg ) {
        return false;
    }

    FMemoryStream File;

    if ( !File.OpenRead( "mpg", _EncodedData, _EncodedDataLength ) ) {
        return false;
    }

    if ( ( Handle = mpg_new( NULL, &Result ) ) == NULL ) {
        GLogger.Printf( "Failed to create mp3 handle: %s\n", mpg_strerror( Handle ) );
        return false;
    }

    if ( mpg_replace_reader_handle( Handle, ReadMem, SeekMem, NULL ) != MPG123_OK ) {
        GLogger.Printf( "Failed to set file callbacks: %s\n", mpg_strerror( Handle ) );
        mpg_delete( Handle );
        Handle = NULL;
        return false;
    }

    if ( mpg_open_handle( Handle, &File ) != MPG123_OK ) {
        GLogger.Printf( "Failed to open handle: %s\n", mpg_strerror( Handle ) );
        mpg_delete( Handle );
        Handle = NULL;
        return false;
    }

    if ( mpg_getformat( Handle, &SampleRate, &NumChannels, &Encoding ) != MPG123_OK ) {
        GLogger.Printf( "Failed to get file format: %s\n", mpg_strerror( Handle ) );
        mpg_close( Handle );
        mpg_delete( Handle );
        Handle = NULL;
        return false;
    }

    BlockSize = mpg_outblock( Handle );

    return true;
}

void FMp3AudioTrack::StreamRewind() {
    if ( Handle ) {
        mpg_seek( Handle, 0, SEEK_SET );
    }
}

void FMp3AudioTrack::StreamSeek( int _PositionInSamples ) {
    if ( Handle ) {
        mpg_seek( Handle, _PositionInSamples, SEEK_CUR );
    }
}

int FMp3AudioTrack::StreamDecodePCM( short * _Buffer, int _NumShorts ) {
    if ( Handle ) {
        int Result;
        size_t BytesRead = 0;
        int NumSamples = 0;
        int BufferSize;
        byte * pBuffer = (byte *) _Buffer;
        do {
            BufferSize = FMath::Min( _NumShorts<<1, BlockSize );
            Result = mpg_read( Handle, pBuffer, BufferSize, &BytesRead );
            NumSamples += BytesRead;
            pBuffer += BytesRead;
            _NumShorts -= BytesRead>>1;
        } while ( _NumShorts > 0 && BytesRead && Result == MPG123_OK );

        return NumSamples >> 1;
    }
    return 0;
}

FMp3Decoder::FMp3Decoder() {

}

IAudioStreamInterface * FMp3Decoder::CreateAudioStream() {
    return CreateInstanceOf< FMp3AudioTrack >();
}

bool FMp3Decoder::DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    mpg123_handle * mh = NULL;
    int Result = MPG123_OK;
    long SampleRate = 0;
    int Channels = 0;
    int Encoding = 0;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;

    if ( _PCM ) *_PCM = NULL;

    LoadLibMpg123();

     if ( !LibMpg ) {
        return false;
    }

    if ( ( mh = mpg_new( NULL, &Result ) ) == NULL ) {
        GLogger.Printf( "Failed to create mp3 handle: %s\n", mpg_strerror( mh ) );
        return false;
    }

    if ( mpg_open( mh, _FileName ) != MPG123_OK ) {
        GLogger.Printf( "Failed to open file %s : %s\n", _FileName, mpg_strerror( mh ) );
        mpg_delete( mh );
        return false;
    }    

    if ( mpg_getformat( mh, &SampleRate, &Channels, &Encoding ) != MPG123_OK ) {
        GLogger.Printf( "Failed to get file format: %s\n", mpg_strerror( mh ) );
        mpg_close( mh );
        mpg_delete( mh );
        return false;
    }

#if 0
    off_t NumSamples = mpg_length( mh ); // FIXME?
#else
    off_t NumSamples = mpg_seek( mh, 0, SEEK_END );
    mpg_seek( mh, 0, SEEK_SET );
#endif

    if ( _PCM ) {
        int ReadBlockSize = mpg_outblock( mh );

        *_PCM = (short *)GMainMemoryZone.Alloc( NumSamples * Channels * sizeof( short ), 1 );
        byte * pBuffer = (byte *) *_PCM;

        size_t BytesRead = 0;
        off_t CheckSum = 0;
        do {
            Result = mpg_read( mh, pBuffer, ReadBlockSize, &BytesRead );
            CheckSum += BytesRead;
            pBuffer += BytesRead;
        } while ( BytesRead && Result == MPG123_OK );

        if ( Result != MPG123_DONE ) {
            GLogger.Printf( "Warning: Decoding ended prematurely because: %s\n", ( Result == MPG123_ERR ? mpg_strerror( mh ) : mpg_plain_strerror( Result ) ) );
        }

        assert( CheckSum == NumSamples * Channels * sizeof( short ) );
    }

    *_SamplesCount = NumSamples;
    *_Channels = Channels;
    *_SampleRate = SampleRate;
    *_BitsPerSample = 16;

    mpg_close( mh );
    mpg_delete( mh );

    return true;
}

bool FMp3Decoder::ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    mpg123_handle * mh = NULL;
    int Result = MPG123_OK;
    long SampleRate = 0;
    int Channels = 0;
    int Encoding = 0;

    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;

    LoadLibMpg123();

     if ( !LibMpg ) {
        return false;
    }

    if ( ( mh = mpg_new( NULL, &Result ) ) == NULL ) {
        GLogger.Printf( "Failed to create mp3 handle: %s\n", mpg_strerror( mh ) );
        return false;
    }

    if ( mpg_replace_reader_handle( mh, ReadFS, SeekFS, NULL ) != MPG123_OK ) {
        GLogger.Printf( "Failed to set file callbacks: %s\n", mpg_strerror( mh ) );
        mpg_delete( mh );
        return false;
    }

    FFileStream File;

    if ( !File.OpenRead( _FileName ) ) {
        GLogger.Printf( "Failed to open file %s\n", _FileName );
        mpg_delete( mh );
        return false;
    }

    if ( mpg_open_handle( mh, ( void * )&File ) != MPG123_OK ) {
        GLogger.Printf( "Failed to open handle: %s\n", mpg_strerror( mh ) );
        mpg_delete( mh );
        return false;
    }

    if ( mpg_getformat( mh, &SampleRate, &Channels, &Encoding ) != MPG123_OK ) {
        GLogger.Printf( "Failed to get file format: %s\n", mpg_strerror( mh ) );
        mpg_close( mh );
        mpg_delete( mh );
        return false;
    }

    *_SamplesCount = mpg_seek( mh, 0, SEEK_END );

    mpg_close( mh );
    mpg_delete( mh );

    File.SeekEnd( 0 );
    int BufferLength = File.Tell();
    byte * Buffer = ( byte * )GMainMemoryZone.Alloc( BufferLength, 1 );
    File.SeekSet( 0 );
    File.Read( Buffer, BufferLength );

    *_Channels = Channels;
    *_SampleRate = SampleRate;
    *_BitsPerSample = 16;
    *_EncodedData = Buffer;
    *_EncodedDataLength = BufferLength;

    return true;
}
