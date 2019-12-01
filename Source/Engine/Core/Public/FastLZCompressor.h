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

#pragma once

#include <Core/Public/BaseTypes.h>

// Lightning-fast lossless compression
class ANGIE_API AFastLZCompressor {
    AN_FORBID_COPY( AFastLZCompressor )

public:
    AFastLZCompressor() {}

    /**
    Compress a block of data in the input buffer and returns the size of
    compressed block. The size of input buffer is specified by length. The
    minimum input buffer size is 16.

    The output buffer must be at least 5% larger than the input buffer
    and can not be smaller than 66 bytes.

    The input buffer and the output buffer can not overlap.
    */
    static size_t CalcAppropriateCompressedDataSize( size_t _SourceSize );

    bool CompressData( const byte * _Data, size_t _DataSz, byte * _CompressedData, size_t & _CompressedDataSz );

    enum ECompressionLevel {
        /** The fastest compression and generally useful for short data */
        Fastes = 1,
        /** slightly slower but it gives better compression ratio */
        BetterRatio = 2
    };

    /**
    Compress a block of data in the input buffer and returns the size of
    compressed block. The size of input buffer is specified by length. The
    minimum input buffer size is 16.

    The output buffer must be at least 5% larger than the input buffer
    and can not be smaller than 66 bytes.

    The input buffer and the output buffer can not overlap.

    Note that the compressed data, regardless of the level, can always be
    decompressed using the function DecompressData.
    */
    bool CompressDataLevel( ECompressionLevel _Level, const byte * _Data, size_t _DataSz, byte * _CompressedData, size_t & _CompressedDataSz );

    /**
    Decompress a block of compressed data and returns the size of the
    decompressed block.

    The input buffer and the output buffer can not overlap.

    Decompression is memory safe and guaranteed not to write the output buffer
    more than what is specified in maxout.
    */
    bool DecompressData( const byte * _CompressedData, size_t _CompressedDataSz, byte * _Data, size_t & _DataSz, int _MaxOut );
};
