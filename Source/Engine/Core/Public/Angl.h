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

#include "Quat.h"

class Angl final {
public:
    Float Pitch;
    Float Yaw;
    Float Roll;

    Angl() = default;
    explicit constexpr Angl( const Float3 & _Value ) : Pitch( _Value.X ), Yaw( _Value.Y ), Roll( _Value.Z ) {}
    constexpr Angl( const float & _Pitch, const float & _Yaw, const float & _Roll ) : Pitch( _Pitch ), Yaw( _Yaw ), Roll( _Roll ) {}

    Float * ToPtr() {
        return &Pitch;
    }

    const Float * ToPtr() const {
        return &Pitch;
    }

    Float3 & ToVec3() {
        return *reinterpret_cast< Float3 * >( &Pitch );
    }

    const Float3 & ToVec3() const {
        return *reinterpret_cast< const Float3 * >( &Pitch );
    }

    Angl & operator=( const Angl & _Other ) {
        Pitch = _Other.Pitch;
        Yaw = _Other.Yaw;
        Roll = _Other.Roll;
        return *this;
    }

    Float & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&Pitch)[ _Index ];
    }

    const Float & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < NumComponents(), "Index out of range" );
        return (&Pitch)[ _Index ];
    }

    Bool operator==( const Angl & _Other ) const { return Compare( _Other ); }
    Bool operator!=( const Angl & _Other ) const { return !Compare( _Other ); }

    // Math operators
    Angl operator+() const {
        return *this;
    }
    Angl operator-() const {
        return Angl( -Pitch, -Yaw, -Roll );
    }
    Angl operator+( const Angl & _Other ) const {
        return Angl( Pitch + _Other.Pitch, Yaw + _Other.Yaw, Roll + _Other.Roll );
    }
    Angl operator-( const Angl & _Other ) const {
        return Angl( Pitch - _Other.Pitch, Yaw - _Other.Yaw, Roll - _Other.Roll );
    }
    Angl operator/( const float & _Other ) const {
        float Denom = 1.0f / _Other;
        return Angl( Pitch * Denom, Yaw * Denom, Roll * Denom );
    }
    Angl operator*( const float & _Other ) const {
        return Angl( Pitch * _Other, Yaw * _Other, Roll * _Other );
    }
    Angl operator/( const Angl & _Other ) const {
        return Angl( Pitch / _Other.Pitch, Yaw / _Other.Yaw, Roll / _Other.Roll );
    }
    Angl operator*( const Angl & _Other ) const {
        return Angl( Pitch * _Other.Pitch, Yaw * _Other.Yaw, Roll * _Other.Roll );
    }
    void operator+=( const Angl & _Other ) {
        Pitch += _Other.Pitch;
        Yaw   += _Other.Yaw;
        Roll  += _Other.Roll;
    }
    void operator-=( const Angl & _Other ) {
        Pitch -= _Other.Pitch;
        Yaw   -= _Other.Yaw;
        Roll  -= _Other.Roll;
    }
    void operator/=( const Angl & _Other ) {
        Pitch /= _Other.Pitch;
        Yaw   /= _Other.Yaw;
        Roll  /= _Other.Roll;
    }
    void operator*=( const Angl & _Other ) {
        Pitch *= _Other.Pitch;
        Yaw   *= _Other.Yaw;
        Roll  *= _Other.Roll;
    }
    void operator/=( const float & _Other ) {
        float Denom = 1.0f / _Other;
        Pitch *= Denom;
        Yaw   *= Denom;
        Roll  *= Denom;
    }
    void operator*=( const float & _Other ) {
        Pitch *= _Other;
        Yaw   *= _Other;
        Roll  *= _Other;
    }

    // Floating point specific
    Bool3 IsInfinite() const {
        return Bool3( Pitch.IsInfinite(), Yaw.IsInfinite(), Roll.IsInfinite() );
    }

    Bool3 IsNan() const {
        return Bool3( Pitch.IsNan(), Yaw.IsNan(), Roll.IsNan() );
    }

    Bool3 IsNormal() const {
        return Bool3( Pitch.IsNormal(), Yaw.IsNormal(), Roll.IsNormal() );
    }

    Bool3 NotEqual( const Angl & _Other ) const {
        return Bool3( Pitch.NotEqual( _Other.Pitch ), Yaw.NotEqual( _Other.Yaw ), Roll.NotEqual( _Other.Roll ) );
    }

    Bool Compare( const Angl & _Other ) const {
        return !NotEqual( _Other ).Any();
    }

    Bool CompareEps( const Angl & _Other, const Float & _Epsilon ) const {
        return Bool3( Pitch.CompareEps( _Other.Pitch, _Epsilon ),
                      Yaw  .CompareEps( _Other.Yaw  , _Epsilon ),
                      Roll .CompareEps( _Other.Roll , _Epsilon ) ).All();
    }

    void Clear() {
        Pitch = Yaw = Roll = 0;
    }

    Quat ToQuat() const {
        float sx, sy, sz, cx, cy, cz;

        FMath::DegSinCos( Pitch * 0.5f, sx, cx );
        FMath::DegSinCos( Yaw * 0.5f, sy, cy );
        FMath::DegSinCos( Roll * 0.5f, sz, cz );

        const float w = cy*cx;
        const float x = cy*sx;
        const float y = sy*cx;
        const float z = sy*sx;

        return Quat( w*cz + z*sz, x*cz + y*sz, -x*sz + y*cz, w*sz - z*cz );
    }

    Float3x3 ToMat3() const {
        float sx, sy, sz, cx, cy, cz;

        FMath::DegSinCos( Pitch, sx, cx );
        FMath::DegSinCos( Yaw, sy, cy );
        FMath::DegSinCos( Roll, sz, cz );

        return Float3x3( cy * cz + sy * sx * sz, sz * cx, -sy * cz + cy * sx * sz,
                        -cy * sz + sy * sx * cz, cz * cx, sz * sy + cy * sx * cz,
                         sy * cx, -sx, cy * cx );
    }

    Float4x4 ToMat4() const {
        float sx, sy, sz, cx, cy, cz;

        FMath::DegSinCos( Pitch, sx, cx );
        FMath::DegSinCos( Yaw, sy, cy );
        FMath::DegSinCos( Roll, sz, cz );

        return Float4x4( cy * cz + sy * sx * sz, sz * cx, -sy * cz + cy * sx * sz, 0.0f,
                        -cy * sz + sy * sx * cz, cz * cx, sz * sy + cy * sx * cz, 0.0f,
                         sy * cx, -sx, cy * cx, 0,
                         0.0f, 0.0f, 0.0f, 1.0f );
    }

    static float Normalize360( const float & _Angle ) {
        //return (360.0f/65536) * (static_cast< int >(_Angle*(65536/360.0f)) & 65535);
        Float Norm = std::fmod( _Angle, 360.0f );
        if ( Norm < 0.0f ) {
            Norm += 360.0f;
        }
        return Norm;
    }

    static float Normalize180( const float & _Angle ) {
        float Norm = Normalize360( _Angle );
        if ( Norm > 180.0f ) {
            Norm -= 360.0f;
        }
        return Norm;
    }

    void Normalize360Self() {
        Pitch = Normalize360( Pitch );
        Yaw   = Normalize360( Yaw );
        Roll  = Normalize360( Roll );
    }

    Angl Normalized360() const {
        return Angl( Normalize360( Pitch ),
                     Normalize360( Yaw ),
                     Normalize360( Roll ) );
    }

    void Normalize180Self() {
        Pitch = Normalize180( Pitch );
        Yaw   = Normalize180( Yaw );
        Roll  = Normalize180( Roll );
    }

    Angl Normalized180() const {
        return Angl( Normalize180( Pitch ),
                     Normalize180( Yaw ),
                     Normalize180( Roll ) );
    }

    Angl Delta( const Angl & _Other ) const {
        return ( *this - _Other ).Normalized180();
    }

    static byte PackByte( const float & _Angle ) {
        return Float( _Angle * ( 256.0f / 360.0f ) ).ToIntFast() & 255;
    }

    static UShort PackShort( const float & _Angle ) {
        return Float( _Angle * ( 65536.0f / 360.0f ) ).ToIntFast() & 65535;
    }

    static Float UnpackByte( const byte & _Angle ) {
        return _Angle * ( 360.0f / 256.0f );
    }

    static Float UnpackShort( const uint16_t & _Angle ) {
        return _Angle * ( 360.0f / 65536.0f );
    }

    // String conversions
    FString ToString( int _Precision = -1 ) const {
        return FString( "( " ) + Pitch.ToString( _Precision ) + " " + Yaw.ToString( _Precision ) + " " + Roll.ToString( _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + Pitch.ToHexString( _LeadingZeros, _Prefix ) + " " + Yaw.ToHexString( _LeadingZeros, _Prefix ) + " " + Roll.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Pitch << Yaw << Roll;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Pitch;
        _Stream >> Yaw;
        _Stream >> Roll;
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }

    static const Angl & Zero() {
        static constexpr Angl ZeroAngle(0.0f,0.0f,0.0f);
        return ZeroAngle;
    }
};

AN_FORCEINLINE Angl operator*( const float & _Left, const Angl & _Right ) {
    return Angl( _Left * _Right.Pitch, _Left * _Right.Yaw, _Left * _Right.Roll );
}
