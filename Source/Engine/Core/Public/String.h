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
#include "BinaryStream.h"

/**

AString

*/
class ANGIE_API AString final {
    using Allocator = AZoneAllocator;

public:
    static const int GRANULARITY = 32;
    static const int BASE_CAPACITY = 32;

    AString();
    AString( const char * _Str );
    AString( AString const & _Str );
    AString( const char * _Begin, const char * _End );
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

    /** Clear string but not free a memory */
    void Clear();

    /** Clear string and free a memory */
    void Free();

    /** Set a new length for the string */
    void Resize( int _Length );

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

    /** Build from c-string */
    void FromCStr( const char * _Str, int _Num );

    /** Append the string */
    void Concat( AString const & _Str );
    void ConcatN( AString const & _Str, int _Num );

    /** Append the string */
    void Concat( const char * _Str );
    void ConcatN( const char * _Str, int _Num );

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

    /** Find the substring. Return substring position in string or -1. */
    int SubstringIcmp( const char * _Substring ) const;

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

    /** Path utility.
    Fix path string insitu: replace separator \\ to /, skip series of /,
    skip redunant sequinces of dir/../dir2 -> dir. */
    void FixPath();

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
    int Hash() const { return Core::Hash( Data, Size ); }

    /** Get string hash case insensitive */
    int HashCase() const { return Core::HashCase( Data, Size ); }

    void FromFile( IBinaryStream & _Stream );

    void Write( IBinaryStream & _Stream ) const;

    void Read( IBinaryStream & _Stream );

    static const char * NullCString() { return ""; }

    static AString const & NullString() { return NullStr; }

private:
    void GrowCapacity( int _Capacity, bool _CopyOld );

protected:
    char Base[BASE_CAPACITY];
    char * Data;
    int Capacity;
    int Size;

private:
    static const AString NullStr;
};

AN_FORCEINLINE AString::AString()
    : Data( Base )
    , Capacity( BASE_CAPACITY )
    , Size( 0 )
{
    Base[0] = 0;
}

AN_FORCEINLINE AString::AString( const char * _Str )
    : AString()
{
    if ( _Str ) {
        const int newLen = Core::Strlen( _Str );
        GrowCapacity( newLen + 1, false );
        Core::Memcpy( Data, _Str, newLen + 1 );
        Size = newLen;
    }
}

AN_FORCEINLINE AString::AString( AString const & _Str )
    : AString()
{
    const int newLen = _Str.Length();
    GrowCapacity( newLen + 1, false );
    Core::Memcpy( Data, _Str.Data, newLen + 1 );
    Size = newLen;
}

AN_FORCEINLINE AString::AString( const char * _Begin, const char * _End )
    : AString()
{
    Resize( _End - _Begin );
    Core::Memcpy( Data, _Begin, Size );
}

AN_FORCEINLINE AString::~AString() {
    if ( Data != Base ) {
        Allocator::Inst().Free( Data );
    }
}

AN_FORCEINLINE const char & AString::operator[]( const int _Index ) const {
    AN_ASSERT_( ( _Index >= 0 ) && ( _Index <= Size ), "AString[]" );
    return Data[ _Index ];
}

AN_FORCEINLINE char &AString::operator[]( const int _Index ) {
    AN_ASSERT_( ( _Index >= 0 ) && ( _Index <= Size ), "AString[]" );
    return Data[ _Index ];
}

AN_FORCEINLINE void AString::operator=( AString const & _Str ) {
    const int newLen = _Str.Length();
    GrowCapacity( newLen+1, false );
    Core::Memcpy( Data, _Str.Data, newLen + 1 );
    Size = newLen;
}

AN_FORCEINLINE void AString::operator=( AStdString const & _Str ) {
    const int newLen = _Str.length();
    GrowCapacity( newLen+1, false );
    Core::Memcpy( Data, _Str.data(), newLen + 1 );
    Size = newLen;
}

AN_FORCEINLINE void AString::Clear() {
    Size = 0;
    Data[0] = '\0';
}

AN_FORCEINLINE void AString::Free() {
    if ( Data != Base ) {
        Allocator::Inst().Free( Data );
        Data = Base;
        Capacity = BASE_CAPACITY;
    }
    Size = 0;
    Data[0] = '\0';
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
    return ( !Core::Strcmp( _Str1.Data, _Str2.Data ) );
}

AN_FORCEINLINE bool operator==( AString const & _Str1, const char * _Str2 ) {
    return ( !Core::Strcmp( _Str1.Data, _Str2 ) );
}

AN_FORCEINLINE bool operator==( const char * _Str1, AString const & _Str2 ) {
    return ( !Core::Strcmp( _Str1, _Str2.Data ) );
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
    return Core::Stricmp( Data, _Str );
}

AN_FORCEINLINE int AString::Cmp( const char * _Str ) const {
    return Core::Strcmp( Data, _Str );
}

AN_FORCEINLINE int AString::IcmpN( const char * _Str, int _Num ) const {
    return Core::StricmpN( Data, _Str, _Num );
}

AN_FORCEINLINE int AString::CmpN( const char * _Str, int _Num ) const {
    return Core::StrcmpN( Data, _Str, _Num );
}

AN_FORCEINLINE const char * AString::CStr() const {
    return Data;
}

AN_FORCEINLINE const char * AString::Begin() const {
    return Data;
}
AN_FORCEINLINE const char * AString::End() const {
    return Data + Size;
}

AN_FORCEINLINE int AString::Length() const {
    return Size;
}

AN_FORCEINLINE bool AString::IsEmpty() const {
    return Size == 0;
}

AN_FORCEINLINE char * AString::ToPtr() const {
    return Data;
}

AN_FORCEINLINE void AString::Resize( int _Length ) {
    GrowCapacity( _Length+1, true );
    if ( _Length > Size ) {
        Core::Memset( &Data[Size], ' ', _Length - Size );
    }
    Size = _Length;
    Data[Size] = 0;
}

AN_FORCEINLINE void AString::FixSeparator() {
    Core::FixSeparator( Data );
}

AN_FORCEINLINE void AString::ReplaceExt( const char * _Extension ) {
    StripExt();
    (*this) += _Extension;
}

AN_FORCEINLINE uint32_t AString::HexToUInt32( int _Len ) const {
    AN_ASSERT_( _Len <= Size, "AString::HexToUInt32" );
    return Core::HexToUInt32( Data, _Len );
}

AN_FORCEINLINE uint64_t AString::HexToUInt64( int _Len ) const {
    AN_ASSERT_( _Len <= Size, "AString::HexToUInt64" );
    return Core::HexToUInt64( Data, _Len );
}

AN_FORCEINLINE uint32_t AString::HexToUInt32() const {
    return Core::HexToUInt32( Data, StdMin( Size, 8 ) );
}

AN_FORCEINLINE uint64_t AString::HexToUInt64() const {
    return Core::HexToUInt64( Data, StdMin( Size, 16 ) );
}

AN_FORCEINLINE void AString::FromFile( IBinaryStream & _Stream ) {
    _Stream.SeekEnd( 0 );
    long fileSz = _Stream.Tell();
    _Stream.SeekSet( 0 );
    GrowCapacity( fileSz + 1, false );
    _Stream.ReadBuffer( Data, fileSz );
    Data[fileSz] = 0;
    Size = fileSz;
}

AN_FORCEINLINE void AString::Write( IBinaryStream & _Stream ) const {
    _Stream.WriteUInt32( Size );
    _Stream.WriteBuffer( Data, Size );
}

AN_FORCEINLINE void AString::Read( IBinaryStream & _Stream ) {
    int len = _Stream.ReadUInt32();
    GrowCapacity( len + 1, false );
    _Stream.ReadBuffer( Data, len );
    Data[len] = 0;
    Size = len;
}


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
        AN_ASSERT( _Format );
        va_list VaList;
        va_start( VaList, _Format );
        Core::VSprintf( Data, Size, _Format, VaList );
        va_end( VaList );
        return Data;
    }
};
