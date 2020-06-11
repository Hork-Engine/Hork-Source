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

#include "BaseTypes.h"
#include "Std.h"
#include "IO.h"

#include <emmintrin.h>

#ifndef FLT_DIG
#define FLT_DIG 6
#endif
#ifndef DBL_DIG
#define DBL_DIG 10
#endif

namespace Math {

template< typename T >
constexpr bool IsSigned() {
    return std::is_signed< T >::value;
}

template< typename T >
constexpr bool IsUnsigned() {
    return std::is_unsigned< T >::value;
}

template< typename T >
constexpr bool IsIntegral() {
    return std::is_integral<T>::value;
}

template< typename T >
constexpr bool IsReal() {
    return std::is_floating_point<T>::value;
}

template< typename T >
constexpr int BitsCount() {
    return sizeof( T ) * 8;
}

template< typename T >
constexpr int FloatingPointPrecision();

template<>
constexpr int FloatingPointPrecision< float >() {
    return FLT_DIG;
}

template<>
constexpr int FloatingPointPrecision< double >() {
    return DBL_DIG;
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
T Abs( T const & Value ) {
    const T Mask = Value >> ( BitsCount< T >() - 1 );
    return ( ( Value ^ Mask ) - Mask );
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
constexpr T Dist( T const & A, T const & B ) {
    // NOTE: We don't use Abs() to control value overflow
    return ( B > A ) ? ( B - A ) : ( A - B );
}

AN_FORCEINLINE float Abs( float const & Value ) {
    int32_t i = *reinterpret_cast< const int32_t * >( &Value ) & 0x7FFFFFFF;
    return *reinterpret_cast< const float * >( &i );
}

AN_FORCEINLINE double Abs( double const & Value ) {
    int64_t i = *reinterpret_cast< const int64_t * >( &Value ) & 0x7FFFFFFFFFFFFFFFLL;
    return *reinterpret_cast< const double * >( &i );
}

AN_FORCEINLINE float Dist( float const & A, float const & B ) {
    return Abs( A - B );
}

AN_FORCEINLINE double Dist( double const & A, double const & B ) {
    return Abs( A - B );
}

template< typename T >
constexpr T MinValue() { return TStdNumericLimits< T >::min(); }

template< typename T >
constexpr T MaxValue() { return TStdNumericLimits< T >::max(); }

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
constexpr int SignBits( T const & Value ) {
    return IsSigned< T >() ? static_cast< uint64_t >( Value ) >> ( BitsCount< T >() - 1 ) : 0;
}

constexpr int SignBits( float const & Value ) {
    return *reinterpret_cast< const uint32_t * >(&Value) >> 31;
}

constexpr int SignBits( double const & Value ) {
    return *reinterpret_cast< const uint64_t * >(&Value) >> 63;
}

/** Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0 */
template< typename T >
constexpr T Sign( T const & Value ) {
    return Value > 0 ? 1 : -SignBits( Value );
}

template< typename T >
constexpr T MaxPowerOfTwo() {
    return T(1) << ( BitsCount< T >() - 2 );
}

template<>
constexpr float MaxPowerOfTwo() {
    return float( 1u << 31 );
}

template< typename T >
constexpr T MinPowerOfTwo() {
    return T(1);
}

constexpr int32_t ToIntFast( float const & Value ) {
    return static_cast< int32_t >(Value);
}

constexpr int64_t ToLongFast( float const & Value ) {
    return static_cast< int64_t >(Value);
}

template< typename T >
T ToGreaterPowerOfTwo( T const & Value );

template< typename T >
T ToLessPowerOfTwo( T const & Value );

template<>
AN_FORCEINLINE int8_t ToGreaterPowerOfTwo< int8_t >( int8_t const & Value ) {
    using T = int8_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val + 1;
}

template<>
AN_FORCEINLINE uint8_t ToGreaterPowerOfTwo< uint8_t >( uint8_t const & Value ) {
    using T = uint8_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val + 1;
}

template<>
AN_FORCEINLINE int16_t ToGreaterPowerOfTwo< int16_t >( int16_t const & Value ) {
    using T = int16_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val + 1;
}

template<>
AN_FORCEINLINE uint16_t ToGreaterPowerOfTwo< uint16_t >( uint16_t const & Value ) {
    using T = uint16_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val + 1;
}

template<>
AN_FORCEINLINE int32_t ToGreaterPowerOfTwo< int32_t >( int32_t const & Value ) {
    using T = int32_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val + 1;
}

template<>
AN_FORCEINLINE uint32_t ToGreaterPowerOfTwo< uint32_t >( uint32_t const & Value ) {
    using T = uint32_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val + 1;
}

template<>
AN_FORCEINLINE int64_t ToGreaterPowerOfTwo< int64_t >( int64_t const & Value ) {
    using T = int64_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val + 1;
}

template<>
AN_FORCEINLINE uint64_t ToGreaterPowerOfTwo< uint64_t >( uint64_t const & Value ) {
    using T = uint64_t;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val + 1;
}

template<>
AN_FORCEINLINE float ToGreaterPowerOfTwo( float const & Value ) {
    using T = float;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    uint32_t Val = ToIntFast( Value ) - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast< float >(Val + 1);
}

template<>
AN_FORCEINLINE int8_t ToLessPowerOfTwo< int8_t >( int8_t const & Value ) {
    using T = int8_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE uint8_t ToLessPowerOfTwo< uint8_t >( uint8_t const & Value ) {
    using T = uint8_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE int16_t ToLessPowerOfTwo< int16_t >( int16_t const & Value ) {
    using T = int16_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE uint16_t ToLessPowerOfTwo< uint16_t >( uint16_t const & Value ) {
    using T = uint16_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE int32_t ToLessPowerOfTwo< int32_t >( int32_t const & Value ) {
    using T = int32_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE uint32_t ToLessPowerOfTwo< uint32_t >( uint32_t const & Value ) {
    using T = uint32_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE int64_t ToLessPowerOfTwo< int64_t >( int64_t const & Value ) {
    using T = int64_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE uint64_t ToLessPowerOfTwo< uint64_t >( uint64_t const & Value ) {
    using T = uint64_t;
    T Val = Value;
    if ( Val < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val - ( Val >> 1 );
}

template<>
AN_FORCEINLINE float ToLessPowerOfTwo( float const & Value ) {
    using T = float;
    if ( Value >= MaxPowerOfTwo< T >() ) {
        return MaxPowerOfTwo< T >();
    }
    if ( Value < MinPowerOfTwo< T >() ) {
        return MinPowerOfTwo< T >();
    }
    uint32_t Val = ToIntFast( Value );
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast< float >(Val - (Val >> 1));
}

template< typename T >
AN_FORCEINLINE T ToClosestPowerOfTwo( T const & Value ) {
    T GreaterPow = ToGreaterPowerOfTwo( Value );
    T LessPow = ToLessPowerOfTwo( Value );
    return Dist( GreaterPow, Value ) < Dist( LessPow, Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE int Log2( uint32_t Value ) {
    uint32_t r;
    uint32_t shift;
    uint32_t v = Value;
    r = (v > 0xffff) << 4; v >>= r;
    shift = (v > 0xff) << 3; v >>= shift; r |= shift;
    shift = (v > 0xf) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3) << 1; v >>= shift; r |= shift;
    r |= (v >> 1);
    return r;
}

AN_FORCEINLINE int Log2( uint8_t value ) {
    int log2 = 0;
    while ( value >>= 1 )
        log2++;
    return log2;
}

AN_FORCEINLINE int Log2( uint16_t value ) {
    int log2 = 0;
    while ( value >>= 1 )
        log2++;
    return log2;
}

AN_FORCEINLINE int Log2( uint64_t value ) {
    int log2 = 0;
    while ( value >>= 1 )
        log2++;
    return log2;
}

/** Return half float sign bit */
constexpr int HalfFloatSignBits( uint16_t const & Value ) {
    return Value >> 15;
}

/** Return half float exponent */
constexpr int HalfFloatExponent( uint16_t const & Value ) {
    return (Value >> 10) & 0x1f;
}

/** Return half float mantissa */
constexpr int HalfFloatMantissa( uint16_t const & Value ) {
    return Value & 0x3ff;
}

uint16_t _FloatToHalf( const uint32_t & _I );

uint32_t _HalfToFloat( const uint16_t & _I );

void FloatToHalf( const float * _In, uint16_t * _Out, int _Count );

void HalfToFloat( const uint16_t * _In, float * _Out, int _Count );

AN_FORCEINLINE uint16_t FloatToHalf( float const & Value ) {
    return _FloatToHalf( *reinterpret_cast< const uint32_t * >( &Value ) );
}

AN_FORCEINLINE float HalfToFloat( const uint16_t & Value ) {
    float f;
    *reinterpret_cast< uint32_t * >( &f ) = _HalfToFloat( Value );
    return f;
}

/** Return floating point exponent */
constexpr int Exponent( float const & Value ) {
    return ( *reinterpret_cast< const uint32_t * >( &Value ) >> 23 ) & 0xff;
}

/** Return floating point mantissa */
constexpr int Mantissa( float const & Value ) {
    return *reinterpret_cast< const uint32_t * >( &Value ) & 0x7fffff;
}

/** Return floating point exponent */
constexpr int Exponent( double const & Value ) {
    return ( *reinterpret_cast< const uint64_t * >( &Value ) >> 52 ) & 0x7ff;
}

/** Return floating point mantissa */
constexpr int64_t Mantissa( double const & Value ) {
    return *reinterpret_cast< const uint64_t * >( &Value ) & 0xfffffffffffff;
}

constexpr bool IsInfinite( float const & Value ) {
    //return std::isinf( Self );
    return ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x7fffffff ) == 0x7f800000;
}

constexpr bool IsNan( float const & Value ) {
    //return std::isnan( Self );
    return ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x7f800000 ) == 0x7f800000;
}

AN_FORCEINLINE/*constexpr*/ bool IsNormal( float const & Value ) {
    return std::isnormal( Value );
}

constexpr bool IsDenormal( float const & Value ) {
    return ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x7f800000 ) == 0x00000000
        && ( *reinterpret_cast< const uint32_t * >( &Value ) & 0x007fffff ) != 0x00000000;
}

// Floating point specific
constexpr bool IsInfinite( double const & Value ) {
    //return std::isinf( Value );
    return ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x7fffffffffffffffULL) ) == uint64_t( 0x7f80000000000000ULL );
}

constexpr bool IsNan( double const & Value ) {
    //return std::isnan( Value );
    return ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x7f80000000000000ULL) ) == uint64_t( 0x7f80000000000000ULL );
}

AN_FORCEINLINE/*constexpr*/ bool IsNormal( double const & Value ) {
    return std::isnormal( Value );
}

constexpr bool IsDenormal( double const & Value ) {
    return ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x7f80000000000000ULL) ) == uint64_t( 0x0000000000000000ULL )
        && ( *reinterpret_cast< const uint64_t * >( &Value ) & uint64_t(0x007fffffffffffffULL) ) != uint64_t( 0x0000000000000000ULL );
}

template< typename T >
constexpr int MaxExponent();

template<>
constexpr int MaxExponent< float >() { return 127; }

template<>
constexpr int MaxExponent< double >() { return 1023; }

template< typename T >
AN_FORCEINLINE T Floor( T const & Value ) {
    return std::floor( Value );
}

template< typename T >
AN_FORCEINLINE T Ceil( T const & Value ) {
    return std::ceil( Value );
}

template< typename T >
AN_FORCEINLINE T Fract( T const & Value ) {
    return Value - std::floor( Value );
}

//template< typename T >
//AN_FORCEINLINE T Clamp( T const & _Value, T const & _Min, T const & _Max ) {
//    return StdMin( StdMax( _Value, _Min ), _Max );
//}

//template< typename T >
//AN_FORCEINLINE T Saturate( T const & Value ) {
//    return Clamp( Value, T(0), T(1) );
//}

template< typename T >
AN_FORCEINLINE T Step( T const & Value, T const & Edge ) {
    return Value < Edge ? T(0) : T(1);
}

template< typename T >
AN_FORCEINLINE T SmoothStep( T const & Value, T const & Edge0, T const & Edge1 ) {
    const T t = Saturate( ( Value - Edge0 ) / ( Edge1 - Edge0 ) );
    return t * t * ( T(3) - T(2) * t );
}

template< typename T >
constexpr T Lerp( T const & _From, T const & _To, T const & _Mix ) {
    return _From + _Mix * ( _To - _From );
}

template< typename T >
AN_FORCEINLINE T Round( T const & Value ) {
    return Floor( Value + T(0.5) );
}

template< typename T >
AN_FORCEINLINE T RoundN( T const & Value, T const & _N ) {
    return Floor( Value * _N + 0.5f ) / _N;
}

template< typename T >
AN_FORCEINLINE T Round1( T const & Value ) { return RoundN( Value, T(10) ); }

template< typename T >
AN_FORCEINLINE T Round2( T const & Value ) { return RoundN( Value, T(100) ); }

template< typename T >
AN_FORCEINLINE T Round3( T const & Value ) { return RoundN( Value, T(1000) ); }

template< typename T >
AN_FORCEINLINE T Round4( T const & Value ) { return RoundN( Value, T(10000) ); }

template< typename T >
AN_FORCEINLINE T Snap( T const & Value, T const & SnapValue ) {
    AN_ASSERT( SnapValue > T(0) );
    return Round( Value / SnapValue ) * SnapValue;
}

constexpr double _PI_DBL = 3.1415926535897932384626433832795;
constexpr double _2PI_DBL = 2.0 * _PI_DBL;
constexpr double _HALF_PI_DBL = 0.5 * _PI_DBL;
constexpr double _EXP_DBL = 2.71828182845904523536;
constexpr double _DEG2RAD_DBL = _PI_DBL / 180.0;
constexpr double _RAD2DEG_DBL = 180.0 / _PI_DBL;

constexpr float _PI = static_cast< float >( _PI_DBL );
constexpr float _2PI = static_cast< float >( _2PI_DBL );
constexpr float _HALF_PI = static_cast< float >( _HALF_PI_DBL );
constexpr float _EXP = static_cast< float >( _EXP_DBL );
constexpr float _DEG2RAD = static_cast< float >( _DEG2RAD_DBL );
constexpr float _RAD2DEG = static_cast< float >( _RAD2DEG_DBL );
constexpr float _INFINITY = 1e30f;
constexpr float _ZERO_TOLERANCE = 1.1754944e-038f;

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T Min( T a, T b ) {
    return StdMin( a, b );
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T Min3( T a, T b, T c ) {
    return Min( Min( a, b ), c );
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T Max( T a, T b ) {
    return StdMax( a, b );
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T Max3( T a, T b, T c ) {
    return Max( Max( a, b ), c );
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T Clamp( T val, T a, T b ) {
    return StdMin( StdMax( val, a ), b );
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T Saturate( T val ) {
    return Clamp( val, T(0), T(1) );
}

/** Branchless min */
AN_FORCEINLINE float Min( float a, float b ) {
    _mm_store_ss( &a, _mm_min_ss( _mm_set_ss( a ), _mm_set_ss( b ) ) );
    return a;
}

AN_FORCEINLINE float Min3( float a, float b, float c ) {
    _mm_store_ss( &a, _mm_min_ss( _mm_min_ss( _mm_set_ss( a ), _mm_set_ss( b ) ), _mm_set_ss( c ) ) );
    return a;
}

/** Branchless max */
AN_FORCEINLINE float Max( float a, float b ) {
    _mm_store_ss( &a, _mm_max_ss( _mm_set_ss( a ), _mm_set_ss( b ) ) );
    return a;
}

AN_FORCEINLINE float Max3( float a, float b, float c ) {
    _mm_store_ss( &a, _mm_max_ss( _mm_max_ss( _mm_set_ss( a ), _mm_set_ss( b ) ), _mm_set_ss( c ) ) );
    return a;
}

/** Branchless clamp */
AN_FORCEINLINE float Clamp( float val, float minval, float maxval ) {
    _mm_store_ss( &val, _mm_min_ss( _mm_max_ss( _mm_set_ss( val ), _mm_set_ss( minval ) ), _mm_set_ss( maxval ) ) );
    return val;
}

/** Branchless saturate */
AN_FORCEINLINE float Saturate( float val ) {
    _mm_store_ss( &val, _mm_min_ss( _mm_max_ss( _mm_set_ss( val ), _mm_setzero_ps() ), _mm_set_ss( 1.0f ) ) );
    return val;
}

/** Branchless min */
AN_FORCEINLINE double Min( double a, double b ) {
    _mm_store_sd( &a, _mm_min_sd( _mm_set_sd( a ), _mm_set_sd( b ) ) );
    return a;
}

AN_FORCEINLINE double Min3( double a, double b, double c ) {
    _mm_store_sd( &a, _mm_min_sd( _mm_min_sd( _mm_set_sd( a ), _mm_set_sd( b ) ), _mm_set_sd( c ) ) );
    return a;
}

/** Branchless max */
AN_FORCEINLINE double Max( double a, double b ) {
    _mm_store_sd( &a, _mm_max_sd( _mm_set_sd( a ), _mm_set_sd( b ) ) );
    return a;
}

AN_FORCEINLINE double Max3( double a, double b, double c ) {
    _mm_store_sd( &a, _mm_max_sd( _mm_max_sd( _mm_set_sd( a ), _mm_set_sd( b ) ), _mm_set_sd( c ) ) );
    return a;
}

/** Branchless clamp */
AN_FORCEINLINE double Clamp( double val, double minval, double maxval ) {
    _mm_store_sd( &val, _mm_min_sd( _mm_max_sd( _mm_set_sd( val ), _mm_set_sd( minval ) ), _mm_set_sd( maxval ) ) );
    return val;
}

/** Branchless saturate */
AN_FORCEINLINE double Saturate( double val ) {
    _mm_store_sd( &val, _mm_min_sd( _mm_max_sd( _mm_set_sd( val ), _mm_setzero_pd() ), _mm_set_sd( 1.0 ) ) );
    return val;
}

AN_FORCEINLINE uint8_t Saturate8( int x ) {
    if ( (unsigned int)x <= 255 )
        return x;

    if ( x < 0 )
        return 0;

    return 255;
}

AN_FORCEINLINE uint16_t Saturate16( int x ) {
    if ( (unsigned int)x <= 65535 )
        return x;

    if ( x < 0 )
        return 0;

    return 65535;
}

AN_FORCEINLINE void MinMax( float _A, float _B, float & _Min, float & _Max ) {
    __m128 v1 = _mm_set_ss( _A );
    __m128 v2 = _mm_set_ss( _B );

    _mm_store_ss( &_Min, _mm_min_ss( v1, v2 ) );
    _mm_store_ss( &_Max, _mm_max_ss( v1, v2 ) );
#if 0
    if ( _A > _B ) {
        _Min = _B;
        _Max = _A;
    } else {
        _Min = _A;
        _Max = _B;
    }
#endif
}

AN_FORCEINLINE void MinMax( double _A, double _B, double & _Min, double & _Max ) {
    __m128d v1 = _mm_set_sd( _A );
    __m128d v2 = _mm_set_sd( _B );

    _mm_store_sd( &_Min, _mm_min_sd( v1, v2 ) );
    _mm_store_sd( &_Max, _mm_max_sd( v1, v2 ) );
}

AN_FORCEINLINE void MinMax( float _A, float _B, float _C, float & _Min, float & _Max ) {
    __m128 v1 = _mm_set_ss( _A );
    __m128 v2 = _mm_set_ss( _B );
    __m128 v3 = _mm_set_ss( _C );

    _mm_store_ss( &_Min, _mm_min_ss( _mm_min_ss( v1, v2 ), v3 ) );
    _mm_store_ss( &_Max, _mm_max_ss( _mm_max_ss( v1, v2 ), v3 ) );

#if 0
    if ( _A > _B ) {
        _Min = _B;
        _Max = _A;
    } else {
        _Min = _A;
        _Max = _B;
    }

    if ( _C > _Max ) {
        _Max = _C;
    } else if ( _C < _Min ) {
        _Min = _C;
    }
#endif
}

AN_FORCEINLINE void MinMax( double _A, double _B, double _C, double & _Min, double & _Max ) {
    __m128d v1 = _mm_set_sd( _A );
    __m128d v2 = _mm_set_sd( _B );
    __m128d v3 = _mm_set_sd( _C );

    _mm_store_sd( &_Min, _mm_min_sd( _mm_min_sd( v1, v2 ), v3 ) );
    _mm_store_sd( &_Max, _mm_max_sd( _mm_max_sd( v1, v2 ), v3 ) );
}

template< typename T >
AN_FORCEINLINE T Sqrt( T const & _Value ) {
    return _Value > T(0) ? T(StdSqrt( _Value )) : T(0);
}

template< typename T >
AN_FORCEINLINE T InvSqrt( T const & _Value ) {
    return _Value > _ZERO_TOLERANCE ? StdSqrt( T(1) / _Value ) : _INFINITY;
}

// Дает большое значение при value == 0, приближенно эквивалентен 1/sqrt(x)
AN_FORCEINLINE float RSqrt( const float & _Value ) {
    float Half = _Value * 0.5f;
    int32_t Temp = 0x5f3759df - ( *reinterpret_cast< const int32_t * >( &_Value ) >> 1 );
    float Result = *reinterpret_cast< const float * >( &Temp );
    Result = Result * ( 1.5f - Result * Result * Half );
    return Result;
}

template< typename T >
AN_FORCEINLINE T Pow( T const Value, T const Power ) {
    return std::pow( Value, Power );
}

AN_FORCEINLINE float FMod( float _X, float _Y ) {
    return std::fmod( _X, _Y );
}

AN_FORCEINLINE double FMod( double _X, double _Y ) {
    return std::fmod( _X, _Y );
}

template< typename T >
AN_INLINE T GreaterCommonDivisor( T m, T n ) {
    return ( m < T(0.0001) ) ? n : GreaterCommonDivisor( FMod( n, m ), m );
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

enum EAxialType {
    AxialX = 0,
    AxialY,
    AxialZ,
    AxialW,
    NonAxial
};

// Comparision
template< typename T >
constexpr bool LessThan( T const & A, T const & B ) { return std::isless( A, B ); }
template< typename T >
constexpr bool LequalThan( T const & A, T const & B ) { return std::islessequal( A, B ); }
template< typename T >
constexpr bool GreaterThan( T const & A, T const & B ) { return std::isgreater( A, B ); }
template< typename T >
constexpr bool GequalThan( T const & A, T const & B ) { return !std::isless( A, B ); }
template< typename T >
constexpr bool NotEqual( T const & A, T const & B ) { return std::islessgreater( A, B ); }
template< typename T >
constexpr bool Compare( T const & A, T const & B ) { return !NotEqual( A, B ); }
template< typename T >
constexpr bool CompareEps( T const & A, T const & B, T const & Epsilon ) { return Dist( A, B ) < Epsilon; }

// Trigonometric

template< typename T >
constexpr T Degrees( T const & _Rad ) { return _Rad * T(_RAD2DEG_DBL); }

template< typename T >
constexpr T Radians( T const & _Deg ) { return _Deg * T(_DEG2RAD_DBL); }

template< typename T >
AN_FORCEINLINE T Sin( T const & _Rad ) { return std::sin( _Rad ); }

template< typename T >
AN_FORCEINLINE T Cos( T const & _Rad ) { return std::cos( _Rad ); }

template< typename T >
AN_FORCEINLINE T DegSin( T const & _Deg ) { return std::sin( Radians( _Deg ) ); }

template< typename T >
AN_FORCEINLINE T DegCos( T const & _Deg ) { return std::cos( Radians( _Deg ) ); }

template< typename T >
AN_FORCEINLINE void SinCos( T const & _Rad, T & _Sin, T & _Cos ) {
    _Sin = std::sin( _Rad );
    _Cos = std::cos( _Rad );
}

template< typename T >
AN_FORCEINLINE void DegSinCos( T const & _Deg, T & _Sin, T & _Cos ) {
    SinCos( Radians( _Deg ), _Sin, _Cos );
}


AN_FORCEINLINE float Atan2( float _Y, float _X ) {
    return std::atan2( _Y, _X );
}

AN_FORCEINLINE float Atan2Fast( float _Y, float _X ) {
    const float k1 = _PI / 4.0f;
    const float k2 = 3.0f * k1;
    const float absY = Abs( _Y );
    const float angle = _X >= 0.0f ? ( k1 - k1 * ((_X - absY) / (_X + absY)) ) : ( k2 - k1 * ((_X + absY) / (absY - _X)) );
    return _Y < 0.0f ? -angle : angle;
}


// String conversion

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T ToInt( const char * _String ) {
    T val;
    T sign;
    char c;

    if ( *_String == '-' ) {
        sign = IsSigned< T >() ? -1 : 1;
        _String++;
    } else {
        sign = 1;
    }

    val = 0;

    // check for hex
    if ( _String[0] == '0' && (_String[1] == 'x' || _String[1] == 'X') ) {
        _String += 2;
        while ( 1 ) {
            c = *_String++;
            if ( c >= '0' && c <= '9' ) {
                val = (val<<4) + c - '0';
            } else if ( c >= 'a' && c <= 'f' ) {
                val = (val<<4) + c - 'a' + 10;
            } else if ( c >= 'A' && c <= 'F' ) {
                val = (val<<4) + c - 'A' + 10;
            } else {
                return val*sign;
            }
        }
    }

    // check for character
    if ( _String[0] == '\'' ) {
        return sign*T( _String[1] );
    }

    while ( 1 ) {
        c = *_String++;
        if ( c < '0' || c > '9' ) {
            return val*sign;
        }
        val = val*10 + c - '0';
    }

    return 0;
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AN_FORCEINLINE T ToInt( AString const & _String ) {
    return ToInt< T >( _String.CStr() );
}

template< typename T, typename = TStdEnableIf< IsReal<T>() > >
AN_FORCEINLINE T ToReal( const char * _String ) {
    double val;
    int sign;
    char c;
    int decimal, total;

    if ( *_String == '-' ) {
        sign = -1;
        _String++;
    }
    else {
        sign = 1;
    }

    val = 0;

    // check for hex
    if ( _String[0] == '0' && (_String[1] == 'x' || _String[1] == 'X') ) {
        _String += 2;
        while ( 1 ) {
            c = *_String++;
            if ( c >= '0' && c <= '9' ) {
                val = (val*16) + c - '0';
            }
            else if ( c >= 'a' && c <= 'f' ) {
                val = (val*16) + c - 'a' + 10;
            }
            else if ( c >= 'A' && c <= 'F' ) {
                val = (val*16) + c - 'A' + 10;
            }
            else {
                return val * sign;
            }
        }
    }

    // check for character
    if ( _String[0] == '\'' ) {
        return sign * _String[1];
    }

    decimal = -1;
    total = 0;
    while ( 1 ) {
        c = *_String++;
        if ( c == '.' ) {
            decimal = total;
            continue;
        }
        if ( c < '0' || c > '9' ) {
            break;
        }
        val = val*10 + c - '0';
        total++;
    }

    if ( decimal == -1 ) {
        return val * sign;
    }
    while ( total > decimal ) {
        val /= 10;
        total--;
    }

    return val * sign;
}

AN_FORCEINLINE bool ToBool( const char * _String ) {
    if ( !Core::Strcmp( _String, "0" )
         || !Core::Strcmp( _String, "false" ) ) {
        return false;
    } else if ( !Core::Strcmp( _String, "true" ) ) {
        return true;
    } else {
        return Math::ToInt< int >( _String ) != 0;
    }
}

AN_FORCEINLINE bool ToBool( AString const & _String ) {
    return ToBool( _String.CStr() );
}

template< typename T, typename = TStdEnableIf< IsReal<T>() > >
AN_FORCEINLINE T ToReal( AString const & _String ) {
    return ToReal< T >( _String.CStr() );
}

AN_FORCEINLINE float ToFloat( const char * _String ) {
    return ToReal< float >( _String );
}

AN_FORCEINLINE float ToFloat( AString const & _String ) {
    return ToReal< float >( _String.CStr() );
}

AN_FORCEINLINE double ToDouble( const char * _String ) {
    return ToReal< double >( _String );
}

AN_FORCEINLINE double ToDouble( AString const & _String ) {
    return ToReal< double >( _String.CStr() );
}

template< typename T, typename = TStdEnableIf< IsIntegral<T>() > >
AString ToString( T const & _Value ) {
    constexpr const char * format = IsSigned< T >() ? "%d" : "%u";
    TSprintfBuffer< 32 > buffer;
    return buffer.Sprintf( format, _Value );
}

template< typename T, typename = TStdEnableIf< IsReal<T>() > >
AN_FORCEINLINE AString ToString( T const & _Value, int _Precision = FloatingPointPrecision< T >() ) {
    TSprintfBuffer< 64 > value;
    if ( _Precision >= 0 ) {
        TSprintfBuffer< 64 > format;
        value.Sprintf( format.Sprintf( "%%.%df", _Precision ), _Value );
    } else {
        value.Sprintf( "%f", _Value );
    }
    for ( char * p = &value.Data[ Core::Strlen( value.Data ) - 1 ] ; p >= &value.Data[0] ; p-- ) {
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

AN_FORCEINLINE AString ToString( bool _Value ) {
    constexpr const char * s[2] = { "false", "true" };
    return s[ _Value ];
}

constexpr int32_t INT64_HIGH_INT( uint64_t i64 ) {
    return i64 >> 32;
}

constexpr int32_t INT64_LOW_INT( uint64_t i64 ) {
    return i64 & 0xFFFFFFFF;
}

template< typename T >
AN_FORCEINLINE AString ToHexString( T const & _Value, bool _LeadingZeros = false, bool _Prefix = false ) {
    TSprintfBuffer< 32 > value;
    TSprintfBuffer< 32 > format;
    const char * prefixStr = _Prefix ? "0x" : "";

    constexpr size_t typeSize = sizeof( T );
    static_assert( typeSize == 1 || typeSize == 2 || typeSize == 4 || typeSize == 8, "ToHexString" );

    switch( typeSize ) {
    case 1:
        format.Sprintf( _LeadingZeros ? "%s%%02x" : "%s%%x", prefixStr );
        return value.Sprintf( format.Data, *reinterpret_cast< const uint8_t * > ( &_Value ) );
    case 2:
        format.Sprintf( _LeadingZeros ? "%s%%04x" : "%s%%x", prefixStr );
        return value.Sprintf( format.Data, *reinterpret_cast< const uint16_t * > ( &_Value ) );
    case 4:
        format.Sprintf( _LeadingZeros ? "%s%%08x" : "%s%%x", prefixStr );
        return value.Sprintf( format.Data, *reinterpret_cast< const uint32_t * > ( &_Value ) );
    case 8: {
            uint64_t Temp = *reinterpret_cast< const uint64_t * > ( &_Value );
            if ( _LeadingZeros ) {
                return value.Sprintf( "%s%08x%08x", prefixStr, INT64_HIGH_INT( Temp ), INT64_LOW_INT( Temp ) );
            } else {
                return value.Sprintf( "%s%I64x", prefixStr, Temp );
            }
        }
    }
    return "";
}

}

struct Int2 {
    int32_t X, Y;

    Int2() = default;
    constexpr Int2( const int32_t & _X, const int32_t & _Y ) : X( _X ), Y( _Y ) {}

    int32_t & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&X)[ _Index ];
    }

    int32_t const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&X)[ _Index ];
    }
};

#if 0

class SignedByte2 final {
public:
    int8_t X;
    int8_t Y;

    SignedByte2() = default;
    explicit constexpr SignedByte2( const int8_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr SignedByte2( const int8_t & _X, const int8_t & _Y ) : X( _X ), Y( _Y ) {}

    int8_t & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const int8_t & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class Byte2 final {
public:
    Byte X;
    Byte Y;

    Byte2() = default;
    explicit constexpr Byte2( const uint8_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Byte2( const uint8_t & _X, const uint8_t & _Y ) : X( _X ), Y( _Y ) {}

    Byte & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Byte & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class Short2 final {
public:
    Short X;
    Short Y;

    Short2() = default;
    explicit constexpr Short2( const int16_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Short2( const int16_t & _X, const int16_t & _Y ) : X( _X ), Y( _Y ) {}

    Short & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Short & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class UShort2 final {
public:
    UShort X;
    UShort Y;

    UShort2() = default;
    explicit constexpr UShort2( const uint16_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr UShort2( const uint16_t & _X, const uint16_t & _Y ) : X( _X ), Y( _Y ) {}

    UShort & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const UShort & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class Int2 final {
public:
    Int X;
    Int Y;

    Int2() = default;
    explicit constexpr Int2( const int32_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Int2( const int32_t & _X, const int32_t & _Y ) : X( _X ), Y( _Y ) {}

    Int & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Int & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class UInt2 final {
public:
    UInt X;
    UInt Y;

    UInt2() = default;
    explicit constexpr UInt2( const uint32_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr UInt2( const uint32_t & _X, const uint32_t & _Y ) : X( _X ), Y( _Y ) {}

    UInt & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const UInt & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class Long2 final {
public:
    Long X;
    Long Y;

    Long2() = default;
    explicit constexpr Long2( const int64_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Long2( const int64_t & _X, const int64_t & _Y ) : X( _X ), Y( _Y ) {}

    Long & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Long & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class ULong2 final {
public:
    ULong X;
    ULong Y;

    ULong2() = default;
    explicit constexpr ULong2( const uint64_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr ULong2( const uint64_t & _X, const uint64_t & _Y ) : X( _X ), Y( _Y ) {}

    ULong & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const ULong & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
};
class SignedByte3 final {
public:
    int8_t X;
    int8_t Y;
    int8_t Z;

    SignedByte3() = default;
    explicit constexpr SignedByte3( const int8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr SignedByte3( const int8_t & _X, const int8_t & _Y, const int8_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    int8_t & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const int8_t & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class Byte3 final {
public:
    Byte X;
    Byte Y;
    Byte Z;

    Byte3() = default;
    explicit constexpr Byte3( const uint8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Byte3( const uint8_t & _X, const uint8_t & _Y, const uint8_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    Byte & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Byte & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class Short3 final {
public:
    Short X;
    Short Y;
    Short Z;

    Short3() = default;
    explicit constexpr Short3( const int16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Short3( const int16_t & _X, const int16_t & _Y, const int16_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    Short & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Short & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class UShort3 final {
public:
    UShort X;
    UShort Y;
    UShort Z;

    UShort3() = default;
    explicit constexpr UShort3( const uint16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr UShort3( const uint16_t & _X, const uint16_t & _Y, const uint16_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    UShort & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const UShort & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class Int3 final {
public:
    Int X;
    Int Y;
    Int Z;

    Int3() = default;
    explicit constexpr Int3( const int32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Int3( const int32_t & _X, const int32_t & _Y, const int32_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    Int & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Int & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class UInt3 final {
public:
    UInt X;
    UInt Y;
    UInt Z;

    UInt3() = default;
    explicit constexpr UInt3( const uint32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr UInt3( const uint32_t & _X, const uint32_t & _Y, const uint32_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    UInt & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const UInt & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class Long3 final {
public:
    Long X;
    Long Y;
    Long Z;

    Long3() = default;
    explicit constexpr Long3( const int64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Long3( const int64_t & _X, const int64_t & _Y, const int64_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    Long & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Long & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class ULong3 final {
public:
    ULong X;
    ULong Y;
    ULong Z;

    ULong3() = default;
    explicit constexpr ULong3( const uint64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr ULong3( const uint64_t & _X, const uint64_t & _Y, const uint64_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    ULong & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const ULong & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
};
class SignedByte4 final {
public:
    int8_t X;
    int8_t Y;
    int8_t Z;
    int8_t W;

    SignedByte4() = default;
    explicit constexpr SignedByte4( const int8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr SignedByte4( const int8_t & _X, const int8_t & _Y, const int8_t & _Z, const int8_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    int8_t & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const int8_t & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};
class Byte4 final {
public:
    Byte X;
    Byte Y;
    Byte Z;
    Byte W;

    Byte4() = default;
    explicit constexpr Byte4( const uint8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Byte4( const uint8_t & _X, const uint8_t & _Y, const uint8_t & _Z, const uint8_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    Byte & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Byte & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};
class Short4 final {
public:
    Short X;
    Short Y;
    Short Z;
    Short W;

    Short4() = default;
    explicit constexpr Short4( const int16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Short4( const int16_t & _X, const int16_t & _Y, const int16_t & _Z, const int16_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    Short & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Short & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};
class UShort4 final {
public:
    UShort X;
    UShort Y;
    UShort Z;
    UShort W;

    UShort4() = default;
    explicit constexpr UShort4( const uint16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr UShort4( const uint16_t & _X, const uint16_t & _Y, const uint16_t & _Z, const uint16_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    UShort & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const UShort & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};
class Int4 final {
public:
    Int X;
    Int Y;
    Int Z;
    Int W;

    Int4() = default;
    explicit constexpr Int4( const int32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Int4( const int32_t & _X, const int32_t & _Y, const int32_t & _Z, const int32_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    Int & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Int & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};
class UInt4 final {
public:
    UInt X;
    UInt Y;
    UInt Z;
    UInt W;

    UInt4() = default;
    explicit constexpr UInt4( const uint32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr UInt4( const uint32_t & _X, const uint32_t & _Y, const uint32_t & _Z, const uint32_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    UInt & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const UInt & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};
class Long4 final {
public:
    Long X;
    Long Y;
    Long Z;
    Long W;

    Long4() = default;
    explicit constexpr Long4( const int64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Long4( const int64_t & _X, const int64_t & _Y, const int64_t & _Z, const int64_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    Long & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const Long & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};
class ULong4 final {
public:
    ULong X;
    ULong Y;
    ULong Z;
    ULong W;

    ULong4() = default;
    explicit constexpr ULong4( const uint64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr ULong4( const uint64_t & _X, const uint64_t & _Y, const uint64_t & _Z, const uint64_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    ULong & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const ULong & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        X.Write( _Stream );
        Y.Write( _Stream );
        Z.Write( _Stream );
        W.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        X.Read( _Stream );
        Y.Read( _Stream );
        Z.Read( _Stream );
        W.Read( _Stream );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
};

#endif
