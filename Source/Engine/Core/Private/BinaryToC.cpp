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

#include <Core/Public/Alloc.h>
#include <Core/Public/Compress.h>
#include <Core/Public/IO.h>
#include <Core/Public/Logger.h>
#include <Core/Public/BaseMath.h>

namespace Core {

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

    if ( _EncodeBase85 ) {
        dest.Printf( "static const char %s_Data_Base85[%d+1] =\n    \"", _SymName, (int)((compressedSize+3)/4)*5 );
        char prev_c = 0;
        for ( size_t i = 0; i < compressedSize; i += 4 ) {
            uint32_t d = *(uint32_t *)(compressedData + i);
            for ( int j = 0; j < 5; j++, d /= 85 ) {
                unsigned int x = ( d % 85 ) + 35;
                char c = ( x >= '\\' ) ? x + 1 : x;
                dest.Printf( ( c == '?' && prev_c == '?' ) ? "\\%c" : "%c", c );
                prev_c = c;
            }
            if ( (i % 112) == 112-4 )
                dest.Printf( "\"\n    \"" );
        }
        dest.Printf( "\";\n\n" );
    }
    else {
        dest.Printf( "static const size_t %s_Size = %d;\n", _SymName, (int)compressedSize );
        dest.Printf( "static const uint64_t %s_Data[%d] =\n{", _SymName, Align( compressedSize, 8 ) );
        int column = 0;
        for ( size_t i = 0; i < compressedSize; i += 8 ) {
            uint64_t d = *(uint64_t *)(compressedData + i);
            if ( (column++ % 6) == 0 ) {
                dest.Printf( "\n    0x%08x%08x, ", Math::INT64_HIGH_INT( d ), Math::INT64_LOW_INT( d ) );
            } else {
                dest.Printf( "0x%08x%08x, ", Math::INT64_HIGH_INT( d ), Math::INT64_LOW_INT( d ) );
            }
        }
        dest.Printf( "\n};\n\n" );
    }

    GHeapMemory.Free( compressedData );
    GHeapMemory.Free( decompressedData );

    return true;
}

}
