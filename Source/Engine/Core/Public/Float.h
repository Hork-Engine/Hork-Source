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

#include "Integer.h"
#include "Double.h"
#include "Shuffle.h"

class Float;
class Float2;
class Float3;
class Float4;
class Float2x2;
class Float3x3;
class Float4x4;
class Float3x4;

#ifndef FLT_DIG
#define FLT_DIG 6
#endif

class ANGIE_API Float final {
public:
    float Value;

    Float() = default;
    constexpr Float( const float & _Value ) : Value( _Value ) {}

    constexpr Float( const SignedByte & _Value );
    constexpr Float( const Byte & _Value );
    constexpr Float( const Short & _Value );
    constexpr Float( const UShort & _Value );
    constexpr Float( const Int & _Value );
    constexpr Float( const UInt & _Value );
    constexpr Float( const Long & _Value );
    constexpr Float( const ULong & _Value );
    constexpr Float( const Float & _Value );
    constexpr Float( const Double & _Value );

    Float * ToPtr() {
        return this;
    }

    const Float * ToPtr() const {
        return this;
    }

    operator float() const {
        return Value;
    }

    Float & operator=( const float & _Other ) {
        Value = _Other;
        return *this;
    }

    Float & operator=( const Double & _Other );

    // Comparision operators
#if 0
    bool operator<( const float & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const float & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const float & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const float & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const float & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const float & _Other ) const {
        return Value >= _Other;
    }
#endif

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }

#if 0
    // Float::operator==
    friend bool operator==( const Float & _Left, const float & _Right );
    friend bool operator==( const float & _Left, const Float & _Right );
    friend bool operator==( const Float & _Left, const Float & _Right );

    // Float::operator!=
    friend bool operator!=( const Float & _Left, const float & _Right );
    friend bool operator!=( const float & _Left, const Float & _Right );
    friend bool operator!=( const Float & _Left, const Float & _Right );

    // Float::operator<
    friend bool operator<( const Float & _Left, const float & _Right );
    friend bool operator<( const float & _Left, const Float & _Right );
    friend bool operator<( const Float & _Left, const Float & _Right );

    // Float::operator>
    friend bool operator>( const Float & _Left, const float & _Right );
    friend bool operator>( const float & _Left, const Float & _Right );
    friend bool operator>( const Float & _Left, const Float & _Right );

    // Float::operator<=
    friend bool operator<=( const Float & _Left, const float & _Right );
    friend bool operator<=( const float & _Left, const Float & _Right );
    friend bool operator<=( const Float & _Left, const Float & _Right );

    // Float::operator>=
    friend bool operator>=( const Float & _Left, const float & _Right );
    friend bool operator>=( const float & _Left, const Float & _Right );
    friend bool operator>=( const Float & _Left, const Float & _Right );
#endif
    // Math operators
    Float operator+() const {
        return *this;
    }
    Float operator-() const {
        return -Value;
    }
    Float operator+( const float & _Other ) const {
        return Value + _Other;
    }
    Float operator-( const float & _Other ) const {
        return Value - _Other;
    }
    Float operator/( const float & _Other ) const {
        return Value / _Other;
    }
    Float operator*( const float & _Other ) const {
        return Value * _Other;
    }
    void operator+=( const float & _Other ) {
        Value += _Other;
    }
    void operator-=( const float & _Other ) {
        Value -= _Other;
    }
    void operator/=( const float & _Other ) {
        Value /= _Other;
    }
    void operator*=( const float & _Other ) {
        Value *= _Other;
    }

    // Floating point specific
    Bool IsInfinite() const {
        //return std::isinf( Value );
        return ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x7fffffff ) == 0x7f800000;
    }
    Bool IsNan() const {
        //return std::isnan( Value );
        return ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x7f800000 ) == 0x7f800000;
    }
    Bool IsNormal() const {
        return std::isnormal( Value );
    }
    Bool IsDenormal() const {
        return ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x7f800000 ) == 0x00000000
            && ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x007fffff ) != 0x00000000;
    }

    // Comparision
    Bool LessThan( const float & _Other ) const { return std::isless( Value, _Other ); }
    Bool LequalThan( const float & _Other ) const { return std::islessequal( Value, _Other ); }
    Bool GreaterThan( const float & _Other ) const { return std::isgreater( Value, _Other ); }
    Bool GequalThan( const float & _Other ) const { return !std::isless( Value, _Other ); }
    Bool NotEqual( const float & _Other ) const { return std::islessgreater( Value, _Other ); }
    Bool Compare( const float & _Other ) const { return !NotEqual( _Other ); }

    Bool CompareEps( const Float & _Other, const Float & _Epsilon ) const {
        return Dist( _Other ) < _Epsilon;
    }

    void Clear() {
        Value = 0;
    }

    Float Abs() const {
        int32_t i = *reinterpret_cast< const int32_t * >( &Value ) & 0x7FFFFFFF;
        return *reinterpret_cast< const float * >( &i );
    }

    // Vector methods
    Float Length() const {
        return Abs();
    }

    Float Dist( const float & _Other ) const {
        return ( *this - _Other ).Length();
    }

    Float NormalizeSelf() {
        float L = Length();
        if ( L != 0.0f ) {
            Value /= L;
        }
        return L;
    }

    Float Normalized() const {
        const float L = Length();
        if ( L != 0.0f ) {
            return *this / L;
        }
        return *this;
    }

    Float Floor() const {
        return floorf( Value );
    }

    Float Ceil() const {
        return ceilf( Value );
    }

    Float Fract() const {
        return Value - floorf( Value );
    }

    Float Step( const float & _Edge ) const {
        return Value < _Edge ? 0.0f : 1.0f;
    }

    Float SmoothStep( const float & _Edge0, const float & _Edge1 ) const {
        const Float t = Float( ( Value - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
        return t * t * ( 3.0f - 2.0f * t );
    }

    Int ToIntFast() const {
    #if 0
        // FIXME: overflowing on 31 bit?
        int32_t i;
        __asm fld   Value
        __asm fistp i
        return i;
    #else
        return static_cast< int32_t >( Value );
    #endif
    }

    Long ToLongFast() const {
    #if 0
        // FIXME: overflowing on 31 bit?
        int64_t i;
        __asm fld   Value
        __asm fistp i
        return i;
    #else
        return static_cast< int64_t >( Value );
    #endif
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Float ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Float ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Float ToClosestPowerOfTwo() const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Float Sign() const {
        return Value > 0 ? 1.0f : -SignBits();
    }

    // Return 1 if value is less than 0 or 1 if value is greater or equal to 0
    int SignBits() const {
        return *reinterpret_cast< const uint32_t * >( &Value ) >> 31;
    }

    // Return floating point exponent
    int Exponent() const {
        return ( *reinterpret_cast< const uint32_t * >( &Value ) >> 23 ) & 0xff;
    }

    static constexpr int MaxExponent() { return 127; }

    // Return floating point mantissa
    int Mantissa() const {
        return *reinterpret_cast< const uint32_t * >( &Value ) & 0x7fffff;
    }

    UShort ToHalfFloat() const {
        return FloatToHalf( *reinterpret_cast< const uint32_t * >( &Value ) );
    }

    void FromHalfFloat( const UShort & _HalfFloat ) {
        *reinterpret_cast< uint32_t * >( &Value ) = HalfToFloat( _HalfFloat );
    }

    static UShort FloatToHalf( const UInt & _I );
    static UInt HalfToFloat( const UShort & _I );

    static void FloatToHalf( const float * _In, uint16_t * _Out, int _Count );
    static void HalfToFloat( const uint16_t * _In, float * _Out, int _Count );

    Float Lerp( const float & _To, const float & _Mix ) const {
        return Lerp( Value, _To, _Mix );
    }

    static Float Lerp( const float & _From, const float & _To, const float & _Mix ) {
        return _From + _Mix * ( _To - _From );
    }

    Float Clamp( const float & _Min, const float & _Max ) const {
        //if ( Value < _Min )
        //    return _Min;
        //if ( Value > _Max )
        //    return _Max;
        //return Value;
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    Float Saturate() const {
        return Clamp( 0.0f, 1.0f );
    }

    Float Round() const {
        return floorf( Value + 0.5f );
    }

    Float RoundN( const float & _N ) const {
        return floorf( Value * _N + 0.5f ) / _N;
    }

    Float Round1() const { return RoundN( 10.0f ); }
    Float Round2() const { return RoundN( 100.0f ); }
    Float Round3() const { return RoundN( 1000.0f ); }
    Float Round4() const { return RoundN( 10000.0f ); }

    Float Snap( const float & _SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        return Float( Value / _SnapValue ).Round() * _SnapValue;
    }

    Float SwapBytes() const {
        union {
            float f;
            byte  b[4];
        } dat1, dat2;

        dat1.f = Value;
        dat2.b[0] = dat1.b[3];
        dat2.b[1] = dat1.b[2];
        dat2.b[2] = dat1.b[1];
        dat2.b[3] = dat1.b[0];
        return dat2.f;
    }

    Float ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    Float ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    FString ToString( int _Precision = FLT_DIG ) const {
        TSprintfBuffer< 64 > value;
        if ( _Precision >= 0 ) {
            TSprintfBuffer< 64 > format;
            value.Sprintf( format.Sprintf( "%%.%df", _Precision ), Value );
        } else {
            value.Sprintf( "%f", Value );
        }
        for ( char * p = &value.Data[ FString::Length( value.Data ) - 1 ] ; p >= &value.Data[0] ; p-- ) {
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

    const char * ToConstChar( int _Precision = FLT_DIG ) const {
        char * s;
        if ( _Precision >= 0 ) {
            TSprintfBuffer< 64 > format;
            s = FString::Fmt( format.Sprintf( "%%.%df", _Precision ), Value );
        } else {
            s = FString::Fmt( "%f", Value );
        }
        for ( char * p = s + FString::Length( s ) - 1 ; p >= s ; p-- ) {
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

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    Float & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    Float & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( *( dword * )&Value );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( *( dword * )&Value );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr int BitsCount() { return sizeof( Float ) * 8; }
    static constexpr Float MinPowerOfTwo() { return 1.0f; }
    static constexpr Float MaxPowerOfTwo() { return float( 1u << 31 ); }
    static constexpr Float MinValue() { return std::numeric_limits< float >::min(); }
    static constexpr Float MaxValue() { return std::numeric_limits< float >::max(); }
};

namespace FMath {

AN_FORCEINLINE bool IsInfinite( const float & _Self ) {
    //return std::isinf( Self );
    return ( *reinterpret_cast< const uint32_t * >( &_Self ) & 0x7fffffff ) == 0x7f800000;
}

AN_FORCEINLINE bool IsNan( const float & _Self ) {
    //return std::isnan( Self );
    return ( *reinterpret_cast< const uint32_t * >( &_Self ) & 0x7f800000 ) == 0x7f800000;
}

AN_FORCEINLINE bool IsNormal( const float & _Self ) {
    return std::isnormal( _Self );
}

AN_FORCEINLINE bool IsDenormal( const float & _Self ) {
    return ( *reinterpret_cast< const uint32_t * >( &_Self ) & 0x7f800000 ) == 0x00000000
        && ( *reinterpret_cast< const uint32_t * >( &_Self ) & 0x007fffff ) != 0x00000000;
}

// Comparision
AN_FORCEINLINE bool LessThan( const float & _Self, const float & _Other ) { return std::isless( _Self, _Other ); }
AN_FORCEINLINE bool LequalThan( const float & _Self, const float & _Other ) { return std::islessequal( _Self, _Other ); }
AN_FORCEINLINE bool GreaterThan( const float & _Self, const float & _Other ) { return std::isgreater( _Self, _Other ); }
AN_FORCEINLINE bool GequalThan( const float & _Self, const float & _Other ) { return !std::isless( _Self, _Other ); }
AN_FORCEINLINE bool NotEqual( const float & _Self, const float & _Other ) { return std::islessgreater( _Self, _Other ); }
AN_FORCEINLINE bool Compare( const float & _Self, const float & _Other ) { return !NotEqual( _Self, _Other ); }

AN_FORCEINLINE float Abs( const float & _Self ) {
    int32_t i = *reinterpret_cast< const int32_t * >( &_Self ) & 0x7FFFFFFF;
    return *reinterpret_cast< const float * >( &i );
}

// Vector methods
AN_FORCEINLINE float Length( const float & _Self ) {
    return Abs( _Self );
}

AN_FORCEINLINE float Dist( const float & _Self, const float & _Other ) {
    return Length( _Self - _Other );
}

AN_FORCEINLINE bool CompareEps( const float & _Self, const float & _Other, const float & _Epsilon ) {
    return Dist( _Self, _Other ) < _Epsilon;
}

AN_FORCEINLINE float Floor( const float & _Self ) {
    return floorf( _Self );
}

AN_FORCEINLINE float Ceil( const float & _Self ) {
    return ceilf( _Self );
}

AN_FORCEINLINE float Fract( const float & _Self ) {
    return _Self - floorf( _Self );
}

AN_FORCEINLINE float Clamp( const float & _Self, const float & _Min, const float & _Max ) {
    //if ( _Self < _Min )
    //    return _Min;
    //if ( _Self > _Max )
    //    return _Max;
    //return _Self;
    return FMath::Min( FMath::Max( _Self, _Min ), _Max );
}

AN_FORCEINLINE float Saturate( const float & _Self ) {
    return Clamp( _Self, 0.0f, 1.0f );
}

AN_FORCEINLINE float Step(  const float & _Self, const float & _Edge ) {
    return _Self < _Edge ? 0.0f : 1.0f;
}

AN_FORCEINLINE float SmoothStep( const float & _Self, const float & _Edge0, const float & _Edge1 ) {
    const float t = Saturate( ( _Self - _Edge0 ) / ( _Edge1 - _Edge0 ) );
    return t * t * ( 3.0f - 2.0f * t );
}

AN_FORCEINLINE int32_t ToIntFast( const float & _Self ) {
#if 0
    // FIXME: overflowing on 31 bit?
    int32_t i;
    __asm fld   _Self
    __asm fistp i
    return i;
#else
    return static_cast< int32_t >( _Self );
#endif
}

AN_FORCEINLINE int64_t ToLongFast( const float & _Self ) {
#if 0
    // FIXME: overflowing on 31 bit?
    int64_t i;
    __asm fld   _Self
    __asm fistp i
    return i;
#else
    return static_cast< int64_t >( _Self );
#endif
}

// Return value is between MinPowerOfTwo and MaxPowerOfTwo
AN_FORCEINLINE float ToGreaterPowerOfTwo( const float & _Self ) {
    if ( _Self >= Float::MaxPowerOfTwo() ) {
        return Float::MaxPowerOfTwo();
    }
    if ( _Self < Float::MinPowerOfTwo() ) {
        return Float::MinPowerOfTwo();
    }
    uint32_t Val = ToIntFast( _Self ) - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast< float >( Val + 1 );
}

// Return value is between MinPowerOfTwo and MaxPowerOfTwo
AN_FORCEINLINE float ToLessPowerOfTwo( const float & _Self ) {
    if ( _Self >= Float::MaxPowerOfTwo() ) {
        return Float::MaxPowerOfTwo();
    }
    if ( _Self < Float::MinPowerOfTwo() ) {
        return Float::MinPowerOfTwo();
    }
    uint32_t Val = ToIntFast( _Self );
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast< float >( Val - ( Val >> 1 ) );
}

// Return value is between MinPowerOfTwo and MaxPowerOfTwo
AN_FORCEINLINE float ToClosestPowerOfTwo( const float & _Self ) {
    float GreaterPow = ToGreaterPowerOfTwo( _Self );
    float LessPow = ToLessPowerOfTwo( _Self );
    return Dist( GreaterPow, _Self ) < Dist( LessPow, _Self ) ? GreaterPow : LessPow;
}

// Return 1 if value is less than 0 or 1 if value is greater or equal to 0
AN_FORCEINLINE int SignBits( const float & _Self ) {
    return *reinterpret_cast< const uint32_t * >( &_Self ) >> 31;
}

// Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
AN_FORCEINLINE float Sign( const float & _Self ) {
    return _Self > 0 ? 1.0f : -SignBits( _Self );
}

// Return floating point exponent
AN_FORCEINLINE int Exponent( const float & _Self ) {
    return ( *reinterpret_cast< const uint32_t * >( &_Self ) >> 23 ) & 0xff;
}

// Return floating point mantissa
AN_FORCEINLINE int Mantissa( const float & _Self ) {
    return *reinterpret_cast< const uint32_t * >( &_Self ) & 0x7fffff;
}

UShort FloatToHalf( const UInt & _I );
UInt HalfToFloat( const UShort & _I );

AN_FORCEINLINE UShort ToHalfFloat( const float & _Self ) {
    return FloatToHalf( *reinterpret_cast< const uint32_t * >( &_Self ) );
}

AN_FORCEINLINE float FromHalfFloat( const UShort & _HalfFloat ) {
    float f;
    *reinterpret_cast< uint32_t * >( &f ) = Float::HalfToFloat( _HalfFloat );
    return f;
}


void FloatToHalf( const float * _In, uint16_t * _Out, int _Count );

void HalfToFloat( const uint16_t * _In, float * _Out, int _Count );

AN_FORCEINLINE float Lerp( const float & _From, const float & _To, const float & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

AN_FORCEINLINE float Round( const float & _Self ) {
    return floorf( _Self + 0.5f );
}

AN_FORCEINLINE float RoundN( const float & _Self, const float & _N ) {
    return floorf( _Self * _N + 0.5f ) / _N;
}

AN_FORCEINLINE float Round1( const float & _Self ) { return RoundN( _Self, 10.0f ); }
AN_FORCEINLINE float Round2( const float & _Self ) { return RoundN( _Self, 100.0f ); }
AN_FORCEINLINE float Round3( const float & _Self ) { return RoundN( _Self, 1000.0f ); }
AN_FORCEINLINE float Round4( const float & _Self ) { return RoundN( _Self, 10000.0f ); }

AN_FORCEINLINE float Snap( const float & _Self, const float & _SnapValue ) {
    AN_ASSERT( _SnapValue > 0, "Snap" );
    return Round( _Self / _SnapValue ) * _SnapValue;
}

AN_FORCEINLINE float SwapBytes( const float & _Self ) {
    union {
        float f;
        byte  b[4];
    } dat1, dat2;

    dat1.f = _Self;
    dat2.b[0] = dat1.b[3];
    dat2.b[1] = dat1.b[2];
    dat2.b[2] = dat1.b[1];
    dat2.b[3] = dat1.b[0];
    return dat2.f;
}

AN_FORCEINLINE float ToBigEndian( const float & _Self ) {
#ifdef AN_LITTLE_ENDIAN
    return SwapBytes( _Self );
#else
    return _Self;
#endif
}

AN_FORCEINLINE float ToLittleEndian( const float & _Self ) {
#ifdef AN_LITTLE_ENDIAN
    return _Self;
#else
    return SwapBytes( _Self );
#endif
}

// String conversions
AN_FORCEINLINE FString ToString( const float & _Self, int _Precision = FLT_DIG ) {
    TSprintfBuffer< 64 > value;
    if ( _Precision >= 0 ) {
        TSprintfBuffer< 64 > format;
        value.Sprintf( format.Sprintf( "%%.%df", _Precision ), _Self );
    } else {
        value.Sprintf( "%f", _Self );
    }
    for ( char * p = &value.Data[ FString::Length( value.Data ) - 1 ] ; p >= &value.Data[0] ; p-- ) {
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

AN_FORCEINLINE const char * ToConstChar( const float & _Self, int _Precision = FLT_DIG ) {
    char * s;
    if ( _Precision >= 0 ) {
        TSprintfBuffer< 64 > format;
        s = FString::Fmt( format.Sprintf( "%%.%df", _Precision ), _Self );
    } else {
        s = FString::Fmt( "%f", _Self );
    }
    for ( char * p = s + FString::Length( s ) - 1 ; p >= s ; p-- ) {
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

AN_FORCEINLINE FString ToHexString( const float & _Self, bool _LeadingZeros = false, bool _Prefix = false ) {
    return FString::ToHexString( _Self, _LeadingZeros, _Prefix );
}

float FromString( const char * _String );

AN_FORCEINLINE float FromString( FString const & _String ) {
    return FromString( _String.ToConstChar() );
}

}

class Float2 final {
public:
    float X;
    float Y;

    Float2() = default;
    explicit constexpr Float2( const float & _Value ) : X( _Value ), Y( _Value ) {}
    explicit Float2( const Float3 & _Value );
    explicit Float2( const Float4 & _Value );
    constexpr Float2( const float & _X, const float & _Y ) : X( _X ), Y( _Y ) {}
    explicit Float2( const Double2 & _Value );

    float * ToPtr() {
        return &X;
    }

    const float * ToPtr() const {
        return &X;
    }

    Float2 & operator=( const Float2 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        return *this;
    }

    float & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const float & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int _Shuffle >
    Float2 Shuffle2() const;

    template< int _Shuffle >
    Float3 Shuffle3() const;

    template< int _Shuffle >
    Float4 Shuffle4() const;

    Bool operator==( const Float2 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Float2 & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Float2 operator+() const {
        return *this;
    }
    Float2 operator-() const {
        return Float2( -X, -Y );
    }
    Float2 operator+( const Float2 & _Other ) const {
        return Float2( X + _Other.X, Y + _Other.Y );
    }
    Float2 operator-( const Float2 & _Other ) const {
        return Float2( X - _Other.X, Y - _Other.Y );
    }
    Float2 operator/( const Float2 & _Other ) const {
        return Float2( X / _Other.X, Y / _Other.Y );
    }
    Float2 operator*( const Float2 & _Other ) const {
        return Float2( X * _Other.X, Y * _Other.Y );
    }
    Float2 operator+( const float & _Other ) const {
        return Float2( X + _Other, Y + _Other );
    }
    Float2 operator-( const float & _Other ) const {
        return Float2( X - _Other, Y - _Other );
    }
    Float2 operator/( const float & _Other ) const {
        float Denom = 1.0f / _Other;
        return Float2( X * Denom, Y * Denom );
    }
    Float2 operator*( const float & _Other ) const {
        return Float2( X * _Other, Y * _Other );
    }
    void operator+=( const Float2 & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
    }
    void operator-=( const Float2 & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
    }
    void operator/=( const Float2 & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
    }
    void operator*=( const Float2 & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
    }
    void operator+=( const float & _Other ) {
        X += _Other;
        Y += _Other;
    }
    void operator-=( const float & _Other ) {
        X -= _Other;
        Y -= _Other;
    }
    void operator/=( const float & _Other ) {
        float Denom = 1.0f / _Other;
        X *= Denom;
        Y *= Denom;
    }
    void operator*=( const float & _Other ) {
        X *= _Other;
        Y *= _Other;
    }

    float Min() const {
        return FMath::Min( X, Y );
    }

    float Max() const {
        return FMath::Max( X, Y );
    }

    int MinorAxis() const {
        return int( FMath::Abs( X ) >= FMath::Abs( Y ) );
    }

    int MajorAxis() const {
        return int( FMath::Abs( X ) < FMath::Abs( Y ) );
    }

    // Floating point specific
    Bool2 IsInfinite() const {
        return Bool2( FMath::IsInfinite( X ), FMath::IsInfinite( Y ) );
    }

    Bool2 IsNan() const {
        return Bool2( FMath::IsNan( X ), FMath::IsNan( Y ) );
    }

    Bool2 IsNormal() const {
        return Bool2( FMath::IsNormal( X ), FMath::IsNormal( Y ) );
    }

    Bool2 IsDenormal() const {
        return Bool2( FMath::IsDenormal( X ), FMath::IsDenormal( Y ) );
    }

    // Comparision

    Bool2 LessThan( const Float2 & _Other ) const {
        return Bool2( FMath::LessThan( X, _Other.X ), FMath::LessThan( Y, _Other.Y ) );
    }
    Bool2 LessThan( const float & _Other ) const {
        return Bool2( FMath::LessThan( X, _Other ), FMath::LessThan( Y, _Other ) );
    }

    Bool2 LequalThan( const Float2 & _Other ) const {
        return Bool2( FMath::LequalThan( X, _Other.X ), FMath::LequalThan( Y, _Other.Y ) );
    }
    Bool2 LequalThan( const float & _Other ) const {
        return Bool2( FMath::LequalThan( X, _Other ), FMath::LequalThan( Y, _Other ) );
    }

    Bool2 GreaterThan( const Float2 & _Other ) const {
        return Bool2( FMath::GreaterThan( X, _Other.X ), FMath::GreaterThan( Y, _Other.Y ) );
    }
    Bool2 GreaterThan( const float & _Other ) const {
        return Bool2( FMath::GreaterThan( X, _Other ), FMath::GreaterThan( Y, _Other ) );
    }

    Bool2 GequalThan( const Float2 & _Other ) const {
        return Bool2( FMath::GequalThan( X, _Other.X ), FMath::GequalThan( Y, _Other.Y ) );
    }
    Bool2 GequalThan( const float & _Other ) const {
        return Bool2( FMath::GequalThan( X, _Other ), FMath::GequalThan( Y, _Other ) );
    }

    Bool2 NotEqual( const Float2 & _Other ) const {
        return Bool2( FMath::NotEqual( X, _Other.X ), FMath::NotEqual( Y, _Other.Y ) );
    }
    Bool2 NotEqual( const float & _Other ) const {
        return Bool2( FMath::NotEqual( X, _Other ), FMath::NotEqual( Y, _Other ) );
    }

    Bool Compare( const Float2 & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    Bool CompareEps( const Float2 & _Other, const float & _Epsilon ) const {
        return Bool2( FMath::CompareEps( X, _Other.X, _Epsilon ),
                      FMath::CompareEps( Y, _Other.Y, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = 0;
    }

    Float2 Abs() const { return Float2( FMath::Abs( X ), FMath::Abs( Y ) ); }

    // Vector methods
    float LengthSqr() const {
        return X * X + Y * Y;
    }

    float Length() const {
        return StdSqrt( LengthSqr() );
    }

    float DistSqr( const Float2 & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }

    float Dist( const Float2 & _Other ) const {
        return ( *this - _Other ).Length();
    }

    float NormalizeSelf() {
        float L = Length();
        if ( L != 0.0f ) {
            float InvLength = 1.0f / L;
            X *= InvLength;
            Y *= InvLength;
        }
        return L;
    }

    Float2 Normalized() const {
        const float L = Length();
        if ( L != 0.0f ) {
            const float InvLength = 1.0f / L;
            return Float2( X * InvLength, Y * InvLength );
        }
        return *this;
    }

    float Cross( const Float2 & _Other ) const {
        return X * _Other.Y - Y * _Other.X;
    }

    Float2 Floor() const {
        return Float2( FMath::Floor( X ), FMath::Floor( Y ) );
    }

    Float2 Ceil() const {
        return Float2( FMath::Ceil( X ), FMath::Ceil( Y ) );
    }

    Float2 Fract() const {
        return Float2( FMath::Fract( X ), FMath::Fract( Y ) );
    }

    Float2 Step( const float & _Edge ) const;
    Float2 Step( const Float2 & _Edge ) const;

    Float2 SmoothStep( const float & _Edge0, const float & _Edge1 ) const;
    Float2 SmoothStep( const Float2 & _Edge0, const Float2 & _Edge1 ) const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Float2 Sign() const {
        return Float2( FMath::Sign( X ), FMath::Sign( Y ) );
    }

    int SignBits() const {
        return FMath::SignBits( X ) | (FMath::SignBits( Y )<<1);
    }

    Float2 Lerp( const Float2 & _To, const float & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    static Float2 Lerp( const Float2 & _From, const Float2 & _To, const float & _Mix );

    float Bilerp( const float & _A, const float & _B, const float & _C, const float & _D ) const;
    Float2 Bilerp( const Float2 & _A, const Float2 & _B, const Float2 & _C, const Float2 & _D ) const;
    Float3 Bilerp( const Float3 & _A, const Float3 & _B, const Float3 & _C, const Float3 & _D ) const;
    Float4 Bilerp( const Float4 & _A, const Float4 & _B, const Float4 & _C, const Float4 & _D ) const;

    Float2 Clamp( const float & _Min, const float & _Max ) const {
        return Float2( FMath::Clamp( X, _Min, _Max ), FMath::Clamp( Y, _Min, _Max ) );
    }

    Float2 Clamp( const Float2 & _Min, const Float2 & _Max ) const {
        return Float2( FMath::Clamp( X, _Min.X, _Max.X ), FMath::Clamp( Y, _Min.Y, _Max.Y ) );
    }

    Float2 Saturate() const {
        return Clamp( 0.0f, 1.0f );
    }

    Float2 Snap( const float &_SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        Float2 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = FMath::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = FMath::Round( SnapVector.Y ) * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const {
        if ( X == 1.0f || X == -1.0f ) return FMath::AxialX;
        if ( Y == 1.0f || Y == -1.0f ) return FMath::AxialY;
        return FMath::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == 1.0f ) return FMath::AxialX;
        if ( Y == 1.0f ) return FMath::AxialY;
        return FMath::NonAxial;
    }

    int VectorAxialType() const {
        if ( FMath::Abs( X ) < 0.00001f ) {
            return ( FMath::Abs( Y ) < 0.00001f ) ? FMath::NonAxial : FMath::AxialY;
        }
        return ( FMath::Abs( Y ) < 0.00001f ) ? FMath::AxialX : FMath::NonAxial;
    }

    float Dot( const Float2 & _Other ) const {
        return X * _Other.X + Y * _Other.Y;
    }

    // String conversions
    FString ToString( int _Precision = FLT_DIG ) const {
        return FString( "( " ) + FMath::ToString( X, _Precision ) + " " + FMath::ToString( Y, _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + FMath::ToHexString( X, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( Y, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
    static const Float2 & Zero() {
        static constexpr Float2 ZeroVec(0.0f);
        return ZeroVec;
    }
};

class Float3 final {
public:
    float X;
    float Y;
    float Z;

    Float3() = default;
    explicit constexpr Float3( const float & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    explicit Float3( const Float2 & _Value, const float & _Z = 0.0f );
    explicit Float3( const Float4 & _Value );
    constexpr Float3( const float & _X, const float & _Y, const float & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}
    explicit Float3( const Double3 & _Value );

    float * ToPtr() {
        return &X;
    }

    const float * ToPtr() const {
        return &X;
    }

    Float3 & operator=( const Float3 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        return *this;
    }

    float & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const float & operator[]( const int & _Index ) const {
        //AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int _Shuffle >
    Float2 Shuffle2() const;

    template< int _Shuffle >
    Float3 Shuffle3() const;

    template< int _Shuffle >
    Float4 Shuffle4() const;

    Bool operator==( const Float3 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Float3 & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Float3 operator+() const {
        return *this;
    }
    Float3 operator-() const {
        return Float3( -X, -Y, -Z );
    }
     Float3 operator+( const Float3 & _Other ) const {
        return Float3( X + _Other.X, Y + _Other.Y, Z + _Other.Z );
    }
    Float3 operator-( const Float3 & _Other ) const {
        return Float3( X - _Other.X, Y - _Other.Y, Z - _Other.Z );
    }
    Float3 operator/( const Float3 & _Other ) const {
        return Float3( X / _Other.X, Y / _Other.Y, Z / _Other.Z );
    }
    Float3 operator*( const Float3 & _Other ) const {
        return Float3( X * _Other.X, Y * _Other.Y, Z * _Other.Z );
    }
    Float3 operator+( const float & _Other ) const {
        return Float3( X + _Other, Y + _Other, Z + _Other );
    }
    Float3 operator-( const float & _Other ) const {
        return Float3( X - _Other, Y - _Other, Z - _Other );
    }
    Float3 operator/( const float & _Other ) const {
        float Denom = 1.0f / _Other;
        return Float3( X * Denom, Y * Denom, Z * Denom );
    }
    Float3 operator*( const float & _Other ) const {
        return Float3( X * _Other, Y * _Other, Z * _Other );
    }
    void operator+=( const Float3 & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
    }
    void operator-=( const Float3 & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
    }
    void operator/=( const Float3 & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
    }
    void operator*=( const Float3 & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
    }
    void operator+=( const float & _Other ) {
        X += _Other;
        Y += _Other;
        Z += _Other;
    }
    void operator-=( const float & _Other ) {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
    }
    void operator/=( const float & _Other ) {
        float Denom = 1.0f / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
    }
    void operator*=( const float & _Other ) {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
    }

    float Min() const {
        return FMath::Min( FMath::Min( X, Y ), Z );
    }

    float Max() const {
        return FMath::Max( FMath::Max( X, Y ), Z );
    }

    int MinorAxis() const {
        float Minor = FMath::Abs( X );
        int Axis = 0;
        float Tmp;

        Tmp = FMath::Abs( Y );
        if ( Tmp <= Minor ) {
            Axis = 1;
            Minor = Tmp;
        }

        Tmp = FMath::Abs( Z );
        if ( Tmp <= Minor ) {
            Axis = 2;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const {
        float Major = FMath::Abs( X );
        int Axis = 0;
        float Tmp;

        Tmp = FMath::Abs( Y );
        if ( Tmp > Major ) {
            Axis = 1;
            Major = Tmp;
        }

        Tmp = FMath::Abs( Z );
        if ( Tmp > Major ) {
            Axis = 2;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    Bool3 IsInfinite() const {
        return Bool3( FMath::IsInfinite( X ), FMath::IsInfinite( Y ), FMath::IsInfinite( Z ) );
    }

    Bool3 IsNan() const {
        return Bool3( FMath::IsNan( X ), FMath::IsNan( Y ), FMath::IsNan( Z ) );
    }

    Bool3 IsNormal() const {
        return Bool3( FMath::IsNormal( X ), FMath::IsNormal( Y ), FMath::IsNormal( Z ) );
    }

    Bool3 IsDenormal() const {
        return Bool3( FMath::IsDenormal( X ), FMath::IsDenormal( Y ), FMath::IsDenormal( Z ) );
    }

    // Comparision
    Bool3 LessThan( const Float3 & _Other ) const {
        return Bool3( FMath::LessThan( X, _Other.X ), FMath::LessThan( Y, _Other.Y ), FMath::LessThan( Z, _Other.Z ) );
    }
    Bool3 LessThan( const float & _Other ) const {
        return Bool3( FMath::LessThan( X, _Other ), FMath::LessThan( Y, _Other ), FMath::LessThan( Z, _Other ) );
    }

    Bool3 LequalThan( const Float3 & _Other ) const {
        return Bool3( FMath::LequalThan( X, _Other.X ), FMath::LequalThan( Y, _Other.Y ), FMath::LequalThan( Z, _Other.Z ) );
    }
    Bool3 LequalThan( const float & _Other ) const {
        return Bool3( FMath::LequalThan( X, _Other ), FMath::LequalThan( Y, _Other ), FMath::LequalThan( Z, _Other ) );
    }

    Bool3 GreaterThan( const Float3 & _Other ) const {
        return Bool3( FMath::GreaterThan( X, _Other.X ), FMath::GreaterThan( Y, _Other.Y ), FMath::GreaterThan( Z, _Other.Z ) );
    }
    Bool3 GreaterThan( const float & _Other ) const {
        return Bool3( FMath::GreaterThan( X, _Other ), FMath::GreaterThan( Y, _Other ), FMath::GreaterThan( Z, _Other ) );
    }

    Bool3 GequalThan( const Float3 & _Other ) const {
        return Bool3( FMath::GequalThan( X, _Other.X ), FMath::GequalThan( Y, _Other.Y ), FMath::GequalThan( Z, _Other.Z ) );
    }
    Bool3 GequalThan( const float & _Other ) const {
        return Bool3( FMath::GequalThan( X, _Other ), FMath::GequalThan( Y, _Other ), FMath::GequalThan( Z, _Other ) );
    }

    Bool3 NotEqual( const Float3 & _Other ) const {
        return Bool3( FMath::NotEqual( X, _Other.X ), FMath::NotEqual( Y, _Other.Y ), FMath::NotEqual( Z, _Other.Z ) );
    }
    Bool3 NotEqual( const float & _Other ) const {
        return Bool3( FMath::NotEqual( X, _Other ), FMath::NotEqual( Y, _Other ), FMath::NotEqual( Z, _Other ) );
    }

    Bool Compare( const Float3 & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    Bool CompareEps( const Float3 & _Other, const float & _Epsilon ) const {
        return Bool3( FMath::CompareEps( X, _Other.X, _Epsilon ),
                      FMath::CompareEps( Y, _Other.Y, _Epsilon ),
                      FMath::CompareEps( Z, _Other.Z, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = Z = 0;
    }

    Float3 Abs() const { return Float3( FMath::Abs( X ), FMath::Abs( Y ), FMath::Abs( Z ) ); }

    // Vector methods
    float LengthSqr() const {
        return X * X + Y * Y + Z * Z;
    }
    float Length() const {
        return StdSqrt( LengthSqr() );
    }
    float DistSqr( const Float3 & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }
    float Dist( const Float3 & _Other ) const {
        return ( *this - _Other ).Length();
    }
    float NormalizeSelf() {
        const float L = Length();
        if ( L != 0.0f ) {
            const float InvLength = 1.0f / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
        }
        return L;
    }
    Float3 Normalized() const {
        const float L = Length();
        if ( L != 0.0f ) {
            const float InvLength = 1.0f / L;
            return Float3( X * InvLength, Y * InvLength, Z * InvLength );
        }
        return *this;
    }
    Float3 NormalizeFix() const {
        Float3 Normal = Normalized();
        Normal.FixNormal();
        return Normal;
    }
    // Return true if normal was fixed
    bool FixNormal() {
        const float ZERO = 0;
        const float ONE = 1;
        const float MINUS_ONE = -1;

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

        if ( FMath::Abs( X ) == ONE ) {
            if ( Y != ZERO || Z != ZERO ) {
                Y = Z = ZERO;
                return true;
            }
            return false;
        }

        if ( FMath::Abs( Y ) == ONE ) {
            if ( X != ZERO || Z != ZERO ) {
                X = Z = ZERO;
                return true;
            }
            return false;
        }

        if ( FMath::Abs( Z ) == ONE ) {
            if ( X != ZERO || Y != ZERO ) {
                X = Y = ZERO;
                return true;
            }
            return false;
        }

        return false;
    }

    Float3 Floor() const {
        return Float3( FMath::Floor( X ), FMath::Floor( Y ), FMath::Floor( Z ) );
    }

    Float3 Ceil() const {
        return Float3( FMath::Ceil( X ), FMath::Ceil( Y ), FMath::Ceil( Z ) );
    }

    Float3 Fract() const {
        return Float3( FMath::Fract( X ), FMath::Fract( Y ), FMath::Fract( Z ) );
    }

    Float3 Step( const float & _Edge ) const;
    Float3 Step( const Float3 & _Edge ) const;

    Float3 SmoothStep( const float & _Edge0, const float & _Edge1 ) const;
    Float3 SmoothStep( const Float3 & _Edge0, const Float3 & _Edge1 ) const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Float3 Sign() const {
        return Float3( FMath::Sign( X ), FMath::Sign( Y ), FMath::Sign( Z ) );
    }

    int SignBits() const {
        return FMath::SignBits( X ) | (FMath::SignBits( Y )<<1) | (FMath::SignBits( Z )<<2);
    }

    Float3 Lerp( const Float3 & _To, const float & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    static Float3 Lerp( const Float3 & _From, const Float3 & _To, const float & _Mix );

    Float3 Clamp( const float & _Min, const float & _Max ) const {
        return Float3( FMath::Clamp( X, _Min, _Max ), FMath::Clamp( Y, _Min, _Max ), FMath::Clamp( Z, _Min, _Max ) );
    }

    Float3 Clamp( const Float3 & _Min, const Float3 & _Max ) const {
        return Float3( FMath::Clamp( X, _Min.X, _Max.X ), FMath::Clamp( Y, _Min.Y, _Max.Y ), FMath::Clamp( Z, _Min.Z, _Max.Z ) );
    }

    Float3 Saturate() const {
        return Clamp( 0.0f, 1.0f );
    }

    Float3 Snap( const float &_SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        Float3 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = FMath::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = FMath::Round( SnapVector.Y ) * _SnapValue;
        SnapVector.Z = FMath::Round( SnapVector.Z ) * _SnapValue;
        return SnapVector;
    }

    Float3 SnapNormal( const float & _Epsilon ) const {
        Float3 Normal = *this;
        for ( int i = 0 ; i < 3 ; i++ ) {
            if ( FMath::Abs( Normal[i] - 1.0f ) < _Epsilon ) {
                Normal = Float3(0);
                Normal[i] = 1;
                break;
            }
            if ( FMath::Abs( Normal[i] - -1.0f ) < _Epsilon ) {
                Normal = Float3(0);
                Normal[i] = -1;
                break;
            }
        }

        if ( FMath::Abs( Normal[0] ) < _Epsilon && FMath::Abs( Normal[1] ) >= _Epsilon && FMath::Abs( Normal[2] ) >= _Epsilon ) {
            Normal[0] = 0;
            Normal.NormalizeSelf();
        } else if ( FMath::Abs( Normal[1] ) < _Epsilon && FMath::Abs( Normal[0] ) >= _Epsilon && FMath::Abs( Normal[2] ) >= _Epsilon ) {
            Normal[1] = 0;
            Normal.NormalizeSelf();
        } else if ( FMath::Abs( Normal[2] ) < _Epsilon && FMath::Abs( Normal[0] ) >= _Epsilon && FMath::Abs( Normal[1] ) >= _Epsilon ) {
            Normal[2] = 0;
            Normal.NormalizeSelf();
        }

        return Normal;
    }

    int NormalAxialType() const {
        if ( X == 1.0f || X == -1.0f ) return FMath::AxialX;
        if ( Y == 1.0f || Y == -1.0f ) return FMath::AxialY;
        if ( Z == 1.0f || Z == -1.0f ) return FMath::AxialZ;
        return FMath::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == 1.0f ) return FMath::AxialX;
        if ( Y == 1.0f ) return FMath::AxialY;
        if ( Z == 1.0f ) return FMath::AxialZ;
        return FMath::NonAxial;
    }

    int VectorAxialType() const {
        Bool3 Zero = Abs().LessThan( 0.00001f );

        if ( int( Zero.X + Zero.Y + Zero.Z ) != 2 ) {
            return FMath::NonAxial;
        }

        if ( !Zero.X ) {
            return FMath::AxialX;
        }

        if ( !Zero.Y ) {
            return FMath::AxialY;
        }

        if ( !Zero.Z ) {
            return FMath::AxialZ;
        }

        return FMath::NonAxial;
    }

    float Dot( const Float3 & _Other ) const {
        return X * _Other.X + Y * _Other.Y + Z * _Other.Z;
    }
    Float3 Cross( const Float3 & _Other ) const {
        return Float3( Y * _Other.Z - _Other.Y * Z,
                       Z * _Other.X - _Other.Z * X,
                       X * _Other.Y - _Other.X * Y );
    }

    Float3 Perpendicular() const {
        float dp = X * X + Y * Y;
        if ( !dp ) {
            return Float3(1,0,0);
        } else {
            dp = FMath::InvSqrt( dp );
            return Float3( -Y * dp, X * dp, 0 );
        }
    }

    void ComputeBasis( Float3 & _XVec, Float3 & _YVec ) const {
        _YVec = Perpendicular();
        _XVec = _YVec.Cross( *this );
    }

    // String conversions
    FString ToString( int _Precision = FLT_DIG ) const {
        return FString( "( " ) + FMath::ToString( X, _Precision ) + " " + FMath::ToString( Y, _Precision ) + " " + FMath::ToString( Z, _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + FMath::ToHexString( X, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( Z, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
    static const Float3 & Zero() {
        static constexpr Float3 ZeroVec(0.0f);
        return ZeroVec;
    }
};

class Float4 /*final*/ {
public:
    float X;
    float Y;
    float Z;
    float W;

    Float4() = default;
    explicit constexpr Float4( const float & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    explicit Float4( const Float2 & _Value, const float & _Z = 0.0f, const float & _W = 0.0f );
    explicit Float4( const Float3 & _Value, const float & _W = 0.0f );
    constexpr Float4( const float & _X, const float & _Y, const float & _Z, const float & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}
    explicit Float4( const Double4 & _Value );

    float * ToPtr() {
        return &X;
    }

    const float * ToPtr() const {
        return &X;
    }

    Float4 & operator=( const Float4 & _Other ) {
        X = _Other.X;
        Y = _Other.Y;
        Z = _Other.Z;
        W = _Other.W;
        return *this;
    }

    float & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const float & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int _Shuffle >
    Float2 Shuffle2() const;

    template< int _Shuffle >
    Float3 Shuffle3() const;

    template< int _Shuffle >
    Float4 Shuffle4() const;

    Bool operator==( const Float4 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Float4 & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Float4 operator+() const {
        return *this;
    }
    Float4 operator-() const {
        return Float4( -X, -Y, -Z, -W );
    }
    Float4 operator+( const Float4 & _Other ) const {
        return Float4( X + _Other.X, Y + _Other.Y, Z + _Other.Z, W + _Other.W );
    }
    Float4 operator-( const Float4 & _Other ) const {
        return Float4( X - _Other.X, Y - _Other.Y, Z - _Other.Z, W - _Other.W );
    }
    Float4 operator/( const Float4 & _Other ) const {
        return Float4( X / _Other.X, Y / _Other.Y, Z / _Other.Z, W / _Other.W );
    }
    Float4 operator*( const Float4 & _Other ) const {
        return Float4( X * _Other.X, Y * _Other.Y, Z * _Other.Z, W * _Other.W );
    }
    Float4 operator+( const float & _Other ) const {
        return Float4( X + _Other, Y + _Other, Z + _Other, W + _Other );
    }
    Float4 operator-( const float & _Other ) const {
        return Float4( X - _Other, Y - _Other, Z - _Other, W - _Other );
    }
    Float4 operator/( const float & _Other ) const {
        float Denom = 1.0f / _Other;
        return Float4( X * Denom, Y * Denom, Z * Denom, W * Denom );
    }
    Float4 operator*( const float & _Other ) const {
        return Float4( X * _Other, Y * _Other, Z * _Other, W * _Other );
    }
    void operator+=( const Float4 & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
        W += _Other.W;
    }
    void operator-=( const Float4 & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
        W -= _Other.W;
    }
    void operator/=( const Float4 & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
        W /= _Other.W;
    }
    void operator*=( const Float4 & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
        W *= _Other.W;
    }
    void operator+=( const float & _Other ) {
        X += _Other;
        Y += _Other;
        Z += _Other;
        W += _Other;
    }
    void operator-=( const float & _Other ) {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
        W -= _Other;
    }
    void operator/=( const float & _Other ) {
        float Denom = 1.0f / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
        W *= Denom;
    }
    void operator*=( const float & _Other ) {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
        W *= _Other;
    }

    float Min() const {
        return FMath::Min( FMath::Min( FMath::Min( X, Y ), Z ), W );
    }

    float Max() const {
        return FMath::Max( FMath::Max( FMath::Max( X, Y ), Z ), W );
    }

    int MinorAxis() const {
        float Minor = FMath::Abs( X );
        int Axis = 0;
        float Tmp;

        Tmp = FMath::Abs( Y );
        if ( Tmp <= Minor ) {
            Axis = 1;
            Minor = Tmp;
        }

        Tmp = FMath::Abs( Z );
        if ( Tmp <= Minor ) {
            Axis = 2;
            Minor = Tmp;
        }

        Tmp = FMath::Abs( W );
        if ( Tmp <= Minor ) {
            Axis = 3;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const {
        float Major = FMath::Abs( X );
        int Axis = 0;
        float Tmp;

        Tmp = FMath::Abs( Y );
        if ( Tmp > Major ) {
            Axis = 1;
            Major = Tmp;
        }

        Tmp = FMath::Abs( Z );
        if ( Tmp > Major ) {
            Axis = 2;
            Major = Tmp;
        }

        Tmp = FMath::Abs( W );
        if ( Tmp > Major ) {
            Axis = 3;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    Bool4 IsInfinite() const {
        return Bool4( FMath::IsInfinite( X ), FMath::IsInfinite( Y ), FMath::IsInfinite( Z ), FMath::IsInfinite( W ) );
    }

    Bool4 IsNan() const {
        return Bool4( FMath::IsNan( X ), FMath::IsNan( Y ), FMath::IsNan( Z ), FMath::IsNan( W ) );
    }

    Bool4 IsNormal() const {
        return Bool4( FMath::IsNormal( X ), FMath::IsNormal( Y ), FMath::IsNormal( Z ), FMath::IsNormal( W ) );
    }

    Bool4 IsDenormal() const {
        return Bool4( FMath::IsDenormal( X ), FMath::IsDenormal( Y ), FMath::IsDenormal( Z ), FMath::IsDenormal( W ) );
    }

    // Comparision
    Bool4 LessThan( const Float4 & _Other ) const {
        return Bool4( FMath::LessThan( X, _Other.X ), FMath::LessThan( Y, _Other.Y ), FMath::LessThan( Z, _Other.Z ), FMath::LessThan( W, _Other.W ) );
    }
    Bool4 LessThan( const float & _Other ) const {
        return Bool4( FMath::LessThan( X, _Other ), FMath::LessThan( Y, _Other ), FMath::LessThan( Z, _Other ), FMath::LessThan( W, _Other ) );
    }

    Bool4 LequalThan( const Float4 & _Other ) const {
        return Bool4( FMath::LequalThan( X, _Other.X ), FMath::LequalThan( Y, _Other.Y ), FMath::LequalThan( Z, _Other.Z ), FMath::LequalThan( W, _Other.W ) );
    }
    Bool4 LequalThan( const float & _Other ) const {
        return Bool4( FMath::LequalThan( X, _Other ), FMath::LequalThan( Y, _Other ), FMath::LequalThan( Z, _Other ), FMath::LequalThan( W, _Other ) );
    }

    Bool4 GreaterThan( const Float4 & _Other ) const {
        return Bool4( FMath::GreaterThan( X, _Other.X ), FMath::GreaterThan( Y, _Other.Y ), FMath::GreaterThan( Z, _Other.Z ), FMath::GreaterThan( W, _Other.W ) );
    }
    Bool4 GreaterThan( const float & _Other ) const {
        return Bool4( FMath::GreaterThan( X, _Other ), FMath::GreaterThan( Y, _Other ), FMath::GreaterThan( Z, _Other ), FMath::GreaterThan( W, _Other ) );
    }

    Bool4 GequalThan( const Float4 & _Other ) const {
        return Bool4( FMath::GequalThan( X, _Other.X ), FMath::GequalThan( Y, _Other.Y ), FMath::GequalThan( Z, _Other.Z ), FMath::GequalThan( W, _Other.W ) );
    }
    Bool4 GequalThan( const float & _Other ) const {
        return Bool4( FMath::GequalThan( X, _Other ), FMath::GequalThan( Y, _Other ), FMath::GequalThan( Z, _Other ), FMath::GequalThan( W, _Other ) );
    }

    Bool4 NotEqual( const Float4 & _Other ) const {
        return Bool4( FMath::NotEqual( X, _Other.X ), FMath::NotEqual( Y, _Other.Y ), FMath::NotEqual( Z, _Other.Z ), FMath::NotEqual( W, _Other.W ) );
    }
    Bool4 NotEqual( const float & _Other ) const {
        return Bool4( FMath::NotEqual( X, _Other ), FMath::NotEqual( Y, _Other ), FMath::NotEqual( Z, _Other ), FMath::NotEqual( W, _Other ) );
    }

    Bool Compare( const Float4 & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    Bool CompareEps( const Float4 & _Other, const float & _Epsilon ) const {
        return Bool4( FMath::CompareEps( X, _Other.X, _Epsilon ),
                      FMath::CompareEps( Y, _Other.Y, _Epsilon ),
                      FMath::CompareEps( Z, _Other.Z, _Epsilon ),
                      FMath::CompareEps( W, _Other.W, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = Z = W = 0;
    }

    Float4 Abs() const { return Float4( FMath::Abs( X ), FMath::Abs( Y ), FMath::Abs( Z ), FMath::Abs( W ) ); }

    // Vector methods
    float LengthSqr() const {
        return X * X + Y * Y + Z * Z + W * W;
    }
    float Length() const {
        return StdSqrt( LengthSqr() );
    }
    float DistSqr( const Float4 & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }
    float Dist( const Float4 & _Other ) const {
        return ( *this - _Other ).Length();
    }
    float NormalizeSelf() {
        const float L = Length();
        if ( L != 0.0f ) {
            const float InvLength = 1.0f / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
            W *= InvLength;
        }
        return L;
    }
    Float4 Normalized() const {
        const float L = Length();
        if ( L != 0.0f ) {
            const float InvLength = 1.0f / L;
            return Float4( X * InvLength, Y * InvLength, Z * InvLength, W * InvLength );
        }
        return *this;
    }

    Float4 Floor() const {
        return Float4( FMath::Floor( X ), FMath::Floor( Y ), FMath::Floor( Z ), FMath::Floor( W ) );
    }

    Float4 Ceil() const {
        return Float4( FMath::Ceil( X ), FMath::Ceil( Y ), FMath::Ceil( Z ), FMath::Ceil( W ) );
    }

    Float4 Fract() const {
        return Float4( FMath::Fract( X ), FMath::Fract( Y ), FMath::Fract( Z ), FMath::Fract( W ) );
    }

    Float4 Step( const float & _Edge ) const;
    Float4 Step( const Float4 & _Edge ) const;

    Float4 SmoothStep( const float & _Edge0, const float & _Edge1 ) const;
    Float4 SmoothStep( const Float4 & _Edge0, const Float4 & _Edge1 ) const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Float4 Sign() const {
        return Float4( FMath::Sign( X ), FMath::Sign( Y ), FMath::Sign( Z ), FMath::Sign( W ) );
    }

    int SignBits() const {
        return FMath::SignBits( X ) | (FMath::SignBits( Y )<<1) | (FMath::SignBits( Z )<<2) | (FMath::SignBits( W )<<3);
    }

    Float4 Lerp( const Float4 & _To, const float & _Mix ) const {
        return Lerp( *this, _To, _Mix );
    }

    static Float4 Lerp( const Float4 & _From, const Float4 & _To, const float & _Mix );

    Float4 Clamp( const float & _Min, const float & _Max ) const {
        return Float4( FMath::Clamp( X, _Min, _Max ), FMath::Clamp( Y, _Min, _Max ), FMath::Clamp( Z, _Min, _Max ), FMath::Clamp( W, _Min, _Max ) );
    }

    Float4 Clamp( const Float4 & _Min, const Float4 & _Max ) const {
        return Float4( FMath::Clamp( X, _Min.X, _Max.X ), FMath::Clamp( Y, _Min.Y, _Max.Y ), FMath::Clamp( Z, _Min.Z, _Max.Z ), FMath::Clamp( W, _Min.W, _Max.W ) );
    }

    Float4 Saturate() const {
        return Clamp( 0.0f, 1.0f );
    }

    Float4 Snap( const float &_SnapValue ) const {
        AN_ASSERT( _SnapValue > 0, "Snap" );
        Float4 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = FMath::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = FMath::Round( SnapVector.Y ) * _SnapValue;
        SnapVector.Z = FMath::Round( SnapVector.Z ) * _SnapValue;
        SnapVector.W = FMath::Round( SnapVector.W ) * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const {
        if ( X == 1.0f || X == -1.0f ) return FMath::AxialX;
        if ( Y == 1.0f || Y == -1.0f ) return FMath::AxialY;
        if ( Z == 1.0f || Z == -1.0f ) return FMath::AxialZ;
        if ( W == 1.0f || W == -1.0f ) return FMath::AxialW;
        return FMath::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == 1.0f ) return FMath::AxialX;
        if ( Y == 1.0f ) return FMath::AxialY;
        if ( Z == 1.0f ) return FMath::AxialZ;
        if ( W == 1.0f ) return FMath::AxialW;
        return FMath::NonAxial;
    }

    int VectorAxialType() const {
        Bool4 Zero = Abs().LessThan( 0.00001f );

        if ( int( Zero.X + Zero.Y + Zero.Z + Zero.W ) != 3 ) {
            return FMath::NonAxial;
        }

        if ( !Zero.X ) {
            return FMath::AxialX;
        }

        if ( !Zero.Y ) {
            return FMath::AxialY;
        }

        if ( !Zero.Z ) {
            return FMath::AxialZ;
        }

        if ( !Zero.W ) {
            return FMath::AxialW;
        }

        return FMath::NonAxial;
    }

    float Dot( const Float4 & _Other ) const {
        return X * _Other.X + Y * _Other.Y + Z * _Other.Z + W * _Other.W;
    }

    // String conversions
    FString ToString( int _Precision = FLT_DIG ) const {
        return FString( "( " ) + FMath::ToString( X, _Precision ) + " " + FMath::ToString( Y, _Precision ) + " " + FMath::ToString( Z, _Precision ) + " " + FMath::ToString( W, _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + FMath::ToHexString( X, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( Z, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( W, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
    static const Float4 & Zero() {
        static constexpr Float4 ZeroVec(0.0f);
        return ZeroVec;
    }
};

// Type conversion

AN_FORCEINLINE constexpr Float::Float( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const ULong & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const Float & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Float::Float( const Double & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr Double::Double( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const ULong & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const Float & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Double::Double( const Double & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE Double & Double::operator=( const Float & _Other ) {
    Value = _Other;
    return *this;
}

// Float to SignedByte
AN_FORCEINLINE constexpr SignedByte::SignedByte( const Float & _Value ) : Value( _Value.Value ) {}

// Float to Byte
AN_FORCEINLINE constexpr Byte::Byte( const Float & _Value ) : Value( _Value.Value ) {}

// Float to Short
AN_FORCEINLINE constexpr Short::Short( const Float & _Value ) : Value( _Value.Value ) {}

// Float to UShort
AN_FORCEINLINE constexpr UShort::UShort( const Float & _Value ) : Value( _Value.Value ) {}

// Float to Int
AN_FORCEINLINE constexpr Int::Int( const Float & _Value ) : Value( _Value.Value ) {}

// Float to UInt
AN_FORCEINLINE constexpr UInt::UInt( const Float & _Value ) : Value( _Value.Value ) {}

// Float to Long
AN_FORCEINLINE constexpr Long::Long( const Float & _Value ) : Value( _Value.Value ) {}

// Float to ULong
AN_FORCEINLINE constexpr ULong::ULong( const Float & _Value ) : Value( _Value.Value ) {}

// Float to SignedByte
AN_FORCEINLINE constexpr SignedByte::SignedByte( const Double & _Value ) : Value( _Value.Value ) {}

// Float to Byte
AN_FORCEINLINE constexpr Byte::Byte( const Double & _Value ) : Value( _Value.Value ) {}

// Double to Short
AN_FORCEINLINE constexpr Short::Short( const Double & _Value ) : Value( _Value.Value ) {}

// Double to UShort
AN_FORCEINLINE constexpr UShort::UShort( const Double & _Value ) : Value( _Value.Value ) {}

// Double to Int
AN_FORCEINLINE constexpr Int::Int( const Double & _Value ) : Value( _Value.Value ) {}

// Double to UInt
AN_FORCEINLINE constexpr UInt::UInt( const Double & _Value ) : Value( _Value.Value ) {}

// Double to Long
AN_FORCEINLINE constexpr Long::Long( const Double & _Value ) : Value( _Value.Value ) {}

// Double to ULong
AN_FORCEINLINE constexpr ULong::ULong( const Double & _Value ) : Value( _Value.Value ) {}

// Double to Float
AN_FORCEINLINE Float & Float::operator=( const Double & _Other ) {
    Value = _Other;
    return *this;
}

// Float3 to Float2
AN_FORCEINLINE Float2::Float2( const Float3 & _Value ) : X( _Value.X ), Y( _Value.Y ) {}
// Float4 to Float2
AN_FORCEINLINE Float2::Float2( const Float4 & _Value ) : X( _Value.X ), Y( _Value.Y ) {}

// Float4 to Float3
AN_FORCEINLINE Float3::Float3( const Float4 & _Value ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}
// Float2 to Float3
AN_FORCEINLINE Float3::Float3( const Float2 & _Value, const float & _Z ) : X( _Value.X ), Y( _Value.Y ), Z( _Z ) {}

// Float2 to Float4
AN_FORCEINLINE Float4::Float4( const Float2 & _Value, const float & _Z, const float & _W ) : X( _Value.X ), Y( _Value.Y ), Z( _Z ), W( _W ) {}
// Float3 to Float4
AN_FORCEINLINE Float4::Float4( const Float3 & _Value, const float & _W ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ), W( _W ) {}

// Float2 to Double2
AN_FORCEINLINE Float2::Float2( const Double2 & _Value ) : X( _Value.X ), Y( _Value.Y ) {}
// Float3 to Double3
AN_FORCEINLINE Float3::Float3( const Double3 & _Value ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}
// Float4 to Double4
AN_FORCEINLINE Float4::Float4( const Double4 & _Value ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ), W( _Value.W ) {}

// Double2 to Float2
AN_FORCEINLINE Double2::Double2( const Float2 & _Value ) : X( _Value.X ), Y( _Value.Y ) {}
// Double3 to Float3
AN_FORCEINLINE Double3::Double3( const Float3 & _Value ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}
// Double4 to Float4
AN_FORCEINLINE Double4::Double4( const Float4 & _Value ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ), W( _Value.W ) {}

// Float2 Shuffle

template< int _Shuffle >
Float2 Float2::Shuffle2() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    return Float2( (&X)[iX], (&X)[iY] );
}

template< int _Shuffle >
Float3 Float2::Shuffle3() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    return Float3( (&X)[iX], (&X)[iY], (&X)[iZ] );
}

template< int _Shuffle >
Float4 Float2::Shuffle4() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    constexpr int iW = _Shuffle & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    static_assert( iW >= 0 && iW < NumComponents(), "Index out of range" );
    return Float4( (&X)[iX], (&X)[iY], (&X)[iZ], (&X)[iW] );
}

// Float3 Shuffle

template< int _Shuffle >
Float2 Float3::Shuffle2() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    return Float2( (&X)[iX], (&X)[iY] );
}

template< int _Shuffle >
Float3 Float3::Shuffle3() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    return Float3( (&X)[iX], (&X)[iY], (&X)[iZ] );
}

template< int _Shuffle >
Float4 Float3::Shuffle4() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    constexpr int iW = _Shuffle & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    static_assert( iW >= 0 && iW < NumComponents(), "Index out of range" );
    return Float4( (&X)[iX], (&X)[iY], (&X)[iZ], (&X)[iW] );
}

// Float4 Shuffle

template< int _Shuffle >
Float2 Float4::Shuffle2() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    return Float2( (&X)[iX], (&X)[iY] );
}

template< int _Shuffle >
Float3 Float4::Shuffle3() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    return Float3( (&X)[iX], (&X)[iY], (&X)[iZ] );
}

template< int _Shuffle >
Float4 Float4::Shuffle4() const {
    constexpr int iX = _Shuffle >> 6;
    constexpr int iY = ( _Shuffle >> 4 ) & 3;
    constexpr int iZ = ( _Shuffle >> 2 ) & 3;
    constexpr int iW = _Shuffle & 3;
    static_assert( iX >= 0 && iX < NumComponents(), "Index out of range" );
    static_assert( iY >= 0 && iY < NumComponents(), "Index out of range" );
    static_assert( iZ >= 0 && iZ < NumComponents(), "Index out of range" );
    static_assert( iW >= 0 && iW < NumComponents(), "Index out of range" );
    return Float4( (&X)[iX], (&X)[iY], (&X)[iZ], (&X)[iW] );
}

#if 0
// Float::operator==
AN_FORCEINLINE bool operator==( const Float & _Left, const float & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const float & _Left, const Float & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const Float & _Left, const Float & _Right ) { return _Left.Value == _Right.Value; }

// Float::operator!=
AN_FORCEINLINE bool operator!=( const Float & _Left, const float & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const float & _Left, const Float & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const Float & _Left, const Float & _Right ) { return _Left.Value != _Right.Value; }

// Float::operator<
AN_FORCEINLINE bool operator<( const Float & _Left, const float & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const float & _Left, const Float & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const Float & _Left, const Float & _Right ) { return _Left.Value < _Right.Value; }

// Float::operator>
AN_FORCEINLINE bool operator>( const Float & _Left, const float & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const float & _Left, const Float & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const Float & _Left, const Float & _Right ) { return _Left.Value > _Right.Value; }

// Float::operator<=
AN_FORCEINLINE bool operator<=( const Float & _Left, const float & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const float & _Left, const Float & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const Float & _Left, const Float & _Right ) { return _Left.Value <= _Right.Value; }

// Float::operator>=
AN_FORCEINLINE bool operator>=( const Float & _Left, const float & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const float & _Left, const Float & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const Float & _Left, const Float & _Right ) { return _Left.Value >= _Right.Value; }
#endif

//AN_FORCEINLINE Float operator+( const float & _Left, const Float & _Right ) {
//    return _Left + _Right.Value;
//}
//AN_FORCEINLINE Float operator-( const float & _Left, const Float & _Right ) {
//    return _Left - _Right.Value;
//}
AN_FORCEINLINE Float2 operator+( const float & _Left, const Float2 & _Right ) {
    return Float2( _Left + _Right.X, _Left + _Right.Y );
}
AN_FORCEINLINE Float2 operator-( const float & _Left, const Float2 & _Right ) {
    return Float2( _Left - _Right.X, _Left - _Right.Y );
}
AN_FORCEINLINE Float3 operator+( const float & _Left, const Float3 & _Right ) {
    return Float3( _Left + _Right.X, _Left + _Right.Y, _Left + _Right.Z );
}
AN_FORCEINLINE Float3 operator-( const float & _Left, const Float3 & _Right ) {
    return Float3( _Left - _Right.X, _Left - _Right.Y, _Left - _Right.Z );
}
AN_FORCEINLINE Float4 operator+( const float & _Left, const Float4 & _Right ) {
    return Float4( _Left + _Right.X, _Left + _Right.Y, _Left + _Right.Z, _Left + _Right.W );
}
AN_FORCEINLINE Float4 operator-( const float & _Left, const Float4 & _Right ) {
    return Float4( _Left - _Right.X, _Left - _Right.Y, _Left - _Right.Z, _Left - _Right.W );
}

//AN_FORCEINLINE Float operator*( const float & _Left, const Float & _Right ) {
//    return _Left * _Right;
//}
//AN_FORCEINLINE Float operator/( const float & _Left, const Float & _Right ) {
//    return _Left / _Right;
//}
AN_FORCEINLINE Float2 operator*( const float & _Left, const Float2 & _Right ) {
    return Float2( _Left * _Right.X, _Left * _Right.Y );
}
AN_FORCEINLINE Float3 operator*( const float & _Left, const Float3 & _Right ) {
    return Float3( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z );
}
AN_FORCEINLINE Float4 operator*( const float & _Left, const Float4 & _Right ) {
    return Float4( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z, _Left * _Right.W );
}

AN_FORCEINLINE Float2 Float2::Lerp( const Float2 & _From, const Float2 & _To, const float & _Mix ) {
    return _From + _Mix * ( _To - _From );
}
AN_FORCEINLINE float Float2::Bilerp( const float & _A, const float & _B, const float & _C, const float & _D ) const {
    return _A * ( 1.0f - X ) * ( 1.0f - Y ) + _B * X * ( 1.0f - Y ) + _C * ( 1.0f - X ) * Y + _D * X * Y;
}
AN_FORCEINLINE Float2 Float2::Bilerp( const Float2 & _A, const Float2 & _B, const Float2 & _C, const Float2 & _D ) const {
    return _A * ( 1.0f - X ) * ( 1.0f - Y ) + _B * X * ( 1.0f - Y ) + _C * ( 1.0f - X ) * Y + _D * X * Y;
}
AN_FORCEINLINE Float3 Float2::Bilerp( const Float3 & _A, const Float3 & _B, const Float3 & _C, const Float3 & _D ) const {
    return _A * ( 1.0f - X ) * ( 1.0f - Y ) + _B * X * ( 1.0f - Y ) + _C * ( 1.0f - X ) * Y + _D * X * Y;
}
AN_FORCEINLINE Float4 Float2::Bilerp( const Float4 & _A, const Float4 & _B, const Float4 & _C, const Float4 & _D ) const {
    return _A * ( 1.0f - X ) * ( 1.0f - Y ) + _B * X * ( 1.0f - Y ) + _C * ( 1.0f - X ) * Y + _D * X * Y;
}

AN_FORCEINLINE Float3 Float3::Lerp( const Float3 & _From, const Float3 & _To, const float & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

AN_FORCEINLINE Float4 Float4::Lerp( const Float4 & _From, const Float4 & _To, const float & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

AN_FORCEINLINE Float2 Float2::Step( const float & _Edge ) const {
    return Float2( X < _Edge ? 0.0f : 1.0f, Y < _Edge ? 0.0f : 1.0f );
}

AN_FORCEINLINE Float2 Float2::Step( const Float2 & _Edge ) const {
    return Float2( X < _Edge.X ? 0.0f : 1.0f, Y < _Edge.Y ? 0.0f : 1.0f );
}

AN_FORCEINLINE Float2 Float2::SmoothStep( const float & _Edge0, const float & _Edge1 ) const {
    float Denom = 1.0f / ( _Edge1 - _Edge0 );
    const Float2 t = ( ( *this - _Edge0 ) * Denom ).Saturate();
    return t * t * ( ( -2.0f ) * t + 3.0f );
}

AN_FORCEINLINE Float2 Float2::SmoothStep( const Float2 & _Edge0, const Float2 & _Edge1 ) const {
    const Float2 t = ( ( *this - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
    return t * t * ( ( -2.0f ) * t + 3.0f );
}

AN_FORCEINLINE Float3 Float3::Step( const float & _Edge ) const {
    return Float3( X < _Edge ? 0.0f : 1.0f, Y < _Edge ? 0.0f : 1.0f, Z < _Edge ? 0.0f : 1.0f );
}

AN_FORCEINLINE Float3 Float3::Step( const Float3 & _Edge ) const {
    return Float3( X < _Edge.X ? 0.0f : 1.0f, Y < _Edge.Y ? 0.0f : 1.0f, Z < _Edge.Z ? 0.0f : 1.0f );
}

AN_FORCEINLINE Float3 Float3::SmoothStep( const float & _Edge0, const float & _Edge1 ) const {
    float Denom = 1.0f / ( _Edge1 - _Edge0 );
    const Float3 t = ( ( *this - _Edge0 ) * Denom ).Saturate();
    return t * t * ( ( -2.0f ) * t + 3.0f );
}

AN_FORCEINLINE Float3 Float3::SmoothStep( const Float3 & _Edge0, const Float3 & _Edge1 ) const {
    const Float3 t = ( ( *this - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
    return t * t * ( ( -2.0f ) * t + 3.0f );
}

AN_FORCEINLINE Float4 Float4::Step( const float & _Edge ) const {
    return Float4( X < _Edge ? 0.0f : 1.0f, Y < _Edge ? 0.0f : 1.0f, Z < _Edge ? 0.0f : 1.0f, W < _Edge ? 0.0f : 1.0f );
}

AN_FORCEINLINE Float4 Float4::Step( const Float4 & _Edge ) const {
    return Float4( X < _Edge.X ? 0.0f : 1.0f, Y < _Edge.Y ? 0.0f : 1.0f, Z < _Edge.Z ? 0.0f : 1.0f, W < _Edge.W ? 0.0f : 1.0f );
}

AN_FORCEINLINE Float4 Float4::SmoothStep( const float & _Edge0, const float & _Edge1 ) const {
    float Denom = 1.0f / ( _Edge1 - _Edge0 );
    const Float4 t = ( ( *this - _Edge0 ) * Denom ).Saturate();
    return t * t * ( ( -2.0f ) * t + 3.0f );
}

AN_FORCEINLINE Float4 Float4::SmoothStep( const Float4 & _Edge0, const Float4 & _Edge1 ) const {
    const Float4 t = ( ( *this - _Edge0 ) / ( _Edge1 - _Edge0 ) ).Saturate();
    return t * t * ( ( -2.0f ) * t + 3.0f );
}

AN_FORCEINLINE Float Float::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    uint32_t Val = ToIntFast() - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast< float >( Val + 1 );
}

AN_FORCEINLINE Float Float::ToLessPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    uint32_t Val = ToIntFast();
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast< float >( Val - ( Val >> 1 ) );
}

AN_FORCEINLINE Float Float::ToClosestPowerOfTwo() const {
    Float GreaterPow = ToGreaterPowerOfTwo();
    Float LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

namespace FMath {

AN_FORCEINLINE float Dot( const Float2 & _Left, const Float2 & _Right ) { return _Left.Dot( _Right ); }
AN_FORCEINLINE float Dot( const Float3 & _Left, const Float3 & _Right ) { return _Left.Dot( _Right ); }
AN_FORCEINLINE float Dot( const Float4 & _Left, const Float4 & _Right ) { return _Left.Dot( _Right ); }

AN_FORCEINLINE Float3 Cross( const Float3 & _Left, const Float3 & _Right ) { return _Left.Cross( _Right ); }

AN_FORCEINLINE constexpr float Degrees( const float & _Rad ) { return _Rad * _RAD2DEG; }
AN_FORCEINLINE constexpr float Radians( const float & _Deg ) { return _Deg * _DEG2RAD; }

AN_FORCEINLINE /*constexpr*/ float RadSin( const float & _Rad ) { return std::sin( _Rad ); }
AN_FORCEINLINE /*constexpr*/ float RadCos( const float & _Rad ) { return std::cos( _Rad ); }
AN_FORCEINLINE /*constexpr*/ float DegSin( const float & _Deg ) { return std::sin( Radians( _Deg ) ); }
AN_FORCEINLINE /*constexpr*/ float DegCos( const float & _Deg ) { return std::cos( Radians( _Deg ) ); }

AN_FORCEINLINE void RadSinCos( const float & _Rad, float & _Sin, float & _Cos ) {
#if defined(_WIN32) && !defined(_WIN64)
    _asm {
        fld  _Rad
        fsincos
        mov  ecx, _Cos
        mov  edx, _Sin
        fstp dword ptr [ecx]
        fstp dword ptr [edx]
    }
#else
    _Sin = sinf( _Rad );
    _Cos = cosf( _Rad );
#endif
}

AN_FORCEINLINE void DegSinCos( const float & _Deg, float & _Sin, float & _Cos ) {
    RadSinCos( Radians( _Deg ), _Sin, _Cos );
}

AN_INLINE float GreaterCommonDivisor( float m, float n ) {
    return ( m < 0.0001f ) ? n : GreaterCommonDivisor( std::fmod( n, m ), m );
}

template< typename T >
AN_FORCEINLINE T HermiteCubicSpline( T const & p0, T const & m0, T const & p1, T const & m1, float t ) {
    const float tt = t * t;
    const float ttt = tt * t;
    const float s2 = -2 * ttt + 3 * tt;
    const float s3 = ttt - tt;
    const float s0 = 1 - s2;
    const float s1 = s3 - tt + t;
    return s0 * p0 + s1 * m0 * t + s2 * p1 + s3 * m1 * t;
}

}

// Column-major matrix 2x2
class Float2x2 final {
public:
    Float2 Col0;
    Float2 Col1;

    Float2x2() = default;
    explicit Float2x2( const Float3x3 & _Value );
    explicit Float2x2( const Float3x4 & _Value );
    explicit Float2x2( const Float4x4 & _Value );
    constexpr Float2x2( const Float2 & _Col0, const Float2 & _Col1 ) : Col0( _Col0 ), Col1( _Col1 ) {}
    constexpr Float2x2( const float & _00, const float & _01,
                        const float & _10, const float & _11 )
        : Col0( _00, _01 ), Col1( _10, _11 ) {}
    constexpr explicit Float2x2( const float & _Diagonal ) : Col0( _Diagonal, 0 ), Col1( 0, _Diagonal ) {}
    constexpr explicit Float2x2( const Float2 & _Diagonal ) : Col0( _Diagonal.X, 0 ), Col1( 0, _Diagonal.Y ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

    Float2x2 & operator=( const Float2x2 & _Other ) {
        Col0 = _Other.Col0;
        Col1 = _Other.Col1;
        return *this;
    }

    Float2 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Float2 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float2 GetRow( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 2, "Index out of range" );
        return Float2( Col0[ _Index ], Col1[ _Index ] );
    }

    Bool operator==( const Float2x2 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Float2x2 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Float2x2 & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 4 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Float2x2 & _Other, const float & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 4 ; i++ ) {
            if ( FMath::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf() {
        float Temp = Col0.Y;
        Col0.Y = Col1.X;
        Col1.X = Temp;
    }

    Float2x2 Transposed() const {
        return Float2x2( Col0.X, Col1.X, Col0.Y, Col1.Y );
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Float2x2 Inversed() const {
        const float OneOverDeterminant = 1.0f / ( Col0[0] * Col1[1] - Col1[0] * Col0[1] );
        return Float2x2( Col1[1] * OneOverDeterminant,
                    -Col0[1] * OneOverDeterminant,
                    -Col1[0] * OneOverDeterminant,
                     Col0[0] * OneOverDeterminant );
    }

    float Determinant() const {
        return Col0[0] * Col1[1] - Col1[0] * Col0[1];
    }

    void Clear() {
        ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        Col0.Y = Col1.X = 0;
        Col0.X = Col1.Y = 1;
    }

    static Float2x2 Scale( const Float2 & _Scale  ) {
        return Float2x2( _Scale );
    }

    Float2x2 Scaled( const Float2 & _Scale ) const {
        return Float2x2( Col0 * _Scale[0], Col1 * _Scale[1] );
    }

    // Return rotation around Z axis
    static Float2x2 Rotation( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float2x2( c, s,
                        -s, c );
    }

    Float2x2 operator*( const float & _Value ) const {
        return Float2x2( Col0 * _Value,
                         Col1 * _Value );
    }

    void operator*=( const float & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
    }

    Float2x2 operator/( const float & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float2x2( Col0 * OneOverValue,
                         Col1 * OneOverValue );
    }

    void operator/=( const float & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
    }

    Float2 operator*( const Float2 & _Vec ) const {
        return Float2( Col0[0] * _Vec.X + Col1[0] * _Vec.Y,
                       Col0[1] * _Vec.X + Col1[1] * _Vec.Y );
    }

    Float2x2 operator*( const Float2x2 & _Mat ) const {
        const float & L00 = Col0[0];
        const float & L01 = Col0[1];
        const float & L10 = Col1[0];
        const float & L11 = Col1[1];
        const float & R00 = _Mat[0][0];
        const float & R01 = _Mat[0][1];
        const float & R10 = _Mat[1][0];
        const float & R11 = _Mat[1][1];
        return Float2x2( L00 * R00 + L10 * R01,
                         L01 * R00 + L11 * R01,
                         L00 * R10 + L10 * R11,
                         L01 * R10 + L11 * R11 );
    }

    void operator*=( const Float2x2 & _Mat ) {
        //*this = *this * _Mat;

        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float & R00 = _Mat[0][0];
        const float & R01 = _Mat[0][1];
        const float & R10 = _Mat[1][0];
        const float & R11 = _Mat[1][1];
        Col0[0] = L00 * R00 + L10 * R01;
        Col0[1] = L01 * R00 + L11 * R01;
        Col1[0] = L00 * R10 + L10 * R11;
        Col1[1] = L01 * R10 + L11 * R11;
    }

    // String conversions
    FString ToString( int _Precision = FLT_DIG ) const {
        return FString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Col0 << Col1;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Col0;
        _Stream >> Col1;
    }

    // Static methods

    static const Float2x2 & Identity() {
        static constexpr Float2x2 IdentityMat( 1 );
        return IdentityMat;
    }

};

// Column-major matrix 3x3
class Float3x3 final {
public:
    Float3 Col0;
    Float3 Col1;
    Float3 Col2;

    Float3x3() = default;
    explicit Float3x3( const Float2x2 & _Value );
    explicit Float3x3( const Float3x4 & _Value );
    explicit Float3x3( const Float4x4 & _Value );
    constexpr Float3x3( const Float3 & _Col0, const Float3 & _Col1, const Float3 & _Col2 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Float3x3( const float & _00, const float & _01, const float & _02,
          const float & _10, const float & _11, const float & _12,
          const float & _20, const float & _21, const float & _22 )
        : Col0( _00, _01, _02 ), Col1( _10, _11, _12 ), Col2( _20, _21, _22 ) {}
    constexpr explicit Float3x3( const float & _Diagonal ) : Col0( _Diagonal, 0, 0 ), Col1( 0, _Diagonal, 0 ), Col2( 0, 0, _Diagonal ) {}
    constexpr explicit Float3x3( const Float3 & _Diagonal ) : Col0( _Diagonal.X, 0, 0 ), Col1( 0, _Diagonal.Y, 0 ), Col2( 0, 0, _Diagonal.Z ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

    Float3x3 & operator=( const Float3x3 & _Other ) {
        //Col0 = _Other.Col0;
        //Col1 = _Other.Col1;
        //Col2 = _Other.Col2;
        memcpy( this, &_Other, sizeof( *this ) );
        return *this;
    }

    Float3 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Float3 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float3 GetRow( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return Float3( Col0[ _Index ], Col1[ _Index ], Col2[ _Index ] );
    }

    Bool operator==( const Float3x3 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Float3x3 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Float3x3 & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 9 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Float3x3 & _Other, const float & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 9 ; i++ ) {
            if ( FMath::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf() {
        float Temp = Col0.Y;
        Col0.Y = Col1.X;
        Col1.X = Temp;
        Temp = Col0.Z;
        Col0.Z = Col2.X;
        Col2.X = Temp;
        Temp = Col1.Z;
        Col1.Z = Col2.Y;
        Col2.Y = Temp;
    }

    Float3x3 Transposed() const {
        return Float3x3( Col0.X, Col1.X, Col2.X, Col0.Y, Col1.Y, Col2.Y, Col0.Z, Col1.Z, Col2.Z );
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Float3x3 Inversed() const {
        const Float3x3 & m = *this;
        const float A = m[1][1] * m[2][2] - m[2][1] * m[1][2];
        const float B = m[0][1] * m[2][2] - m[2][1] * m[0][2];
        const float C = m[0][1] * m[1][2] - m[1][1] * m[0][2];
        const float OneOverDeterminant = 1.0f / ( m[0][0] * A - m[1][0] * B + m[2][0] * C );

        Float3x3 Inversed;
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

    float Determinant() const {
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

    static Float3x3 Scale( const Float3 & _Scale  ) {
        return Float3x3( _Scale );
    }

    Float3x3 Scaled( const Float3 & _Scale ) const {
        return Float3x3( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2] );
    }

    // Return rotation around normalized axis
    static Float3x3 RotationAroundNormal( const float & _AngleRad, const Float3 & _Normal ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        const Float3 Temp = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float3x3( c + Temp[0] * _Normal[0],                Temp[0] * _Normal[1] + Temp2[2],     Temp[0] * _Normal[2] - Temp2[1],
                             Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1],                Temp[1] * _Normal[2] + Temp2[0],
                             Temp[2] * _Normal[0] + Temp2[1],     Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2] );
    }

    // Return rotation around normalized axis
    Float3x3 RotateAroundNormal( const float & _AngleRad, const Float3 & _Normal ) const {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        Float3 Temp = ( 1.0f - c ) * _Normal;
        Float3 Temp2 = s * _Normal;
        return Float3x3( Col0 * (c + Temp[0] * _Normal[0])            + Col1 * (    Temp[0] * _Normal[1] + Temp2[2]) + Col2 * (    Temp[0] * _Normal[2] - Temp2[1]),
                         Col0 * (    Temp[1] * _Normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * _Normal[1])            + Col2 * (    Temp[1] * _Normal[2] + Temp2[0]),
                         Col0 * (    Temp[2] * _Normal[0] + Temp2[1]) + Col1 * (    Temp[2] * _Normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * _Normal[2]) );
    }

    // Return rotation around unnormalized vector
    static Float3x3 RotationAroundVector( const float & _AngleRad, const Float3 & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Float3x3 RotateAroundVector( const float & _AngleRad, const Float3 & _Vector ) const {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float3x3 RotationX( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float3x3( 1, 0, 0,
                         0, c, s,
                         0,-s, c );
    }

    // Return rotation around Y axis
    static Float3x3 RotationY( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float3x3( c, 0,-s,
                         0, 1, 0,
                         s, 0, c );
    }

    // Return rotation around Z axis
    static Float3x3 RotationZ( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float3x3( c, s, 0,
                        -s, c, 0,
                         0, 0, 1 );
    }

    Float3x3 operator*( const float & _Value ) const {
        return Float3x3( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( const float & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Float3x3 operator/( const float & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float3x3( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue );
    }

    void operator/=( const float & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    Float3 operator*( const Float3 & _Vec ) const {
        return Float3( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z,
                       Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z,
                       Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z );
    }

    Float3x3 operator*( const Float3x3 & _Mat ) const {
        const float & L00 = Col0[0];
        const float & L01 = Col0[1];
        const float & L02 = Col0[2];
        const float & L10 = Col1[0];
        const float & L11 = Col1[1];
        const float & L12 = Col1[2];
        const float & L20 = Col2[0];
        const float & L21 = Col2[1];
        const float & L22 = Col2[2];
        const float & R00 = _Mat[0][0];
        const float & R01 = _Mat[0][1];
        const float & R02 = _Mat[0][2];
        const float & R10 = _Mat[1][0];
        const float & R11 = _Mat[1][1];
        const float & R12 = _Mat[1][2];
        const float & R20 = _Mat[2][0];
        const float & R21 = _Mat[2][1];
        const float & R22 = _Mat[2][2];
        return Float3x3( L00 * R00 + L10 * R01 + L20 * R02,
                         L01 * R00 + L11 * R01 + L21 * R02,
                         L02 * R00 + L12 * R01 + L22 * R02,
                         L00 * R10 + L10 * R11 + L20 * R12,
                         L01 * R10 + L11 * R11 + L21 * R12,
                         L02 * R10 + L12 * R11 + L22 * R12,
                         L00 * R20 + L10 * R21 + L20 * R22,
                         L01 * R20 + L11 * R21 + L21 * R22,
                         L02 * R20 + L12 * R21 + L22 * R22 );
    }

    void operator*=( const Float3x3 & _Mat ) {
        //*this = *this * _Mat;

        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L02 = Col0[2];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float L12 = Col1[2];
        const float L20 = Col2[0];
        const float L21 = Col2[1];
        const float L22 = Col2[2];
        const float & R00 = _Mat[0][0];
        const float & R01 = _Mat[0][1];
        const float & R02 = _Mat[0][2];
        const float & R10 = _Mat[1][0];
        const float & R11 = _Mat[1][1];
        const float & R12 = _Mat[1][2];
        const float & R20 = _Mat[2][0];
        const float & R21 = _Mat[2][1];
        const float & R22 = _Mat[2][2];
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

    AN_FORCEINLINE Float3x3 ViewInverseFast() const {
        return Transposed();
    }

    // String conversions
    FString ToString( int _Precision = FLT_DIG ) const {
        return FString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Col0 << Col1 << Col2;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Col0;
        _Stream >> Col1;
        _Stream >> Col2;
    }

    // Static methods

    static const Float3x3 & Identity() {
        static constexpr Float3x3 IdentityMat( 1 );
        return IdentityMat;
    }

};

// Column-major matrix 4x4
class Float4x4 final {
public:
    Float4 Col0;
    Float4 Col1;
    Float4 Col2;
    Float4 Col3;

    Float4x4() = default;
    explicit Float4x4( const Float2x2 & _Value );
    explicit Float4x4( const Float3x3 & _Value );
    explicit Float4x4( const Float3x4 & _Value );
    constexpr Float4x4( const Float4 & _Col0, const Float4 & _Col1, const Float4 & _Col2, const Float4 & _Col3 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ), Col3( _Col3 ) {}
    constexpr Float4x4( const float & _00, const float & _01, const float & _02, const float & _03,
                        const float & _10, const float & _11, const float & _12, const float & _13,
                        const float & _20, const float & _21, const float & _22, const float & _23,
                        const float & _30, const float & _31, const float & _32, const float & _33 )
        : Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ), Col3( _30, _31, _32, _33 ) {}
    constexpr explicit Float4x4( const float & _Diagonal ) : Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ), Col3( 0, 0, 0, _Diagonal ) {}
    constexpr explicit Float4x4( const Float4 & _Diagonal ) : Col0( _Diagonal.X, 0, 0, 0 ), Col1( 0, _Diagonal.Y, 0, 0 ), Col2( 0, 0, _Diagonal.Z, 0 ), Col3( 0, 0, 0, _Diagonal.W ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

    Float4x4 & operator=( const Float4x4 & _Other ) {
        //Col0 = _Other.Col0;
        //Col1 = _Other.Col1;
        //Col2 = _Other.Col2;
        //Col3 = _Other.Col3;
        memcpy( this, &_Other, sizeof( *this ) );
        return *this;
    }

    Float4 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Float4 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float4 GetRow( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return Float4( Col0[ _Index ], Col1[ _Index ], Col2[ _Index ], Col3[ _Index ] );
    }

    Bool operator==( const Float4x4 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Float4x4 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Float4x4 & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 16 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Float4x4 & _Other, const float & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 16 ; i++ ) {
            if ( FMath::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf() {
        float Temp = Col0.Y;
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

    Float4x4 Transposed() const {
        return Float4x4( Col0.X, Col1.X, Col2.X, Col3.X,
                         Col0.Y, Col1.Y, Col2.Y, Col3.Y,
                         Col0.Z, Col1.Z, Col2.Z, Col3.Z,
                         Col0.W, Col1.W, Col2.W, Col3.W );
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Float4x4 Inversed() const {
        const Float4x4 & m = *this;

        const float Coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        const float Coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        const float Coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

        const float Coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        const float Coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
        const float Coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

        const float Coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        const float Coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
        const float Coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

        const float Coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        const float Coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
        const float Coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

        const float Coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        const float Coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
        const float Coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

        const float Coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        const float Coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
        const float Coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

        const Float4 Fac0(Coef00, Coef00, Coef02, Coef03);
        const Float4 Fac1(Coef04, Coef04, Coef06, Coef07);
        const Float4 Fac2(Coef08, Coef08, Coef10, Coef11);
        const Float4 Fac3(Coef12, Coef12, Coef14, Coef15);
        const Float4 Fac4(Coef16, Coef16, Coef18, Coef19);
        const Float4 Fac5(Coef20, Coef20, Coef22, Coef23);

        const Float4 Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
        const Float4 Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
        const Float4 Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
        const Float4 Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

        const Float4 Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
        const Float4 Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
        const Float4 Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
        const Float4 Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

        const Float4 SignA(+1, -1, +1, -1);
        const Float4 SignB(-1, +1, -1, +1);
        Float4x4 Inversed(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

        const Float4 Row0(Inversed[0][0], Inversed[1][0], Inversed[2][0], Inversed[3][0]);

        const Float4 Dot0(m[0] * Row0);
        const float Dot1 = (Dot0.X + Dot0.Y) + (Dot0.Z + Dot0.W);

        const float OneOverDeterminant = 1.0f / Dot1;

        return Inversed * OneOverDeterminant;
    }

    float Determinant() const {
        const float SubFactor00 = Col2[2] * Col3[3] - Col3[2] * Col2[3];
        const float SubFactor01 = Col2[1] * Col3[3] - Col3[1] * Col2[3];
        const float SubFactor02 = Col2[1] * Col3[2] - Col3[1] * Col2[2];
        const float SubFactor03 = Col2[0] * Col3[3] - Col3[0] * Col2[3];
        const float SubFactor04 = Col2[0] * Col3[2] - Col3[0] * Col2[2];
        const float SubFactor05 = Col2[0] * Col3[1] - Col3[0] * Col2[1];

        const Float4 DetCof(
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

    static Float4x4 Translation( const Float3 & _Vec ) {
        return Float4x4( Float4( 1,0,0,0 ),
                         Float4( 0,1,0,0 ),
                         Float4( 0,0,1,0 ),
                         Float4( _Vec[0], _Vec[1], _Vec[2], 1 ) );
    }

    Float4x4 Translated( const Float3 & _Vec ) const {
        return Float4x4( Col0, Col1, Col2, Col0 * _Vec[0] + Col1 * _Vec[1] + Col2 * _Vec[2] + Col3 );
    }

    static Float4x4 Scale( const Float3 & _Scale  ) {
        return Float4x4( Float4( _Scale[0],0,0,0 ),
                         Float4( 0,_Scale[1],0,0 ),
                         Float4( 0,0,_Scale[2],0 ),
                         Float4( 0,0,0,1 ) );
    }

    Float4x4 Scaled( const Float3 & _Scale ) const {
        return Float4x4( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2], Col3 );
    }


    // Return rotation around normalized axis
    static Float4x4 RotationAroundNormal( const float & _AngleRad, const Float3 & _Normal ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        const Float3 Temp = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float4x4( c + Temp[0] * _Normal[0],                Temp[0] * _Normal[1] + Temp2[2],     Temp[0] * _Normal[2] - Temp2[1], 0,
                             Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1],                Temp[1] * _Normal[2] + Temp2[0], 0,
                             Temp[2] * _Normal[0] + Temp2[1],     Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2],            0,
                         0,0,0,1 );
    }

    // Return rotation around normalized axis
    Float4x4 RotateAroundNormal( const float & _AngleRad, const Float3 & _Normal ) const {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        Float3 Temp = ( 1.0f - c ) * _Normal;
        Float3 Temp2 = s * _Normal;
        return Float4x4( Col0 * (c + Temp[0] * _Normal[0])            + Col1 * (    Temp[0] * _Normal[1] + Temp2[2]) + Col2 * (    Temp[0] * _Normal[2] - Temp2[1]),
                         Col0 * (    Temp[1] * _Normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * _Normal[1])            + Col2 * (    Temp[1] * _Normal[2] + Temp2[0]),
                         Col0 * (    Temp[2] * _Normal[0] + Temp2[1]) + Col1 * (    Temp[2] * _Normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * _Normal[2]),
                         Col3 );
    }

    // Return rotation around unnormalized vector
    static Float4x4 RotationAroundVector( const float & _AngleRad, const Float3 & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Float4x4 RotateAroundVector( const float & _AngleRad, const Float3 & _Vector ) const {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float4x4 RotationX( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float4x4( 1, 0, 0, 0,
                         0, c, s, 0,
                         0,-s, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Y axis
    static Float4x4 RotationY( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float4x4( c, 0,-s, 0,
                         0, 1, 0, 0,
                         s, 0, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Z axis
    static Float4x4 RotationZ( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float4x4( c, s, 0, 0,
                        -s, c, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1 );
    }

    Float4 operator*( const Float4 & _Vec ) const {
        return Float4( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z + Col3[0] * _Vec.W,
                       Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z + Col3[1] * _Vec.W,
                       Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z + Col3[2] * _Vec.W,
                       Col0[3] * _Vec.X + Col1[3] * _Vec.Y + Col2[3] * _Vec.Z + Col3[3] * _Vec.W );
    }

    // Assume _Vec.W = 1
    Float4 operator*( const Float3 & _Vec ) const {
        return Float4( Col0[ 0 ] * _Vec.X + Col1[ 0 ] * _Vec.Y + Col2[ 0 ] * _Vec.Z + Col3[ 0 ],
            Col0[ 1 ] * _Vec.X + Col1[ 1 ] * _Vec.Y + Col2[ 1 ] * _Vec.Z + Col3[ 1 ],
            Col0[ 2 ] * _Vec.X + Col1[ 2 ] * _Vec.Y + Col2[ 2 ] * _Vec.Z + Col3[ 2 ],
            Col0[ 3 ] * _Vec.X + Col1[ 3 ] * _Vec.Y + Col2[ 3 ] * _Vec.Z + Col3[ 3 ] );
    }

    Float4x4 operator*( const float & _Value ) const {
        return Float4x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value,
                         Col3 * _Value );
    }

    void operator*=( const float & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
        Col3 *= _Value;
    }

    Float4x4 operator/( const float & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float4x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue,
                         Col3 * OneOverValue );
    }

    void operator/=( const float & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
        Col3 *= OneOverValue;
    }

    Float4x4 operator*( const Float4x4 & _Mat ) const {
        const float & L00 = Col0[0];
        const float & L01 = Col0[1];
        const float & L02 = Col0[2];
        const float & L03 = Col0[3];
        const float & L10 = Col1[0];
        const float & L11 = Col1[1];
        const float & L12 = Col1[2];
        const float & L13 = Col1[3];
        const float & L20 = Col2[0];
        const float & L21 = Col2[1];
        const float & L22 = Col2[2];
        const float & L23 = Col2[3];
        const float & L30 = Col3[0];
        const float & L31 = Col3[1];
        const float & L32 = Col3[2];
        const float & L33 = Col3[3];
        const float & R00 = _Mat[0][0];
        const float & R01 = _Mat[0][1];
        const float & R02 = _Mat[0][2];
        const float & R03 = _Mat[0][3];
        const float & R10 = _Mat[1][0];
        const float & R11 = _Mat[1][1];
        const float & R12 = _Mat[1][2];
        const float & R13 = _Mat[1][3];
        const float & R20 = _Mat[2][0];
        const float & R21 = _Mat[2][1];
        const float & R22 = _Mat[2][2];
        const float & R23 = _Mat[2][3];
        const float & R30 = _Mat[3][0];
        const float & R31 = _Mat[3][1];
        const float & R32 = _Mat[3][2];
        const float & R33 = _Mat[3][3];
        return Float4x4( L00 * R00 + L10 * R01 + L20 * R02 + L30 * R03,
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

    void operator*=( const Float4x4 & _Mat ) {
        //*this = *this * _Mat;

        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L02 = Col0[2];
        const float L03 = Col0[3];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float L12 = Col1[2];
        const float L13 = Col1[3];
        const float L20 = Col2[0];
        const float L21 = Col2[1];
        const float L22 = Col2[2];
        const float L23 = Col2[3];
        const float L30 = Col3[0];
        const float L31 = Col3[1];
        const float L32 = Col3[2];
        const float L33 = Col3[3];
        const float & R00 = _Mat[0][0];
        const float & R01 = _Mat[0][1];
        const float & R02 = _Mat[0][2];
        const float & R03 = _Mat[0][3];
        const float & R10 = _Mat[1][0];
        const float & R11 = _Mat[1][1];
        const float & R12 = _Mat[1][2];
        const float & R13 = _Mat[1][3];
        const float & R20 = _Mat[2][0];
        const float & R21 = _Mat[2][1];
        const float & R22 = _Mat[2][2];
        const float & R23 = _Mat[2][3];
        const float & R30 = _Mat[3][0];
        const float & R31 = _Mat[3][1];
        const float & R32 = _Mat[3][2];
        const float & R33 = _Mat[3][3];
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

    Float4x4 operator*( const Float3x4 & _Mat ) const;

    void operator*=( const Float3x4 & _Mat );

    Float4x4 ViewInverseFast() const {
        Float4x4 Inversed;

        float * DstPtr = Inversed.ToPtr();
        const float * SrcPtr = ToPtr();

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

    AN_FORCEINLINE Float4x4 PerspectiveProjectionInverseFast() const {
        Float4x4 Inversed;

        // TODO: check correctness for all perspective projections

        float * DstPtr = Inversed.ToPtr();
        const float * SrcPtr = ToPtr();

        DstPtr[0] = 1.0f / SrcPtr[0];
        DstPtr[1] = 0;
        DstPtr[2] = 0;
        DstPtr[3] = 0;

        DstPtr[4] = 0;
        DstPtr[5] = 1.0f / SrcPtr[5];
        DstPtr[6] = 0;
        DstPtr[7] = 0;

        DstPtr[8] = 0;
        DstPtr[9] = 0;
        DstPtr[10] = 0;
        DstPtr[11] = 1.0f / SrcPtr[14];

        DstPtr[12] = 0;
        DstPtr[13] = 0;
        DstPtr[14] = 1.0f / SrcPtr[11];
        DstPtr[15] = - SrcPtr[10] / (SrcPtr[11] * SrcPtr[14]);

        return Inversed;
    }

    AN_FORCEINLINE Float4x4 OrthoProjectionInverseFast() const {
        // TODO: ...
        return Inversed();
    }

    // String conversions
    FString ToString( int _Precision = FLT_DIG ) const {
        return FString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " " + Col3.ToString( _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " " + Col3.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Col0 << Col1 << Col2 << Col3;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Col0;
        _Stream >> Col1;
        _Stream >> Col2;
        _Stream >> Col3;
    }

    // Static methods

    static const Float4x4 & Identity() {
        static constexpr Float4x4 IdentityMat( 1 );
        return IdentityMat;
    }

    // Conversion from standard projection matrix to clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE const Float4x4 & ClipControl_UpperLeft_ZeroToOne() {
        static constexpr float ClipTransform[] = {
            1,        0,        0,        0,
            0,       -1,        0,        0,
            0,        0,      0.5f,       0,
            0,        0,      0.5f,       1
        };

        return *reinterpret_cast< const Float4x4 * >( &ClipTransform[0] );

        // Same
        //return Float4x4::Identity().Scaled(Float3(1.0f,1.0f,0.5f)).Translated(Float3(0,0,0.5)).Scaled(Float3(1,-1,1));
    }

    // Standard OpenGL ortho projection for 2D
    static AN_FORCEINLINE Float4x4 Ortho2D( Float2 const & _Mins, Float2 const & _Maxs ) {
        const float InvX = 1.0f / (_Maxs.X - _Mins.X);
        const float InvY = 1.0f / (_Maxs.Y - _Mins.Y);
        const float tx = -(_Maxs.X + _Mins.X) * InvX;
        const float ty = -(_Maxs.Y + _Mins.Y) * InvY;
        return Float4x4( 2 * InvX, 0,        0, 0,
                         0,        2 * InvY, 0, 0,
                         0,        0,       -2, 0,
                         tx,       ty,      -1, 1 );
    }

    // OpenGL ortho projection for 2D with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 Ortho2DCC( Float2 const & _Mins, Float2 const & _Maxs ) {
        return ClipControl_UpperLeft_ZeroToOne() * Ortho2D( _Mins, _Maxs );
    }

    // Standard OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 Ortho( Float2 const & _Mins, Float2 const & _Maxs, const float & _ZNear, const float & _ZFar ) {
        const float InvX = 1.0f / (_Maxs.X - _Mins.X);
        const float InvY = 1.0f / (_Maxs.Y - _Mins.Y);
        const float InvZ = 1.0f / (_ZFar - _ZNear);
        const float tx = -(_Maxs.X + _Mins.X) * InvX;
        const float ty = -(_Maxs.Y + _Mins.Y) * InvY;
        const float tz = -(_ZFar + _ZNear) * InvZ;
        return Float4x4( 2 * InvX,0,        0,         0,
                         0,       2 * InvY, 0,         0,
                         0,       0,        -2 * InvZ, 0,
                         tx,      ty,       tz,        1 );
    }

    // OpenGL ortho projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 OrthoCC( Float2 const & _Mins, Float2 const & _Maxs, const float & _ZNear, const float & _ZFar ) {
        const float InvX = 1.0/(_Maxs.X - _Mins.X);
        const float InvY = 1.0/(_Maxs.Y - _Mins.Y);
        const float InvZ = 1.0/(_ZFar - _ZNear);
        const float tx = -(_Maxs.X + _Mins.X) * InvX;
        const float ty = -(_Maxs.Y + _Mins.Y) * InvY;
        const float tz = -(_ZFar + _ZNear) * InvZ;
        return Float4x4( 2 * InvX,  0,         0,              0,
                         0,         -2 * InvY, 0,              0,
                         0,         0,         -InvZ,          0,
                         tx,       -ty,        tz * 0.5 + 0.5, 1 );
        // Same
        // Transform according to clip control
        //return ClipControl_UpperLeft_ZeroToOne() * Ortho( _Mins, _Maxs, _ZNear, _ZFar );
    }

    // Reversed-depth OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 OrthoRev( Float2 const & _Mins, Float2 const & _Maxs, const float & _ZNear, const float & _ZFar ) {
        const float InvX = 1.0f / (_Maxs.X - _Mins.X);
        const float InvY = 1.0f / (_Maxs.Y - _Mins.Y);
        const float InvZ = 1.0f / (_ZNear - _ZFar);
        const float tx = -(_Maxs.X + _Mins.X) * InvX;
        const float ty = -(_Maxs.Y + _Mins.Y) * InvY;
        const float tz = -(_ZNear + _ZFar) * InvZ;
        return Float4x4( 2 * InvX, 0,        0,         0,
                         0,        2 * InvY, 0,         0,
                         0,        0,        -2 * InvZ, 0,
                         tx,       ty,       tz,        1 );
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 OrthoRevCC( Float2 const & _Mins, Float2 const & _Maxs, const float & _ZNear, const float & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * OrthoRev( _Mins, _Maxs, _ZNear, _ZFar );
    }

    // Standard OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 Perspective( const float & _FovXRad, const float & _Width, const float & _Height, const float & _ZNear, const float & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY = (float)(atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZFar + _ZNear) / ( _ZNear - _ZFar ),  -1,
                         0,                0,               2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    static AN_FORCEINLINE Float4x4 Perspective( const float & _FovXRad, const float & _FovYRad, const float & _ZNear, const float & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZFar + _ZNear) / ( _ZNear - _ZFar ),  -1,
                         0,                0,               2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    // OpenGL perspective projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 PerspectiveCC( const float & _FovXRad, const float & _Width, const float & _Height, const float & _ZNear, const float & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _Width, _Height, _ZNear, _ZFar );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveCC( const float & _FovXRad, const float & _FovYRad, const float & _ZNear, const float & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _FovYRad, _ZNear, _ZFar );
    }

    // Reversed-depth OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRev( const float & _FovXRad, const float & _Width, const float & _Height, const float & _ZNear, const float & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY = (float)(atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZNear + _ZFar) / ( _ZFar - _ZNear ),  -1,
                         0,                0,               2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRev( const float & _FovXRad, const float & _FovYRad, const float & _ZNear, const float & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZNear + _ZFar) / ( _ZFar - _ZNear ),  -1,
                         0,                0,               2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRevCC( const float & _FovXRad, const float & _Width, const float & _Height, const float & _ZNear, const float & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY = (float)(atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX, 0,               0,                                         0,
                         0,              -1 / TanHalfFovY, 0,                                         0,
                         0,               0,               _ZNear / ( _ZFar - _ZNear),               -1,
                         0,               0,               _ZNear * _ZFar / ( _ZFar - _ZNear ),       0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRevCC( const float & _FovXRad, const float & _FovYRad, const float & _ZNear, const float & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX, 0,               0,                                         0,
                         0,              -1 / TanHalfFovY, 0,                                         0,
                         0,               0,               _ZNear / ( _ZFar - _ZNear),               -1,
                         0,               0,               _ZNear * _ZFar / ( _ZFar - _ZNear ),       0 );
    }

    static AN_FORCEINLINE void GetCubeFaceMatrices( Float4x4 & _PositiveX,
                                                    Float4x4 & _NegativeX,
                                                    Float4x4 & _PositiveY,
                                                    Float4x4 & _NegativeY,
                                                    Float4x4 & _PositiveZ,
                                                    Float4x4 & _NegativeZ ) {
        _PositiveX = Float4x4::RotationZ( FMath::_PI ).RotateAroundNormal( FMath::_HALF_PI, Float3(0,1,0) );
        _NegativeX = Float4x4::RotationZ( FMath::_PI ).RotateAroundNormal( -FMath::_HALF_PI, Float3(0,1,0) );
        _PositiveY = Float4x4::RotationX( -FMath::_HALF_PI );
        _NegativeY = Float4x4::RotationX( FMath::_HALF_PI );
        _PositiveZ = Float4x4::RotationX( FMath::_PI );
        _NegativeZ = Float4x4::RotationZ( FMath::_PI );
    }

    static AN_FORCEINLINE const Float4x4 * GetCubeFaceMatrices() {
        // TODO: Precompute this matrices
        static const Float4x4 CubeFaceMatrices[6] = {
            Float4x4::RotationZ( FMath::_PI ).RotateAroundNormal( FMath::_HALF_PI, Float3(0,1,0) ),
            Float4x4::RotationZ( FMath::_PI ).RotateAroundNormal( -FMath::_HALF_PI, Float3(0,1,0) ),
            Float4x4::RotationX( -FMath::_HALF_PI ),
            Float4x4::RotationX( FMath::_HALF_PI ),
            Float4x4::RotationX( FMath::_PI ),
            Float4x4::RotationZ( FMath::_PI )
        };
        return CubeFaceMatrices;
    }
};

// Column-major matrix 3x4
// Keep transformations transposed
class Float3x4 final {
public:
    Float4 Col0;
    Float4 Col1;
    Float4 Col2;

    Float3x4() = default;
    explicit Float3x4( const Float2x2 & _Value );
    explicit Float3x4( const Float3x3 & _Value );
    explicit Float3x4( const Float4x4 & _Value );
    constexpr Float3x4( const Float4 & _Col0, const Float4 & _Col1, const Float4 & _Col2 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Float3x4( const float & _00, const float & _01, const float & _02, const float & _03,
                        const float & _10, const float & _11, const float & _12, const float & _13,
                        const float & _20, const float & _21, const float & _22, const float & _23 )
        : Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ) {}
    constexpr explicit Float3x4( const float & _Diagonal ) : Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ) {}
    constexpr explicit Float3x4( const Float3 & _Diagonal ) : Col0( _Diagonal.X, 0, 0, 0 ), Col1( 0, _Diagonal.Y, 0, 0 ), Col2( 0, 0, _Diagonal.Z, 0 ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

    Float3x4 & operator=( const Float3x4 & _Other ) {
        //Col0 = _Other.Col0;
        //Col1 = _Other.Col1;
        //Col2 = _Other.Col2;
        memcpy( this, &_Other, sizeof( *this ) );
        return *this;
    }

    Float4 & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    const Float4 & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float3 GetRow( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return Float3( Col0[ _Index ], Col1[ _Index ], Col2[ _Index ] );
    }

    Bool operator==( const Float3x4 & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Float3x4 & _Other ) const { return !Compare( _Other ); }

    Bool Compare( const Float3x4 & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 12 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    Bool CompareEps( const Float3x4 & _Other, const float & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 12 ; i++ ) {
            if ( FMath::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void Compose( const Float3 & _Translation, const Float3x3 & _Rotation, const Float3 & _Scale ) {
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

    void Compose( const Float3 & _Translation, const Float3x3 & _Rotation ) {
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

    void SetTranslation( const Float3 & _Translation ) {
        Col0[3] = _Translation.X;
        Col1[3] = _Translation.Y;
        Col2[3] = _Translation.Z;
    }

    void DecomposeAll( Float3 & _Translation, Float3x3 & _Rotation, Float3 & _Scale ) const {
        _Translation.X = Col0[3];
        _Translation.Y = Col1[3];
        _Translation.Z = Col2[3];

        _Scale.X = Float3( Col0[0], Col1[0], Col2[0] ).Length();
        _Scale.Y = Float3( Col0[1], Col1[1], Col2[1] ).Length();
        _Scale.Z = Float3( Col0[2], Col1[2], Col2[2] ).Length();

        float sx = 1.0f / _Scale.X;
        float sy = 1.0f / _Scale.Y;
        float sz = 1.0f / _Scale.Z;

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

    Float3 DecomposeTranslation() const {
        return Float3( Col0[3], Col1[3], Col2[3] );
    }

    Float3x3 DecomposeRotation() const {
        return Float3x3( Float3( Col0[0], Col1[0], Col2[0] ) / Float3( Col0[0], Col1[0], Col2[0] ).Length(),
                         Float3( Col0[1], Col1[1], Col2[1] ) / Float3( Col0[1], Col1[1], Col2[1] ).Length(),
                         Float3( Col0[2], Col1[2], Col2[2] ) / Float3( Col0[2], Col1[2], Col2[2] ).Length() );
    }

    Float3 DecomposeScale() const {
        return Float3( Float3( Col0[0], Col1[0], Col2[0] ).Length(),
                       Float3( Col0[1], Col1[1], Col2[1] ).Length(),
                       Float3( Col0[2], Col1[2], Col2[2] ).Length() );
    }

    void DecomposeRotationAndScale( Float3x3 & _Rotation, Float3 & _Scale ) const {
        _Scale.X = Float3( Col0[0], Col1[0], Col2[0] ).Length();
        _Scale.Y = Float3( Col0[1], Col1[1], Col2[1] ).Length();
        _Scale.Z = Float3( Col0[2], Col1[2], Col2[2] ).Length();

        float sx = 1.0f / _Scale.X;
        float sy = 1.0f / _Scale.Y;
        float sz = 1.0f / _Scale.Z;

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

    void DecomposeNormalMatrix( Float3x3 & _NormalMatrix ) const {
        const Float3x4 & m = *this;

        const float Determinant = m[0][0] * m[1][1] * m[2][2] +
                                  m[1][0] * m[2][1] * m[0][2] +
                                  m[2][0] * m[0][1] * m[1][2] -
                                  m[2][0] * m[1][1] * m[0][2] -
                                  m[1][0] * m[0][1] * m[2][2] -
                                  m[0][0] * m[2][1] * m[1][2];

        const float OneOverDeterminant = 1.0f / Determinant;

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
        //_NormalMatrix = Float3x3(Inversed());
    }

    void InverseSelf() {
        *this = Inversed();
    }

    Float3x4 Inversed() const {
        const Float3x4 & m = *this;

        const float Determinant = m[0][0] * m[1][1] * m[2][2] +
                                  m[1][0] * m[2][1] * m[0][2] +
                                  m[2][0] * m[0][1] * m[1][2] -
                                  m[2][0] * m[1][1] * m[0][2] -
                                  m[1][0] * m[0][1] * m[2][2] -
                                  m[0][0] * m[2][1] * m[1][2];

        const float OneOverDeterminant = 1.0f / Determinant;
        Float3x4 Result;

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

    float Determinant() const {
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

    static Float3x4 Translation( const Float3 & _Vec ) {
        return Float3x4( Float4( 1,0,0,_Vec[0] ),
                         Float4( 0,1,0,_Vec[1] ),
                         Float4( 0,0,1,_Vec[2] ) );
    }

    static Float3x4 Scale( const Float3 & _Scale  ) {
        return Float3x4( Float4( _Scale[0],0,0,0 ),
                         Float4( 0,_Scale[1],0,0 ),
                         Float4( 0,0,_Scale[2],0 ) );
    }

    // Return rotation around normalized axis
    static Float3x4 RotationAroundNormal( const float & _AngleRad, const Float3 & _Normal ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        const Float3 Temp = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float3x4( c + Temp[0] * _Normal[0]        ,        Temp[1] * _Normal[0] - Temp2[2],     Temp[2] * _Normal[0] + Temp2[1], 0,
                             Temp[0] * _Normal[1] + Temp2[2], c + Temp[1] * _Normal[1],                Temp[2] * _Normal[1] - Temp2[0], 0,
                             Temp[0] * _Normal[2] - Temp2[1],     Temp[1] * _Normal[2] + Temp2[0], c + Temp[2] * _Normal[2],            0 );
    }

    // Return rotation around unnormalized vector
    static Float3x4 RotationAroundVector( const float & _AngleRad, const Float3 & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float3x4 RotationX( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float3x4( 1, 0, 0, 0,
                         0, c,-s, 0,
                         0, s, c, 0 );
    }

    // Return rotation around Y axis
    static Float3x4 RotationY( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float3x4( c, 0, s, 0,
                         0, 1, 0, 0,
                        -s, 0, c, 0 );
    }

    // Return rotation around Z axis
    static Float3x4 RotationZ( const float & _AngleRad ) {
        float s, c;
        FMath::RadSinCos( _AngleRad, s, c );
        return Float3x4( c,-s, 0, 0,
                         s, c, 0, 0,
                         0, 0, 1, 0 );
    }

    // Assume _Vec.W = 1
    Float3 operator*( const Float3 & _Vec ) const {
        return Float3( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[2] * _Vec.Z + Col0[3],
                       Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[2] * _Vec.Z + Col1[3],
                       Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[2] * _Vec.Z + Col2[3] );
    }

    // Assume _Vec.Z = 0, _Vec.W = 1
    Float3 operator*( const Float2 & _Vec ) const {
        return Float3( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                       Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3],
                       Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[3] );
    }

    Float2 Mult_Vec2_IgnoreZ( const Float2 & _Vec ) {
        return Float2( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                       Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3] );
    }

    Float3x4 operator*( const float & _Value ) const {
        return Float3x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( const float & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Float3x4 operator/( const float & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float3x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue);
    }

    void operator/=( const float & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    Float3x4 operator*( const Float3x4 & _Mat ) const {
        return Float3x4( Col0[0] * _Mat[0][0] + Col0[1] * _Mat[1][0] + Col0[2] * _Mat[2][0],
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

    void operator*=( const Float3x4 & _Mat ) {
        *this = Float3x4( Col0[0] * _Mat[0][0] + Col0[1] * _Mat[1][0] + Col0[2] * _Mat[2][0],
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
    FString ToString( int _Precision = FLT_DIG ) const {
        return FString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Col0 << Col1 << Col2;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Col0;
        _Stream >> Col1;
        _Stream >> Col2;
    }

    // Static methods

    static const Float3x4 & Identity() {
        static constexpr Float3x4 IdentityMat( 1 );
        return IdentityMat;
    }
};

// Type conversion

// Float3x3 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( const Float3x3 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float3x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( const Float3x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float4x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( const Float4x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float2x2 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( const Float2x2 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0,0,1 ) {}

// Float3x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( const Float3x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float4x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( const Float4x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float2x2 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( const Float2x2 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0,0,1,0 ), Col3( 0,0,0,1 ) {}

// Float3x3 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( const Float3x3 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0,0,0,1 ) {}

// Float3x4 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( const Float3x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0,0,0,1 ) {}

// Float2x2 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( const Float2x2 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0.0f ) {}

// Float3x3 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( const Float3x3 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float4x4 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( const Float4x4 & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}


AN_FORCEINLINE Float2 operator*( const Float2 & _Vec, const Float2x2 & _Mat ) {
    return Float2( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y,
                   _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y );
}

AN_FORCEINLINE Float3 operator*( const Float3 & _Vec, const Float3x3 & _Mat ) {
    return Float3( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z,
                   _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z,
                   _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z);
}

AN_FORCEINLINE Float4 operator*( const Float4 & _Vec, const Float4x4 & _Mat ) {
    return Float4( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z + _Mat[0][3] * _Vec.W,
                   _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z + _Mat[1][3] * _Vec.W,
                   _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z + _Mat[2][3] * _Vec.W,
                   _Mat[3][0] * _Vec.X + _Mat[3][1] * _Vec.Y + _Mat[3][2] * _Vec.Z + _Mat[3][3] * _Vec.W );
}

AN_FORCEINLINE Float4x4 Float4x4::operator*( const Float3x4 & _Mat ) const {
    const Float4 & SrcB0 = _Mat.Col0;
    const Float4 & SrcB1 = _Mat.Col1;
    const Float4 & SrcB2 = _Mat.Col2;

    return Float4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                     Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                     Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                     Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

AN_FORCEINLINE void Float4x4::operator*=( const Float3x4 & _Mat ) {
    const Float4 & SrcB0 = _Mat.Col0;
    const Float4 & SrcB1 = _Mat.Col1;
    const Float4 & SrcB2 = _Mat.Col2;

    *this = Float4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                      Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                      Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                      Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

namespace FMath {

    /*static*/ AN_FORCEINLINE bool Unproject( const Float4x4 & _ModelViewProjectionInversed, const float _Viewport[4], const Float3 & _Coord, Float3 & _Result ) {
        Float4 In( _Coord, 1.0f );

        // Map x and y from window coordinates
        In.X = (In.X - _Viewport[0]) / _Viewport[2];
        In.Y = (In.Y - _Viewport[1]) / _Viewport[3];

        // Map to range -1 to 1
        In.X = In.X * 2.0f - 1.0f;
        In.Y = In.Y * 2.0f - 1.0f;
        In.Z = In.Z * 2.0f - 1.0f;

        _Result.X = _ModelViewProjectionInversed[0][0] * In[0] + _ModelViewProjectionInversed[1][0] * In[1] + _ModelViewProjectionInversed[2][0] * In[2] + _ModelViewProjectionInversed[3][0] * In[3];
        _Result.Y = _ModelViewProjectionInversed[0][1] * In[0] + _ModelViewProjectionInversed[1][1] * In[1] + _ModelViewProjectionInversed[2][1] * In[2] + _ModelViewProjectionInversed[3][1] * In[3];
        _Result.Z = _ModelViewProjectionInversed[0][2] * In[0] + _ModelViewProjectionInversed[1][2] * In[1] + _ModelViewProjectionInversed[2][2] * In[2] + _ModelViewProjectionInversed[3][2] * In[3];
        const float Div = _ModelViewProjectionInversed[0][3] * In[0] + _ModelViewProjectionInversed[1][3] * In[1] + _ModelViewProjectionInversed[2][3] * In[2] + _ModelViewProjectionInversed[3][3] * In[3];

        if ( Div == 0.0f ) {
            return false;
        }

        _Result /= Div;

        return true;
    }

    /*static*/ AN_FORCEINLINE bool UnprojectRay( const Float4x4 & _ModelViewProjectionInversed, const float _Viewport[4], const float & _X, const float & _Y, Float3 & _RayStart, Float3 & _RayEnd ) {
        Float3 Coord;

        Coord.X = _X;
        Coord.Y = _Y;

        // get start point
        Coord.Z = -1.0f;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayStart ) )
            return false;

        // get end point
        Coord.Z = 1.0f;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayEnd ) )
            return false;

        return true;
    }

    /*static*/ AN_FORCEINLINE bool UnprojectRayDir( const Float4x4 & _ModelViewProjectionInversed, const float _Viewport[4], const float & _X, const float & _Y, Float3 & _RayStart, Float3 & _RayDir ) {
        Float3 Coord;

        Coord.X = _X;
        Coord.Y = _Y;

        // get start point
        Coord.Z = -1.0f;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayStart ) )
            return false;

        // get end point
        Coord.Z = 1.0f;
        if ( !Unproject( _ModelViewProjectionInversed, _Viewport, Coord, _RayDir ) )
            return false;

        _RayDir -= _RayStart;
        _RayDir.NormalizeSelf();

        return true;
    }

    /*static*/ AN_FORCEINLINE bool UnprojectPoint( const Float4x4 & _ModelViewProjectionInversed,
                                               const float _Viewport[4],
                                               const float & _X, const float & _Y, const float & _Depth,
                                               Float3 & _Result )
    {
        return Unproject( _ModelViewProjectionInversed, _Viewport, Float3( _X, _Y, _Depth ), _Result );
    }

}
