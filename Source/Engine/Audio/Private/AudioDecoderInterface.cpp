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

#include <Engine/Audio/Public/AudioDecoderInterface.h>
#include <Engine/Core/Public/Logger.h>

AN_CLASS_META( IAudioStreamInterface )
AN_CLASS_META( IAudioDecoderInterface )

bool IAudioStreamInterface::InitializeFileStream( const char * _FileName ) {
    return false;
}

bool IAudioStreamInterface::InitializeMemoryStream( const byte * _EncodedData, int _EncodedDataLength ) {
    return false;
}

void IAudioStreamInterface::StreamRewind() {

}

void IAudioStreamInterface::StreamSeek( int _PositionInSamples ) {

}

int IAudioStreamInterface::StreamDecodePCM( short * _Buffer, int _NumShorts ) {
    return 0;
}

IAudioStreamInterface * IAudioDecoderInterface::CreateAudioStream() {
    return nullptr;
}

bool IAudioDecoderInterface::DecodePCM( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    if ( _PCM ) *_PCM = NULL;
    GLogger.Printf( "IAudioDecoderInterface::DecodePCM is not implemented\n" );
    return false;
}

bool IAudioDecoderInterface::DecodePCM( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, short ** _PCM ) {
    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_BitsPerSample = 0;
    if ( _PCM ) *_PCM = NULL;
    GLogger.Printf( "IAudioDecoderInterface::DecodePCM is not implemented\n" );
    return false;
}

bool IAudioDecoderInterface::ReadEncoded( const char * _FileName, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;
    GLogger.Printf( "IAudioDecoderInterface::ReadEncoded is not implemented\n" );
    return false;
}

bool IAudioDecoderInterface::ReadEncoded( const char * _FileName, const byte * _Data, size_t _DataLength, int * _SamplesCount, int * _Channels, int * _SampleRate, int * _BitsPerSample, byte ** _EncodedData, size_t * _EncodedDataLength ) {
    *_SamplesCount = 0;
    *_Channels = 0;
    *_SampleRate = 0;
    *_EncodedData = NULL;
    *_EncodedDataLength = 0;
    GLogger.Printf( "IAudioDecoderInterface::ReadEncoded is not implemented\n" );
    return false;
}
