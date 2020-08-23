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

#include "VectorMath.h"

struct Float2x2;
struct Float3x3;
struct Float4x4;
struct Float3x4;

// Column-major matrix 2x2
struct Float2x2 {
    Float2 Col0;
    Float2 Col1;

    Float2x2() = default;
    explicit Float2x2( Float3x3 const & _Value );
    explicit Float2x2( Float3x4 const & _Value );
    explicit Float2x2( Float4x4 const & _Value );
    constexpr Float2x2( Float2 const & _Col0, Float2 const & _Col1 ) : Col0( _Col0 ), Col1( _Col1 ) {}
    constexpr Float2x2( float const & _00, float const & _01,
                        float const & _10, float const & _11 )
        : Col0( _00, _01 ), Col1( _10, _11 ) {}
    constexpr explicit Float2x2( float const & _Diagonal ) : Col0( _Diagonal, 0 ), Col1( 0, _Diagonal ) {}
    constexpr explicit Float2x2( Float2 const & _Diagonal ) : Col0( _Diagonal.X, 0 ), Col1( 0, _Diagonal.Y ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

//    Float2x2 & operator=( Float2x2 const & _Other ) {
//        Col0 = _Other.Col0;
//        Col1 = _Other.Col1;
//        return *this;
//    }

    Float2 & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float2 const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float2 GetRow( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 2, "Index out of range" );
        return Float2( Col0[ _Index ], Col1[ _Index ] );
    }

    bool operator==( Float2x2 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float2x2 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float2x2 const & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 4 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float2x2 const & _Other, float const & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 4 ; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
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
        Core::ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        Col0.Y = Col1.X = 0;
        Col0.X = Col1.Y = 1;
    }

    static Float2x2 Scale( Float2 const & _Scale  ) {
        return Float2x2( _Scale );
    }

    Float2x2 Scaled( Float2 const & _Scale ) const {
        return Float2x2( Col0 * _Scale[0], Col1 * _Scale[1] );
    }

    // Return rotation around Z axis
    static Float2x2 Rotation( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float2x2( c, s,
                        -s, c );
    }

    Float2x2 operator*( float const & _Value ) const {
        return Float2x2( Col0 * _Value,
                         Col1 * _Value );
    }

    void operator*=( float const & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
    }

    Float2x2 operator/( float const & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float2x2( Col0 * OneOverValue,
                         Col1 * OneOverValue );
    }

    void operator/=( float const & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
    }

    template< typename T >
    TVector2< T > operator*( TVector2< T > const & _Vec ) const {
        return TVector2< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y );
    }

    Float2x2 operator*( Float2x2 const & _Mat ) const {
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

    void operator*=( Float2x2 const & _Mat ) {
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
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
    }

    // Static methods

    static Float2x2 const & Identity() {
        static constexpr Float2x2 IdentityMat( 1 );
        return IdentityMat;
    }

};

// Column-major matrix 3x3
struct Float3x3 {
    Float3 Col0;
    Float3 Col1;
    Float3 Col2;

    Float3x3() = default;
    explicit Float3x3( Float2x2 const & _Value );
    explicit Float3x3( Float3x4 const & _Value );
    explicit Float3x3( Float4x4 const & _Value );
    constexpr Float3x3( Float3 const & _Col0, Float3 const & _Col1, Float3 const & _Col2 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Float3x3( float const & _00, float const & _01, float const & _02,
          float const & _10, float const & _11, float const & _12,
          float const & _20, float const & _21, float const & _22 )
        : Col0( _00, _01, _02 ), Col1( _10, _11, _12 ), Col2( _20, _21, _22 ) {}
    constexpr explicit Float3x3( float const & _Diagonal ) : Col0( _Diagonal, 0, 0 ), Col1( 0, _Diagonal, 0 ), Col2( 0, 0, _Diagonal ) {}
    constexpr explicit Float3x3( Float3 const & _Diagonal ) : Col0( _Diagonal.X, 0, 0 ), Col1( 0, _Diagonal.Y, 0 ), Col2( 0, 0, _Diagonal.Z ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

//    Float3x3 & operator=( Float3x3 const & _Other ) {
//        //Col0 = _Other.Col0;
//        //Col1 = _Other.Col1;
//        //Col2 = _Other.Col2;
//        Core::Memcpy( this, &_Other, sizeof( *this ) );
//        return *this;
//    }

    Float3 & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float3 const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float3 GetRow( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return Float3( Col0[ _Index ], Col1[ _Index ], Col2[ _Index ] );
    }

    bool operator==( Float3x3 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float3x3 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float3x3 const & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 9 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float3x3 const & _Other, float const & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 9 ; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
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
        Float3x3 const & m = *this;
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
        Core::ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        //Clear();
        //Col0.X = Col1.Y = Col2.Z = 1;
        *this = Identity();
    }

    static Float3x3 Scale( Float3 const & _Scale  ) {
        return Float3x3( _Scale );
    }

    Float3x3 Scaled( Float3 const & _Scale ) const {
        return Float3x3( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2] );
    }

    // Return rotation around normalized axis
    static Float3x3 RotationAroundNormal( float const & _AngleRad, Float3 const & _Normal ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        const Float3 Temp = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float3x3( c + Temp[0] * _Normal[0],                Temp[0] * _Normal[1] + Temp2[2],     Temp[0] * _Normal[2] - Temp2[1],
                             Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1],                Temp[1] * _Normal[2] + Temp2[0],
                             Temp[2] * _Normal[0] + Temp2[1],     Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2] );
    }

    // Return rotation around normalized axis
    Float3x3 RotateAroundNormal( float const & _AngleRad, Float3 const & _Normal ) const {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        Float3 Temp = ( 1.0f - c ) * _Normal;
        Float3 Temp2 = s * _Normal;
        return Float3x3( Col0 * (c + Temp[0] * _Normal[0])            + Col1 * (    Temp[0] * _Normal[1] + Temp2[2]) + Col2 * (    Temp[0] * _Normal[2] - Temp2[1]),
                         Col0 * (    Temp[1] * _Normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * _Normal[1])            + Col2 * (    Temp[1] * _Normal[2] + Temp2[0]),
                         Col0 * (    Temp[2] * _Normal[0] + Temp2[1]) + Col1 * (    Temp[2] * _Normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * _Normal[2]) );
    }

    // Return rotation around unnormalized vector
    static Float3x3 RotationAroundVector( float const & _AngleRad, Float3 const & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Float3x3 RotateAroundVector( float const & _AngleRad, Float3 const & _Vector ) const {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float3x3 RotationX( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x3( 1, 0, 0,
                         0, c, s,
                         0,-s, c );
    }

    // Return rotation around Y axis
    static Float3x3 RotationY( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x3( c, 0,-s,
                         0, 1, 0,
                         s, 0, c );
    }

    // Return rotation around Z axis
    static Float3x3 RotationZ( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x3( c, s, 0,
                        -s, c, 0,
                         0, 0, 1 );
    }

    Float3x3 operator*( float const & _Value ) const {
        return Float3x3( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( float const & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Float3x3 operator/( float const & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float3x3( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue );
    }

    void operator/=( float const & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    template< typename T >
    TVector3< T > operator*( TVector3< T > const & _Vec ) const {
        return TVector3< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z,
                              Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z );
    }

    Float3x3 operator*( Float3x3 const & _Mat ) const {
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

    void operator*=( Float3x3 const & _Mat ) {
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
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
    }

    // Static methods

    static Float3x3 const & Identity() {
        static constexpr Float3x3 IdentityMat( 1 );
        return IdentityMat;
    }

};

// Column-major matrix 4x4
struct Float4x4 {
    Float4 Col0;
    Float4 Col1;
    Float4 Col2;
    Float4 Col3;

    Float4x4() = default;
    explicit Float4x4( Float2x2 const & _Value );
    explicit Float4x4( Float3x3 const & _Value );
    explicit Float4x4( Float3x4 const & _Value );
    constexpr Float4x4( Float4 const & _Col0, Float4 const & _Col1, Float4 const & _Col2, Float4 const & _Col3 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ), Col3( _Col3 ) {}
    constexpr Float4x4( float const & _00, float const & _01, float const & _02, float const & _03,
                        float const & _10, float const & _11, float const & _12, float const & _13,
                        float const & _20, float const & _21, float const & _22, float const & _23,
                        float const & _30, float const & _31, float const & _32, float const & _33 )
        : Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ), Col3( _30, _31, _32, _33 ) {}
    constexpr explicit Float4x4( float const & _Diagonal ) : Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ), Col3( 0, 0, 0, _Diagonal ) {}
    constexpr explicit Float4x4( Float4 const & _Diagonal ) : Col0( _Diagonal.X, 0, 0, 0 ), Col1( 0, _Diagonal.Y, 0, 0 ), Col2( 0, 0, _Diagonal.Z, 0 ), Col3( 0, 0, 0, _Diagonal.W ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

//    Float4x4 & operator=( Float4x4 const & _Other ) {
//        //Col0 = _Other.Col0;
//        //Col1 = _Other.Col1;
//        //Col2 = _Other.Col2;
//        //Col3 = _Other.Col3;
//        Core::Memcpy( this, &_Other, sizeof( *this ) );
//        return *this;
//    }

    Float4 & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float4 const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float4 GetRow( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return Float4( Col0[ _Index ], Col1[ _Index ], Col2[ _Index ], Col3[ _Index ] );
    }

    bool operator==( Float4x4 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float4x4 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float4x4 const & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 16 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float4x4 const & _Other, float const & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 16 ; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
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
        Core::ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        //Clear();
        //Col0.X = Col1.Y = Col2.Z = Col3.W = 1;
        *this = Identity();
    }

    static Float4x4 Translation( Float3 const & _Vec ) {
        return Float4x4( Float4( 1,0,0,0 ),
                         Float4( 0,1,0,0 ),
                         Float4( 0,0,1,0 ),
                         Float4( _Vec[0], _Vec[1], _Vec[2], 1 ) );
    }

    Float4x4 Translated( Float3 const & _Vec ) const {
        return Float4x4( Col0, Col1, Col2, Col0 * _Vec[0] + Col1 * _Vec[1] + Col2 * _Vec[2] + Col3 );
    }

    static Float4x4 Scale( Float3 const & _Scale  ) {
        return Float4x4( Float4( _Scale[0],0,0,0 ),
                         Float4( 0,_Scale[1],0,0 ),
                         Float4( 0,0,_Scale[2],0 ),
                         Float4( 0,0,0,1 ) );
    }

    Float4x4 Scaled( Float3 const & _Scale ) const {
        return Float4x4( Col0 * _Scale[0], Col1 * _Scale[1], Col2 * _Scale[2], Col3 );
    }


    // Return rotation around normalized axis
    static Float4x4 RotationAroundNormal( float const & _AngleRad, Float3 const & _Normal ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        const Float3 Temp = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float4x4( c + Temp[0] * _Normal[0],                Temp[0] * _Normal[1] + Temp2[2],     Temp[0] * _Normal[2] - Temp2[1], 0,
                             Temp[1] * _Normal[0] - Temp2[2], c + Temp[1] * _Normal[1],                Temp[1] * _Normal[2] + Temp2[0], 0,
                             Temp[2] * _Normal[0] + Temp2[1],     Temp[2] * _Normal[1] - Temp2[0], c + Temp[2] * _Normal[2],            0,
                         0,0,0,1 );
    }

    // Return rotation around normalized axis
    Float4x4 RotateAroundNormal( float const & _AngleRad, Float3 const & _Normal ) const {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        Float3 Temp = ( 1.0f - c ) * _Normal;
        Float3 Temp2 = s * _Normal;
        return Float4x4( Col0 * (c + Temp[0] * _Normal[0])            + Col1 * (    Temp[0] * _Normal[1] + Temp2[2]) + Col2 * (    Temp[0] * _Normal[2] - Temp2[1]),
                         Col0 * (    Temp[1] * _Normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * _Normal[1])            + Col2 * (    Temp[1] * _Normal[2] + Temp2[0]),
                         Col0 * (    Temp[2] * _Normal[0] + Temp2[1]) + Col1 * (    Temp[2] * _Normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * _Normal[2]),
                         Col3 );
    }

    // Return rotation around unnormalized vector
    static Float4x4 RotationAroundVector( float const & _AngleRad, Float3 const & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around unnormalized vector
    Float4x4 RotateAroundVector( float const & _AngleRad, Float3 const & _Vector ) const {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float4x4 RotationX( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float4x4( 1, 0, 0, 0,
                         0, c, s, 0,
                         0,-s, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Y axis
    static Float4x4 RotationY( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float4x4( c, 0,-s, 0,
                         0, 1, 0, 0,
                         s, 0, c, 0,
                         0, 0, 0, 1 );
    }

    // Return rotation around Z axis
    static Float4x4 RotationZ( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float4x4( c, s, 0, 0,
                        -s, c, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1 );
    }

    template< typename T >
    TVector4< T > operator*( TVector4< T > const & _Vec ) const {
        return TVector4< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z + Col3[0] * _Vec.W,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z + Col3[1] * _Vec.W,
                              Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z + Col3[2] * _Vec.W,
                              Col0[3] * _Vec.X + Col1[3] * _Vec.Y + Col2[3] * _Vec.Z + Col3[3] * _Vec.W );
    }

    // Assume _Vec.W = 1
    template< typename T >
    TVector4< T > operator*( TVector3< T > const & _Vec ) const {
        return TVector4< T >( Col0[ 0 ] * _Vec.X + Col1[ 0 ] * _Vec.Y + Col2[ 0 ] * _Vec.Z + Col3[ 0 ],
                              Col0[ 1 ] * _Vec.X + Col1[ 1 ] * _Vec.Y + Col2[ 1 ] * _Vec.Z + Col3[ 1 ],
                              Col0[ 2 ] * _Vec.X + Col1[ 2 ] * _Vec.Y + Col2[ 2 ] * _Vec.Z + Col3[ 2 ],
                              Col0[ 3 ] * _Vec.X + Col1[ 3 ] * _Vec.Y + Col2[ 3 ] * _Vec.Z + Col3[ 3 ] );
    }

    // Same as Float3x3(*this)*_Vec
    template< typename T >
    TVector3< T > TransformAsFloat3x3( TVector3< T > const & _Vec ) const {
        return TVector3< T >( Col0[0] * _Vec.X + Col1[0] * _Vec.Y + Col2[0] * _Vec.Z,
                              Col0[1] * _Vec.X + Col1[1] * _Vec.Y + Col2[1] * _Vec.Z,
                              Col0[2] * _Vec.X + Col1[2] * _Vec.Y + Col2[2] * _Vec.Z );
    }

    // Same as Float3x3(*this)*_Mat
    Float3x3 TransformAsFloat3x3( Float3x3 const & _Mat ) const {
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

    Float4x4 operator*( float const & _Value ) const {
        return Float4x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value,
                         Col3 * _Value );
    }

    void operator*=( float const & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
        Col3 *= _Value;
    }

    Float4x4 operator/( float const & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float4x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue,
                         Col3 * OneOverValue );
    }

    void operator/=( float const & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
        Col3 *= OneOverValue;
    }

    Float4x4 operator*( Float4x4 const & _Mat ) const {
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

    void operator*=( Float4x4 const & _Mat ) {
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

    Float4x4 operator*( Float3x4 const & _Mat ) const;

    void operator*=( Float3x4 const & _Mat );

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
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " " + Col3.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " " + Col3.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
        Col3.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
        Col3.Read( _Stream );
    }

    // Static methods

    static Float4x4 const & Identity() {
        static constexpr Float4x4 IdentityMat( 1 );
        return IdentityMat;
    }

    static AN_FORCEINLINE Float4x4 LookAt( Float3 const & eye, Float3 const & center, Float3 const & up )
    {
        Float3 const f( (center - eye).Normalized() );
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
    static AN_FORCEINLINE Float4x4 const & ClipControl_UpperLeft_ZeroToOne() {
        static constexpr float ClipTransform[] = {
            1,        0,        0,        0,
            0,       -1,        0,        0,
            0,        0,      0.5f,       0,
            0,        0,      0.5f,       1
        };

        return *reinterpret_cast< Float4x4 const * >( &ClipTransform[0] );

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
    static AN_FORCEINLINE Float4x4 Perspective( float const & _FovXRad, float const & _Width, float const & _Height, float const & _ZNear, float const & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY = (float)(atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZFar + _ZNear) / ( _ZNear - _ZFar ),  -1,
                         0,                0,               2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    static AN_FORCEINLINE Float4x4 Perspective( float const & _FovXRad, float const & _FovYRad, float const & _ZNear, float const & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZFar + _ZNear) / ( _ZNear - _ZFar ),  -1,
                         0,                0,               2 * _ZFar * _ZNear / ( _ZNear - _ZFar ), 0 );
    }

    // OpenGL perspective projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 PerspectiveCC( float const & _FovXRad, float const & _Width, float const & _Height, float const & _ZNear, float const & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _Width, _Height, _ZNear, _ZFar );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveCC( float const & _FovXRad, float const & _FovYRad, float const & _ZNear, float const & _ZFar ) {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective( _FovXRad, _FovYRad, _ZNear, _ZFar );
    }

    // Reversed-depth OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRev( float const & _FovXRad, float const & _Width, float const & _Height, float const & _ZNear, float const & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY = (float)(atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZNear + _ZFar) / ( _ZFar - _ZNear ),  -1,
                         0,                0,               2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRev( float const & _FovXRad, float const & _FovYRad, float const & _ZNear, float const & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        return Float4x4( 1 / TanHalfFovX,  0,               0,                          0,
                         0,                1 / TanHalfFovY, 0,                          0,
                         0,                0,               (_ZNear + _ZFar) / ( _ZFar - _ZNear ),  -1,
                         0,                0,               2 * _ZNear * _ZFar / ( _ZFar - _ZNear ), 0 );
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRevCC( float const & _FovXRad, float const & _Width, float const & _Height, float const & _ZNear, float const & _ZFar ) {
        const float TanHalfFovX = tan( _FovXRad * 0.5f );
        const float HalfFovY = (float)(atan2( _Height, _Width / TanHalfFovX ) );
        const float TanHalfFovY = tan( HalfFovY );
        return Float4x4( 1 / TanHalfFovX, 0,               0,                                         0,
                         0,              -1 / TanHalfFovY, 0,                                         0,
                         0,               0,               _ZNear / ( _ZFar - _ZNear),               -1,
                         0,               0,               _ZNear * _ZFar / ( _ZFar - _ZNear ),       0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRevCC_Y( float const & _FovYRad, float const & _Width, float const & _Height, float const & _ZNear, float const & _ZFar ) {
        const float TanHalfFovY = tan( _FovYRad * 0.5f );
        const float HalfFovX = atan2( TanHalfFovY * _Width, _Height );
        const float TanHalfFovX = tan( HalfFovX );
        return Float4x4( 1 / TanHalfFovX, 0,               0,                                         0,
                         0,              -1 / TanHalfFovY, 0,                                         0,
                         0,               0,               _ZNear / ( _ZFar - _ZNear),               -1,
                         0,               0,               _ZNear * _ZFar / ( _ZFar - _ZNear ),       0 );
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRevCC( float const & _FovXRad, float const & _FovYRad, float const & _ZNear, float const & _ZFar ) {
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
        _PositiveX = Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( Math::_HALF_PI, Float3(0,1,0) );
        _NegativeX = Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( -Math::_HALF_PI, Float3(0,1,0) );
        _PositiveY = Float4x4::RotationX( -Math::_HALF_PI );
        _NegativeY = Float4x4::RotationX( Math::_HALF_PI );
        _PositiveZ = Float4x4::RotationX( Math::_PI );
        _NegativeZ = Float4x4::RotationZ( Math::_PI );
    }

    static AN_FORCEINLINE Float4x4 const * GetCubeFaceMatrices() {
        // TODO: Precompute this matrices
        static Float4x4 const CubeFaceMatrices[6] = {
            Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( Math::_HALF_PI, Float3(0,1,0) ),
            Float4x4::RotationZ( Math::_PI ).RotateAroundNormal( -Math::_HALF_PI, Float3(0,1,0) ),
            Float4x4::RotationX( -Math::_HALF_PI ),
            Float4x4::RotationX( Math::_HALF_PI ),
            Float4x4::RotationX( Math::_PI ),
            Float4x4::RotationZ( Math::_PI )
        };
        return CubeFaceMatrices;
    }
};

// Column-major matrix 3x4
// Keep transformations transposed
struct Float3x4 {
    Float4 Col0;
    Float4 Col1;
    Float4 Col2;

    Float3x4() = default;
    explicit Float3x4( Float2x2 const & _Value );
    explicit Float3x4( Float3x3 const & _Value );
    explicit Float3x4( Float4x4 const & _Value );
    constexpr Float3x4( Float4 const & _Col0, Float4 const & _Col1, Float4 const & _Col2 ) : Col0( _Col0 ), Col1( _Col1 ), Col2( _Col2 ) {}
    constexpr Float3x4( float const & _00, float const & _01, float const & _02, float const & _03,
                        float const & _10, float const & _11, float const & _12, float const & _13,
                        float const & _20, float const & _21, float const & _22, float const & _23 )
        : Col0( _00, _01, _02, _03 ), Col1( _10, _11, _12, _13 ), Col2( _20, _21, _22, _23 ) {}
    constexpr explicit Float3x4( float const & _Diagonal ) : Col0( _Diagonal, 0, 0, 0 ), Col1( 0, _Diagonal, 0, 0 ), Col2( 0, 0, _Diagonal, 0 ) {}
    constexpr explicit Float3x4( Float3 const & _Diagonal ) : Col0( _Diagonal.X, 0, 0, 0 ), Col1( 0, _Diagonal.Y, 0, 0 ), Col2( 0, 0, _Diagonal.Z, 0 ) {}

    float * ToPtr() {
        return &Col0.X;
    }

    const float * ToPtr() const {
        return &Col0.X;
    }

//    Float3x4 & operator=( Float3x4 const & _Other ) {
//        //Col0 = _Other.Col0;
//        //Col1 = _Other.Col1;
//        //Col2 = _Other.Col2;
//        Core::Memcpy( this, &_Other, sizeof( *this ) );
//        return *this;
//    }

    Float4 & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float4 const & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 3, "Index out of range" );
        return (&Col0)[ _Index ];
    }

    Float3 GetRow( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < 4, "Index out of range" );
        return Float3( Col0[ _Index ], Col1[ _Index ], Col2[ _Index ] );
    }

    bool operator==( Float3x4 const & _Other ) const { return Compare( _Other ); }
    bool operator!=( Float3x4 const & _Other ) const { return !Compare( _Other ); }

    bool Compare( Float3x4 const & _Other ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 12 ; i++ ) {
            if ( *pm1++ != *pm2++ ) {
                return false;
            }
        }
        return true;
    }

    bool CompareEps( Float3x4 const & _Other, float const & _Epsilon ) const {
        const float * pm1 = ToPtr();
        const float * pm2 = _Other.ToPtr();
        for ( int i = 0 ; i < 12 ; i++ ) {
            if ( Math::Abs( *pm1++ - *pm2++ ) >= _Epsilon ) {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void Compose( Float3 const & _Translation, Float3x3 const & _Rotation, Float3 const & _Scale ) {
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

    void Compose( Float3 const & _Translation, Float3x3 const & _Rotation ) {
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

    void SetTranslation( Float3 const & _Translation ) {
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
        Float3x4 const & m = *this;

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
        Float3x4 const & m = *this;

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
        Core::ZeroMem( this, sizeof( *this) );
    }

    void SetIdentity() {
        //Clear();
        //Col0.X = Col1.Y = Col2.Z = 1;
        *this = Identity();
    }

    static Float3x4 Translation( Float3 const & _Vec ) {
        return Float3x4( Float4( 1,0,0,_Vec[0] ),
                         Float4( 0,1,0,_Vec[1] ),
                         Float4( 0,0,1,_Vec[2] ) );
    }

    static Float3x4 Scale( Float3 const & _Scale  ) {
        return Float3x4( Float4( _Scale[0],0,0,0 ),
                         Float4( 0,_Scale[1],0,0 ),
                         Float4( 0,0,_Scale[2],0 ) );
    }

    // Return rotation around normalized axis
    static Float3x4 RotationAroundNormal( float const & _AngleRad, Float3 const & _Normal ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        const Float3 Temp = ( 1.0f - c ) * _Normal;
        const Float3 Temp2 = s * _Normal;
        return Float3x4( c + Temp[0] * _Normal[0]        ,        Temp[1] * _Normal[0] - Temp2[2],     Temp[2] * _Normal[0] + Temp2[1], 0,
                             Temp[0] * _Normal[1] + Temp2[2], c + Temp[1] * _Normal[1],                Temp[2] * _Normal[1] - Temp2[0], 0,
                             Temp[0] * _Normal[2] - Temp2[1],     Temp[1] * _Normal[2] + Temp2[0], c + Temp[2] * _Normal[2],            0 );
    }

    // Return rotation around unnormalized vector
    static Float3x4 RotationAroundVector( float const & _AngleRad, Float3 const & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Float3x4 RotationX( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x4( 1, 0, 0, 0,
                         0, c,-s, 0,
                         0, s, c, 0 );
    }

    // Return rotation around Y axis
    static Float3x4 RotationY( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x4( c, 0, s, 0,
                         0, 1, 0, 0,
                        -s, 0, c, 0 );
    }

    // Return rotation around Z axis
    static Float3x4 RotationZ( float const & _AngleRad ) {
        float s, c;
        Math::SinCos( _AngleRad, s, c );
        return Float3x4( c,-s, 0, 0,
                         s, c, 0, 0,
                         0, 0, 1, 0 );
    }

    // Assume _Vec.W = 1
    template< typename T >
    TVector3< T > operator*( TVector3< T > const & _Vec ) const {
        return TVector3< T >( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[2] * _Vec.Z + Col0[3],
                              Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[2] * _Vec.Z + Col1[3],
                              Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[2] * _Vec.Z + Col2[3] );
    }

    // Assume _Vec.Z = 0, _Vec.W = 1
    template< typename T >
    TVector3< T > operator*( TVector2< T > const & _Vec ) const {
        return TVector3< T >( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                              Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3],
                              Col2[0] * _Vec.X + Col2[1] * _Vec.Y + Col2[3] );
    }

    template< typename T >
    TVector2< T > Mult_Vec2_IgnoreZ( TVector2< T > const & _Vec ) {
        return TVector2< T >( Col0[0] * _Vec.X + Col0[1] * _Vec.Y + Col0[3],
                              Col1[0] * _Vec.X + Col1[1] * _Vec.Y + Col1[3] );
    }

    Float3x4 operator*( float const & _Value ) const {
        return Float3x4( Col0 * _Value,
                         Col1 * _Value,
                         Col2 * _Value );
    }

    void operator*=( float const & _Value ) {
        Col0 *= _Value;
        Col1 *= _Value;
        Col2 *= _Value;
    }

    Float3x4 operator/( float const & _Value ) const {
        const float OneOverValue = 1.0f / _Value;
        return Float3x4( Col0 * OneOverValue,
                         Col1 * OneOverValue,
                         Col2 * OneOverValue);
    }

    void operator/=( float const & _Value ) {
        const float OneOverValue = 1.0f / _Value;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
    }

    Float3x4 operator*( Float3x4 const & _Mat ) const {
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

    void operator*=( Float3x4 const & _Mat ) {
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
    AString ToString( int _Precision = Math::FloatingPointPrecision< float >() ) const {
        return AString( "( " ) + Col0.ToString( _Precision ) + " " + Col1.ToString( _Precision ) + " " + Col2.ToString( _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Col0.ToHexString( _LeadingZeros, _Prefix ) + " " + Col1.ToHexString( _LeadingZeros, _Prefix ) + " " + Col2.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        Col0.Write( _Stream );
        Col1.Write( _Stream );
        Col2.Write( _Stream );
    }

    void Read( IBinaryStream & _Stream ) {
        Col0.Read( _Stream );
        Col1.Read( _Stream );
        Col2.Read( _Stream );
    }

    // Static methods

    static Float3x4 const & Identity() {
        static constexpr Float3x4 IdentityMat( 1 );
        return IdentityMat;
    }
};

// Type conversion

// Float3x3 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( Float3x3 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float3x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( Float3x4 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float4x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2( Float4x4 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ) {}

// Float2x2 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( Float2x2 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0,0,1 ) {}

// Float3x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( Float3x4 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float4x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3( Float4x4 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float2x2 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( Float2x2 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0,0,1,0 ), Col3( 0,0,0,1 ) {}

// Float3x3 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( Float3x3 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0,0,0,1 ) {}

// Float3x4 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4( Float3x4 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ), Col3( 0,0,0,1 ) {}

// Float2x2 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( Float2x2 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( 0.0f ) {}

// Float3x3 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( Float3x3 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}

// Float4x4 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4( Float4x4 const & _Value ) : Col0( _Value.Col0 ), Col1( _Value.Col1 ), Col2( _Value.Col2 ) {}


template< typename T >
AN_FORCEINLINE TVector2< T > operator*( TVector2< T > const & _Vec, Float2x2 const & _Mat ) {
    return TVector2< T >( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y,
                          _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y );
}

template< typename T >
AN_FORCEINLINE TVector3< T > operator*( TVector3< T > const & _Vec, Float3x3 const & _Mat ) {
    return TVector3< T >( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z,
                          _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z,
                          _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z);
}

template< typename T >
AN_FORCEINLINE TVector4< T > operator*( TVector4< T > const & _Vec, Float4x4 const & _Mat ) {
    return TVector4< T >( _Mat[0][0] * _Vec.X + _Mat[0][1] * _Vec.Y + _Mat[0][2] * _Vec.Z + _Mat[0][3] * _Vec.W,
                          _Mat[1][0] * _Vec.X + _Mat[1][1] * _Vec.Y + _Mat[1][2] * _Vec.Z + _Mat[1][3] * _Vec.W,
                          _Mat[2][0] * _Vec.X + _Mat[2][1] * _Vec.Y + _Mat[2][2] * _Vec.Z + _Mat[2][3] * _Vec.W,
                          _Mat[3][0] * _Vec.X + _Mat[3][1] * _Vec.Y + _Mat[3][2] * _Vec.Z + _Mat[3][3] * _Vec.W );
}

AN_FORCEINLINE Float4x4 Float4x4::operator*( Float3x4 const & _Mat ) const {
    Float4 const & SrcB0 = _Mat.Col0;
    Float4 const & SrcB1 = _Mat.Col1;
    Float4 const & SrcB2 = _Mat.Col2;

    return Float4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                     Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                     Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                     Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

AN_FORCEINLINE void Float4x4::operator*=( Float3x4 const & _Mat ) {
    Float4 const & SrcB0 = _Mat.Col0;
    Float4 const & SrcB1 = _Mat.Col1;
    Float4 const & SrcB2 = _Mat.Col2;

    *this = Float4x4( Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                      Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                      Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                      Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3 );
}

namespace Math {

    /*static*/ AN_FORCEINLINE bool Unproject( Float4x4 const & _ModelViewProjectionInversed, const float _Viewport[4], Float3 const & _Coord, Float3 & _Result ) {
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

    /*static*/ AN_FORCEINLINE bool UnprojectRay( Float4x4 const & _ModelViewProjectionInversed, const float _Viewport[4], float const & _X, float const & _Y, Float3 & _RayStart, Float3 & _RayEnd ) {
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

    /*static*/ AN_FORCEINLINE bool UnprojectRayDir( Float4x4 const & _ModelViewProjectionInversed, const float _Viewport[4], float const & _X, float const & _Y, Float3 & _RayStart, Float3 & _RayDir ) {
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

    /*static*/ AN_FORCEINLINE bool UnprojectPoint( Float4x4 const & _ModelViewProjectionInversed,
                                                   const float _Viewport[4],
                                                   float const & _X, float const & _Y, float const & _Depth,
                                                   Float3 & _Result )
    {
        return Unproject( _ModelViewProjectionInversed, _Viewport, Float3( _X, _Y, _Depth ), _Result );
    }

}
