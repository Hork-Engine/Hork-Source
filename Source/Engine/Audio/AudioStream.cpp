/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "AudioStream.h"

#include <Platform/Platform.h>
#include <Core/BaseMath.h>

#include <miniaudio/miniaudio.h>

SAudioStream::SAudioStream( SFileInMemory * _pFileInMemory, int _FrameCount, int _SampleRate, int _SampleBits, int _Channels )
{
    ma_format format = ma_format_unknown;

    switch ( _SampleBits ) {
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
        // Shouldn't happen
        CriticalError( "SAudioStream::ctor: expected 8, 16 or 32 sample bits\n" );
    }

    Decoder = (ma_decoder *)GHeapMemory.Alloc( sizeof( *Decoder ) );

    ma_decoder_config config = ma_decoder_config_init( format, _Channels, _SampleRate );

    ma_result result = ma_decoder_init_memory( _pFileInMemory->GetHeapPtr(), _pFileInMemory->GetSizeInBytes(), &config, Decoder );
    if ( result != MA_SUCCESS ) {
        CriticalError( "SAudioStream::ctor: failed to initialize decoder\n" );
    }

    // Capture the reference
    pFileInMemory = _pFileInMemory;

    FrameCount = _FrameCount;
    Channels = _Channels;
    SampleBits = _SampleBits;
    SampleStride = (_SampleBits >> 3) << (_Channels - 1);
}

SAudioStream::~SAudioStream()
{
    ma_decoder_uninit( Decoder );
    GHeapMemory.Free( Decoder );
}

void SAudioStream::SeekToFrame( int FrameNum )
{
    ma_decoder_seek_to_pcm_frame( Decoder, Math::Max( 0, FrameNum ) );
}

//static const int ma_format_sample_bits[] =
//{
//    0,  // ma_format_unknown
//    8,  // ma_format_u8
//    16, // ma_format_s16
//    24, // ma_format_s24
//    32, // ma_format_s32
//    32, // ma_format_f32
//};

int SAudioStream::ReadFrames( void * _pFrames, int _FrameCount, size_t _SizeInBytes )
{
    if ( _FrameCount <= 0 ) {
        return 0;
    }

    //const int stride = ( ma_format_sample_bits[Decoder->outputFormat] >> 3 ) << ( Decoder->outputChannels - 1 );

    if ( (size_t)_FrameCount * SampleStride > _SizeInBytes ) {
        _FrameCount = _SizeInBytes / SampleStride;
    }

    return ma_decoder_read_pcm_frames( Decoder, _pFrames, _FrameCount );
}
