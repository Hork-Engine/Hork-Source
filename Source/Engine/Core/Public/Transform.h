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

#include "Angl.h"

struct STransform {
    Float3 Position;
    Quat Rotation;
    Float3 Scale;

    STransform() = default;
    STransform( Float3 const & _Position, Quat const & _Rotation, Float3 const & _Scale );
    STransform( Float3 const & _Position, Quat const & _Rotation );
    void Clear();
    void SetIdentity();
    void SetScale( Float3 const & _Scale );
    void SetScale( float const & _X, float const & _Y, float const & _Z );
    void SetScale( float const & _ScaleXYZ );
    void SetAngles( Angl const & _Angles );
    void SetAngles( float const & _Pitch, float const & _Yaw, float const & _Roll );
    Angl GetAngles() const;
    float GetPitch() const;
    float GetYaw() const;
    float GetRoll() const;
    Float3 GetRightVector() const;
    Float3 GetLeftVector() const;
    Float3 GetUpVector() const;
    Float3 GetDownVector() const;
    Float3 GetBackVector() const;
    Float3 GetForwardVector() const;
    void GetVectors( Float3 * _Right, Float3 * _Up, Float3 * _Back ) const;
    void ComputeTransformMatrix( Float3x4 & _LocalTransformMatrix ) const;
    void TurnRightFPS( float _DeltaAngleRad );
    void TurnLeftFPS( float _DeltaAngleRad );
    void TurnUpFPS( float _DeltaAngleRad );
    void TurnDownFPS( float _DeltaAngleRad );
    void TurnAroundAxis( float _DeltaAngleRad, Float3 const & _NormalizedAxis );
    void TurnAroundVector( float _DeltaAngleRad, Float3 const & _Vector );
    void StepRight( float _Units );
    void StepLeft( float _Units );
    void StepUp( float _Units );
    void StepDown( float _Units );
    void StepBack( float _Units );
    void StepForward( float _Units );
    void Step( Float3 const & _Vector );

    STransform operator*( STransform const & _T ) const;

    STransform Inversed() const;
    void InverseSelf();

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const;
    void Read( IBinaryStream & _Stream );
};

//AN_FORCEINLINE STransform::STransform()
//    : Position( 0, 0, 0 )
//    , Rotation( 1, 0, 0, 0 )
//    , Scale( 1, 1, 1 )
//{
//}

AN_FORCEINLINE STransform::STransform( Float3 const & _Position, Quat const & _Rotation, Float3 const & _Scale )
    : Position( _Position )
    , Rotation( _Rotation )
    , Scale( _Scale )
{
}

AN_FORCEINLINE STransform::STransform( Float3 const & _Position, Quat const & _Rotation )
    : Position( _Position )
    , Rotation( _Rotation )
    , Scale( 1, 1, 1 )
{
}

AN_FORCEINLINE void STransform::Clear() {
    Position.X = Position.Y = Position.Z = 0;
    SetIdentity();
    SetScale( 1 );
}

AN_FORCEINLINE void STransform::SetIdentity() {
    Rotation.SetIdentity();
}

AN_FORCEINLINE void STransform::SetScale( Float3 const & _Scale ) {
    Scale = _Scale;
}

AN_FORCEINLINE void STransform::SetScale( float const & _X, float const & _Y, float const & _Z ) {
    Scale.X = _X; Scale.Y = _Y; Scale.Z = _Z;
}

AN_FORCEINLINE void STransform::SetScale( float const & _ScaleXYZ ) {
    Scale.X = Scale.Y = Scale.Z = _ScaleXYZ;
}

AN_FORCEINLINE void STransform::SetAngles( Angl const & _Angles ) {
    Rotation = _Angles.ToQuat();
}

AN_FORCEINLINE void STransform::SetAngles( float const & _Pitch, float const & _Yaw, float const & _Roll ) {
    Rotation = Angl( _Pitch, _Yaw, _Roll ).ToQuat();
}

AN_FORCEINLINE Angl STransform::GetAngles() const {
    Angl Angles;
    Rotation.ToAngles( Angles.Pitch, Angles.Yaw, Angles.Roll );
    Angles.Pitch = Math::Degrees( Angles.Pitch );
    Angles.Yaw = Math::Degrees( Angles.Yaw );
    Angles.Roll = Math::Degrees( Angles.Roll );
    return Angles;
}

AN_FORCEINLINE float STransform::GetPitch() const {
    return Math::Degrees( Rotation.Pitch() );
}

AN_FORCEINLINE float STransform::GetYaw() const {
    return Math::Degrees( Rotation.Yaw() );
}

AN_FORCEINLINE float STransform::GetRoll() const {
    return Math::Degrees( Rotation.Roll() );
}

AN_FORCEINLINE Float3 STransform::GetRightVector() const {
    //return Math::ToMat3(Rotation)[0];

    const float qyy(Rotation.Y * Rotation.Y);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxz(Rotation.X * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qwy(Rotation.W * Rotation.Y);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3( 1 - 2 * (qyy +  qzz), 2 * (qxy + qwz), 2 * (qxz - qwy) );
}

AN_FORCEINLINE Float3 STransform::GetLeftVector() const {
    //return -Math::ToMat3(Rotation)[0];

    const float qyy(Rotation.Y * Rotation.Y);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxz(Rotation.X * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qwy(Rotation.W * Rotation.Y);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3( -1 + 2 * (qyy +  qzz), -2 * (qxy + qwz), -2 * (qxz - qwy) );
}

AN_FORCEINLINE Float3 STransform::GetUpVector() const {
    //return Math::ToMat3(Rotation)[1];

    const float qxx(Rotation.X * Rotation.X);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3( 2 * (qxy - qwz), 1 - 2 * (qxx +  qzz), 2 * (qyz + qwx) );
}

AN_FORCEINLINE Float3 STransform::GetDownVector() const {
    //return -Math::ToMat3(Rotation)[1];

    const float qxx(Rotation.X * Rotation.X);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3( -2 * (qxy - qwz), -1 + 2 * (qxx +  qzz), -2 * (qyz + qwx) );
}

AN_FORCEINLINE Float3 STransform::GetBackVector() const {
    //return Math::ToMat3(Rotation)[2];

    const float qxx(Rotation.X * Rotation.X);
    const float qyy(Rotation.Y * Rotation.Y);
    const float qxz(Rotation.X * Rotation.Z);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwy(Rotation.W * Rotation.Y);

    return Float3( 2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx +  qyy) );
}

AN_FORCEINLINE Float3 STransform::GetForwardVector() const {
    //return -Math::ToMat3(Rotation)[2];

    const float qxx(Rotation.X * Rotation.X);
    const float qyy(Rotation.Y * Rotation.Y);
    const float qxz(Rotation.X * Rotation.Z);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwy(Rotation.W * Rotation.Y);

    return Float3( -2 * (qxz + qwy), -2 * (qyz - qwx), -1 + 2 * (qxx +  qyy) );
}

AN_FORCEINLINE void STransform::GetVectors( Float3 * _Right, Float3 * _Up, Float3 * _Back ) const {
    //if ( _Right ) *_Right = Math::ToMat3(Rotation)[0];
    //if ( _Up ) *_Up = Math::ToMat3(Rotation)[1];
    //if ( _Back ) *_Back = Math::ToMat3(Rotation)[2];
    //return;

    const float qxx(Rotation.X * Rotation.X);
    const float qyy(Rotation.Y * Rotation.Y);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxz(Rotation.X * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwy(Rotation.W * Rotation.Y);
    const float qwz(Rotation.W * Rotation.Z);

    if ( _Right ) {
        _Right->X = 1 - 2 * (qyy +  qzz);
        _Right->Y = 2 * (qxy + qwz);
        _Right->Z = 2 * (qxz - qwy);
    }

    if ( _Up ) {
        _Up->X = 2 * (qxy - qwz);
        _Up->Y = 1 - 2 * (qxx +  qzz);
        _Up->Z = 2 * (qyz + qwx);
    }

    if ( _Back ) {
        _Back->X = 2 * (qxz + qwy);
        _Back->Y = 2 * (qyz - qwx);
        _Back->Z = 1 - 2 * (qxx +  qyy);
    }
}


AN_FORCEINLINE void STransform::ComputeTransformMatrix( Float3x4 & _LocalTransformMatrix ) const {
    _LocalTransformMatrix.Compose( Position, Rotation.ToMatrix(), Scale );
}

AN_FORCEINLINE void STransform::TurnRightFPS( float _DeltaAngleRad ) {
    TurnLeftFPS( -_DeltaAngleRad );
}

AN_FORCEINLINE void STransform::TurnLeftFPS( float _DeltaAngleRad ) {
    TurnAroundAxis( _DeltaAngleRad, Float3( 0,1,0 ) );
}

AN_FORCEINLINE void STransform::TurnUpFPS( float _DeltaAngleRad ) {
    TurnAroundAxis( _DeltaAngleRad, GetRightVector() );
}

AN_FORCEINLINE void STransform::TurnDownFPS( float _DeltaAngleRad ) {
    TurnUpFPS( -_DeltaAngleRad );
}

AN_FORCEINLINE void STransform::TurnAroundAxis( float _DeltaAngleRad, Float3 const & _NormalizedAxis ) {
    float s, c;

    Math::SinCos( _DeltaAngleRad * 0.5f, s, c );

    Rotation = Quat( c, s * _NormalizedAxis.X, s * _NormalizedAxis.Y, s * _NormalizedAxis.Z ) * Rotation;
    Rotation.NormalizeSelf();
}

AN_FORCEINLINE void STransform::TurnAroundVector( float _DeltaAngleRad, Float3 const & _Vector ) {
    TurnAroundAxis( _DeltaAngleRad, _Vector.Normalized() );
}

AN_FORCEINLINE void STransform::StepRight( float _Units ) {
    Step( GetRightVector() * _Units );
}

AN_FORCEINLINE void STransform::StepLeft( float _Units ) {
    Step( GetLeftVector() * _Units );
}

AN_FORCEINLINE void STransform::StepUp( float _Units ) {
    Step( GetUpVector() * _Units );
}

AN_FORCEINLINE void STransform::StepDown( float _Units ) {
    Step( GetDownVector() * _Units );
}

AN_FORCEINLINE void STransform::StepBack( float _Units ) {
    Step( GetBackVector() * _Units );
}

AN_FORCEINLINE void STransform::StepForward( float _Units ) {
    Step( GetForwardVector() * _Units );
}

AN_FORCEINLINE void STransform::Step( Float3 const & _Vector ) {
    Position += _Vector;
}

AN_FORCEINLINE STransform STransform::operator*( STransform const & _T ) const {
    Float3x4 M;
    ComputeTransformMatrix( M );
    return STransform( M * _T.Position, Rotation * _T.Rotation, Scale * _T.Scale );
}

AN_FORCEINLINE STransform STransform::Inversed() const {
    Float3x4 M;
    ComputeTransformMatrix( M );
    return STransform( M.Inversed().DecomposeTranslation(), Rotation.Inversed(), Float3(1.0) / Scale );
}

AN_FORCEINLINE void STransform::InverseSelf() {
    Float3x4 M;
    ComputeTransformMatrix( M );
    Position = M.Inversed().DecomposeTranslation();
    Rotation.InverseSelf();
    Scale = Float3(1.0f) / Scale;
}

AN_FORCEINLINE void STransform::Write( IBinaryStream & _Stream ) const {
    Position.Write( _Stream );
    Rotation.Write( _Stream );
    Scale.Write( _Stream );
}

AN_FORCEINLINE void STransform::Read( IBinaryStream & _Stream ) {
    Position.Read( _Stream );
    Rotation.Read( _Stream );
    Scale.Read( _Stream );
}
