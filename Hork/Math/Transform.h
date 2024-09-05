/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

HK_NAMESPACE_BEGIN

struct Transform
{
    Float3 Position;
    Quat   Rotation;
    Float3 Scale{1,1,1};

    Transform() = default;
    Transform(Float3 const& position, Quat const& rotation, Float3 const& scale);
    Transform(Float3 const& position, Quat const& rotation);
    Transform(Float3 const& position);
    void   Clear();
    void   SetIdentity();
    void   SetScale(Float3 const& scale);
    void   SetScale(float x, float y, float z);
    void   SetScale(float uniformScale);
    void   SetAngles(Angl const& angles);
    void   SetAngles(float pitch, float yaw, float roll);
    Angl   GetAngles() const;
    float  GetPitch() const;
    float  GetYaw() const;
    float  GetRoll() const;
    Float3 GetRightVector() const;
    Float3 GetLeftVector() const;
    Float3 GetUpVector() const;
    Float3 GetDownVector() const;
    Float3 GetBackVector() const;
    Float3 GetForwardVector() const;
    void   GetVectors(Float3* right, Float3* up, Float3* back) const;
    void   ComputeTransformMatrix(Float3x4& localTransformMatrix) const;
    void   TurnRightFPS(float angleDeltaInRadians);
    void   TurnLeftFPS(float angleDeltaInRadians);
    void   TurnUpFPS(float angleDeltaInRadians);
    void   TurnDownFPS(float angleDeltaInRadians);
    void   TurnAroundAxis(float angleDeltaInRadians, Float3 const& normalizedAxis);
    void   TurnAroundVector(float angleDeltaInRadians, Float3 const& vector);
    void   StepRight(float units);
    void   StepLeft(float units);
    void   StepUp(float units);
    void   StepDown(float units);
    void   StepBack(float units);
    void   StepForward(float units);
    void   Step(Float3 const& vector);
    Float3x4 ToMatrix() const;
    Float3x3 GetNormalMatrix() const;

    bool operator==(Transform const& rhs) const
    {
        return Position == rhs.Position && Rotation == rhs.Rotation && Scale == rhs.Scale;
    }

    bool operator!=(Transform const& rhs) const
    {
        return !operator==(rhs);
    }

    Transform operator*(Transform const& rhs) const;

    Transform Inversed() const;
    void      InverseSelf();

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const;
    void Read(IBinaryStreamReadInterface& stream);
};

HK_FORCEINLINE Transform::Transform(Float3 const& position, Quat const& rotation, Float3 const& scale) :
    Position(position), Rotation(rotation), Scale(scale)
{}

HK_FORCEINLINE Transform::Transform(Float3 const& position, Quat const& rotation) :
    Position(position), Rotation(rotation)
{}

HK_FORCEINLINE Transform::Transform(Float3 const& position) :
    Position(position)
{}

HK_FORCEINLINE void Transform::Clear()
{
    Position.X = Position.Y = Position.Z = 0;
    SetIdentity();
    SetScale(1);
}

HK_FORCEINLINE void Transform::SetIdentity()
{
    Rotation.SetIdentity();
}

HK_FORCEINLINE void Transform::SetScale(Float3 const& scale)
{
    Scale = scale;
}

HK_FORCEINLINE void Transform::SetScale(float x, float y, float z)
{
    Scale.X = x;
    Scale.Y = y;
    Scale.Z = z;
}

HK_FORCEINLINE void Transform::SetScale(float uniformScale)
{
    Scale.X = Scale.Y = Scale.Z = uniformScale;
}

HK_FORCEINLINE void Transform::SetAngles(Angl const& angles)
{
    Rotation = angles.ToQuat();
}

HK_FORCEINLINE void Transform::SetAngles(float pitch, float yaw, float roll)
{
    Rotation = Angl(pitch, yaw, roll).ToQuat();
}

HK_FORCEINLINE Angl Transform::GetAngles() const
{
    Angl angles;
    Rotation.ToAngles(angles.Pitch, angles.Yaw, angles.Roll);
    angles.Pitch = Math::Degrees(angles.Pitch);
    angles.Yaw   = Math::Degrees(angles.Yaw);
    angles.Roll  = Math::Degrees(angles.Roll);
    return angles;
}

HK_FORCEINLINE float Transform::GetPitch() const
{
    return Math::Degrees(Rotation.Pitch());
}

HK_FORCEINLINE float Transform::GetYaw() const
{
    return Math::Degrees(Rotation.Yaw());
}

HK_FORCEINLINE float Transform::GetRoll() const
{
    return Math::Degrees(Rotation.Roll());
}

HK_FORCEINLINE Float3 Transform::GetRightVector() const
{
    //return Math::ToMat3(Rotation)[0];

    const float qyy(Rotation.Y * Rotation.Y);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxz(Rotation.X * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qwy(Rotation.W * Rotation.Y);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3(1 - 2 * (qyy + qzz), 2 * (qxy + qwz), 2 * (qxz - qwy));
}

HK_FORCEINLINE Float3 Transform::GetLeftVector() const
{
    //return -Math::ToMat3(Rotation)[0];

    const float qyy(Rotation.Y * Rotation.Y);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxz(Rotation.X * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qwy(Rotation.W * Rotation.Y);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3(-1 + 2 * (qyy + qzz), -2 * (qxy + qwz), -2 * (qxz - qwy));
}

HK_FORCEINLINE Float3 Transform::GetUpVector() const
{
    //return Math::ToMat3(Rotation)[1];

    const float qxx(Rotation.X * Rotation.X);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3(2 * (qxy - qwz), 1 - 2 * (qxx + qzz), 2 * (qyz + qwx));
}

HK_FORCEINLINE Float3 Transform::GetDownVector() const
{
    //return -Math::ToMat3(Rotation)[1];

    const float qxx(Rotation.X * Rotation.X);
    const float qzz(Rotation.Z * Rotation.Z);
    const float qxy(Rotation.X * Rotation.Y);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwz(Rotation.W * Rotation.Z);

    return Float3(-2 * (qxy - qwz), -1 + 2 * (qxx + qzz), -2 * (qyz + qwx));
}

HK_FORCEINLINE Float3 Transform::GetBackVector() const
{
    //return Math::ToMat3(Rotation)[2];

    const float qxx(Rotation.X * Rotation.X);
    const float qyy(Rotation.Y * Rotation.Y);
    const float qxz(Rotation.X * Rotation.Z);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwy(Rotation.W * Rotation.Y);

    return Float3(2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx + qyy));
}

HK_FORCEINLINE Float3 Transform::GetForwardVector() const
{
    //return -Math::ToMat3(Rotation)[2];

    const float qxx(Rotation.X * Rotation.X);
    const float qyy(Rotation.Y * Rotation.Y);
    const float qxz(Rotation.X * Rotation.Z);
    const float qyz(Rotation.Y * Rotation.Z);
    const float qwx(Rotation.W * Rotation.X);
    const float qwy(Rotation.W * Rotation.Y);

    return Float3(-2 * (qxz + qwy), -2 * (qyz - qwx), -1 + 2 * (qxx + qyy));
}

HK_FORCEINLINE void Transform::GetVectors(Float3* right, Float3* up, Float3* back) const
{
    //if ( right ) *right = Math::ToMat3(Rotation)[0];
    //if ( up ) *up = Math::ToMat3(Rotation)[1];
    //if ( back ) *back = Math::ToMat3(Rotation)[2];
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

    if (right)
    {
        right->X = 1 - 2 * (qyy + qzz);
        right->Y = 2 * (qxy + qwz);
        right->Z = 2 * (qxz - qwy);
    }

    if (up)
    {
        up->X = 2 * (qxy - qwz);
        up->Y = 1 - 2 * (qxx + qzz);
        up->Z = 2 * (qyz + qwx);
    }

    if (back)
    {
        back->X = 2 * (qxz + qwy);
        back->Y = 2 * (qyz - qwx);
        back->Z = 1 - 2 * (qxx + qyy);
    }
}


HK_FORCEINLINE void Transform::ComputeTransformMatrix(Float3x4& localTransformMatrix) const
{
    localTransformMatrix.Compose(Position, Rotation.ToMatrix3x3(), Scale);
}

HK_FORCEINLINE void Transform::TurnRightFPS(float angleDeltaInRadians)
{
    TurnLeftFPS(-angleDeltaInRadians);
}

HK_FORCEINLINE void Transform::TurnLeftFPS(float angleDeltaInRadians)
{
    TurnAroundAxis(angleDeltaInRadians, Float3(0, 1, 0));
}

HK_FORCEINLINE void Transform::TurnUpFPS(float angleDeltaInRadians)
{
    TurnAroundAxis(angleDeltaInRadians, GetRightVector());
}

HK_FORCEINLINE void Transform::TurnDownFPS(float angleDeltaInRadians)
{
    TurnUpFPS(-angleDeltaInRadians);
}

HK_FORCEINLINE void Transform::TurnAroundAxis(float angleDeltaInRadians, Float3 const& normalizedAxis)
{
    float s, c;

    Math::SinCos(angleDeltaInRadians * 0.5f, s, c);

    Rotation = Quat(c, s * normalizedAxis.X, s * normalizedAxis.Y, s * normalizedAxis.Z) * Rotation;
    Rotation.NormalizeSelf();
}

HK_FORCEINLINE void Transform::TurnAroundVector(float angleDeltaInRadians, Float3 const& vector)
{
    TurnAroundAxis(angleDeltaInRadians, vector.Normalized());
}

HK_FORCEINLINE void Transform::StepRight(float units)
{
    Step(GetRightVector() * units);
}

HK_FORCEINLINE void Transform::StepLeft(float units)
{
    Step(GetLeftVector() * units);
}

HK_FORCEINLINE void Transform::StepUp(float units)
{
    Step(GetUpVector() * units);
}

HK_FORCEINLINE void Transform::StepDown(float units)
{
    Step(GetDownVector() * units);
}

HK_FORCEINLINE void Transform::StepBack(float units)
{
    Step(GetBackVector() * units);
}

HK_FORCEINLINE void Transform::StepForward(float units)
{
    Step(GetForwardVector() * units);
}

HK_FORCEINLINE void Transform::Step(Float3 const& vector)
{
    Position += vector;
}

HK_FORCEINLINE Float3x4 Transform::ToMatrix() const
{
    Float3x4 matrix;
    matrix.Compose(Position, Rotation.ToMatrix3x3(), Scale);
    return matrix;
}

HK_FORCEINLINE Float3x3 Transform::GetNormalMatrix() const
{
    Float3x3 normalMatrix;
    ToMatrix().DecomposeNormalMatrix(normalMatrix);
    return normalMatrix;
}

HK_FORCEINLINE Transform Transform::operator*(Transform const& rhs) const
{
    Float3x4 m;
    ComputeTransformMatrix(m);
    return Transform(m * rhs.Position, Rotation * rhs.Rotation, Scale * rhs.Scale);
}

HK_FORCEINLINE Float3 operator*(Transform const& lhs, Float3 const& rhs)
{
    return lhs.Position + lhs.Rotation * (lhs.Scale * rhs);
}

HK_FORCEINLINE Transform Transform::Inversed() const
{
    Float3x4 m;
    ComputeTransformMatrix(m);
    return Transform(m.Inversed().DecomposeTranslation(), Rotation.Inversed(), Float3(1.0) / Scale);
}

HK_FORCEINLINE void Transform::InverseSelf()
{
    Float3x4 m;
    ComputeTransformMatrix(m);
    Position = m.Inversed().DecomposeTranslation();
    Rotation.InverseSelf();
    Scale = Float3(1.0f) / Scale;
}

HK_FORCEINLINE void Transform::Write(IBinaryStreamWriteInterface& stream) const
{
    Position.Write(stream);
    Rotation.Write(stream);
    Scale.Write(stream);
}

HK_FORCEINLINE void Transform::Read(IBinaryStreamReadInterface& stream)
{
    Position.Read(stream);
    Rotation.Read(stream);
    Scale.Read(stream);
}

HK_NAMESPACE_END
