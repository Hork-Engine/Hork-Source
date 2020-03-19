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

#include <Core/Public/FastLZCompressor.h>
#include <Core/Public/BaseMath.h>
#include "fastlz/fastlz.h"

size_t AFastLZCompressor::CalcAppropriateCompressedDataSize( size_t _SourceSize ) {
    // The output buffer must be at least 5% larger than the input buffer
    // and can not be smaller than 66 bytes.
    size_t size = Math::Floor( _SourceSize * 1.05 + 0.5 );
    if ( size < 66 ) {
        size = 66;
    }
    return size;
}

bool AFastLZCompressor::CompressData( const byte * _Data, size_t _DataSz, byte * _CompressedData, size_t & _CompressedDataSz ) {
    if ( _DataSz < 16 ) {
        // too small buffer
        return false;
    }

    int result = fastlz_compress( _Data, _DataSz, _CompressedData );
    _CompressedDataSz = result;

    return _CompressedDataSz <= _DataSz;
}

bool AFastLZCompressor::CompressDataLevel( ECompressionLevel _Level, const byte * _Data, size_t _DataSz, byte * _CompressedData, size_t & _CompressedDataSz ) {
    if ( _DataSz < 16 ) {
        // too small buffer
        return false;
    }

    int result = fastlz_compress_level( _Level, _Data, _DataSz, _CompressedData );
    _CompressedDataSz = result;

    return _CompressedDataSz <= _DataSz;
}

bool AFastLZCompressor::DecompressData( const byte * _CompressedData, size_t _CompressedDataSz, byte * _Data, size_t & _DataSz, int _MaxOut ) {
    int result = fastlz_decompress( _CompressedData, _CompressedDataSz, _Data, _MaxOut );
    _DataSz = result;

    return result > 0;
}
