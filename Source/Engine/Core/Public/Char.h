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

#include "String.h"
#include "IO.h"

class Char final {
public:
    char Value;

    Char() = default;
    constexpr Char( const char & _Value ) : Value( _Value ) {}

    Char * ToPtr() {
        return this;
    }

    const Char * ToPtr() const {
        return this;
    }

    operator char() const {
        return Value;
    }

    Char & operator=( const char & _Other ) {
        Value = _Other;
        return *this;
    }

    template< typename RightType > bool operator==( const RightType & _Right ) { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) { return Value >= _Right; }

    Char ToLower() const {
        return ::tolower( Value );
    }

    Char ToUpper() const {
        return ::toupper( Value );
    }

    // String conversions
    AString ToString() const {
        TSprintfBuffer< 4 > buffer;
        return buffer.Sprintf( "%c", Value );
    }

    const char * CStr() const {
        return AString::Fmt( "%c", Value );
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteInt8( Value );
    }

    void Read( IStreamBase & _Stream ) {
        Value = _Stream.ReadInt8();
    }

    // Static methods
    static constexpr Char MinValue() { return TStdNumericLimits< char >::min(); }
    static constexpr Char MaxValue() { return TStdNumericLimits< char >::max(); }
};

#if 0
class Byte final {
public:
    unsigned char Value;

    Byte() = default;
    constexpr Byte( const byte & _Value ) : Value( _Value ) {}

    Byte * ToPtr() {
        return this;
    }

    const Byte * ToPtr() const {
        return this;
    }

    operator unsigned char() const {
        return Value;
    }

    Byte & operator=( const unsigned char & _Other ) {
        Value = _Other;
        return *this;
    }

    // String conversions
    AString ToString() const {
        TSprintfBuffer< 4 > buffer;
        return buffer.Sprintf( "%d", Value );
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return Math::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        _Stream.Write( &Value, 1 );
    }

    void Read( IStreamBase & _Stream ) {
        _Stream.Read( &Value, 1 );
    }

    // Static methods
    static constexpr Char MinValue() { return TStdNumericLimits< unsigned char >::min(); }
    static constexpr Char MaxValue() { return TStdNumericLimits< unsigned char >::max(); }
};
#endif
