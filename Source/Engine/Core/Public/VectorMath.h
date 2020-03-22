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

#include "Bool.h"

template< typename T >
struct TVector2;

template< typename T >
struct TVector3;

template< typename T >
struct TVector4;

namespace  Math {

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr float Dot( TVector2< T > const & A, TVector2< T > const & B ) {
    return A.X * B.X + A.Y * B.Y;
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr float Dot( TVector3< T > const & A, TVector3< T > const & B ) {
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr float Dot( TVector4< T > const & A, TVector4< T > const & B ) {
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr T Cross( TVector2< T > const & A, TVector2< T > const & B ) {
    return A.X * B.Y - A.Y * B.X;
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector3< T > Cross( TVector3< T > const & A, TVector3< T > const & B ) {
    return TVector3< T >( A.Y * B.Z - B.Y * A.Z,
                          A.Z * B.X - B.Z * A.X,
                          A.X * B.Y - B.X * A.Y );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector2< T > Reflect( TVector2< T > const & _IncidentVector, TVector2< T > const & _Normal ) {
    return _IncidentVector - 2 * Dot( _Normal, _IncidentVector ) * _Normal;
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector3< T > Reflect( TVector3< T > const & _IncidentVector, TVector3< T > const & _Normal ) {
    return _IncidentVector - 2 * Dot( _Normal, _IncidentVector ) * _Normal;
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
AN_FORCEINLINE TVector2< T > Refract( TVector2< T > const & _IncidentVector, TVector2< T > const & _Normal, T const _Eta ) {
    const T NdotI = Dot( _Normal, _IncidentVector );
    const T k = T(1) - _Eta*_Eta * (T(1) - NdotI*NdotI);
    if ( k < T(0) ) {
        return TVector2< T >( T(0) );
    }
    return _IncidentVector * _Eta - _Normal * (_Eta * NdotI + sqrt(k));
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
AN_FORCEINLINE TVector3< T > Refract( TVector3< T > const & _IncidentVector, TVector3< T > const & _Normal, const T _Eta ) {
    const T NdotI = Dot( _Normal, _IncidentVector );
    const T k = T(1) - _Eta*_Eta * (T(1) - NdotI*NdotI);
    if ( k < T(0) ) {
        return TVector3< T >( T(0) );
    }
    return _IncidentVector * _Eta - _Normal * (_Eta * NdotI + sqrt(k));
}

template< typename T >
constexpr TVector2< T > Lerp( TVector2< T > const & _From, TVector2< T > const & _To, T const & _Mix ) {
    return _From + _Mix * (_To - _From);
}

template< typename T >
constexpr TVector3< T > Lerp( TVector3< T > const & _From, TVector3< T > const & _To, T const & _Mix ) {
    return _From + _Mix * (_To - _From);
}

template< typename T >
constexpr TVector4< T > Lerp( TVector4< T > const & _From, TVector4< T > const & _To, T const & _Mix ) {
    return _From + _Mix * (_To - _From);
}

template< typename T >
constexpr T Bilerp( T const & _A, T const & _B, T const & _C, T const & _D, TVector2< T > const & _Lerp ) {
    return _A * (T( 1 ) - _Lerp.X) * (T( 1 ) - _Lerp.Y) + _B * _Lerp.X * (T( 1 ) - _Lerp.Y) + _C * (T( 1 ) - _Lerp.X) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector2< T > Bilerp( TVector2< T > const & _A, TVector2< T > const & _B, TVector2< T > const & _C, TVector2< T > const & _D, TVector2< T > const & _Lerp ) {
    return _A * (T( 1 ) - _Lerp.X) * (T( 1 ) - _Lerp.Y) + _B * _Lerp.X * (T( 1 ) - _Lerp.Y) + _C * (T( 1 ) - _Lerp.X) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector3< T > Bilerp( TVector3< T > const & _A, TVector3< T > const & _B, TVector3< T > const & _C, TVector3< T > const & _D, TVector2< T > const & _Lerp ) {
    return _A * (T( 1 ) - _Lerp.X) * (T( 1 ) - _Lerp.Y) + _B * _Lerp.X * (T( 1 ) - _Lerp.Y) + _C * (T( 1 ) - _Lerp.X) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector4< T > Bilerp( TVector4< T > const & _A, TVector4< T > const & _B, TVector4< T > const & _C, TVector4< T > const & _D, TVector2< T > const & _Lerp ) {
    return _A * (T( 1 ) - _Lerp.X) * (T( 1 ) - _Lerp.Y) + _B * _Lerp.X * (T( 1 ) - _Lerp.Y) + _C * (T( 1 ) - _Lerp.X) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector2< T > Step( TVector2< T > const & _Vec, T const & _Edge ) {
    return TVector2< T >( _Vec.X < _Edge ? T( 0 ) : T( 1 ), _Vec.Y < _Edge ? T( 0 ) : T( 1 ) );
}

template< typename T >
constexpr TVector2< T > Step( TVector2< T > const & _Vec, TVector2< T > const & _Edge ) {
    return TVector2< T >( _Vec.X < _Edge.X ? T( 0 ) : T( 1 ), _Vec.Y < _Edge.Y ? T( 0 ) : T( 1 ) );
}

template< typename T >
AN_FORCEINLINE TVector2< T > SmoothStep( TVector2< T > const & _Vec, T const & _Edge0, T const & _Edge1 ) {
    const T Denom = T( 1 ) / (_Edge1 - _Edge0);
    const TVector2< T > t = ((_Vec - _Edge0) * Denom).Saturate();
    return t * t * (T(-2) * t + T(3));
}

template< typename T >
AN_FORCEINLINE TVector2< T > SmoothStep( TVector2< T > const & _Vec, TVector2< T > const & _Edge0, TVector2< T > const & _Edge1 ) {
    const TVector2< T > t = ((_Vec - _Edge0) / (_Edge1 - _Edge0)).Saturate();
    return t * t * (T(-2) * t + T(3));
}

template< typename T >
constexpr TVector3< T > Step( TVector3< T > const & _Vec, T const & _Edge ) {
    return TVector3< T >( _Vec.X < _Edge ? T( 0 ) : T( 1 ), _Vec.Y < _Edge ? T( 0 ) : T( 1 ), _Vec.Z < _Edge ? T( 0 ) : T( 1 ) );
}

template< typename T >
constexpr TVector3< T > Step( TVector3< T > const & _Vec, TVector2< T > const & _Edge ) {
    return TVector3< T >( _Vec.X < _Edge.X ? T( 0 ) : T( 1 ), _Vec.Y < _Edge.Y ? T( 0 ) : T( 1 ), _Vec.Z < _Edge.Z ? T( 0 ) : T( 1 ) );
}

template< typename T >
AN_FORCEINLINE TVector3< T > SmoothStep( TVector3< T > const & _Vec, T const & _Edge0, T const & _Edge1 ) {
    const T Denom = T( 1 ) / (_Edge1 - _Edge0);
    const TVector3< T > t = ((_Vec - _Edge0) * Denom).Saturate();
    return t * t * (T(-2) * t + T(3));
}

template< typename T >
AN_FORCEINLINE TVector3< T > SmoothStep( TVector3< T > const & _Vec, TVector2< T > const & _Edge0, TVector2< T > const & _Edge1 ) {
    const TVector3< T > t = ((_Vec - _Edge0) / (_Edge1 - _Edge0)).Saturate();
    return t * t * (T(-2) * t + T(3));
}

template< typename T >
constexpr TVector4< T > Step( TVector4< T > const & _Vec, T const & _Edge ) {
    return TVector4< T >( _Vec.X < _Edge ? T( 0 ) : T( 1 ), _Vec.Y < _Edge ? T( 0 ) : T( 1 ), _Vec.Z < _Edge ? T( 0 ) : T( 1 ), _Vec.W < _Edge ? T( 0 ) : T( 1 ) );
}

template< typename T >
constexpr TVector4< T > Step( TVector4< T > const & _Vec, TVector4< T > const & _Edge ) {
    return TVector4< T >( _Vec.X < _Edge.X ? T( 0 ) : T( 1 ), _Vec.Y < _Edge.Y ? T( 0 ) : T( 1 ), _Vec.Z < _Edge.Z ? T( 0 ) : T( 1 ), _Vec.W < _Edge.W ? T( 0 ) : T( 1 ) );
}

template< typename T >
AN_FORCEINLINE TVector4< T > SmoothStep( TVector4< T > const & _Vec, T const & _Edge0, T const & _Edge1 ) {
    const T Denom = T( 1 ) / (_Edge1 - _Edge0);
    const TVector4< T > t = ((_Vec - _Edge0) * Denom).Saturate();
    return t * t * (T(-2) * t + T(3));
}

template< typename T >
AN_FORCEINLINE TVector4< T > SmoothStep( TVector4< T > const & _Vec, TVector4< T > const & _Edge0, TVector4< T > const & _Edge1 ) {
    const TVector4< T > t = ((_Vec - _Edge0) / (_Edge1 - _Edge0)).Saturate();
    return t * t * (T(-2) * t + T(3));
}

}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector2< T > operator+( T const & _Left, TVector2< T > const & _Right ) {
    return TVector2< T >( _Left + _Right.X, _Left + _Right.Y );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector2< T > operator-( T const & _Left, TVector2< T > const & _Right ) {
    return TVector2< T >( _Left - _Right.X, _Left - _Right.Y );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector3< T > operator+( T const & _Left, TVector3< T > const & _Right ) {
    return TVector3< T >( _Left + _Right.X, _Left + _Right.Y, _Left + _Right.Z );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector3< T > operator-( T const & _Left, TVector3< T > const & _Right ) {
    return TVector3< T >( _Left - _Right.X, _Left - _Right.Y, _Left - _Right.Z );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector4< T > operator+( T const & _Left, TVector4< T > const & _Right ) {
    return TVector4< T >( _Left + _Right.X, _Left + _Right.Y, _Left + _Right.Z, _Left + _Right.W );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector4< T > operator-( T const & _Left, TVector4< T > const & _Right ) {
    return TVector4< T >( _Left - _Right.X, _Left - _Right.Y, _Left - _Right.Z, _Left - _Right.W );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector2< T > operator*( T const & _Left, TVector2< T > const & _Right ) {
    return TVector2< T >( _Left * _Right.X, _Left * _Right.Y );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector3< T > operator*( T const & _Left, TVector3< T > const & _Right ) {
    return TVector3< T >( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z );
}

template< typename T, typename = TStdEnableIf< Math::IsReal<T>() > >
constexpr TVector4< T > operator*( T const & _Left, TVector4< T > const & _Right ) {
    return TVector4< T >( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z, _Left * _Right.W );
}

template< typename T >
struct TVector2 {
    T X;
    T Y;

    TVector2() = default;

    constexpr explicit TVector2( T const & _Value ) : X( _Value ), Y( _Value ) {}

    constexpr TVector2( T const & _X, T const & _Y ) : X( _X ), Y( _Y ) {}

    constexpr explicit TVector2( TVector3< T > const & _Value ) : X( _Value.X ), Y( _Value.Y ) {}
    constexpr explicit TVector2( TVector4< T > const & _Value ) : X( _Value.X ), Y( _Value.Y ) {}

    template< typename T2 >
    constexpr explicit TVector2< T >( TVector2< T2 > const & _Value )
        : X( _Value.X ), Y( _Value.Y ) {}

    template< typename T2 >
    constexpr explicit TVector2< T >( TVector3< T2 > const & _Value )
        : X( _Value.X ), Y( _Value.Y ) {}

    template< typename T2 >
    constexpr explicit TVector2< T >( TVector4< T2 > const & _Value )
        : X( _Value.X ), Y( _Value.Y ) {}

    T * ToPtr() {
        return &X;
    }

    T const * ToPtr() const {
        return &X;
    }

//    TVector2 & operator=( TVector2 const & _Other ) {
//        X = _Other.X;
//        Y = _Other.Y;
//        return *this;
//    }

    T & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    T const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int Index >
    constexpr T const & Get() const {
        static_assert( Index >= 0 && Index < NumComponents(), "Index out of range" );
        return (&X)[ Index ];
    }

    template< int _Shuffle >
    constexpr TVector2< T > Shuffle2() const {
        return TVector2< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>() );
    }

    template< int _Shuffle >
    constexpr TVector3< T > Shuffle3() const {
        return TVector3< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>() );
    }

    template< int _Shuffle >
    constexpr TVector4< T > Shuffle4() const {
        return TVector4< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>(), Get<_Shuffle & 3>() );
    }

    constexpr bool operator==( TVector2 const & _Other ) const { return Compare( _Other ); }
    constexpr bool operator!=( TVector2 const & _Other ) const { return !Compare( _Other ); }

    // Math operators
    constexpr TVector2 operator+() const {
        return *this;
    }
    constexpr TVector2 operator-() const {
        return TVector2( -X, -Y );
    }
    constexpr TVector2 operator+( TVector2 const & _Other ) const {
        return TVector2( X + _Other.X, Y + _Other.Y );
    }
    constexpr TVector2 operator-( TVector2 const & _Other ) const {
        return TVector2( X - _Other.X, Y - _Other.Y );
    }
    constexpr TVector2 operator/( TVector2 const & _Other ) const {
        return TVector2( X / _Other.X, Y / _Other.Y );
    }
    constexpr TVector2 operator*( TVector2 const & _Other ) const {
        return TVector2( X * _Other.X, Y * _Other.Y );
    }
    constexpr TVector2 operator+( T const & _Other ) const {
        return TVector2( X + _Other, Y + _Other );
    }
    constexpr TVector2 operator-( T const & _Other ) const {
        return TVector2( X - _Other, Y - _Other );
    }
    TVector2 operator/( T const & _Other ) const {
        T Denom = T( 1 ) / _Other;
        return TVector2( X * Denom, Y * Denom );
    }
    constexpr TVector2 operator*( T const & _Other ) const {
        return TVector2( X * _Other, Y * _Other );
    }
    void operator+=( TVector2 const & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
    }
    void operator-=( TVector2 const & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
    }
    void operator/=( TVector2 const & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
    }
    void operator*=( TVector2 const & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
    }
    void operator+=( T const & _Other ) {
        X += _Other;
        Y += _Other;
    }
    void operator-=( T const & _Other ) {
        X -= _Other;
        Y -= _Other;
    }
    void operator/=( T const & _Other ) {
        T Denom = T( 1 ) / _Other;
        X *= Denom;
        Y *= Denom;
    }
    void operator*=( T const & _Other ) {
        X *= _Other;
        Y *= _Other;
    }

    T Min() const {
        return Math::Min( X, Y );
    }

    T Max() const {
        return Math::Max( X, Y );
    }

    int MinorAxis() const {
        return int( Math::Abs( X ) >= Math::Abs( Y ) );
    }

    int MajorAxis() const {
        return int( Math::Abs( X ) < Math::Abs( Y ) );
    }

    // Floating point specific
    constexpr Bool2 IsInfinite() const {
        return Bool2( Math::IsInfinite( X ), Math::IsInfinite( Y ) );
    }

    constexpr Bool2 IsNan() const {
        return Bool2( Math::IsNan( X ), Math::IsNan( Y ) );
    }

    constexpr Bool2 IsNormal() const {
        return Bool2( Math::IsNormal( X ), Math::IsNormal( Y ) );
    }

    constexpr Bool2 IsDenormal() const {
        return Bool2( Math::IsDenormal( X ), Math::IsDenormal( Y ) );
    }

    // Comparision

    constexpr Bool2 LessThan( TVector2 const & _Other ) const {
        return Bool2( Math::LessThan( X, _Other.X ), Math::LessThan( Y, _Other.Y ) );
    }
    constexpr Bool2 LessThan( T const & _Other ) const {
        return Bool2( Math::LessThan( X, _Other ), Math::LessThan( Y, _Other ) );
    }

    constexpr Bool2 LequalThan( TVector2 const & _Other ) const {
        return Bool2( Math::LequalThan( X, _Other.X ), Math::LequalThan( Y, _Other.Y ) );
    }
    constexpr Bool2 LequalThan( T const & _Other ) const {
        return Bool2( Math::LequalThan( X, _Other ), Math::LequalThan( Y, _Other ) );
    }

    constexpr Bool2 GreaterThan( TVector2 const & _Other ) const {
        return Bool2( Math::GreaterThan( X, _Other.X ), Math::GreaterThan( Y, _Other.Y ) );
    }
    constexpr Bool2 GreaterThan( T const & _Other ) const {
        return Bool2( Math::GreaterThan( X, _Other ), Math::GreaterThan( Y, _Other ) );
    }

    constexpr Bool2 GequalThan( TVector2 const & _Other ) const {
        return Bool2( Math::GequalThan( X, _Other.X ), Math::GequalThan( Y, _Other.Y ) );
    }
    constexpr Bool2 GequalThan( T const & _Other ) const {
        return Bool2( Math::GequalThan( X, _Other ), Math::GequalThan( Y, _Other ) );
    }

    constexpr Bool2 NotEqual( TVector2 const & _Other ) const {
        return Bool2( Math::NotEqual( X, _Other.X ), Math::NotEqual( Y, _Other.Y ) );
    }
    constexpr Bool2 NotEqual( T const & _Other ) const {
        return Bool2( Math::NotEqual( X, _Other ), Math::NotEqual( Y, _Other ) );
    }

    constexpr bool Compare( TVector2 const & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    constexpr bool CompareEps( TVector2 const & _Other, T const & _Epsilon ) const {
        return Bool2( Math::CompareEps( X, _Other.X, _Epsilon ),
                      Math::CompareEps( Y, _Other.Y, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = 0;
    }

    TVector2 Abs() const { return TVector2( Math::Abs( X ), Math::Abs( Y ) ); }

    // Vector methods
    constexpr T LengthSqr() const {
        return X * X + Y * Y;
    }

    T Length() const {
        return StdSqrt( LengthSqr() );
    }

    constexpr T DistSqr( TVector2 const & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }

    T Dist( TVector2 const & _Other ) const {
        return ( *this - _Other ).Length();
    }

    T NormalizeSelf() {
        T L = Length();
        if ( L != T( 0 ) ) {
            T InvLength = T( 1 ) / L;
            X *= InvLength;
            Y *= InvLength;
        }
        return L;
    }

    TVector2 Normalized() const {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            return TVector2( X * InvLength, Y * InvLength );
        }
        return *this;
    }

    TVector2 Floor() const {
        return TVector2( Math::Floor( X ), Math::Floor( Y ) );
    }

    TVector2 Ceil() const {
        return TVector2( Math::Ceil( X ), Math::Ceil( Y ) );
    }

    TVector2 Fract() const {
        return TVector2( Math::Fract( X ), Math::Fract( Y ) );
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector2 Sign() const {
        return TVector2( Math::Sign( X ), Math::Sign( Y ) );
    }

    constexpr int SignBits() const {
        return Math::SignBits( X ) | (Math::SignBits( Y )<<1);
    }

    TVector2 Clamp( T const & _Min, T const & _Max ) const {
        return TVector2( Math::Clamp( X, _Min, _Max ), Math::Clamp( Y, _Min, _Max ) );
    }

    TVector2 Clamp( TVector2 const & _Min, TVector2 const & _Max ) const {
        return TVector2( Math::Clamp( X, _Min.X, _Max.X ), Math::Clamp( Y, _Min.Y, _Max.Y ) );
    }

    TVector2 Saturate() const {
        return Clamp( T( 0 ), T( 1 ) );
    }

    TVector2 Snap( T const &_SnapValue ) const {
        AN_ASSERT_( _SnapValue > 0, "Snap" );
        TVector2 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = Math::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = Math::Round( SnapVector.Y ) * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const {
        if ( X == T( 1 ) || X == T( -1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) || Y == T( -1 ) ) return Math::AxialY;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == T( 1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) ) return Math::AxialY;
        return Math::NonAxial;
    }

    int VectorAxialType() const {
        if ( Math::Abs( X ) < T(0.00001) ) {
            return ( Math::Abs( Y ) < T(0.00001) ) ? Math::NonAxial : Math::AxialY;
        }
        return ( Math::Abs( Y ) < T(0.00001) ) ? Math::AxialX : Math::NonAxial;
    }

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< T >() ) const {
        return AString( "( " ) + Math::ToString( X, _Precision ) + " " + Math::ToString( Y, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {

        struct Writer {
            Writer( IBinaryStream & _Stream, const float & _Value ) {
                _Stream.WriteFloat( _Value );
            }

            Writer( IBinaryStream & _Stream, const double & _Value ) {
                _Stream.WriteDouble( _Value );
            }
        };
        Writer( _Stream, X );
        Writer( _Stream, Y );
    }

    void Read( IBinaryStream & _Stream ) {

        struct Reader {
            Reader( IBinaryStream & _Stream, float & _Value ) {
                _Value = _Stream.ReadFloat();
            }

            Reader( IBinaryStream & _Stream, double & _Value ) {
                _Value = _Stream.ReadDouble();
            }
        };
        Reader( _Stream, X );
        Reader( _Stream, Y );
    }

    // Static methods
    static constexpr int NumComponents() { return 2; }
    static TVector2 const & Zero() {
        static constexpr TVector2< T > ZeroVec(T(0));
        return ZeroVec;
    }
};

template< typename T >
struct TVector3 {
    T X;
    T Y;
    T Z;

    TVector3() = default;

    constexpr explicit TVector3( T const & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}

    constexpr TVector3( T const & _X, T const & _Y, T const & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    template< typename T2 >
    constexpr explicit TVector3< T >( TVector2< T2 > const & _Value, T2 const & _Z = T2(0) )
        : X(_Value.X), Y(_Value.Y), Z(_Z) {}

    template< typename T2 >
    constexpr explicit TVector3< T >( TVector3< T2 > const & _Value )
        : X(_Value.X), Y(_Value.Y), Z(_Value.Z) {}

    template< typename T2 >
    constexpr explicit TVector3< T >( TVector4< T2 > const & _Value )
        : X(_Value.X), Y(_Value.Y), Z(_Value.Z) {}

    T * ToPtr() {
        return &X;
    }

    T const * ToPtr() const {
        return &X;
    }

//    TVector3 & operator=( TVector3 const & _Other ) {
//        X = _Other.X;
//        Y = _Other.Y;
//        Z = _Other.Z;
//        return *this;
//    }

    T & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    T const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int Index >
    constexpr T const & Get() const {
        static_assert( Index >= 0 && Index < NumComponents(), "Index out of range" );
        return (&X)[ Index ];
    }

    template< int _Shuffle >
    constexpr TVector2< T > Shuffle2() const {
        return TVector2< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>() );
    }

    template< int _Shuffle >
    constexpr TVector3< T > Shuffle3() const {
        return TVector3< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>() );
    }

    template< int _Shuffle >
    constexpr TVector4< T > Shuffle4() const {
        return TVector4< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>(), Get<_Shuffle & 3>() );
    }

    constexpr bool operator==( TVector3 const & _Other ) const { return Compare( _Other ); }
    constexpr bool operator!=( TVector3 const & _Other ) const { return !Compare( _Other ); }

    // Math operators
    constexpr TVector3 operator+() const {
        return *this;
    }
    constexpr TVector3 operator-() const {
        return TVector3( -X, -Y, -Z );
    }
    constexpr TVector3 operator+( TVector3 const & _Other ) const {
        return TVector3( X + _Other.X, Y + _Other.Y, Z + _Other.Z );
    }
    constexpr TVector3 operator-( TVector3 const & _Other ) const {
        return TVector3( X - _Other.X, Y - _Other.Y, Z - _Other.Z );
    }
    constexpr TVector3 operator/( TVector3 const & _Other ) const {
        return TVector3( X / _Other.X, Y / _Other.Y, Z / _Other.Z );
    }
    constexpr TVector3 operator*( TVector3 const & _Other ) const {
        return TVector3( X * _Other.X, Y * _Other.Y, Z * _Other.Z );
    }
    constexpr TVector3 operator+( T const & _Other ) const {
        return TVector3( X + _Other, Y + _Other, Z + _Other );
    }
    constexpr TVector3 operator-( T const & _Other ) const {
        return TVector3( X - _Other, Y - _Other, Z - _Other );
    }
    TVector3 operator/( T const & _Other ) const {
        T Denom = T( 1 ) / _Other;
        return TVector3( X * Denom, Y * Denom, Z * Denom );
    }
    constexpr TVector3 operator*( T const & _Other ) const {
        return TVector3( X * _Other, Y * _Other, Z * _Other );
    }
    void operator+=( TVector3 const & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
    }
    void operator-=( TVector3 const & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
    }
    void operator/=( TVector3 const & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
    }
    void operator*=( TVector3 const & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
    }
    void operator+=( T const & _Other ) {
        X += _Other;
        Y += _Other;
        Z += _Other;
    }
    void operator-=( T const & _Other ) {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
    }
    void operator/=( T const & _Other ) {
        T Denom = T( 1 ) / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
    }
    void operator*=( T const & _Other ) {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
    }

    T Min() const {
        return Math::Min( Math::Min( X, Y ), Z );
    }

    T Max() const {
        return Math::Max( Math::Max( X, Y ), Z );
    }

    int MinorAxis() const {
        T Minor = Math::Abs( X );
        int Axis = 0;
        T Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp <= Minor ) {
            Axis = 1;
            Minor = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp <= Minor ) {
            Axis = 2;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const {
        T Major = Math::Abs( X );
        int Axis = 0;
        T Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp > Major ) {
            Axis = 1;
            Major = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp > Major ) {
            Axis = 2;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    constexpr Bool3 IsInfinite() const {
        return Bool3( Math::IsInfinite( X ), Math::IsInfinite( Y ), Math::IsInfinite( Z ) );
    }

    constexpr Bool3 IsNan() const {
        return Bool3( Math::IsNan( X ), Math::IsNan( Y ), Math::IsNan( Z ) );
    }

    constexpr Bool3 IsNormal() const {
        return Bool3( Math::IsNormal( X ), Math::IsNormal( Y ), Math::IsNormal( Z ) );
    }

    constexpr Bool3 IsDenormal() const {
        return Bool3( Math::IsDenormal( X ), Math::IsDenormal( Y ), Math::IsDenormal( Z ) );
    }

    // Comparision
    constexpr Bool3 LessThan( TVector3 const & _Other ) const {
        return Bool3( Math::LessThan( X, _Other.X ), Math::LessThan( Y, _Other.Y ), Math::LessThan( Z, _Other.Z ) );
    }
    constexpr Bool3 LessThan( T const & _Other ) const {
        return Bool3( Math::LessThan( X, _Other ), Math::LessThan( Y, _Other ), Math::LessThan( Z, _Other ) );
    }

    constexpr Bool3 LequalThan( TVector3 const & _Other ) const {
        return Bool3( Math::LequalThan( X, _Other.X ), Math::LequalThan( Y, _Other.Y ), Math::LequalThan( Z, _Other.Z ) );
    }
    constexpr Bool3 LequalThan( T const & _Other ) const {
        return Bool3( Math::LequalThan( X, _Other ), Math::LequalThan( Y, _Other ), Math::LequalThan( Z, _Other ) );
    }

    constexpr Bool3 GreaterThan( TVector3 const & _Other ) const {
        return Bool3( Math::GreaterThan( X, _Other.X ), Math::GreaterThan( Y, _Other.Y ), Math::GreaterThan( Z, _Other.Z ) );
    }
    constexpr Bool3 GreaterThan( T const & _Other ) const {
        return Bool3( Math::GreaterThan( X, _Other ), Math::GreaterThan( Y, _Other ), Math::GreaterThan( Z, _Other ) );
    }

    constexpr Bool3 GequalThan( TVector3 const & _Other ) const {
        return Bool3( Math::GequalThan( X, _Other.X ), Math::GequalThan( Y, _Other.Y ), Math::GequalThan( Z, _Other.Z ) );
    }
    constexpr Bool3 GequalThan( T const & _Other ) const {
        return Bool3( Math::GequalThan( X, _Other ), Math::GequalThan( Y, _Other ), Math::GequalThan( Z, _Other ) );
    }

    constexpr Bool3 NotEqual( TVector3 const & _Other ) const {
        return Bool3( Math::NotEqual( X, _Other.X ), Math::NotEqual( Y, _Other.Y ), Math::NotEqual( Z, _Other.Z ) );
    }
    constexpr Bool3 NotEqual( T const & _Other ) const {
        return Bool3( Math::NotEqual( X, _Other ), Math::NotEqual( Y, _Other ), Math::NotEqual( Z, _Other ) );
    }

    constexpr bool Compare( TVector3 const & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    constexpr bool CompareEps( TVector3 const & _Other, T const & _Epsilon ) const {
        return Bool3( Math::CompareEps( X, _Other.X, _Epsilon ),
                      Math::CompareEps( Y, _Other.Y, _Epsilon ),
                      Math::CompareEps( Z, _Other.Z, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = Z = 0;
    }

    TVector3 Abs() const { return TVector3( Math::Abs( X ), Math::Abs( Y ), Math::Abs( Z ) ); }

    // Vector methods
    constexpr T LengthSqr() const {
        return X * X + Y * Y + Z * Z;
    }
    T Length() const {
        return StdSqrt( LengthSqr() );
    }
    constexpr T DistSqr( TVector3 const & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }
    T Dist( TVector3 const & _Other ) const {
        return ( *this - _Other ).Length();
    }
    T NormalizeSelf() {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
        }
        return L;
    }
    TVector3 Normalized() const {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            return TVector3( X * InvLength, Y * InvLength, Z * InvLength );
        }
        return *this;
    }
    TVector3 NormalizeFix() const {
        TVector3 Normal = Normalized();
        Normal.FixNormal();
        return Normal;
    }
    // Return true if normal was fixed
    bool FixNormal() {
        const T ZERO = 0;
        const T ONE = 1;
        const T MINUS_ONE = -1;

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

        if ( Math::Abs( X ) == ONE ) {
            if ( Y != ZERO || Z != ZERO ) {
                Y = Z = ZERO;
                return true;
            }
            return false;
        }

        if ( Math::Abs( Y ) == ONE ) {
            if ( X != ZERO || Z != ZERO ) {
                X = Z = ZERO;
                return true;
            }
            return false;
        }

        if ( Math::Abs( Z ) == ONE ) {
            if ( X != ZERO || Y != ZERO ) {
                X = Y = ZERO;
                return true;
            }
            return false;
        }

        return false;
    }

    TVector3 Floor() const {
        return TVector3( Math::Floor( X ), Math::Floor( Y ), Math::Floor( Z ) );
    }

    TVector3 Ceil() const {
        return TVector3( Math::Ceil( X ), Math::Ceil( Y ), Math::Ceil( Z ) );
    }

    TVector3 Fract() const {
        return TVector3( Math::Fract( X ), Math::Fract( Y ), Math::Fract( Z ) );
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector3 Sign() const {
        return TVector3( Math::Sign( X ), Math::Sign( Y ), Math::Sign( Z ) );
    }

    constexpr int SignBits() const {
        return Math::SignBits( X ) | (Math::SignBits( Y )<<1) | (Math::SignBits( Z )<<2);
    }

    TVector3 Clamp( T const & _Min, T const & _Max ) const {
        return TVector3( Math::Clamp( X, _Min, _Max ), Math::Clamp( Y, _Min, _Max ), Math::Clamp( Z, _Min, _Max ) );
    }

    TVector3 Clamp( TVector3 const & _Min, TVector3 const & _Max ) const {
        return TVector3( Math::Clamp( X, _Min.X, _Max.X ), Math::Clamp( Y, _Min.Y, _Max.Y ), Math::Clamp( Z, _Min.Z, _Max.Z ) );
    }

    TVector3 Saturate() const {
        return Clamp( T( 0 ), T( 1 ) );
    }

    TVector3 Snap( T const &_SnapValue ) const {
        AN_ASSERT_( _SnapValue > 0, "Snap" );
        TVector3 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = Math::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = Math::Round( SnapVector.Y ) * _SnapValue;
        SnapVector.Z = Math::Round( SnapVector.Z ) * _SnapValue;
        return SnapVector;
    }

    TVector3 SnapNormal( T const & _Epsilon ) const {
        TVector3 Normal = *this;
        for ( int i = 0 ; i < 3 ; i++ ) {
            if ( Math::Abs( Normal[i] - T( 1 ) ) < _Epsilon ) {
                Normal = TVector3(0);
                Normal[i] = 1;
                break;
            }
            if ( Math::Abs( Normal[i] - T( -1 ) ) < _Epsilon ) {
                Normal = TVector3(0);
                Normal[i] = -1;
                break;
            }
        }

        if ( Math::Abs( Normal[0] ) < _Epsilon && Math::Abs( Normal[1] ) >= _Epsilon && Math::Abs( Normal[2] ) >= _Epsilon ) {
            Normal[0] = 0;
            Normal.NormalizeSelf();
        } else if ( Math::Abs( Normal[1] ) < _Epsilon && Math::Abs( Normal[0] ) >= _Epsilon && Math::Abs( Normal[2] ) >= _Epsilon ) {
            Normal[1] = 0;
            Normal.NormalizeSelf();
        } else if ( Math::Abs( Normal[2] ) < _Epsilon && Math::Abs( Normal[0] ) >= _Epsilon && Math::Abs( Normal[1] ) >= _Epsilon ) {
            Normal[2] = 0;
            Normal.NormalizeSelf();
        }

        return Normal;
    }

    int NormalAxialType() const {
        if ( X == T( 1 ) || X == T( -1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) || Y == T( -1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) || Z == T( -1 ) ) return Math::AxialZ;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == T( 1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) ) return Math::AxialZ;
        return Math::NonAxial;
    }

    int VectorAxialType() const {
        Bool3 Zero = Abs().LessThan( T(0.00001) );

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

    TVector3 Perpendicular() const {
        T dp = X * X + Y * Y;
        if ( !dp ) {
            return TVector3(1,0,0);
        } else {
            dp = Math::InvSqrt( dp );
            return TVector3( -Y * dp, X * dp, 0 );
        }
    }

    void ComputeBasis( TVector3 & _XVec, TVector3 & _YVec ) const {
        _YVec = Perpendicular();
        _XVec = Math::Cross( _YVec, *this );
    }

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< T >() ) const {
        return AString( "( " ) + Math::ToString( X, _Precision ) + " " + Math::ToString( Y, _Precision ) + " " + Math::ToString( Z, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Z, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {

        struct Writer {
            Writer( IBinaryStream & _Stream, const float & _Value ) {
                _Stream.WriteFloat( _Value );
            }

            Writer( IBinaryStream & _Stream, const double & _Value ) {
                _Stream.WriteDouble( _Value );
            }
        };
        Writer( _Stream, X );
        Writer( _Stream, Y );
        Writer( _Stream, Z );
    }

    void Read( IBinaryStream & _Stream ) {

        struct Reader {
            Reader( IBinaryStream & _Stream, float & _Value ) {
                _Value = _Stream.ReadFloat();
            }

            Reader( IBinaryStream & _Stream, double & _Value ) {
                _Value = _Stream.ReadDouble();
            }
        };
        Reader( _Stream, X );
        Reader( _Stream, Y );
        Reader( _Stream, Z );
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
    static TVector3 const & Zero() {
        static constexpr TVector3 ZeroVec( T( 0 ) );
        return ZeroVec;
    }
};

template< typename T >
struct TVector4 {
    T X;
    T Y;
    T Z;
    T W;

    TVector4() = default;

    constexpr explicit TVector4( T const & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}

    constexpr TVector4( T const & _X, T const & _Y, T const & _Z, T const & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    template< typename T2 >
    constexpr explicit TVector4< T >( TVector2< T2 > const & _Value, T2 const & _Z = T2( 0 ), T2 const & _W = T2( 0 ) )
        : X( _Value.X ), Y( _Value.Y ), Z( _Z ), W( _W ) {}

    template< typename T2 >
    constexpr explicit TVector4< T >( TVector3< T2 > const & _Value, T const & _W = T( 0 ) )
        : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ), W( _W ) {}

    template< typename T2 >
    constexpr explicit TVector4< T >( TVector4< T2 > const & _Value )
        : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}

    T * ToPtr() {
        return &X;
    }

    T const * ToPtr() const {
        return &X;
    }

//    TVector4 & operator=( TVector4 const & _Other ) {
//        X = _Other.X;
//        Y = _Other.Y;
//        Z = _Other.Z;
//        W = _Other.W;
//        return *this;
//    }

    T & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    T const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    template< int Index >
    constexpr T const & Get() const {
        static_assert( Index >= 0 && Index < NumComponents(), "Index out of range" );
        return (&X)[ Index ];
    }

    template< int _Shuffle >
    constexpr TVector2< T > Shuffle2() const {
        return TVector2< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>() );
    }

    template< int _Shuffle >
    constexpr TVector3< T > Shuffle3() const {
        return TVector3< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>() );
    }

    template< int _Shuffle >
    constexpr TVector4< T > Shuffle4() const {
        return TVector4< T >( Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>(), Get<_Shuffle & 3>() );
    }

    constexpr bool operator==( TVector4 const & _Other ) const { return Compare( _Other ); }
    constexpr bool operator!=( TVector4 const & _Other ) const { return !Compare( _Other ); }

    // Math operators
    constexpr TVector4 operator+() const {
        return *this;
    }
    constexpr TVector4 operator-() const {
        return TVector4( -X, -Y, -Z, -W );
    }
    constexpr TVector4 operator+( TVector4 const & _Other ) const {
        return TVector4( X + _Other.X, Y + _Other.Y, Z + _Other.Z, W + _Other.W );
    }
    constexpr TVector4 operator-( TVector4 const & _Other ) const {
        return TVector4( X - _Other.X, Y - _Other.Y, Z - _Other.Z, W - _Other.W );
    }
    constexpr TVector4 operator/( TVector4 const & _Other ) const {
        return TVector4( X / _Other.X, Y / _Other.Y, Z / _Other.Z, W / _Other.W );
    }
    constexpr TVector4 operator*( TVector4 const & _Other ) const {
        return TVector4( X * _Other.X, Y * _Other.Y, Z * _Other.Z, W * _Other.W );
    }
    constexpr TVector4 operator+( T const & _Other ) const {
        return TVector4( X + _Other, Y + _Other, Z + _Other, W + _Other );
    }
    constexpr TVector4 operator-( T const & _Other ) const {
        return TVector4( X - _Other, Y - _Other, Z - _Other, W - _Other );
    }
    TVector4 operator/( T const & _Other ) const {
        T Denom = T( 1 ) / _Other;
        return TVector4( X * Denom, Y * Denom, Z * Denom, W * Denom );
    }
    constexpr TVector4 operator*( T const & _Other ) const {
        return TVector4( X * _Other, Y * _Other, Z * _Other, W * _Other );
    }
    void operator+=( TVector4 const & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
        W += _Other.W;
    }
    void operator-=( TVector4 const & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
        W -= _Other.W;
    }
    void operator/=( TVector4 const & _Other ) {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
        W /= _Other.W;
    }
    void operator*=( TVector4 const & _Other ) {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
        W *= _Other.W;
    }
    void operator+=( T const & _Other ) {
        X += _Other;
        Y += _Other;
        Z += _Other;
        W += _Other;
    }
    void operator-=( T const & _Other ) {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
        W -= _Other;
    }
    void operator/=( T const & _Other ) {
        T Denom = T( 1 ) / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
        W *= Denom;
    }
    void operator*=( T const & _Other ) {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
        W *= _Other;
    }

    T Min() const {
        return Math::Min( Math::Min( Math::Min( X, Y ), Z ), W );
    }

    T Max() const {
        return Math::Max( Math::Max( Math::Max( X, Y ), Z ), W );
    }

    int MinorAxis() const {
        T Minor = Math::Abs( X );
        int Axis = 0;
        T Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp <= Minor ) {
            Axis = 1;
            Minor = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp <= Minor ) {
            Axis = 2;
            Minor = Tmp;
        }

        Tmp = Math::Abs( W );
        if ( Tmp <= Minor ) {
            Axis = 3;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const {
        T Major = Math::Abs( X );
        int Axis = 0;
        T Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp > Major ) {
            Axis = 1;
            Major = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp > Major ) {
            Axis = 2;
            Major = Tmp;
        }

        Tmp = Math::Abs( W );
        if ( Tmp > Major ) {
            Axis = 3;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    constexpr Bool4 IsInfinite() const {
        return Bool4( Math::IsInfinite( X ), Math::IsInfinite( Y ), Math::IsInfinite( Z ), Math::IsInfinite( W ) );
    }

    constexpr Bool4 IsNan() const {
        return Bool4( Math::IsNan( X ), Math::IsNan( Y ), Math::IsNan( Z ), Math::IsNan( W ) );
    }

    constexpr Bool4 IsNormal() const {
        return Bool4( Math::IsNormal( X ), Math::IsNormal( Y ), Math::IsNormal( Z ), Math::IsNormal( W ) );
    }

    constexpr Bool4 IsDenormal() const {
        return Bool4( Math::IsDenormal( X ), Math::IsDenormal( Y ), Math::IsDenormal( Z ), Math::IsDenormal( W ) );
    }

    // Comparision
    constexpr Bool4 LessThan( TVector4 const & _Other ) const {
        return Bool4( Math::LessThan( X, _Other.X ), Math::LessThan( Y, _Other.Y ), Math::LessThan( Z, _Other.Z ), Math::LessThan( W, _Other.W ) );
    }
    constexpr Bool4 LessThan( T const & _Other ) const {
        return Bool4( Math::LessThan( X, _Other ), Math::LessThan( Y, _Other ), Math::LessThan( Z, _Other ), Math::LessThan( W, _Other ) );
    }

    constexpr Bool4 LequalThan( TVector4 const & _Other ) const {
        return Bool4( Math::LequalThan( X, _Other.X ), Math::LequalThan( Y, _Other.Y ), Math::LequalThan( Z, _Other.Z ), Math::LequalThan( W, _Other.W ) );
    }
    constexpr Bool4 LequalThan( T const & _Other ) const {
        return Bool4( Math::LequalThan( X, _Other ), Math::LequalThan( Y, _Other ), Math::LequalThan( Z, _Other ), Math::LequalThan( W, _Other ) );
    }

    constexpr Bool4 GreaterThan( TVector4 const & _Other ) const {
        return Bool4( Math::GreaterThan( X, _Other.X ), Math::GreaterThan( Y, _Other.Y ), Math::GreaterThan( Z, _Other.Z ), Math::GreaterThan( W, _Other.W ) );
    }
    constexpr Bool4 GreaterThan( T const & _Other ) const {
        return Bool4( Math::GreaterThan( X, _Other ), Math::GreaterThan( Y, _Other ), Math::GreaterThan( Z, _Other ), Math::GreaterThan( W, _Other ) );
    }

    constexpr Bool4 GequalThan( TVector4 const & _Other ) const {
        return Bool4( Math::GequalThan( X, _Other.X ), Math::GequalThan( Y, _Other.Y ), Math::GequalThan( Z, _Other.Z ), Math::GequalThan( W, _Other.W ) );
    }
    constexpr Bool4 GequalThan( T const & _Other ) const {
        return Bool4( Math::GequalThan( X, _Other ), Math::GequalThan( Y, _Other ), Math::GequalThan( Z, _Other ), Math::GequalThan( W, _Other ) );
    }

    constexpr Bool4 NotEqual( TVector4 const & _Other ) const {
        return Bool4( Math::NotEqual( X, _Other.X ), Math::NotEqual( Y, _Other.Y ), Math::NotEqual( Z, _Other.Z ), Math::NotEqual( W, _Other.W ) );
    }
    constexpr Bool4 NotEqual( T const & _Other ) const {
        return Bool4( Math::NotEqual( X, _Other ), Math::NotEqual( Y, _Other ), Math::NotEqual( Z, _Other ), Math::NotEqual( W, _Other ) );
    }

    constexpr bool Compare( TVector4 const & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    constexpr bool CompareEps( TVector4 const & _Other, T const & _Epsilon ) const {
        return Bool4( Math::CompareEps( X, _Other.X, _Epsilon ),
                      Math::CompareEps( Y, _Other.Y, _Epsilon ),
                      Math::CompareEps( Z, _Other.Z, _Epsilon ),
                      Math::CompareEps( W, _Other.W, _Epsilon ) ).All();
    }

    void Clear() {
        X = Y = Z = W = 0;
    }

    TVector4 Abs() const { return TVector4( Math::Abs( X ), Math::Abs( Y ), Math::Abs( Z ), Math::Abs( W ) ); }

    // Vector methods
    constexpr T LengthSqr() const {
        return X * X + Y * Y + Z * Z + W * W;
    }
    T Length() const {
        return StdSqrt( LengthSqr() );
    }
    constexpr T DistSqr( TVector4 const & _Other ) const {
        return ( *this - _Other ).LengthSqr();
    }
    T Dist( TVector4 const & _Other ) const {
        return ( *this - _Other ).Length();
    }
    T NormalizeSelf() {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
            W *= InvLength;
        }
        return L;
    }
    TVector4 Normalized() const {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            return TVector4( X * InvLength, Y * InvLength, Z * InvLength, W * InvLength );
        }
        return *this;
    }

    TVector4 Floor() const {
        return TVector4( Math::Floor( X ), Math::Floor( Y ), Math::Floor( Z ), Math::Floor( W ) );
    }

    TVector4 Ceil() const {
        return TVector4( Math::Ceil( X ), Math::Ceil( Y ), Math::Ceil( Z ), Math::Ceil( W ) );
    }

    TVector4 Fract() const {
        return TVector4( Math::Fract( X ), Math::Fract( Y ), Math::Fract( Z ), Math::Fract( W ) );
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector4 Sign() const {
        return TVector4( Math::Sign( X ), Math::Sign( Y ), Math::Sign( Z ), Math::Sign( W ) );
    }

    constexpr int SignBits() const {
        return Math::SignBits( X ) | (Math::SignBits( Y )<<1) | (Math::SignBits( Z )<<2) | (Math::SignBits( W )<<3);
    }

    TVector4 Clamp( T const & _Min, T const & _Max ) const {
        return TVector4( Math::Clamp( X, _Min, _Max ), Math::Clamp( Y, _Min, _Max ), Math::Clamp( Z, _Min, _Max ), Math::Clamp( W, _Min, _Max ) );
    }

    TVector4 Clamp( TVector4 const & _Min, TVector4 const & _Max ) const {
        return TVector4( Math::Clamp( X, _Min.X, _Max.X ), Math::Clamp( Y, _Min.Y, _Max.Y ), Math::Clamp( Z, _Min.Z, _Max.Z ), Math::Clamp( W, _Min.W, _Max.W ) );
    }

    TVector4 Saturate() const {
        return Clamp( T( 0 ), T( 1 ) );
    }

    TVector4 Snap( T const &_SnapValue ) const {
        AN_ASSERT_( _SnapValue > 0, "Snap" );
        TVector4 SnapVector;
        SnapVector = *this / _SnapValue;
        SnapVector.X = Math::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = Math::Round( SnapVector.Y ) * _SnapValue;
        SnapVector.Z = Math::Round( SnapVector.Z ) * _SnapValue;
        SnapVector.W = Math::Round( SnapVector.W ) * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const {
        if ( X == T( 1 ) || X == T( -1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) || Y == T( -1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) || Z == T( -1 ) ) return Math::AxialZ;
        if ( W == T( 1 ) || W == T( -1 ) ) return Math::AxialW;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const {
        if ( X == T( 1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) ) return Math::AxialZ;
        if ( W == T( 1 ) ) return Math::AxialW;
        return Math::NonAxial;
    }

    int VectorAxialType() const {
        Bool4 Zero = Abs().LessThan( T(0.00001) );

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

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< T >() ) const {
        return AString( "( " ) + Math::ToString( X, _Precision ) + " " + Math::ToString( Y, _Precision ) + " " + Math::ToString( Z, _Precision ) + " " + Math::ToString( W, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Z, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( W, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {

        struct Writer {
            Writer( IBinaryStream & _Stream, const float & _Value ) {
                _Stream.WriteFloat( _Value );
            }

            Writer( IBinaryStream & _Stream, const double & _Value ) {
                _Stream.WriteDouble( _Value );
            }
        };
        Writer( _Stream, X );
        Writer( _Stream, Y );
        Writer( _Stream, Z );
        Writer( _Stream, W );
    }

    void Read( IBinaryStream & _Stream ) {

        struct Reader {
            Reader( IBinaryStream & _Stream, float & _Value ) {
                _Value = _Stream.ReadFloat();
            }

            Reader( IBinaryStream & _Stream, double & _Value ) {
                _Value = _Stream.ReadDouble();
            }
        };
        Reader( _Stream, X );
        Reader( _Stream, Y );
        Reader( _Stream, Z );
        Reader( _Stream, W );
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
    static TVector4 const & Zero() {
        static constexpr TVector4 ZeroVec( T( 0 ) );
        return ZeroVec;
    }
};

using Float2 = TVector2< float >;
using Float3 = TVector3< float >;
using Float4 = TVector4< float >;
using Double2 = TVector2< double >;
using Double3 = TVector3< double >;
using Double4 = TVector4< double >;
