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

#include <Engine/Core/Public/String.h>

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#ifdef AN_COMPILER_GCC
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#include <stb_sprintf.h>

const char FString::__NullCString[] = { '\0' };
const FString FString::__NullFString;

int FString::Sprintf( char * _Buffer, int _Size, const char * _Format, ... ) {
    return stbsp_snprintf( _Buffer, _Size, _Format );
}

int FString::SprintfUnsafe( char * _Buffer, const char * _Format, ... ) {
    return stbsp_sprintf( _Buffer, _Format );
}

int FString::vsnprintf( char * _Buffer, int _Size, const char * _Format, va_list _VaList ) {
    return stbsp_vsnprintf( _Buffer, _Size, _Format, _VaList );
}

// Allows max nested calls = 4
char * FString::Fmt( const char * _Format, ... ) {
    thread_local static char String[4][16384];
    thread_local static int Index = 0;
    va_list VaList;
    Index = ( Index + 1 ) & 3; // for nested calls
    va_start( VaList, _Format );
    stbsp_vsnprintf( String[Index], sizeof(String[0]), _Format, VaList );
    va_end( VaList );
    return String[Index];
}

void FString::ReallocIfNeed( int _Alloc, bool _CopyOld ) {
    AN_ASSERT( _Alloc > 0, "Invalid string size" );

    if ( _Alloc <= Allocated ) {
        return;
    }

    int mod = _Alloc % GRANULARITY;
    if ( mod ) {
        _Alloc = _Alloc + GRANULARITY - mod;
    }

    if ( StringData == StaticData ) {
        StringData = ( char * )Allocator::Inst().Alloc1( _Alloc );

        if ( _CopyOld ) {
            memcpy( StringData, StaticData, StringLength );
        } else {
            StringData[0] = 0;
            StringLength = 0;
        }

        Allocated = _Alloc;
        return;
    }

    StringData = ( char * )Allocator::Inst().Extend1( StringData, Allocated, _Alloc, _CopyOld );
    Allocated = _Alloc;

    if ( !_CopyOld ) {
        StringData[0] = 0;
        StringLength = 0;
    }
}

static int OptimizePath( char * _Path, int _Length ) {
    char * s = _Path;
    char * t;
    char tmp;
    int ofs = -1;
    int num;
    for ( ; *s ; ) {
        // skip multiple series of '/';
        num=0;
        while ( *(s+num) == '/' ) {
            num++;
        }
        if ( num > 0 ) {
            memmove( s, s + num, _Length - ( s + num - _Path ) + 1 ); // + 1 for trailing zero
            _Length -= num;
        }
        // get dir name
        t = s;
        while ( *t && *t != '/' ) {
            t++;
        }
        tmp = *t;
        *t = 0;
        // check if "up-dir"
        if ( !strcmp( s, ".." ) ) {
            *t = tmp;
            // skip ".." or "../"
            s += 2;
            if ( *s == '/' ) {
                s++;
            }
            if ( ofs < 0 ) {
                // no offset, parse next dir
                continue;
            }
            // cut off pice of path
            memmove( _Path + ofs, s, _Length - ( s - _Path ) + 1 ); // + 1 for trailing zero
            _Length -= ( s - _Path ) - ofs;
            s = _Path + ofs;
            // get dir name
            t = s;
            while ( *t && *t != '/' ) {
                t++;
            }
            tmp = *t;
            *t = 0;
            // check if "up-dir"
            if ( !strcmp( s, ".." ) ) {
                *t = tmp;
                // skip ".."
                t = s - 2;
                // find previous offset
                while ( t >= _Path ) {
                    if ( *t == '/' ) {
                        ofs = t - _Path + 1;
                        break;
                    }
                    t--;
                }
                // no previos offset, so finish
                if ( t < _Path ) {
                    return _Length;
                }
            } else {
                *t = tmp;
            }
            // parse next dir
            continue;
        } else {
            *t = tmp;
            // save offset
            ofs = s - _Path;
            // go to next dir
            s = t;
            if ( !*s ) {
                break;
            }
            s++;
        }
    }
    return _Length;
}

FString & FString::OptimizePath() {
    StringLength = ::OptimizePath( StringData, StringLength );
    return *this;
}

void FString::OptimizePath( char * _Path ) {
    ::OptimizePath( _Path, Length( _Path ) );
}

int FString::Substring( const char * _Substring ) const {
    const char * s = strstr( StringData, _Substring );
    if ( !s ) {
        return -1;
    }
    return (int)(s - StringData);
}

FString & FString::Replace( const char * _Substring, const char * _NewStr ) {
    int len = FString::Length( _Substring );
    int index;
    while ( -1 != (index = Substring( _Substring )) ) {
        Cut( index, len );
        Insert( _NewStr, index );
    }
    return *this;
}
