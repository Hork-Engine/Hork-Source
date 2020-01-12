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

#pragma once

#include "Std.h"
#include "HashFunc.h"
#include <stdarg.h>

/**

AString

*/
class ANGIE_API AString final {
    using Allocator = AZoneAllocator;

public:
    static const int GRANULARITY = 32;
    static const int BASE_ALLOC = 32;

    AString();
    AString( const char * _Str );
    AString( AString const & _Str );
    ~AString();

    const char & operator[]( const int _Index ) const;
    char & operator[]( const int _Index );

    void operator=( AString const & _Str );
    void operator=( const char * _Str );
    void operator=( AStdString const & _Str );

    friend AString operator+( AString const & _Str1, AString const & _Str2 );
    friend AString operator+( AString const & _Str1, const char * _Str2 );
    friend AString operator+( const char * _Str1, AString const & _Str2 );
    friend AString operator+( AString const & _Str, char _Char );
    friend AString operator+( char _Char, AString const & _Str );

    AString & operator+=( AString const & _Str );
    AString & operator+=( const char * _Str );
    AString & operator+=( char _Char );

    friend bool operator==( AString const & _Str1, AString const & _Str2 );
    friend bool operator==( AString const & _Str1, const char * _Str2 );
    friend bool operator==( const char * _Str1, AString const & _Str2 );
    friend bool operator!=( AString const & _Str1, AString const & _Str2 );
    friend bool operator!=( AString const & _Str1, const char * _Str2 );
    friend bool operator!=( const char * _Str1, AString const & _Str2 );

    /** Clear the string but not free the memory */
    void Clear();

    /** Clear the string and free memory */
    void Free();

    /** Set a new length for the string */
    void Resize( int _Length );

    /** Set a new length for the string and discard previous content */
    void ResizeInvalidate( int _Length );

    /** Reserve memory for the string */
    void Reserve( int _Length );

    /** Reserve memory and discard content */
    void ReserveInvalidate( int _Length );

    /** Return is string empty or not */
    bool IsEmpty() const;

    /** Return string length */
    int Length() const;

    /** Get const char pointer of the string */
    const char * CStr() const;

    /** Pointer to the beggining of the string */
    const char * Begin() const;

    /** Pointer to the ending of the string */
    const char * End() const;

    /** Get raw pointer */
    char * ToPtr() const;

    /** Copy substring */
    void CopyN( const char * _Str, int _Num );

    /** Append the string */
    void Concat( AString const & _Str );
    //void ConcatN( AString const & _Str, int _Num ); todo

    /** Append the string */
    void Concat( const char * _Str );
    //void ConcatN( const char * _Str, int _Num ); todo

    /** Append the character */
    void Concat( char _Char );

    /** Insert the string at specified index */
    void Insert( AString const & _Str, int _Index );
    //void InsertN( AString const & _Str, int _Num, int _Index ); todo

    /** Insert the string at specified index */
    void Insert( const char * _Str, int _Index );
    //void InsertN( const char * _Str, int _Num, int _Index ); todo

    /** Insert the character at specified index */
    void Insert( char _Char, int _Index );

    /** Replace the string at specified index */
    void Replace( AString const & _Str, int _Index );

    /** Replace the string at specified index */
    void Replace( const char * _Str, int _Index );

    /** Replace all substrings with a new string */
    void Replace( const char * _Substring, const char * _NewStr );

    /** Cut substring at specified index */
    void Cut( int _Index, int _Count );

    /** Find the character. Return character position in string or -1. */
    int Contains( char _Ch ) const;

    /** Find the substring. Return substring position in string or -1. */
    int Substring( const char * _Substring ) const;

    /** Convert to lower case */
    void ToLower();

    /** Convert to upper case */
    void ToUpper();

    uint32_t HexToUInt32( int _Len ) const;
    uint64_t HexToUInt64( int _Len ) const;

    uint32_t HexToUInt32() const;
    uint64_t HexToUInt64() const;

    /** Compare the strings (case insensitive) */
    int Icmp( const char * _Str ) const;

    /** Compare the strings (case insensitive) */
    int Icmp( AString const & _Str ) const { return Icmp( _Str.CStr() ); }

    /** Compare the strings (case sensitive) */
    int Cmp( const char * _Str ) const;

    /** Compare the strings (case sensitive) */
    int Cmp( AString const & _Str ) const { return Cmp( _Str.CStr() ); }

    /** Compare the strings (case insensitive) */
    int IcmpN( const char * _Str, int _Num ) const;

    /** Compare the strings (case insensitive) */
    int IcmpN( AString const & _Str, int _Num ) const { return IcmpN( _Str.CStr(), _Num ); }

    /** Compare the strings (case sensitive) */
    int CmpN( const char * _Str, int _Num ) const;

    /** Compare the strings (case sensitive) */
    int CmpN( AString const & _Str, int _Num ) const { return CmpN( _Str.CStr(), _Num ); }

    /** Skip trailing zeros for the numbers. */
    void SkipTrailingZeros();

    /** Path utility. Fix OS-specific separator. */
    void FixSeparator();

    /** Path utility. Optimize path string. */
    void OptimizePath();

    /** Path utility. Cut the path. */
    void StripPath();

    /** Path utility. Return index of the path end. */
    int FindPath() const;

    /** Path utility. Cut the extension. */
    void StripExt();

    /** Path utility. Cut the file name. */
    void StripFilename();

    /** Path utility. Check file extension. */
    bool CompareExt( const char * _Ext, bool _CaseSensitive ) const;

    /** Path utility. Set file extension if not exists. */
    void UpdateExt( const char * _Extension );

    /** Path utility. Add or replace existing file extension. */
    void ReplaceExt( const char * _Extension );

    /** Path utility. Return index where the extension begins. */
    int FindExt() const;

    /** Path utility. Return index where the extension begins after dot. */
    int FindExtWithoutDot() const;

    /** Get string hash */
    int Hash() const { return Core::Hash( StringData, StringLength ); }

    /** Get string hash case insensitive */
    int HashCase() const { return Core::HashCase( StringData, StringLength ); }

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
    static AString ToHexString( Type const & _Val, bool _LeadingZeros = false, bool _Prefix = false );

    // The output is always null-terminated and truncated if necessary. The return value is either the number of characters stored or -1 if truncation occurs.
    static int Sprintf( char * _Buffer, int _Size, const char * _Format, ... );
    static int SprintfUnsafe( char * _Buffer, const char * _Format, ... );
    static int vsnprintf( char * _Buffer, int _Size, const char * _Format, va_list _VaList );
    static const char * NullCString();
    static AString const & NullFString();
    static void FixSeparator( char * _String );
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
    static const AString __NullFString;

protected:
    char * StringData;
    int Allocated;
    int StringLength;
    char StaticData[BASE_ALLOC];
};

/**

TSprintfBuffer

Usage:

TSprintfBuffer< 128 > buffer;
buffer.Sprintf( "%d %f", 10, 15.1f );

AString s = TSprintfBuffer< 128 >().Sprintf( "%d %f", 10, 15.1f );

*/
template< int Size >
struct TSprintfBuffer {
    char Data[ Size ];

    char * Sprintf( const char * _Format, ... ) {
        static_assert( Size > 0, "Invalid buffer size" );
        va_list VaList;
        va_start( VaList, _Format );
        AString::vsnprintf( Data, Size, _Format, VaList );
        va_end( VaList );
        return Data;
    }
};

AN_FORCEINLINE AString::AString()
    : StringData( StaticData ), Allocated(BASE_ALLOC), StringLength(0)
{
    StaticData[0] = 0;
}

AN_FORCEINLINE AString::AString( const char * _Str ) : AString() {
    int tmpLen = Length( _Str );
    ReallocIfNeed( tmpLen + 1, false );
    memcpy( StringData, _Str, tmpLen );
    StringData[tmpLen] = '\0';
    StringLength = static_cast< int >( tmpLen );
}

AN_FORCEINLINE AString::AString( AString const & _Str ) : AString() {
    int tmpLen = _Str.Length();
    ReallocIfNeed( tmpLen + 1, false );
    memcpy( StringData, _Str.StringData, tmpLen );
    StringData[tmpLen] = '\0';
    StringLength = static_cast< int >( tmpLen );
}

AN_FORCEINLINE AString::~AString() {
    if ( StringData != StaticData ) {
        Allocator::Inst().Dealloc( StringData );
    }
}

AN_FORCEINLINE const char & AString::operator[]( const int _Index ) const {
    AN_ASSERT_( ( _Index >= 0 ) && ( _Index <= StringLength ), "AString[]" );
    return StringData[ _Index ];
}

AN_FORCEINLINE char &AString::operator[]( const int _Index ) {
    AN_ASSERT_( ( _Index >= 0 ) && ( _Index <= StringLength ), "AString[]" );
    return StringData[ _Index ];
}

AN_FORCEINLINE void AString::operator=( AString const & _Str ) {
    int newlength;
    newlength = _Str.Length();
    ReallocIfNeed( newlength+1, false );
    memcpy( StringData, _Str.StringData, newlength );
    StringData[newlength] = '\0';
    StringLength = newlength;
}

AN_FORCEINLINE void AString::operator=( AStdString const & _Str ) {
    int newlength;
    newlength = _Str.length();
    ReallocIfNeed( newlength+1, false );
    memcpy( StringData, _Str.data(), newlength );
    StringData[newlength] = '\0';
    StringLength = newlength;
}

AN_FORCEINLINE void AString::Clear() {
    StringLength = 0;
    StringData[0] = '\0';
}

AN_FORCEINLINE void AString::Free() {
    if ( StringData != StaticData ) {
        Allocator::Inst().Dealloc( StringData );
        StringData = StaticData;
        Allocated = BASE_ALLOC;
    }
    StringLength = 0;
    StringData[0] = '\0';
}

AN_FORCEINLINE AString operator+( AString const & _Str1, AString const & _Str2 ) {
    AString result( _Str1 );
    result.Concat( _Str2 );
    return result;
}

AN_FORCEINLINE AString operator+( AString const & _Str1, const char * _Str2 ) {
    AString result( _Str1 );
    result.Concat( _Str2 );
    return result;
}

AN_FORCEINLINE AString operator+( const char * _Str1, AString const & _Str2 ) {
    AString result( _Str1 );
    result.Concat( _Str2 );
    return result;
}

AN_FORCEINLINE AString operator+( AString const & _Str, char _Char ) {
    AString result( _Str );
    result.Concat( _Char );
    return result;
}

AN_FORCEINLINE AString operator+( char _Char, AString const & _Str ) {
    AString result( _Str );
    result.Concat( _Char );
    return result;
}

AN_FORCEINLINE AString & AString::operator+=( AString const & _Str ) {
    Concat( _Str );
    return *this;
}

AN_FORCEINLINE AString & AString::operator+=( const char * _Str ) {
    Concat( _Str );
    return *this;
}

AN_FORCEINLINE AString & AString::operator+=( char _Char ) {
    Concat( _Char );
    return *this;
}

AN_FORCEINLINE bool operator==( AString const & _Str1, AString const & _Str2 ) {
    return ( !AString::Cmp( _Str1.StringData, _Str2.StringData ) );
}

AN_FORCEINLINE bool operator==( AString const & _Str1, const char * _Str2 ) {
    AN_ASSERT_( _Str2, "Strings comparison" );
    return ( !AString::Cmp( _Str1.StringData, _Str2 ) );
}

AN_FORCEINLINE bool operator==( const char * _Str1, AString const & _Str2 ) {
    AN_ASSERT_( _Str1, "Strings comparison" );
    return ( !AString::Cmp( _Str1, _Str2.StringData ) );
}

AN_FORCEINLINE bool operator!=( AString const & _Str1, AString const & _Str2 ) {
    return !( _Str1 == _Str2 );
}

AN_FORCEINLINE bool operator!=( AString const & _Str1, const char * _Str2 ) {
    return !( _Str1 == _Str2 );
}

AN_FORCEINLINE bool operator!=( const char * _Str1, AString const & _Str2 ) {
    return !( _Str1 == _Str2 );
}

AN_FORCEINLINE int AString::Icmp( const char * _Str ) const {
    AN_ASSERT_( _Str, "AString::Icmp" );
    return AString::Icmp( StringData, _Str );
}

AN_FORCEINLINE int AString::Cmp( const char * _Str ) const {
    AN_ASSERT_( _Str, "AString::Cmp" );
    return AString::Cmp( StringData, _Str );
}

AN_FORCEINLINE int AString::IcmpN( const char * _Str, int _Num ) const {
    AN_ASSERT_( _Str, "AString::CmpN" );
    return AString::IcmpN( StringData, _Str, _Num );
}

AN_FORCEINLINE int AString::CmpN( const char * _Str, int _Num ) const {
    AN_ASSERT_( _Str, "AString::CmpN" );
    return AString::CmpN( StringData, _Str, _Num );
}

AN_FORCEINLINE const char * AString::CStr() const {
    return StringData;
}

AN_FORCEINLINE const char * AString::Begin() const {
    return StringData;
}
AN_FORCEINLINE const char * AString::End() const {
    return StringData + StringLength;
}

AN_FORCEINLINE int AString::Length() const {
    return StringLength;
}

AN_FORCEINLINE bool AString::IsEmpty() const {
    return StringLength == 0;
}

AN_FORCEINLINE char * AString::ToPtr() const {
    return StringData;
}

AN_FORCEINLINE void AString::Resize( int _Length ) {
    ReallocIfNeed( _Length+1, true );
    StringLength = _Length;
    StringData[StringLength] = 0;
}

AN_FORCEINLINE void AString::ResizeInvalidate( int _Length ) {
    ReallocIfNeed( _Length+1, false );
    StringLength = _Length;
    StringData[StringLength] = 0;
}

AN_FORCEINLINE void AString::Reserve( int _Capacity ) {
    ReallocIfNeed( _Capacity, true );
}

AN_FORCEINLINE void AString::ReserveInvalidate( int _Capacity ) {
    ReallocIfNeed( _Capacity, false );
}

AN_FORCEINLINE void AString::FixSeparator() {
    FixSeparator( StringData );
}

AN_FORCEINLINE void AString::ReplaceExt( const char * _Extension ) {
    StripExt();
    (*this) += _Extension;
}

AN_FORCEINLINE uint32_t AString::HexToUInt32( int _Len ) const {
    AN_ASSERT_( _Len <= StringLength, "AString::HexToUInt32" );
    return HexToUInt32( StringData, _Len );
}

AN_FORCEINLINE uint64_t AString::HexToUInt64( int _Len ) const {
    AN_ASSERT_( _Len <= StringLength, "AString::HexToUInt64" );
    return HexToUInt64( StringData, _Len );
}

AN_FORCEINLINE uint32_t AString::HexToUInt32() const {
    return HexToUInt32( StringData, StdMin( StringLength, 8 ) );
}

AN_FORCEINLINE uint64_t AString::HexToUInt64() const {
    return HexToUInt64( StringData, StdMin( StringLength, 16 ) );
}

AN_FORCEINLINE void AString::ConcatSafe( char * _Dest, size_t _DestSz, const char * _Src ) {
    if ( !_Src ) {
        return;
    }
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

AN_FORCEINLINE void AString::CopySafe( char * _Dest, size_t _DestSz, const char * _Src ) {
    if ( !_Src ) {
        _Src = "null";
    }
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

AN_FORCEINLINE void AString::CopySafeN( char * _Dest, size_t _DestSz, const char * _Src, size_t _Num ) {
    if ( !_Src ) {
        _Src = "null";
    }
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

AN_FORCEINLINE void AString::Concat( char * _Dest, const char * _Src ) {
    if ( !_Src ) {
        return;
    }
    strcat( _Dest, _Src );
}

AN_FORCEINLINE void AString::Copy( char * _Dest, const char * _Src ) {
    if ( !_Src ) {
        _Src = "null";
    }
    strcpy( _Dest, _Src );
}

AN_FORCEINLINE bool AString::IsPathSeparator( const char & _Char ) {
#ifdef AN_OS_WIN32
    return _Char == '/' || _Char == '\\';
#else
    return _Char == '/';
#endif
}

AN_FORCEINLINE const char * AString::NullCString() {
    return __NullCString;
}

AN_FORCEINLINE AString const & AString::NullFString() {
    return __NullFString;
}

#define AN_INT64_HIGH_INT( i64 )   int32_t( uint64_t(i64) >> 32 )
#define AN_INT64_LOW_INT( i64 )    int32_t( uint64_t(i64) & 0xFFFFFFFF )

template< typename Type >
AString AString::ToHexString( Type const & _Val, bool _LeadingZeros, bool _Prefix ) {
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
