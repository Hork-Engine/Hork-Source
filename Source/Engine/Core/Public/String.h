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

#include "Alloc.h"
#include "HashFunc.h"
#include <stdarg.h>

/*

FString

*/
class ANGIE_API FString final {
    using Allocator = FAllocator;

public:
    static const int    GRANULARITY = 32;
    static const int    BASE_ALLOC = 32;

    FString();
    FString( const char * _Str );
    FString( const FString & _Str );
    ~FString();

    const char & operator[]( int _Index ) const;
    char & operator[]( int _Index );

    void operator=( FString const & _Str );
    void operator=( const char * _Str );

    friend FString operator+( FString const & _Str1, FString const & _Str2 );
    friend FString operator+( FString const & _Str1, const char * _Str2 );
    friend FString operator+( const char * _Str1, FString const & _Str2 );

    FString & operator+=( FString const & _Str );
    FString & operator+=( const char * _Str );

    friend bool operator==( FString const & _Str1, FString const & _Str2 );
    friend bool operator==( FString const & _Str1, const char * _Str2 );
    friend bool operator==( const char * _Str1, FString const & _Str2 );
    friend bool operator!=( FString const & _Str1, FString const & _Str2 );
    friend bool operator!=( FString const & _Str1, const char * _Str2 );
    friend bool operator!=( const char * _Str1, FString const & _Str2 );

    void Clear();
    void Free();
    void Resize( int _Length );
    void Reserve( int _Length );
    void ReserveInvalidate( int _Length );
    bool IsEmpty() const;
    int Length() const;
    const char * ToConstChar() const;
    char * ToPtr() const;

    void CopyN( const char * _Str, int _Num );

    FString & Concat( char _Val );
    FString & Concat( FString const & _Str );
    FString & Concat( const char * _Str );

    FString & Insert( char _Val, int _Index );
    FString & Insert( FString const & _Str, int _Index );
    FString & Insert( const char * _Str, int _Index );

    //FString & Replace( char _Val, int _Index );
    FString & Replace( FString const & _Str, int _Index );
    FString & Replace( const char * _Str, int _Index );

    FString & Cut( int _Index, int _Count );

    int Contains( char _Ch ) const;

    FString & ToLower();
    FString & ToUpper();

    uint32_t HexToUInt32( int _Len ) const;
    uint64_t HexToUInt64( int _Len ) const;

    uint32_t HexToUInt32() const;
    uint64_t HexToUInt64() const;

    int Icmp( const char * _Str ) const;
    int Icmp( FString const & _Str ) const { return Icmp( _Str.ToConstChar() ); }

    int Cmp( const char * _Str ) const;
    int Cmp( FString const & _Str ) const { return Cmp( _Str.ToConstChar() ); }

    int IcmpN( const char * _Str, int _Num ) const;
    int IcmpN( FString const & _Str, int _Num ) const { return IcmpN( _Str.ToConstChar(), _Num ); }

    int CmpN( const char * _Str, int _Num ) const;
    int CmpN( FString const & _Str, int _Num ) const { return CmpN( _Str.ToConstChar(), _Num ); }

    FString & SkipTrailingZeros();

    FString & UpdateSeparator();
    FString & OptimizePath();
    FString & StripPath();
    int FindPath() const;
    FString & StripExt();
    FString & StripFilename();
    bool CompareExt( const char * _Ext, bool _CaseSensitive ) const;
    FString & UpdateExt( const char * _Extension );
    FString & ReplaceExt( const char * _Extension );
    int FindExt() const;
    int FindExtWithoutDot() const;

    int Hash() const { return FCore::Hash( StringData, StringLength ); }
    int HashCase() const { return FCore::HashCase( StringData, StringLength ); }

    /*

    Static utilites

    */

    static int Icmp( const char * _S1, const char * _S2 );
    static int IcmpN( const char * _S1, const char * _S2, int _Num );
    static int Cmp( const char * _S1, const char * _S2 );
    static int CmpN( const char * _S1, const char * _S2, int _Num );
    static int CmpPath( const char * _S1, const char * _S2 );
    static int CmpPathN( const char * _S1, const char * _S2, int _Num );

    static int Length( const char * _Str );
    static char * ToLower( char * _Str );
    static char * ToUpper( char * _Str );
    static uint32_t HexToUInt32( const char * _Str, int _Len );
    static uint64_t HexToUInt64( const char * _Str, int _Len );
    static void ConcatSafe( char * _Dest, size_t _DestSz, const char * _Src );
    static void CopySafe( char * _Dest, size_t _DestSz, const char * _Src );
    static void CopySafeN( char * _Dest, size_t _DestSz, const char * _Src, size_t _Num );
    static void Concat( char * _Dest, const char * _Src );
    static void Copy( char * _Dest, const char * _Src );

    template< typename Type >
    static FString ToHexString( Type const & _Val, bool _LeadingZeros = false, bool _Prefix = false );

    // The output is always null-terminated and truncated if necessary. The return value is either the number of characters stored or -1 if truncation occurs.
    static int Sprintf( char * _Buffer, int _Size, const char * _Format, ... );
    static int SprintfUnsafe( char * _Buffer, const char * _Format, ... );
    static int vsnprintf( char * _Buffer, int _Size, const char * _Format, va_list _VaList );
    static const char * NullCString();
    static FString const & NullFString();
    static void UpdateSeparator( char * _String );
    static void OptimizePath( char * _Path );
    static int FindPath( const char * _Str );
    static int FindExt( const char * _Str );
    static int FindExtWithoutDot( const char * _Str );
    static int Contains( const char* _String, char _Ch );
    static char * Fmt( const char * _Format, ... );
    static bool IsPathSeparator( const char & _Char );

private:
    void ReallocIfNeed( int _Alloc, bool _CopyOld );

    static const char __NullCString[];
    static const FString __NullFString;

protected:
    int Allocated;
    int StringLength;
    char StaticData[BASE_ALLOC];
    char * StringData;
};

/*

TSprintfBuffer

Usage:

TSprintfBuffer< 128 > buffer;
buffer.Sprintf( "%d %f", 10, 15.1f );

FString s = TSprintfBuffer< 128 >().Sprintf( "%d %f", 10, 15.1f );

*/
template< int Size >
struct TSprintfBuffer {
    char Data[ Size ];

    char * Sprintf( const char * _Format, ... ) {
        static_assert( Size > 0, "Invalid buffer size" );
        va_list VaList;
        va_start( VaList, _Format );
        FString::vsnprintf( Data, Size, _Format, VaList );
        va_end( VaList );
        return Data;
    }
};

AN_FORCEINLINE FString::FString()
    : Allocated(BASE_ALLOC), StringLength(0), StringData( StaticData )
{
    StaticData[0] = 0;
}

AN_FORCEINLINE FString::FString( const char * _Str ) : FString() {
    int tmpLen = Length( _Str );
    ReallocIfNeed( tmpLen + 1, false );
    memcpy( StringData, _Str, tmpLen );
    StringData[tmpLen] = '\0';
    StringLength = static_cast< int >( tmpLen );
}

AN_FORCEINLINE FString::FString( const FString & _Str ) : FString() {
    int tmpLen = _Str.Length();
    ReallocIfNeed( tmpLen + 1, false );
    memcpy( StringData, _Str.StringData, tmpLen );
    StringData[tmpLen] = '\0';
    StringLength = static_cast< int >( tmpLen );
}

AN_FORCEINLINE FString::~FString() {
    if ( StringData != StaticData ) {
        Allocator::Dealloc( StringData );
    }
}

AN_FORCEINLINE const char & FString::operator[]( int _Index ) const {
    AN_ASSERT( ( _Index >= 0 ) && ( _Index <= StringLength ), "FString[]" );
    return StringData[ _Index ];
}

AN_FORCEINLINE char &FString::operator[]( int _Index ) {
    AN_ASSERT( ( _Index >= 0 ) && ( _Index <= StringLength ), "FString[]" );
    return StringData[ _Index ];
}

AN_FORCEINLINE void FString::operator=( FString const & _Str ) {
    int newlength;
    newlength = _Str.Length();
    ReallocIfNeed( newlength+1, false );
    memcpy( StringData, _Str.StringData, newlength );
    StringData[newlength] = '\0';
    StringLength = newlength;
}

AN_FORCEINLINE void FString::operator=( const char * _Str ) {
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
        AN_ASSERT( Length( _Str ) < StringLength, "FString=" );
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

AN_FORCEINLINE void FString::Clear() {
    StringLength = 0;
    StringData[0] = '\0';
}

AN_FORCEINLINE void FString::Free() {
    if ( StringData != StaticData ) {
        Allocator::Dealloc( StringData );
        StringData = StaticData;
        Allocated = BASE_ALLOC;
    }
    StringLength = 0;
    StringData[0] = '\0';
}

AN_FORCEINLINE void FString::CopyN( const char * _Str, int _Num ) {
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
        AN_ASSERT( Length( _Str ) < StringLength, "FString::CopyN" );
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

AN_FORCEINLINE FString operator+( FString const & _Str1, FString const & _Str2 ) {
    FString result( _Str1 );
    result.Concat( _Str2 );
    return result;
}

AN_FORCEINLINE FString operator+( FString const & _Str1, const char * _Str2 ) {
    FString result( _Str1 );
    result.Concat( _Str2 );
    return result;
}

AN_FORCEINLINE FString operator+( const char * _Str1, FString const & _Str2 ) {
    FString result( _Str1 );
    result.Concat( _Str2 );
    return result;
}

AN_FORCEINLINE FString &FString::operator+=( FString const & _Str ) {
    Concat( _Str );
    return *this;
}

AN_FORCEINLINE FString &FString::operator+=( const char * _Str ) {
    Concat( _Str );
    return *this;
}

AN_FORCEINLINE bool operator==( FString const & _Str1, FString const & _Str2 ) {
    return ( !FString::Cmp( _Str1.StringData, _Str2.StringData ) );
}

AN_FORCEINLINE bool operator==( FString const & _Str1, const char * _Str2 ) {
    AN_ASSERT( _Str2, "Strings comparison" );
    return ( !FString::Cmp( _Str1.StringData, _Str2 ) );
}

AN_FORCEINLINE bool operator==( const char * _Str1, FString const & _Str2 ) {
    AN_ASSERT( _Str1, "Strings comparison" );
    return ( !FString::Cmp( _Str1, _Str2.StringData ) );
}

AN_FORCEINLINE bool operator!=( FString const & _Str1, FString const & _Str2 ) {
    return !( _Str1 == _Str2 );
}

AN_FORCEINLINE bool operator!=( FString const & _Str1, const char * _Str2 ) {
    return !( _Str1 == _Str2 );
}

AN_FORCEINLINE bool operator!=( const char * _Str1, FString const & _Str2 ) {
    return !( _Str1 == _Str2 );
}

AN_FORCEINLINE int FString::Icmp( const char * _Str ) const {
    AN_ASSERT( _Str, "FString::Icmp" );
    return FString::Icmp( StringData, _Str );
}

AN_FORCEINLINE int FString::Cmp( const char * _Str ) const {
    AN_ASSERT( _Str, "FString::Cmp" );
    return FString::Cmp( StringData, _Str );
}

AN_FORCEINLINE int FString::IcmpN( const char * _Str, int _Num ) const {
    AN_ASSERT( _Str, "FString::CmpN" );
    return FString::IcmpN( StringData, _Str, _Num );
}

AN_FORCEINLINE int FString::CmpN( const char * _Str, int _Num ) const {
    AN_ASSERT( _Str, "FString::CmpN" );
    return FString::CmpN( StringData, _Str, _Num );
}

AN_FORCEINLINE int FString::Icmp( const char* _S1, const char* _S2 ) {
    char        c1, c2;

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

AN_FORCEINLINE int FString::IcmpN( const char* _S1, const char* _S2, int _Num ) {
    char        c1, c2;

    AN_ASSERT( _Num >= 0, "FString::IcmpN" );

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

AN_FORCEINLINE int FString::CmpPath( const char* _S1, const char* _S2 ) {
    char        c1, c2;

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

AN_FORCEINLINE int FString::CmpPathN( const char* _S1, const char* _S2, int _Num ) {
    char        c1, c2;

    AN_ASSERT( _Num >= 0, "FString::CmpPathN" );

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

AN_FORCEINLINE int FString::Cmp( const char * _S1, const char * _S2 ) {
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

AN_FORCEINLINE int FString::CmpN( const char * _S1, const char * _S2, int _Num ) {
    char        c1, c2;
    
    AN_ASSERT( _Num >= 0, "FString::CmpN" );

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

AN_FORCEINLINE const char * FString::ToConstChar() const {
    return StringData;
}

AN_FORCEINLINE int FString::Length() const {
    return StringLength;
}

AN_FORCEINLINE bool FString::IsEmpty() const {
    return StringLength == 0;
}

AN_FORCEINLINE char * FString::ToPtr() const {
    return StringData;
}

AN_FORCEINLINE FString & FString::Concat( char _Val ) {
    ReallocIfNeed( StringLength+2, true );
    StringData[StringLength] = _Val;
    StringLength++;
    StringData[StringLength] = '\0';
    return *this;
}

AN_FORCEINLINE FString & FString::Concat( FString const & _Str ) {
    int newlength;
    int i;

    newlength = StringLength + _Str.StringLength;
    ReallocIfNeed( newlength+1, true );
    for ( i = 0; i < _Str.StringLength; i++ )
        StringData[ StringLength + i ] = _Str[i];
    StringLength = newlength;
    StringData[StringLength] = '\0';
    return *this;
}

AN_FORCEINLINE FString & FString::Concat( const char * _Str ) {
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
    return *this;
}

AN_FORCEINLINE FString & FString::Insert( char _Val, int _Index ) {
    if ( _Index < 0 ) {
        _Index = 0;
    } else if ( _Index > StringLength ) {
        _Index = StringLength;
    }
    ReallocIfNeed( StringLength + 2, true );
    for ( int i = StringLength ; i >= _Index; i-- ) {
        StringData[i+1] = StringData[i];
    }
    StringData[_Index] = _Val;
    StringLength++;
    return *this;
}

AN_FORCEINLINE FString & FString::Insert( FString const & _Str, int _Index ) {
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
    return *this;
}

AN_FORCEINLINE FString & FString::Insert( const char * _Str, int _Index ) {
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
    return *this;
}

AN_FORCEINLINE void FString::Resize( int _Length ) {
    ReallocIfNeed( _Length+1, true );
    StringLength = _Length;
    StringData[StringLength] = 0;
}

AN_FORCEINLINE void FString::Reserve( int _Capacity ) {
    ReallocIfNeed( _Capacity, true );
}

AN_FORCEINLINE void FString::ReserveInvalidate( int _Capacity ) {
    ReallocIfNeed( _Capacity, false );
}

//AN_FORCEINLINE FString & FString::Replace( char _Val, int _Index ) {
//    if ( _Index < 0 ) _Index = 0;

//    // append to end
//    if ( _Index >= StringLength ){
//        _Index = StringLength;
//        return Insert( _Val, _Index );
//    }

//    // replace
//    StringData[_Index] = _Val;
//    return *this;
//}

AN_FORCEINLINE FString & FString::Replace( FString const & _Str, int _Index ) {
    int newlength = _Index + _Str.StringLength;

    ReallocIfNeed( newlength+1, true );
    for ( int i = 0; i < _Str.StringLength; i++ ) {
        StringData[ _Index + i ] = _Str[i];
    }
    StringLength = newlength;
    StringData[StringLength] = '\0';
    return *this;
}

AN_FORCEINLINE FString & FString::Replace( const char * _Str, int _Index ) {
    int newlength = _Index + Length( _Str );

    ReallocIfNeed( newlength+1, true );
    for ( int i = 0 ; _Str[i] ; i++ ) {
        StringData[ _Index + i ] = _Str[i];
    }
    StringLength = newlength;
    StringData[StringLength] = '\0';
    return *this;
}

AN_FORCEINLINE FString & FString::Cut( int _Index, int _Count ) {
    int i;
    AN_ASSERT( _Count > 0, "FString::Cut" );
    if ( _Index < 0 ) _Index = 0;
    else if ( _Index > StringLength ) _Index = StringLength;
    for ( i = _Index + _Count ; i < StringLength ; i++ ) {
        StringData[ i - _Count ] = StringData[ i ];
    }
    StringLength = i - _Count;
    StringData[ StringLength ] = 0;
    return *this;
}

AN_FORCEINLINE int FString::Contains( char _Ch ) const {
    for ( const char *s=StringData ; *s ; s++ ) {
        if ( *s == _Ch ) {
            return s-StringData;
        }
    }
    return -1;
}

AN_FORCEINLINE FString & FString::SkipTrailingZeros() {
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
    return *this;
}

AN_FORCEINLINE FString & FString::UpdateSeparator() {
    UpdateSeparator( StringData );
    return *this;
}

AN_FORCEINLINE void FString::UpdateSeparator( char * _String ) {
    while ( *_String ) {
        if ( *_String == '\\' ) {
            *_String = '/';
        }
        _String++;
    }
}

// get file name without path
AN_FORCEINLINE FString & FString::StripPath() {
    FString s(*this);
    char    *p = s.StringData + s.StringLength;
    while ( --p >= s.StringData && !IsPathSeparator( *p ) ) {
        ;
    }
    p++;
    *this = p;
    return *this;
}

AN_FORCEINLINE int FString::FindPath() const {
    char * p = StringData + StringLength;
    while ( --p >= StringData ) {
        if ( IsPathSeparator( *p ) ) {
            return p - StringData + 1;
        }
    }
    return 0;
}

AN_FORCEINLINE int FString::FindPath( const char * _Str ) {
    int len = Length( _Str );
    const char * p = _Str + len;
    while ( --p >= _Str ) {
        if ( IsPathSeparator( *p ) ) {
            return p - _Str + 1;
        }
    }
    return 0;
}

// get file name without extension
AN_FORCEINLINE FString & FString::StripExt() {
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
    return *this;
}

// get file path without file name
AN_FORCEINLINE FString & FString::StripFilename() {
    char * p = StringData + StringLength;
    while ( --p > StringData && !IsPathSeparator( *p ) ) {
        ;
    }
    *p = '\0';
    StringLength = p-StringData;
    return *this;
}

AN_FORCEINLINE bool FString::CompareExt( const char * _Ext, bool _CaseSensitive ) const {
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

// add extension to file name
AN_FORCEINLINE FString & FString::UpdateExt( const char * _Extension ) {
    char * p = StringData + StringLength;
    while ( --p >= StringData && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return *this;
        }
    }
    (*this) += _Extension;
    return *this;
}

AN_FORCEINLINE FString & FString::ReplaceExt( const char * _Extension ) {
    StripExt();
    (*this) += _Extension;
    return *this;
}

AN_FORCEINLINE int FString::FindExt() const {
    char * p = StringData + StringLength;
    while ( --p >= StringData && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - StringData;
        }
    }
    return StringLength;
}

AN_FORCEINLINE int FString::FindExtWithoutDot() const {
    char * p = StringData + StringLength;
    while ( --p >= StringData && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - StringData + 1;
        }
    }
    return StringLength;
}

AN_FORCEINLINE int FString::FindExt( const char * _Str ) {
    int len = Length(_Str);
    const char * p = _Str + len;
    while ( --p >= _Str && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - _Str;
        }
    }
    return len;
}

AN_FORCEINLINE int FString::FindExtWithoutDot( const char * _Str ) {
    int len = Length(_Str);
    const char * p = _Str + len;
    while ( --p >= _Str && !IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - _Str + 1;
        }
    }
    return len;
}

AN_FORCEINLINE int FString::Contains( const char * _String, char _Ch ) {
    for ( const char *s=_String ; *s ; s++ ) {
        if ( *s == _Ch ) {
            return s-_String;
        }
    }
    return -1;
}

AN_FORCEINLINE int FString::Length( const char * _Str ) {
    const char *p = _Str;
    while ( *p ) {
        p++;
    }
    return p-_Str;

    //return strlen( s );
}

// convert string case to lower
AN_FORCEINLINE FString & FString::ToLower() {
    char * s = StringData;
    while ( *s ) {
        *s = ::tolower( *s );
        s++;
    }
    return *this;
}

// convert string case to upper
AN_FORCEINLINE FString & FString::ToUpper() {
    char * s = StringData;
    while ( *s ) {
        *s = ::toupper( *s );
        s++;
    }
    return *this;
}

// convert string case to lower
AN_FORCEINLINE char * FString::ToLower( char * _Str ) {
    char * str = _Str;
    while ( *_Str ) {
        *_Str = ::tolower( *_Str );
        _Str++;
    }
    return str;
}

// convert string case to upper
AN_FORCEINLINE char * FString::ToUpper( char * _Str ) {
    char * str = _Str;
    while ( *_Str ) {
        *_Str = ::toupper( *_Str );
        _Str++;
    }
    return str;
}

AN_FORCEINLINE uint32_t FString::HexToUInt32( int _Len ) const {
    AN_ASSERT( _Len <= StringLength, "FString::HexToUInt32" );
    return HexToUInt32( StringData, _Len );
}

AN_FORCEINLINE uint64_t FString::HexToUInt64( int _Len ) const {
    AN_ASSERT( _Len <= StringLength, "FString::HexToUInt64" );
    return HexToUInt64( StringData, _Len );
}

AN_FORCEINLINE uint32_t FString::HexToUInt32() const {
    return HexToUInt32( StringData, FMath_Min( StringLength, 8 ) );
}

AN_FORCEINLINE uint64_t FString::HexToUInt64() const {
    return HexToUInt64( StringData, FMath_Min( StringLength, 16 ) );
}

AN_FORCEINLINE uint32_t FString::HexToUInt32( const char * _Str, int _Len ) {
    uint32_t value = 0;

    for ( int i = FMath_Max( 0, _Len - 8 ) ; i < _Len ; i++ ) {
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

AN_FORCEINLINE uint64_t FString::HexToUInt64( const char * _Str, int _Len ) {
    uint64_t value = 0;

    for ( int i = FMath_Max( 0, _Len - 16 ) ; i < _Len ; i++ ) {
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

AN_FORCEINLINE void FString::ConcatSafe( char * _Dest, size_t _DestSz, const char * _Src ) {
#ifdef AN_COMPILER_MSVC
    strcat_s( _Dest, _DestSz, _Src );
#else
    size_t destLength = Length( _Dest );
    if ( destLength >= _DestSz ) {
        return;
    }
    CopySafe( _Dest + destLength, _DestSz - destLength, _Src );
#endif
}

AN_FORCEINLINE void FString::CopySafe( char * _Dest, size_t _DestSz, const char * _Src ) {
#ifdef AN_COMPILER_MSVC
    strcpy_s( _Dest, _DestSz, _Src );
#else
    if ( _DestSz > 0 ) {
        while ( *_Src && --_DestSz != 0 ) {
            *_Dest++ = *_Src++;
        }
        *_Dest = 0;
    }
#endif
}

AN_FORCEINLINE void FString::CopySafeN( char * _Dest, size_t _DestSz, const char * _Src, size_t _Num ) {
#ifdef AN_COMPILER_MSVC
    strncpy_s( _Dest, _DestSz, _Src, _Num );
#else
    if ( _DestSz > 0 && _Num > 0 ) {
        while ( *_Src && --_DestSz != 0 && --_Num != static_cast< size_t >( -1 ) ) {
            *_Dest++ = *_Src++;
        }
        *_Dest = 0;
    }
#endif
}

AN_FORCEINLINE void FString::Concat( char * _Dest, const char * _Src ) {
    strcat( _Dest, _Src );
}

AN_FORCEINLINE void FString::Copy( char * _Dest, const char * _Src ) {
    strcpy( _Dest, _Src );
}

AN_FORCEINLINE bool FString::IsPathSeparator( const char & _Char ) {
#ifdef AN_OS_WIN32
    return _Char == '/' || _Char == '\\';
#else
    return _Char == '/';
#endif
}

AN_FORCEINLINE const char * FString::NullCString() {
    return __NullCString;
}

AN_FORCEINLINE FString const & FString::NullFString() {
    return __NullFString;
}

#define AN_INT64_HIGH_INT( i64 )   int32_t( uint64_t(i64) >> 32 )
#define AN_INT64_LOW_INT( i64 )    int32_t( uint64_t(i64) & 0xFFFFFFFF )

template< typename Type >
FString FString::ToHexString( Type const & _Val, bool _LeadingZeros, bool _Prefix ) {
    TSprintfBuffer< 32 > value;
    TSprintfBuffer< 32 > format;
    const char * PrefixStr = _Prefix ? "0x" : "";

    static_assert( sizeof( Type ) == 1 || sizeof( Type ) == 2 || sizeof( Type ) == 4 || sizeof( Type ) == 8, "ToHexString" );

    switch( sizeof( Type ) ) {
    case 1:
        format.Sprintf( _LeadingZeros ? "%s%%02x" : "%s%%x", PrefixStr );
        return value.Sprintf( format.Data, *reinterpret_cast< const uint8_t * > ( &_Val ) );
    case 2:
        format.Sprintf( _LeadingZeros ? "%s%%04x" : "%s%%x", PrefixStr );
        return value.Sprintf( format.Data, *reinterpret_cast< const uint16_t * > ( &_Val ) );
    case 4:
        format.Sprintf( _LeadingZeros ? "%s%%08x" : "%s%%x", PrefixStr );
        return value.Sprintf( format.Data, *reinterpret_cast< const uint32_t * > ( &_Val ) );
    case 8: {
            uint64_t Temp = *reinterpret_cast< const uint64_t * > ( &_Val );
            if ( _LeadingZeros ) {
                return value.Sprintf( "%s%08x%08x", PrefixStr, AN_INT64_HIGH_INT( Temp ), AN_INT64_LOW_INT( Temp ) );
            } else {
                return value.Sprintf( "%s%I64x", PrefixStr, Temp );
            }
        }
    }
    return "";
}
