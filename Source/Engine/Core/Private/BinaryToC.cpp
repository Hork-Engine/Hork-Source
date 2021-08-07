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

#include <Core/Public/BinaryToC.h>
#include <Core/Public/Memory.h>
#include <Core/Public/Compress.h>
#include <Core/Public/Logger.h>
#include <Core/Public/BaseMath.h>

namespace Core {

bool BinaryToC( const char * _SourceFile, const char * _DestFile, const char * _SymName, bool _EncodeBase85 )
{
    AFileStream source, dest;
    
    if ( !source.OpenRead( _SourceFile ) ) {
        GLogger.Printf( "Failed to open %s\n", _SourceFile );
        return false;
    }

    if ( !dest.OpenWrite( _DestFile ) ) {
        GLogger.Printf( "Failed to open %s\n", _DestFile );
        return false;
    }

    size_t size = source.SizeInBytes();
    byte * data = (byte *)GHeapMemory.Alloc( size );
    source.ReadBuffer( data, size );

    WriteBinaryToC( dest, _SymName, data, size, _EncodeBase85 );

    GHeapMemory.Free( data );

    return true;
}

bool BinaryToCompressedC( const char * _SourceFile, const char * _DestFile, const char * _SymName, bool _EncodeBase85 )
{
    AFileStream source, dest;

    if ( !source.OpenRead( _SourceFile ) ) {
        GLogger.Printf( "Failed to open %s\n", _SourceFile );
        return false;
    }

    if ( !dest.OpenWrite( _DestFile ) ) {
        GLogger.Printf( "Failed to open %s\n", _DestFile );
        return false;
    }

    size_t decompressedSize = source.SizeInBytes();
    byte * decompressedData = (byte *)GHeapMemory.Alloc( decompressedSize );
    source.ReadBuffer( decompressedData, decompressedSize );

    size_t compressedSize = Core::ZMaxCompressedSize( decompressedSize );
    byte * compressedData = (byte *)GHeapMemory.Alloc( compressedSize );
    Core::ZCompress( compressedData, &compressedSize, (byte *)decompressedData, decompressedSize, ZLIB_UBER_COMPRESSION );

    WriteBinaryToC( dest, _SymName, compressedData, compressedSize, _EncodeBase85 );

    GHeapMemory.Free( compressedData );
    GHeapMemory.Free( decompressedData );

    return true;
}

void WriteBinaryToC( IBinaryStream & Stream, const char * _SymName, const void * pData, size_t SizeInBytes, bool bEncodeBase85 )
{
    const byte * bytes = (const byte *)pData;

    if ( bEncodeBase85 ) {
        Stream.Printf( "static const char %s_Data_Base85[%d+1] =\n    \"", _SymName, (int)((SizeInBytes+3)/4)*5 );
        char prev_c = 0;
        for ( size_t i = 0; i < SizeInBytes; i += 4 ) {
            uint32_t d = *(uint32_t *)(bytes + i);
            for ( int j = 0; j < 5; j++, d /= 85 ) {
                unsigned int x = ( d % 85 ) + 35;
                char c = ( x >= '\\' ) ? x + 1 : x;
                Stream.Printf( ( c == '?' && prev_c == '?' ) ? "\\%c" : "%c", c );
                prev_c = c;
            }
            if ( (i % 112) == 112-4 )
                Stream.Printf( "\"\n    \"" );
        }
        Stream.Printf( "\";\n\n" );
    }
    else {
        Stream.Printf( "static const size_t %s_Size = %d;\n", _SymName, (int)SizeInBytes );
        Stream.Printf( "static const uint64_t %s_Data[%d] =\n{", _SymName, Align( SizeInBytes, 8 ) );
        int column = 0;
        for ( size_t i = 0; i < SizeInBytes; i += 8 ) {
            uint64_t d = *(uint64_t *)(bytes + i);
            if ( (column++ % 6) == 0 ) {
                Stream.Printf( "\n    0x%08x%08x", Math::INT64_HIGH_INT( d ), Math::INT64_LOW_INT( d ) );
            } else {
                Stream.Printf( "0x%08x%08x", Math::INT64_HIGH_INT( d ), Math::INT64_LOW_INT( d ) );
            }
            if ( i + 8 < SizeInBytes ) {
                Stream.Printf( ", " );
            }
        }
        Stream.Printf( "\n};\n\n" );
    }
}

}
