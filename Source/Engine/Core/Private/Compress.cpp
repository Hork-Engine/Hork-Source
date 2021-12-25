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

#include <Core/Public/Compress.h>
#include <Core/Public/BaseMath.h>
#include <Core/Public/Logger.h>
#include <fastlz/fastlz.h>
#include <miniz/miniz.h>

namespace Core {

uint32_t Crc32( uint32_t _Crc, byte const * _Data, size_t _SizeInBytes ) {
    return (uint32_t)mz_crc32( _Crc, _Data, _SizeInBytes );
}

uint32_t Adler32( uint32_t _Adler, byte const * _Data, size_t _SizeInBytes ) {
    return (uint32_t)mz_adler32( _Adler, _Data, _SizeInBytes );
}

size_t FastLZMaxCompressedSize( size_t SourceSize ) {
    // The output buffer must be at least 5% larger than the input buffer
    // and can not be smaller than 66 bytes.
    size_t size = Math::Floor( SourceSize * 1.05 + 0.5 );
    if ( size < 66 ) {
        size = 66;
    }
    return size;
}

bool FastLZCompress( byte * pCompressedData, size_t * pCompressedSize, byte const * pSource, size_t SourceSize, int Level ) {
    *pCompressedSize = 0;

    if ( SourceSize < 16 )
    {
        // too small buffer
        GLogger.Printf( "FastLZCompress: failed to compress, source size < 16 bytes!\n" );
        return false;
    }

    int result = ( Level == FASTLZ_DEFAULT_COMPRESSION )
        ? fastlz_compress( pSource, SourceSize, pCompressedData )
        : fastlz_compress_level( Level, pSource, SourceSize, pCompressedData );

    if ( result <= 0 ) {
        return false;
    }

    *pCompressedSize = result;

    return true;
}

bool FastLZDecompress( byte const * pCompressedData, size_t CompressedSize, byte * pDest, size_t * pDestSize, int MaxOut ) {
    *pDestSize = 0;

    int result = fastlz_decompress( pCompressedData, CompressedSize, pDest, MaxOut );
    if ( result <= 0 ) {
        return false;
    }

    *pDestSize = result;

    return true;
}

size_t ZMaxCompressedSize( size_t SourceSize ) {
    return (mz_ulong)mz_compressBound( SourceSize );
}

bool ZCompress( byte * pCompressedData, size_t * pCompressedSize, byte const * pSource, size_t SourceSize, int Level ) {
    mz_ulong compressedSize = *pCompressedSize;
    *pCompressedSize = 0;
    if ( mz_compress2( pCompressedData, &compressedSize, pSource, SourceSize, Level ) != MZ_OK ) {
        return false;
    }
    *pCompressedSize = compressedSize;
    return true;
}

bool ZDecompress( byte const * pCompressedData, size_t CompressedSize, byte * pDest, size_t * pDestSize ) {
    mz_ulong decompressedSize;
    *pDestSize = 0;
    if ( mz_uncompress( pDest, &decompressedSize, pCompressedData, CompressedSize ) != MZ_OK ) {
        return false;
    }
    *pDestSize = decompressedSize;
    return true;
}

bool ZDecompressToHeap( byte const * pCompressedData, size_t CompressedSize, byte ** pDest, size_t * pDestSize ) {
    *pDest = NULL;
    *pDestSize = 0;

    if ( CompressedSize > 0xFFFFFFFFU ) {
        return false;
    }

    mz_stream stream;
    ZeroMem( &stream, sizeof( stream ) );
    stream.next_in = pCompressedData;
    stream.avail_in = (mz_uint32)CompressedSize;

    int status = mz_inflateInit( &stream );
    if ( status != MZ_OK )
        return false;

    unsigned char chunk[1024];

    size_t allocated = Align( CompressedSize<<2, 16 );
    byte * data = ( byte * )GHeapMemory.Alloc( allocated, 16 );
    size_t size = 0;

    do {
        stream.next_out = chunk;
        stream.avail_out = sizeof( chunk );
        status = mz_inflate( &stream, MZ_NO_FLUSH );
        if ( status == MZ_STREAM_END || status == MZ_OK ) {
            if ( stream.total_out > allocated ) {
                allocated <<= 1;
                data = ( byte * )GHeapMemory.Realloc( data, allocated, 16, true );
            }
            Memcpy( data + size, chunk, stream.total_out - size );
        }
        size = stream.total_out;
    } while (status == MZ_OK);

    mz_inflateEnd( &stream );

    if ( status != MZ_STREAM_END && status != MZ_OK ) {
        GHeapMemory.Free( data );
        return false;
    }

    *pDest = data;
    *pDestSize = stream.total_out;

    return true;
}

}
