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

#include "String.h"
#include "IO.h"

class Bool final {
public:
    bool Value;

    Bool() = default;
    //Bool( const bool & _Value ) : Value( _Value ) {}
    constexpr Bool( const bool & _Value ) : Value( _Value ) {}

    Bool * ToPtr() {
        return this;
    }

    const Bool * ToPtr() const {
        return this;
    }

    operator bool() const {
        return Value;
    }

    Bool & operator=( const bool & _Other ) {
        Value = _Other;
        return *this;
    }

    Bool operator==( const Bool & _Other ) const { return Value == _Other.Value; }
    Bool operator!=( const Bool & _Other ) const { return Value != _Other.Value; }

    Bool Any() const { return Value; }
    Bool All() const { return Value; }

    // String conversions
    AString ToString() const {
        static constexpr const char * s[2] = { "false", "true" };
        return s[ Value ];
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    Bool & FromString( const AString & _String ) {
        return FromString( _String.CStr() );
    }

    Bool & FromString( const char * _String );


    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteBool( Value );
    }

    void Read( IStreamBase & _Stream ) {
        Value = _Stream.ReadBool();
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr Bool MinValue() { return TStdNumericLimits< bool >::min(); }
    static constexpr Bool MaxValue() { return TStdNumericLimits< bool >::max(); }
};

class Bool2 final {
public:
    Bool X;
    Bool Y;

    Bool2() = default;
    explicit constexpr Bool2( const bool & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Bool2( const bool & _X, const bool & _Y ) : X( _X ), Y( _Y ) {}

    Bool * ToPtr() {
        return &X;
    }

    const Bool * ToPtr() const {
        return &X;
    }

    Bool2 & operator=( const Bool2 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        return *this;
    }

    Bool & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }
    const Bool & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    Bool operator==( const Bool2 & _Other ) const { return X == _Other.X && Y == _Other.Y; }
    Bool operator!=( const Bool2 & _Other ) const { return X != _Other.X || Y != _Other.Y; }

    //Bool Any() const { return ( X || Y ); }
    //Bool All() const { return ( X && Y ); }

    Bool Any() const { return ( X | Y ); }
    Bool All() const { return ( X & Y ); }

    // String conversions
    AString ToString() const {
        return AString( "( " ) + X.ToString() + " " + Y.ToString() + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + X.ToHexString( _LeadingZeros, _Prefix ) + " " + Y.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
    static constexpr Bool2 MinValue() { return Bool2( TStdNumericLimits< bool >::min() ); }
    static constexpr Bool2 MaxValue() { return Bool2( TStdNumericLimits< bool >::max() ); }
    static const Bool2 & Zero() {
        static constexpr Bool2 ZeroVec(false);
        return ZeroVec;
    }
};

class Bool3 final {
public:
    Bool X;
    Bool Y;
    Bool Z;

    Bool3() = default;
    explicit constexpr Bool3( const bool & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Bool3( const bool & _X, const bool & _Y, const bool & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    Bool * ToPtr() {
        return &X;
    }

    const Bool * ToPtr() const {
        return &X;
    }

    Bool3 & operator=( const Bool3 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        return *this;
    }

    Bool & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }
    const Bool & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    Bool operator==( const Bool3 & _Other ) const { return X == _Other.X && Y == _Other.Y && Z == _Other.Z; }
    Bool operator!=( const Bool3 & _Other ) const { return X != _Other.X || Y != _Other.Y || Z != _Other.Z; }

    //Bool Any() const { return ( X || Y || Z ); }
    //Bool All() const { return ( X && Y && Z ); }

    Bool Any() const { return ( X | Y | Z ); }
    Bool All() const { return ( X & Y & Z ); }

    // String conversions
    AString ToString() const {
        return AString( "( " ) + X.ToString() + " " + Y.ToString() + " " + Z.ToString() + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + X.ToHexString( _LeadingZeros, _Prefix ) + " " + Y.ToHexString( _LeadingZeros, _Prefix ) + " " + Z.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
    static const Bool3 & Zero() {
        static constexpr Bool3 ZeroVec(false);
        return ZeroVec;
    }
};

class Bool4 final {
public:
    Bool X;
    Bool Y;
    Bool Z;
    Bool W;

    Bool4() = default;
    explicit constexpr Bool4( const bool & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Bool4( const bool & _X, const bool & _Y, const bool & _Z, const bool & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    Bool * ToPtr() {
        return &X;
    }

    const Bool * ToPtr() const {
        return &X;
    }

    Bool4 & operator=( const Bool4 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        W = _Other.W;
        return *this;
    }

    Bool & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }
    const Bool & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    Bool operator==( const Bool4 & _Other ) const { return X == _Other.X && Y == _Other.Y && Z == _Other.Z && W == _Other.W; }
    Bool operator!=( const Bool4 & _Other ) const { return X != _Other.X || Y != _Other.Y || Z != _Other.Z || W != _Other.W; }

    //Bool Any() const { return ( X || Y || Z || W ); }
    //Bool All() const { return ( X && Y && Z && W ); }
    Bool Any() const { return ( X | Y | Z | W ); }
    Bool All() const { return ( X & Y & Z & W ); }

    // String conversions
    AString ToString() const {
        return AString( "( " ) + X.ToString() + " " + Y.ToString() + " " + Z.ToString() + " " + W.ToString() + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + X.ToHexString( _LeadingZeros, _Prefix ) + " " + Y.ToHexString( _LeadingZeros, _Prefix ) + " " + Z.ToHexString( _LeadingZeros, _Prefix ) + " " + W.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
    static const Bool4 & Zero() {
        static constexpr Bool4 ZeroVec(false);
        return ZeroVec;
    }
};
