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

#include "BaseMath.h"

struct Bool2 {
    bool X;
    bool Y;

    Bool2() = default;
    explicit constexpr Bool2( const bool & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Bool2( const bool & _X, const bool & _Y ) : X( _X ), Y( _Y ) {}

    bool * ToPtr() {
        return &X;
    }

    const bool * ToPtr() const {
        return &X;
    }

    Bool2 & operator=( const Bool2 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        return *this;
    }

    bool & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }
    const bool & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    constexpr bool operator==( const Bool2 & _Other ) const { return X == _Other.X && Y == _Other.Y; }
    constexpr bool operator!=( const Bool2 & _Other ) const { return X != _Other.X || Y != _Other.Y; }

    //constexpr bool Any() const { return ( X || Y ); }
    //constexpr bool All() const { return ( X && Y ); }

    constexpr bool Any() const { return ( X | Y ); }
    constexpr bool All() const { return ( X & Y ); }

    // String conversions
    AString ToString() const {
        return AString( "( " ) + Math::ToString( X ) + " " + Math::ToString( Y ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        _Stream.WriteBool( X );
        _Stream.WriteBool( Y );
    }

    void Read( IBinaryStream & _Stream ) {
        X = _Stream.ReadBool();
        Y = _Stream.ReadBool();
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

struct Bool3 {
    bool X;
    bool Y;
    bool Z;

    Bool3() = default;
    explicit constexpr Bool3( const bool & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Bool3( const bool & _X, const bool & _Y, const bool & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    bool * ToPtr() {
        return &X;
    }

    const bool * ToPtr() const {
        return &X;
    }

    Bool3 & operator=( const Bool3 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        return *this;
    }

    bool & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }
    const bool & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    constexpr bool operator==( const Bool3 & _Other ) const { return X == _Other.X && Y == _Other.Y && Z == _Other.Z; }
    constexpr bool operator!=( const Bool3 & _Other ) const { return X != _Other.X || Y != _Other.Y || Z != _Other.Z; }

    //constexpr bool Any() const { return ( X || Y || Z ); }
    //constexpr bool All() const { return ( X && Y && Z ); }

    constexpr bool Any() const { return ( X | Y | Z ); }
    constexpr bool All() const { return ( X & Y & Z ); }

    // String conversions
    AString ToString() const {
        return AString( "( " ) + Math::ToString( X ) + " " + Math::ToString( Y ) + " " + Math::ToString( Z ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Z, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        _Stream.WriteBool( X );
        _Stream.WriteBool( Y );
        _Stream.WriteBool( Z );
    }

    void Read( IBinaryStream & _Stream ) {
        X = _Stream.ReadBool();
        Y = _Stream.ReadBool();
        Z = _Stream.ReadBool();
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
    static const Bool3 & Zero() {
        static constexpr Bool3 ZeroVec(false);
        return ZeroVec;
    }
};

struct Bool4 {
    bool X;
    bool Y;
    bool Z;
    bool W;

    Bool4() = default;
    explicit constexpr Bool4( const bool & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Bool4( const bool & _X, const bool & _Y, const bool & _Z, const bool & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    bool * ToPtr() {
        return &X;
    }

    const bool * ToPtr() const {
        return &X;
    }

    Bool4 & operator=( const Bool4 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        W = _Other.W;
        return *this;
    }

    bool & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }
    const bool & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    constexpr bool operator==( const Bool4 & _Other ) const { return X == _Other.X && Y == _Other.Y && Z == _Other.Z && W == _Other.W; }
    constexpr bool operator!=( const Bool4 & _Other ) const { return X != _Other.X || Y != _Other.Y || Z != _Other.Z || W != _Other.W; }

    //constexpr bool Any() const { return ( X || Y || Z || W ); }
    //constexpr bool All() const { return ( X && Y && Z && W ); }
    constexpr bool Any() const { return ( X | Y | Z | W ); }
    constexpr bool All() const { return ( X & Y & Z & W ); }

    // String conversions
    AString ToString() const {
        return AString( "( " ) + Math::ToString( X ) + " " + Math::ToString( Y ) + " " + Math::ToString( Z ) + " " + Math::ToString( W ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Z, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( W, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        _Stream.WriteBool( X );
        _Stream.WriteBool( Y );
        _Stream.WriteBool( Z );
        _Stream.WriteBool( W );
    }

    void Read( IBinaryStream & _Stream ) {
        X = _Stream.ReadBool();
        Y = _Stream.ReadBool();
        Z = _Stream.ReadBool();
        W = _Stream.ReadBool();
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
    static const Bool4 & Zero() {
        static constexpr Bool4 ZeroVec(false);
        return ZeroVec;
    }
};
