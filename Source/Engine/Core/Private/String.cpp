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

const char AString::__NullCString[] = { '\0' };
const AString AString::__NullFString;

int AString::Sprintf( char * _Buffer, int _Size, const char * _Format, ... ) {
    return stbsp_snprintf( _Buffer, _Size, _Format );
}

int AString::SprintfUnsafe( char * _Buffer, const char * _Format, ... ) {
    return stbsp_sprintf( _Buffer, _Format );
}

int AString::vsnprintf( char * _Buffer, int _Size, const char * _Format, va_list _VaList ) {
    return stbsp_vsnprintf( _Buffer, _Size, _Format, _VaList );
}

// Allows max nested calls = 4
char * AString::Fmt( const char * _Format, ... ) {
    thread_local static char String[4][16384];
    thread_local static int Index = 0;
    va_list VaList;
    Index = ( Index + 1 ) & 3; // for nested calls
    va_start( VaList, _Format );
    stbsp_vsnprintf( String[Index], sizeof(String[0]), _Format, VaList );
    va_end( VaList );
    return String[Index];
}

void AString::ReallocIfNeed( int _Alloc, bool _CopyOld ) {
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

void AString::operator=( const char * _Str ) {
    int newlength;
    int diff;
    int i;

    if ( !_Str ) {
        _Str = "(null)";
    }

    if ( _Str == StringData ) {
        return; // copying same thing
    }

    // check if we're aliasing
    if ( _Str >= StringData && _Str <= StringData + StringLength ) {
        diff = _Str - StringData;
        AN_ASSERT( Length( _Str ) < StringLength, "AString=" );
        for ( i = 0; _Str[ i ]; i++ ) {
            StringData[ i ] = _Str[ i ];
        }
        StringData[ i ] = '\0';
        StringLength -= diff;
        return;
    }

    newlength = Length( _Str );
    ReallocIfNeed( newlength+1, false );
    strcpy( StringData, _Str );
    StringLength = newlength;
}

void AString::CopyN( const char * _Str, int _Num ) {
    int newlength;
    //int diff;
    int i;

    if ( !_Str ) {
        _Str = "(null)";
    }

    if ( _Str == StringData ) {
        return; // copying same thing
    }

    // check if we're aliasing
    if ( _Str >= StringData && _Str <= StringData + StringLength ) {
        //diff = _Str - StringData;
        AN_ASSERT( Length( _Str ) < StringLength, "AString::CopyN" );
        for ( i = 0; _Str[ i ] && i<_Num ; i++ ) {
            StringData[ i ] = _Str[ i ];
        }
        StringData[ i ] = '\0';
        StringLength = i;
        //StringLength -= diff;
        return;
    }

    newlength = Length( _Str );
    if ( newlength > _Num ) {
        newlength = _Num;
    }
    ReallocIfNeed( newlength+1, false );
    strncpy( StringData, _Str, newlength );
    StringData[newlength] = 0;
    StringLength = newlength;
}

int AString::Icmp( const char* _S1, const char* _S2 ) {
    char c1, c2;

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
                return c1 < c2 ? -1 : 1;        // strings not equal
            }
        }
    } while ( c1 );

    return 0;        // strings are equal
}

int AString::IcmpN( const char* _S1, const char* _S2, int _Num ) {
    char        c1, c2;

    AN_ASSERT( _Num >= 0, "AString::IcmpN" );

    do {
        c1 = *_S1++;
        c2 = *_S2++;

        if ( !_Num-- ) {
            return 0;        // strings are equal until end point
        }

        if ( c1 != c2 ) {
            if ( c1 >= 'a' && c1 <= 'z' ) {
                c1 -= ('a' - 'A');
            }
            if ( c2 >= 'a' && c2 <= 'z' ) {
                c2 -= ('a' - 'A');
            }
            if ( c1 != c2 ) {
                return c1 < c2 ? -1 : 1;        // strings not equal
            }
        }
    } while ( c1 );

    return 0;        // strings are equal
}

int AString::CmpPath( const char* _S1, const char* _S2 ) {
    char c1, c2;

    do {
        c1 = *_S1++;
        c2 = *_S2++;

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
                return c1 < c2 ? -1 : 1;        // strings not equal
            }
        }
    } while ( c1 );

    return 0;        // strings are equal
}

int AString::CmpPathN( const char* _S1, const char* _S2, int _Num ) {
    char c1, c2;

    AN_ASSERT( _Num >= 0, "AString::CmpPathN" );

    do {
        c1 = *_S1++;
        c2 = *_S2++;

        if ( !_Num-- ) {
            return 0;        // strings are equal until end point
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
                return c1 < c2 ? -1 : 1;        // strings not equal
            }
        }
    } while ( c1 );

    return 0;        // strings are equal
}

int AString::Cmp( const char * _S1, const char * _S2 ) {
    while ( *_S1 == *_S2 ) {
        if ( !*_S1 ) {        // strings are equal
            return 0;
        }
        _S1++;
        _S2++;
    }
    // strings not equal
    return *_S1 < *_S2 ? -1 : 1;
}

int AString::CmpN( const char * _S1, const char * _S2, int _Num ) {
    char        c1, c2;

    AN_ASSERT( _Num >= 0, "AString::CmpN" );

    do {
        c1 = *_S1++;
        c2 = *_S2++;

        if ( !_Num-- ) {
            return 0;        // strings are equal until end point
        }

        if ( c1 != c2 ) {
            return c1 < c2 ? -1 : 1;
        }

    } while ( c1 );

    return 0;        // strings are equal
}

void AString::Concat( AString const & _Str ) {
    int newlength;
    int i;

    newlength = StringLength + _Str.StringLength;
    ReallocIfNeed( newlength+1, true );
    for ( i = 0; i < _Str.StringLength; i++ )
        StringData[ StringLength + i ] = _Str[i];
    StringLength = newlength;
    StringData[StringLength] = '\0';
}

void AString::Concat( const char * _Str ) {
    int newlength;
    int i;

    if ( _Str == NULL ) {
        _Str = "(null)";
    }

    newlength = StringLength + Length( _Str );
    ReallocIfNeed( newlength+1, true );
    for ( i = 0 ; _Str[i] ; i++ ) {
        StringData[StringLength + i] = _Str[i];
    }
    StringLength = newlength;
    StringData[StringLength] = '\0';
}

void AString::Concat( char _Char ) {
    int newlength;

    if ( _Char == 0 ) {
        return;
    }

    newlength = StringLength + 1;
    ReallocIfNeed( newlength+1, true );
    StringData[StringLength++] = _Char;
    StringData[StringLength] = '\0';
}

void AString::Insert( AString const & _Str, int _Index ) {
    int i;
    int textLength = _Str.Length();
    if ( _Index < 0 ) {
        _Index = 0;
    } else if ( _Index > StringLength ) {
        _Index = StringLength;
    }
    ReallocIfNeed( StringLength + textLength + 1, true );
    for ( i = StringLength; i >= _Index; i-- ) {
        StringData[ i + textLength ] = StringData[ i ];
    }
    for ( i = 0; i < textLength; i++ ) {
        StringData[ _Index + i ] = _Str[ i ];
    }
    StringLength += textLength;
}

void AString::Insert( const char * _Str, int _Index ) {
    int i;
    int textLength = Length( _Str );
    if ( _Index < 0 ) _Index = 0;
    else if ( _Index > StringLength ) _Index = StringLength;
    ReallocIfNeed( StringLength + textLength + 1, true );
    for ( i = StringLength; i >= _Index; i-- ) {
        StringData[ i + textLength ] = StringData[ i ];
    }
    for ( i = 0; i < textLength; i++ ) {
        StringData[ _Index + i ] = _Str[ i ];
    }
    StringLength += textLength;
}

void AString::Insert( char _Char, int _Index ) {
    if ( _Index < 0 ) {
        _Index = 0;
    } else if ( _Index > StringLength ) {
        _Index = StringLength;
    }
    ReallocIfNeed( StringLength + 2, true );
    for ( int i = StringLength ; i >= _Index; i-- ) {
        StringData[i+1] = StringData[i];
    }
    StringData[_Index] = _Char;
    StringLength++;
}

void AString::Replace( AString const & _Str, int _Index ) {
    int newlength = _Index + _Str.StringLength;

    ReallocIfNeed( newlength+1, true );
    for ( int i = 0; i < _Str.StringLength; i++ ) {
        StringData[ _Index + i ] = _Str[i];
    }
    StringLength = newlength;
    StringData[StringLength] = '\0';
}

void AString::Replace( const char * _Str, int _Index ) {
    int newlength = _Index + Length( _Str );

    ReallocIfNeed( newlength+1, true );
    for ( int i = 0 ; _Str[i] ; i++ ) {
        StringData[ _Index + i ] = _Str[i];
    }
    StringLength = newlength;
    StringData[StringLength] = '\0';
}

void AString::Cut( int _Index, int _Count ) {
    int i;
    AN_ASSERT( _Count > 0, "AString::Cut" );
    if ( _Index < 0 ) _Index = 0;
    else if ( _Index > StringLength ) _Index = StringLength;
    for ( i = _Index + _Count ; i < StringLength ; i++ ) {
        StringData[ i - _Count ] = StringData[ i ];
    }
    StringLength = i - _Count;
    StringData[ StringLength ] = 0;
}

int AString::Contains( char _Ch ) const {
    for ( const char *s=StringData ; *s ; s++ ) {
        if ( *s == _Ch ) {
            return s-StringData;
        }
    }
    return -1;
}

void AString::SkipTrailingZeros() {
    int i;
    for ( i = StringLength - 1; i >= 0 ; i-- ) {
        if ( StringData[ i ] != '0' ) {
            break;
        }
    }
    if ( StringData[ i ] == '.' ) {
        Resize( i );
    } else {
        Resize( i + 1 );
    }
}

void AString::FixSeparator( char * _String ) {
    while ( *_String ) {
        if ( *_String == '\\' ) {
            *_String = '/';
        }
        _String++;
    }
}

void AString::StripPath() {
    AString s(*this);
    char    *p = s.StringData + s.StringLength;
    while ( --p >= s.StringData && !IsPathSeparator( *p ) ) {
        ;
    }
    p++;
    *this = p;
}

int AString::FindPath() const {
    char * p = StringData + StringLength;
    while ( --p >= StringData ) {
        if ( IsPathSeparator( *p ) ) {
            return p - StringData + 1;
        }
    }
    return 0;
}

int AString::FindPath( const char * _Str ) {
    int len = Length( _Str );
    const char * p = _Str + len;
    while ( --p >= _Str ) {
        if ( IsPathSeparator( *p ) ) {
            return p - _Str + 1;
        }
    }
    return 0;
}

void AString::StripExt() {
    char    *p = StringData + StringLength;
    while ( --p >= StringData ) {
        if ( *p == '.' ) {
            *p = '\0';
            StringLength = p-StringData;
            break;
        }
        if ( IsPathSeparator( *p ) ) {
            break;        // no extension
        }
    }
}

void AString::StripFilename() {
    char * p = StringData + StringLength;
    while ( --p > StringData && !IsPathSeparator( *p ) ) {
        ;
    }
    *p = '\0';
    StringLength = p-StringData;
}

bool AString::CompareExt( const char * _Ext, bool _CaseSensitive ) const {
    char * p = StringData + StringLength;
    int ExtLength = Length( _Ext );
    const char * ext = _Ext + ExtLength;
    if ( _CaseSensitive ) {
        char c1, c2;
        while ( --ext >= _Ext ) {

            if ( --p < StringData ) {
                return false;
            }

            c1 = *p;
            c2 = *ext;

            if ( c1 != c2 ) {
                if ( c1 >= 'a' && c1 <= 'z' ) {
                    c1 -= ('a' - 'A');
                }
                if ( c2 >= 'a' && c2 <= 'z' ) {
                    c2 -= ('a' - 'A');
                }
                if ( c1 != c2 ) {
                    return false;
                }
            }
        }
    } else {
        while ( --ext >= _Ext ) {
            if ( --p < StringData || *p != *ext ) {
                return false;
            }
        }
    }
    return true;
}

void AString::UpdateExt( const char * _Extension ) {
    char * p = StringData + StringLength;
    while ( --p >= StringData && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return;
        }
    }
    (*this) += _Extension;
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

void AString::OptimizePath() {
    StringLength = ::OptimizePath( StringData, StringLength );
}

void AString::OptimizePath( char * _Path ) {
    ::OptimizePath( _Path, Length( _Path ) );
}

int AString::Substring( const char * _Substring ) const {
    const char * s = strstr( StringData, _Substring );
    if ( !s ) {
        return -1;
    }
    return (int)(s - StringData);
}

void AString::Replace( const char * _Substring, const char * _NewStr ) {
    int len = AString::Length( _Substring );
    int index;
    while ( -1 != (index = Substring( _Substring )) ) {
        Cut( index, len );
        Insert( _NewStr, index );
    }
}

int AString::FindExt() const {
    char * p = StringData + StringLength;
    while ( --p >= StringData && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - StringData;
        }
    }
    return StringLength;
}

int AString::FindExtWithoutDot() const {
    char * p = StringData + StringLength;
    while ( --p >= StringData && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - StringData + 1;
        }
    }
    return StringLength;
}

int AString::FindExt( const char * _Str ) {
    int len = Length(_Str);
    const char * p = _Str + len;
    while ( --p >= _Str && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - _Str;
        }
    }
    return len;
}

int AString::FindExtWithoutDot( const char * _Str ) {
    int len = Length(_Str);
    const char * p = _Str + len;
    while ( --p >= _Str && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - _Str + 1;
        }
    }
    return len;
}

int AString::Contains( const char * _String, char _Ch ) {
    for ( const char *s=_String ; *s ; s++ ) {
        if ( *s == _Ch ) {
            return s-_String;
        }
    }
    return -1;
}

int AString::Length( const char * _Str ) {
    const char *p = _Str;
    while ( *p ) {
        p++;
    }
    return p-_Str;

    //return strlen( s );
}

void AString::ToLower() {
    char * s = StringData;
    while ( *s ) {
        *s = ::tolower( *s );
        s++;
    }
}

void AString::ToUpper() {
    char * s = StringData;
    while ( *s ) {
        *s = ::toupper( *s );
        s++;
    }
}

char * AString::ToLower( char * _Str ) {
    char * str = _Str;
    while ( *_Str ) {
        *_Str = ::tolower( *_Str );
        _Str++;
    }
    return str;
}

char * AString::ToUpper( char * _Str ) {
    char * str = _Str;
    while ( *_Str ) {
        *_Str = ::toupper( *_Str );
        _Str++;
    }
    return str;
}

uint32_t AString::HexToUInt32( const char * _Str, int _Len ) {
    uint32_t value = 0;

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

uint64_t AString::HexToUInt64( const char * _Str, int _Len ) {
    uint64_t value = 0;

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
