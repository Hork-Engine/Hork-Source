/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

template< typename T >
struct TPlane;

namespace Math
{

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr float Dot( TVector2< T > const & A, TVector2< T > const & B )
{
    return A.X * B.X + A.Y * B.Y;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr float Dot( TVector3< T > const & A, TVector3< T > const & B )
{
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr float Dot( TVector4< T > const & A, TVector4< T > const & B )
{
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr float Dot( TPlane< T > const & A, TVector3< T > const & B )
{
    return A.Normal.X * B.X + A.Normal.Y * B.Y + A.Normal.Z * B.Z + A.D;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr float Dot( TVector3< T > const & A, TPlane< T > const & B )
{
    return A.X * B.Normal.X + A.Y * B.Normal.Y + A.Z * B.Normal.Z + B.D;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr float Dot( TPlane< T > const & A, TVector4< T > const & B )
{
    return A.Normal.X * B.X + A.Normal.Y * B.Y + A.Normal.Z * B.Z + A.D * B.W;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr float Dot( TVector4< T > const & A, TPlane< T > const & B )
{
    return A.X * B.Normal.X + A.Y * B.Normal.Y + A.Z * B.Normal.Z + A.W * B.D;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr T Cross( TVector2< T > const & A, TVector2< T > const & B )
{
    return A.X * B.Y - A.Y * B.X;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr TVector3< T > Cross( TVector3< T > const & A, TVector3< T > const & B )
{
    return TVector3< T >( A.Y * B.Z - B.Y * A.Z,
                          A.Z * B.X - B.Z * A.X,
                          A.X * B.Y - B.X * A.Y );
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr TVector2< T > Reflect( TVector2< T > const & _IncidentVector, TVector2< T > const & _Normal )
{
    return _IncidentVector - 2 * Dot( _Normal, _IncidentVector ) * _Normal;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
constexpr TVector3< T > Reflect( TVector3< T > const & _IncidentVector, TVector3< T > const & _Normal )
{
    return _IncidentVector - 2 * Dot( _Normal, _IncidentVector ) * _Normal;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
AN_FORCEINLINE TVector2< T > Refract( TVector2< T > const & _IncidentVector, TVector2< T > const & _Normal, T const _Eta )
{
    const T NdotI = Dot( _Normal, _IncidentVector );
    const T k     = T( 1 ) - _Eta * _Eta * ( T( 1 ) - NdotI * NdotI );
    if ( k < T( 0 ) ) {
        return TVector2< T >( T( 0 ) );
    }
    return _IncidentVector * _Eta - _Normal * ( _Eta * NdotI + sqrt( k ) );
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
AN_FORCEINLINE TVector3< T > Refract( TVector3< T > const & _IncidentVector, TVector3< T > const & _Normal, const T _Eta )
{
    const T NdotI = Dot( _Normal, _IncidentVector );
    const T k     = T( 1 ) - _Eta * _Eta * ( T( 1 ) - NdotI * NdotI );
    if ( k < T( 0 ) ) {
        return TVector3< T >( T( 0 ) );
    }
    return _IncidentVector * _Eta - _Normal * ( _Eta * NdotI + sqrt( k ) );
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
TVector3< T > ProjectVector( TVector3< T > const & _Vector, TVector3< T > const & _Normal, T _Overbounce )
{
    return _Vector - _Normal * Dot( _Vector, _Normal ) * _Overbounce;
}

template< typename T, typename = TStdEnableIf< IsReal< T >() > >
TVector3< T > ProjectVector( TVector3< T > const & _Vector, TVector3< T > const & _Normal )
{
    return _Vector - _Normal * Dot( _Vector, _Normal );
}

template< typename T >
constexpr TVector2< T > Lerp( TVector2< T > const & _From, TVector2< T > const & _To, T const & _Mix )
{
    return _From + _Mix * ( _To - _From );
}

template< typename T >
constexpr TVector3< T > Lerp( TVector3< T > const & _From, TVector3< T > const & _To, T const & _Mix )
{
    return _From + _Mix * ( _To - _From );
}

template< typename T >
constexpr TVector4< T > Lerp( TVector4< T > const & _From, TVector4< T > const & _To, T const & _Mix )
{
    return _From + _Mix * ( _To - _From );
}

template< typename T >
constexpr T Bilerp( T const & _A, T const & _B, T const & _C, T const & _D, TVector2< T > const & _Lerp )
{
    return _A * ( T( 1 ) - _Lerp.X ) * ( T( 1 ) - _Lerp.Y ) + _B * _Lerp.X * ( T( 1 ) - _Lerp.Y ) + _C * ( T( 1 ) - _Lerp.X ) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector2< T > Bilerp( TVector2< T > const & _A, TVector2< T > const & _B, TVector2< T > const & _C, TVector2< T > const & _D, TVector2< T > const & _Lerp )
{
    return _A * ( T( 1 ) - _Lerp.X ) * ( T( 1 ) - _Lerp.Y ) + _B * _Lerp.X * ( T( 1 ) - _Lerp.Y ) + _C * ( T( 1 ) - _Lerp.X ) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector3< T > Bilerp( TVector3< T > const & _A, TVector3< T > const & _B, TVector3< T > const & _C, TVector3< T > const & _D, TVector2< T > const & _Lerp )
{
    return _A * ( T( 1 ) - _Lerp.X ) * ( T( 1 ) - _Lerp.Y ) + _B * _Lerp.X * ( T( 1 ) - _Lerp.Y ) + _C * ( T( 1 ) - _Lerp.X ) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector4< T > Bilerp( TVector4< T > const & _A, TVector4< T > const & _B, TVector4< T > const & _C, TVector4< T > const & _D, TVector2< T > const & _Lerp )
{
    return _A * ( T( 1 ) - _Lerp.X ) * ( T( 1 ) - _Lerp.Y ) + _B * _Lerp.X * ( T( 1 ) - _Lerp.Y ) + _C * ( T( 1 ) - _Lerp.X ) * _Lerp.Y + _D * _Lerp.X * _Lerp.Y;
}

template< typename T >
constexpr TVector2< T > Step( TVector2< T > const & _Vec, T const & _Edge )
{
    return TVector2< T >( _Vec.X < _Edge ? T( 0 ) : T( 1 ), _Vec.Y < _Edge ? T( 0 ) : T( 1 ) );
}

template< typename T >
constexpr TVector2< T > Step( TVector2< T > const & _Vec, TVector2< T > const & _Edge )
{
    return TVector2< T >( _Vec.X < _Edge.X ? T( 0 ) : T( 1 ), _Vec.Y < _Edge.Y ? T( 0 ) : T( 1 ) );
}

template< typename T >
AN_FORCEINLINE TVector2< T > SmoothStep( TVector2< T > const & _Vec, T const & _Edge0, T const & _Edge1 )
{
    const T             Denom = T( 1 ) / ( _Edge1 - _Edge0 );
    const TVector2< T > t     = Saturate( ( _Vec - _Edge0 ) * Denom );
    return t * t * ( T( -2 ) * t + T( 3 ) );
}

template< typename T >
AN_FORCEINLINE TVector2< T > SmoothStep( TVector2< T > const & _Vec, TVector2< T > const & _Edge0, TVector2< T > const & _Edge1 )
{
    const TVector2< T > t = Saturate( ( _Vec - _Edge0 ) / ( _Edge1 - _Edge0 ) );
    return t * t * ( T( -2 ) * t + T( 3 ) );
}

template< typename T >
constexpr TVector3< T > Step( TVector3< T > const & _Vec, T const & _Edge )
{
    return TVector3< T >( _Vec.X < _Edge ? T( 0 ) : T( 1 ), _Vec.Y < _Edge ? T( 0 ) : T( 1 ), _Vec.Z < _Edge ? T( 0 ) : T( 1 ) );
}

template< typename T >
constexpr TVector3< T > Step( TVector3< T > const & _Vec, TVector2< T > const & _Edge )
{
    return TVector3< T >( _Vec.X < _Edge.X ? T( 0 ) : T( 1 ), _Vec.Y < _Edge.Y ? T( 0 ) : T( 1 ), _Vec.Z < _Edge.Z ? T( 0 ) : T( 1 ) );
}

template< typename T >
AN_FORCEINLINE TVector3< T > SmoothStep( TVector3< T > const & _Vec, T const & _Edge0, T const & _Edge1 )
{
    const T             Denom = T( 1 ) / ( _Edge1 - _Edge0 );
    const TVector3< T > t     = Saturate( ( _Vec - _Edge0 ) * Denom );
    return t * t * ( T( -2 ) * t + T( 3 ) );
}

template< typename T >
AN_FORCEINLINE TVector3< T > SmoothStep( TVector3< T > const & _Vec, TVector2< T > const & _Edge0, TVector2< T > const & _Edge1 )
{
    const TVector3< T > t = Saturate( ( _Vec - _Edge0 ) / ( _Edge1 - _Edge0 ) );
    return t * t * ( T( -2 ) * t + T( 3 ) );
}

template< typename T >
constexpr TVector4< T > Step( TVector4< T > const & _Vec, T const & _Edge )
{
    return TVector4< T >( _Vec.X < _Edge ? T( 0 ) : T( 1 ), _Vec.Y < _Edge ? T( 0 ) : T( 1 ), _Vec.Z < _Edge ? T( 0 ) : T( 1 ), _Vec.W < _Edge ? T( 0 ) : T( 1 ) );
}

template< typename T >
constexpr TVector4< T > Step( TVector4< T > const & _Vec, TVector4< T > const & _Edge )
{
    return TVector4< T >( _Vec.X < _Edge.X ? T( 0 ) : T( 1 ), _Vec.Y < _Edge.Y ? T( 0 ) : T( 1 ), _Vec.Z < _Edge.Z ? T( 0 ) : T( 1 ), _Vec.W < _Edge.W ? T( 0 ) : T( 1 ) );
}

template< typename T >
AN_FORCEINLINE TVector4< T > SmoothStep( TVector4< T > const & _Vec, T const & _Edge0, T const & _Edge1 )
{
    const T             Denom = T( 1 ) / ( _Edge1 - _Edge0 );
    const TVector4< T > t     = Saturate( ( _Vec - _Edge0 ) * Denom );
    return t * t * ( T( -2 ) * t + T( 3 ) );
}

template< typename T >
AN_FORCEINLINE TVector4< T > SmoothStep( TVector4< T > const & _Vec, TVector4< T > const & _Edge0, TVector4< T > const & _Edge1 )
{
    const TVector4< T > t = Saturate( ( _Vec - _Edge0 ) / ( _Edge1 - _Edge0 ) );
    return t * t * ( T( -2 ) * t + T( 3 ) );
}

} // namespace Math

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector2< T > operator+( T const & _Left, TVector2< T > const & _Right )
{
    return TVector2< T >( _Left + _Right.X, _Left + _Right.Y );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector2< T > operator-( T const & _Left, TVector2< T > const & _Right )
{
    return TVector2< T >( _Left - _Right.X, _Left - _Right.Y );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector3< T > operator+( T const & _Left, TVector3< T > const & _Right )
{
    return TVector3< T >( _Left + _Right.X, _Left + _Right.Y, _Left + _Right.Z );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector3< T > operator-( T const & _Left, TVector3< T > const & _Right )
{
    return TVector3< T >( _Left - _Right.X, _Left - _Right.Y, _Left - _Right.Z );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector4< T > operator+( T const & _Left, TVector4< T > const & _Right )
{
    return TVector4< T >( _Left + _Right.X, _Left + _Right.Y, _Left + _Right.Z, _Left + _Right.W );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector4< T > operator-( T const & _Left, TVector4< T > const & _Right )
{
    return TVector4< T >( _Left - _Right.X, _Left - _Right.Y, _Left - _Right.Z, _Left - _Right.W );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector2< T > operator*( T const & _Left, TVector2< T > const & _Right )
{
    return TVector2< T >( _Left * _Right.X, _Left * _Right.Y );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector3< T > operator*( T const & _Left, TVector3< T > const & _Right )
{
    return TVector3< T >( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z );
}

template< typename T, typename = TStdEnableIf< Math::IsReal< T >() > >
constexpr TVector4< T > operator*( T const & _Left, TVector4< T > const & _Right )
{
    return TVector4< T >( _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z, _Left * _Right.W );
}

template< typename T >
struct TVector2
{
    T X;
    T Y;

    TVector2() = default;

    constexpr explicit TVector2( T const & _Value ) :
        X( _Value ), Y( _Value ) {}

    constexpr TVector2( T const & _X, T const & _Y ) :
        X( _X ), Y( _Y ) {}

    constexpr explicit TVector2( TVector3< T > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ) {}
    constexpr explicit TVector2( TVector4< T > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ) {}

    template< typename T2 >
    constexpr explicit TVector2< T >( TVector2< T2 > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ) {}

    template< typename T2 >
    constexpr explicit TVector2< T >( TVector3< T2 > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ) {}

    template< typename T2 >
    constexpr explicit TVector2< T >( TVector4< T2 > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ) {}

    T * ToPtr()
    {
        return &X;
    }

    T const * ToPtr() const
    {
        return &X;
    }

    T & operator[]( int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return ( &X )[_Index];
    }

    T const & operator[]( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return ( &X )[_Index];
    }

    template< int Index >
    constexpr T const & Get() const
    {
        static_assert( Index >= 0 && Index < NumComponents(), "Index out of range" );
        return ( &X )[Index];
    }

    template< int _Shuffle >
    constexpr TVector2< T > Shuffle2() const
    {
        return TVector2< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >() );
    }

    template< int _Shuffle >
    constexpr TVector3< T > Shuffle3() const
    {
        return TVector3< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >(), Get< ( _Shuffle >> 2 ) & 3 >() );
    }

    template< int _Shuffle >
    constexpr TVector4< T > Shuffle4() const
    {
        return TVector4< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >(), Get< ( _Shuffle >> 2 ) & 3 >(), Get< _Shuffle & 3 >() );
    }

    constexpr bool operator==( TVector2 const & _Other ) const { return Compare( _Other ); }
    constexpr bool operator!=( TVector2 const & _Other ) const { return !Compare( _Other ); }

    // Math operators
    constexpr TVector2 operator+() const
    {
        return *this;
    }
    constexpr TVector2 operator-() const
    {
        return TVector2( -X, -Y );
    }
    constexpr TVector2 operator+( TVector2 const & _Other ) const
    {
        return TVector2( X + _Other.X, Y + _Other.Y );
    }
    constexpr TVector2 operator-( TVector2 const & _Other ) const
    {
        return TVector2( X - _Other.X, Y - _Other.Y );
    }
    constexpr TVector2 operator/( TVector2 const & _Other ) const
    {
        return TVector2( X / _Other.X, Y / _Other.Y );
    }
    constexpr TVector2 operator*( TVector2 const & _Other ) const
    {
        return TVector2( X * _Other.X, Y * _Other.Y );
    }
    constexpr TVector2 operator+( T const & _Other ) const
    {
        return TVector2( X + _Other, Y + _Other );
    }
    constexpr TVector2 operator-( T const & _Other ) const
    {
        return TVector2( X - _Other, Y - _Other );
    }
    TVector2 operator/( T const & _Other ) const
    {
        T Denom = T( 1 ) / _Other;
        return TVector2( X * Denom, Y * Denom );
    }
    constexpr TVector2 operator*( T const & _Other ) const
    {
        return TVector2( X * _Other, Y * _Other );
    }
    void operator+=( TVector2 const & _Other )
    {
        X += _Other.X;
        Y += _Other.Y;
    }
    void operator-=( TVector2 const & _Other )
    {
        X -= _Other.X;
        Y -= _Other.Y;
    }
    void operator/=( TVector2 const & _Other )
    {
        X /= _Other.X;
        Y /= _Other.Y;
    }
    void operator*=( TVector2 const & _Other )
    {
        X *= _Other.X;
        Y *= _Other.Y;
    }
    void operator+=( T const & _Other )
    {
        X += _Other;
        Y += _Other;
    }
    void operator-=( T const & _Other )
    {
        X -= _Other;
        Y -= _Other;
    }
    void operator/=( T const & _Other )
    {
        T Denom = T( 1 ) / _Other;
        X *= Denom;
        Y *= Denom;
    }
    void operator*=( T const & _Other )
    {
        X *= _Other;
        Y *= _Other;
    }

    T Min() const
    {
        return Math::Min( X, Y );
    }

    T Max() const
    {
        return Math::Max( X, Y );
    }

    int MinorAxis() const
    {
        return int( Math::Abs( X ) >= Math::Abs( Y ) );
    }

    int MajorAxis() const
    {
        return int( Math::Abs( X ) < Math::Abs( Y ) );
    }

    // Floating point specific
    constexpr Bool2 IsInfinite() const
    {
        return Bool2( Math::IsInfinite( X ), Math::IsInfinite( Y ) );
    }

    constexpr Bool2 IsNan() const
    {
        return Bool2( Math::IsNan( X ), Math::IsNan( Y ) );
    }

    constexpr Bool2 IsNormal() const
    {
        return Bool2( Math::IsNormal( X ), Math::IsNormal( Y ) );
    }

    constexpr Bool2 IsDenormal() const
    {
        return Bool2( Math::IsDenormal( X ), Math::IsDenormal( Y ) );
    }

    // Comparision

    constexpr Bool2 LessThan( TVector2 const & _Other ) const
    {
        return Bool2( Math::LessThan( X, _Other.X ), Math::LessThan( Y, _Other.Y ) );
    }
    constexpr Bool2 LessThan( T const & _Other ) const
    {
        return Bool2( Math::LessThan( X, _Other ), Math::LessThan( Y, _Other ) );
    }

    constexpr Bool2 LequalThan( TVector2 const & _Other ) const
    {
        return Bool2( Math::LequalThan( X, _Other.X ), Math::LequalThan( Y, _Other.Y ) );
    }
    constexpr Bool2 LequalThan( T const & _Other ) const
    {
        return Bool2( Math::LequalThan( X, _Other ), Math::LequalThan( Y, _Other ) );
    }

    constexpr Bool2 GreaterThan( TVector2 const & _Other ) const
    {
        return Bool2( Math::GreaterThan( X, _Other.X ), Math::GreaterThan( Y, _Other.Y ) );
    }
    constexpr Bool2 GreaterThan( T const & _Other ) const
    {
        return Bool2( Math::GreaterThan( X, _Other ), Math::GreaterThan( Y, _Other ) );
    }

    constexpr Bool2 GequalThan( TVector2 const & _Other ) const
    {
        return Bool2( Math::GequalThan( X, _Other.X ), Math::GequalThan( Y, _Other.Y ) );
    }
    constexpr Bool2 GequalThan( T const & _Other ) const
    {
        return Bool2( Math::GequalThan( X, _Other ), Math::GequalThan( Y, _Other ) );
    }

    constexpr Bool2 NotEqual( TVector2 const & _Other ) const
    {
        return Bool2( Math::NotEqual( X, _Other.X ), Math::NotEqual( Y, _Other.Y ) );
    }
    constexpr Bool2 NotEqual( T const & _Other ) const
    {
        return Bool2( Math::NotEqual( X, _Other ), Math::NotEqual( Y, _Other ) );
    }

    constexpr bool Compare( TVector2 const & _Other ) const
    {
        return !NotEqual( _Other ).Any();
    }

    constexpr bool CompareEps( TVector2 const & _Other, T const & _Epsilon ) const
    {
        return Bool2( Math::CompareEps( X, _Other.X, _Epsilon ),
                      Math::CompareEps( Y, _Other.Y, _Epsilon ) )
            .All();
    }

    void Clear()
    {
        X = Y = 0;
    }

    TVector2 Abs() const { return TVector2( Math::Abs( X ), Math::Abs( Y ) ); }

    // Vector methods
    constexpr T LengthSqr() const
    {
        return X * X + Y * Y;
    }

    T Length() const
    {
        return StdSqrt( LengthSqr() );
    }

    constexpr T DistSqr( TVector2 const & _Other ) const
    {
        return ( *this - _Other ).LengthSqr();
    }

    T Dist( TVector2 const & _Other ) const
    {
        return ( *this - _Other ).Length();
    }

    T NormalizeSelf()
    {
        T L = Length();
        if ( L != T( 0 ) ) {
            T InvLength = T( 1 ) / L;
            X *= InvLength;
            Y *= InvLength;
        }
        return L;
    }

    TVector2 Normalized() const
    {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            return TVector2( X * InvLength, Y * InvLength );
        }
        return *this;
    }

    TVector2 Floor() const
    {
        return TVector2( Math::Floor( X ), Math::Floor( Y ) );
    }

    TVector2 Ceil() const
    {
        return TVector2( Math::Ceil( X ), Math::Ceil( Y ) );
    }

    TVector2 Fract() const
    {
        return TVector2( Math::Fract( X ), Math::Fract( Y ) );
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector2 Sign() const
    {
        return TVector2( Math::Sign( X ), Math::Sign( Y ) );
    }

    constexpr int SignBits() const
    {
        return Math::SignBits( X ) | ( Math::SignBits( Y ) << 1 );
    }

    //TVector2 Clamp( T const & _Min, T const & _Max ) const {
    //    return TVector2( Math::Clamp( X, _Min, _Max ), Math::Clamp( Y, _Min, _Max ) );
    //}

    //TVector2 Clamp( TVector2 const & _Min, TVector2 const & _Max ) const {
    //    return TVector2( Math::Clamp( X, _Min.X, _Max.X ), Math::Clamp( Y, _Min.Y, _Max.Y ) );
    //}

    //TVector2 Saturate() const {
    //    return Clamp( T( 0 ), T( 1 ) );
    //}

    TVector2 Snap( T const & _SnapValue ) const
    {
        AN_ASSERT_( _SnapValue > 0, "Snap" );
        TVector2 SnapVector;
        SnapVector   = *this / _SnapValue;
        SnapVector.X = Math::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = Math::Round( SnapVector.Y ) * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const
    {
        if ( X == T( 1 ) || X == T( -1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) || Y == T( -1 ) ) return Math::AxialY;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const
    {
        if ( X == T( 1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) ) return Math::AxialY;
        return Math::NonAxial;
    }

    int VectorAxialType() const
    {
        if ( Math::Abs( X ) < T( 0.00001 ) ) {
            return ( Math::Abs( Y ) < T( 0.00001 ) ) ? Math::NonAxial : Math::AxialY;
        }
        return ( Math::Abs( Y ) < T( 0.00001 ) ) ? Math::AxialX : Math::NonAxial;
    }

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< T >() ) const
    {
        return AString( "( " ) + Math::ToString( X, _Precision ) + " " + Math::ToString( Y, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {

        struct Writer
        {
            Writer( IBinaryStream & _Stream, const float & _Value )
            {
                _Stream.WriteFloat( _Value );
            }

            Writer( IBinaryStream & _Stream, const double & _Value )
            {
                _Stream.WriteDouble( _Value );
            }
        };
        Writer( _Stream, X );
        Writer( _Stream, Y );
    }

    void Read( IBinaryStream & _Stream )
    {

        struct Reader
        {
            Reader( IBinaryStream & _Stream, float & _Value )
            {
                _Value = _Stream.ReadFloat();
            }

            Reader( IBinaryStream & _Stream, double & _Value )
            {
                _Value = _Stream.ReadDouble();
            }
        };
        Reader( _Stream, X );
        Reader( _Stream, Y );
    }

    // Static methods
    static constexpr int    NumComponents() { return 2; }
    static TVector2 const & Zero()
    {
        static constexpr TVector2< T > ZeroVec( T( 0 ) );
        return ZeroVec;
    }
};

template< typename T >
struct TVector3
{
    T X;
    T Y;
    T Z;

    TVector3() = default;

    constexpr explicit TVector3( T const & _Value ) :
        X( _Value ), Y( _Value ), Z( _Value ) {}

    constexpr TVector3( T const & _X, T const & _Y, T const & _Z ) :
        X( _X ), Y( _Y ), Z( _Z ) {}

    template< typename T2 >
    constexpr explicit TVector3< T >( TVector2< T2 > const & _Value, T2 const & _Z = T2( 0 ) ) :
        X( _Value.X ), Y( _Value.Y ), Z( _Z ) {}

    template< typename T2 >
    constexpr explicit TVector3< T >( TVector3< T2 > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}

    template< typename T2 >
    constexpr explicit TVector3< T >( TVector4< T2 > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}

    T * ToPtr()
    {
        return &X;
    }

    T const * ToPtr() const
    {
        return &X;
    }

    //    TVector3 & operator=( TVector3 const & _Other ) {
    //        X = _Other.X;
    //        Y = _Other.Y;
    //        Z = _Other.Z;
    //        return *this;
    //    }

    T & operator[]( int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return ( &X )[_Index];
    }

    T const & operator[]( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return ( &X )[_Index];
    }

    template< int Index >
    constexpr T const & Get() const
    {
        static_assert( Index >= 0 && Index < NumComponents(), "Index out of range" );
        return ( &X )[Index];
    }

    template< int _Shuffle >
    constexpr TVector2< T > Shuffle2() const
    {
        return TVector2< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >() );
    }

    template< int _Shuffle >
    constexpr TVector3< T > Shuffle3() const
    {
        return TVector3< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >(), Get< ( _Shuffle >> 2 ) & 3 >() );
    }

    template< int _Shuffle >
    constexpr TVector4< T > Shuffle4() const
    {
        return TVector4< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >(), Get< ( _Shuffle >> 2 ) & 3 >(), Get< _Shuffle & 3 >() );
    }

    constexpr bool operator==( TVector3 const & _Other ) const { return Compare( _Other ); }
    constexpr bool operator!=( TVector3 const & _Other ) const { return !Compare( _Other ); }

    // Math operators
    constexpr TVector3 operator+() const
    {
        return *this;
    }
    constexpr TVector3 operator-() const
    {
        return TVector3( -X, -Y, -Z );
    }
    constexpr TVector3 operator+( TVector3 const & _Other ) const
    {
        return TVector3( X + _Other.X, Y + _Other.Y, Z + _Other.Z );
    }
    constexpr TVector3 operator-( TVector3 const & _Other ) const
    {
        return TVector3( X - _Other.X, Y - _Other.Y, Z - _Other.Z );
    }
    constexpr TVector3 operator/( TVector3 const & _Other ) const
    {
        return TVector3( X / _Other.X, Y / _Other.Y, Z / _Other.Z );
    }
    constexpr TVector3 operator*( TVector3 const & _Other ) const
    {
        return TVector3( X * _Other.X, Y * _Other.Y, Z * _Other.Z );
    }
    constexpr TVector3 operator+( T const & _Other ) const
    {
        return TVector3( X + _Other, Y + _Other, Z + _Other );
    }
    constexpr TVector3 operator-( T const & _Other ) const
    {
        return TVector3( X - _Other, Y - _Other, Z - _Other );
    }
    TVector3 operator/( T const & _Other ) const
    {
        T Denom = T( 1 ) / _Other;
        return TVector3( X * Denom, Y * Denom, Z * Denom );
    }
    constexpr TVector3 operator*( T const & _Other ) const
    {
        return TVector3( X * _Other, Y * _Other, Z * _Other );
    }
    void operator+=( TVector3 const & _Other )
    {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
    }
    void operator-=( TVector3 const & _Other )
    {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
    }
    void operator/=( TVector3 const & _Other )
    {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
    }
    void operator*=( TVector3 const & _Other )
    {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
    }
    void operator+=( T const & _Other )
    {
        X += _Other;
        Y += _Other;
        Z += _Other;
    }
    void operator-=( T const & _Other )
    {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
    }
    void operator/=( T const & _Other )
    {
        T Denom = T( 1 ) / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
    }
    void operator*=( T const & _Other )
    {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
    }

    T Min() const
    {
        return Math::Min( Math::Min( X, Y ), Z );
    }

    T Max() const
    {
        return Math::Max( Math::Max( X, Y ), Z );
    }

    int MinorAxis() const
    {
        T   Minor = Math::Abs( X );
        int Axis  = 0;
        T   Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp <= Minor ) {
            Axis  = 1;
            Minor = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp <= Minor ) {
            Axis  = 2;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const
    {
        T   Major = Math::Abs( X );
        int Axis  = 0;
        T   Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp > Major ) {
            Axis  = 1;
            Major = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp > Major ) {
            Axis  = 2;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    constexpr Bool3 IsInfinite() const
    {
        return Bool3( Math::IsInfinite( X ), Math::IsInfinite( Y ), Math::IsInfinite( Z ) );
    }

    constexpr Bool3 IsNan() const
    {
        return Bool3( Math::IsNan( X ), Math::IsNan( Y ), Math::IsNan( Z ) );
    }

    constexpr Bool3 IsNormal() const
    {
        return Bool3( Math::IsNormal( X ), Math::IsNormal( Y ), Math::IsNormal( Z ) );
    }

    constexpr Bool3 IsDenormal() const
    {
        return Bool3( Math::IsDenormal( X ), Math::IsDenormal( Y ), Math::IsDenormal( Z ) );
    }

    // Comparision
    constexpr Bool3 LessThan( TVector3 const & _Other ) const
    {
        return Bool3( Math::LessThan( X, _Other.X ), Math::LessThan( Y, _Other.Y ), Math::LessThan( Z, _Other.Z ) );
    }
    constexpr Bool3 LessThan( T const & _Other ) const
    {
        return Bool3( Math::LessThan( X, _Other ), Math::LessThan( Y, _Other ), Math::LessThan( Z, _Other ) );
    }

    constexpr Bool3 LequalThan( TVector3 const & _Other ) const
    {
        return Bool3( Math::LequalThan( X, _Other.X ), Math::LequalThan( Y, _Other.Y ), Math::LequalThan( Z, _Other.Z ) );
    }
    constexpr Bool3 LequalThan( T const & _Other ) const
    {
        return Bool3( Math::LequalThan( X, _Other ), Math::LequalThan( Y, _Other ), Math::LequalThan( Z, _Other ) );
    }

    constexpr Bool3 GreaterThan( TVector3 const & _Other ) const
    {
        return Bool3( Math::GreaterThan( X, _Other.X ), Math::GreaterThan( Y, _Other.Y ), Math::GreaterThan( Z, _Other.Z ) );
    }
    constexpr Bool3 GreaterThan( T const & _Other ) const
    {
        return Bool3( Math::GreaterThan( X, _Other ), Math::GreaterThan( Y, _Other ), Math::GreaterThan( Z, _Other ) );
    }

    constexpr Bool3 GequalThan( TVector3 const & _Other ) const
    {
        return Bool3( Math::GequalThan( X, _Other.X ), Math::GequalThan( Y, _Other.Y ), Math::GequalThan( Z, _Other.Z ) );
    }
    constexpr Bool3 GequalThan( T const & _Other ) const
    {
        return Bool3( Math::GequalThan( X, _Other ), Math::GequalThan( Y, _Other ), Math::GequalThan( Z, _Other ) );
    }

    constexpr Bool3 NotEqual( TVector3 const & _Other ) const
    {
        return Bool3( Math::NotEqual( X, _Other.X ), Math::NotEqual( Y, _Other.Y ), Math::NotEqual( Z, _Other.Z ) );
    }
    constexpr Bool3 NotEqual( T const & _Other ) const
    {
        return Bool3( Math::NotEqual( X, _Other ), Math::NotEqual( Y, _Other ), Math::NotEqual( Z, _Other ) );
    }

    constexpr bool Compare( TVector3 const & _Other ) const
    {
        return !NotEqual( _Other ).Any();
    }

    constexpr bool CompareEps( TVector3 const & _Other, T const & _Epsilon ) const
    {
        return Bool3( Math::CompareEps( X, _Other.X, _Epsilon ),
                      Math::CompareEps( Y, _Other.Y, _Epsilon ),
                      Math::CompareEps( Z, _Other.Z, _Epsilon ) )
            .All();
    }

    void Clear()
    {
        X = Y = Z = 0;
    }

    TVector3 Abs() const { return TVector3( Math::Abs( X ), Math::Abs( Y ), Math::Abs( Z ) ); }

    // Vector methods
    constexpr T LengthSqr() const
    {
        return X * X + Y * Y + Z * Z;
    }
    T Length() const
    {
        return StdSqrt( LengthSqr() );
    }
    constexpr T DistSqr( TVector3 const & _Other ) const
    {
        return ( *this - _Other ).LengthSqr();
    }
    T Dist( TVector3 const & _Other ) const
    {
        return ( *this - _Other ).Length();
    }
    T NormalizeSelf()
    {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
        }
        return L;
    }
    TVector3 Normalized() const
    {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            return TVector3( X * InvLength, Y * InvLength, Z * InvLength );
        }
        return *this;
    }
    TVector3 NormalizeFix() const
    {
        TVector3 Normal = Normalized();
        Normal.FixNormal();
        return Normal;
    }
    // Return true if normal was fixed
    bool FixNormal()
    {
        const T ZERO      = 0;
        const T ONE       = 1;
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
            }
            else if ( Z == ZERO ) {
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
        }
        else if ( Y == ZERO ) {
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

    TVector3 Floor() const
    {
        return TVector3( Math::Floor( X ), Math::Floor( Y ), Math::Floor( Z ) );
    }

    TVector3 Ceil() const
    {
        return TVector3( Math::Ceil( X ), Math::Ceil( Y ), Math::Ceil( Z ) );
    }

    TVector3 Fract() const
    {
        return TVector3( Math::Fract( X ), Math::Fract( Y ), Math::Fract( Z ) );
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector3 Sign() const
    {
        return TVector3( Math::Sign( X ), Math::Sign( Y ), Math::Sign( Z ) );
    }

    constexpr int SignBits() const
    {
        return Math::SignBits( X ) | ( Math::SignBits( Y ) << 1 ) | ( Math::SignBits( Z ) << 2 );
    }

    //TVector3 Clamp( T const & _Min, T const & _Max ) const {
    //    return TVector3( Math::Clamp( X, _Min, _Max ), Math::Clamp( Y, _Min, _Max ), Math::Clamp( Z, _Min, _Max ) );
    //}

    //TVector3 Clamp( TVector3 const & _Min, TVector3 const & _Max ) const {
    //    return TVector3( Math::Clamp( X, _Min.X, _Max.X ), Math::Clamp( Y, _Min.Y, _Max.Y ), Math::Clamp( Z, _Min.Z, _Max.Z ) );
    //}

    //TVector3 Saturate() const {
    //    return Clamp( T( 0 ), T( 1 ) );
    //}

    TVector3 Snap( T const & _SnapValue ) const
    {
        AN_ASSERT_( _SnapValue > 0, "Snap" );
        TVector3 SnapVector;
        SnapVector   = *this / _SnapValue;
        SnapVector.X = Math::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = Math::Round( SnapVector.Y ) * _SnapValue;
        SnapVector.Z = Math::Round( SnapVector.Z ) * _SnapValue;
        return SnapVector;
    }

    TVector3 SnapNormal( T const & _Epsilon ) const
    {
        TVector3 Normal = *this;
        for ( int i = 0; i < 3; i++ ) {
            if ( Math::Abs( Normal[i] - T( 1 ) ) < _Epsilon ) {
                Normal    = TVector3( 0 );
                Normal[i] = 1;
                break;
            }
            if ( Math::Abs( Normal[i] - T( -1 ) ) < _Epsilon ) {
                Normal    = TVector3( 0 );
                Normal[i] = -1;
                break;
            }
        }

        if ( Math::Abs( Normal[0] ) < _Epsilon && Math::Abs( Normal[1] ) >= _Epsilon && Math::Abs( Normal[2] ) >= _Epsilon ) {
            Normal[0] = 0;
            Normal.NormalizeSelf();
        }
        else if ( Math::Abs( Normal[1] ) < _Epsilon && Math::Abs( Normal[0] ) >= _Epsilon && Math::Abs( Normal[2] ) >= _Epsilon ) {
            Normal[1] = 0;
            Normal.NormalizeSelf();
        }
        else if ( Math::Abs( Normal[2] ) < _Epsilon && Math::Abs( Normal[0] ) >= _Epsilon && Math::Abs( Normal[1] ) >= _Epsilon ) {
            Normal[2] = 0;
            Normal.NormalizeSelf();
        }

        return Normal;
    }

    int NormalAxialType() const
    {
        if ( X == T( 1 ) || X == T( -1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) || Y == T( -1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) || Z == T( -1 ) ) return Math::AxialZ;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const
    {
        if ( X == T( 1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) ) return Math::AxialZ;
        return Math::NonAxial;
    }

    int VectorAxialType() const
    {
        Bool3 Zero = Abs().LessThan( T( 0.00001 ) );

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

    TVector3 Perpendicular() const
    {
        T dp = X * X + Y * Y;
        if ( !dp ) {
            return TVector3( 1, 0, 0 );
        }
        else {
            dp = Math::InvSqrt( dp );
            return TVector3( -Y * dp, X * dp, 0 );
        }
    }

    void ComputeBasis( TVector3 & _XVec, TVector3 & _YVec ) const
    {
        _YVec = Perpendicular();
        _XVec = Math::Cross( _YVec, *this );
    }

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< T >() ) const
    {
        return AString( "( " ) + Math::ToString( X, _Precision ) + " " + Math::ToString( Y, _Precision ) + " " + Math::ToString( Z, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Z, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {

        struct Writer
        {
            Writer( IBinaryStream & _Stream, const float & _Value )
            {
                _Stream.WriteFloat( _Value );
            }

            Writer( IBinaryStream & _Stream, const double & _Value )
            {
                _Stream.WriteDouble( _Value );
            }
        };
        Writer( _Stream, X );
        Writer( _Stream, Y );
        Writer( _Stream, Z );
    }

    void Read( IBinaryStream & _Stream )
    {

        struct Reader
        {
            Reader( IBinaryStream & _Stream, float & _Value )
            {
                _Value = _Stream.ReadFloat();
            }

            Reader( IBinaryStream & _Stream, double & _Value )
            {
                _Value = _Stream.ReadDouble();
            }
        };
        Reader( _Stream, X );
        Reader( _Stream, Y );
        Reader( _Stream, Z );
    }

    // Static methods
    static constexpr int    NumComponents() { return 3; }
    static TVector3 const & Zero()
    {
        static constexpr TVector3 ZeroVec( T( 0 ) );
        return ZeroVec;
    }
};

template< typename T >
struct TVector4
{
    T X;
    T Y;
    T Z;
    T W;

    TVector4() = default;

    constexpr explicit TVector4( T const & _Value ) :
        X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}

    constexpr TVector4( T const & _X, T const & _Y, T const & _Z, T const & _W ) :
        X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    template< typename T2 >
    constexpr explicit TVector4< T >( TVector2< T2 > const & _Value, T2 const & _Z = T2( 0 ), T2 const & _W = T2( 0 ) ) :
        X( _Value.X ), Y( _Value.Y ), Z( _Z ), W( _W ) {}

    template< typename T2 >
    constexpr explicit TVector4< T >( TVector3< T2 > const & _Value, T const & _W = T( 0 ) ) :
        X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ), W( _W ) {}

    template< typename T2 >
    constexpr explicit TVector4< T >( TVector4< T2 > const & _Value ) :
        X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ) {}

    T * ToPtr()
    {
        return &X;
    }

    T const * ToPtr() const
    {
        return &X;
    }

    T & operator[]( int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return ( &X )[_Index];
    }

    T const & operator[]( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return ( &X )[_Index];
    }

    template< int Index >
    constexpr T const & Get() const
    {
        static_assert( Index >= 0 && Index < NumComponents(), "Index out of range" );
        return ( &X )[Index];
    }

    template< int _Shuffle >
    constexpr TVector2< T > Shuffle2() const
    {
        return TVector2< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >() );
    }

    template< int _Shuffle >
    constexpr TVector3< T > Shuffle3() const
    {
        return TVector3< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >(), Get< ( _Shuffle >> 2 ) & 3 >() );
    }

    template< int _Shuffle >
    constexpr TVector4< T > Shuffle4() const
    {
        return TVector4< T >( Get< ( _Shuffle >> 6 ) >(), Get< ( _Shuffle >> 4 ) & 3 >(), Get< ( _Shuffle >> 2 ) & 3 >(), Get< _Shuffle & 3 >() );
    }

    constexpr bool operator==( TVector4 const & _Other ) const { return Compare( _Other ); }
    constexpr bool operator!=( TVector4 const & _Other ) const { return !Compare( _Other ); }

    // Math operators
    constexpr TVector4 operator+() const
    {
        return *this;
    }
    constexpr TVector4 operator-() const
    {
        return TVector4( -X, -Y, -Z, -W );
    }
    constexpr TVector4 operator+( TVector4 const & _Other ) const
    {
        return TVector4( X + _Other.X, Y + _Other.Y, Z + _Other.Z, W + _Other.W );
    }
    constexpr TVector4 operator-( TVector4 const & _Other ) const
    {
        return TVector4( X - _Other.X, Y - _Other.Y, Z - _Other.Z, W - _Other.W );
    }
    constexpr TVector4 operator/( TVector4 const & _Other ) const
    {
        return TVector4( X / _Other.X, Y / _Other.Y, Z / _Other.Z, W / _Other.W );
    }
    constexpr TVector4 operator*( TVector4 const & _Other ) const
    {
        return TVector4( X * _Other.X, Y * _Other.Y, Z * _Other.Z, W * _Other.W );
    }
    constexpr TVector4 operator+( T const & _Other ) const
    {
        return TVector4( X + _Other, Y + _Other, Z + _Other, W + _Other );
    }
    constexpr TVector4 operator-( T const & _Other ) const
    {
        return TVector4( X - _Other, Y - _Other, Z - _Other, W - _Other );
    }
    TVector4 operator/( T const & _Other ) const
    {
        T Denom = T( 1 ) / _Other;
        return TVector4( X * Denom, Y * Denom, Z * Denom, W * Denom );
    }
    constexpr TVector4 operator*( T const & _Other ) const
    {
        return TVector4( X * _Other, Y * _Other, Z * _Other, W * _Other );
    }
    void operator+=( TVector4 const & _Other )
    {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
        W += _Other.W;
    }
    void operator-=( TVector4 const & _Other )
    {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
        W -= _Other.W;
    }
    void operator/=( TVector4 const & _Other )
    {
        X /= _Other.X;
        Y /= _Other.Y;
        Z /= _Other.Z;
        W /= _Other.W;
    }
    void operator*=( TVector4 const & _Other )
    {
        X *= _Other.X;
        Y *= _Other.Y;
        Z *= _Other.Z;
        W *= _Other.W;
    }
    void operator+=( T const & _Other )
    {
        X += _Other;
        Y += _Other;
        Z += _Other;
        W += _Other;
    }
    void operator-=( T const & _Other )
    {
        X -= _Other;
        Y -= _Other;
        Z -= _Other;
        W -= _Other;
    }
    void operator/=( T const & _Other )
    {
        T Denom = T( 1 ) / _Other;
        X *= Denom;
        Y *= Denom;
        Z *= Denom;
        W *= Denom;
    }
    void operator*=( T const & _Other )
    {
        X *= _Other;
        Y *= _Other;
        Z *= _Other;
        W *= _Other;
    }

    T Min() const
    {
        return Math::Min( Math::Min( Math::Min( X, Y ), Z ), W );
    }

    T Max() const
    {
        return Math::Max( Math::Max( Math::Max( X, Y ), Z ), W );
    }

    int MinorAxis() const
    {
        T   Minor = Math::Abs( X );
        int Axis  = 0;
        T   Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp <= Minor ) {
            Axis  = 1;
            Minor = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp <= Minor ) {
            Axis  = 2;
            Minor = Tmp;
        }

        Tmp = Math::Abs( W );
        if ( Tmp <= Minor ) {
            Axis  = 3;
            Minor = Tmp;
        }

        return Axis;
    }

    int MajorAxis() const
    {
        T   Major = Math::Abs( X );
        int Axis  = 0;
        T   Tmp;

        Tmp = Math::Abs( Y );
        if ( Tmp > Major ) {
            Axis  = 1;
            Major = Tmp;
        }

        Tmp = Math::Abs( Z );
        if ( Tmp > Major ) {
            Axis  = 2;
            Major = Tmp;
        }

        Tmp = Math::Abs( W );
        if ( Tmp > Major ) {
            Axis  = 3;
            Major = Tmp;
        }

        return Axis;
    }

    // Floating point specific
    constexpr Bool4 IsInfinite() const
    {
        return Bool4( Math::IsInfinite( X ), Math::IsInfinite( Y ), Math::IsInfinite( Z ), Math::IsInfinite( W ) );
    }

    constexpr Bool4 IsNan() const
    {
        return Bool4( Math::IsNan( X ), Math::IsNan( Y ), Math::IsNan( Z ), Math::IsNan( W ) );
    }

    constexpr Bool4 IsNormal() const
    {
        return Bool4( Math::IsNormal( X ), Math::IsNormal( Y ), Math::IsNormal( Z ), Math::IsNormal( W ) );
    }

    constexpr Bool4 IsDenormal() const
    {
        return Bool4( Math::IsDenormal( X ), Math::IsDenormal( Y ), Math::IsDenormal( Z ), Math::IsDenormal( W ) );
    }

    // Comparision
    constexpr Bool4 LessThan( TVector4 const & _Other ) const
    {
        return Bool4( Math::LessThan( X, _Other.X ), Math::LessThan( Y, _Other.Y ), Math::LessThan( Z, _Other.Z ), Math::LessThan( W, _Other.W ) );
    }
    constexpr Bool4 LessThan( T const & _Other ) const
    {
        return Bool4( Math::LessThan( X, _Other ), Math::LessThan( Y, _Other ), Math::LessThan( Z, _Other ), Math::LessThan( W, _Other ) );
    }

    constexpr Bool4 LequalThan( TVector4 const & _Other ) const
    {
        return Bool4( Math::LequalThan( X, _Other.X ), Math::LequalThan( Y, _Other.Y ), Math::LequalThan( Z, _Other.Z ), Math::LequalThan( W, _Other.W ) );
    }
    constexpr Bool4 LequalThan( T const & _Other ) const
    {
        return Bool4( Math::LequalThan( X, _Other ), Math::LequalThan( Y, _Other ), Math::LequalThan( Z, _Other ), Math::LequalThan( W, _Other ) );
    }

    constexpr Bool4 GreaterThan( TVector4 const & _Other ) const
    {
        return Bool4( Math::GreaterThan( X, _Other.X ), Math::GreaterThan( Y, _Other.Y ), Math::GreaterThan( Z, _Other.Z ), Math::GreaterThan( W, _Other.W ) );
    }
    constexpr Bool4 GreaterThan( T const & _Other ) const
    {
        return Bool4( Math::GreaterThan( X, _Other ), Math::GreaterThan( Y, _Other ), Math::GreaterThan( Z, _Other ), Math::GreaterThan( W, _Other ) );
    }

    constexpr Bool4 GequalThan( TVector4 const & _Other ) const
    {
        return Bool4( Math::GequalThan( X, _Other.X ), Math::GequalThan( Y, _Other.Y ), Math::GequalThan( Z, _Other.Z ), Math::GequalThan( W, _Other.W ) );
    }
    constexpr Bool4 GequalThan( T const & _Other ) const
    {
        return Bool4( Math::GequalThan( X, _Other ), Math::GequalThan( Y, _Other ), Math::GequalThan( Z, _Other ), Math::GequalThan( W, _Other ) );
    }

    constexpr Bool4 NotEqual( TVector4 const & _Other ) const
    {
        return Bool4( Math::NotEqual( X, _Other.X ), Math::NotEqual( Y, _Other.Y ), Math::NotEqual( Z, _Other.Z ), Math::NotEqual( W, _Other.W ) );
    }
    constexpr Bool4 NotEqual( T const & _Other ) const
    {
        return Bool4( Math::NotEqual( X, _Other ), Math::NotEqual( Y, _Other ), Math::NotEqual( Z, _Other ), Math::NotEqual( W, _Other ) );
    }

    constexpr bool Compare( TVector4 const & _Other ) const
    {
        return !NotEqual( _Other ).Any();
    }

    constexpr bool CompareEps( TVector4 const & _Other, T const & _Epsilon ) const
    {
        return Bool4( Math::CompareEps( X, _Other.X, _Epsilon ),
                      Math::CompareEps( Y, _Other.Y, _Epsilon ),
                      Math::CompareEps( Z, _Other.Z, _Epsilon ),
                      Math::CompareEps( W, _Other.W, _Epsilon ) )
            .All();
    }

    void Clear()
    {
        X = Y = Z = W = 0;
    }

    TVector4 Abs() const { return TVector4( Math::Abs( X ), Math::Abs( Y ), Math::Abs( Z ), Math::Abs( W ) ); }

    // Vector methods
    constexpr T LengthSqr() const
    {
        return X * X + Y * Y + Z * Z + W * W;
    }
    T Length() const
    {
        return StdSqrt( LengthSqr() );
    }
    constexpr T DistSqr( TVector4 const & _Other ) const
    {
        return ( *this - _Other ).LengthSqr();
    }
    T Dist( TVector4 const & _Other ) const
    {
        return ( *this - _Other ).Length();
    }
    T NormalizeSelf()
    {
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
    TVector4 Normalized() const
    {
        const T L = Length();
        if ( L != T( 0 ) ) {
            const T InvLength = T( 1 ) / L;
            return TVector4( X * InvLength, Y * InvLength, Z * InvLength, W * InvLength );
        }
        return *this;
    }

    TVector4 Floor() const
    {
        return TVector4( Math::Floor( X ), Math::Floor( Y ), Math::Floor( Z ), Math::Floor( W ) );
    }

    TVector4 Ceil() const
    {
        return TVector4( Math::Ceil( X ), Math::Ceil( Y ), Math::Ceil( Z ), Math::Ceil( W ) );
    }

    TVector4 Fract() const
    {
        return TVector4( Math::Fract( X ), Math::Fract( Y ), Math::Fract( Z ), Math::Fract( W ) );
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector4 Sign() const
    {
        return TVector4( Math::Sign( X ), Math::Sign( Y ), Math::Sign( Z ), Math::Sign( W ) );
    }

    constexpr int SignBits() const
    {
        return Math::SignBits( X ) | ( Math::SignBits( Y ) << 1 ) | ( Math::SignBits( Z ) << 2 ) | ( Math::SignBits( W ) << 3 );
    }

    //TVector4 Clamp( T const & _Min, T const & _Max ) const {
    //    return TVector4( Math::Clamp( X, _Min, _Max ), Math::Clamp( Y, _Min, _Max ), Math::Clamp( Z, _Min, _Max ), Math::Clamp( W, _Min, _Max ) );
    //}

    //TVector4 Clamp( TVector4 const & _Min, TVector4 const & _Max ) const {
    //    return TVector4( Math::Clamp( X, _Min.X, _Max.X ), Math::Clamp( Y, _Min.Y, _Max.Y ), Math::Clamp( Z, _Min.Z, _Max.Z ), Math::Clamp( W, _Min.W, _Max.W ) );
    //}

    //TVector4 Saturate() const {
    //    return Clamp( T( 0 ), T( 1 ) );
    //}

    TVector4 Snap( T const & _SnapValue ) const
    {
        AN_ASSERT_( _SnapValue > 0, "Snap" );
        TVector4 SnapVector;
        SnapVector   = *this / _SnapValue;
        SnapVector.X = Math::Round( SnapVector.X ) * _SnapValue;
        SnapVector.Y = Math::Round( SnapVector.Y ) * _SnapValue;
        SnapVector.Z = Math::Round( SnapVector.Z ) * _SnapValue;
        SnapVector.W = Math::Round( SnapVector.W ) * _SnapValue;
        return SnapVector;
    }

    int NormalAxialType() const
    {
        if ( X == T( 1 ) || X == T( -1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) || Y == T( -1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) || Z == T( -1 ) ) return Math::AxialZ;
        if ( W == T( 1 ) || W == T( -1 ) ) return Math::AxialW;
        return Math::NonAxial;
    }

    int NormalPositiveAxialType() const
    {
        if ( X == T( 1 ) ) return Math::AxialX;
        if ( Y == T( 1 ) ) return Math::AxialY;
        if ( Z == T( 1 ) ) return Math::AxialZ;
        if ( W == T( 1 ) ) return Math::AxialW;
        return Math::NonAxial;
    }

    int VectorAxialType() const
    {
        Bool4 Zero = Abs().LessThan( T( 0.00001 ) );

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
    AString ToString( int _Precision = Math::FloatingPointPrecision< T >() ) const
    {
        return AString( "( " ) + Math::ToString( X, _Precision ) + " " + Math::ToString( Y, _Precision ) + " " + Math::ToString( Z, _Precision ) + " " + Math::ToString( W, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Z, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( W, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {

        struct Writer
        {
            Writer( IBinaryStream & _Stream, const float & _Value )
            {
                _Stream.WriteFloat( _Value );
            }

            Writer( IBinaryStream & _Stream, const double & _Value )
            {
                _Stream.WriteDouble( _Value );
            }
        };
        Writer( _Stream, X );
        Writer( _Stream, Y );
        Writer( _Stream, Z );
        Writer( _Stream, W );
    }

    void Read( IBinaryStream & _Stream )
    {

        struct Reader
        {
            Reader( IBinaryStream & _Stream, float & _Value )
            {
                _Value = _Stream.ReadFloat();
            }

            Reader( IBinaryStream & _Stream, double & _Value )
            {
                _Value = _Stream.ReadDouble();
            }
        };
        Reader( _Stream, X );
        Reader( _Stream, Y );
        Reader( _Stream, Z );
        Reader( _Stream, W );
    }

    // Static methods
    static constexpr int    NumComponents() { return 4; }
    static TVector4 const & Zero()
    {
        static constexpr TVector4 ZeroVec( T( 0 ) );
        return ZeroVec;
    }
};

namespace Math
{

AN_FORCEINLINE TVector2< float > Min( TVector2< float > const & a, TVector2< float > const & b )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_set_ps( a.X, a.Y, 0.0f, 0.0f ), _mm_set_ps( b.X, b.Y, 0.0f, 0.0f ) ) );

    return TVector2< float >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< float > Min( TVector3< float > const & a, TVector3< float > const & b )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_set_ps( a.X, a.Y, a.Z, 0.0f ), _mm_set_ps( b.X, b.Y, b.Z, 0.0f ) ) );

    return TVector3< float >( result[0], result[1], result[2] );
}

AN_FORCEINLINE TVector4< float > Min( TVector4< float > const & a, TVector4< float > const & b )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_set_ps( a.X, a.Y, a.Z, a.W ), _mm_set_ps( b.X, b.Y, b.Z, b.W ) ) );

    return TVector4< float >( result[0], result[1], result[2], result[3] );
}

AN_FORCEINLINE TVector2< float > Max( TVector2< float > const & a, TVector2< float > const & b )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_max_ps( _mm_set_ps( a.X, a.Y, 0.0f, 0.0f ), _mm_set_ps( b.X, b.Y, 0.0f, 0.0f ) ) );

    return TVector2< float >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< float > Max( TVector3< float > const & a, TVector3< float > const & b )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_max_ps( _mm_set_ps( a.X, a.Y, a.Z, 0.0f ), _mm_set_ps( b.X, b.Y, b.Z, 0.0f ) ) );

    return TVector3< float >( result[0], result[1], result[2] );
}

AN_FORCEINLINE TVector4< float > Max( TVector4< float > const & a, TVector4< float > const & b )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_max_ps( _mm_set_ps( a.X, a.Y, a.Z, a.W ), _mm_set_ps( b.X, b.Y, b.Z, b.W ) ) );

    return TVector4< float >( result[0], result[1], result[2], result[3] );
}

AN_FORCEINLINE TVector2< float > Clamp( TVector2< float > const & val, TVector2< float > const & minval, TVector2< float > const & maxval )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_max_ps( _mm_set_ps( val.X, val.Y, 0.0f, 0.0f ), _mm_set_ps( minval.X, minval.Y, 0.0f, 0.0f ) ), _mm_set_ps( maxval.X, maxval.Y, 0.0f, 0.0f ) ) );

    return TVector2< float >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< float > Clamp( TVector3< float > const & val, TVector3< float > const & minval, TVector3< float > const & maxval )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_max_ps( _mm_set_ps( val.X, val.Y, val.Z, 0.0f ), _mm_set_ps( minval.X, minval.Y, minval.Z, 0.0f ) ), _mm_set_ps( maxval.X, maxval.Y, maxval.Z, 0.0f ) ) );

    return TVector3< float >( result[0], result[1], result[2] );
}

AN_FORCEINLINE TVector4< float > Clamp( TVector4< float > const & val, TVector4< float > const & minval, TVector4< float > const & maxval )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_max_ps( _mm_set_ps( val.X, val.Y, val.Z, val.W ), _mm_set_ps( minval.X, minval.Y, minval.Z, minval.W ) ), _mm_set_ps( maxval.X, maxval.Y, maxval.Z, maxval.W ) ) );

    return TVector4< float >( result[0], result[1], result[2], result[3] );
}

AN_FORCEINLINE TVector2< float > Saturate( TVector2< float > const & val )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_max_ps( _mm_set_ps( val.X, val.Y, 0.0f, 0.0f ), _mm_setzero_ps() ), _mm_set_ps( 1.0f, 1.0f, 1.0f, 1.0f ) ) );

    return TVector2< float >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< float > Saturate( TVector3< float > const & val )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_max_ps( _mm_set_ps( val.X, val.Y, val.Z, 0.0f ), _mm_setzero_ps() ), _mm_set_ps( 1.0f, 1.0f, 1.0f, 1.0f ) ) );

    return TVector3< float >( result[0], result[1], result[2] );
}

AN_FORCEINLINE TVector4< float > Saturate( TVector4< float > const & val )
{
    alignas( 16 ) float result[4];

    _mm_storer_ps( result, _mm_min_ps( _mm_max_ps( _mm_set_ps( val.X, val.Y, val.Z, val.W ), _mm_setzero_ps() ), _mm_set_ps( 1.0f, 1.0f, 1.0f, 1.0f ) ) );

    return TVector4< float >( result[0], result[1], result[2], result[3] );
}

AN_FORCEINLINE TVector2< double > Min( TVector2< double > const & a, TVector2< double > const & b )
{
    alignas( 16 ) double result[2];

    _mm_storer_pd( result, _mm_min_pd( _mm_set_pd( a.X, a.Y ), _mm_set_pd( b.X, b.Y ) ) );

    return TVector2< double >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< double > Min( TVector3< double > const & a, TVector3< double > const & b )
{
    alignas( 16 ) double result[2];
    double               z;

    _mm_storer_pd( result, _mm_min_pd( _mm_set_pd( a.X, a.Y ), _mm_set_pd( b.X, b.Y ) ) );
    _mm_store_sd( &z, _mm_min_sd( _mm_set_sd( a.Z ), _mm_set_sd( b.Z ) ) );

    return TVector3< double >( result[0], result[1], z );
}

AN_FORCEINLINE TVector4< double > Min( TVector4< double > const & a, TVector4< double > const & b )
{
    alignas( 16 ) double result1[2];
    alignas( 16 ) double result2[2];

    _mm_storer_pd( result1, _mm_min_pd( _mm_set_pd( a.X, a.Y ), _mm_set_pd( b.X, b.Y ) ) );
    _mm_storer_pd( result2, _mm_min_pd( _mm_set_pd( a.Z, a.W ), _mm_set_pd( b.Z, b.W ) ) );

    return TVector4< double >( result1[0], result1[1], result2[0], result2[1] );
}

AN_FORCEINLINE TVector2< double > Max( TVector2< double > const & a, TVector2< double > const & b )
{
    alignas( 16 ) double result[2];

    _mm_storer_pd( result, _mm_max_pd( _mm_set_pd( a.X, a.Y ), _mm_set_pd( b.X, b.Y ) ) );

    return TVector2< double >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< double > Max( TVector3< double > const & a, TVector3< double > const & b )
{
    alignas( 16 ) double result[2];
    double               z;

    _mm_storer_pd( result, _mm_max_pd( _mm_set_pd( a.X, a.Y ), _mm_set_pd( b.X, b.Y ) ) );
    _mm_store_sd( &z, _mm_max_sd( _mm_set_sd( a.Z ), _mm_set_sd( b.Z ) ) );

    return TVector3< double >( result[0], result[1], z );
}

AN_FORCEINLINE TVector4< double > Max( TVector4< double > const & a, TVector4< double > const & b )
{
    alignas( 16 ) double result1[2];
    alignas( 16 ) double result2[2];

    _mm_storer_pd( result1, _mm_max_pd( _mm_set_pd( a.X, a.Y ), _mm_set_pd( b.X, b.Y ) ) );
    _mm_storer_pd( result2, _mm_max_pd( _mm_set_pd( a.Z, a.W ), _mm_set_pd( b.Z, b.W ) ) );

    return TVector4< double >( result1[0], result1[1], result2[0], result2[1] );
}

AN_FORCEINLINE TVector2< double > Clamp( TVector2< double > const & val, TVector2< double > const & minval, TVector2< double > const & maxval )
{
    alignas( 16 ) double result[2];

    _mm_storer_pd( result, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.X, val.Y ), _mm_set_pd( minval.X, minval.Y ) ), _mm_set_pd( maxval.X, maxval.Y ) ) );

    return TVector2< double >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< double > Clamp( TVector3< double > const & val, TVector3< double > const & minval, TVector3< double > const & maxval )
{
    alignas( 16 ) double result[2];
    double               z;

    _mm_storer_pd( result, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.X, val.Y ), _mm_set_pd( minval.X, minval.Y ) ), _mm_set_pd( maxval.X, maxval.Y ) ) );
    _mm_store_sd( &z, _mm_min_sd( _mm_max_sd( _mm_set_sd( val.Z ), _mm_set_sd( minval.Z ) ), _mm_set_sd( maxval.Z ) ) );

    return TVector3< double >( result[0], result[1], z );
}

AN_FORCEINLINE TVector4< double > Clamp( TVector4< double > const & val, TVector4< double > const & minval, TVector4< double > const & maxval )
{
    alignas( 16 ) double result1[2];
    alignas( 16 ) double result2[2];

    _mm_storer_pd( result1, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.X, val.Y ), _mm_set_pd( minval.X, minval.Y ) ), _mm_set_pd( maxval.X, maxval.Y ) ) );
    _mm_storer_pd( result2, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.Z, val.W ), _mm_set_pd( minval.Z, minval.W ) ), _mm_set_pd( maxval.Z, maxval.W ) ) );

    return TVector4< double >( result1[0], result1[1], result2[0], result2[1] );
}

AN_FORCEINLINE TVector2< double > Saturate( TVector2< double > const & val )
{
    alignas( 16 ) double result[2];

    _mm_storer_pd( result, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.X, val.Y ), _mm_setzero_pd() ), _mm_set_pd( 1.0, 1.0 ) ) );

    return TVector2< double >( result[0], result[1] );
}

AN_FORCEINLINE TVector3< double > Saturate( TVector3< double > const & val )
{
    alignas( 16 ) double result[2];
    double               z;

    _mm_storer_pd( result, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.X, val.Y ), _mm_setzero_pd() ), _mm_set_pd( 1.0, 1.0 ) ) );
    _mm_store_sd( &z, _mm_min_sd( _mm_max_sd( _mm_set_sd( val.Z ), _mm_setzero_pd() ), _mm_set_sd( 1.0 ) ) );

    return TVector3< double >( result[0], result[1], z );
}

AN_FORCEINLINE TVector4< double > Saturate( TVector4< double > const & val )
{
    alignas( 16 ) double result1[2];
    alignas( 16 ) double result2[2];

    _mm_storer_pd( result1, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.X, val.Y ), _mm_setzero_pd() ), _mm_set_pd( 1.0, 1.0 ) ) );
    _mm_storer_pd( result2, _mm_min_pd( _mm_max_pd( _mm_set_pd( val.Z, val.W ), _mm_setzero_pd() ), _mm_set_pd( 1.0, 1.0 ) ) );

    return TVector4< double >( result1[0], result1[1], result2[0], result2[1] );
}

} // namespace Math

using Float2  = TVector2< float >;
using Float3  = TVector3< float >;
using Float4  = TVector4< float >;
using Double2 = TVector2< double >;
using Double3 = TVector3< double >;
using Double4 = TVector4< double >;

struct Float2x2;
struct Float3x3;
struct Float4x4;
struct Float3x4;

// Column-major matrix 2x2
struct Float2x2
{
    Float2 Col0;
    Float2 Col1;

    Float2x2() = default;
    explicit Float2x2( Float3x3 const & _Value );
    explicit Float2x2( Float3x4 const & _Value );
    explicit Float2x2( Float4x4 const & _Value );
    constexpr Float2x2( Float2 const & _Col0, Float2 const & _Col1 ) :
        Col0( _Col0 ), Col1( _Col1 ) {}
    constexpr Float2x2( float _00, float _01, float _10, float _11 ) :
        Col0( _00, _01 ), Col1( _10, _11 ) {}
    constexpr explicit Float2x2( float _Diagonal ) :
        Col0( _Diagonal, 0 ), Col1( 0, _Diagonal ) {}
    constexpr explicit Float2x2( Float2 const & _Diagonal ) :
        Col0( _Diagonal.X, 0 ), Col1( 0, _Diagonal.Y ) {}

    float * ToPtr()
    {
        return &Col0.X;
    }

    const float * ToPtr() const
    {
        return &Col0.X;
    }

    Float2 & operator[]( int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float2 const & operator[]( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float2 GetRow( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return Float2( Col0[_Index], Col1[_Index] );
    }

    bool operator==( Float2x2 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float2x2 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float2x2 const & _Other ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 4; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float2x2 const & _Other, float _Epsilon ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 4; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf()
    {
        float Temp = Col0.Y;
        Col0.Y     = Col1.X;
        Col1.X     = Temp;
    }

    Float2x2 Transposed() const
    {
        return Float2x2( Col0.X, Col1.X, Col0.Y, Col1.Y );
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float2x2 Inversed() const
    {
        const float OneOverDeterminant = 1.0f / ( Col0[0] * Col1[1] - Col1[0] * Col0[1] );
        return Float2x2( Col1[1] * OneOverDeterminant,
                         -Col0[1] * OneOverDeterminant,
                         -Col1[0] * OneOverDeterminant,
                         Col0[0] * OneOverDeterminant );
    }

    float Determinant() const
    {
        return Col0[0] * Col1[1] - Col1[0] * Col0[1];
    }

    void Clear()
    {
        Core::ZeroMem( this, sizeof( *this ) );
    }

    void SetIdentity()
    {
        Col0.Y = Col1.X = 0;
        Col0.X = Col1.Y = 1;
    }

    static Float2x2 Scale( Float2 const & _Scale )
    {
        return Float2x2( _Scale );
    }

    Float2x2 Scaled( Float2 const & _Scale ) const
    {
        return Float2x2( Col0 * _Scale[0], Col1 * _Scale[1] );
    }

    // Return rotation around Z axis
    static Float2x2 Rotation( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float2x2( c, s,
                         -s, c );
    }

    Float2x2 operator*( float _Value ) const
    {
        return Float2x2( Col0 * _Value,
                         Col1 * _Value );
    }

    void operator*=( float _Value )
    {
        Col0 *= _Value;
        Col1 *= _Value;
    }

    Float2x2 operator/( float _Value ) const
    {
        const float OneOverValue = 1.0f / _Value;
        return Float2x2( Col0 * OneOverValue,
                         Col1 * OneOverValue );
    }

    void operator/=( float _Value )
    {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
    }

    template< typename T >
    TVector2< T > operator*( TVector2< T > const & _Vec ) const
    {
        return TVector2< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y );
    }

    Float2x2 operator*( Float2x2 const & _Mat ) const
    {
        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float R00 = _Mat[0][0];
        const float R01 = _Mat[0][1];
        const float R10 = _Mat[1][0];
        const float R11 = _Mat[1][1];
        return Float2x2( L00 * R00 + L10 * R01,
                         L01 * R00 + L11 * R01,
                         L00 * R10 + L10 * R11,
                         L01 * R10 + L11 * R11 );
    }

    void operator*=( Float2x2 const & _Mat )
    {
        //*this = *this * _Mat;

        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float R00 = _Mat[0][0];
        const float R01 = _Mat[0][1];
        const float R10 = _Mat[1][0];
        const float R11 = _Mat[1][1];
        Col0[0]         = L00 * R00 + L10 * R01;
        Col0[1]         = L01 * R00 + L11 * R01;
        Col1[0]         = L00 * R10 + L10 * R11;
        Col1[1]         = L01 * R10 + L11 * R11;
    }

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const
    {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream )
    {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
    }

    // Static methods

    static Float2x2 const & Identity()
    {
        static constexpr Float2x2 IdentityMat( 1 );
        return IdentityMat;
    }
};

// Column-major matrix 3x3
struct Float3x3
{
    Float3 Col0;
    Float3 Col1;
    Float3 Col2;

    Float3x3() = default;
    explicit Float3x3( Float2x2 const & _Value );
    explicit Float3x3( Float3x4 const & _Value );
    explicit Float3x3( Float4x4 const & _Value );
    constexpr Float3x3( Float3 const & _Col0, Float3 const & _Col1, Float3 const & _Col2 ) :
        Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Float3x3( float _00, float _01, float _02, float _10, float _11, float _12, float _20, float _21, float _22 ) :
        Col0( _00, _01, _02 ), Col1( _10, _11, _12 ), Col2( _20, _21, _22 ) {}
    constexpr explicit Float3x3( float _Diagonal ) :
        Col0( _Diagonal, 0, 0 ), Col1( 0, _Diagonal, 0 ), Col2( 0, 0, _Diagonal ) {}
    constexpr explicit Float3x3( Float3 const & _Diagonal ) :
        Col0( _Diagonal.X, 0, 0 ), Col1( 0, _Diagonal.Y, 0 ), Col2( 0, 0, _Diagonal.Z ) {}

    float * ToPtr()
    {
        return &Col0.X;
    }

    const float * ToPtr() const
    {
        return &Col0.X;
    }

    Float3 & operator[]( int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float3 const & operator[]( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float3 GetRow( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return Float3( Col0[_Index], Col1[_Index], Col2[_Index] );
    }

    bool operator==( Float3x3 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float3x3 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float3x3 const & _Other ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 9; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float3x3 const & _Other, float _Epsilon ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 9; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf()
    {
        float Temp = Col0.Y;
        Col0.Y     = Col1.X;
        Col1.X     = Temp;
        Temp       = Col0.Z;
        Col0.Z     = Col2.X;
        Col2.X     = Temp;
        Temp       = Col1.Z;
        Col1.Z     = Col2.Y;
        Col2.Y     = Temp;
    }

    Float3x3 Transposed() const
    {
        return Float3x3( Col0.X, Col1.X, Col2.X, Col0.Y, Col1.Y, Col2.Y, Col0.Z, Col1.Z, Col2.Z );
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float3x3 Inversed() const
    {
        Float3x3 const & m                  = *this;
        const float      A                  = m[1][1] * m[2][2] - m[2][1] * m[1][2];
        const float      B                  = m[0][1] * m[2][2] - m[2][1] * m[0][2];
        const float      C                  = m[0][1] * m[1][2] - m[1][1] * m[0][2];
        const float      OneOverDeterminant = 1.0f / ( m[0][0] * A - m[1][0] * B + m[2][0] * C );

        Float3x3 Inversed;
        Inversed[0][0] = A * OneOverDeterminant;
        Inversed[1][0] = -( m[1][0] * m[2][2] - m[2][0] * m[1][2] ) * OneOverDeterminant;
        Inversed[2][0] = ( m[1][0] * m[2][1] - m[2][0] * m[1][1] ) * OneOverDeterminant;
        Inversed[0][1] = -B * OneOverDeterminant;
        Inversed[1][1] = ( m[0][0] * m[2][2] - m[2][0] * m[0][2] ) * OneOverDeterminant;
        Inversed[2][1] = -( m[0][0] * m[2][1] - m[2][0] * m[0][1] ) * OneOverDeterminant;
        Inversed[0][2] = C * OneOverDeterminant;
        Inversed[1][2] = -( m[0][0] * m[1][2] - m[1][0] * m[0][2] ) * OneOverDeterminant;
        Inversed[2][2] = ( m[0][0] * m[1][1] - m[1][0] * m[0][1] ) * OneOverDeterminant;
        return Inversed;
    }

    float Determinant() const
    {
        return Col0[0] * ( Col1[1] * Col2[2] - Col2[1] * Col1[2] ) -
            Col1[0] * ( Col0[1] * Col2[2] - Col2[1] * Col0[2] ) +
            Col2[0] * ( Col0[1] * Col1[2] - Col1[1] * Col0[2] );
    }

    void Clear()
    {
        Core::ZeroMem( this, sizeof( *this ) );
    }

    void SetIdentity()
    {
        *this = Identity();
    }

    static Float3x3 Scale( Float3 const & _Scale )
    {
        return Float3x3( _Scale );
    }

    Float3x3 Scaled( Float3 const & _Scale ) const
    {
        return Float3x3( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2] );
    }

    // Return rotation around normalized axis
    static Float3x3 RotationAroundNormal( float _AngleRad, Float3 const & _Normal )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        const Float3 Temp  = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float3x3( c + Temp[0] * _Normal[0], Temp[0] * _Normal[1] + Temp2[2], Temp[0] * _Normal[2] - Temp2[1],
                         Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1], Temp[1] * _Normal[2] + Temp2[0],
                         Temp[2] * _Normal[0] + Temp2[1], Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2] );
    }

    // Return rotation around normalized axis
    Float3x3 RotateAroundNormal( float _AngleRad, Float3 const & _Normal ) const
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        Float3 Temp  = ( 1.0f - c ) * _Normal;
        Float3 Temp2 = s * _Normal;
        return Float3x3( Col0 * ( c + Temp[0] * _Normal[0] ) + Col1 * ( Temp[0] * _Normal[1] + Temp2[2] ) + Col2 * ( Temp[0] * _Normal[2] - Temp2[1] ),
                         Col0 * ( Temp[1] * _Normal[0] - Temp2[2] ) + Col1 * ( c + Temp[1] * _Normal[1] ) + Col2 * ( Temp[1] * _Normal[2] + Temp2[0] ),
                         Col0 * ( Temp[2] * _Normal[0] + Temp2[1] ) + Col1 * ( Temp[2] * _Normal[1] - Temp2[0] ) + Col2 * ( c + Temp[2] * _Normal[2] ) );
    }

    // Return rotation around unnormalized vector
    static Float3x3 RotationAroundVector( float _AngleRad, Float3 const & _Vector )
    {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Float3x3 RotateAroundVector( float _AngleRad, Float3 const & _Vector ) const
    {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float3x3 RotationX( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x3( 1, 0, 0,
                         0, c, s,
                         0, -s, c );
    }

    // Return rotation around Y axis
    static Float3x3 RotationY( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x3( c, 0, -s,
                         0, 1, 0,
                         s, 0, c );
    }

    // Return rotation around Z axis
    static Float3x3 RotationZ( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x3( c, s, 0,
                         -s, c, 0,
                         0, 0, 1 );
    }

    Float3x3 operator*( float _Value ) const
    {
        return Float3x3( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( float _Value )
    {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Float3x3 operator/( float _Value ) const
    {
        const float OneOverValue = 1.0f / _Value;
        return Float3x3( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue );
    }

    void operator/=( float _Value )
    {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    template< typename T >
    TVector3< T > operator*( TVector3< T > const & _Vec ) const
    {
        return TVector3< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z,
                              Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z );
    }

    Float3x3 operator*( Float3x3 const & _Mat ) const
    {
        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L02 = Col0[2];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float L12 = Col1[2];
        const float L20 = Col2[0];
        const float L21 = Col2[1];
        const float L22 = Col2[2];
        const float R00 = _Mat[0][0];
        const float R01 = _Mat[0][1];
        const float R02 = _Mat[0][2];
        const float R10 = _Mat[1][0];
        const float R11 = _Mat[1][1];
        const float R12 = _Mat[1][2];
        const float R20 = _Mat[2][0];
        const float R21 = _Mat[2][1];
        const float R22 = _Mat[2][2];
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

    void operator*=( Float3x3 const & _Mat )
    {
        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L02 = Col0[2];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float L12 = Col1[2];
        const float L20 = Col2[0];
        const float L21 = Col2[1];
        const float L22 = Col2[2];
        const float R00 = _Mat[0][0];
        const float R01 = _Mat[0][1];
        const float R02 = _Mat[0][2];
        const float R10 = _Mat[1][0];
        const float R11 = _Mat[1][1];
        const float R12 = _Mat[1][2];
        const float R20 = _Mat[2][0];
        const float R21 = _Mat[2][1];
        const float R22 = _Mat[2][2];
        Col0[0]         = L00 * R00 + L10 * R01 + L20 * R02;
        Col0[1]         = L01 * R00 + L11 * R01 + L21 * R02;
        Col0[2]         = L02 * R00 + L12 * R01 + L22 * R02;
        Col1[0]         = L00 * R10 + L10 * R11 + L20 * R12;
        Col1[1]         = L01 * R10 + L11 * R11 + L21 * R12;
        Col1[2]         = L02 * R10 + L12 * R11 + L22 * R12;
        Col2[0]         = L00 * R20 + L10 * R21 + L20 * R22;
        Col2[1]         = L01 * R20 + L11 * R21 + L21 * R22;
        Col2[2]         = L02 * R20 + L12 * R21 + L22 * R22;
    }

    AN_FORCEINLINE Float3x3 ViewInverseFast() const
    {
        return Transposed();
    }

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const
    {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream )
    {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
    }

    // Static methods

    static Float3x3 const & Identity()
    {
        static constexpr Float3x3 IdentityMat( 1 );
        return IdentityMat;
    }
};

// Column-major matrix 4x4
struct Float4x4
{
    Float4 Col0;
    Float4 Col1;
    Float4 Col2;
    Float4 Col3;

    Float4x4() = default;
    explicit Float4x4( Float2x2 const & _Value );
    explicit Float4x4( Float3x3 const & _Value );
    explicit Float4x4( Float3x4 const & _Value );
    constexpr Float4x4( Float4 const & _Col0, Float4 const & _Col1, Float4 const & _Col2, Float4 const & _Col3 ) :
        Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ), Col3( _Col3 ) {}
    constexpr Float4x4( float _00, float _01, float _02, float _03, float _10, float _11, float _12, float _13, float _20, float _21, float _22, float _23, float _30, float _31, float _32, float _33 ) :
        Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ), Col3( _30, _31, _32, _33 ) {}
    constexpr explicit Float4x4( float _Diagonal ) :
        Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ), Col3( 0, 0, 0, _Diagonal ) {}
    constexpr explicit Float4x4( Float4 const & _Diagonal ) :
        Col0( _Diagonal.X, 0, 0, 0 ), Col1( 0, _Diagonal.Y, 0, 0 ), Col2( 0, 0, _Diagonal.Z, 0 ), Col3( 0, 0, 0, _Diagonal.W ) {}

    float * ToPtr()
    {
        return &Col0.X;
    }

    const float * ToPtr() const
    {
        return &Col0.X;
    }

    Float4 & operator[]( int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float4 const & operator[]( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float4 GetRow( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return Float4( Col0[_Index], Col1[_Index], Col2[_Index], Col3[_Index] );
    }

    bool operator==( Float4x4 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float4x4 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float4x4 const & _Other ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 16; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float4x4 const & _Other, float _Epsilon ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 16; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void TransposeSelf()
    {
        float Temp = Col0.Y;
        Col0.Y     = Col1.X;
        Col1.X     = Temp;

        Temp   = Col0.Z;
        Col0.Z = Col2.X;
        Col2.X = Temp;

        Temp   = Col1.Z;
        Col1.Z = Col2.Y;
        Col2.Y = Temp;

        Temp   = Col0.W;
        Col0.W = Col3.X;
        Col3.X = Temp;

        Temp   = Col1.W;
        Col1.W = Col3.Y;
        Col3.Y = Temp;

        Temp   = Col2.W;
        Col2.W = Col3.Z;
        Col3.Z = Temp;
    }

    Float4x4 Transposed() const
    {
        return Float4x4( Col0.X, Col1.X, Col2.X, Col3.X,
                         Col0.Y, Col1.Y, Col2.Y, Col3.Y,
                         Col0.Z, Col1.Z, Col2.Z, Col3.Z,
                         Col0.W, Col1.W, Col2.W, Col3.W );
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float4x4 Inversed() const
    {
        Float4x4 const & m = *this;

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

        const Float4 Fac0( Coef00, Coef00, Coef02, Coef03 );
        const Float4 Fac1( Coef04, Coef04, Coef06, Coef07 );
        const Float4 Fac2( Coef08, Coef08, Coef10, Coef11 );
        const Float4 Fac3( Coef12, Coef12, Coef14, Coef15 );
        const Float4 Fac4( Coef16, Coef16, Coef18, Coef19 );
        const Float4 Fac5( Coef20, Coef20, Coef22, Coef23 );

        const Float4 Vec0( m[1][0], m[0][0], m[0][0], m[0][0] );
        const Float4 Vec1( m[1][1], m[0][1], m[0][1], m[0][1] );
        const Float4 Vec2( m[1][2], m[0][2], m[0][2], m[0][2] );
        const Float4 Vec3( m[1][3], m[0][3], m[0][3], m[0][3] );

        const Float4 Inv0( Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2 );
        const Float4 Inv1( Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4 );
        const Float4 Inv2( Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5 );
        const Float4 Inv3( Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5 );

        const Float4 SignA( +1, -1, +1, -1 );
        const Float4 SignB( -1, +1, -1, +1 );
        Float4x4     Inversed( Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB );

        const Float4 Row0( Inversed[0][0], Inversed[1][0], Inversed[2][0], Inversed[3][0] );

        const Float4 Dot0( m[0] * Row0 );
        const float  Dot1 = ( Dot0.X + Dot0.Y ) + ( Dot0.Z + Dot0.W );

        const float OneOverDeterminant = 1.0f / Dot1;

        return Inversed * OneOverDeterminant;
    }

    float Determinant() const
    {
        const float SubFactor00 = Col2[2] * Col3[3] - Col3[2] * Col2[3];
        const float SubFactor01 = Col2[1] * Col3[3] - Col3[1] * Col2[3];
        const float SubFactor02 = Col2[1] * Col3[2] - Col3[1] * Col2[2];
        const float SubFactor03 = Col2[0] * Col3[3] - Col3[0] * Col2[3];
        const float SubFactor04 = Col2[0] * Col3[2] - Col3[0] * Col2[2];
        const float SubFactor05 = Col2[0] * Col3[1] - Col3[0] * Col2[1];

        const Float4 DetCof(
            +( Col1[1] * SubFactor00 - Col1[2] * SubFactor01 + Col1[3] * SubFactor02 ),
            -( Col1[0] * SubFactor00 - Col1[2] * SubFactor03 + Col1[3] * SubFactor04 ),
            +( Col1[0] * SubFactor01 - Col1[1] * SubFactor03 + Col1[3] * SubFactor05 ),
            -( Col1[0] * SubFactor02 - Col1[1] * SubFactor04 + Col1[2] * SubFactor05 ) );

        return Col0[0] * DetCof[0] + Col0[1] * DetCof[1] +
            Col0[2] * DetCof[2] + Col0[3] * DetCof[3];
    }

    void Clear()
    {
        Core::ZeroMem( this, sizeof( *this ) );
    }

    void SetIdentity()
    {
        *this = Identity();
    }

    static Float4x4 Translation( Float3 const & _Vec )
    {
        return Float4x4( Float4( 1, 0, 0, 0 ),
                         Float4( 0, 1, 0, 0 ),
                         Float4( 0, 0, 1, 0 ),
                         Float4( _Vec[0], _Vec[1], _Vec[2], 1 ) );
    }

    Float4x4 Translated( Float3 const & _Vec ) const
    {
        return Float4x4( Col0, Col1, Col2, Col0 * _Vec[0] + Col1 * _Vec[1] + Col2 * _Vec[2] + Col3 );
    }

    static Float4x4 Scale( Float3 const & _Scale )
    {
        return Float4x4( Float4( _Scale[0], 0, 0, 0 ),
                         Float4( 0, _Scale[1], 0, 0 ),
                         Float4( 0, 0, _Scale[2], 0 ),
                         Float4( 0, 0, 0, 1 ) );
    }

    Float4x4 Scaled( Float3 const & _Scale ) const
    {
        return Float4x4( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2], Col3 );
    }


    // Return rotation around normalized axis
    static Float4x4 RotationAroundNormal( float _AngleRad, Float3 const & _Normal )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        const Float3 Temp  = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float4x4( c + Temp[0] * _Normal[0], Temp[0] * _Normal[1] + Temp2[2], Temp[0] * _Normal[2] - Temp2[1], 0,
                         Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1], Temp[1] * _Normal[2] + Temp2[0], 0,
                         Temp[2] * _Normal[0] + Temp2[1], Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2], 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around normalized axis
    Float4x4 RotateAroundNormal( float _AngleRad, Float3 const & _Normal ) const
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        Float3 Temp  = ( 1.0f - c ) * _Normal;
        Float3 Temp2 = s * _Normal;
        return Float4x4( Col0 * ( c + Temp[0] * _Normal[0] ) + Col1 * ( Temp[0] * _Normal[1] + Temp2[2] ) + Col2 * ( Temp[0] * _Normal[2] - Temp2[1] ),
                         Col0 * ( Temp[1] * _Normal[0] - Temp2[2] ) + Col1 * ( c + Temp[1] * _Normal[1] ) + Col2 * ( Temp[1] * _Normal[2] + Temp2[0] ),
                         Col0 * ( Temp[2] * _Normal[0] + Temp2[1] ) + Col1 * ( Temp[2] * _Normal[1] - Temp2[0] ) + Col2 * ( c + Temp[2] * _Normal[2] ),
                         Col3 );
    }

    // Return rotation around unnormalized vector
    static Float4x4 RotationAroundVector( float _AngleRad, Float3 const & _Vector )
    {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Float4x4 RotateAroundVector( float _AngleRad, Float3 const & _Vector ) const
    {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float4x4 RotationX( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float4x4( 1, 0, 0, 0,
                         0, c, s, 0,
                         0, -s, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Y axis
    static Float4x4 RotationY( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float4x4( c, 0, -s, 0,
                         0, 1, 0, 0,
                         s, 0, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Z axis
    static Float4x4 RotationZ( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float4x4( c, s, 0, 0,
                         -s, c, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1 );
    }

    template< typename T >
    TVector4< T > operator*( TVector4< T > const & _Vec ) const
    {
        return TVector4< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z + Col3[0] * _Vec.W,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z + Col3[1] * _Vec.W,
                              Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z + Col3[2] * _Vec.W,
                              Col0[3] * _Vec.X + Col1[3] * _Vec.Y + Col2[3] * _Vec.Z + Col3[3] * _Vec.W );
    }

    // Assume _Vec.W = 1
    template< typename T >
    TVector4< T > operator*( TVector3< T > const & _Vec ) const
    {
        return TVector4< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z + Col3[0],
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z + Col3[1],
                              Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z + Col3[2],
                              Col0[3] * _Vec.X + Col1[3] * _Vec.Y + Col2[3] * _Vec.Z + Col3[3] );
    }

    // Same as Float3x3(*this)*_Vec
    template< typename T >
    TVector3< T > TransformAsFloat3x3( TVector3< T > const & _Vec ) const
    {
        return TVector3< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z,
                              Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z );
    }

    // Same as Float3x3(*this)*_Mat
    Float3x3 TransformAsFloat3x3( Float3x3 const & _Mat ) const
    {
        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L02 = Col0[2];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float L12 = Col1[2];
        const float L20 = Col2[0];
        const float L21 = Col2[1];
        const float L22 = Col2[2];
        const float R00 = _Mat[0][0];
        const float R01 = _Mat[0][1];
        const float R02 = _Mat[0][2];
        const float R10 = _Mat[1][0];
        const float R11 = _Mat[1][1];
        const float R12 = _Mat[1][2];
        const float R20 = _Mat[2][0];
        const float R21 = _Mat[2][1];
        const float R22 = _Mat[2][2];
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

    Float4x4 operator*( float _Value ) const
    {
        return Float4x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value,
                         Col3 * _Value );
    }

    void operator*=( float _Value )
    {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
        Col3 *= _Value;
    }

    Float4x4 operator/( float _Value ) const
    {
        const float OneOverValue = 1.0f / _Value;
        return Float4x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue,
                         Col3 * OneOverValue );
    }

    void operator/=( float _Value )
    {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
        Col3 *= OneOverValue;
    }

    Float4x4 operator*( Float4x4 const & _Mat ) const
    {
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
        const float R00 = _Mat[0][0];
        const float R01 = _Mat[0][1];
        const float R02 = _Mat[0][2];
        const float R03 = _Mat[0][3];
        const float R10 = _Mat[1][0];
        const float R11 = _Mat[1][1];
        const float R12 = _Mat[1][2];
        const float R13 = _Mat[1][3];
        const float R20 = _Mat[2][0];
        const float R21 = _Mat[2][1];
        const float R22 = _Mat[2][2];
        const float R23 = _Mat[2][3];
        const float R30 = _Mat[3][0];
        const float R31 = _Mat[3][1];
        const float R32 = _Mat[3][2];
        const float R33 = _Mat[3][3];
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

    void operator*=( Float4x4 const & _Mat )
    {
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
        const float R00 = _Mat[0][0];
        const float R01 = _Mat[0][1];
        const float R02 = _Mat[0][2];
        const float R03 = _Mat[0][3];
        const float R10 = _Mat[1][0];
        const float R11 = _Mat[1][1];
        const float R12 = _Mat[1][2];
        const float R13 = _Mat[1][3];
        const float R20 = _Mat[2][0];
        const float R21 = _Mat[2][1];
        const float R22 = _Mat[2][2];
        const float R23 = _Mat[2][3];
        const float R30 = _Mat[3][0];
        const float R31 = _Mat[3][1];
        const float R32 = _Mat[3][2];
        const float R33 = _Mat[3][3];
        Col0[0]         = L00 * R00 + L10 * R01 + L20 * R02 + L30 * R03,
        Col0[1]         = L01 * R00 + L11 * R01 + L21 * R02 + L31 * R03,
        Col0[2]         = L02 * R00 + L12 * R01 + L22 * R02 + L32 * R03,
        Col0[3]         = L03 * R00 + L13 * R01 + L23 * R02 + L33 * R03,
        Col1[0]         = L00 * R10 + L10 * R11 + L20 * R12 + L30 * R13,
        Col1[1]         = L01 * R10 + L11 * R11 + L21 * R12 + L31 * R13,
        Col1[2]         = L02 * R10 + L12 * R11 + L22 * R12 + L32 * R13,
        Col1[3]         = L03 * R10 + L13 * R11 + L23 * R12 + L33 * R13,
        Col2[0]         = L00 * R20 + L10 * R21 + L20 * R22 + L30 * R23,
        Col2[1]         = L01 * R20 + L11 * R21 + L21 * R22 + L31 * R23,
        Col2[2]         = L02 * R20 + L12 * R21 + L22 * R22 + L32 * R23,
        Col2[3]         = L03 * R20 + L13 * R21 + L23 * R22 + L33 * R23,
        Col3[0]         = L00 * R30 + L10 * R31 + L20 * R32 + L30 * R33,
        Col3[1]         = L01 * R30 + L11 * R31 + L21 * R32 + L31 * R33,
        Col3[2]         = L02 * R30 + L12 * R31 + L22 * R32 + L32 * R33,
        Col3[3]         = L03 * R30 + L13 * R31 + L23 * R32 + L33 * R33;
    }

    Float4x4 operator*( Float3x4 const & _Mat ) const;

    void operator*=( Float3x4 const & _Mat );

    Float4x4 ViewInverseFast() const
    {
        Float4x4 Inversed;

        float *       DstPtr = Inversed.ToPtr();
        const float * SrcPtr = ToPtr();

        DstPtr[0] = SrcPtr[0];
        DstPtr[1] = SrcPtr[4];
        DstPtr[2] = SrcPtr[8];
        DstPtr[3] = 0;

        DstPtr[4] = SrcPtr[1];
        DstPtr[5] = SrcPtr[5];
        DstPtr[6] = SrcPtr[9];
        DstPtr[7] = 0;

        DstPtr[8]  = SrcPtr[2];
        DstPtr[9]  = SrcPtr[6];
        DstPtr[10] = SrcPtr[10];
        DstPtr[11] = 0;

        DstPtr[12] = -( DstPtr[0] * SrcPtr[12] + DstPtr[4] * SrcPtr[13] + DstPtr[8] * SrcPtr[14] );
        DstPtr[13] = -( DstPtr[1] * SrcPtr[12] + DstPtr[5] * SrcPtr[13] + DstPtr[9] * SrcPtr[14] );
        DstPtr[14] = -( DstPtr[2] * SrcPtr[12] + DstPtr[6] * SrcPtr[13] + DstPtr[10] * SrcPtr[14] );
        DstPtr[15] = 1;

        return Inversed;
    }

    AN_FORCEINLINE Float4x4 PerspectiveProjectionInverseFast() const
    {
        Float4x4 Inversed;

        // TODO: check correctness for all perspective projections

        float *       DstPtr = Inversed.ToPtr();
        const float * SrcPtr = ToPtr();

        DstPtr[0] = 1.0f / SrcPtr[0];
        DstPtr[1] = 0;
        DstPtr[2] = 0;
        DstPtr[3] = 0;

        DstPtr[4] = 0;
        DstPtr[5] = 1.0f / SrcPtr[5];
        DstPtr[6] = 0;
        DstPtr[7] = 0;

        DstPtr[8]  = 0;
        DstPtr[9]  = 0;
        DstPtr[10] = 0;
        DstPtr[11] = 1.0f / SrcPtr[14];

        DstPtr[12] = 0;
        DstPtr[13] = 0;
        DstPtr[14] = 1.0f / SrcPtr[11];
        DstPtr[15] = -SrcPtr[10] / ( SrcPtr[11] * SrcPtr[14] );

        return Inversed;
    }

    AN_FORCEINLINE Float4x4 OrthoProjectionInverseFast() const
    {
        // TODO: ...
        return Inversed();
    }

    // String conversions
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const
    {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " " + Col3.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " " + Col3.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
        Col3.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream )
    {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
        Col3.Read( _Stream );
    }

    // Static methods

    static Float4x4 const & Identity()
    {
        static constexpr Float4x4 IdentityMat( 1 );
        return IdentityMat;
    }

    static AN_FORCEINLINE Float4x4 LookAt( Float3 const & eye, Float3 const & center, Float3 const & up )
    {
        Float3 const f( ( center - eye ).Normalized() );
        Float3 const s( Math::Cross( up, f ).Normalized() );
        Float3 const u( Math::Cross( f, s ) );

        Float4x4 result;
        result[0][0] = s.X;
        result[1][0] = s.Y;
        result[2][0] = s.Z;
        result[3][0] = -Math::Dot( s, eye );

        result[0][1] = u.X;
        result[1][1] = u.Y;
        result[2][1] = u.Z;
        result[3][1] = -Math::Dot( u, eye );

        result[0][2] = f.X;
        result[1][2] = f.Y;
        result[2][2] = f.Z;
        result[3][2] = -Math::Dot( f, eye );

        result[0][3] = 0;
        result[1][3] = 0;
        result[2][3] = 0;
        result[3][3] = 1;

        return result;
    }

    // Conversion from standard projection matrix to clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 const & ClipControl_UpperLeft_ZeroToOne()
    {
        static constexpr float ClipTransform[] = {
            1, 0, 0, 0,
            0, -1, 0, 0,
            0, 0, 0.5f, 0,
            0, 0, 0.5f, 1 };

        return *reinterpret_cast< Float4x4 const * >( &ClipTransform[0] );

        // Same
        //return Float4x4::Identity().Scaled(Float3(1.0f,1.0f,0.5f)).Translated(Float3(0,0,0.5)).Scaled(Float3(1,-1,1));
    }

    // Standard OpenGL ortho projection for 2D
    static AN_FORCEINLINE Float4x4 Ortho2D( Float2 const & _Mins, Float2 const & _Maxs )
    {
        const float InvX = 1.0f / ( _Maxs.X - _Mins.X );
        const float InvY = 1.0f / ( _Maxs.Y - _Mins.Y );
        const float tx   = -( _Maxs.X + _Mins.X ) * InvX;
        const float ty   = -( _Maxs.Y + _Mins.Y ) * InvY;
        return Float4x4( 2 * InvX, 0, 0, 0,
                         0, 2 * InvY, 0, 0,
                         0, 0, -2, 0,
                         tx, ty, -1, 1 );
    }

    // OpenGL ortho projection for 2D with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 Ortho2DCC( Float2 const & _Mins, Float2 const & _Maxs )
    {
        return ClipControl_UpperLeft_ZeroToOne() * Ortho2D( _Mins, _Maxs );
    }

    // Standard OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 Ortho( Float2 const & _Mins, Float2 const & _Maxs, float _ZNear, float _ZFar )
    {
        const float InvX = 1.0f / ( _Maxs.X - _Mins.X );
        const float InvY = 1.0f / ( _Maxs.Y - _Mins.Y );
        const float InvZ = 1.0f / ( _ZFar - _ZNear );
        const float tx   = -( _Maxs.X + _Mins.X ) * InvX;
        const float ty   = -( _Maxs.Y + _Mins.Y ) * InvY;
        const float tz   = -( _ZFar + _ZNear ) * InvZ;
        return Float4x4( 2 * InvX, 0, 0, 0,
                         0, 2 * InvY, 0, 0,
                         0, 0, -2 * InvZ, 0,
                         tx, ty, tz, 1 );
    }

    // OpenGL ortho projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 OrthoCC( Float2 const & _Mins, Float2 const & _Maxs, float _ZNear, float _ZFar )
    {
        const float InvX = 1.0 / ( _Maxs.X - _Mins.X );
        const float InvY = 1.0 / ( _Maxs.Y - _Mins.Y );
        const float InvZ = 1.0 / ( _ZFar - _ZNear );
        const float tx   = -( _Maxs.X + _Mins.X ) * InvX;
        const float ty   = -( _Maxs.Y + _Mins.Y ) * InvY;
        const float tz   = -( _ZFar + _ZNear ) * InvZ;
        return Float4x4( 2 * InvX, 0, 0, 0,
                         0, -2 * InvY, 0, 0,
                         0, 0, -InvZ, 0,
                         tx, -ty, tz * 0.5 + 0.5, 1 );
        // Same
        // Transform according to clip control
        //return ClipControl_UpperLeft_ZeroToOne() * Ortho( _Mins, _Maxs, _ZNear, _ZFar );
    }

    // Reversed-depth OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 OrthoRev( Float2 const & _Mins, Float2 const & _Maxs, float _ZNear, float _ZFar )
    {
        const float InvX = 1.0f / ( _Maxs.X - _Mins.X );
        const float InvY = 1.0f / ( _Maxs.Y - _Mins.Y );
        const float InvZ = 1.0f / ( _ZNear - _ZFar );
        const float tx   = -( _Maxs.X + _Mins.X ) * InvX;
        const float ty   = -( _Maxs.Y + _Mins.Y ) * InvY;
        const float tz   = -( _ZNear + _ZFar ) * InvZ;
        return Float4x4( 2 * InvX, 0, 0, 0,
                         0, 2 * InvY, 0, 0,
                         0, 0, -2 * InvZ, 0,
                         tx, ty, tz, 1 );
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 OrthoRevCC( Float2 const & _Mins, Float2 const & _Maxs, float _ZNear, float _ZFar )
    {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * OrthoRev( _Mins, _Maxs, _ZNear, _ZFar );
    }

    // Standard OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 Perspective( float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar )
    {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY    = (float)( atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX, 0, 0, 0,
                         0, 1 / TanHalfFovY, 0, 0,
                         0, 0, ( _ZFar + _ZNear ) / ( _ZNear - _ZFar ), -1,
                         0, 0, 2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    static AN_FORCEINLINE Float4x4 Perspective( float _FovXRad, float _FovYRad, float _ZNear, float _ZFar )
    {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX, 0, 0, 0,
                         0, 1 / TanHalfFovY, 0, 0,
                         0, 0, ( _ZFar + _ZNear ) / ( _ZNear - _ZFar ), -1,
                         0, 0, 2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    // OpenGL perspective projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 PerspectiveCC( float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar )
    {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _Width, _Height, _ZNear, _ZFar );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveCC( float _FovXRad, float _FovYRad, float _ZNear, float _ZFar )
    {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _FovYRad, _ZNear, _ZFar );
    }

    // Reversed-depth OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRev( float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar )
    {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY    = (float)( atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX, 0, 0, 0,
                         0, 1 / TanHalfFovY, 0, 0,
                         0, 0, ( _ZNear + _ZFar ) / ( _ZFar - _ZNear ), -1,
                         0, 0, 2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRev( float _FovXRad, float _FovYRad, float _ZNear, float _ZFar )
    {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX, 0, 0, 0,
                         0, 1 / TanHalfFovY, 0, 0,
                         0, 0, ( _ZNear + _ZFar ) / ( _ZFar - _ZNear ), -1,
                         0, 0, 2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRevCC( float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar )
    {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY    = (float)( atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX, 0, 0, 0,
                         0, -1 / TanHalfFovY, 0, 0,
                         0, 0, _ZNear / ( _ZFar - _ZNear ), -1,
                         0, 0, _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRevCC_Y( float _FovYRad, float _Width, float _Height, float _ZNear, float _ZFar )
    {
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        const float HalfFovX    = atan2( TanHalfFovY * _Width, _Height );
        const float TanHalfFovX = tan( HalfFovX );
        return Float4x4( 1 / TanHalfFovX, 0, 0, 0,
                         0, -1 / TanHalfFovY, 0, 0,
                         0, 0, _ZNear / ( _ZFar - _ZNear ), -1,
                         0, 0, _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRevCC( float _FovXRad, float _FovYRad, float _ZNear, float _ZFar )
    {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX, 0, 0, 0,
                         0, -1 / TanHalfFovY, 0, 0,
                         0, 0, _ZNear / ( _ZFar - _ZNear ), -1,
                         0, 0, _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    static AN_INLINE void GetCubeFaceMatrices( Float4x4 & _PositiveX,
                                               Float4x4 & _NegativeX,
                                               Float4x4 & _PositiveY,
                                               Float4x4 & _NegativeY,
                                               Float4x4 & _PositiveZ,
                                               Float4x4 & _NegativeZ )
    {
        _PositiveX = Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( Math::_HALF_PI, Float3( 0, 1, 0 ) );
        _NegativeX = Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( -Math::_HALF_PI, Float3( 0, 1, 0 ) );
        _PositiveY = Float4x4::RotationX( -Math::_HALF_PI );
        _NegativeY = Float4x4::RotationX( Math::_HALF_PI );
        _PositiveZ = Float4x4::RotationX( Math::_PI );
        _NegativeZ = Float4x4::RotationZ( Math::_PI );
    }

    static AN_INLINE Float4x4 const * GetCubeFaceMatrices()
    {
        // TODO: Precompute this matrices
        static Float4x4 const CubeFaceMatrices[6] = {
            Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( Math::_HALF_PI, Float3( 0, 1, 0 ) ),
            Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( -Math::_HALF_PI, Float3( 0, 1, 0 ) ),
            Float4x4::RotationX( -Math::_HALF_PI ),
            Float4x4::RotationX( Math::_HALF_PI ),
            Float4x4::RotationX( Math::_PI ),
            Float4x4::RotationZ( Math::_PI ) };
        return CubeFaceMatrices;
    }
};

// Column-major matrix 3x4
// Keep transformations transposed
struct Float3x4
{
    Float4 Col0;
    Float4 Col1;
    Float4 Col2;

    Float3x4() = default;
    explicit Float3x4( Float2x2 const & _Value );
    explicit Float3x4( Float3x3 const & _Value );
    explicit Float3x4( Float4x4 const & _Value );
    constexpr Float3x4( Float4 const & _Col0, Float4 const & _Col1, Float4 const & _Col2 ) :
        Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Float3x4( float _00, float _01, float _02, float _03, float _10, float _11, float _12, float _13, float _20, float _21, float _22, float _23 ) :
        Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ) {}
    constexpr explicit Float3x4( float _Diagonal ) :
        Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ) {}
    constexpr explicit Float3x4( Float3 const & _Diagonal ) :
        Col0( _Diagonal.X, 0, 0, 0 ), Col1( 0, _Diagonal.Y, 0, 0 ), Col2( 0, 0, _Diagonal.Z, 0 ) {}

    float * ToPtr()
    {
        return &Col0.X;
    }

    const float * ToPtr() const
    {
        return &Col0.X;
    }

    Float4 & operator[]( int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float4 const & operator[]( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return ( &Col0 )[_Index];
    }

    Float3 GetRow( int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return Float3( Col0[_Index], Col1[_Index], Col2[_Index] );
    }

    bool operator==( Float3x4 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float3x4 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float3x4 const & _Other ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 12; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float3x4 const & _Other, float _Epsilon ) const
    {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0; i < 12; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void Compose( Float3 const & _Translation, Float3x3 const & _Rotation, Float3 const & _Scale )
    {
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

    void Compose( Float3 const & _Translation, Float3x3 const & _Rotation )
    {
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

    void SetTranslation( Float3 const & _Translation )
    {
        Col0[3] = _Translation.X;
        Col1[3] = _Translation.Y;
        Col2[3] = _Translation.Z;
    }

    void DecomposeAll( Float3 & _Translation, Float3x3 & _Rotation, Float3 & _Scale ) const
    {
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

    Float3 DecomposeTranslation() const
    {
        return Float3( Col0[3], Col1[3], Col2[3] );
    }

    Float3x3 DecomposeRotation() const
    {
        return Float3x3( Float3( Col0[0], Col1[0], Col2[0] ) / Float3( Col0[0], Col1[0], Col2[0] ).Length(),
                         Float3( Col0[1], Col1[1], Col2[1] ) / Float3( Col0[1], Col1[1], Col2[1] ).Length(),
                         Float3( Col0[2], Col1[2], Col2[2] ) / Float3( Col0[2], Col1[2], Col2[2] ).Length() );
    }

    Float3 DecomposeScale() const
    {
        return Float3( Float3( Col0[0], Col1[0], Col2[0] ).Length(),
                       Float3( Col0[1], Col1[1], Col2[1] ).Length(),
                       Float3( Col0[2], Col1[2], Col2[2] ).Length() );
    }

    void DecomposeRotationAndScale( Float3x3 & _Rotation, Float3 & _Scale ) const
    {
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

    void DecomposeNormalMatrix( Float3x3 & _NormalMatrix ) const
    {
        Float3x4 const & m = *this;

        const float Determinant = m[0][0] * m[1][1] * m[2][2] +
            m[1][0] * m[2][1] * m[0][2] +
            m[2][0] * m[0][1] * m[1][2] -
            m[2][0] * m[1][1] * m[0][2] -
            m[1][0] * m[0][1] * m[2][2] -
            m[0][0] * m[2][1] * m[1][2];

        const float OneOverDeterminant = 1.0f / Determinant;

        _NormalMatrix[0][0] = ( m[1][1] * m[2][2] - m[2][1] * m[1][2] ) * OneOverDeterminant;
        _NormalMatrix[0][1] = -( m[0][1] * m[2][2] - m[2][1] * m[0][2] ) * OneOverDeterminant;
        _NormalMatrix[0][2] = ( m[0][1] * m[1][2] - m[1][1] * m[0][2] ) * OneOverDeterminant;

        _NormalMatrix[1][0] = -( m[1][0] * m[2][2] - m[2][0] * m[1][2] ) * OneOverDeterminant;
        _NormalMatrix[1][1] = ( m[0][0] * m[2][2] - m[2][0] * m[0][2] ) * OneOverDeterminant;
        _NormalMatrix[1][2] = -( m[0][0] * m[1][2] - m[1][0] * m[0][2] ) * OneOverDeterminant;

        _NormalMatrix[2][0] = ( m[1][0] * m[2][1] - m[2][0] * m[1][1] ) * OneOverDeterminant;
        _NormalMatrix[2][1] = -( m[0][0] * m[2][1] - m[2][0] * m[0][1] ) * OneOverDeterminant;
        _NormalMatrix[2][2] = ( m[0][0] * m[1][1] - m[1][0] * m[0][1] ) * OneOverDeterminant;

        // Or
        //_NormalMatrix = Float3x3(Inversed());
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float3x4 Inversed() const
    {
        Float3x4 const & m = *this;

        const float Determinant = m[0][0] * m[1][1] * m[2][2] +
            m[1][0] * m[2][1] * m[0][2] +
            m[2][0] * m[0][1] * m[1][2] -
            m[2][0] * m[1][1] * m[0][2] -
            m[1][0] * m[0][1] * m[2][2] -
            m[0][0] * m[2][1] * m[1][2];

        const float OneOverDeterminant = 1.0f / Determinant;
        Float3x4    Result;

        Result[0][0] = ( m[1][1] * m[2][2] - m[2][1] * m[1][2] ) * OneOverDeterminant;
        Result[0][1] = -( m[0][1] * m[2][2] - m[2][1] * m[0][2] ) * OneOverDeterminant;
        Result[0][2] = ( m[0][1] * m[1][2] - m[1][1] * m[0][2] ) * OneOverDeterminant;
        Result[0][3] = -( m[0][3] * Result[0][0] + m[1][3] * Result[0][1] + m[2][3] * Result[0][2] );

        Result[1][0] = -( m[1][0] * m[2][2] - m[2][0] * m[1][2] ) * OneOverDeterminant;
        Result[1][1] = ( m[0][0] * m[2][2] - m[2][0] * m[0][2] ) * OneOverDeterminant;
        Result[1][2] = -( m[0][0] * m[1][2] - m[1][0] * m[0][2] ) * OneOverDeterminant;
        Result[1][3] = -( m[0][3] * Result[1][0] + m[1][3] * Result[1][1] + m[2][3] * Result[1][2] );

        Result[2][0] = ( m[1][0] * m[2][1] - m[2][0] * m[1][1] ) * OneOverDeterminant;
        Result[2][1] = -( m[0][0] * m[2][1] - m[2][0] * m[0][1] ) * OneOverDeterminant;
        Result[2][2] = ( m[0][0] * m[1][1] - m[1][0] * m[0][1] ) * OneOverDeterminant;
        Result[2][3] = -( m[0][3] * Result[2][0] + m[1][3] * Result[2][1] + m[2][3] * Result[2][2] );

        return Result;
    }

    float Determinant() const
    {
        return Col0[0] * ( Col1[1] * Col2[2] - Col2[1] * Col1[2] ) +
            Col1[0] * ( Col2[1] * Col0[2] - Col0[1] * Col2[2] ) +
            Col2[0] * ( Col0[1] * Col1[2] - Col1[1] * Col0[2] );
    }

    void Clear()
    {
        Core::ZeroMem( this, sizeof( *this ) );
    }

    void SetIdentity()
    {
        *this = Identity();
    }

    static Float3x4 Translation( Float3 const & _Vec )
    {
        return Float3x4( Float4( 1, 0, 0, _Vec[0] ),
                         Float4( 0, 1, 0, _Vec[1] ),
                         Float4( 0, 0, 1, _Vec[2] ) );
    }

    static Float3x4 Scale( Float3 const & _Scale )
    {
        return Float3x4( Float4( _Scale[0], 0, 0, 0 ),
                         Float4( 0, _Scale[1], 0, 0 ),
                         Float4( 0, 0, _Scale[2], 0 ) );
    }

    // Return rotation around normalized axis
    static Float3x4 RotationAroundNormal( float _AngleRad, Float3 const & _Normal )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        const Float3 Temp  = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float3x4( c + Temp[0] * _Normal[0], Temp[1] * _Normal[0] - Temp2[2], Temp[2] * _Normal[0] + Temp2[1], 0,
                         Temp[0] * _Normal[1] + Temp2[2], c + Temp[1] * _Normal[1], Temp[2] * _Normal[1] - Temp2[0], 0,
                         Temp[0] * _Normal[2] - Temp2[1], Temp[1] * _Normal[2] + Temp2[0], c + Temp[2] * _Normal[2], 0 );
    }

    // Return rotation around unnormalized vector
    static Float3x4 RotationAroundVector( float _AngleRad, Float3 const & _Vector )
    {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float3x4 RotationX( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x4( 1, 0, 0, 0,
                         0, c, -s, 0,
                         0, s, c, 0 );
    }

    // Return rotation around Y axis
    static Float3x4 RotationY( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x4( c, 0, s, 0,
                         0, 1, 0, 0,
                         -s, 0, c, 0 );
    }

    // Return rotation around Z axis
    static Float3x4 RotationZ( float _AngleRad )
    {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x4( c, -s, 0, 0,
                         s, c, 0, 0,
                         0, 0, 1, 0 );
    }

    // Assume _Vec.W = 1
    template< typename T >
    TVector3< T > operator*( TVector3< T > const & _Vec ) const
    {
        return TVector3< T >( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[2] * _Vec.Z + Col0[3],
                              Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[2] * _Vec.Z + Col1[3],
                              Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[2] * _Vec.Z + Col2[3] );
    }

    // Assume _Vec.Z = 0, _Vec.W = 1
    template< typename T >
    TVector3< T > operator*( TVector2< T > const & _Vec ) const
    {
        return TVector3< T >( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                              Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3],
                              Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[3] );
    }

    template< typename T >
    TVector2< T > Mult_Vec2_IgnoreZ( TVector2< T > const & _Vec )
    {
        return TVector2< T >( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                              Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3] );
    }

    Float3x4 operator*( float _Value ) const
    {
        return Float3x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( float _Value )
    {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Float3x4 operator/( float _Value ) const
    {
        const float OneOverValue = 1.0f / _Value;
        return Float3x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue );
    }

    void operator/=( float _Value )
    {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    Float3x4 operator*( Float3x4 const & _Mat ) const
    {
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

    void operator*=( Float3x4 const & _Mat )
    {
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
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const
    {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream )
    {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
    }

    // Static methods

    static Float3x4 const & Identity()
    {
        static constexpr Float3x4 IdentityMat( 1 );
        return IdentityMat;
    }
};

// Type conversion

// Float3x3 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( Float3x3 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float3x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( Float3x4 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float4x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( Float4x4 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float2x2 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( Float2x2 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0, 0, 1 ) {}

// Float3x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( Float3x4 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float4x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( Float4x4 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float2x2 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( Float2x2 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0, 0, 1, 0 ), Col3( 0, 0, 0, 1 ) {}

// Float3x3 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( Float3x3 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0, 0, 0, 1 ) {}

// Float3x4 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( Float3x4 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0, 0, 0, 1 ) {}

// Float2x2 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( Float2x2 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0.0f ) {}

// Float3x3 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( Float3x3 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float4x4 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( Float4x4 const & _Value ) :
    Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}


template< typename T >
AN_FORCEINLINE TVector2< T > operator*( TVector2< T > const & _Vec, Float2x2 const & _Mat )
{
    return TVector2< T >( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y,
                          _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y );
}

template< typename T >
AN_FORCEINLINE TVector3< T > operator*( TVector3< T > const & _Vec, Float3x3 const & _Mat )
{
    return TVector3< T >( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z,
                          _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z,
                          _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z );
}

template< typename T >
AN_FORCEINLINE TVector4< T > operator*( TVector4< T > const & _Vec, Float4x4 const & _Mat )
{
    return TVector4< T >( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z + _Mat[0][3] * _Vec.W,
                          _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z + _Mat[1][3] * _Vec.W,
                          _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z + _Mat[2][3] * _Vec.W,
                          _Mat[3][0] * _Vec.X + _Mat[3][1] * _Vec.Y + _Mat[3][2] * _Vec.Z + _Mat[3][3] * _Vec.W );
}

AN_FORCEINLINE Float4x4 Float4x4::operator*( Float3x4 const & _Mat ) const
{
    Float4 const & SrcB0 = _Mat.Col0;
    Float4 const & SrcB1 = _Mat.Col1;
    Float4 const & SrcB2 = _Mat.Col2;

    return Float4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                     Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                     Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                     Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

AN_FORCEINLINE void Float4x4::operator*=( Float3x4 const & _Mat )
{
    Float4 const & SrcB0 = _Mat.Col0;
    Float4 const & SrcB1 = _Mat.Col1;
    Float4 const & SrcB2 = _Mat.Col2;

    *this = Float4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                      Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                      Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                      Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

namespace Math
{

AN_INLINE bool Unproject( Float4x4 const & _ModelViewProjectionInversed, const float _Viewport[4], Float3 const & _Coord, Float3 & _Result )
{
    Float4 In( _Coord, 1.0f );

    // Map x and y from window coordinates
    In.X = ( In.X - _Viewport[0] ) / _Viewport[2];
    In.Y = ( In.Y - _Viewport[1] ) / _Viewport[3];

    // Map to range -1 to 1
    In.X = In.X * 2.0f - 1.0f;
    In.Y = In.Y * 2.0f - 1.0f;
    In.Z = In.Z * 2.0f - 1.0f;

    _Result.X       = _ModelViewProjectionInversed[0][0] * In[0] + _ModelViewProjectionInversed[1][0] * In[1] + _ModelViewProjectionInversed[2][0] * In[2] + _ModelViewProjectionInversed[3][0] * In[3];
    _Result.Y       = _ModelViewProjectionInversed[0][1] * In[0] + _ModelViewProjectionInversed[1][1] * In[1] + _ModelViewProjectionInversed[2][1] * In[2] + _ModelViewProjectionInversed[3][1] * In[3];
    _Result.Z       = _ModelViewProjectionInversed[0][2] * In[0] + _ModelViewProjectionInversed[1][2] * In[1] + _ModelViewProjectionInversed[2][2] * In[2] + _ModelViewProjectionInversed[3][2] * In[3];
    const float Div = _ModelViewProjectionInversed[0][3] * In[0] + _ModelViewProjectionInversed[1][3] * In[1] + _ModelViewProjectionInversed[2][3] * In[2] + _ModelViewProjectionInversed[3][3] * In[3];

    if ( Div == 0.0f ) {
        return false;
    }

    _Result /= Div;

    return true;
}

AN_INLINE bool UnprojectRay( Float4x4 const & _ModelViewProjectionInversed, const float _Viewport[4], float _X, float _Y, Float3 & _RayStart, Float3 & _RayEnd )
{
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

AN_INLINE bool UnprojectRayDir( Float4x4 const & _ModelViewProjectionInversed, const float _Viewport[4], float _X, float _Y, Float3 & _RayStart, Float3 & _RayDir )
{
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

AN_INLINE bool UnprojectPoint( Float4x4 const & _ModelViewProjectionInversed,
                               const float      _Viewport[4],
                               float            _X,
                               float            _Y,
                               float            _Depth,
                               Float3 &         _Result )
{
    return Unproject( _ModelViewProjectionInversed, _Viewport, Float3( _X, _Y, _Depth ), _Result );
}

} // namespace Math
