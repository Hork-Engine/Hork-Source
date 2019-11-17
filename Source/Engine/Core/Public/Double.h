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

#include "BaseMath.h"
#include "Bool.h"

class SignedByte;
class Byte;
class Short;
class UShort;
class Int;
class UInt;
class Long;
class ULong;
class Float;
class Float2;
class Float3;
class Float4;
class Double;
class Double2;
class Double3;
class Double4;
class Double2x2;
class Double3x3;
class Double3x4;
class Double4x4;

#ifndef DBL_DIG
#define DBL_DIG 10
#endif

class Double final {
public:
    double Value;

    Double() = default;
    constexpr Double( const double & _Value ) : Value( _Value ) {}

    constexpr Double( const SignedByte & _Value );
    constexpr Double( const Byte & _Value );
    constexpr Double( const Short & _Value );
    constexpr Double( const UShort & _Value );
    constexpr Double( const Int & _Value );
    constexpr Double( const UInt & _Value );
    constexpr Double( const Long & _Value );
    constexpr Double( const ULong & _Value );
    constexpr Double( const Float & _Value );
    constexpr Double( const Double & _Value );

    Double * ToPtr() {
        return this;
    }

    const Double * ToPtr() const {
        return this;
    }

    operator double() const {
        return Value;
    }

    Double & operator=( const double & _Other ) {
        Value = _Other;
        return *this;
    }

    Double & operator=( const Float & _Other );

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }
#if 0
    // Double::operator==
    friend bool operator==( const Double & _Left, const double & _Right );
    friend bool operator==( const double & _Left, const Double & _Right );
    friend bool operator==( const Double & _Left, const Double & _Right );

    // Double::operator!=
    friend bool operator!=( const Double & _Left, const double & _Right );
    friend bool operator!=( const double & _Left, const Double & _Right );
    friend bool operator!=( const Double & _Left, const Double & _Right );

    // Double::operator<
    friend bool operator<( const Double & _Left, const double & _Right );
    friend bool operator<( const double & _Left, const Double & _Right );
    friend bool operator<( const Double & _Left, const Double & _Right );

    // Double::operator>
    friend bool operator>( const Double & _Left, const double & _Right );
    friend bool operator>( const double & _Left, const Double & _Right );
    friend bool operator>( const Double & _Left, const Double & _Right );

    // Double::operator<=
    friend bool operator<=( const Double & _Left, const double & _Right );
    friend bool operator<=( const double & _Left, const Double & _Right );
    friend bool operator<=( const Double & _Left, const Double & _Right );

    // Double::operator>=
    friend bool operator>=( const Double & _Left, const double & _Right );
    friend bool operator>=( const double & _Left, const Double & _Right );
    friend bool operator>=( const Double & _Left, const Double & _Right );
#endif
    // Math operators
    Double operator+() const {
        return *this;
    }
    Double operator-() const {
        return -Value;
    }
    Double operator+( const double & _Other ) const {
        return Value + _Other;
    }
    Double operator-( const double & _Other ) const {
        return Value - _Other;
    }
    Double operator/( const double & _Other ) const {
        return Value / _Other;
    }
    Double operator*( const double & _Other ) const {
        return Value * _Other;
    }
    void operator+=( const double & _Other ) {
        Value += _Other;
    }
    void operator-=( const double & _Other ) {
        Value -= _Other;
    }
    void operator*=( const double & _Other ) {
        Value *= _Other;
    }
    void operator/=( const double & _Other ) {
        Value /= _Other;
    }

    // Floating point specific
    Bool IsInfinite() const {
        //return std::isinf( Value );
        return ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x7fffffffffffffffULL) ) == uint64_t( 0x7f80000000000000ULL );
    }

    Bool IsNan() const {
        //return std::isnan( Value );
        return ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x7f80000000000000ULL) ) == uint64_t( 0x7f80000000000000ULL );
    }

    Bool IsNormal() const {
        return std::isnormal( Value );
    }

    Bool IsDenormal() const {
        return ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x7f80000000000000ULL) ) == uint64_t( 0x0000000000000000ULL )
            && ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x007fffffffffffffULL) ) != uint64_t( 0x0000000000000000ULL );
    }

    // Comparision
    Bool LessThan( const double & _Other ) const { return std::isless( Value, _Other ); }
    Bool LequalThan( const double & _Other ) const { return std::islessequal( Value, _Other ); }
    Bool GreaterThan( const double & _Other ) const { return std::isgreater( Value, _Other ); }
    Bool GequalThan( const double & _Other ) const { return !std::isless( Value, _Other ); }
    Bool NotEqual( const double & _Other ) const { return std::islessgreater( Value, _Other ); }
    Bool Compare( const double & _Other ) const { return !NotEqual( _Other ); }

    Bool CompareEps( const Double & _Other, const Double & _Epsilon ) const {
        return Dist( _Other ) < _Epsilon;
    }

    void Clear() {
        Value = 0;
    }

    Double Abs() const {
        int64_t i = *reinterpret_cast< const int64_t * >( &Value ) & 0x7FFFFFFFFFFFFFFFLL;
        return *reinterpret_cast< const double * >( &i );
    }

    // Vector methods
    Double Length() const {
        return Abs();
    }

    Double Dist( const double & _Other ) const {
        return ( *this - _Other ).Length();
    }

    Double NormalizeSelf() {
        double L = Length();
        if ( L != 0.0 ) {
            Value /= L;
        }
        return L;
    }

    Double Normalized() const {
        const double L = Length();
        if ( L != 0.0 ) {
            return *this / L;
        }
        return *this;
    }

    Double Floor() const {
        return floor( Value );
    }

    Double Ceil() const {
        return ceil( Value );
    }

    Double Fract() const {
        return Value - floor( Value );
    }

    Double Step( const double & _Edge ) const {
        return Value < _Edge ? 0.0 : 1.0;
    }

    Double SmoothStep( const double & _Edge0, const double & _Edge1 ) const {
        const Double t = Double( ( Value - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
        return t * t * ( 3.0 - 2.0 * t );
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Double Sign() const {
        return Value > 0 ? 1.0 : -SignBits();
    }

    // Return 1 if value is less than 0 or 1 if value is greater or equal to 0
    int SignBits() const {
        return *reinterpret_cast< const uint64_t * >( &Value ) >> 63;
    }

    // Return floating point exponent
    int Exponent() const {
        return ( *reinterpret_cast< const uint64_t * >( &Value ) >> 52 ) & 0x7ff;
    }

    static constexpr int MaxExponent() { return 1023; }

    // Return floating point mantissa
    int64_t Mantissa() const {
        return *reinterpret_cast< const uint64_t * >( &Value ) & 0xfffffffffffff;
    }

    Double Lerp( const double & _To, const double & _Mix ) const {
        return Lerp( Value, _To, _Mix );
    }

    static Double Lerp( const double & _From, const double & _To, const double & _Mix ) {
        return _From + _Mix * ( _To - _From );
    }

    Double Clamp( const double & _Min, const double & _Max ) const {
        //if ( Value < _Min )
        //    return _Min;
        //if ( Value > _Max )
        //    return _Max;
        //return Value;
        return Math::Min( Math::Max( Value, _Min ), _Max );
    }

    Double Saturate() const {
        return Clamp( 0.0, 1.0 );
    }

    Double Round() const {
        return floor( Value + 0.5 );
    }

    Double RoundN( const double & _N ) const {
        return floor( Value * _N + 0.5 ) / _N;
    }

    Double Round1() const { return RoundN( 10.0 ); }
    Double Round2() const { return RoundN( 100.0 ); }
    Double Round3() const { return RoundN( 1000.0 ); }
    Double Round4() const { return RoundN( 10000.0 ); }

    Double Snap( const double & _SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        return Double( Value / _SnapValue ).Round() * _SnapValue;
    }

    Double SwapBytes() const {
        union {
            double f;
            byte   b[8];
        } dat1, dat2;

        dat1.f = Value;
        dat2.b[0] = dat1.b[7];
        dat2.b[1] = dat1.b[6];
        dat2.b[2] = dat1.b[5];
        dat2.b[3] = dat1.b[4];
        dat2.b[4] = dat1.b[3];
        dat2.b[5] = dat1.b[2];
        dat2.b[6] = dat1.b[1];
        dat2.b[7] = dat1.b[0];
        return dat2.f;
    }

    Double ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    Double ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        TSprintfBuffer< 64 > value;
        if ( _Precision >= 0 ) {
            TSprintfBuffer< 64 > format;
            value.Sprintf( format.Sprintf( "%%.%df", _Precision ), Value );
        } else {
            value.Sprintf( "%f", Value );
        }
        for ( char * p = &value.Data[ AString::Length( value.Data ) - 1 ] ; p >= &value.Data[0] ; p-- ) {
            if ( *p != '0' ) {
                if ( *p != '.' ) {
                    p++;
                }
                *p = '\0';
                return value.Data;
            }
        }
        return value.Data;
    }

    const char * CStr( int _Precision = DBL_DIG ) const {
        char * s;
        if ( _Precision >= 0 ) {
            TSprintfBuffer< 64 > format;
            s = AString::Fmt( format.Sprintf( "%%.%df", _Precision ), Value );
        } else {
            s = AString::Fmt( "%f", Value );
        }
        for ( char * p = s + AString::Length( s ) - 1 ; p >= s ; p-- ) {
            if ( *p != '0' ) {
                if ( *p != '.' ) {
                    p++;
                }
                *p = '\0';
                return s;
            }
        }
        return s;
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    Double & FromString( const AString & _String ) {
        return FromString( _String.CStr() );
    }

    Double & FromString( const char * _String );

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteDouble( Value );
    }

    void Read( IStreamBase & _Stream ) {
        Value = _Stream.ReadDouble();
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr Double MinValue() { return TStdNumericLimits< double >::min(); }
    static constexpr Double MaxValue() { return TStdNumericLimits< double >::max(); }
};

class Double2 final {
public:
    Double X;
    Double Y;

    Double2() = default;
    explicit constexpr Double2( const double & _Value ) : X( _Value ), Y( _Value ) {}
    explicit Double2( const Double3 & _Value );
    explicit Double2( const Double4 & _Value );
    constexpr Double2( const double & _X, const double & _Y ) : X( _X ), Y( _Y ) {}
    explicit Double2( const Float2 & _Value );

    Double * ToPtr() {
        return &X;
    }

    const Double * ToPtr() const {
        return &X;
    }

    Double2 & operator=( const Double2 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        return *this;
    }

    Double & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Double & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int _Shuffle >
    Double2 Shuffle2() const;

    template< int _Shuffle >
    Double3 Shuffle3() const;

    template< int _Shuffle >
    Double4 Shuffle4() const;

    Bool operator==( const Double2 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Double2 & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Double2 operator+() const {
        return *this;
    }
    Double2 operator-() const {
        return Double2( -X, -Y );
    }
    Double2 operator+( const Double2 & _Other ) const {
        return Double2( X + _Other.X, Y + _Other.Y );
    }
    Double2 operator-( const Double2 & _Other ) const {
        return Double2( X - _Other.X, Y - _Other.Y );
    }
    Double2 operator/( const Double2 & _Other ) const {
        return Double2( X / _Other.X, Y / _Other.Y );
    }
    Double2 operator*( const Double2 & _Other ) const {
        return Double2( X * _Other.X, Y * _Other.Y );
    }
    Double2 operator+( const double & _Other ) const {
        return Double2( X + _Other, Y + _Other );
    }
    Double2 operator-( const double & _Other ) const {
        return Double2( X - _Other, Y - _Other );
    }
    Double2 operator/( const double & _Other ) const {
        double Denom = 1.0 / _Other;
        return Double2( X * Denom, Y * Denom );
    }
    Double2 operator*( const double & _Other ) const {
        return Double2( X * _Other, Y * _Other );
    }
    void operator+=( const Double2 & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
    }
    void operator-=( const Double2 & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
    }
    void operator/=( const Double2 & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
    }
    void operator*=( const Double2 & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
    }
    void operator+=( const double & _Other ) {
        X += _Other;
        Y += _Other;
    }
    void operator-=( const double & _Other ) {
        X -= _Other;
        Y -= _Other;
    }
    void operator/=( const double & _Other ) {
        double Denom = 1.0 / _Other;
        X *= Denom;
        Y *= Denom;
    }
    void operator*=( const double & _Other ) {
        X *= _Other;
        Y *= _Other;
    }

    Double Min() const {
        return Math::Min( X, Y );
    }

    Double Max() const {
        return Math::Max( X, Y );
    }

    int MinorAxis() const {
        return int( X.Abs() >= Y.Abs() );
    }

    int MajorAxis() const {
        return int( X.Abs() < Y.Abs() );
    }

    // Floating point specific
    Bool2 IsInfinite() const {
        return Bool2( X.IsInfinite(), Y.IsInfinite() );
    }

    Bool2 IsNan() const {
        return Bool2( X.IsNan(), Y.IsNan() );
    }

    Bool2 IsNormal() const {
        return Bool2( X.IsNormal(), Y.IsNormal() );
    }

    Bool2 IsDenormal() const {
        return Bool2( X.IsDenormal(), Y.IsDenormal() );
    }

    // Comparision

    Bool2 LessThan( const Double2 & _Other ) const {
        return Bool2( X.LessThan( _Other.X ), Y.LessThan( _Other.Y ) );
    }
    Bool2 LessThan( const double & _Other ) const {
        return Bool2( X.LessThan( _Other ), Y.LessThan( _Other ) );
    }

    Bool2 LequalThan( const Double2 & _Other ) const {
        return Bool2( X.LequalThan( _Other.X ), Y.LequalThan( _Other.Y ) );
    }
    Bool2 LequalThan( const double & _Other ) const {
        return Bool2( X.LequalThan( _Other ), Y.LequalThan( _Other ) );
    }

    Bool2 GreaterThan( const Double2 & _Other ) const {
        return Bool2( X.GreaterThan( _Other.X ), Y.GreaterThan( _Other.Y ) );
    }
    Bool2 GreaterThan( const double & _Other ) const {
        return Bool2( X.GreaterThan( _Other ), Y.GreaterThan( _Other ) );
    }

    Bool2 GequalThan( const Double2 & _Other ) const {
        return Bool2( X.GequalThan( _Other.X ), Y.GequalThan( _Other.Y ) );
    }
    Bool2 GequalThan( const double & _Other ) const {
        return Bool2( X.GequalThan( _Other ), Y.GequalThan( _Other ) );
    }

    Bool2 NotEqual( const Double2 & _Other ) const {
        return Bool2( X.NotEqual( _Other.X ), Y.NotEqual( _Other.Y ) );
    }
    Bool2 NotEqual( const double & _Other ) const {
        return Bool2( X.NotEqual( _Other ), Y.NotEqual( _Other ) );
    }

    Bool Compare( const Double2 & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    Bool CompareEps( const Double2 & _Other, const Double & _Epsilon ) const {
        return Bool2( X.CompareEps( _Other.X, _Epsilon ),
                      Y.CompareEps( _Other.Y, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = 0;
    }

    Double2 Abs() const { return Double2( X.Abs(), Y.Abs() ); }

    // Vector methods
    Double LengthSqr() const {
        return X * X + Y * Y;
    }

    Double Length() const {
        return StdSqrt( LengthSqr() );
    }

    Double DistSqr( const Double2 & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }

    Double Dist( const Double2 & _Other ) const {
        return ( *this - _Other ).Length();
    }

    Double NormalizeSelf() {
        double L = Length();
        if ( L != 0.0 ) {
            double InvLength = 1.0 / L;
            X *= InvLength;
            Y *= InvLength;
        }
        return L;
    }

    Double2 Normalized() const {
        const double L = Length();
        if ( L != 0.0 ) {
            const double InvLength = 1.0 / L;
            return Double2( X * InvLength, Y * InvLength );
        }
        return *this;
    }

    double Cross( const Double2 & _Other ) const {
        return X * _Other.Y - Y * _Other.X;
    }

    Double2 Floor() const {
        return Double2( X.Floor(), Y.Floor() );
    }

    Double2 Ceil() const {
        return Double2( X.Ceil(), Y.Ceil() );
    }

    Double2 Fract() const {
        return Double2( X.Fract(), Y.Fract() );
    }

    Double2 Step( const double & _Edge ) const;
    Double2 Step( const Double2 & _Edge ) const;

    Double2 SmoothStep( const double & _Edge0, const double & _Edge1 ) const;
    Double2 SmoothStep( const Double2 & _Edge0, const Double2 & _Edge1 ) const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Double2 Sign() const {
        return Double2( X.Sign(), Y.Sign() );
    }

    int SignBits() const {
        return X.SignBits() | (Y.SignBits()<<1);
    }

    Double2 Lerp( const Double2 & _To, const double & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    Double2 Lerp( const Double2 & _To, const Double2 & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    static Double2 Lerp( const Double2 & _From, const Double2 & _To, const double & _Mix );
    static Double2 Lerp( const Double2 & _From, const Double2 & _To, const Double2 & _Mix );

    Double Bilerp( const double & _A, const double & _B, const double & _C, const double & _D ) const;
    Double2 Bilerp( const Double2 & _A, const Double2 & _B, const Double2 & _C, const Double2 & _D ) const;
    Double3 Bilerp( const Double3 & _A, const Double3 & _B, const Double3 & _C, const Double3 & _D ) const;
    Double4 Bilerp( const Double4 & _A, const Double4 & _B, const Double4 & _C, const Double4 & _D ) const;

    Double2 Clamp( const double & _Min, const double & _Max ) const {
        return Double2( X.Clamp( _Min, _Max ), Y.Clamp( _Min, _Max ) );
    }

    Double2 Clamp( const Double2 & _Min, const Double2 & _Max ) const {
        return Double2( X.Clamp( _Min.X, _Max.X ), Y.Clamp( _Min.Y, _Max.Y ) );
    }

    Double2 Saturate() const {
        return Clamp( 0.0, 1.0 );
    }

    Double2 Snap( const double &_SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        Double2 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = SnapVector.X.Round() * _SnapValue;
        SnapVector.Y = SnapVector.Y.Round() * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const {
        if ( X == 1.0 || X == -1.0 ) return Math::AxialX;
        if ( Y == 1.0 || Y == -1.0 ) return Math::AxialY;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == 1.0 ) return Math::AxialX;
        if ( Y == 1.0 ) return Math::AxialY;
        return Math::NonAxial;
    }

    int VectorAxialType() const {
        if ( X.Abs() < 0.00001f ) {
            return ( Y.Abs() < 0.00001f ) ? Math::NonAxial : Math::AxialY;
        }
        return ( Y.Abs() < 0.00001f ) ? Math::AxialX : Math::NonAxial;
    }

    Double Dot( const Double2 & _Other ) const {
        return X * _Other.X + Y * _Other.Y;
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        return AString( "( " ) + X.ToString( _Precision ) + " " + Y.ToString( _Precision ) + " )";
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
    static const Double2 & Zero() {
        static constexpr Double2 ZeroVec(0.0);
        return ZeroVec;
    }
};

class Double3 final {
public:
    Double X;
    Double Y;
    Double Z;

    Double3() = default;
    explicit constexpr Double3( const double & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    explicit Double3( const Double2 & _Value, const double & _Z = 0.0 );
    explicit Double3( const Double4 & _Value );
    constexpr Double3( const double & _X, const double & _Y, const double & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}
    explicit Double3( const Float3 & _Value );

    Double * ToPtr() {
        return &X;
    }

    const Double * ToPtr() const {
        return &X;
    }

    Double3 & operator=( const Double3 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        return *this;
    }

    Double & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Double & operator[]( const int & _Index ) const {
        //AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int _Shuffle >
    Double2 Shuffle2() const;

    template< int _Shuffle >
    Double3 Shuffle3() const;

    template< int _Shuffle >
    Double4 Shuffle4() const;

    Bool operator==( const Double3 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Double3 & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Double3 operator+() const {
        return *this;
    }
    Double3 operator-() const {
        return Double3( -X, -Y, -Z );
    }
     Double3 operator+( const Double3 & _Other ) const {
        return Double3( X + _Other.X, Y + _Other.Y, Z + _Other.Z );
    }
    Double3 operator-( const Double3 & _Other ) const {
        return Double3( X - _Other.X, Y - _Other.Y, Z - _Other.Z );
    }
    Double3 operator/( const Double3 & _Other ) const {
        return Double3( X / _Other.X, Y / _Other.Y, Z / _Other.Z );
    }
    Double3 operator*( const Double3 & _Other ) const {
        return Double3( X * _Other.X, Y * _Other.Y, Z * _Other.Z );
    }
    Double3 operator+( const double & _Other ) const {
        return Double3( X + _Other, Y + _Other, Z + _Other );
    }
    Double3 operator-( const double & _Other ) const {
        return Double3( X - _Other, Y - _Other, Z - _Other );
    }
    Double3 operator/( const double & _Other ) const {
        double Denom = 1.0 / _Other;
        return Double3( X * Denom, Y * Denom, Z * Denom );
    }
    Double3 operator*( const double & _Other ) const {
        return Double3( X * _Other, Y * _Other, Z * _Other );
    }
    void operator+=( const Double3 & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
    }
    void operator-=( const Double3 & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
    }
    void operator/=( const Double3 & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
    }
    void operator*=( const Double3 & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
    }
    void operator+=( const double & _Other ) {
        X += _Other;
        Y += _Other;
        Z += _Other;
    }
    void operator-=( const double & _Other ) {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
    }
    void operator/=( const double & _Other ) {
        double Denom = 1.0 / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
    }
    void operator*=( const double & _Other ) {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
    }

    Double Min() const {
        return Math::Min( Math::Min( X, Y ), Z );
    }

    Double Max() const {
        return Math::Max( Math::Max( X, Y ), Z );
    }

    int MinorAxis() const {
        double Minor = X.Abs();
        int Axis = 0;
        double Tmp;

        Tmp = Y.Abs();
        if ( Tmp <= Minor ) {
            Axis = 1;
            Minor = Tmp;
        }

        Tmp = Z.Abs();
        if ( Tmp <= Minor ) {
            Axis = 2;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const {
        double Major = X.Abs();
        int Axis = 0;
        double Tmp;

        Tmp = Y.Abs();
        if ( Tmp > Major ) {
            Axis = 1;
            Major = Tmp;
        }

        Tmp = Z.Abs();
        if ( Tmp > Major ) {
            Axis = 2;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    Bool3 IsInfinite() const {
        return Bool3( X.IsInfinite(), Y.IsInfinite(), Z.IsInfinite() );
    }

    Bool3 IsNan() const {
        return Bool3( X.IsNan(), Y.IsNan(), Z.IsNan() );
    }

    Bool3 IsNormal() const {
        return Bool3( X.IsNormal(), Y.IsNormal(), Z.IsNormal() );
    }

    Bool3 IsDenormal() const {
        return Bool3( X.IsDenormal(), Y.IsDenormal(), Z.IsDenormal() );
    }

    // Comparision
    Bool3 LessThan( const Double3 & _Other ) const {
        return Bool3( X.LessThan( _Other.X ), Y.LessThan( _Other.Y ), Z.LessThan( _Other.Z ) );
    }
    Bool3 LessThan( const double & _Other ) const {
        return Bool3( X.LessThan( _Other ), Y.LessThan( _Other ), Z.LessThan( _Other ) );
    }

    Bool3 LequalThan( const Double3 & _Other ) const {
        return Bool3( X.LequalThan( _Other.X ), Y.LequalThan( _Other.Y ), Z.LequalThan( _Other.Z ) );
    }
    Bool3 LequalThan( const double & _Other ) const {
        return Bool3( X.LequalThan( _Other ), Y.LequalThan( _Other ), Z.LequalThan( _Other ) );
    }

    Bool3 GreaterThan( const Double3 & _Other ) const {
        return Bool3( X.GreaterThan( _Other.X ), Y.GreaterThan( _Other.Y ), Z.GreaterThan( _Other.Z ) );
    }
    Bool3 GreaterThan( const double & _Other ) const {
        return Bool3( X.GreaterThan( _Other ), Y.GreaterThan( _Other ), Z.GreaterThan( _Other ) );
    }

    Bool3 GequalThan( const Double3 & _Other ) const {
        return Bool3( X.GequalThan( _Other.X ), Y.GequalThan( _Other.Y ), Z.GequalThan( _Other.Z ) );
    }
    Bool3 GequalThan( const double & _Other ) const {
        return Bool3( X.GequalThan( _Other ), Y.GequalThan( _Other ), Z.GequalThan( _Other ) );
    }

    Bool3 NotEqual( const Double3 & _Other ) const {
        return Bool3( X.NotEqual( _Other.X ), Y.NotEqual( _Other.Y ), Z.NotEqual( _Other.Z ) );
    }
    Bool3 NotEqual( const double & _Other ) const {
        return Bool3( X.NotEqual( _Other ), Y.NotEqual( _Other ), Z.NotEqual( _Other ) );
    }

    Bool Compare( const Double3 & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    Bool CompareEps( const Double3 & _Other, const Double & _Epsilon ) const {
        return Bool3( X.CompareEps( _Other.X, _Epsilon ),
                      Y.CompareEps( _Other.Y, _Epsilon ),
                      Z.CompareEps( _Other.Z, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = Z = 0;
    }

    Double3 Abs() const { return Double3( X.Abs(), Y.Abs(), Z.Abs() ); }

    // Vector methods
    Double LengthSqr() const {
        return X * X + Y * Y + Z * Z;
    }
    Double Length() const {
        return StdSqrt( LengthSqr() );
    }
    Double DistSqr( const Double3 & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }
    Double Dist( const Double3 & _Other ) const {
        return ( *this - _Other ).Length();
    }
    Double NormalizeSelf() {
        const double L = Length();
        if ( L != 0.0 ) {
            const double InvLength = 1.0 / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
        }
        return L;
    }
    Double3 Normalized() const {
        const double L = Length();
        if ( L != 0.0 ) {
            const double InvLength = 1.0 / L;
            return Double3( X * InvLength, Y * InvLength, Z * InvLength );
        }
        return *this;
    }
    Double3 NormalizeFix() const {
        Double3 Normal = Normalized();
        Normal.FixNormal();
        return Normal;
    }
    // Return true if normal was fixed
    bool FixNormal() {
        const double ZERO = 0;
        const double ONE = 1;
        const double MINUS_ONE = -1;

        if ( X == -ZERO ) {
            X = ZERO;
        }

        if ( Y == -ZERO ) {
            Y = ZERO;
        }

        if ( Z == -ZERO ) {
            Z = ZERO;
        }

        if ( X == ZERO ) {
            if ( Y == ZERO ) {
                if ( Z > ZERO ) {
                    if ( Z != ONE ) {
                        Z = ONE;
                        return true;
                    }
                    return false;
                }
                if ( Z != MINUS_ONE ) {
                    Z = MINUS_ONE;
                    return true;
                }
                return false;
            } else if ( Z == ZERO ) {
                if ( Y > ZERO ) {
                    if ( Y != ONE ) {
                        Y = ONE;
                        return true;
                    }
                    return false;
                }
                if ( Y != MINUS_ONE ) {
                    Y = MINUS_ONE;
                    return true;
                }
                return false;
            }
        } else if ( Y == ZERO ) {
            if ( Z == ZERO ) {
                if ( X > ZERO ) {
                    if ( X != ONE ) {
                        X = ONE;
                        return true;
                    }
                    return false;
                }
                if ( X != MINUS_ONE ) {
                    X = MINUS_ONE;
                    return true;
                }
                return false;
            }
        }

        if ( X.Abs() == ONE ) {
            if ( Y != ZERO || Z != ZERO ) {
                Y = Z = ZERO;
                return true;
            }
            return false;
        }

        if ( Y.Abs() == ONE ) {
            if ( X != ZERO || Z != ZERO ) {
                X = Z = ZERO;
                return true;
            }
            return false;
        }

        if ( Z.Abs() == ONE ) {
            if ( X != ZERO || Y != ZERO ) {
                X = Y = ZERO;
                return true;
            }
            return false;
        }

        return false;
    }

    Double3 Floor() const {
        return Double3( X.Floor(), Y.Floor(), Z.Floor() );
    }

    Double3 Ceil() const {
        return Double3( X.Ceil(), Y.Ceil(), Z.Ceil() );
    }

    Double3 Fract() const {
        return Double3( X.Fract(), Y.Fract(), Z.Fract() );
    }

    Double3 Step( const double & _Edge ) const;
    Double3 Step( const Double3 & _Edge ) const;

    Double3 SmoothStep( const double & _Edge0, const double & _Edge1 ) const;
    Double3 SmoothStep( const Double3 & _Edge0, const Double3 & _Edge1 ) const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Double3 Sign() const {
        return Double3( X.Sign(), Y.Sign(), Z.Sign() );
    }

    int SignBits() const {
        return X.SignBits() | (Y.SignBits()<<1) | (Z.SignBits()<<2);
    }

    Double3 Lerp( const Double3 & _To, const double & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    Double3 Lerp( const Double3 & _To, const Double3 & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    static Double3 Lerp( const Double3 & _From, const Double3 & _To, const double & _Mix );
    static Double3 Lerp( const Double3 & _From, const Double3 & _To, const Double3 & _Mix );

    Double3 Clamp( const double & _Min, const double & _Max ) const {
        return Double3( X.Clamp( _Min, _Max ), Y.Clamp( _Min, _Max ), Z.Clamp( _Min, _Max ) );
    }

    Double3 Clamp( const Double3 & _Min, const Double3 & _Max ) const {
        return Double3( X.Clamp( _Min.X, _Max.X ), Y.Clamp( _Min.Y, _Max.Y ), Z.Clamp( _Min.Z, _Max.Z ) );
    }

    Double3 Saturate() const {
        return Clamp( 0.0, 1.0 );
    }

    Double3 Snap( const double &_SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        Double3 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = SnapVector.X.Round() * _SnapValue;
        SnapVector.Y = SnapVector.Y.Round() * _SnapValue;
        SnapVector.Z = SnapVector.Z.Round() * _SnapValue;
        return SnapVector;
    }

    Double3 SnapNormal( const double & _Epsilon ) const {
        Double3 Normal = *this;
        for ( int i = 0 ; i < 3 ; i++ ) {
            if ( ( Normal[i] - 1.0 ).Abs() < _Epsilon ) {
                Normal = Double3(0);
                Normal[i] = 1;
                break;
            }
            if ( ( Normal[i] - -1.0 ).Abs() < _Epsilon ) {
                Normal = Double3(0);
                Normal[i] = -1;
                break;
            }
        }

        if ( Normal[0].Abs() < _Epsilon && Normal[1].Abs() >= _Epsilon && Normal[2].Abs() >= _Epsilon ) {
            Normal[0] = 0;
            Normal.NormalizeSelf();
        } else if ( Normal[1].Abs() < _Epsilon && Normal[0].Abs() >= _Epsilon && Normal[2].Abs() >= _Epsilon ) {
            Normal[1] = 0;
            Normal.NormalizeSelf();
        } else if ( Normal[2].Abs() < _Epsilon && Normal[0].Abs() >= _Epsilon && Normal[1].Abs() >= _Epsilon ) {
            Normal[2] = 0;
            Normal.NormalizeSelf();
        }

        return Normal;
    }

    int NormalAxialType() const {
        if ( X == 1.0 || X == -1.0 ) return Math::AxialX;
        if ( Y == 1.0 || Y == -1.0 ) return Math::AxialY;
        if ( Z == 1.0 || Z == -1.0 ) return Math::AxialZ;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == 1.0 ) return Math::AxialX;
        if ( Y == 1.0 ) return Math::AxialY;
        if ( Z == 1.0 ) return Math::AxialZ;
        return Math::NonAxial;
    }

    int VectorAxialType() const {
        Bool3 Zero = Abs().LessThan( 0.00001f );

        if ( int( Zero.X + Zero.Y + Zero.Z ) != 2 ) {
            return Math::NonAxial;
        }

        if ( !Zero.X ) {
            return Math::AxialX;
        }

        if ( !Zero.Y ) {
            return Math::AxialY;
        }

        if ( !Zero.Z ) {
            return Math::AxialZ;
        }

        return Math::NonAxial;
    }

    Double Dot( const Double3 & _Other ) const {
        return X * _Other.X + Y * _Other.Y + Z * _Other.Z;
    }
    Double3 Cross( const Double3 & _Other ) const {
        return Double3( Y * _Other.Z - _Other.Y * Z,
                        Z * _Other.X - _Other.Z * X,
                        X * _Other.Y - _Other.X * Y );
    }

    Double3 Perpendicular() const {
        double dp = X * X + Y * Y;
        if ( !dp ) {
            return Double3(1,0,0);
        } else {
            dp = Math::InvSqrt( dp );
            return Double3( -Y * dp, X * dp, 0 );
        }
    }

    void ComputeBasis( Double3 & _XVec, Double3 & _YVec ) const {
        _YVec = Perpendicular();
        _XVec = _YVec.Cross( *this );
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        return AString( "( " ) + X.ToString( _Precision ) + " " + Y.ToString( _Precision ) + " " + Z.ToString( _Precision ) + " )";
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
    static const Double3 & Zero() {
        static constexpr Double3 ZeroVec(0.0);
        return ZeroVec;
    }
};

class Double4 final {
public:
    Double X;
    Double Y;
    Double Z;
    Double W;

    Double4() = default;
    explicit constexpr Double4( const double & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    explicit Double4( const Double2 & _Value, const double & _Z = 0.0, const double & _W = 0.0 );
    explicit Double4( const Double3 & _Value, const double & _W = 0.0 );
    constexpr Double4( const double & _X, const double & _Y, const double & _Z, const double & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}
    explicit Double4( const Float4 & _Value );

    Double * ToPtr() {
        return &X;
    }

    const Double * ToPtr() const {
        return &X;
    }

    Double4 & operator=( const Double4 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        W = _Other.W;
        return *this;
    }

    Double & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Double & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int _Shuffle >
    Double2 Shuffle2() const;

    template< int _Shuffle >
    Double3 Shuffle3() const;

    template< int _Shuffle >
    Double4 Shuffle4() const;

    Bool operator==( const Double4 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Double4 & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Double4 operator+() const {
        return *this;
    }
    Double4 operator-() const {
        return Double4( -X, -Y, -Z, -W );
    }
    Double4 operator+( const Double4 & _Other ) const {
        return Double4( X + _Other.X, Y + _Other.Y, Z + _Other.Z, W + _Other.W );
    }
    Double4 operator-( const Double4 & _Other ) const {
        return Double4( X - _Other.X, Y - _Other.Y, Z - _Other.Z, W - _Other.W );
    }
    Double4 operator/( const Double4 & _Other ) const {
        return Double4( X / _Other.X, Y / _Other.Y, Z / _Other.Z, W / _Other.W );
    }
    Double4 operator*( const Double4 & _Other ) const {
        return Double4( X * _Other.X, Y * _Other.Y, Z * _Other.Z, W * _Other.W );
    }
    Double4 operator+( const double & _Other ) const {
        return Double4( X + _Other, Y + _Other, Z + _Other, W + _Other );
    }
    Double4 operator-( const double & _Other ) const {
        return Double4( X - _Other, Y - _Other, Z - _Other, W - _Other );
    }
    Double4 operator/( const double & _Other ) const {
        double Denom = 1.0 / _Other;
        return Double4( X * Denom, Y * Denom, Z * Denom, W * Denom );
    }
    Double4 operator*( const double & _Other ) const {
        return Double4( X * _Other, Y * _Other, Z * _Other, W * _Other );
    }
    void operator+=( const Double4 & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
        W += _Other.W;
    }
    void operator-=( const Double4 & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
        W -= _Other.W;
    }
    void operator/=( const Double4 & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
        W /= _Other.W;
    }
    void operator*=( const Double4 & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
        W *= _Other.W;
    }
    void operator+=( const double & _Other ) {
        X += _Other;
        Y += _Other;
        Z += _Other;
        W += _Other;
    }
    void operator-=( const double & _Other ) {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
        W -= _Other;
    }
    void operator/=( const double & _Other ) {
        double Denom = 1.0 / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
        W *= Denom;
    }
    void operator*=( const double & _Other ) {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
        W *= _Other;
    }

    Double Min() const {
        return Math::Min( Math::Min( Math::Min( X, Y ), Z ), W );
    }

    Double Max() const {
        return Math::Max( Math::Max( Math::Max( X, Y ), Z ), W );
    }

    int MinorAxis() const {
        double Minor = X.Abs();
        int Axis = 0;
        double Tmp;

        Tmp = Y.Abs();
        if ( Tmp <= Minor ) {
            Axis = 1;
            Minor = Tmp;
        }

        Tmp = Z.Abs();
        if ( Tmp <= Minor ) {
            Axis = 2;
            Minor = Tmp;
        }

        Tmp = W.Abs();
        if ( Tmp <= Minor ) {
            Axis = 3;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const {
        double Major = X.Abs();
        int Axis = 0;
        double Tmp;

        Tmp = Y.Abs();
        if ( Tmp > Major ) {
            Axis = 1;
            Major = Tmp;
        }

        Tmp = Z.Abs();
        if ( Tmp > Major ) {
            Axis = 2;
            Major = Tmp;
        }

        Tmp = W.Abs();
        if ( Tmp > Major ) {
            Axis = 3;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    Bool4 IsInfinite() const {
        return Bool4( X.IsInfinite(), Y.IsInfinite(), Z.IsInfinite(), W.IsInfinite() );
    }

    Bool4 IsNan() const {
        return Bool4( X.IsNan(), Y.IsNan(), Z.IsNan(), W.IsNan() );
    }

    Bool4 IsNormal() const {
        return Bool4( X.IsNormal(), Y.IsNormal(), Z.IsNormal(), W.IsNormal() );
    }

    Bool4 IsDenormal() const {
        return Bool4( X.IsDenormal(), Y.IsDenormal(), Z.IsDenormal(), W.IsDenormal() );
    }

    // Comparision
    Bool4 LessThan( const Double4 & _Other ) const {
        return Bool4( X.LessThan( _Other.X ), Y.LessThan( _Other.Y ), Z.LessThan( _Other.Z ), W.LessThan( _Other.W ) );
    }
    Bool4 LessThan( const double & _Other ) const {
        return Bool4( X.LessThan( _Other ), Y.LessThan( _Other ), Z.LessThan( _Other ), W.LessThan( _Other ) );
    }

    Bool4 LequalThan( const Double4 & _Other ) const {
        return Bool4( X.LequalThan( _Other.X ), Y.LequalThan( _Other.Y ), Z.LequalThan( _Other.Z ), W.LequalThan( _Other.W ) );
    }
    Bool4 LequalThan( const double & _Other ) const {
        return Bool4( X.LequalThan( _Other ), Y.LequalThan( _Other ), Z.LequalThan( _Other ), W.LequalThan( _Other ) );
    }

    Bool4 GreaterThan( const Double4 & _Other ) const {
        return Bool4( X.GreaterThan( _Other.X ), Y.GreaterThan( _Other.Y ), Z.GreaterThan( _Other.Z ), W.GreaterThan( _Other.W ) );
    }
    Bool4 GreaterThan( const double & _Other ) const {
        return Bool4( X.GreaterThan( _Other ), Y.GreaterThan( _Other ), Z.GreaterThan( _Other ), W.GreaterThan( _Other ) );
    }

    Bool4 GequalThan( const Double4 & _Other ) const {
        return Bool4( X.GequalThan( _Other.X ), Y.GequalThan( _Other.Y ), Z.GequalThan( _Other.Z ), W.GequalThan( _Other.W ) );
    }
    Bool4 GequalThan( const double & _Other ) const {
        return Bool4( X.GequalThan( _Other ), Y.GequalThan( _Other ), Z.GequalThan( _Other ), W.GequalThan( _Other ) );
    }

    Bool4 NotEqual( const Double4 & _Other ) const {
        return Bool4( X.NotEqual( _Other.X ), Y.NotEqual( _Other.Y ), Z.NotEqual( _Other.Z ), W.NotEqual( _Other.W ) );
    }
    Bool4 NotEqual( const double & _Other ) const {
        return Bool4( X.NotEqual( _Other ), Y.NotEqual( _Other ), Z.NotEqual( _Other ), W.NotEqual( _Other ) );
    }

    Bool Compare( const Double4 & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    Bool CompareEps( const Double4 & _Other, const Double & _Epsilon ) const {
        return Bool4( X.CompareEps( _Other.X, _Epsilon ),
                      Y.CompareEps( _Other.Y, _Epsilon ),
                      Z.CompareEps( _Other.Z, _Epsilon ),
                      W.CompareEps( _Other.W, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = Z = W = 0;
    }

    Double4 Abs() const { return Double4( X.Abs(), Y.Abs(), Z.Abs(), W.Abs() ); }

    // Vector methods
    Double LengthSqr() const {
        return X * X + Y * Y + Z * Z + W * W;
    }
    Double Length() const {
        return StdSqrt( LengthSqr() );
    }
    Double DistSqr( const Double4 & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }
    Double Dist( const Double4 & _Other ) const {
        return ( *this - _Other ).Length();
    }
    Double NormalizeSelf() {
        const double L = Length();
        if ( L != 0.0 ) {
            const double InvLength = 1.0 / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
            W *= InvLength;
        }
        return L;
    }
    Double4 Normalized() const {
        const double L = Length();
        if ( L != 0.0 ) {
            const double InvLength = 1.0 / L;
            return Double4( X * InvLength, Y * InvLength, Z * InvLength, W * InvLength );
        }
        return *this;
    }

    Double4 Floor() const {
        return Double4( X.Floor(), Y.Floor(), Z.Floor(), W.Floor() );
    }

    Double4 Ceil() const {
        return Double4( X.Ceil(), Y.Ceil(), Z.Ceil(), W.Ceil() );
    }

    Double4 Fract() const {
        return Double4( X.Fract(), Y.Fract(), Z.Fract(), W.Fract() );
    }

    Double4 Step( const double & _Edge ) const;
    Double4 Step( const Double4 & _Edge ) const;

    Double4 SmoothStep( const double & _Edge0, const double & _Edge1 ) const;
    Double4 SmoothStep( const Double4 & _Edge0, const Double4 & _Edge1 ) const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Double4 Sign() const {
        return Double4( X.Sign(), Y.Sign(), Z.Sign(), W.Sign() );
    }

    int SignBits() const {
        return X.SignBits() | (Y.SignBits()<<1) | (Z.SignBits()<<2) | (W.SignBits()<<3);
    }

    Double4 Lerp( const Double4 & _To, const double & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    Double4 Lerp( const Double4 & _To, const Double4 & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    static Double4 Lerp( const Double4 & _From, const Double4 & _To, const double & _Mix );
    static Double4 Lerp( const Double4 & _From, const Double4 & _To, const Double4 & _Mix );

    Double4 Clamp( const double & _Min, const double & _Max ) const {
        return Double4( X.Clamp( _Min, _Max ), Y.Clamp( _Min, _Max ), Z.Clamp( _Min, _Max ), W.Clamp( _Min, _Max ) );
    }

    Double4 Clamp( const Double4 & _Min, const Double4 & _Max ) const {
        return Double4( X.Clamp( _Min.X, _Max.X ), Y.Clamp( _Min.Y, _Max.Y ), Z.Clamp( _Min.Z, _Max.Z ), W.Clamp( _Min.W, _Max.W ) );
    }

    Double4 Saturate() const {
        return Clamp( 0.0, 1.0 );
    }

    Double4 Snap( const double &_SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        Double4 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = SnapVector.X.Round() * _SnapValue;
        SnapVector.Y = SnapVector.Y.Round() * _SnapValue;
        SnapVector.Z = SnapVector.Z.Round() * _SnapValue;
        SnapVector.W = SnapVector.W.Round() * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const {
        if ( X == 1.0 || X == -1.0 ) return Math::AxialX;
        if ( Y == 1.0 || Y == -1.0 ) return Math::AxialY;
        if ( Z == 1.0 || Z == -1.0 ) return Math::AxialZ;
        if ( W == 1.0 || W == -1.0 ) return Math::AxialW;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == 1.0 ) return Math::AxialX;
        if ( Y == 1.0 ) return Math::AxialY;
        if ( Z == 1.0 ) return Math::AxialZ;
        if ( W == 1.0 ) return Math::AxialW;
        return Math::NonAxial;
    }

    int VectorAxialType() const {
        Bool4 Zero = Abs().LessThan( 0.00001f );

        if ( int( Zero.X + Zero.Y + Zero.Z + Zero.W ) != 3 ) {
            return Math::NonAxial;
        }

        if ( !Zero.X ) {
            return Math::AxialX;
        }

        if ( !Zero.Y ) {
            return Math::AxialY;
        }

        if ( !Zero.Z ) {
            return Math::AxialZ;
        }

        if ( !Zero.W ) {
            return Math::AxialW;
        }

        return Math::NonAxial;
    }

    Double Dot( const Double4 & _Other ) const {
        return X * _Other.X + Y * _Other.Y + Z * _Other.Z + W * _Other.W;
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        return AString( "( " ) + X.ToString( _Precision ) + " " + Y.ToString( _Precision ) + " " + Z.ToString( _Precision ) + " " + W.ToString( _Precision ) + " )";
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
    static const Double4 & Zero() {
        static constexpr Double4 ZeroVec(0.0);
        return ZeroVec;
    }
};

// Double3 to Double2
AN_FORCEINLINE Double2::Double2( const Double3 & _Value ) : X( _Value.X ), Y( _Value.Y ) {}
// Double4 to Double2
AN_FORCEINLINE Double2::Double2( const Double4 & _Value ) : X( _Value.X ), Y( _Value.Y ) {}

// Double4 to Double3
AN_FORCEINLINE Double3::Double3( const Double4 & _Value ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}
// Double2 to Double3
AN_FORCEINLINE Double3::Double3( const Double2 & _Value, const double & _Z ) : X( _Value.X ), Y( _Value.Y ), Z( _Z ) {}

// Double2 to Double4
AN_FORCEINLINE Double4::Double4( const Double2 & _Value, const double & _Z, const double & _W ) : X( _Value.X ), Y( _Value.Y ), Z( _Z ), W( _W ) {}
// Double3 to Double4
AN_FORCEINLINE Double4::Double4( const Double3 & _Value, const double & _W ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ), W( _W ) {}

// Double2 Shuffle

template< int _Shuffle >
Double2 Double2::Shuffle2() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    return Double2( (&X)[iX], (&X)[iY] );
}

template< int _Shuffle >
Double3 Double2::Shuffle3() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    return Double3( (&X)[iX], (&X)[iY], (&X)[iZ] );
}

template< int _Shuffle >
Double4 Double2::Shuffle4() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    constexpr int iW = _Shuffle & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    static_assert( iW >= 0 && iW < NumComponents(), "Index out of range" );
    return Double4( (&X)[iX], (&X)[iY], (&X)[iZ], (&X)[iW] );
}

// Double3 Shuffle

template< int _Shuffle >
Double2 Double3::Shuffle2() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    return Double2( (&X)[iX], (&X)[iY] );
}

template< int _Shuffle >
Double3 Double3::Shuffle3() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    return Double3( (&X)[iX], (&X)[iY], (&X)[iZ] );
}

template< int _Shuffle >
Double4 Double3::Shuffle4() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    constexpr int iW = _Shuffle & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    static_assert( iW >= 0 && iW < NumComponents(), "Index out of range" );
    return Double4( (&X)[iX], (&X)[iY], (&X)[iZ], (&X)[iW] );
}

// Double4 Shuffle

template< int _Shuffle >
Double2 Double4::Shuffle2() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    return Double2( (&X)[iX], (&X)[iY] );
}

template< int _Shuffle >
Double3 Double4::Shuffle3() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    return Double3( (&X)[iX], (&X)[iY], (&X)[iZ] );
}

template< int _Shuffle >
Double4 Double4::Shuffle4() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    constexpr int iW = _Shuffle & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    static_assert( iW >= 0 && iW < NumComponents(), "Index out of range" );
    return Double4( (&X)[iX], (&X)[iY], (&X)[iZ], (&X)[iW] );
}

AN_FORCEINLINE Double2 operator+( const double & _Left, const Double2 & _Right ) {
    return Double2( _Left + _Right.X.Value, _Left + _Right.Y.Value );
}
AN_FORCEINLINE Double2 operator-( const double & _Left, const Double2 & _Right ) {
    return Double2( _Left - _Right.X.Value, _Left - _Right.Y.Value );
}
AN_FORCEINLINE Double3 operator+( const double & _Left, const Double3 & _Right ) {
    return Double3( _Left + _Right.X.Value, _Left + _Right.Y.Value, _Left + _Right.Z.Value );
}
AN_FORCEINLINE Double3 operator-( const double & _Left, const Double3 & _Right ) {
    return Double3( _Left - _Right.X.Value, _Left - _Right.Y.Value, _Left - _Right.Z.Value );
}
AN_FORCEINLINE Double4 operator+( const double & _Left, const Double4 & _Right ) {
    return Double4( _Left + _Right.X.Value, _Left + _Right.Y.Value, _Left + _Right.Z.Value, _Left + _Right.W.Value );
}
AN_FORCEINLINE Double4 operator-( const double & _Left, const Double4 & _Right ) {
    return Double4( _Left - _Right.X.Value, _Left - _Right.Y.Value, _Left - _Right.Z.Value, _Left - _Right.W.Value );
}

AN_FORCEINLINE Double2 operator*( const double & _Left, const Double2 & _Right ) {
    return Double2( _Left * _Right.X, _Left * _Right.Y );
}
AN_FORCEINLINE Double3 operator*( const double & _Left, const Double3 & _Right ) {
    return Double3( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z );
}
AN_FORCEINLINE Double4 operator*( const double & _Left, const Double4 & _Right ) {
    return Double4( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z, _Left * _Right.W );
}

AN_FORCEINLINE Double2 Double2::Lerp( const Double2 & _From, const Double2 & _To, const double & _Mix ) {
    return _From + _Mix * ( _To - _From );
}
AN_FORCEINLINE Double2 Double2::Lerp( const Double2 & _From, const Double2 & _To, const Double2 & _Mix ) {
    return _From + _Mix * ( _To - _From );
}
AN_FORCEINLINE Double Double2::Bilerp( const double & _A, const double & _B, const double & _C, const double & _D ) const {
    return _A * ( 1.0 - X ) * ( 1.0 - Y ) + _B * X * ( 1.0 - Y ) + _C * ( 1.0 - X ) * Y + _D * X * Y;
}
AN_FORCEINLINE Double2 Double2::Bilerp( const Double2 & _A, const Double2 & _B, const Double2 & _C, const Double2 & _D ) const {
    return _A * ( 1.0 - X ) * ( 1.0 - Y ) + _B * X * ( 1.0 - Y ) + _C * ( 1.0 - X ) * Y + _D * X * Y;
}
AN_FORCEINLINE Double3 Double2::Bilerp( const Double3 & _A, const Double3 & _B, const Double3 & _C, const Double3 & _D ) const {
    return _A * ( 1.0 - X ) * ( 1.0 - Y ) + _B * X * ( 1.0 - Y ) + _C * ( 1.0 - X ) * Y + _D * X * Y;
}
AN_FORCEINLINE Double4 Double2::Bilerp( const Double4 & _A, const Double4 & _B, const Double4 & _C, const Double4 & _D ) const {
    return _A * ( 1.0 - X ) * ( 1.0 - Y ) + _B * X * ( 1.0 - Y ) + _C * ( 1.0 - X ) * Y + _D * X * Y;
}

AN_FORCEINLINE Double3 Double3::Lerp( const Double3 & _From, const Double3 & _To, const double & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

AN_FORCEINLINE Double3 Double3::Lerp( const Double3 & _From, const Double3 & _To, const Double3 & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

AN_FORCEINLINE Double4 Double4::Lerp( const Double4 & _From, const Double4 & _To, const double & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

AN_FORCEINLINE Double4 Double4::Lerp( const Double4 & _From, const Double4 & _To, const Double4 & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

AN_FORCEINLINE Double2 Double2::Step( const double & _Edge ) const {
    return Double2( X < _Edge ? 0.0 : 1.0, Y < _Edge ? 0.0 : 1.0 );
}

AN_FORCEINLINE Double2 Double2::Step( const Double2 & _Edge ) const {
    return Double2( X < _Edge.X ? 0.0 : 1.0, Y < _Edge.Y ? 0.0 : 1.0 );
}

AN_FORCEINLINE Double2 Double2::SmoothStep( const double & _Edge0, const double & _Edge1 ) const {
    double Denom = 1.0 / ( _Edge1 - _Edge0 );
    const Double2 t = ( ( *this - _Edge0 ) * Denom ).Saturate();
    return t * t * ( ( -2.0 ) * t + 3.0 );
}

AN_FORCEINLINE Double2 Double2::SmoothStep( const Double2 & _Edge0, const Double2 & _Edge1 ) const {
    const Double2 t = ( ( *this - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
    return t * t * ( ( -2.0 ) * t + 3.0 );
}

AN_FORCEINLINE Double3 Double3::Step( const double & _Edge ) const {
    return Double3( X < _Edge ? 0.0 : 1.0, Y < _Edge ? 0.0 : 1.0, Z < _Edge ? 0.0 : 1.0 );
}

AN_FORCEINLINE Double3 Double3::Step( const Double3 & _Edge ) const {
    return Double3( X < _Edge.X ? 0.0 : 1.0, Y < _Edge.Y ? 0.0 : 1.0, Z < _Edge.Z ? 0.0 : 1.0 );
}

AN_FORCEINLINE Double3 Double3::SmoothStep( const double & _Edge0, const double & _Edge1 ) const {
    double Denom = 1.0 / ( _Edge1 - _Edge0 );
    const Double3 t = ( ( *this - _Edge0 ) * Denom ).Saturate();
    return t * t * ( ( -2.0 ) * t + 3.0 );
}

AN_FORCEINLINE Double3 Double3::SmoothStep( const Double3 & _Edge0, const Double3 & _Edge1 ) const {
    const Double3 t = ( ( *this - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
    return t * t * ( ( -2.0 ) * t + 3.0 );
}

AN_FORCEINLINE Double4 Double4::Step( const double & _Edge ) const {
    return Double4( X < _Edge ? 0.0 : 1.0, Y < _Edge ? 0.0 : 1.0, Z < _Edge ? 0.0 : 1.0, W < _Edge ? 0.0 : 1.0 );
}

AN_FORCEINLINE Double4 Double4::Step( const Double4 & _Edge ) const {
    return Double4( X < _Edge.X ? 0.0 : 1.0, Y < _Edge.Y ? 0.0 : 1.0, Z < _Edge.Z ? 0.0 : 1.0, W < _Edge.W ? 0.0 : 1.0 );
}

AN_FORCEINLINE Double4 Double4::SmoothStep( const double & _Edge0, const double & _Edge1 ) const {
    double Denom = 1.0 / ( _Edge1 - _Edge0 );
    const Double4 t = ( ( *this - _Edge0 ) * Denom ).Saturate();
    return t * t * ( ( -2.0 ) * t + 3.0 );
}

AN_FORCEINLINE Double4 Double4::SmoothStep( const Double4 & _Edge0, const Double4 & _Edge1 ) const {
    const Double4 t = ( ( *this - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
    return t * t * ( ( -2.0 ) * t + 3.0 );
}

namespace Math {

AN_FORCEINLINE Double Dot( const Double2 & _Left, const Double2 & _Right ) { return _Left.Dot( _Right ); }
AN_FORCEINLINE Double Dot( const Double3 & _Left, const Double3 & _Right ) { return _Left.Dot( _Right ); }
AN_FORCEINLINE Double Dot( const Double4 & _Left, const Double4 & _Right ) { return _Left.Dot( _Right ); }

AN_FORCEINLINE Double3 Cross( const Double3 & _Left, const Double3 & _Right ) { return _Left.Cross( _Right ); }

AN_FORCEINLINE double Degrees( const double & _Rad ) { return _Rad * _RAD2DEG_DBL; }
AN_FORCEINLINE double Radians( const double & _Deg ) { return _Deg * _DEG2RAD_DBL; }

AN_FORCEINLINE double RadSin( const double & _Rad ) { return sin( _Rad ); }
AN_FORCEINLINE double RadCos( const double & _Rad ) { return cos( _Rad ); }
AN_FORCEINLINE double DegSin( const double & _Deg ) { return sin( Radians( _Deg ) ); }
AN_FORCEINLINE double DegCos( const double & _Deg ) { return cos( Radians( _Deg ) ); }

AN_FORCEINLINE void RadSinCos( const double & _Rad, double & _Sin, double & _Cos ) {
    _Sin = sin( _Rad );
    _Cos = cos( _Rad );
}

AN_FORCEINLINE void DegSinCos( const double & _Deg, double & _Sin, double & _Cos ) {
    float Rad = Radians( _Deg );
    _Sin = sin( Rad );
    _Cos = cos( Rad );
}

AN_FORCEINLINE void RadSinCos( const Double & _Rad, Double & _Sin, Double & _Cos ) {
    _Sin = sin( _Rad );
    _Cos = cos( _Rad );
}

AN_FORCEINLINE void DegSinCos( const Double & _Deg, Double & _Sin, Double & _Cos ) {
    Double Rad = Radians( _Deg );
    _Sin = sin( Rad );
    _Cos = cos( Rad );
}

}


// Column-major matrix 2x2
class Double2x2 final {
public:
    Double2 Col0;
    Double2 Col1;

    Double2x2() = default;
    explicit Double2x2( const Double3x3 & _Value );
    explicit Double2x2( const Double3x4 & _Value );
    explicit Double2x2( const Double4x4 & _Value );
    constexpr Double2x2( const Double2 & _Col0, const Double2 & _Col1 ) : Col0( _Col0 ), Col1( _Col1 ) {}
    constexpr Double2x2( const double & _00, const double & _01,
                        const double & _10, const double & _11 )
        : Col0( _00, _01 ), Col1( _10, _11 ) {}
    constexpr explicit Double2x2( const double & _Diagonal ) : Col0( _Diagonal, 0 ), Col1( 0, _Diagonal ) {}
    constexpr explicit Double2x2( const Double2 & _Diagonal ) : Col0( _Diagonal.X.Value, 0 ), Col1( 0, _Diagonal.Y.Value ) {}

    Double * ToPtr() {
        return &Col0.X;
    }

    const Double * ToPtr() const {
        return &Col0.X;
    }

    Double2x2 & operator=( const Double2x2 & _Other ) {
        Col0 = _Other.Col0;
        Col1 = _Other.Col1;
        return *this;
    }

    Double2 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Double2 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Bool operator==( const Double2x2 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Double2x2 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Double2x2 & _Other ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 4 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Double2x2 & _Other, const Double & _Epsilon ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 4 ; i++ ) {
            if ( ( *pm1++ - *pm2++ ).Length() >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf() {
        double Temp = Col0.Y;
        Col0.Y = Col1.X;
        Col1.X = Temp;
    }

    Double2x2 Transposed() const {
        return Double2x2( Col0.X, Col1.X, Col0.Y, Col1.Y );
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Double2x2 Inversed() const {
        const double OneOverDeterminant = 1.0 / ( Col0[0] * Col1[1] - Col1[0] * Col0[1] );
        return Double2x2( Col1[1] * OneOverDeterminant,
                    -Col0[1] * OneOverDeterminant,
                    -Col1[0] * OneOverDeterminant,
                     Col0[0] * OneOverDeterminant );
    }

    double Determinant() const {
        return Col0[0] * Col1[1] - Col1[0] * Col0[1];
    }

    void Clear() {
        ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        Col0.Y = Col1.X = 0;
        Col0.X = Col1.Y = 1;
    }

    static Double2x2 Scale( const Double2 & _Scale  ) {
        return Double2x2( _Scale );
    }

    Double2x2 Scaled( const Double2 & _Scale ) const {
        return Double2x2( Col0 * _Scale[0], Col1 * _Scale[1] );
    }

    // Return rotation around Z axis
    static Double2x2 Rotation( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double2x2( c, s,
                        -s, c );
    }

    Double2x2 operator*( const double & _Value ) const {
        return Double2x2( Col0 * _Value,
                         Col1 * _Value );
    }

    void operator*=( const double & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
    }

    Double2x2 operator/( const double & _Value ) const {
        const double OneOverValue = 1.0 / _Value;
        return Double2x2( Col0 * OneOverValue,
                         Col1 * OneOverValue );
    }

    void operator/=( const double & _Value ) {
        const double OneOverValue = 1.0 / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
    }

    Double2 operator*( const Double2 & _Vec ) const {
        return Double2( Col0[0] * _Vec.X + Col1[0] * _Vec.Y,
                       Col0[1] * _Vec.X + Col1[1] * _Vec.Y );
    }

    Double2x2 operator*( const Double2x2 & _Mat ) const {
        const double & L00 = Col0[0];
        const double & L01 = Col0[1];
        const double & L10 = Col1[0];
        const double & L11 = Col1[1];
        const double & R00 = _Mat[0][0];
        const double & R01 = _Mat[0][1];
        const double & R10 = _Mat[1][0];
        const double & R11 = _Mat[1][1];
        return Double2x2( L00 * R00 + L10 * R01,
                         L01 * R00 + L11 * R01,
                         L00 * R10 + L10 * R11,
                         L01 * R10 + L11 * R11 );
    }

    void operator*=( const Double2x2 & _Mat ) {
        //*this = *this * _Mat;

        const double L00 = Col0[0];
        const double L01 = Col0[1];
        const double L10 = Col1[0];
        const double L11 = Col1[1];
        const double & R00 = _Mat[0][0];
        const double & R01 = _Mat[0][1];
        const double & R10 = _Mat[1][0];
        const double & R11 = _Mat[1][1];
        Col0[0] = L00 * R00 + L10 * R01;
        Col0[1] = L01 * R00 + L11 * R01;
        Col1[0] = L00 * R10 + L10 * R11;
        Col1[1] = L01 * R10 + L11 * R11;
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
    }

    // Static methods

    static const Double2x2 & Identity() {
        static constexpr Double2x2 IdentityMat( 1 );
        return IdentityMat;
    }

};

// Column-major matrix 3x3
class Double3x3 final {
public:
    Double3 Col0;
    Double3 Col1;
    Double3 Col2;

    Double3x3() = default;
    explicit Double3x3( const Double2x2 & _Value );
    explicit Double3x3( const Double3x4 & _Value );
    explicit Double3x3( const Double4x4 & _Value );
    constexpr Double3x3( const Double3 & _Col0, const Double3 & _Col1, const Double3 & _Col2 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Double3x3( const double & _00, const double & _01, const double & _02,
          const double & _10, const double & _11, const double & _12,
          const double & _20, const double & _21, const double & _22 )
        : Col0( _00, _01, _02 ), Col1( _10, _11, _12 ), Col2( _20, _21, _22 ) {}
    constexpr explicit Double3x3( const double & _Diagonal ) : Col0( _Diagonal, 0, 0 ), Col1( 0, _Diagonal, 0 ), Col2( 0, 0, _Diagonal ) {}
    constexpr explicit Double3x3( const Double3 & _Diagonal ) : Col0( _Diagonal.X.Value, 0, 0 ), Col1( 0, _Diagonal.Y.Value, 0 ), Col2( 0, 0, _Diagonal.Z.Value ) {}

    Double * ToPtr() {
        return &Col0.X;
    }

    const Double * ToPtr() const {
        return &Col0.X;
    }

    Double3x3 & operator=( const Double3x3 & _Other ) {
        //Col0 = _Other.Col0;
        //Col1 = _Other.Col1;
        //Col2 = _Other.Col2;
        memcpy( this, &_Other, sizeof( *this ) );
        return *this;
    }

    Double3 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Double3 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Bool operator==( const Double3x3 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Double3x3 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Double3x3 & _Other ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 9 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Double3x3 & _Other, const Double & _Epsilon ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 9 ; i++ ) {
            if ( ( *pm1++ - *pm2++ ).Abs() >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf() {
        double Temp = Col0.Y;
        Col0.Y = Col1.X;
        Col1.X = Temp;
        Temp = Col0.Z;
        Col0.Z = Col2.X;
        Col2.X = Temp;
        Temp = Col1.Z;
        Col1.Z = Col2.Y;
        Col2.Y = Temp;
    }

    Double3x3 Transposed() const {
        return Double3x3( Col0.X, Col1.X, Col2.X, Col0.Y, Col1.Y, Col2.Y, Col0.Z, Col1.Z, Col2.Z );
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Double3x3 Inversed() const {
        const Double3x3 & m = *this;
        const double A = m[1][1] * m[2][2] - m[2][1] * m[1][2];
        const double B = m[0][1] * m[2][2] - m[2][1] * m[0][2];
        const double C = m[0][1] * m[1][2] - m[1][1] * m[0][2];
        const double OneOverDeterminant = 1.0 / ( m[0][0] * A - m[1][0] * B + m[2][0] * C );

        Double3x3 Inversed;
        Inversed[0][0] =   A * OneOverDeterminant;
        Inversed[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]) * OneOverDeterminant;
        Inversed[2][0] =   (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
        Inversed[0][1] = - B * OneOverDeterminant;
        Inversed[1][1] =   (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * OneOverDeterminant;
        Inversed[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;
        Inversed[0][2] =   C * OneOverDeterminant;
        Inversed[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]) * OneOverDeterminant;
        Inversed[2][2] =   (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * OneOverDeterminant;
        return Inversed;
    }

    double Determinant() const {
        return Col0[0] * ( Col1[1] * Col2[2] - Col2[1] * Col1[2] ) -
               Col1[0] * ( Col0[1] * Col2[2] - Col2[1] * Col0[2] ) +
               Col2[0] * ( Col0[1] * Col1[2] - Col1[1] * Col0[2] );
    }

    void Clear() {
        ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        Clear();
        Col0.X = Col1.Y = Col2.Z = 1;
    }

    static Double3x3 Scale( const Double3 & _Scale  ) {
        return Double3x3( _Scale );
    }

    Double3x3 Scaled( const Double3 & _Scale ) const {
        return Double3x3( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2] );
    }

    // Return rotation around normalized axis
    static Double3x3 RotationAroundNormal( const double & _AngleRad, const Double3 & _Normal ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        const Double3 Temp = ( 1.0 - c ) * _Normal;
        const Double3 Temp2 = s * _Normal;
        return Double3x3( c + Temp[0] * _Normal[0],                Temp[0] * _Normal[1] + Temp2[2],     Temp[0] * _Normal[2] - Temp2[1],
                             Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1],                Temp[1] * _Normal[2] + Temp2[0],
                             Temp[2] * _Normal[0] + Temp2[1],     Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2] );
    }

    // Return rotation around normalized axis
    Double3x3 RotateAroundNormal( const double & _AngleRad, const Double3 & _Normal ) const {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        Double3 Temp = ( 1.0 - c ) * _Normal;
        Double3 Temp2 = s * _Normal;
        return Double3x3( Col0 * (c + Temp[0] * _Normal[0])            + Col1 * (    Temp[0] * _Normal[1] + Temp2[2]) + Col2 * (    Temp[0] * _Normal[2] - Temp2[1]),
                         Col0 * (    Temp[1] * _Normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * _Normal[1])            + Col2 * (    Temp[1] * _Normal[2] + Temp2[0]),
                         Col0 * (    Temp[2] * _Normal[0] + Temp2[1]) + Col1 * (    Temp[2] * _Normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * _Normal[2]) );
    }

    // Return rotation around unnormalized vector
    static Double3x3 RotationAroundVector( const double & _AngleRad, const Double3 & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Double3x3 RotateAroundVector( const double & _AngleRad, const Double3 & _Vector ) const {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Double3x3 RotationX( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double3x3( 1, 0, 0,
                         0, c, s,
                         0,-s, c );
    }

    // Return rotation around Y axis
    static Double3x3 RotationY( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double3x3( c, 0,-s,
                         0, 1, 0,
                         s, 0, c );
    }

    // Return rotation around Z axis
    static Double3x3 RotationZ( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double3x3( c, s, 0,
                        -s, c, 0,
                         0, 0, 1 );
    }

    Double3x3 operator*( const double & _Value ) const {
        return Double3x3( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( const double & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Double3x3 operator/( const double & _Value ) const {
        const double OneOverValue = 1.0 / _Value;
        return Double3x3( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue );
    }

    void operator/=( const double & _Value ) {
        const double OneOverValue = 1.0 / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    Double3 operator*( const Double3 & _Vec ) const {
        return Double3( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z,
                       Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z,
                       Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z );
    }

    Double3x3 operator*( const Double3x3 & _Mat ) const {
        const double & L00 = Col0[0];
        const double & L01 = Col0[1];
        const double & L02 = Col0[2];
        const double & L10 = Col1[0];
        const double & L11 = Col1[1];
        const double & L12 = Col1[2];
        const double & L20 = Col2[0];
        const double & L21 = Col2[1];
        const double & L22 = Col2[2];
        const double & R00 = _Mat[0][0];
        const double & R01 = _Mat[0][1];
        const double & R02 = _Mat[0][2];
        const double & R10 = _Mat[1][0];
        const double & R11 = _Mat[1][1];
        const double & R12 = _Mat[1][2];
        const double & R20 = _Mat[2][0];
        const double & R21 = _Mat[2][1];
        const double & R22 = _Mat[2][2];
        return Double3x3( L00 * R00 + L10 * R01 + L20 * R02,
                         L01 * R00 + L11 * R01 + L21 * R02,
                         L02 * R00 + L12 * R01 + L22 * R02,
                         L00 * R10 + L10 * R11 + L20 * R12,
                         L01 * R10 + L11 * R11 + L21 * R12,
                         L02 * R10 + L12 * R11 + L22 * R12,
                         L00 * R20 + L10 * R21 + L20 * R22,
                         L01 * R20 + L11 * R21 + L21 * R22,
                         L02 * R20 + L12 * R21 + L22 * R22 );
    }

    void operator*=( const Double3x3 & _Mat ) {
        //*this = *this * _Mat;

        const double L00 = Col0[0];
        const double L01 = Col0[1];
        const double L02 = Col0[2];
        const double L10 = Col1[0];
        const double L11 = Col1[1];
        const double L12 = Col1[2];
        const double L20 = Col2[0];
        const double L21 = Col2[1];
        const double L22 = Col2[2];
        const double & R00 = _Mat[0][0];
        const double & R01 = _Mat[0][1];
        const double & R02 = _Mat[0][2];
        const double & R10 = _Mat[1][0];
        const double & R11 = _Mat[1][1];
        const double & R12 = _Mat[1][2];
        const double & R20 = _Mat[2][0];
        const double & R21 = _Mat[2][1];
        const double & R22 = _Mat[2][2];
        Col0[0] = L00 * R00 + L10 * R01 + L20 * R02;
        Col0[1] = L01 * R00 + L11 * R01 + L21 * R02;
        Col0[2] = L02 * R00 + L12 * R01 + L22 * R02;
        Col1[0] = L00 * R10 + L10 * R11 + L20 * R12;
        Col1[1] = L01 * R10 + L11 * R11 + L21 * R12;
        Col1[2] = L02 * R10 + L12 * R11 + L22 * R12;
        Col2[0] = L00 * R20 + L10 * R21 + L20 * R22;
        Col2[1] = L01 * R20 + L11 * R21 + L21 * R22;
        Col2[2] = L02 * R20 + L12 * R21 + L22 * R22;
    }

    AN_FORCEINLINE Double3x3 ViewInverseFast() const {
        return Transposed();
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
    }

    // Static methods

    static const Double3x3 & Identity() {
        static constexpr Double3x3 IdentityMat( 1 );
        return IdentityMat;
    }

};

// Column-major matrix 4x4
class Double4x4 final {
public:
    Double4 Col0;
    Double4 Col1;
    Double4 Col2;
    Double4 Col3;

    Double4x4() = default;
    explicit Double4x4( const Double2x2 & _Value );
    explicit Double4x4( const Double3x3 & _Value );
    explicit Double4x4( const Double3x4 & _Value );
    constexpr Double4x4( const Double4 & _Col0, const Double4 & _Col1, const Double4 & _Col2, const Double4 & _Col3 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ), Col3( _Col3 ) {}
    constexpr Double4x4( const double & _00, const double & _01, const double & _02, const double & _03,
                        const double & _10, const double & _11, const double & _12, const double & _13,
                        const double & _20, const double & _21, const double & _22, const double & _23,
                        const double & _30, const double & _31, const double & _32, const double & _33 )
        : Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ), Col3( _30, _31, _32, _33 ) {}
    constexpr explicit Double4x4( const double & _Diagonal ) : Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ), Col3( 0, 0, 0, _Diagonal ) {}
    constexpr explicit Double4x4( const Double4 & _Diagonal ) : Col0( _Diagonal.X.Value, 0, 0, 0 ), Col1( 0, _Diagonal.Y.Value, 0, 0 ), Col2( 0, 0, _Diagonal.Z.Value, 0 ), Col3( 0, 0, 0, _Diagonal.W.Value ) {}

    Double * ToPtr() {
        return &Col0.X;
    }

    const Double * ToPtr() const {
        return &Col0.X;
    }

    Double4x4 & operator=( const Double4x4 & _Other ) {
        //Col0 = _Other.Col0;
        //Col1 = _Other.Col1;
        //Col2 = _Other.Col2;
        //Col3 = _Other.Col3;
        memcpy( this, &_Other, sizeof( *this ) );
        return *this;
    }

    Double4 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Double4 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Bool operator==( const Double4x4 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Double4x4 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Double4x4 & _Other ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 16 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Double4x4 & _Other, const Double & _Epsilon ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 16 ; i++ ) {
            if ( ( *pm1++ - *pm2++ ).Abs() >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf() {
        double Temp = Col0.Y;
        Col0.Y = Col1.X;
        Col1.X = Temp;

        Temp = Col0.Z;
        Col0.Z = Col2.X;
        Col2.X = Temp;

        Temp = Col1.Z;
        Col1.Z = Col2.Y;
        Col2.Y = Temp;

        Temp = Col0.W;
        Col0.W = Col3.X;
        Col3.X = Temp;

        Temp = Col1.W;
        Col1.W = Col3.Y;
        Col3.Y = Temp;

        Temp = Col2.W;
        Col2.W = Col3.Z;
        Col3.Z = Temp;

        //swap( Col0.Y, Col1.X );
        //swap( Col0.Z, Col2.X );
        //swap( Col1.Z, Col2.Y );
        //swap( Col0.W, Col3.X );
        //swap( Col1.W, Col3.Y );
        //swap( Col2.W, Col3.Z );
    }

    Double4x4 Transposed() const {
        return Double4x4( Col0.X, Col1.X, Col2.X, Col3.X,
                         Col0.Y, Col1.Y, Col2.Y, Col3.Y,
                         Col0.Z, Col1.Z, Col2.Z, Col3.Z,
                         Col0.W, Col1.W, Col2.W, Col3.W );
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Double4x4 Inversed() const {
        const Double4x4 & m = *this;

        const double Coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        const double Coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        const double Coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

        const double Coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        const double Coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
        const double Coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

        const double Coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        const double Coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
        const double Coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

        const double Coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        const double Coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
        const double Coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

        const double Coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        const double Coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
        const double Coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

        const double Coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        const double Coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
        const double Coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

        const Double4 Fac0(Coef00, Coef00, Coef02, Coef03);
        const Double4 Fac1(Coef04, Coef04, Coef06, Coef07);
        const Double4 Fac2(Coef08, Coef08, Coef10, Coef11);
        const Double4 Fac3(Coef12, Coef12, Coef14, Coef15);
        const Double4 Fac4(Coef16, Coef16, Coef18, Coef19);
        const Double4 Fac5(Coef20, Coef20, Coef22, Coef23);

        const Double4 Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
        const Double4 Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
        const Double4 Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
        const Double4 Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

        const Double4 Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
        const Double4 Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
        const Double4 Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
        const Double4 Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

        const Double4 SignA(+1, -1, +1, -1);
        const Double4 SignB(-1, +1, -1, +1);
        Double4x4 Inversed(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

        const Double4 Row0(Inversed[0][0], Inversed[1][0], Inversed[2][0], Inversed[3][0]);

        const Double4 Dot0(m[0] * Row0);
        const double Dot1 = (Dot0.X + Dot0.Y) + (Dot0.Z + Dot0.W);

        const double OneOverDeterminant = 1.0 / Dot1;

        return Inversed * OneOverDeterminant;
    }

    double Determinant() const {
        const double SubFactor00 = Col2[2] * Col3[3] - Col3[2] * Col2[3];
        const double SubFactor01 = Col2[1] * Col3[3] - Col3[1] * Col2[3];
        const double SubFactor02 = Col2[1] * Col3[2] - Col3[1] * Col2[2];
        const double SubFactor03 = Col2[0] * Col3[3] - Col3[0] * Col2[3];
        const double SubFactor04 = Col2[0] * Col3[2] - Col3[0] * Col2[2];
        const double SubFactor05 = Col2[0] * Col3[1] - Col3[0] * Col2[1];

        const Double4 DetCof(
                + (Col1[1] * SubFactor00 - Col1[2] * SubFactor01 + Col1[3] * SubFactor02),
                - (Col1[0] * SubFactor00 - Col1[2] * SubFactor03 + Col1[3] * SubFactor04),
                + (Col1[0] * SubFactor01 - Col1[1] * SubFactor03 + Col1[3] * SubFactor05),
                - (Col1[0] * SubFactor02 - Col1[1] * SubFactor04 + Col1[2] * SubFactor05));

        return Col0[0] * DetCof[0] + Col0[1] * DetCof[1] +
               Col0[2] * DetCof[2] + Col0[3] * DetCof[3];
    }

    void Clear() {
        ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        Clear();
        Col0.X = Col1.Y = Col2.Z = Col3.W = 1;
    }

    static Double4x4 Translation( const Double3 & _Vec ) {
        return Double4x4( Double4( 1,0,0,0 ),
                         Double4( 0,1,0,0 ),
                         Double4( 0,0,1,0 ),
                         Double4( _Vec[0], _Vec[1], _Vec[2], 1 ) );
    }

    Double4x4 Translated( const Double3 & _Vec ) const {
        return Double4x4( Col0, Col1, Col2, Col0 * _Vec[0] + Col1 * _Vec[1] + Col2 * _Vec[2] + Col3 );
    }

    static Double4x4 Scale( const Double3 & _Scale  ) {
        return Double4x4( Double4( _Scale[0],0,0,0 ),
                         Double4( 0,_Scale[1],0,0 ),
                         Double4( 0,0,_Scale[2],0 ),
                         Double4( 0,0,0,1 ) );
    }

    Double4x4 Scaled( const Double3 & _Scale ) const {
        return Double4x4( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2], Col3 );
    }


    // Return rotation around normalized axis
    static Double4x4 RotationAroundNormal( const double & _AngleRad, const Double3 & _Normal ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        const Double3 Temp = ( 1.0 - c ) * _Normal;
        const Double3 Temp2 = s * _Normal;
        return Double4x4( c + Temp[0] * _Normal[0],                Temp[0] * _Normal[1] + Temp2[2],     Temp[0] * _Normal[2] - Temp2[1], 0,
                             Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1],                Temp[1] * _Normal[2] + Temp2[0], 0,
                             Temp[2] * _Normal[0] + Temp2[1],     Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2],            0,
                         0,0,0,1 );
    }

    // Return rotation around normalized axis
    Double4x4 RotateAroundNormal( const double & _AngleRad, const Double3 & _Normal ) const {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        Double3 Temp = ( 1.0 - c ) * _Normal;
        Double3 Temp2 = s * _Normal;
        return Double4x4( Col0 * (c + Temp[0] * _Normal[0])            + Col1 * (    Temp[0] * _Normal[1] + Temp2[2]) + Col2 * (    Temp[0] * _Normal[2] - Temp2[1]),
                         Col0 * (    Temp[1] * _Normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * _Normal[1])            + Col2 * (    Temp[1] * _Normal[2] + Temp2[0]),
                         Col0 * (    Temp[2] * _Normal[0] + Temp2[1]) + Col1 * (    Temp[2] * _Normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * _Normal[2]),
                         Col3 );
    }

    // Return rotation around unnormalized vector
    static Double4x4 RotationAroundVector( const double & _AngleRad, const Double3 & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Double4x4 RotateAroundVector( const double & _AngleRad, const Double3 & _Vector ) const {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Double4x4 RotationX( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double4x4( 1, 0, 0, 0,
                         0, c, s, 0,
                         0,-s, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Y axis
    static Double4x4 RotationY( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double4x4( c, 0,-s, 0,
                         0, 1, 0, 0,
                         s, 0, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Z axis
    static Double4x4 RotationZ( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double4x4( c, s, 0, 0,
                        -s, c, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1 );
    }

    Double4 operator*( const Double4 & _Vec ) const {
        return Double4( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z + Col3[0] * _Vec.W,
                       Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z + Col3[1] * _Vec.W,
                       Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z + Col3[2] * _Vec.W,
                       Col0[3] * _Vec.X + Col1[3] * _Vec.Y + Col2[3] * _Vec.Z + Col3[3] * _Vec.W );
    }

    Double4x4 operator*( const double & _Value ) const {
        return Double4x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value,
                         Col3 * _Value );
    }

    void operator*=( const double & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
        Col3 *= _Value;
    }

    Double4x4 operator/( const double & _Value ) const {
        const double OneOverValue = 1.0 / _Value;
        return Double4x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue,
                         Col3 * OneOverValue );
    }

    void operator/=( const double & _Value ) {
        const double OneOverValue = 1.0 / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
        Col3 *= OneOverValue;
    }

    Double4x4 operator*( const Double4x4 & _Mat ) const {
        const double & L00 = Col0[0];
        const double & L01 = Col0[1];
        const double & L02 = Col0[2];
        const double & L03 = Col0[3];
        const double & L10 = Col1[0];
        const double & L11 = Col1[1];
        const double & L12 = Col1[2];
        const double & L13 = Col1[3];
        const double & L20 = Col2[0];
        const double & L21 = Col2[1];
        const double & L22 = Col2[2];
        const double & L23 = Col2[3];
        const double & L30 = Col3[0];
        const double & L31 = Col3[1];
        const double & L32 = Col3[2];
        const double & L33 = Col3[3];
        const double & R00 = _Mat[0][0];
        const double & R01 = _Mat[0][1];
        const double & R02 = _Mat[0][2];
        const double & R03 = _Mat[0][3];
        const double & R10 = _Mat[1][0];
        const double & R11 = _Mat[1][1];
        const double & R12 = _Mat[1][2];
        const double & R13 = _Mat[1][3];
        const double & R20 = _Mat[2][0];
        const double & R21 = _Mat[2][1];
        const double & R22 = _Mat[2][2];
        const double & R23 = _Mat[2][3];
        const double & R30 = _Mat[3][0];
        const double & R31 = _Mat[3][1];
        const double & R32 = _Mat[3][2];
        const double & R33 = _Mat[3][3];
        return Double4x4( L00 * R00 + L10 * R01 + L20 * R02 + L30 * R03,
                         L01 * R00 + L11 * R01 + L21 * R02 + L31 * R03,
                         L02 * R00 + L12 * R01 + L22 * R02 + L32 * R03,
                         L03 * R00 + L13 * R01 + L23 * R02 + L33 * R03,
                         L00 * R10 + L10 * R11 + L20 * R12 + L30 * R13,
                         L01 * R10 + L11 * R11 + L21 * R12 + L31 * R13,
                         L02 * R10 + L12 * R11 + L22 * R12 + L32 * R13,
                         L03 * R10 + L13 * R11 + L23 * R12 + L33 * R13,
                         L00 * R20 + L10 * R21 + L20 * R22 + L30 * R23,
                         L01 * R20 + L11 * R21 + L21 * R22 + L31 * R23,
                         L02 * R20 + L12 * R21 + L22 * R22 + L32 * R23,
                         L03 * R20 + L13 * R21 + L23 * R22 + L33 * R23,
                         L00 * R30 + L10 * R31 + L20 * R32 + L30 * R33,
                         L01 * R30 + L11 * R31 + L21 * R32 + L31 * R33,
                         L02 * R30 + L12 * R31 + L22 * R32 + L32 * R33,
                         L03 * R30 + L13 * R31 + L23 * R32 + L33 * R33 );
    }

    void operator*=( const Double4x4 & _Mat ) {
        //*this = *this * _Mat;

        const double L00 = Col0[0];
        const double L01 = Col0[1];
        const double L02 = Col0[2];
        const double L03 = Col0[3];
        const double L10 = Col1[0];
        const double L11 = Col1[1];
        const double L12 = Col1[2];
        const double L13 = Col1[3];
        const double L20 = Col2[0];
        const double L21 = Col2[1];
        const double L22 = Col2[2];
        const double L23 = Col2[3];
        const double L30 = Col3[0];
        const double L31 = Col3[1];
        const double L32 = Col3[2];
        const double L33 = Col3[3];
        const double & R00 = _Mat[0][0];
        const double & R01 = _Mat[0][1];
        const double & R02 = _Mat[0][2];
        const double & R03 = _Mat[0][3];
        const double & R10 = _Mat[1][0];
        const double & R11 = _Mat[1][1];
        const double & R12 = _Mat[1][2];
        const double & R13 = _Mat[1][3];
        const double & R20 = _Mat[2][0];
        const double & R21 = _Mat[2][1];
        const double & R22 = _Mat[2][2];
        const double & R23 = _Mat[2][3];
        const double & R30 = _Mat[3][0];
        const double & R31 = _Mat[3][1];
        const double & R32 = _Mat[3][2];
        const double & R33 = _Mat[3][3];
        Col0[0] = L00 * R00 + L10 * R01 + L20 * R02 + L30 * R03,
        Col0[1] = L01 * R00 + L11 * R01 + L21 * R02 + L31 * R03,
        Col0[2] = L02 * R00 + L12 * R01 + L22 * R02 + L32 * R03,
        Col0[3] = L03 * R00 + L13 * R01 + L23 * R02 + L33 * R03,
        Col1[0] = L00 * R10 + L10 * R11 + L20 * R12 + L30 * R13,
        Col1[1] = L01 * R10 + L11 * R11 + L21 * R12 + L31 * R13,
        Col1[2] = L02 * R10 + L12 * R11 + L22 * R12 + L32 * R13,
        Col1[3] = L03 * R10 + L13 * R11 + L23 * R12 + L33 * R13,
        Col2[0] = L00 * R20 + L10 * R21 + L20 * R22 + L30 * R23,
        Col2[1] = L01 * R20 + L11 * R21 + L21 * R22 + L31 * R23,
        Col2[2] = L02 * R20 + L12 * R21 + L22 * R22 + L32 * R23,
        Col2[3] = L03 * R20 + L13 * R21 + L23 * R22 + L33 * R23,
        Col3[0] = L00 * R30 + L10 * R31 + L20 * R32 + L30 * R33,
        Col3[1] = L01 * R30 + L11 * R31 + L21 * R32 + L31 * R33,
        Col3[2] = L02 * R30 + L12 * R31 + L22 * R32 + L32 * R33,
        Col3[3] = L03 * R30 + L13 * R31 + L23 * R32 + L33 * R33;
    }

    Double4x4 operator*( const Double3x4 & _Mat ) const;

    void operator*=( const Double3x4 & _Mat );

    Double4x4 ViewInverseFast() const {
        Double4x4 Inversed;

        Double * DstPtr = Inversed.ToPtr();
        const Double * SrcPtr = ToPtr();

        DstPtr[0] = SrcPtr[0];
        DstPtr[1] = SrcPtr[4];
        DstPtr[2] = SrcPtr[8];
        DstPtr[3] = 0;

        DstPtr[4] = SrcPtr[1];
        DstPtr[5] = SrcPtr[5];
        DstPtr[6] = SrcPtr[9];
        DstPtr[7] = 0;

        DstPtr[8] = SrcPtr[2];
        DstPtr[9] = SrcPtr[6];
        DstPtr[10] = SrcPtr[10];
        DstPtr[11] = 0;

        DstPtr[12] = - (DstPtr[0] * SrcPtr[12] + DstPtr[4] * SrcPtr[13] + DstPtr[8] * SrcPtr[14]);
        DstPtr[13] = - (DstPtr[1] * SrcPtr[12] + DstPtr[5] * SrcPtr[13] + DstPtr[9] * SrcPtr[14]);
        DstPtr[14] = - (DstPtr[2] * SrcPtr[12] + DstPtr[6] * SrcPtr[13] + DstPtr[10] * SrcPtr[14]);
        DstPtr[15] = 1;

        return Inversed;
    }

    AN_FORCEINLINE Double4x4 PerspectiveProjectionInverseFast() const {
        Double4x4 Inversed;

        // TODO: check correctness for all perspective projections

        Double * DstPtr = Inversed.ToPtr();
        const Double * SrcPtr = ToPtr();

        DstPtr[0] = 1.0 / SrcPtr[0];
        DstPtr[1] = 0;
        DstPtr[2] = 0;
        DstPtr[3] = 0;

        DstPtr[4] = 0;
        DstPtr[5] = 1.0 / SrcPtr[5];
        DstPtr[6] = 0;
        DstPtr[7] = 0;

        DstPtr[8] = 0;
        DstPtr[9] = 0;
        DstPtr[10] = 0;
        DstPtr[11] = 1.0 / SrcPtr[14];

        DstPtr[12] = 0;
        DstPtr[13] = 0;
        DstPtr[14] = 1.0 / SrcPtr[11];
        DstPtr[15] = - SrcPtr[10] / (SrcPtr[11] * SrcPtr[14]);

        return Inversed;
    }

    AN_FORCEINLINE Double4x4 OrthoProjectionInverseFast() const {
        // TODO: ...
        return Inversed();
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " " + Col3.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " " + Col3.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
        Col3.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
        Col3.Read( _Stream );
    }

    // Static methods

    static const Double4x4 & Identity() {
        static constexpr Double4x4 IdentityMat( 1 );
        return IdentityMat;
    }

    // Conversion from standard projection matrix to clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE const Double4x4 & ClipControl_UpperLeft_ZeroToOne() {
        static constexpr double ClipTransform[] = {
            1,        0,        0,        0,
            0,       -1,        0,        0,
            0,        0,      0.5f,       0,
            0,        0,      0.5f,       1
        };

        return *reinterpret_cast< const Double4x4 * >( &ClipTransform[0] );

        // Same
        //return Double4x4::Identity().Scaled(Double3(1.0,1.0,0.5f)).Translated(Double3(0,0,0.5)).Scaled(Double3(1,-1,1));
    }

    // Standard OpenGL ortho projection for 2D
    static AN_FORCEINLINE Double4x4 Ortho2D( const double & _Left, const double & _Right, const double & _Bottom, const double & _Top ) {
        const double InvX = 1.0 / (_Right - _Left);
        const double InvY = 1.0 / (_Top - _Bottom);
        const double tx = -(_Right + _Left) * InvX;
        const double ty = -(_Top + _Bottom) * InvY;
        return Double4x4( 2 * InvX, 0,        0, 0,
                         0,        2 * InvY, 0, 0,
                         0,        0,       -2, 0,
                         tx,       ty,      -1, 1 );
    }

    // OpenGL ortho projection for 2D with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Double4x4 Ortho2DCC( const double & _Left, const double & _Right, const double & _Bottom, const double & _Top ) {
        return ClipControl_UpperLeft_ZeroToOne() * Ortho2D( _Left, _Right, _Bottom, _Top );
    }

    // Standard OpenGL ortho projection
    static AN_FORCEINLINE Double4x4 Ortho( const double & _Left, const double & _Right, const double & _Bottom, const double & _Top, const double & _ZNear, const double & _ZFar ) {
        const double InvX = 1.0 / (_Right - _Left);
        const double InvY = 1.0 / (_Top - _Bottom);
        const double InvZ = 1.0 / (_ZFar - _ZNear);
        const double tx = -(_Right + _Left) * InvX;
        const double ty = -(_Top + _Bottom) * InvY;
        const double tz = -(_ZFar + _ZNear) * InvZ;
        return Double4x4( 2 * InvX,0,        0,         0,
                         0,       2 * InvY, 0,         0,
                         0,       0,        -2 * InvZ, 0,
                         tx,      ty,       tz,        1 );
    }

    // OpenGL ortho projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Double4x4 OrthoCC( const double & _Left, const double & _Right, const double & _Bottom, const double & _Top, const double & _ZNear, const double & _ZFar ) {
        const double InvX = 1.0/(_Right - _Left);
        const double InvY = 1.0/(_Top - _Bottom);
        const double InvZ = 1.0/(_ZFar - _ZNear);
        const double tx = -(_Right + _Left) * InvX;
        const double ty = -(_Top + _Bottom) * InvY;
        const double tz = -(_ZFar + _ZNear) * InvZ;
        return Double4x4( 2 * InvX,  0,         0,              0,
                         0,         -2 * InvY, 0,              0,
                         0,         0,         -InvZ,          0,
                         tx,       -ty,        tz * 0.5 + 0.5, 1 );
        // Same
        // Transform according to clip control
        //return ClipControl_UpperLeft_ZeroToOne() * Ortho( _Left, _Right, _Bottom, _Top, _ZNear, _ZFar );
    }

    // Reversed-depth OpenGL ortho projection
    static AN_FORCEINLINE Double4x4 OrthoRev( const double & _Left, const double & _Right, const double & _Bottom, const double & _Top, const double & _ZNear, const double & _ZFar ) {
        const double InvX = 1.0 / (_Right - _Left);
        const double InvY = 1.0 / (_Top - _Bottom);
        const double InvZ = 1.0 / (_ZNear - _ZFar);
        const double tx = -(_Right + _Left) * InvX;
        const double ty = -(_Top + _Bottom) * InvY;
        const double tz = -(_ZNear + _ZFar) * InvZ;
        return Double4x4( 2 * InvX, 0,        0,         0,
                         0,        2 * InvY, 0,         0,
                         0,        0,        -2 * InvZ, 0,
                         tx,       ty,       tz,        1 );
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL ortho projection
    static AN_FORCEINLINE Double4x4 OrthoRevCC( const double & _Left, const double & _Right, const double & _Bottom, const double & _Top, const double & _ZNear, const double & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * OrthoRev( _Left, _Right, _Bottom, _Top, _ZNear, _ZFar );
    }

    // Standard OpenGL perspective projection
    static AN_FORCEINLINE Double4x4 Perspective( const double & _FovXRad, const double & _Width, const double & _Height, const double & _ZNear, const double & _ZFar ) {
        const double TanHalfFovX = tan( _FovXRad * 0.5f );
        const double HalfFovY = (double)(atan2( _Height, _Width / TanHalfFovX ) );
        const double TanHalfFovY = tan( HalfFovY );
        return Double4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZFar + _ZNear) / ( _ZNear - _ZFar ),  -1,
                         0,                0,               2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    static AN_FORCEINLINE Double4x4 Perspective( const double & _FovXRad, const double & _FovYRad, const double & _ZNear, const double & _ZFar ) {
        const double TanHalfFovX = tan( _FovXRad * 0.5f );
        const double TanHalfFovY = tan( _FovYRad * 0.5f );
        return Double4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZFar + _ZNear) / ( _ZNear - _ZFar ),  -1,
                         0,                0,               2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    // OpenGL perspective projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Double4x4 PerspectiveCC( const double & _FovXRad, const double & _Width, const double & _Height, const double & _ZNear, const double & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _Width, _Height, _ZNear, _ZFar );
    }

    static AN_FORCEINLINE Double4x4 PerspectiveCC( const double & _FovXRad, const double & _FovYRad, const double & _ZNear, const double & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _FovYRad, _ZNear, _ZFar );
    }

    // Reversed-depth OpenGL perspective projection
    static AN_FORCEINLINE Double4x4 PerspectiveRev( const double & _FovXRad, const double & _Width, const double & _Height, const double & _ZNear, const double & _ZFar ) {
        const double TanHalfFovX = tan( _FovXRad * 0.5f );
        const double HalfFovY = (double)(atan2( _Height, _Width / TanHalfFovX ) );
        const double TanHalfFovY = tan( HalfFovY );
        return Double4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZNear + _ZFar) / ( _ZFar - _ZNear ),  -1,
                         0,                0,               2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    static AN_FORCEINLINE Double4x4 PerspectiveRev( const double & _FovXRad, const double & _FovYRad, const double & _ZNear, const double & _ZFar ) {
        const double TanHalfFovX = tan( _FovXRad * 0.5f );
        const double TanHalfFovY = tan( _FovYRad * 0.5f );
        return Double4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZNear + _ZFar) / ( _ZFar - _ZNear ),  -1,
                         0,                0,               2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL perspective projection
    static AN_FORCEINLINE Double4x4 PerspectiveRevCC( const double & _FovXRad, const double & _Width, const double & _Height, const double & _ZNear, const double & _ZFar ) {
        const double TanHalfFovX = tan( _FovXRad * 0.5f );
        const double HalfFovY = (double)(atan2( _Height, _Width / TanHalfFovX ) );
        const double TanHalfFovY = tan( HalfFovY );
        return Double4x4( 1 / TanHalfFovX, 0,               0,                                         0,
                         0,              -1 / TanHalfFovY, 0,                                         0,
                         0,               0,               _ZNear / ( _ZFar - _ZNear),               -1,
                         0,               0,               _ZNear * _ZFar / ( _ZFar - _ZNear ),       0 );
    }

    static AN_FORCEINLINE Double4x4 PerspectiveRevCC( const double & _FovXRad, const double & _FovYRad, const double & _ZNear, const double & _ZFar ) {
        const double TanHalfFovX = tan( _FovXRad * 0.5f );
        const double TanHalfFovY = tan( _FovYRad * 0.5f );
        return Double4x4( 1 / TanHalfFovX, 0,               0,                                         0,
                         0,              -1 / TanHalfFovY, 0,                                         0,
                         0,               0,               _ZNear / ( _ZFar - _ZNear),               -1,
                         0,               0,               _ZNear * _ZFar / ( _ZFar - _ZNear ),       0 );
    }

    static AN_FORCEINLINE void GetCubeFaceMatrices( Double4x4 & _PositiveX,
                                                    Double4x4 & _NegativeX,
                                                    Double4x4 & _PositiveY,
                                                    Double4x4 & _NegativeY,
                                                    Double4x4 & _PositiveZ,
                                                    Double4x4 & _NegativeZ ) {
        _PositiveX = Double4x4::RotationZ( Math::_PI ).RotateAroundNormal( Math::_HALF_PI, Double3(0,1,0) );
        _NegativeX = Double4x4::RotationZ( Math::_PI ).RotateAroundNormal( -Math::_HALF_PI, Double3(0,1,0) );
        _PositiveY = Double4x4::RotationX( -Math::_HALF_PI );
        _NegativeY = Double4x4::RotationX( Math::_HALF_PI );
        _PositiveZ = Double4x4::RotationX( Math::_PI );
        _NegativeZ = Double4x4::RotationZ( Math::_PI );
    }

    static AN_FORCEINLINE const Double4x4 * GetCubeFaceMatrices() {
        // TODO: Precompute this matrices
        static const Double4x4 CubeFaceMatrices[6] = {
            Double4x4::RotationZ( Math::_PI ).RotateAroundNormal( Math::_HALF_PI, Double3(0,1,0) ),
            Double4x4::RotationZ( Math::_PI ).RotateAroundNormal( -Math::_HALF_PI, Double3(0,1,0) ),
            Double4x4::RotationX( -Math::_HALF_PI ),
            Double4x4::RotationX( Math::_HALF_PI ),
            Double4x4::RotationX( Math::_PI ),
            Double4x4::RotationZ( Math::_PI )
        };
        return CubeFaceMatrices;
    }
};

// Column-major matrix 3x4
// Keep transformations transposed
class Double3x4 final {
public:
    Double4 Col0;
    Double4 Col1;
    Double4 Col2;

    Double3x4() = default;
    explicit Double3x4( const Double2x2 & _Value );
    explicit Double3x4( const Double3x3 & _Value );
    explicit Double3x4( const Double4x4 & _Value );
    constexpr Double3x4( const Double4 & _Col0, const Double4 & _Col1, const Double4 & _Col2 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Double3x4( const double & _00, const double & _01, const double & _02, const double & _03,
                        const double & _10, const double & _11, const double & _12, const double & _13,
                        const double & _20, const double & _21, const double & _22, const double & _23 )
        : Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ) {}
    constexpr explicit Double3x4( const double & _Diagonal ) : Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ) {}
    constexpr explicit Double3x4( const Double3 & _Diagonal ) : Col0( _Diagonal.X.Value, 0, 0, 0 ), Col1( 0, _Diagonal.Y.Value, 0, 0 ), Col2( 0, 0, _Diagonal.Z.Value, 0 ) {}

    Double * ToPtr() {
        return &Col0.X;
    }

    const Double * ToPtr() const {
        return &Col0.X;
    }

    Double3x4 & operator=( const Double3x4 & _Other ) {
        //Col0 = _Other.Col0;
        //Col1 = _Other.Col1;
        //Col2 = _Other.Col2;
        memcpy( this, &_Other, sizeof( *this ) );
        return *this;
    }

    Double4 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Double4 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Bool operator==( const Double3x4 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Double3x4 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Double3x4 & _Other ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 12 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Double3x4 & _Other, const Double & _Epsilon ) const {
        const Double * pm1 = ToPtr();
        const Double * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 12 ; i++ ) {
            if ( ( *pm1++ - *pm2++ ).Abs() >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void Compose( const Double3 & _Translation, const Double3x3 & _Rotation, const Double3 & _Scale ) {
        Col0[3] = _Translation.X;
        Col1[3] = _Translation.Y;
        Col2[3] = _Translation.Z;

        Col0[0] = _Rotation[0][0] * _Scale.X;
        Col0[1] = _Rotation[1][0] * _Scale.Y;
        Col0[2] = _Rotation[2][0] * _Scale.Z;

        Col1[0] = _Rotation[0][1] * _Scale.X;
        Col1[1] = _Rotation[1][1] * _Scale.Y;
        Col1[2] = _Rotation[2][1] * _Scale.Z;

        Col2[0] = _Rotation[0][2] * _Scale.X;
        Col2[1] = _Rotation[1][2] * _Scale.Y;
        Col2[2] = _Rotation[2][2] * _Scale.Z;
    }

    void Compose( const Double3 & _Translation, const Double3x3 & _Rotation ) {
        Col0[3] = _Translation.X;
        Col1[3] = _Translation.Y;
        Col2[3] = _Translation.Z;

        Col0[0] = _Rotation[0][0];
        Col0[1] = _Rotation[1][0];
        Col0[2] = _Rotation[2][0];

        Col1[0] = _Rotation[0][1];
        Col1[1] = _Rotation[1][1];
        Col1[2] = _Rotation[2][1];

        Col2[0] = _Rotation[0][2];
        Col2[1] = _Rotation[1][2];
        Col2[2] = _Rotation[2][2];
    }

    void SetTranslation( const Double3 & _Translation ) {
        Col0[3] = _Translation.X;
        Col1[3] = _Translation.Y;
        Col2[3] = _Translation.Z;
    }

    void DecomposeAll( Double3 & _Translation, Double3x3 & _Rotation, Double3 & _Scale ) const {
        _Translation.X = Col0[3];
        _Translation.Y = Col1[3];
        _Translation.Z = Col2[3];

        _Scale.X = Double3( Col0[0], Col1[0], Col2[0] ).Length();
        _Scale.Y = Double3( Col0[1], Col1[1], Col2[1] ).Length();
        _Scale.Z = Double3( Col0[2], Col1[2], Col2[2] ).Length();

        double sx = 1.0 / _Scale.X;
        double sy = 1.0 / _Scale.Y;
        double sz = 1.0 / _Scale.Z;

        _Rotation[0][0] = Col0[0] * sx;
        _Rotation[1][0] = Col0[1] * sy;
        _Rotation[2][0] = Col0[2] * sz;

        _Rotation[0][1] = Col1[0] * sx;
        _Rotation[1][1] = Col1[1] * sy;
        _Rotation[2][1] = Col1[2] * sz;

        _Rotation[0][2] = Col2[0] * sx;
        _Rotation[1][2] = Col2[1] * sy;
        _Rotation[2][2] = Col2[2] * sz;
    }

    Double3 DecomposeTranslation() const {
        return Double3( Col0[3], Col1[3], Col2[3] );
    }

    Double3x3 DecomposeRotation() const {
        return Double3x3( Double3( Col0[0], Col1[0], Col2[0] ) / Double3( Col0[0], Col1[0], Col2[0] ).Length(),
                         Double3( Col0[1], Col1[1], Col2[1] ) / Double3( Col0[1], Col1[1], Col2[1] ).Length(),
                         Double3( Col0[2], Col1[2], Col2[2] ) / Double3( Col0[2], Col1[2], Col2[2] ).Length() );
    }

    Double3 DecomposeScale() const {
        return Double3( Double3( Col0[0], Col1[0], Col2[0] ).Length(),
                       Double3( Col0[1], Col1[1], Col2[1] ).Length(),
                       Double3( Col0[2], Col1[2], Col2[2] ).Length() );
    }

    void DecomposeRotationAndScale( Double3x3 & _Rotation, Double3 & _Scale ) const {
        _Scale.X = Double3( Col0[0], Col1[0], Col2[0] ).Length();
        _Scale.Y = Double3( Col0[1], Col1[1], Col2[1] ).Length();
        _Scale.Z = Double3( Col0[2], Col1[2], Col2[2] ).Length();

        double sx = 1.0 / _Scale.X;
        double sy = 1.0 / _Scale.Y;
        double sz = 1.0 / _Scale.Z;

        _Rotation[0][0] = Col0[0] * sx;
        _Rotation[1][0] = Col0[1] * sy;
        _Rotation[2][0] = Col0[2] * sz;

        _Rotation[0][1] = Col1[0] * sx;
        _Rotation[1][1] = Col1[1] * sy;
        _Rotation[2][1] = Col1[2] * sz;

        _Rotation[0][2] = Col2[0] * sx;
        _Rotation[1][2] = Col2[1] * sy;
        _Rotation[2][2] = Col2[2] * sz;
    }

    void DecomposeNormalMatrix( Double3x3 & _NormalMatrix ) const {
        const Double3x4 & m = *this;

        const double Determinant = m[0][0] * m[1][1] * m[2][2] +
                                  m[1][0] * m[2][1] * m[0][2] +
                                  m[2][0] * m[0][1] * m[1][2] -
                                  m[2][0] * m[1][1] * m[0][2] -
                                  m[1][0] * m[0][1] * m[2][2] -
                                  m[0][0] * m[2][1] * m[1][2];

        const double OneOverDeterminant = 1.0 / Determinant;

        _NormalMatrix[0][0] =  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * OneOverDeterminant;
        _NormalMatrix[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * OneOverDeterminant;
        _NormalMatrix[0][2] =  (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * OneOverDeterminant;

        _NormalMatrix[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * OneOverDeterminant;
        _NormalMatrix[1][1] =  (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * OneOverDeterminant;
        _NormalMatrix[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * OneOverDeterminant;

        _NormalMatrix[2][0] =  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
        _NormalMatrix[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;
        _NormalMatrix[2][2] =  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * OneOverDeterminant;

        // Or
        //_NormalMatrix = Double3x3(Inversed());
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Double3x4 Inversed() const {
        const Double3x4 & m = *this;

        const double Determinant = m[0][0] * m[1][1] * m[2][2] +
                                  m[1][0] * m[2][1] * m[0][2] +
                                  m[2][0] * m[0][1] * m[1][2] -
                                  m[2][0] * m[1][1] * m[0][2] -
                                  m[1][0] * m[0][1] * m[2][2] -
                                  m[0][0] * m[2][1] * m[1][2];

        const double OneOverDeterminant = 1.0 / Determinant;
        Double3x4 Result;

        Result[0][0] =  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * OneOverDeterminant;
        Result[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * OneOverDeterminant;
        Result[0][2] =  (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * OneOverDeterminant;
        Result[0][3] = -(m[0][3] * Result[0][0] + m[1][3] * Result[0][1] + m[2][3] * Result[0][2]);

        Result[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * OneOverDeterminant;
        Result[1][1] =  (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * OneOverDeterminant;
        Result[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * OneOverDeterminant;
        Result[1][3] = -(m[0][3] * Result[1][0] + m[1][3] * Result[1][1] + m[2][3] * Result[1][2]);

        Result[2][0] =  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
        Result[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;
        Result[2][2] =  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * OneOverDeterminant;
        Result[2][3] = -(m[0][3] * Result[2][0] + m[1][3] * Result[2][1] + m[2][3] * Result[2][2]);

        return Result;
    }

    double Determinant() const {
        return Col0[0] * ( Col1[1] * Col2[2] - Col2[1] * Col1[2] ) +
               Col1[0] * ( Col2[1] * Col0[2] - Col0[1] * Col2[2] ) +
               Col2[0] * ( Col0[1] * Col1[2] - Col1[1] * Col0[2] );
    }

    void Clear() {
        ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        Clear();
        Col0.X = Col1.Y = Col2.Z = 1;
    }

    static Double3x4 Translation( const Double3 & _Vec ) {
        return Double3x4( Double4( 1,0,0,_Vec[0] ),
                         Double4( 0,1,0,_Vec[1] ),
                         Double4( 0,0,1,_Vec[2] ) );
    }

    static Double3x4 Scale( const Double3 & _Scale  ) {
        return Double3x4( Double4( _Scale[0],0,0,0 ),
                         Double4( 0,_Scale[1],0,0 ),
                         Double4( 0,0,_Scale[2],0 ) );
    }

    // Return rotation around normalized axis
    static Double3x4 RotationAroundNormal( const double & _AngleRad, const Double3 & _Normal ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        const Double3 Temp = ( 1.0 - c ) * _Normal;
        const Double3 Temp2 = s * _Normal;
        return Double3x4( c + Temp[0] * _Normal[0]        ,        Temp[1] * _Normal[0] - Temp2[2],     Temp[2] * _Normal[0] + Temp2[1], 0,
                             Temp[0] * _Normal[1] + Temp2[2], c + Temp[1] * _Normal[1],                Temp[2] * _Normal[1] - Temp2[0], 0,
                             Temp[0] * _Normal[2] - Temp2[1],     Temp[1] * _Normal[2] + Temp2[0], c + Temp[2] * _Normal[2],            0 );
    }

    // Return rotation around unnormalized vector
    static Double3x4 RotationAroundVector( const double & _AngleRad, const Double3 & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Double3x4 RotationX( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double3x4( 1, 0, 0, 0,
                         0, c,-s, 0,
                         0, s, c, 0 );
    }

    // Return rotation around Y axis
    static Double3x4 RotationY( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double3x4( c, 0, s, 0,
                         0, 1, 0, 0,
                        -s, 0, c, 0 );
    }

    // Return rotation around Z axis
    static Double3x4 RotationZ( const double & _AngleRad ) {
        double s, c;
        Math::RadSinCos( _AngleRad, s, c );
        return Double3x4( c,-s, 0, 0,
                         s, c, 0, 0,
                         0, 0, 1, 0 );
    }

    // Assume _Vec.W = 1
    Double3 operator*( const Double3 & _Vec ) const {
        return Double3( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[2] * _Vec.Z + Col0[3],
                       Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[2] * _Vec.Z + Col1[3],
                       Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[2] * _Vec.Z + Col2[3] );
    }

    // Assume _Vec.Z = 0, _Vec.W = 1
    Double3 operator*( const Double2 & _Vec ) const {
        return Double3( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                       Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3],
                       Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[3] );
    }

    Double2 Mult_Vec2_IgnoreZ( const Double2 & _Vec ) {
        return Double2( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                       Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3] );
    }

    Double3x4 operator*( const double & _Value ) const {
        return Double3x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( const double & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Double3x4 operator/( const double & _Value ) const {
        const double OneOverValue = 1.0 / _Value;
        return Double3x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue);
    }

    void operator/=( const double & _Value ) {
        const double OneOverValue = 1.0 / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    Double3x4 operator*( const Double3x4 & _Mat ) const {
        return Double3x4( Col0[0] * _Mat[0][0] + Col0[1] * _Mat[1][0] + Col0[2] * _Mat[2][0],
                         Col0[0] * _Mat[0][1] + Col0[1] * _Mat[1][1] + Col0[2] * _Mat[2][1],
                         Col0[0] * _Mat[0][2] + Col0[1] * _Mat[1][2] + Col0[2] * _Mat[2][2],
                         Col0[0] * _Mat[0][3] + Col0[1] * _Mat[1][3] + Col0[2] * _Mat[2][3] + Col0[3],

                         Col1[0] * _Mat[0][0] + Col1[1] * _Mat[1][0] + Col1[2] * _Mat[2][0],
                         Col1[0] * _Mat[0][1] + Col1[1] * _Mat[1][1] + Col1[2] * _Mat[2][1],
                         Col1[0] * _Mat[0][2] + Col1[1] * _Mat[1][2] + Col1[2] * _Mat[2][2],
                         Col1[0] * _Mat[0][3] + Col1[1] * _Mat[1][3] + Col1[2] * _Mat[2][3] + Col1[3],

                         Col2[0] * _Mat[0][0] + Col2[1] * _Mat[1][0] + Col2[2] * _Mat[2][0],
                         Col2[0] * _Mat[0][1] + Col2[1] * _Mat[1][1] + Col2[2] * _Mat[2][1],
                         Col2[0] * _Mat[0][2] + Col2[1] * _Mat[1][2] + Col2[2] * _Mat[2][2],
                         Col2[0] * _Mat[0][3] + Col2[1] * _Mat[1][3] + Col2[2] * _Mat[2][3] + Col2[3] );
    }

    void operator*=( const Double3x4 & _Mat ) {
        *this = Double3x4( Col0[0] * _Mat[0][0] + Col0[1] * _Mat[1][0] + Col0[2] * _Mat[2][0],
                          Col0[0] * _Mat[0][1] + Col0[1] * _Mat[1][1] + Col0[2] * _Mat[2][1],
                          Col0[0] * _Mat[0][2] + Col0[1] * _Mat[1][2] + Col0[2] * _Mat[2][2],
                          Col0[0] * _Mat[0][3] + Col0[1] * _Mat[1][3] + Col0[2] * _Mat[2][3] + Col0[3],

                          Col1[0] * _Mat[0][0] + Col1[1] * _Mat[1][0] + Col1[2] * _Mat[2][0],
                          Col1[0] * _Mat[0][1] + Col1[1] * _Mat[1][1] + Col1[2] * _Mat[2][1],
                          Col1[0] * _Mat[0][2] + Col1[1] * _Mat[1][2] + Col1[2] * _Mat[2][2],
                          Col1[0] * _Mat[0][3] + Col1[1] * _Mat[1][3] + Col1[2] * _Mat[2][3] + Col1[3],

                          Col2[0] * _Mat[0][0] + Col2[1] * _Mat[1][0] + Col2[2] * _Mat[2][0],
                          Col2[0] * _Mat[0][1] + Col2[1] * _Mat[1][1] + Col2[2] * _Mat[2][1],
                          Col2[0] * _Mat[0][2] + Col2[1] * _Mat[1][2] + Col2[2] * _Mat[2][2],
                          Col2[0] * _Mat[0][3] + Col2[1] * _Mat[1][3] + Col2[2] * _Mat[2][3] + Col2[3] );
    }

    // String conversions
    AString ToString( int _Precision = DBL_DIG ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
    }

    // Static methods

    static const Double3x4 & Identity() {
        static constexpr Double3x4 IdentityMat( 1 );
        return IdentityMat;
    }
};

// Type conversion

// Double3x3 to Double2x2
AN_FORCEINLINE Double2x2::Double2x2( const Double3x3 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Double3x4 to Double2x2
AN_FORCEINLINE Double2x2::Double2x2( const Double3x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Double4x4 to Double2x2
AN_FORCEINLINE Double2x2::Double2x2( const Double4x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Double2x2 to Double3x3
AN_FORCEINLINE Double3x3::Double3x3( const Double2x2 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0,0,1 ) {}

// Double3x4 to Double3x3
AN_FORCEINLINE Double3x3::Double3x3( const Double3x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Double4x4 to Double3x3
AN_FORCEINLINE Double3x3::Double3x3( const Double4x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Double2x2 to Double4x4
AN_FORCEINLINE Double4x4::Double4x4( const Double2x2 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0,0,1,0 ), Col3( 0,0,0,1 ) {}

// Double3x3 to Double4x4
AN_FORCEINLINE Double4x4::Double4x4( const Double3x3 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0,0,0,1 ) {}

// Double3x4 to Double4x4
AN_FORCEINLINE Double4x4::Double4x4( const Double3x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0,0,0,1 ) {}

// Double2x2 to Double3x4
AN_FORCEINLINE Double3x4::Double3x4( const Double2x2 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0.0 ) {}

// Double3x3 to Double3x4
AN_FORCEINLINE Double3x4::Double3x4( const Double3x3 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Double4x4 to Double3x4
AN_FORCEINLINE Double3x4::Double3x4( const Double4x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}


AN_FORCEINLINE Double2 operator*( const Double2 & _Vec, const Double2x2 & _Mat ) {
    return Double2( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y,
                   _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y );
}

AN_FORCEINLINE Double3 operator*( const Double3 & _Vec, const Double3x3 & _Mat ) {
    return Double3( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z,
                   _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z,
                   _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z);
}

AN_FORCEINLINE Double4 operator*( const Double4 & _Vec, const Double4x4 & _Mat ) {
    return Double4( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z + _Mat[0][3] * _Vec.W,
                   _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z + _Mat[1][3] * _Vec.W,
                   _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z + _Mat[2][3] * _Vec.W,
                   _Mat[3][0] * _Vec.X + _Mat[3][1] * _Vec.Y + _Mat[3][2] * _Vec.Z + _Mat[3][3] * _Vec.W );
}

AN_FORCEINLINE Double4x4 Double4x4::operator*( const Double3x4 & _Mat ) const {
    const Double4 & SrcB0 = _Mat.Col0;
    const Double4 & SrcB1 = _Mat.Col1;
    const Double4 & SrcB2 = _Mat.Col2;

    return Double4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                     Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                     Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                     Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

AN_FORCEINLINE void Double4x4::operator*=( const Double3x4 & _Mat ) {
    const Double4 & SrcB0 = _Mat.Col0;
    const Double4 & SrcB1 = _Mat.Col1;
    const Double4 & SrcB2 = _Mat.Col2;

    *this = Double4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                      Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                      Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                      Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

namespace Math {

    /*static*/ AN_FORCEINLINE bool Unproject( const Double4x4 & _ModelViewProjectionInversed, const double _Viewport[4], const Double3 & _Coord, Double3 & _Result ) {
        Double4 In( _Coord, 1.0 );

        // Map x and y from window coordinates
        In.X = (In.X - _Viewport[0]) / _Viewport[2];
        In.Y = (In.Y - _Viewport[1]) / _Viewport[3];

        // Map to range -1 to 1
        In.X = In.X * 2.0 - 1.0;
        In.Y = In.Y * 2.0 - 1.0;
        In.Z = In.Z * 2.0 - 1.0;

        _Result.X = _ModelViewProjectionInversed[0][0] * In[0] + _ModelViewProjectionInversed[1][0] * In[1] + _ModelViewProjectionInversed[2][0] * In[2] + _ModelViewProjectionInversed[3][0] * In[3];
        _Result.Y = _ModelViewProjectionInversed[0][1] * In[0] + _ModelViewProjectionInversed[1][1] * In[1] + _ModelViewProjectionInversed[2][1] * In[2] + _ModelViewProjectionInversed[3][1] * In[3];
        _Result.Z = _ModelViewProjectionInversed[0][2] * In[0] + _ModelViewProjectionInversed[1][2] * In[1] + _ModelViewProjectionInversed[2][2] * In[2] + _ModelViewProjectionInversed[3][2] * In[3];
        const double Div = _ModelViewProjectionInversed[0][3] * In[0] + _ModelViewProjectionInversed[1][3] * In[1] + _ModelViewProjectionInversed[2][3] * In[2] + _ModelViewProjectionInversed[3][3] * In[3];

        if ( Div == 0.0 ) {
            return false;
        }

        _Result /= Div;

        return true;
    }

    /*static*/ AN_FORCEINLINE bool UnprojectRay( const Double4x4 & _ModelViewProjectionInversed, const double _Viewport[4], const double & _X, const double & _Y, Double3 & _RayStart, Double3 & _RayEnd ) {
        Double3 Coord;

        Coord.X = _X;
        Coord.Y = _Y;

        // get start point
        Coord.Z = -1.0;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayStart ) )
            return false;

        // get end point
        Coord.Z = 1.0;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayEnd ) )
            return false;

        return true;
    }

    /*static*/ AN_FORCEINLINE bool UnprojectRayDir( const Double4x4 & _ModelViewProjectionInversed, const double _Viewport[4], const double & _X, const double & _Y, Double3 & _RayStart, Double3 & _RayDir ) {
        Double3 Coord;

        Coord.X = _X;
        Coord.Y = _Y;

        // get start point
        Coord.Z = -1.0;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayStart ) )
            return false;

        // get end point
        Coord.Z = 1.0;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayDir ) )
            return false;

        _RayDir -= _RayStart;
        _RayDir.NormalizeSelf();

        return true;
    }

    /*static*/ AN_FORCEINLINE bool UnprojectPoint( const Double4x4 & _ModelViewProjectionInversed,
                                               const double _Viewport[4],
                                               const double & _X, const double & _Y, const double & _Depth,
                                               Double3 & _Result )
    {
        return Unproject( _ModelViewProjectionInversed, _Viewport, Double3( _X, _Y, _Depth ), _Result );
    }

}
