/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "VectorMath.h"

HK_NAMESPACE_BEGIN

struct Quat
{
    using ElementType = float;

    float X{0};
    float Y{0};
    float Z{0};
    float W{1};

    Quat() = default;
    explicit constexpr Quat(Float4 const& v) :
        X(v.X), Y(v.Y), Z(v.Z), W(v.W) {}
    constexpr Quat(float w, float x, float y, float z) :
        X(x), Y(y), Z(z), W(w) {}
    Quat(float pitchInRadians, float yawInRadians, float rollInRadians) { FromAngles(pitchInRadians, yawInRadians, rollInRadians); }

    float* ToPtr()
    {
        return &X;
    }

    const float* ToPtr() const
    {
        return &X;
    }

    float& operator[](int index)
    {
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }

    float const& operator[](int index) const
    {
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }

    bool operator==(Quat const& rhs) const { return X == rhs.X && Y == rhs.Y && Z == rhs.Z; }
    bool operator!=(Quat const& rhs) const { return !(operator==(rhs)); }

    Quat operator+() const
    {
        return *this;
    }
    Quat operator-() const
    {
        return Quat(-W, -X, -Y, -Z);
    }
    Quat operator+(Quat const& rhs) const
    {
        return Quat(W + rhs.W, X + rhs.X, Y + rhs.Y, Z + rhs.Z);
    }
    Quat operator-(Quat const& rhs) const
    {
        return Quat(W - rhs.W, X - rhs.X, Y - rhs.Y, Z - rhs.Z);
    }
    Quat operator*(Quat const& rhs) const
    {
        return Quat(W * rhs.W - X * rhs.X - Y * rhs.Y - Z * rhs.Z,
                    W * rhs.X + X * rhs.W + Y * rhs.Z - Z * rhs.Y,
                    W * rhs.Y + Y * rhs.W + Z * rhs.X - X * rhs.Z,
                    W * rhs.Z + Z * rhs.W + X * rhs.Y - Y * rhs.X);
    }
    constexpr Quat operator*(float rhs) const
    {
        return Quat(W * rhs, X * rhs, Y * rhs, Z * rhs);
    }
    constexpr Quat operator/(float rhs) const
    {
        return (*this) * (1.0f / rhs);
    }
    Quat& operator+=(Quat const& rhs)
    {
        X += rhs.X;
        Y += rhs.Y;
        Z += rhs.Z;
        W += rhs.W;
        return *this;
    }
    Quat& operator-=(Quat const& rhs)
    {
        X -= rhs.X;
        Y -= rhs.Y;
        Z -= rhs.Z;
        W -= rhs.W;
        return *this;
    }
    Quat& operator*=(Quat const& rhs)
    {
        const float TX = X;
        const float TY = Y;
        const float TZ = Z;
        const float TW = W;

        X = TW * rhs.X + TX * rhs.W + TY * rhs.Z - TZ * rhs.Y;
        Y = TW * rhs.Y + TY * rhs.W + TZ * rhs.X - TX * rhs.Z;
        Z = TW * rhs.Z + TZ * rhs.W + TX * rhs.Y - TY * rhs.X;
        W = TW * rhs.W - TX * rhs.X - TY * rhs.Y - TZ * rhs.Z;
        return *this;
    }
    Quat& operator*=(float rhs)
    {
        X *= rhs;
        Y *= rhs;
        Z *= rhs;
        W *= rhs;
        return *this;
    }
    Quat& operator/=(float rhs)
    {
        float denom = 1.0f / rhs;
        X *= denom;
        Y *= denom;
        Z *= denom;
        W *= denom;
        return *this;
    }

    Bool4 IsInfinite() const
    {
        return Bool4(Math::IsInfinite(X), Math::IsInfinite(Y), Math::IsInfinite(Z), Math::IsInfinite(W));
    }

    Bool4 IsNan() const
    {
        return Bool4(Math::IsNan(X), Math::IsNan(Y), Math::IsNan(Z), Math::IsNan(W));
    }

    Bool4 IsNormal() const
    {
        return Bool4(Math::IsNormal(X), Math::IsNormal(Y), Math::IsNormal(Z), Math::IsNormal(W));
    }

    bool CompareEps(Quat const& rhs, float epsilon) const
    {
        return Bool4(Math::Dist(X, rhs.X) < epsilon,
                     Math::Dist(Y, rhs.Y) < epsilon,
                     Math::Dist(Z, rhs.Z) < epsilon,
                     Math::Dist(W, rhs.W) < epsilon)
            .All();
    }

    float NormalizeSelf()
    {
        const float len = std::sqrt(X * X + Y * Y + Z * Z + W * W);
        if (len != 0.0f)
        {
            const float invLen = 1.0f / len;
            X *= invLen;
            Y *= invLen;
            Z *= invLen;
            W *= invLen;
        }
        return len;
    }

    Quat Normalized() const
    {
        const float len = std::sqrt(X * X + Y * Y + Z * Z + W * W);
        if (len != 0.0f)
        {
            const float invLen = 1.0f / len;
            return Quat(W * invLen, X * invLen, Y * invLen, Z * invLen);
        }
        return *this;
    }

    void InverseSelf()
    {
        const float DP = 1.0f / (X * X + Y * Y + Z * Z + W * W);

        X = -X * DP;
        Y = -Y * DP;
        Z = -Z * DP;
        W = W * DP;
    }

    Quat Inversed() const
    {
        return Conjugated() / (X * X + Y * Y + Z * Z + W * W);
    }

    void ConjugateSelf()
    {
        X = -X;
        Y = -Y;
        Z = -Z;
    }

    Quat Conjugated() const
    {
        return Quat(W, -X, -Y, -Z);
    }

    float ComputeW() const
    {
        return std::sqrt(Math::Abs(1.0f - (X * X + Y * Y + Z * Z)));
    }

    Float3 XAxis() const
    {
        float y2 = Y + Y, z2 = Z + Z;
        return Float3(1.0f - (Y * y2 + Z * z2), X * y2 + W * z2, X * z2 - W * y2);
    }

    Float3 YAxis() const
    {
        float x2 = X + X, y2 = Y + Y, z2 = Z + Z;
        return Float3(X * y2 - W * z2, 1.0f - (X * x2 + Z * z2), Y * z2 + W * x2);
    }

    Float3 ZAxis() const
    {
        float x2 = X + X, y2 = Y + Y, z2 = Z + Z;
        return Float3(X * z2 + W * y2, Y * z2 - W * x2, 1.0f - (X * x2 + Y * y2));
    }

    void SetIdentity()
    {
        X = Y = Z = 0;
        W         = 1;
    }

    // Return rotation around normalized axis
    static Quat sRotationAroundNormal(float angleInRadians, Float3 const& normal)
    {
        float s, c;
        Math::SinCos(angleInRadians * 0.5f, s, c);
        return Quat(c, s * normal.X, s * normal.Y, s * normal.Z);
    }

    // Return rotation around unnormalized vector
    static Quat sRotationAroundVector(float angleInRadians, Float3 const& vector)
    {
        return sRotationAroundNormal(angleInRadians, vector.Normalized());
    }

    // Return rotation around X axis
    static Quat sRotationX(float angleInRadians)
    {
        Quat Result;
        Math::SinCos(angleInRadians * 0.5f, Result.X, Result.W);
        Result.Y = 0;
        Result.Z = 0;
        return Result;
    }

    // Return rotation around Y axis
    static Quat sRotationY(float angleInRadians)
    {
        Quat Result;
        Math::SinCos(angleInRadians * 0.5f, Result.Y, Result.W);
        Result.X = 0;
        Result.Z = 0;
        return Result;
    }

    // Return rotation around Z axis
    static Quat sRotationZ(float angleInRadians)
    {
        Quat Result;
        Math::SinCos(angleInRadians * 0.5f, Result.Z, Result.W);
        Result.X = 0;
        Result.Y = 0;
        return Result;
    }

    Quat RotateAroundNormal(float angleInRadians, Float3 const& normal) const
    {
        float s, c;
        Math::SinCos(angleInRadians * 0.5f, s, c);
        return (Quat(c, s * normal.X, s * normal.Y, s * normal.Z) * (*this)).Normalized();
    }

    Quat RotateAroundVector(float angleInRadians, Float3 const& vector) const
    {
        return RotateAroundNormal(angleInRadians, vector.Normalized());
    }

    Float3 operator*(Float3 const& vec) const
    {
        Float3 c1(Math::Cross(*(Float3*)&X, vec));
        Float3 c2(Math::Cross(*(Float3*)&X, c1));

        return vec + 2.0f * (c1 * W + c2);
    }

    void ToAngles(float& pitchInRadians, float& yawInRadians, float& rollInRadians) const
    {
        const float xx = X * X;
        const float yy = Y * Y;
        const float zz = Z * Z;
        const float ww = W * W;

        pitchInRadians = std::atan2(2.0f * (Y * Z + W * X), ww - xx - yy + zz);
        yawInRadians   = std::asin(Math::Clamp(-2.0f * (X * Z - W * Y), -1.0f, 1.0f));
        rollInRadians  = std::atan2(2.0f * (X * Y + W * Z), ww + xx - yy - zz);
    }

    void FromAngles(float pitchInRadians, float yawInRadians, float rollInRadians)
    {
        float sx, sy, sz, cx, cy, cz;

        Math::SinCos(pitchInRadians * 0.5f, sx, cx);
        Math::SinCos(yawInRadians * 0.5f, sy, cy);
        Math::SinCos(rollInRadians * 0.5f, sz, cz);

        const float w = cy * cx;
        const float x = cy * sx;
        const float y = sy * cx;
        const float z = sy * sx;

        X = x * cz + y * sz;
        Y = -x * sz + y * cz;
        Z = w * sz - z * cz;
        W = w * cz + z * sz;
    }

    Float3x3 ToMatrix3x3() const
    {
        const float xx = X * X;
        const float yy = Y * Y;
        const float zz = Z * Z;
        const float xz = X * Z;
        const float xy = X * Y;
        const float yz = Y * Z;
        const float wx = W * X;
        const float wy = W * Y;
        const float wz = W * Z;

        return Float3x3(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy),
                        2.0f * (xy - wz), 1.0f - 2 * (xx + zz), 2.0f * (yz + wx),
                        2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy));
    }

    Float4x4 ToMatrix4x4() const
    {
        const float xx = X * X;
        const float yy = Y * Y;
        const float zz = Z * Z;
        const float xz = X * Z;
        const float xy = X * Y;
        const float yz = Y * Z;
        const float wx = W * X;
        const float wy = W * Y;
        const float wz = W * Z;

        return Float4x4(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0,
                        2.0f * (xy - wz), 1.0f - 2 * (xx + zz), 2.0f * (yz + wx), 0,
                        2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0,
                        0.0f, 0.0f, 0.0f, 1.0f);
    }

    void FromMatrix(Float3x3 const& matrix)
    {
        // Based on code from GLM
        const float fourXSquaredMinus1 = matrix[0][0] - matrix[1][1] - matrix[2][2];
        const float fourYSquaredMinus1 = matrix[1][1] - matrix[0][0] - matrix[2][2];
        const float fourZSquaredMinus1 = matrix[2][2] - matrix[0][0] - matrix[1][1];
        const float fourWSquaredMinus1 = matrix[0][0] + matrix[1][1] + matrix[2][2];

        int   biggestIndex             = 0;
        float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
        if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourXSquaredMinus1;
            biggestIndex             = 1;
        }
        if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourYSquaredMinus1;
            biggestIndex             = 2;
        }
        if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourZSquaredMinus1;
            biggestIndex             = 3;
        }

        const float biggestVal = std::sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
        const float mult       = 0.25f / biggestVal;

        switch (biggestIndex)
        {
            case 0:
                W = biggestVal;
                X = (matrix[1][2] - matrix[2][1]) * mult;
                Y = (matrix[2][0] - matrix[0][2]) * mult;
                Z = (matrix[0][1] - matrix[1][0]) * mult;
                break;
            case 1:
                W = (matrix[1][2] - matrix[2][1]) * mult;
                X = biggestVal;
                Y = (matrix[0][1] + matrix[1][0]) * mult;
                Z = (matrix[2][0] + matrix[0][2]) * mult;
                break;
            case 2:
                W = (matrix[2][0] - matrix[0][2]) * mult;
                X = (matrix[0][1] + matrix[1][0]) * mult;
                Y = biggestVal;
                Z = (matrix[1][2] + matrix[2][1]) * mult;
                break;
            case 3:
                W = (matrix[0][1] - matrix[1][0]) * mult;
                X = (matrix[2][0] + matrix[0][2]) * mult;
                Y = (matrix[1][2] + matrix[2][1]) * mult;
                Z = biggestVal;
                break;

            default: // Silence a -Wswitch-default warning in GCC. Should never actually get here. Assert is just for sanity.
                HK_ASSERT(false);
                break;
        }
    }

    float Pitch() const
    {
        return std::atan2(2.0f * (Y * Z + W * X), W * W - X * X - Y * Y + Z * Z);
    }

    float Yaw() const
    {
        return std::asin(Math::Clamp(-2.0f * (X * Z - W * Y), -1.0f, 1.0f));
    }

    float Roll() const
    {
        return std::atan2(2.0f * (X * Y + W * Z), W * W + X * X - Y * Y - Z * Z);
    }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteFloat(X);
        stream.WriteFloat(Y);
        stream.WriteFloat(Z);
        stream.WriteFloat(W);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        X = stream.ReadFloat();
        Y = stream.ReadFloat();
        Z = stream.ReadFloat();
        W = stream.ReadFloat();
        NormalizeSelf();
    }

    // Static methods
    static constexpr int sNumComponents() { return 4; }

    static Quat const& sZero()
    {
        static constexpr Quat ZeroQuat(0.0f, 0.0f, 0.0f, 0.0f);
        return ZeroQuat;
    }

    static Quat const& sIdentity()
    {
        static constexpr Quat Q(1, 0, 0, 0);
        return Q;
    }
};

HK_FORCEINLINE Quat operator*(float lhs, Quat const& rhs)
{
    return Quat(lhs * rhs.W, lhs * rhs.X, lhs * rhs.Y, lhs * rhs.Z);
}

namespace Math
{

Quat GetRotation(Float3 const& from, Float3 const& to);
Quat Slerp(Quat const& qs, Quat const& qe, float f);
Quat MakeRotationFromDir(Float3 const& direction);
void GetTransformVectors(Quat const& rotation, Float3* right, Float3* up, Float3* back);

} // namespace Math

HK_NAMESPACE_END

HK_FORMAT_DEF_(Hk::Quat, "( {} {} {} {} )", v.X, v.Y, v.Z, v.W);
