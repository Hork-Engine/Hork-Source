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

#include "BaseTypes.h"

namespace FMath {

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

template< typename Type > AN_FORCEINLINE const Type & Min( const Type & _A, const Type & _B ) { return FMath_Min( _A, _B ); }
template< typename Type > AN_FORCEINLINE const Type & Max( const Type & _A, const Type & _B ) { return FMath_Max( _A, _B ); }
template< typename Type > AN_FORCEINLINE const Type & Min( const Type & _A, const Type & _B, const Type & _C ) { return FMath_Min( FMath_Min( _A, _B ), _C ); }
template< typename Type > AN_FORCEINLINE const Type & Max( const Type & _A, const Type & _B, const Type & _C ) { return FMath_Max( FMath_Max( _A, _B ), _C ); }

template< typename Type >
AN_FORCEINLINE void MinMax( const Type & _A, const Type & _B, Type & _Min, Type & _Max ) {
    if ( _A > _B ) {
        _Min = _B;
        _Max = _A;
    } else {
        _Min = _A;
        _Max = _B;
    }
}

template< typename Type >
AN_FORCEINLINE void MinMax( const Type & _A, const Type & _B, const Type & _C, Type & _Min, Type & _Max ) {
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
}

template< typename Type >
AN_FORCEINLINE Type Sqrt( const Type & _Value ) {
    return _Value > Type(0) ? Type(StdSqrt( _Value )) : Type(0);
}

template< typename Type >
AN_FORCEINLINE Type InvSqrt( const Type & _Value ) {
    return _Value > _ZERO_TOLERANCE ? StdSqrt( Type(1) / _Value ) : _INFINITY;
}

// Дает большое значение при value == 0, приближенно эквивалентен 1/sqrt(x)
AN_FORCEINLINE float RSqrt( const float & _Value ) {
    float Half = _Value * 0.5f;
    int32_t Temp = 0x5f3759df - ( *reinterpret_cast< const int32_t * >( &_Value ) >> 1 );
    float Result = *reinterpret_cast< const float * >( &Temp );
    Result = Result * ( 1.5f - Result * Result * Half );
    return Result;
}

enum EAxialType {
    AxialX = 0,
    AxialY,
    AxialZ,
    AxialW,
    NonAxial
};

// Возвращает значения от 0 включительно до 1 включительно
AN_FORCEINLINE float Rand() { return (rand()&0x7FFF)/((float)0x7FFF); }

// Возвращает значения от 0 включительно до 1 не включительно
AN_FORCEINLINE float Rand1() { return (rand()&0x7FFF)/((float)0x8000); }

// Возвращает значения от -1 включительно до 1 включительно
AN_FORCEINLINE float Rand2() { return 2.0f*(Rand() - 0.5f); }

// Возвращает значения в заданном диапазоне включительно
AN_FORCEINLINE float Rand( float from, float to ) { return from + (to-from) * Rand(); }

// Возвращает значения от from включительно до to не включительно
AN_FORCEINLINE float Rand1( float from, float to ) { return from + (to-from) * Rand1(); }

AN_FORCEINLINE int Log2( int32_t value ) {
    int log2 = 0;
    while ( value >>= 1 )
        log2++;
    return log2;
}

AN_FORCEINLINE int Log2( int64_t value ) {
    int log2 = 0;
    while ( value >>= 1 )
        log2++;
    return log2;
}

}
