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

#include "Float.h"

struct Quat {
    float X;
    float Y;
    float Z;
    float W;

    Quat() = default;
    explicit constexpr Quat( const Float4 & _Value ) : X( _Value.X ), Y( _Value.Y ), Z( _Value.Z ), W( _Value.W ) {}
    constexpr Quat( const float _W, const float & _X, const float & _Y, const float & _Z ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}
    Quat( const float & _PitchRad, const float & _YawRad, const float & _RollRad ) { FromAngles( _PitchRad, _YawRad, _RollRad ); }

    float * ToPtr() {
        return &X;
    }

    const float * ToPtr() const {
        return &X;
    }

//    Quat & operator=( const Quat & _Other ) {
//        X = _Other.X;
//        Y = _Other.Y;
//        Z = _Other.Z;
//        W = _Other.W;
//        return *this;
//    }

    float & operator[]( const int & _Index ) {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    const float & operator[]( const int & _Index ) const {
        AN_ASSERT_( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&X)[ _Index ];
    }

    bool operator==( const Quat & _Other ) const { return Compare( _Other ); }
    bool operator!=( const Quat & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Quat operator+() const {
        return *this;
    }
    Quat operator-() const {
        return Quat( -W, -X, -Y, -Z );
    }
    Quat operator+( const Quat & _Other ) const {
        return Quat( W + _Other.W, X + _Other.X, Y + _Other.Y, Z + _Other.Z );
    }
    Quat operator-( const Quat & _Other ) const {
        return Quat( W - _Other.W, X - _Other.X, Y - _Other.Y, Z - _Other.Z );
    }
    Quat operator*( const Quat & _Other ) const {
        return Quat( W * _Other.W - X * _Other.X - Y * _Other.Y - Z * _Other.Z,
                     W * _Other.X + X * _Other.W + Y * _Other.Z - Z * _Other.Y,
                     W * _Other.Y + Y * _Other.W + Z * _Other.X - X * _Other.Z,
                     W * _Other.Z + Z * _Other.W + X * _Other.Y - Y * _Other.X );

    }
    Quat operator/( const float & _Other ) const {
        float Denom = 1.0f / _Other;
        return Quat( W * Denom, X * Denom, Y * Denom, Z * Denom );
    }
    Quat operator*( const float & _Other ) const {
        return Quat( W * _Other, X * _Other, Y * _Other, Z * _Other );
    }
    void operator+=( const Quat & _Other ) {
        X += _Other.X;
        Y += _Other.Y;
        Z += _Other.Z;
        W += _Other.W;
    }
    void operator-=( const Quat & _Other ) {
        X -= _Other.X;
        Y -= _Other.Y;
        Z -= _Other.Z;
        W -= _Other.W;
    }
    void operator*=( const Quat & _Other ) {
        const float TX = X;
        const float TY = Y;
        const float TZ = Z;
        const float TW = W;
        X = TW * _Other.X + TX * _Other.W + TY * _Other.Z - TZ * _Other.Y;
        Y = TW * _Other.Y + TY * _Other.W + TZ * _Other.X - TX * _Other.Z;
        Z = TW * _Other.Z + TZ * _Other.W + TX * _Other.Y - TY * _Other.X;
        W = TW * _Other.W - TX * _Other.X - TY * _Other.Y - TZ * _Other.Z;
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

    // Floating point specific
    Bool4 IsInfinite() const {
        return Bool4( Math::IsInfinite( X ), Math::IsInfinite( Y ), Math::IsInfinite( Z ), Math::IsInfinite( W ) );
    }

    Bool4 IsNan() const {
        return Bool4( Math::IsNan( X ), Math::IsNan( Y ), Math::IsNan( Z ), Math::IsNan( W ) );
    }

    Bool4 IsNormal() const {
        return Bool4( Math::IsNormal( X ), Math::IsNormal( Y ), Math::IsNormal( Z ), Math::IsNormal( W ) );
    }

    Bool4 NotEqual( const Quat & _Other ) const {
        return Bool4( Math::NotEqual( X, _Other.X ), Math::NotEqual( Y, _Other.Y ), Math::NotEqual( Z, _Other.Z ), Math::NotEqual( W, _Other.W ) );
    }

    bool Compare( const Quat & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    bool CompareEps( const Quat & _Other, const float & _Epsilon ) const {
        return Bool4( Math::Dist( X, _Other.X ) < _Epsilon,
                      Math::Dist( Y, _Other.Y ) < _Epsilon,
                      Math::Dist( Z, _Other.Z ) < _Epsilon,
                      Math::Dist( W, _Other.W ) < _Epsilon ).All();
    }

    // Quaternion methods
    float NormalizeSelf() {
        const float L = StdSqrt( X * X + Y * Y + Z * Z + W * W );
        if ( L != 0.0f ) {
            const float InvLength = 1.0f / L;
            X *= InvLength;
            Y *= InvLength;
            Z *= InvLength;
            W *= InvLength;
        }
        return L;
    }
    Quat Normalized() const {
        const float L = StdSqrt( X * X + Y * Y + Z * Z + W * W );
        if ( L != 0.0f ) {
            const float InvLength = 1.0f / L;
            return Quat( W * InvLength, X * InvLength, Y * InvLength, Z * InvLength );
        }
        return *this;
    }

    void InverseSelf() {
        const float DP = 1.0f / ( X * X + Y * Y + Z * Z );

        X = -X * DP;
        Y = -Y * DP;
        Z = -Z * DP;
        W =  W * DP;

        //ConjugateSelf();
        //*this /= DP;
    }

    Quat Inversed() const {
        return Conjugated() / ( X * X + Y * Y + Z * Z );
    }

    void ConjugateSelf() {
        X = -X;
        Y = -Y;
        Z = -Z;
    }

    Quat Conjugated() const {
        return Quat( W, -X, -Y, -Z );
    }

    float ComputeW() const {
        return StdSqrt( Math::Abs( 1.0f - ( X * X + Y * Y + Z * Z ) ) );
    }

    Float3 XAxis() const {
        float y2 = Y + Y, z2 = Z + Z;
        return Float3( 1.0f - (Y*y2 + Z*z2), X*y2 + W*z2, X*z2 - W*y2 );
    }

    Float3 YAxis() const {
        float x2 = X + X, y2 = Y + Y, z2 = Z + Z;
        return Float3( X*y2 - W*z2, 1.0f - (X*x2 + Z*z2), Y*z2 + W*x2 );
    }

    Float3 ZAxis() const {
        float x2 = X + X, y2 = Y + Y, z2 = Z + Z;
        return Float3( X*z2 + W*y2, Y*z2 - W*x2, 1.0f - (X*x2 + Y*y2) );
    }

    void SetIdentity() {
        X = Y = Z = 0;
        W = 1;
    }

    // Return rotation around normalized axis
    static Quat RotationAroundNormal( const float & _AngleRad, const Float3 & _Normal ) {
        float s, c;
        Math::SinCos( _AngleRad * 0.5f, s, c );
        return Quat( c, s * _Normal.X, s * _Normal.Y, s * _Normal.Z );
    }

    // Return rotation around unnormalized vector
    static Quat RotationAroundVector( const float & _AngleRad, const Float3 & _Vector ) {
        return RotationAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    // Return rotation around X axis
    static Quat RotationX( const float & _AngleRad ) {
        Quat Result;
        Math::SinCos( _AngleRad * 0.5f, Result.X, Result.W );
        Result.Y = 0;
        Result.Z = 0;
        return Result;
    }

    // Return rotation around Y axis
    static Quat RotationY( const float & _AngleRad ) {
        Quat Result;
        Math::SinCos( _AngleRad * 0.5f, Result.Y, Result.W );
        Result.X = 0;
        Result.Z = 0;
        return Result;
    }

    // Return rotation around Z axis
    static Quat RotationZ( const float & _AngleRad ) {
        Quat Result;
        Math::SinCos( _AngleRad * 0.5f, Result.Z, Result.W );
        Result.X = 0;
        Result.Y = 0;
        return Result;
    }

    Quat RotateAroundNormal( const float & _AngleRad, const Float3 & _Normal ) const {
        float s, c;
        Math::SinCos( _AngleRad * 0.5f, s, c );
        return *this * Quat( c, s * _Normal.X, s * _Normal.Y, s * _Normal.Z );
        // FIXME: or? return Quat( c, s * _Normal.X, s * _Normal.Y, s * _Normal.Z ) * (*this);
    }

    Quat RotateAroundVector( const float & _AngleRad, const Float3 & _Vector ) const {
        return RotateAroundNormal( _AngleRad, _Vector.Normalized() );
    }

    Float3 operator*( const Float3 & _Vec ) const {
        const float xxzz = X*X - Z*Z;
        const float wwyy = W*W - Y*Y;
        const float xw2 = X*W*2.0f;
        const float xy2 = X*Y*2.0f;
        const float xz2 = X*Z*2.0f;
        const float yw2 = Y*W*2.0f;
        const float yz2 = Y*Z*2.0f;
        const float zw2 = Z*W*2.0f;
        return Float3( Math::Dot( _Vec, Float3( xxzz + wwyy, xy2 + zw2, xz2 - yw2 ) ),
                       Math::Dot( _Vec, Float3( xy2 - zw2, Y*Y+W*W-X*X-Z*Z, yz2 + xw2 ) ),
                       Math::Dot( _Vec, Float3( xz2 + yw2, yz2 - xw2, wwyy - xxzz ) ) );
    }

    void ToAngles( float & _PitchRad, float & _YawRad, float & _RollRad ) const {
        const float xx = X * X;
        const float yy = Y * Y;
        const float zz = Z * Z;
        const float ww = W * W;

        // TODO: singularity test

        _PitchRad = atan2( 2.0f * ( Y * Z + W * X ), ww - xx - yy + zz );
        _YawRad   = asin( Math::Clamp( -2.0f * ( X * Z - W * Y ), -1.0f, 1.0f ) );
        _RollRad  = atan2( 2.0f * ( X * Y + W * Z ), ww + xx - yy - zz );
    }

    void FromAngles( const float & _PitchRad, const float & _YawRad, const float & _RollRad ) {
        float sx, sy, sz, cx, cy, cz;

        Math::SinCos( _PitchRad * 0.5f, sx, cx );
        Math::SinCos( _YawRad * 0.5f, sy, cy );
        Math::SinCos( _RollRad * 0.5f, sz, cz );

        const float w = cy * cx;
        const float x = cy * sx;
        const float y = sy * cx;
        const float z = sy * sx;

        X =  x*cz + y*sz;
        Y = -x*sz + y*cz;
        Z =  w*sz - z*cz;
        W =  w*cz + z*sz;
    }

    Float3x3 ToMatrix() const {
        const float xx = X * X;
        const float yy = Y * Y;
        const float zz = Z * Z;
        const float xz = X * Z;
        const float xy = X * Y;
        const float yz = Y * Z;
        const float wx = W * X;
        const float wy = W * Y;
        const float wz = W * Z;

        return Float3x3( 1.0f - 2.0f * (yy +  zz), 2.0f * (xy + wz), 2.0f * (xz - wy),
                         2.0f * (xy - wz), 1.0f - 2 * (xx +  zz), 2.0f * (yz + wx),
                         2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx +  yy) );
    }

    Float4x4 ToMatrix4x4() const {
        const float xx = X * X;
        const float yy = Y * Y;
        const float zz = Z * Z;
        const float xz = X * Z;
        const float xy = X * Y;
        const float yz = Y * Z;
        const float wx = W * X;
        const float wy = W * Y;
        const float wz = W * Z;

        return Float4x4( 1.0f - 2.0f * (yy +  zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0,
                         2.0f * (xy - wz), 1.0f - 2 * (xx +  zz), 2.0f * (yz + wx), 0,
                         2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx +  yy), 0,
                         0.0f, 0.0f, 0.0f, 1.0f );
    }

    void FromMatrix( const Float3x3 & _Matrix ) {
        // Based on code from GLM
        const float FourXSquaredMinus1 = _Matrix[0][0] - _Matrix[1][1] - _Matrix[2][2];
        const float FourYSquaredMinus1 = _Matrix[1][1] - _Matrix[0][0] - _Matrix[2][2];
        const float FourZSquaredMinus1 = _Matrix[2][2] - _Matrix[0][0] - _Matrix[1][1];
        const float FourWSquaredMinus1 = _Matrix[0][0] + _Matrix[1][1] + _Matrix[2][2];

        int BiggestIndex = 0;
        float FourBiggestSquaredMinus1 = FourWSquaredMinus1;
        if ( FourXSquaredMinus1 > FourBiggestSquaredMinus1 ) {
            FourBiggestSquaredMinus1 = FourXSquaredMinus1;
            BiggestIndex = 1;
        }
        if ( FourYSquaredMinus1 > FourBiggestSquaredMinus1 ) {
            FourBiggestSquaredMinus1 = FourYSquaredMinus1;
            BiggestIndex = 2;
        }
        if ( FourZSquaredMinus1 > FourBiggestSquaredMinus1 ) {
            FourBiggestSquaredMinus1 = FourZSquaredMinus1;
            BiggestIndex = 3;
        }

        const float BiggestVal = sqrt( FourBiggestSquaredMinus1 + 1.0f ) * 0.5f;
        const float Mult = 0.25f / BiggestVal;

        switch ( BiggestIndex ) {
        case 0:
            W = BiggestVal;
            X = (_Matrix[1][2] - _Matrix[2][1]) * Mult;
            Y = (_Matrix[2][0] - _Matrix[0][2]) * Mult;
            Z = (_Matrix[0][1] - _Matrix[1][0]) * Mult;
            break;
        case 1:
            W = (_Matrix[1][2] - _Matrix[2][1]) * Mult;
            X = BiggestVal;
            Y = (_Matrix[0][1] + _Matrix[1][0]) * Mult;
            Z = (_Matrix[2][0] + _Matrix[0][2]) * Mult;
            break;
        case 2:
            W = (_Matrix[2][0] - _Matrix[0][2]) * Mult;
            X = (_Matrix[0][1] + _Matrix[1][0]) * Mult;
            Y = BiggestVal;
            Z = (_Matrix[1][2] + _Matrix[2][1]) * Mult;
            break;
        case 3:
            W = (_Matrix[0][1] - _Matrix[1][0]) * Mult;
            X = (_Matrix[2][0] + _Matrix[0][2]) * Mult;
            Y = (_Matrix[1][2] + _Matrix[2][1]) * Mult;
            Z = BiggestVal;
            break;

        default:                    // Silence a -Wswitch-default warning in GCC. Should never actually get here. Assert is just for sanity.
            AN_ASSERT(false);
            break;
        }
    }

    float Pitch() const {
        return atan2( 2.0f * ( Y * Z + W * X ), W * W - X * X - Y * Y + Z * Z );
    }

    float Yaw() const {
        return asin( Math::Clamp( -2.0f * ( X * Z - W * Y ), -1.0f, 1.0f ) );
    }

    float Roll() const {
        return atan2( 2.0f * ( X * Y + W * Z ), W * W + X * X - Y * Y - Z * Z );
    }

    Quat Slerp( const Quat & _To, const float & _Mix ) const {
        return Slerp( *this, _To, _Mix );
    }

    static Quat Slerp( const Quat & _From, const Quat & _To, const float & _Mix );

    // String conversions
    AString ToString( int _Precision = -1 ) const {
        return AString( "( " ) + Math::ToString( X, _Precision ) + " " + Math::ToString( Y, _Precision ) + " " + Math::ToString( Z, _Precision ) + " " + Math::ToString( W, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return AString( "( " ) + Math::ToHexString( X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Z, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( W, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const {
        _Stream.WriteFloat( X );
        _Stream.WriteFloat( Y );
        _Stream.WriteFloat( Z );
        _Stream.WriteFloat( W );
    }

    void Read( IBinaryStream & _Stream ) {
        X = _Stream.ReadFloat();
        Y = _Stream.ReadFloat();
        Z = _Stream.ReadFloat();
        W = _Stream.ReadFloat();
        NormalizeSelf();
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }

    static const Quat & Zero() {
        static constexpr Quat ZeroQuat(0.0f,0.0f,0.0f,0.0f);
        return ZeroQuat;
    }

    static const Quat & Identity() {
        static constexpr Quat Q( 1,0,0,0 );
        return Q;
    }
};

AN_FORCEINLINE Quat operator*( const float & _Left, const Quat & _Right ) {
    return Quat( _Left * _Right.W, _Left * _Right.X, _Left * _Right.Y, _Left * _Right.Z );
}

AN_FORCEINLINE Quat Quat::Slerp( const Quat & _From, const Quat & _To, const float & _Mix ) {
    if ( _Mix <= 0.0f ) {
        return _From;
    }

    if ( _Mix >= 1.0f || _From.Compare( _To ) ) {
        return _To;
    }

    Quat Temp;
    float Scale0, Scale1;
    float CosOmega = _From.X * _To.X + _From.Y * _To.Y + _From.Z * _To.Z + _From.W * _To.W;
    if ( CosOmega < 0.0f ) {
        Temp = -_To;
        CosOmega = -CosOmega;
    } else {
        Temp = _To;
    }

    if ( 1.0 - CosOmega > 1e-6f ) {
        float SinOmega = StdSqrt( 1.0f - CosOmega * CosOmega );
        float Omega = atan2( SinOmega, CosOmega );
        float invSinOmega = 1.0f / SinOmega;
        Scale0 = sin( (1.0f - _Mix) * Omega ) * invSinOmega;
        Scale1 = sin( _Mix * Omega ) * invSinOmega;
    } else {
        // too small angle, use linear interpolation
        Scale0 = 1.0 - _Mix;
        Scale1 = _Mix;
    }

    return Scale0 * _From + Scale1 * Temp;
}
