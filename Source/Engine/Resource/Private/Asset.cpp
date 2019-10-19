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

#include <Engine/Resource/Public/Asset.h>
#include <Engine/Core/Public/Logger.h>

char * AssetParseTag( char * _Buf, const char * _Tag ) {
    int n = strlen( _Tag );
    if ( !FString::CmpN( _Buf, _Tag, n ) ) {
        _Buf += n;
        return _Buf;
    }
    return nullptr;
}

char * AssetParseName( char * _Buf, char ** _Name ) {
    char * s = _Buf;
    while ( *s && *s != '\"' ) {
        s++;
    }
    char * name = s;
    if ( *s ) {
        s++;
        name++;
    }
    while ( *s && *s != '\"' ) {
        s++;
    }
    if ( *s ) {
        *s = 0;
        s++;
    }
    *_Name = name;
    return s;
}

bool AssetReadFormat( FFileStream & f, int * _Format, int * _Version ) {
    char buf[1024];
    char * s;

    if ( !f.Gets( buf, sizeof( buf ) ) || nullptr == ( s = AssetParseTag( buf, "format " ) ) ) {
        GLogger.Printf( "Expected format description\n" );
        return false;
    }

    if ( sscanf( s, "%d %d", _Format, _Version ) != 2 ) {
        GLogger.Printf( "Expected format type and version\n" );
        return false;
    }
    return true;
}
