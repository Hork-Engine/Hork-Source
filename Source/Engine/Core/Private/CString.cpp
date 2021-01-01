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

#include <Core/Public/CString.h>
#include <Core/Public/Std.h>
#include <Core/Public/PodStack.h>

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#ifdef AN_COMPILER_GCC
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#include "stb/stb_sprintf.h"

namespace Core {

int Stricmp( const char * _S1, const char * _S2 ) {
    char c1, c2;

    AN_ASSERT( _S1 && _S2 );

    do {
        c1 = *_S1++;
        c2 = *_S2++;

        if ( c1 != c2 ) {
            if ( c1 >= 'a' && c1 <= 'z' ) {
                c1 -= ('a' - 'A');
            }
            if ( c2 >= 'a' && c2 <= 'z' ) {
                c2 -= ('a' - 'A');
            }
            if ( c1 != c2 ) {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while ( c1 );

    return 0;
}

int StricmpN( const char * _S1, const char * _S2, int _Num ) {
    char c1, c2;

    AN_ASSERT( _S1 && _S2 && _Num >= 0 );

    do {
        c1 = *_S1++;
        c2 = *_S2++;

        if ( !_Num-- ) {
            return 0;
        }

        if ( c1 != c2 ) {
            if ( c1 >= 'a' && c1 <= 'z' ) {
                c1 -= ('a' - 'A');
            }
            if ( c2 >= 'a' && c2 <= 'z' ) {
                c2 -= ('a' - 'A');
            }
            if ( c1 != c2 ) {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while ( c1 );

    return 0;
}

int Strcmp( const char * _S1, const char * _S2 ) {
    AN_ASSERT( _S1 && _S2 );

    while ( *_S1 == *_S2 ) {
        if ( !*_S1 ) {
            return 0;
        }
        _S1++;
        _S2++;
    }
    return (int)((uint8_t)*_S1 - (uint8_t)*_S2);
}

int StrcmpN( const char * _S1, const char * _S2, int _Num ) {
    char c1, c2;

    AN_ASSERT( _S1 && _S2 && _Num >= 0 );

    do {
        c1 = *_S1++;
        c2 = *_S2++;

        if ( !_Num-- ) {
            return 0;
        }

        if ( c1 != c2 ) {
            return (int)((uint8_t)c1 - (uint8_t)c2);
        }

    } while ( c1 );

    return 0;
}

int Sprintf( char * _Buffer, int _Size, const char * _Format, ... ) {
    int result;
    va_list va;

    va_start( va, _Format );
    result = stbsp_vsnprintf( _Buffer, _Size, _Format, va );
    va_end( va );

    return result;
}

int VSprintf( char * _Buffer, int _Size, const char * _Format, va_list _VaList ) {
    AN_ASSERT( _Buffer && _Format );

    return stbsp_vsnprintf( _Buffer, _Size, _Format, _VaList );
}

char * Fmt( const char * _Format, ... ) {
    AN_ASSERT( _Format );
    thread_local static char String[4][16384];
    thread_local static int Index = 0;
    va_list VaList;
    Index = ( Index + 1 ) & 3; // for nested calls
    va_start( VaList, _Format );
    stbsp_vsnprintf( String[Index], sizeof(String[0]), _Format, VaList );
    va_end( VaList );
    return String[Index];
}

void Strcat( char * _Dest, size_t _DestCapacity, const char * _Src ) {
    if ( !_Dest || !_Src ) {
        return;
    }
//#ifdef AN_COMPILER_MSVC
//    strcat_s( _Dest, _DestCapacity, _Src );
//#else
    size_t destLength = Strlen( _Dest );
    if ( destLength >= _DestCapacity ) {
        return;
    }
    Strcpy( _Dest + destLength, _DestCapacity - destLength, _Src );
//#endif
}

void StrcatN( char * _Dest, size_t _DestCapacity, const char * _Src, int _Num ) {
    if ( !_Dest || !_Src ) {
        return;
    }

    size_t destLength = Strlen( _Dest );
    if ( destLength >= _DestCapacity ) {
        return;
    }

    int len = Strlen( _Src );
    if ( len > _Num ) {
        len = _Num;
    }

    StrcpyN( _Dest + destLength, _DestCapacity - destLength, _Src, _Num );
}

void Strcpy( char * _Dest, size_t _DestCapacity, const char * _Src ) {
    if ( !_Dest ) {
        return;
    }
    if ( !_Src ) {
        _Src = "";
    }
//#ifdef AN_COMPILER_MSVC
//    strcpy_s( _Dest, _DestCapacity, _Src );
//#else
    if ( _DestCapacity > 0 ) {
        while ( *_Src && --_DestCapacity != 0 ) {
            *_Dest++ = *_Src++;
        }
        *_Dest = 0;
    }
//#endif
}

void StrcpyN( char * _Dest, size_t _DestCapacity, const char * _Src, int _Num ) {
    if ( !_Dest ) {
        return;
    }
    if ( !_Src ) {
        _Src = "";
    }
//#ifdef AN_COMPILER_MSVC
//    strncpy_s( _Dest, _DestCapacity, _Src, _Num );
//#else
    if ( _DestCapacity > 0 && _Num > 0 ) {
        while ( *_Src && --_DestCapacity != 0 && --_Num != -1 ) {
            *_Dest++ = *_Src++;
        }
        *_Dest = 0;
    }
//#endif
}

char * ToLower( char * _Str ) {
    char * p = _Str;
    if ( p ) {
        while ( *p ) {
            *p = ::tolower( *p );
            p++;
        }
    }
    return _Str;
}

char * ToUpper( char * _Str ) {
    char * p = _Str;
    if ( p ) {
        while ( *p ) {
            *p = ::toupper( *p );
            p++;
        }
    }
    return _Str;
}

int Strlen( const char * _Str ) {
    if ( !_Str ) {
        return 0;
    }
    const char *p = _Str;
    while ( *p ) {
        p++;
    }
    return p - _Str;

    //return (int)strlen( _Str );
}

int StrContains( const char * _String, char _Ch ) {
    if ( _String ) {
        for ( const char *s = _String ; *s ; s++ ) {
            if ( *s == _Ch ) {
                return s - _String;
            }
        }
    }
    return -1;
}

int Substring( const char * _Str, const char * _SubStr ) {
    if ( !_Str || !_SubStr ) {
        return -1;
    }
    const char * s = strstr( _Str, _SubStr );
    if ( !s ) {
        return -1;
    }
    return (int)(s - _Str);
}

int SubstringIcmp( const char * _Str, const char * _SubStr ) {
    if ( !_Str || !_SubStr ) {
        return -1;
    }
    const char * s = _Str;
    int length = Strlen( _SubStr );
    while ( *s ) {
        if ( StricmpN( s, _SubStr, length ) == 0 ) {
            return (int)(s - _Str);
        }
        ++s;
    }
    return -1;
}

uint32_t HexToUInt32( const char * _Str, int _Len ) {
    uint32_t value = 0;

    if ( !_Str ) {
        return 0;
    }

    for ( int i = StdMax( 0, _Len - 8 ) ; i < _Len ; i++ ) {
        uint32_t ch = _Str[i];
        if ( ch >= 'A' && ch <= 'F' ) {
            value = (value<<4) + ch - 'A' + 10;
        }
        else if ( ch >= 'a' && ch <= 'f' ) {
            value = (value<<4) + ch - 'a' + 10;
        }
        else if ( ch >= '0' && ch <= '9' ) {
            value = (value<<4) + ch - '0';
        }
        else {
            return value;
        }
    }

    return value;
}

uint64_t HexToUInt64( const char * _Str, int _Len ) {
    uint64_t value = 0;

    if ( !_Str ) {
        return 0;
    }

    for ( int i = StdMax( 0, _Len - 16 ) ; i < _Len ; i++ ) {
        uint64_t ch = _Str[i];
        if ( ch >= 'A' && ch <= 'F' ) {
            value = (value<<4) + ch - 'A' + 10;
        }
        else if ( ch >= 'a' && ch <= 'f' ) {
            value = (value<<4) + ch - 'a' + 10;
        }
        else if ( ch >= '0' && ch <= '9' ) {
            value = (value<<4) + ch - '0';
        }
        else {
            return value;
        }
    }

    return value;
}

int CmpPath( const char * _Path1, const char * _Path2 ) {
    char c1, c2;

    AN_ASSERT( _Path1 && _Path2 );

    do {
        c1 = *_Path1++;
        c2 = *_Path2++;

        if ( c1 != c2 ) {
            if ( c1 >= 'a' && c1 <= 'z' ) {
                c1 -= ('a' - 'A');
            } else if ( c1 == '\\' ) {
                c1 = '/';
            }
            if ( c2 >= 'a' && c2 <= 'z' ) {
                c2 -= ('a' - 'A');
            } else if ( c2 == '\\' ) {
                c2 = '/';
            }
            if ( c1 != c2 ) {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while ( c1 );

    return 0;
}

int CmpPathN( const char * _Path1, const char * _Path2, int _Num ) {
    char c1, c2;

    AN_ASSERT( _Path1 && _Path2 && _Num >= 0 );

    do {
        c1 = *_Path1++;
        c2 = *_Path2++;

        if ( !_Num-- ) {
            return 0;
        }

        if ( c1 != c2 ) {
            if ( c1 >= 'a' && c1 <= 'z' ) {
                c1 -= ('a' - 'A');
            } else if ( c1 == '\\' ) {
                c1 = '/';
            }
            if ( c2 >= 'a' && c2 <= 'z' ) {
                c2 -= ('a' - 'A');
            } else if ( c2 == '\\' ) {
                c2 = '/';
            }
            if ( c1 != c2 ) {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while ( c1 );

    return 0;
}

void FixSeparator( char * _Path ) {
    if ( !_Path ) {
        return;
    }
    while ( *_Path ) {
        if ( *_Path == '\\' ) {
            *_Path = '/';
        }
        _Path++;
    }
}

int FixPath( char * _Path, int _Length ) {
    if ( !_Path ) {
        return 0;
    }
    char * s = _Path;
    char * t;
    char tmp;
    int num;
    int ofs;
    TPodStack< int > offsets;
#ifdef AN_OS_LINUX
    bool root = *s == '/' || *s == '\\';
#else
    bool root = false;
#endif
    while ( *s ) {
        // skip multiple series of '/'
        num = 0;
        while ( *(s+num) == '/' || *(s+num) == '\\' ) {
            num++;
        }
        if ( num > 0 ) {
            if ( root ) {
                // Keep '/' at start of the path
                num--;
                *s = '/'; // replace '\\' by '/' (if any)
                s++;
            }
            Memmove( s, s + num, _Length - ( s + num - _Path ) + 1 ); // + 1 for trailing zero
            _Length -= num;
        }
        root = false;
        // get dir name
        t = s;
        while ( *t && *t != '/' && *t != '\\' ) {
            t++;
        }
        tmp = *t;
        if ( tmp == '\\' ) {
            tmp = '/';
        }
        *t = 0;
        // check if "up-dir"
        if ( !Strcmp( s, ".." ) ) {
            *t = tmp;
            // skip ".." or "../"
            s += 2;
            if ( *s == '/' ) {
                s++;
            }
            if ( offsets.IsEmpty() ) {
                // no offset, parse next dir
                continue;
            }
            offsets.Pop( &ofs );
            // cut off pice of path
            Memmove( _Path + ofs, s, _Length - ( s - _Path ) + 1 ); // + 1 for trailing zero
            _Length -= ( s - _Path ) - ofs;
            s = _Path + ofs;
            // parse next dir
            continue;
        } else {
            *t = tmp;
            // save offset
            offsets.Push( s - _Path );
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

int FixPath( char * _Path ) {
    return FixPath( _Path, Strlen( _Path ) );
}

int FindPath( const char * _Path ) {
    int len = Strlen( _Path );
    const char * p = _Path + len;
    while ( --p >= _Path ) {
        if ( IsPathSeparator( *p ) ) {
            return p - _Path + 1;
        }
    }
    return 0;
}

int FindExt( const char * _Path ) {
    int len = Strlen( _Path );
    const char * p = _Path + len;
    while ( --p >= _Path && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - _Path;
        }
    }
    return len;
}

int FindExtWithoutDot( const char * _Path ) {
    int len = Strlen( _Path );
    const char * p = _Path + len;
    while ( --p >= _Path && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - _Path + 1;
        }
    }
    return len;
}

bool IsPathSeparator( char _Char ) {
#ifdef AN_OS_WIN32
    return _Char == '/' || _Char == '\\';
#else
    return _Char == '/';
#endif
}

}
