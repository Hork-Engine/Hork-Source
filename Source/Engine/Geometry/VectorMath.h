/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

template <typename T>
struct TVector2;

template <typename T>
struct TVector3;

template <typename T>
struct TVector4;

template <typename T>
struct TPlane;

enum AXIAL_TYPE
{
    AXIAL_TYPE_X = 0,
    AXIAL_TYPE_Y,
    AXIAL_TYPE_Z,
    AXIAL_TYPE_W,
    AXIAL_TYPE_NON_AXIAL
};

namespace Math
{

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr float Dot(TVector2<T> const& A, TVector2<T> const& B)
{
    return A.X * B.X + A.Y * B.Y;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr float Dot(TVector3<T> const& A, TVector3<T> const& B)
{
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr float Dot(TVector4<T> const& A, TVector4<T> const& B)
{
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr float Dot(TPlane<T> const& A, TVector3<T> const& B)
{
    return A.Normal.X * B.X + A.Normal.Y * B.Y + A.Normal.Z * B.Z + A.D;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr float Dot(TVector3<T> const& A, TPlane<T> const& B)
{
    return A.X * B.Normal.X + A.Y * B.Normal.Y + A.Z * B.Normal.Z + B.D;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr float Dot(TPlane<T> const& A, TVector4<T> const& B)
{
    return A.Normal.X * B.X + A.Normal.Y * B.Y + A.Normal.Z * B.Z + A.D * B.W;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr float Dot(TVector4<T> const& A, TPlane<T> const& B)
{
    return A.X * B.Normal.X + A.Y * B.Normal.Y + A.Z * B.Normal.Z + A.W * B.D;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr T Cross(TVector2<T> const& A, TVector2<T> const& B)
{
    return A.X * B.Y - A.Y * B.X;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr TVector3<T> Cross(TVector3<T> const& A, TVector3<T> const& B)
{
    return TVector3<T>(A.Y * B.Z - B.Y * A.Z,
                       A.Z * B.X - B.Z * A.X,
                       A.X * B.Y - B.X * A.Y);
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr TVector2<T> Reflect(TVector2<T> const& incidentVector, TVector2<T> const& normal)
{
    return incidentVector - 2 * Dot(normal, incidentVector) * normal;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr TVector3<T> Reflect(TVector3<T> const& incidentVector, TVector3<T> const& normal)
{
    return incidentVector - 2 * Dot(normal, incidentVector) * normal;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
AN_FORCEINLINE TVector2<T> Refract(TVector2<T> const& incidentVector, TVector2<T> const& normal, T const eta)
{
    const T NdotI = Dot(normal, incidentVector);
    const T k     = T(1) - eta * eta * (T(1) - NdotI * NdotI);
    if (k < T(0))
    {
        return TVector2<T>(T(0));
    }
    return incidentVector * eta - normal * (eta * NdotI + sqrt(k));
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
AN_FORCEINLINE TVector3<T> Refract(TVector3<T> const& incidentVector, TVector3<T> const& normal, const T eta)
{
    const T NdotI = Dot(normal, incidentVector);
    const T k     = T(1) - eta * eta * (T(1) - NdotI * NdotI);
    if (k < T(0))
    {
        return TVector3<T>(T(0));
    }
    return incidentVector * eta - normal * (eta * NdotI + sqrt(k));
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
TVector3<T> ProjectVector(TVector3<T> const& vector, TVector3<T> const& normal, T overbounce)
{
    return vector - normal * Dot(vector, normal) * overbounce;
}

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
TVector3<T> ProjectVector(TVector3<T> const& vector, TVector3<T> const& normal)
{
    return vector - normal * Dot(vector, normal);
}

template <typename T>
constexpr TVector2<T> Lerp(TVector2<T> const& s, TVector2<T> const& e, T f)
{
    return s + f * (e - s);
}

template <typename T>
constexpr TVector3<T> Lerp(TVector3<T> const& s, TVector3<T> const& e, T f)
{
    return s + f * (e - s);
}

template <typename T>
constexpr TVector4<T> Lerp(TVector4<T> const& s, TVector4<T> const& e, T f)
{
    return s + f * (e - s);
}

template <typename T>
constexpr T Bilerp(T _A, T _B, T _C, T _D, TVector2<T> const& lerp)
{
    return _A * (T(1) - lerp.X) * (T(1) - lerp.Y) + _B * lerp.X * (T(1) - lerp.Y) + _C * (T(1) - lerp.X) * lerp.Y + _D * lerp.X * lerp.Y;
}

template <typename T>
constexpr TVector2<T> Bilerp(TVector2<T> const& _A, TVector2<T> const& _B, TVector2<T> const& _C, TVector2<T> const& _D, TVector2<T> const& lerp)
{
    return _A * (T(1) - lerp.X) * (T(1) - lerp.Y) + _B * lerp.X * (T(1) - lerp.Y) + _C * (T(1) - lerp.X) * lerp.Y + _D * lerp.X * lerp.Y;
}

template <typename T>
constexpr TVector3<T> Bilerp(TVector3<T> const& _A, TVector3<T> const& _B, TVector3<T> const& _C, TVector3<T> const& _D, TVector2<T> const& lerp)
{
    return _A * (T(1) - lerp.X) * (T(1) - lerp.Y) + _B * lerp.X * (T(1) - lerp.Y) + _C * (T(1) - lerp.X) * lerp.Y + _D * lerp.X * lerp.Y;
}

template <typename T>
constexpr TVector4<T> Bilerp(TVector4<T> const& _A, TVector4<T> const& _B, TVector4<T> const& _C, TVector4<T> const& _D, TVector2<T> const& lerp)
{
    return _A * (T(1) - lerp.X) * (T(1) - lerp.Y) + _B * lerp.X * (T(1) - lerp.Y) + _C * (T(1) - lerp.X) * lerp.Y + _D * lerp.X * lerp.Y;
}

template <typename T>
constexpr TVector2<T> Step(TVector2<T> const& vec, T edge)
{
    return TVector2<T>(vec.X < edge ? T(0) : T(1), vec.Y < edge ? T(0) : T(1));
}

template <typename T>
constexpr TVector2<T> Step(TVector2<T> const& vec, TVector2<T> const& edge)
{
    return TVector2<T>(vec.X < edge.X ? T(0) : T(1), vec.Y < edge.Y ? T(0) : T(1));
}

template <typename T>
AN_FORCEINLINE TVector2<T> SmoothStep(TVector2<T> const& vec, T edge0, T edge1)
{
    const T           denom = T(1) / (edge1 - edge0);
    const TVector2<T> t     = Saturate((vec - edge0) * denom);
    return t * t * (T(-2) * t + T(3));
}

template <typename T>
AN_FORCEINLINE TVector2<T> SmoothStep(TVector2<T> const& vec, TVector2<T> const& edge0, TVector2<T> const& edge1)
{
    const TVector2<T> t = Saturate((vec - edge0) / (edge1 - edge0));
    return t * t * (T(-2) * t + T(3));
}

template <typename T>
constexpr TVector3<T> Step(TVector3<T> const& vec, T edge)
{
    return TVector3<T>(vec.X < edge ? T(0) : T(1), vec.Y < edge ? T(0) : T(1), vec.Z < edge ? T(0) : T(1));
}

template <typename T>
constexpr TVector3<T> Step(TVector3<T> const& vec, TVector3<T> const& edge)
{
    return TVector3<T>(vec.X < edge.X ? T(0) : T(1), vec.Y < edge.Y ? T(0) : T(1), vec.Z < edge.Z ? T(0) : T(1));
}

template <typename T>
AN_FORCEINLINE TVector3<T> SmoothStep(TVector3<T> const& vec, T edge0, T edge1)
{
    const T           denom = T(1) / (edge1 - edge0);
    const TVector3<T> t     = Saturate((vec - edge0) * denom);
    return t * t * (T(-2) * t + T(3));
}

template <typename T>
AN_FORCEINLINE TVector3<T> SmoothStep(TVector3<T> const& vec, TVector3<T> const& edge0, TVector3<T> const& edge1)
{
    const TVector3<T> t = Saturate((vec - edge0) / (edge1 - edge0));
    return t * t * (T(-2) * t + T(3));
}

template <typename T>
constexpr TVector4<T> Step(TVector4<T> const& vec, T edge)
{
    return TVector4<T>(vec.X < edge ? T(0) : T(1), vec.Y < edge ? T(0) : T(1), vec.Z < edge ? T(0) : T(1), vec.W < edge ? T(0) : T(1));
}

template <typename T>
constexpr TVector4<T> Step(TVector4<T> const& vec, TVector4<T> const& edge)
{
    return TVector4<T>(vec.X < edge.X ? T(0) : T(1), vec.Y < edge.Y ? T(0) : T(1), vec.Z < edge.Z ? T(0) : T(1), vec.W < edge.W ? T(0) : T(1));
}

template <typename T>
AN_FORCEINLINE TVector4<T> SmoothStep(TVector4<T> const& vec, T edge0, T edge1)
{
    const T           denom = T(1) / (edge1 - edge0);
    const TVector4<T> t     = Saturate((vec - edge0) * denom);
    return t * t * (T(-2) * t + T(3));
}

template <typename T>
AN_FORCEINLINE TVector4<T> SmoothStep(TVector4<T> const& vec, TVector4<T> const& edge0, TVector4<T> const& edge1)
{
    const TVector4<T> t = Saturate((vec - edge0) / (edge1 - edge0));
    return t * t * (T(-2) * t + T(3));
}

} // namespace Math

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector2<T> operator+(T lhs, TVector2<T> const& rhs)
{
    return TVector2<T>(lhs + rhs.X, lhs + rhs.Y);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector2<T> operator-(T lhs, TVector2<T> const& rhs)
{
    return TVector2<T>(lhs - rhs.X, lhs - rhs.Y);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector3<T> operator+(T lhs, TVector3<T> const& rhs)
{
    return TVector3<T>(lhs + rhs.X, lhs + rhs.Y, lhs + rhs.Z);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector3<T> operator-(T lhs, TVector3<T> const& rhs)
{
    return TVector3<T>(lhs - rhs.X, lhs - rhs.Y, lhs - rhs.Z);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector4<T> operator+(T lhs, TVector4<T> const& rhs)
{
    return TVector4<T>(lhs + rhs.X, lhs + rhs.Y, lhs + rhs.Z, lhs + rhs.W);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector4<T> operator-(T lhs, TVector4<T> const& rhs)
{
    return TVector4<T>(lhs - rhs.X, lhs - rhs.Y, lhs - rhs.Z, lhs - rhs.W);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector2<T> operator*(T lhs, TVector2<T> const& rhs)
{
    return TVector2<T>(lhs * rhs.X, lhs * rhs.Y);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector3<T> operator*(T lhs, TVector3<T> const& rhs)
{
    return TVector3<T>(lhs * rhs.X, lhs * rhs.Y, lhs * rhs.Z);
}

template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
constexpr TVector4<T> operator*(T lhs, TVector4<T> const& rhs)
{
    return TVector4<T>(lhs * rhs.X, lhs * rhs.Y, lhs * rhs.Z, lhs * rhs.W);
}

template <typename T>
struct TVector2
{
    using ElementType = T;

    T X;
    T Y;

    TVector2() = default;

    constexpr explicit TVector2(T v) :
        X(v), Y(v) {}

    constexpr TVector2(T x, T y) :
        X(x), Y(y) {}

    constexpr explicit TVector2(TVector3<T> const& v) :
        X(v.X), Y(v.Y) {}
    constexpr explicit TVector2(TVector4<T> const& v) :
        X(v.X), Y(v.Y) {}

    template <typename T2>
    constexpr explicit TVector2<T>(TVector2<T2> const& v) :
        X(v.X), Y(v.Y) {}

    template <typename T2>
    constexpr explicit TVector2<T>(TVector3<T2> const& v) :
        X(v.X), Y(v.Y) {}

    template <typename T2>
    constexpr explicit TVector2<T>(TVector4<T2> const& v) :
        X(v.X), Y(v.Y) {}

    T* ToPtr()
    {
        return &X;
    }

    T const* ToPtr() const
    {
        return &X;
    }

    T& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    T const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    template <int Index>
    constexpr T const& Get() const
    {
        static_assert(Index >= 0 && Index < NumComponents(), "Index out of range");
        return (&X)[Index];
    }

    template <int _Shuffle>
    constexpr TVector2<T> Shuffle2() const
    {
        return TVector2<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>());
    }

    template <int _Shuffle>
    constexpr TVector3<T> Shuffle3() const
    {
        return TVector3<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>());
    }

    template <int _Shuffle>
    constexpr TVector4<T> Shuffle4() const
    {
        return TVector4<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>(), Get<_Shuffle & 3>());
    }

    constexpr bool operator==(TVector2 const& rhs) const { return X == rhs.X && Y == rhs.Y; }
    constexpr bool operator!=(TVector2 const& rhs) const { return !(operator==(rhs)); }

    // Math operators
    constexpr TVector2 operator+() const
    {
        return *this;
    }
    constexpr TVector2 operator-() const
    {
        return TVector2(-X, -Y);
    }
    constexpr TVector2 operator+(TVector2 const& rhs) const
    {
        return TVector2(X + rhs.X, Y + rhs.Y);
    }
    constexpr TVector2 operator-(TVector2 const& rhs) const
    {
        return TVector2(X - rhs.X, Y - rhs.Y);
    }
    constexpr TVector2 operator*(TVector2 const& rhs) const
    {
        return TVector2(X * rhs.X, Y * rhs.Y);
    }
    constexpr TVector2 operator/(TVector2 const& rhs) const
    {
        return TVector2(X / rhs.X, Y / rhs.Y);
    }
    constexpr TVector2 operator+(T rhs) const
    {
        return TVector2(X + rhs, Y + rhs);
    }
    constexpr TVector2 operator-(T rhs) const
    {
        return TVector2(X - rhs, Y - rhs);
    }
    constexpr TVector2 operator*(T rhs) const
    {
        return TVector2(X * rhs, Y * rhs);
    }
    TVector2 operator/(T rhs) const
    {
        T denom = T(1) / rhs;
        return TVector2(X * denom, Y * denom);
    }
    TVector2& operator+=(TVector2 const& rhs)
    {
        X += rhs.X;
        Y += rhs.Y;
        return *this;
    }
    TVector2& operator-=(TVector2 const& rhs)
    {
        X -= rhs.X;
        Y -= rhs.Y;
        return *this;
    }
    TVector2& operator*=(TVector2 const& rhs)
    {
        X *= rhs.X;
        Y *= rhs.Y;
        return *this;
    }
    TVector2& operator/=(TVector2 const& rhs)
    {
        X /= rhs.X;
        Y /= rhs.Y;
        return *this;
    }
    TVector2& operator+=(T rhs)
    {
        X += rhs;
        Y += rhs;
        return *this;
    }
    TVector2& operator-=(T rhs)
    {
        X -= rhs;
        Y -= rhs;
        return *this;
    }
    TVector2& operator*=(T rhs)
    {
        X *= rhs;
        Y *= rhs;
        return *this;
    }
    TVector2& operator/=(T rhs)
    {
        T denom = T(1) / rhs;
        X *= denom;
        Y *= denom;
        return *this;
    }

    T Min() const
    {
        return Math::Min(X, Y);
    }

    T Max() const
    {
        return Math::Max(X, Y);
    }

    int MinorAxis() const
    {
        return int(Math::Abs(X) >= Math::Abs(Y));
    }

    int MajorAxis() const
    {
        return int(Math::Abs(X) < Math::Abs(Y));
    }

    // Floating point specific
    constexpr Bool2 IsInfinite() const
    {
        return Bool2(Math::IsInfinite(X), Math::IsInfinite(Y));
    }

    constexpr Bool2 IsNan() const
    {
        return Bool2(Math::IsNan(X), Math::IsNan(Y));
    }

    constexpr Bool2 IsNormal() const
    {
        return Bool2(Math::IsNormal(X), Math::IsNormal(Y));
    }

    constexpr Bool2 IsDenormal() const
    {
        return Bool2(Math::IsDenormal(X), Math::IsDenormal(Y));
    }

    constexpr bool CompareEps(TVector2 const& rhs, T epsilon) const
    {
        return Bool2(Math::CompareEps(X, rhs.X, epsilon),
                     Math::CompareEps(Y, rhs.Y, epsilon))
            .All();
    }

    void Clear()
    {
        X = Y = 0;
    }

    TVector2 Abs() const { return TVector2(Math::Abs(X), Math::Abs(Y)); }

    constexpr T LengthSqr() const
    {
        return X * X + Y * Y;
    }

    T Length() const
    {
        return std::sqrt(LengthSqr());
    }

    constexpr T DistSqr(TVector2 const& rhs) const
    {
        return (*this - rhs).LengthSqr();
    }

    T Dist(TVector2 const& rhs) const
    {
        return (*this - rhs).Length();
    }

    T NormalizeSelf()
    {
        T len = Length();
        if (len != T(0))
        {
            T invLen = T(1) / len;
            X *= invLen;
            Y *= invLen;
        }
        return len;
    }

    TVector2 Normalized() const
    {
        const T len = Length();
        if (len != T(0))
        {
            const T invLen = T(1) / len;
            return TVector2(X * invLen, Y * invLen);
        }
        return *this;
    }

    TVector2 Floor() const
    {
        return TVector2(Math::Floor(X), Math::Floor(Y));
    }

    TVector2 Ceil() const
    {
        return TVector2(Math::Ceil(X), Math::Ceil(Y));
    }

    TVector2 Fract() const
    {
        return TVector2(Math::Fract(X), Math::Fract(Y));
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector2 Sign() const
    {
        return TVector2(Math::Sign(X), Math::Sign(Y));
    }

    constexpr int SignBits() const
    {
        return Math::SignBits(X) | (Math::SignBits(Y) << 1);
    }

    TVector2 Snap(T snapVal) const
    {
        AN_ASSERT_(snapVal > 0, "Snap");
        TVector2<T> snapVector;
        snapVector   = *this / snapVal;
        snapVector.X = Math::Round(snapVector.X) * snapVal;
        snapVector.Y = Math::Round(snapVector.Y) * snapVal;
        return snapVector;
    }

    int NormalAxialType() const
    {
        if (X == T(1) || X == T(-1)) return AXIAL_TYPE_X;
        if (Y == T(1) || Y == T(-1)) return AXIAL_TYPE_Y;
        return AXIAL_TYPE_NON_AXIAL;
    }

    int NormalPositiveAxialType() const
    {
        if (X == T(1)) return AXIAL_TYPE_X;
        if (Y == T(1)) return AXIAL_TYPE_Y;
        return AXIAL_TYPE_NON_AXIAL;
    }

    int VectorAxialType() const
    {
        if (Math::Abs(X) < T(0.00001))
        {
            return (Math::Abs(Y) < T(0.00001)) ? AXIAL_TYPE_NON_AXIAL : AXIAL_TYPE_Y;
        }
        return (Math::Abs(Y) < T(0.00001)) ? AXIAL_TYPE_X : AXIAL_TYPE_NON_AXIAL;
    }

    // String conversions
    AString ToString(int precision = Math::FloatingPointPrecision<T>()) const
    {
        return AString("( ") + Math::ToString(X, precision) + " " + Math::ToString(Y, precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Math::ToHexString(X, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Y, bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {

        struct Writer
        {
            Writer(IBinaryStream& stream, const float& v)
            {
                stream.WriteFloat(v);
            }

            Writer(IBinaryStream& stream, const double& v)
            {
                stream.WriteDouble(v);
            }
        };
        Writer(stream, X);
        Writer(stream, Y);
    }

    void Read(IBinaryStream& stream)
    {

        struct Reader
        {
            Reader(IBinaryStream& stream, float& v)
            {
                v = stream.ReadFloat();
            }

            Reader(IBinaryStream& stream, double& v)
            {
                v = stream.ReadDouble();
            }
        };
        Reader(stream, X);
        Reader(stream, Y);
    }

    static constexpr int   NumComponents() { return 2; }
    static TVector2 const& Zero()
    {
        static constexpr TVector2<T> ZeroVec(T(0));
        return ZeroVec;
    }
};

template <typename T>
struct TVector3
{
    using ElementType = T;

    T X;
    T Y;
    T Z;

    TVector3() = default;

    constexpr explicit TVector3(T v) :
        X(v), Y(v), Z(v) {}

    constexpr TVector3(T x, T y, T z) :
        X(x), Y(y), Z(z) {}

    template <typename T2>
    constexpr explicit TVector3<T>(TVector2<T2> const& v, T2 z = T2(0)) :
        X(v.X), Y(v.Y), Z(z) {}

    template <typename T2>
    constexpr explicit TVector3<T>(TVector3<T2> const& v) :
        X(v.X), Y(v.Y), Z(v.Z) {}

    template <typename T2>
    constexpr explicit TVector3<T>(TVector4<T2> const& v) :
        X(v.X), Y(v.Y), Z(v.Z) {}

    T* ToPtr()
    {
        return &X;
    }

    T const* ToPtr() const
    {
        return &X;
    }

    T& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    T const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    template <int Index>
    constexpr T const& Get() const
    {
        static_assert(Index >= 0 && Index < NumComponents(), "Index out of range");
        return (&X)[Index];
    }

    template <int _Shuffle>
    constexpr TVector2<T> Shuffle2() const
    {
        return TVector2<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>());
    }

    template <int _Shuffle>
    constexpr TVector3<T> Shuffle3() const
    {
        return TVector3<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>());
    }

    template <int _Shuffle>
    constexpr TVector4<T> Shuffle4() const
    {
        return TVector4<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>(), Get<_Shuffle & 3>());
    }

    constexpr bool operator==(TVector3 const& rhs) const { return X == rhs.X && Y == rhs.Y && Z == rhs.Z; }
    constexpr bool operator!=(TVector3 const& rhs) const { return !(operator==(rhs)); }

    // Math operators
    constexpr TVector3 operator+() const
    {
        return *this;
    }
    constexpr TVector3 operator-() const
    {
        return TVector3(-X, -Y, -Z);
    }
    constexpr TVector3 operator+(TVector3 const& rhs) const
    {
        return TVector3(X + rhs.X, Y + rhs.Y, Z + rhs.Z);
    }
    constexpr TVector3 operator-(TVector3 const& rhs) const
    {
        return TVector3(X - rhs.X, Y - rhs.Y, Z - rhs.Z);
    }
    constexpr TVector3 operator*(TVector3 const& rhs) const
    {
        return TVector3(X * rhs.X, Y * rhs.Y, Z * rhs.Z);
    }
    constexpr TVector3 operator/(TVector3 const& rhs) const
    {
        return TVector3(X / rhs.X, Y / rhs.Y, Z / rhs.Z);
    }
    constexpr TVector3 operator+(T rhs) const
    {
        return TVector3(X + rhs, Y + rhs, Z + rhs);
    }
    constexpr TVector3 operator-(T rhs) const
    {
        return TVector3(X - rhs, Y - rhs, Z - rhs);
    }
    constexpr TVector3 operator*(T rhs) const
    {
        return TVector3(X * rhs, Y * rhs, Z * rhs);
    }
    TVector3 operator/(T rhs) const
    {
        T denom = T(1) / rhs;
        return TVector3(X * denom, Y * denom, Z * denom);
    }
    TVector3& operator+=(TVector3 const& rhs)
    {
        X += rhs.X;
        Y += rhs.Y;
        Z += rhs.Z;
        return *this;
    }
    TVector3& operator-=(TVector3 const& rhs)
    {
        X -= rhs.X;
        Y -= rhs.Y;
        Z -= rhs.Z;
        return *this;
    }
    TVector3& operator*=(TVector3 const& rhs)
    {
        X *= rhs.X;
        Y *= rhs.Y;
        Z *= rhs.Z;
        return *this;
    }
    TVector3& operator/=(TVector3 const& rhs)
    {
        X /= rhs.X;
        Y /= rhs.Y;
        Z /= rhs.Z;
        return *this;
    }
    TVector3& operator+=(T rhs)
    {
        X += rhs;
        Y += rhs;
        Z += rhs;
        return *this;
    }
    TVector3& operator-=(T rhs)
    {
        X -= rhs;
        Y -= rhs;
        Z -= rhs;
        return *this;
    }
    TVector3& operator*=(T rhs)
    {
        X *= rhs;
        Y *= rhs;
        Z *= rhs;
        return *this;
    }
    TVector3& operator/=(T rhs)
    {
        T denom = T(1) / rhs;
        X *= denom;
        Y *= denom;
        Z *= denom;
        return *this;
    }

    T Min() const
    {
        return Math::Min(Math::Min(X, Y), Z);
    }

    T Max() const
    {
        return Math::Max(Math::Max(X, Y), Z);
    }

    int MinorAxis() const
    {
        T   minor = Math::Abs(X);
        int axis  = 0;
        T   tmp;

        tmp = Math::Abs(Y);
        if (tmp <= minor)
        {
            axis  = 1;
            minor = tmp;
        }

        tmp = Math::Abs(Z);
        if (tmp <= minor)
        {
            axis  = 2;
            minor = tmp;
        }

        return axis;
    }

    int MajorAxis() const
    {
        T   major = Math::Abs(X);
        int axis  = 0;
        T   tmp;

        tmp = Math::Abs(Y);
        if (tmp > major)
        {
            axis  = 1;
            major = tmp;
        }

        tmp = Math::Abs(Z);
        if (tmp > major)
        {
            axis  = 2;
            major = tmp;
        }

        return axis;
    }

    // Floating point specific
    constexpr Bool3 IsInfinite() const
    {
        return Bool3(Math::IsInfinite(X), Math::IsInfinite(Y), Math::IsInfinite(Z));
    }

    constexpr Bool3 IsNan() const
    {
        return Bool3(Math::IsNan(X), Math::IsNan(Y), Math::IsNan(Z));
    }

    constexpr Bool3 IsNormal() const
    {
        return Bool3(Math::IsNormal(X), Math::IsNormal(Y), Math::IsNormal(Z));
    }

    constexpr Bool3 IsDenormal() const
    {
        return Bool3(Math::IsDenormal(X), Math::IsDenormal(Y), Math::IsDenormal(Z));
    }

    constexpr bool CompareEps(TVector3 const& rhs, T epsilon) const
    {
        return Bool3(Math::CompareEps(X, rhs.X, epsilon),
                     Math::CompareEps(Y, rhs.Y, epsilon),
                     Math::CompareEps(Z, rhs.Z, epsilon))
            .All();
    }

    void Clear()
    {
        X = Y = Z = 0;
    }

    TVector3 Abs() const { return TVector3(Math::Abs(X), Math::Abs(Y), Math::Abs(Z)); }

    constexpr T LengthSqr() const
    {
        return X * X + Y * Y + Z * Z;
    }
    T Length() const
    {
        return std::sqrt(LengthSqr());
    }
    constexpr T DistSqr(TVector3 const& rhs) const
    {
        return (*this - rhs).LengthSqr();
    }
    T Dist(TVector3 const& rhs) const
    {
        return (*this - rhs).Length();
    }
    T NormalizeSelf()
    {
        const T len = Length();
        if (len != T(0))
        {
            const T invLen = T(1) / len;
            X *= invLen;
            Y *= invLen;
            Z *= invLen;
        }
        return len;
    }
    TVector3 Normalized() const
    {
        const T len = Length();
        if (len != T(0))
        {
            const T invLen = T(1) / len;
            return TVector3(X * invLen, Y * invLen, Z * invLen);
        }
        return *this;
    }
    TVector3 NormalizeFix() const
    {
        TVector3 normal = Normalized();
        normal.FixNormal();
        return normal;
    }
    // Return true if normal was fixed
    bool FixNormal()
    {
        const T ZERO      = 0;
        const T ONE       = 1;
        const T MINUS_ONE = -1;

        if (X == -ZERO)
        {
            X = ZERO;
        }

        if (Y == -ZERO)
        {
            Y = ZERO;
        }

        if (Z == -ZERO)
        {
            Z = ZERO;
        }

        if (X == ZERO)
        {
            if (Y == ZERO)
            {
                if (Z > ZERO)
                {
                    if (Z != ONE)
                    {
                        Z = ONE;
                        return true;
                    }
                    return false;
                }
                if (Z != MINUS_ONE)
                {
                    Z = MINUS_ONE;
                    return true;
                }
                return false;
            }
            else if (Z == ZERO)
            {
                if (Y > ZERO)
                {
                    if (Y != ONE)
                    {
                        Y = ONE;
                        return true;
                    }
                    return false;
                }
                if (Y != MINUS_ONE)
                {
                    Y = MINUS_ONE;
                    return true;
                }
                return false;
            }
        }
        else if (Y == ZERO)
        {
            if (Z == ZERO)
            {
                if (X > ZERO)
                {
                    if (X != ONE)
                    {
                        X = ONE;
                        return true;
                    }
                    return false;
                }
                if (X != MINUS_ONE)
                {
                    X = MINUS_ONE;
                    return true;
                }
                return false;
            }
        }

        if (Math::Abs(X) == ONE)
        {
            if (Y != ZERO || Z != ZERO)
            {
                Y = Z = ZERO;
                return true;
            }
            return false;
        }

        if (Math::Abs(Y) == ONE)
        {
            if (X != ZERO || Z != ZERO)
            {
                X = Z = ZERO;
                return true;
            }
            return false;
        }

        if (Math::Abs(Z) == ONE)
        {
            if (X != ZERO || Y != ZERO)
            {
                X = Y = ZERO;
                return true;
            }
            return false;
        }

        return false;
    }

    TVector3 Floor() const
    {
        return TVector3(Math::Floor(X), Math::Floor(Y), Math::Floor(Z));
    }

    TVector3 Ceil() const
    {
        return TVector3(Math::Ceil(X), Math::Ceil(Y), Math::Ceil(Z));
    }

    TVector3 Fract() const
    {
        return TVector3(Math::Fract(X), Math::Fract(Y), Math::Fract(Z));
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector3 Sign() const
    {
        return TVector3(Math::Sign(X), Math::Sign(Y), Math::Sign(Z));
    }

    constexpr int SignBits() const
    {
        return Math::SignBits(X) | (Math::SignBits(Y) << 1) | (Math::SignBits(Z) << 2);
    }

    TVector3 Snap(T snapVal) const
    {
        AN_ASSERT_(snapVal > 0, "Snap");
        TVector3<T> snapVector;
        snapVector   = *this / snapVal;
        snapVector.X = Math::Round(snapVector.X) * snapVal;
        snapVector.Y = Math::Round(snapVector.Y) * snapVal;
        snapVector.Z = Math::Round(snapVector.Z) * snapVal;
        return snapVector;
    }

    TVector3 SnapNormal(T epsilon) const
    {
        TVector3<T> normal = *this;
        for (int i = 0; i < 3; i++)
        {
            if (Math::Abs(normal[i] - T(1)) < epsilon)
            {
                normal    = TVector3(0);
                normal[i] = 1;
                break;
            }
            if (Math::Abs(normal[i] - T(-1)) < epsilon)
            {
                normal    = TVector3(0);
                normal[i] = -1;
                break;
            }
        }

        if (Math::Abs(normal[0]) < epsilon && Math::Abs(normal[1]) >= epsilon && Math::Abs(normal[2]) >= epsilon)
        {
            normal[0] = 0;
            normal.NormalizeSelf();
        }
        else if (Math::Abs(normal[1]) < epsilon && Math::Abs(normal[0]) >= epsilon && Math::Abs(normal[2]) >= epsilon)
        {
            normal[1] = 0;
            normal.NormalizeSelf();
        }
        else if (Math::Abs(normal[2]) < epsilon && Math::Abs(normal[0]) >= epsilon && Math::Abs(normal[1]) >= epsilon)
        {
            normal[2] = 0;
            normal.NormalizeSelf();
        }

        return normal;
    }

    int NormalAxialType() const
    {
        if (X == T(1) || X == T(-1)) return AXIAL_TYPE_X;
        if (Y == T(1) || Y == T(-1)) return AXIAL_TYPE_Y;
        if (Z == T(1) || Z == T(-1)) return AXIAL_TYPE_Z;
        return AXIAL_TYPE_NON_AXIAL;
    }

    int NormalPositiveAxialType() const
    {
        if (X == T(1)) return AXIAL_TYPE_X;
        if (Y == T(1)) return AXIAL_TYPE_Y;
        if (Z == T(1)) return AXIAL_TYPE_Z;
        return AXIAL_TYPE_NON_AXIAL;
    }

    int VectorAxialType() const
    {
        bool  zeroX = Math::Abs(X) < T(0.00001);
        bool  zeroY = Math::Abs(Y) < T(0.00001);
        bool  zeroZ = Math::Abs(Z) < T(0.00001);

        if (int(zeroX + zeroY + zeroZ) != 2)
        {
            return AXIAL_TYPE_NON_AXIAL;
        }

        if (!zeroX)
        {
            return AXIAL_TYPE_X;
        }

        if (!zeroY)
        {
            return AXIAL_TYPE_Y;
        }

        if (!zeroZ)
        {
            return AXIAL_TYPE_Z;
        }

        return AXIAL_TYPE_NON_AXIAL;
    }

    TVector3 Perpendicular() const
    {
        T dp = X * X + Y * Y;
        if (!dp)
        {
            return TVector3(1, 0, 0);
        }
        else
        {
            dp = Math::InvSqrt(dp);
            return TVector3(-Y * dp, X * dp, 0);
        }
    }

    void ComputeBasis(TVector3& xvec, TVector3& yvec) const
    {
        yvec = Perpendicular();
        xvec = Math::Cross(yvec, *this);
    }

    // String conversions
    AString ToString(int precision = Math::FloatingPointPrecision<T>()) const
    {
        return AString("( ") + Math::ToString(X, precision) + " " + Math::ToString(Y, precision) + " " + Math::ToString(Z, precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Math::ToHexString(X, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Y, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Z, bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        struct Writer
        {
            Writer(IBinaryStream& stream, const float& v)
            {
                stream.WriteFloat(v);
            }

            Writer(IBinaryStream& stream, const double& v)
            {
                stream.WriteDouble(v);
            }
        };
        Writer(stream, X);
        Writer(stream, Y);
        Writer(stream, Z);
    }

    void Read(IBinaryStream& stream)
    {
        struct Reader
        {
            Reader(IBinaryStream& stream, float& v)
            {
                v = stream.ReadFloat();
            }

            Reader(IBinaryStream& stream, double& v)
            {
                v = stream.ReadDouble();
            }
        };
        Reader(stream, X);
        Reader(stream, Y);
        Reader(stream, Z);
    }

    static constexpr int   NumComponents() { return 3; }
    static TVector3 const& Zero()
    {
        static constexpr TVector3 ZeroVec(T(0));
        return ZeroVec;
    }
};

template <typename T>
struct TVector4
{
    using ElementType = T;

    T X;
    T Y;
    T Z;
    T W;

    TVector4() = default;

    constexpr explicit TVector4(T v) :
        X(v), Y(v), Z(v), W(v) {}

    constexpr TVector4(T x, T y, T z, T w) :
        X(x), Y(y), Z(z), W(w) {}

    template <typename T2>
    constexpr explicit TVector4<T>(TVector2<T2> const& v) :
        X(v.X), Y(v.Y), Z(0), W(0) {}

    template <typename T2>
    constexpr explicit TVector4<T>(TVector2<T2> const& v, T2 z, T2 w) :
        X(v.X), Y(v.Y), Z(z), W(w) {}

    template <typename T2>
    constexpr explicit TVector4<T>(TVector3<T2> const& v, T w = T(0)) :
        X(v.X), Y(v.Y), Z(v.Z), W(w) {}

    template <typename T2>
    constexpr explicit TVector4<T>(TVector4<T2> const& v) :
        X(v.X), Y(v.Y), Z(v.Z), W(v.W) {}

    T* ToPtr()
    {
        return &X;
    }

    T const* ToPtr() const
    {
        return &X;
    }

    T& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    T const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    template <int Index>
    constexpr T const& Get() const
    {
        static_assert(Index >= 0 && Index < NumComponents(), "Index out of range");
        return (&X)[Index];
    }

    template <int _Shuffle>
    constexpr TVector2<T> Shuffle2() const
    {
        return TVector2<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>());
    }

    template <int _Shuffle>
    constexpr TVector3<T> Shuffle3() const
    {
        return TVector3<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>());
    }

    template <int _Shuffle>
    constexpr TVector4<T> Shuffle4() const
    {
        return TVector4<T>(Get<(_Shuffle >> 6)>(), Get<(_Shuffle >> 4) & 3>(), Get<(_Shuffle >> 2) & 3>(), Get<_Shuffle & 3>());
    }

    constexpr bool operator==(TVector4 const& rhs) const { return X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W; }
    constexpr bool operator!=(TVector4 const& rhs) const { return !(operator==(rhs)); }

    // Math operators
    constexpr TVector4 operator+() const
    {
        return *this;
    }
    constexpr TVector4 operator-() const
    {
        return TVector4(-X, -Y, -Z, -W);
    }
    constexpr TVector4 operator+(TVector4 const& rhs) const
    {
        return TVector4(X + rhs.X, Y + rhs.Y, Z + rhs.Z, W + rhs.W);
    }
    constexpr TVector4 operator-(TVector4 const& rhs) const
    {
        return TVector4(X - rhs.X, Y - rhs.Y, Z - rhs.Z, W - rhs.W);
    }
    constexpr TVector4 operator*(TVector4 const& rhs) const
    {
        return TVector4(X * rhs.X, Y * rhs.Y, Z * rhs.Z, W * rhs.W);
    }
    constexpr TVector4 operator/(TVector4 const& rhs) const
    {
        return TVector4(X / rhs.X, Y / rhs.Y, Z / rhs.Z, W / rhs.W);
    }
    constexpr TVector4 operator+(T rhs) const
    {
        return TVector4(X + rhs, Y + rhs, Z + rhs, W + rhs);
    }
    constexpr TVector4 operator-(T rhs) const
    {
        return TVector4(X - rhs, Y - rhs, Z - rhs, W - rhs);
    }
    constexpr TVector4 operator*(T rhs) const
    {
        return TVector4(X * rhs, Y * rhs, Z * rhs, W * rhs);
    }
    TVector4 operator/(T rhs) const
    {
        T denom = T(1) / rhs;
        return TVector4(X * denom, Y * denom, Z * denom, W * denom);
    }
    TVector4& operator+=(TVector4 const& rhs)
    {
        X += rhs.X;
        Y += rhs.Y;
        Z += rhs.Z;
        W += rhs.W;
        return *this;
    }
    TVector4& operator-=(TVector4 const& rhs)
    {
        X -= rhs.X;
        Y -= rhs.Y;
        Z -= rhs.Z;
        W -= rhs.W;
        return *this;
    }
    TVector4& operator*=(TVector4 const& rhs)
    {
        X *= rhs.X;
        Y *= rhs.Y;
        Z *= rhs.Z;
        W *= rhs.W;
        return *this;
    }
    TVector4& operator/=(TVector4 const& rhs)
    {
        X /= rhs.X;
        Y /= rhs.Y;
        Z /= rhs.Z;
        W /= rhs.W;
        return *this;
    }
    TVector4& operator+=(T rhs)
    {
        X += rhs;
        Y += rhs;
        Z += rhs;
        W += rhs;
        return *this;
    }
    TVector4& operator-=(T rhs)
    {
        X -= rhs;
        Y -= rhs;
        Z -= rhs;
        W -= rhs;
        return *this;
    }
    TVector4& operator*=(T rhs)
    {
        X *= rhs;
        Y *= rhs;
        Z *= rhs;
        W *= rhs;
        return *this;
    }
    TVector4& operator/=(T rhs)
    {
        T denom = T(1) / rhs;
        X *= denom;
        Y *= denom;
        Z *= denom;
        W *= denom;
        return *this;
    }

    T Min() const
    {
        return Math::Min(Math::Min(Math::Min(X, Y), Z), W);
    }

    T Max() const
    {
        return Math::Max(Math::Max(Math::Max(X, Y), Z), W);
    }

    int MinorAxis() const
    {
        T   minor = Math::Abs(X);
        int axis  = 0;
        T   tmp;

        tmp = Math::Abs(Y);
        if (tmp <= minor)
        {
            axis  = 1;
            minor = tmp;
        }

        tmp = Math::Abs(Z);
        if (tmp <= minor)
        {
            axis  = 2;
            minor = tmp;
        }

        tmp = Math::Abs(W);
        if (tmp <= minor)
        {
            axis  = 3;
            minor = tmp;
        }

        return axis;
    }

    int MajorAxis() const
    {
        T   major = Math::Abs(X);
        int axis  = 0;
        T   tmp;

        tmp = Math::Abs(Y);
        if (tmp > major)
        {
            axis  = 1;
            major = tmp;
        }

        tmp = Math::Abs(Z);
        if (tmp > major)
        {
            axis  = 2;
            major = tmp;
        }

        tmp = Math::Abs(W);
        if (tmp > major)
        {
            axis  = 3;
            major = tmp;
        }

        return axis;
    }

    // Floating point specific
    constexpr Bool4 IsInfinite() const
    {
        return Bool4(Math::IsInfinite(X), Math::IsInfinite(Y), Math::IsInfinite(Z), Math::IsInfinite(W));
    }

    constexpr Bool4 IsNan() const
    {
        return Bool4(Math::IsNan(X), Math::IsNan(Y), Math::IsNan(Z), Math::IsNan(W));
    }

    constexpr Bool4 IsNormal() const
    {
        return Bool4(Math::IsNormal(X), Math::IsNormal(Y), Math::IsNormal(Z), Math::IsNormal(W));
    }

    constexpr Bool4 IsDenormal() const
    {
        return Bool4(Math::IsDenormal(X), Math::IsDenormal(Y), Math::IsDenormal(Z), Math::IsDenormal(W));
    }

    constexpr bool CompareEps(TVector4 const& rhs, T epsilon) const
    {
        return Bool4(Math::CompareEps(X, rhs.X, epsilon),
                     Math::CompareEps(Y, rhs.Y, epsilon),
                     Math::CompareEps(Z, rhs.Z, epsilon),
                     Math::CompareEps(W, rhs.W, epsilon))
            .All();
    }

    void Clear()
    {
        X = Y = Z = W = 0;
    }

    TVector4 Abs() const { return TVector4(Math::Abs(X), Math::Abs(Y), Math::Abs(Z), Math::Abs(W)); }

    constexpr T LengthSqr() const
    {
        return X * X + Y * Y + Z * Z + W * W;
    }
    T Length() const
    {
        return std::sqrt(LengthSqr());
    }
    constexpr T DistSqr(TVector4 const& rhs) const
    {
        return (*this - rhs).LengthSqr();
    }
    T Dist(TVector4 const& rhs) const
    {
        return (*this - rhs).Length();
    }
    T NormalizeSelf()
    {
        const T len = Length();
        if (len != T(0))
        {
            const T invLen = T(1) / len;
            X *= invLen;
            Y *= invLen;
            Z *= invLen;
            W *= invLen;
        }
        return len;
    }
    TVector4 Normalized() const
    {
        const T len = Length();
        if (len != T(0))
        {
            const T invLen = T(1) / len;
            return TVector4(X * invLen, Y * invLen, Z * invLen, W * invLen);
        }
        return *this;
    }

    TVector4 Floor() const
    {
        return TVector4(Math::Floor(X), Math::Floor(Y), Math::Floor(Z), Math::Floor(W));
    }

    TVector4 Ceil() const
    {
        return TVector4(Math::Ceil(X), Math::Ceil(Y), Math::Ceil(Z), Math::Ceil(W));
    }

    TVector4 Fract() const
    {
        return TVector4(Math::Fract(X), Math::Fract(Y), Math::Fract(Z), Math::Fract(W));
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    constexpr TVector4 Sign() const
    {
        return TVector4(Math::Sign(X), Math::Sign(Y), Math::Sign(Z), Math::Sign(W));
    }

    constexpr int SignBits() const
    {
        return Math::SignBits(X) | (Math::SignBits(Y) << 1) | (Math::SignBits(Z) << 2) | (Math::SignBits(W) << 3);
    }

    //TVector4 Clamp( T _Min, T _Max ) const {
    //    return TVector4( Math::Clamp( X, _Min, _Max ), Math::Clamp( Y, _Min, _Max ), Math::Clamp( Z, _Min, _Max ), Math::Clamp( W, _Min, _Max ) );
    //}

    //TVector4 Clamp( TVector4 const & _Min, TVector4 const & _Max ) const {
    //    return TVector4( Math::Clamp( X, _Min.X, _Max.X ), Math::Clamp( Y, _Min.Y, _Max.Y ), Math::Clamp( Z, _Min.Z, _Max.Z ), Math::Clamp( W, _Min.W, _Max.W ) );
    //}

    //TVector4 Saturate() const {
    //    return Clamp( T( 0 ), T( 1 ) );
    //}

    TVector4 Snap(T snapVal) const
    {
        AN_ASSERT_(snapVal > 0, "Snap");
        TVector4<T> snapVector;
        snapVector   = *this / snapVal;
        snapVector.X = Math::Round(snapVector.X) * snapVal;
        snapVector.Y = Math::Round(snapVector.Y) * snapVal;
        snapVector.Z = Math::Round(snapVector.Z) * snapVal;
        snapVector.W = Math::Round(snapVector.W) * snapVal;
        return snapVector;
    }

    int NormalAxialType() const
    {
        if (X == T(1) || X == T(-1)) return AXIAL_TYPE_X;
        if (Y == T(1) || Y == T(-1)) return AXIAL_TYPE_Y;
        if (Z == T(1) || Z == T(-1)) return AXIAL_TYPE_Z;
        if (W == T(1) || W == T(-1)) return AXIAL_TYPE_W;
        return AXIAL_TYPE_NON_AXIAL;
    }

    int NormalPositiveAxialType() const
    {
        if (X == T(1)) return AXIAL_TYPE_X;
        if (Y == T(1)) return AXIAL_TYPE_Y;
        if (Z == T(1)) return AXIAL_TYPE_Z;
        if (W == T(1)) return AXIAL_TYPE_W;
        return AXIAL_TYPE_NON_AXIAL;
    }

    int VectorAxialType() const
    {
        bool zeroX = Math::Abs(X) < T(0.00001);
        bool zeroY = Math::Abs(Y) < T(0.00001);
        bool zeroZ = Math::Abs(Z) < T(0.00001);
        bool zeroW = Math::Abs(W) < T(0.00001);

        if (int(zeroX + zeroY + zeroZ + zeroW) != 3)
        {
            return AXIAL_TYPE_NON_AXIAL;
        }

        if (!zeroX)
        {
            return AXIAL_TYPE_X;
        }

        if (!zeroY)
        {
            return AXIAL_TYPE_Y;
        }

        if (!zeroZ)
        {
            return AXIAL_TYPE_Z;
        }

        if (!zeroW)
        {
            return AXIAL_TYPE_W;
        }

        return AXIAL_TYPE_NON_AXIAL;
    }

    // String conversions
    AString ToString(int precision = Math::FloatingPointPrecision<T>()) const
    {
        return AString("( ") + Math::ToString(X, precision) + " " + Math::ToString(Y, precision) + " " + Math::ToString(Z, precision) + " " + Math::ToString(W, precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Math::ToHexString(X, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Y, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Z, bLeadingZeros, bPrefix) + " " + Math::ToHexString(W, bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {

        struct Writer
        {
            Writer(IBinaryStream& stream, const float& v)
            {
                stream.WriteFloat(v);
            }

            Writer(IBinaryStream& stream, const double& v)
            {
                stream.WriteDouble(v);
            }
        };
        Writer(stream, X);
        Writer(stream, Y);
        Writer(stream, Z);
        Writer(stream, W);
    }

    void Read(IBinaryStream& stream)
    {

        struct Reader
        {
            Reader(IBinaryStream& stream, float& v)
            {
                v = stream.ReadFloat();
            }

            Reader(IBinaryStream& stream, double& v)
            {
                v = stream.ReadDouble();
            }
        };
        Reader(stream, X);
        Reader(stream, Y);
        Reader(stream, Z);
        Reader(stream, W);
    }

    static constexpr int   NumComponents() { return 4; }
    static TVector4 const& Zero()
    {
        static constexpr TVector4 ZeroVec(T(0));
        return ZeroVec;
    }
};

namespace Math
{

AN_FORCEINLINE TVector2<float> Min(TVector2<float> const& a, TVector2<float> const& b)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_set_ps(a.X, a.Y, 0.0f, 0.0f), _mm_set_ps(b.X, b.Y, 0.0f, 0.0f)));

    return TVector2<float>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<float> Min(TVector3<float> const& a, TVector3<float> const& b)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_set_ps(a.X, a.Y, a.Z, 0.0f), _mm_set_ps(b.X, b.Y, b.Z, 0.0f)));

    return TVector3<float>(result[0], result[1], result[2]);
}

AN_FORCEINLINE TVector4<float> Min(TVector4<float> const& a, TVector4<float> const& b)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_set_ps(a.X, a.Y, a.Z, a.W), _mm_set_ps(b.X, b.Y, b.Z, b.W)));

    return TVector4<float>(result[0], result[1], result[2], result[3]);
}

AN_FORCEINLINE TVector2<float> Max(TVector2<float> const& a, TVector2<float> const& b)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_max_ps(_mm_set_ps(a.X, a.Y, 0.0f, 0.0f), _mm_set_ps(b.X, b.Y, 0.0f, 0.0f)));

    return TVector2<float>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<float> Max(TVector3<float> const& a, TVector3<float> const& b)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_max_ps(_mm_set_ps(a.X, a.Y, a.Z, 0.0f), _mm_set_ps(b.X, b.Y, b.Z, 0.0f)));

    return TVector3<float>(result[0], result[1], result[2]);
}

AN_FORCEINLINE TVector4<float> Max(TVector4<float> const& a, TVector4<float> const& b)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_max_ps(_mm_set_ps(a.X, a.Y, a.Z, a.W), _mm_set_ps(b.X, b.Y, b.Z, b.W)));

    return TVector4<float>(result[0], result[1], result[2], result[3]);
}

AN_FORCEINLINE TVector2<float> Clamp(TVector2<float> const& val, TVector2<float> const& minval, TVector2<float> const& maxval)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_max_ps(_mm_set_ps(val.X, val.Y, 0.0f, 0.0f), _mm_set_ps(minval.X, minval.Y, 0.0f, 0.0f)), _mm_set_ps(maxval.X, maxval.Y, 0.0f, 0.0f)));

    return TVector2<float>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<float> Clamp(TVector3<float> const& val, TVector3<float> const& minval, TVector3<float> const& maxval)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_max_ps(_mm_set_ps(val.X, val.Y, val.Z, 0.0f), _mm_set_ps(minval.X, minval.Y, minval.Z, 0.0f)), _mm_set_ps(maxval.X, maxval.Y, maxval.Z, 0.0f)));

    return TVector3<float>(result[0], result[1], result[2]);
}

AN_FORCEINLINE TVector4<float> Clamp(TVector4<float> const& val, TVector4<float> const& minval, TVector4<float> const& maxval)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_max_ps(_mm_set_ps(val.X, val.Y, val.Z, val.W), _mm_set_ps(minval.X, minval.Y, minval.Z, minval.W)), _mm_set_ps(maxval.X, maxval.Y, maxval.Z, maxval.W)));

    return TVector4<float>(result[0], result[1], result[2], result[3]);
}

AN_FORCEINLINE TVector2<float> Saturate(TVector2<float> const& val)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_max_ps(_mm_set_ps(val.X, val.Y, 0.0f, 0.0f), _mm_setzero_ps()), _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f)));

    return TVector2<float>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<float> Saturate(TVector3<float> const& val)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_max_ps(_mm_set_ps(val.X, val.Y, val.Z, 0.0f), _mm_setzero_ps()), _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f)));

    return TVector3<float>(result[0], result[1], result[2]);
}

AN_FORCEINLINE TVector4<float> Saturate(TVector4<float> const& val)
{
    alignas(16) float result[4];

    _mm_storer_ps(result, _mm_min_ps(_mm_max_ps(_mm_set_ps(val.X, val.Y, val.Z, val.W), _mm_setzero_ps()), _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f)));

    return TVector4<float>(result[0], result[1], result[2], result[3]);
}

AN_FORCEINLINE TVector2<double> Min(TVector2<double> const& a, TVector2<double> const& b)
{
    alignas(16) double result[2];

    _mm_storer_pd(result, _mm_min_pd(_mm_set_pd(a.X, a.Y), _mm_set_pd(b.X, b.Y)));

    return TVector2<double>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<double> Min(TVector3<double> const& a, TVector3<double> const& b)
{
    alignas(16) double result[2];
    double             z;

    _mm_storer_pd(result, _mm_min_pd(_mm_set_pd(a.X, a.Y), _mm_set_pd(b.X, b.Y)));
    _mm_store_sd(&z, _mm_min_sd(_mm_set_sd(a.Z), _mm_set_sd(b.Z)));

    return TVector3<double>(result[0], result[1], z);
}

AN_FORCEINLINE TVector4<double> Min(TVector4<double> const& a, TVector4<double> const& b)
{
    alignas(16) double result1[2];
    alignas(16) double result2[2];

    _mm_storer_pd(result1, _mm_min_pd(_mm_set_pd(a.X, a.Y), _mm_set_pd(b.X, b.Y)));
    _mm_storer_pd(result2, _mm_min_pd(_mm_set_pd(a.Z, a.W), _mm_set_pd(b.Z, b.W)));

    return TVector4<double>(result1[0], result1[1], result2[0], result2[1]);
}

AN_FORCEINLINE TVector2<double> Max(TVector2<double> const& a, TVector2<double> const& b)
{
    alignas(16) double result[2];

    _mm_storer_pd(result, _mm_max_pd(_mm_set_pd(a.X, a.Y), _mm_set_pd(b.X, b.Y)));

    return TVector2<double>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<double> Max(TVector3<double> const& a, TVector3<double> const& b)
{
    alignas(16) double result[2];
    double             z;

    _mm_storer_pd(result, _mm_max_pd(_mm_set_pd(a.X, a.Y), _mm_set_pd(b.X, b.Y)));
    _mm_store_sd(&z, _mm_max_sd(_mm_set_sd(a.Z), _mm_set_sd(b.Z)));

    return TVector3<double>(result[0], result[1], z);
}

AN_FORCEINLINE TVector4<double> Max(TVector4<double> const& a, TVector4<double> const& b)
{
    alignas(16) double result1[2];
    alignas(16) double result2[2];

    _mm_storer_pd(result1, _mm_max_pd(_mm_set_pd(a.X, a.Y), _mm_set_pd(b.X, b.Y)));
    _mm_storer_pd(result2, _mm_max_pd(_mm_set_pd(a.Z, a.W), _mm_set_pd(b.Z, b.W)));

    return TVector4<double>(result1[0], result1[1], result2[0], result2[1]);
}

AN_FORCEINLINE TVector2<double> Clamp(TVector2<double> const& val, TVector2<double> const& minval, TVector2<double> const& maxval)
{
    alignas(16) double result[2];

    _mm_storer_pd(result, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.X, val.Y), _mm_set_pd(minval.X, minval.Y)), _mm_set_pd(maxval.X, maxval.Y)));

    return TVector2<double>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<double> Clamp(TVector3<double> const& val, TVector3<double> const& minval, TVector3<double> const& maxval)
{
    alignas(16) double result[2];
    double             z;

    _mm_storer_pd(result, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.X, val.Y), _mm_set_pd(minval.X, minval.Y)), _mm_set_pd(maxval.X, maxval.Y)));
    _mm_store_sd(&z, _mm_min_sd(_mm_max_sd(_mm_set_sd(val.Z), _mm_set_sd(minval.Z)), _mm_set_sd(maxval.Z)));

    return TVector3<double>(result[0], result[1], z);
}

AN_FORCEINLINE TVector4<double> Clamp(TVector4<double> const& val, TVector4<double> const& minval, TVector4<double> const& maxval)
{
    alignas(16) double result1[2];
    alignas(16) double result2[2];

    _mm_storer_pd(result1, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.X, val.Y), _mm_set_pd(minval.X, minval.Y)), _mm_set_pd(maxval.X, maxval.Y)));
    _mm_storer_pd(result2, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.Z, val.W), _mm_set_pd(minval.Z, minval.W)), _mm_set_pd(maxval.Z, maxval.W)));

    return TVector4<double>(result1[0], result1[1], result2[0], result2[1]);
}

AN_FORCEINLINE TVector2<double> Saturate(TVector2<double> const& val)
{
    alignas(16) double result[2];

    _mm_storer_pd(result, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.X, val.Y), _mm_setzero_pd()), _mm_set_pd(1.0, 1.0)));

    return TVector2<double>(result[0], result[1]);
}

AN_FORCEINLINE TVector3<double> Saturate(TVector3<double> const& val)
{
    alignas(16) double result[2];
    double             z;

    _mm_storer_pd(result, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.X, val.Y), _mm_setzero_pd()), _mm_set_pd(1.0, 1.0)));
    _mm_store_sd(&z, _mm_min_sd(_mm_max_sd(_mm_set_sd(val.Z), _mm_setzero_pd()), _mm_set_sd(1.0)));

    return TVector3<double>(result[0], result[1], z);
}

AN_FORCEINLINE TVector4<double> Saturate(TVector4<double> const& val)
{
    alignas(16) double result1[2];
    alignas(16) double result2[2];

    _mm_storer_pd(result1, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.X, val.Y), _mm_setzero_pd()), _mm_set_pd(1.0, 1.0)));
    _mm_storer_pd(result2, _mm_min_pd(_mm_max_pd(_mm_set_pd(val.Z, val.W), _mm_setzero_pd()), _mm_set_pd(1.0, 1.0)));

    return TVector4<double>(result1[0], result1[1], result2[0], result2[1]);
}

} // namespace Math

using Float2  = TVector2<float>;
using Float3  = TVector3<float>;
using Float4  = TVector4<float>;
using Double2 = TVector2<double>;
using Double3 = TVector3<double>;
using Double4 = TVector4<double>;

struct Float2x2;
struct Float3x3;
struct Float4x4;
struct Float3x4;

// Column-major matrix 2x2
struct Float2x2
{
    using ElementType = Float2;

    Float2 Col0;
    Float2 Col1;

    Float2x2() = default;
    explicit Float2x2(Float3x3 const& v);
    explicit Float2x2(Float3x4 const& v);
    explicit Float2x2(Float4x4 const& v);
    constexpr Float2x2(Float2 const& col0, Float2 const& col1) :
        Col0(col0), Col1(col1) {}
    constexpr Float2x2(float _00, float _01, float _10, float _11) :
        Col0(_00, _01), Col1(_10, _11) {}
    constexpr explicit Float2x2(float diagonal) :
        Col0(diagonal, 0), Col1(0, diagonal) {}
    constexpr explicit Float2x2(Float2 const& diagonal) :
        Col0(diagonal.X, 0), Col1(0, diagonal.Y) {}

    float* ToPtr()
    {
        return &Col0.X;
    }

    const float* ToPtr() const
    {
        return &Col0.X;
    }

    Float2& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float2 const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float2 GetRow(int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return Float2(Col0[index], Col1[index]);
    }

    bool operator==(Float2x2 const& rhs) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 4; i++)
        {
            if (*pm1++ != *pm2++)
            {
                return false;
            }
        }
        return true;
    }

    bool operator!=(Float2x2 const& rhs) const { return !(operator==(rhs)); }

    bool CompareEps(Float2x2 const& rhs, float epsilon) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 4; i++)
        {
            if (Math::Abs(*pm1++ - *pm2++) >= epsilon)
            {
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
        return Float2x2(Col0.X, Col1.X, Col0.Y, Col1.Y);
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float2x2 Inversed() const
    {
        const float OneOverDeterminant = 1.0f / (Col0[0] * Col1[1] - Col1[0] * Col0[1]);
        return Float2x2(Col1[1] * OneOverDeterminant,
                        -Col0[1] * OneOverDeterminant,
                        -Col1[0] * OneOverDeterminant,
                        Col0[0] * OneOverDeterminant);
    }

    float Determinant() const
    {
        return Col0[0] * Col1[1] - Col1[0] * Col0[1];
    }

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }

    void SetIdentity()
    {
        Col0.Y = Col1.X = 0;
        Col0.X = Col1.Y = 1;
    }

    static Float2x2 Scale(Float2 const& scale)
    {
        return Float2x2(scale);
    }

    Float2x2 Scaled(Float2 const& scale) const
    {
        return Float2x2(Col0 * scale[0], Col1 * scale[1]);
    }

    // Return rotation around Z axis
    static Float2x2 Rotation(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float2x2(c, s,
                        -s, c);
    }

    Float2x2 operator*(float v) const
    {
        return Float2x2(Col0 * v,
                        Col1 * v);
    }

    Float2x2& operator*=(float v)
    {
        Col0 *= v;
        Col1 *= v;
        return *this;
    }

    Float2x2 operator/(float v) const
    {
        const float OneOverValue = 1.0f / v;
        return Float2x2(Col0 * OneOverValue,
                        Col1 * OneOverValue);
    }

    Float2x2& operator/=(float v)
    {
        const float OneOverValue = 1.0f / v;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        return *this;
    }

    template <typename T>
    TVector2<T> operator*(TVector2<T> const& vec) const
    {
        return TVector2<T>(Col0[0] * vec.X + Col1[0] * vec.Y,
                           Col0[1] * vec.X + Col1[1] * vec.Y);
    }

    Float2x2 operator*(Float2x2 const& matrix) const
    {
        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float R00 = matrix[0][0];
        const float R01 = matrix[0][1];
        const float R10 = matrix[1][0];
        const float R11 = matrix[1][1];
        return Float2x2(L00 * R00 + L10 * R01,
                        L01 * R00 + L11 * R01,
                        L00 * R10 + L10 * R11,
                        L01 * R10 + L11 * R11);
    }

    Float2x2& operator*=(Float2x2 const& matrix)
    {
        //*this = *this * matrix;

        const float L00 = Col0[0];
        const float L01 = Col0[1];
        const float L10 = Col1[0];
        const float L11 = Col1[1];
        const float R00 = matrix[0][0];
        const float R01 = matrix[0][1];
        const float R10 = matrix[1][0];
        const float R11 = matrix[1][1];
        Col0[0]         = L00 * R00 + L10 * R01;
        Col0[1]         = L01 * R00 + L11 * R01;
        Col1[0]         = L00 * R10 + L10 * R11;
        Col1[1]         = L01 * R10 + L11 * R11;
        return *this;
    }

    // String conversions
    AString ToString(int precision = Math::FloatingPointPrecision<float>()) const
    {
        return AString("( ") + Col0.ToString(precision) + " " + Col1.ToString(precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Col0.ToHexString(bLeadingZeros, bPrefix) + " " + Col1.ToHexString(bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        Col0.Write(stream);
        Col1.Write(stream);
    }

    void Read(IBinaryStream& stream)
    {
        Col0.Read(stream);
        Col1.Read(stream);
    }

    static constexpr int   NumComponents() { return 2; }

    static Float2x2 const& Identity()
    {
        static constexpr Float2x2 IdentityMat(1);
        return IdentityMat;
    }
};

// Column-major matrix 3x3
struct Float3x3
{
    using ElementType = Float3;

    Float3 Col0;
    Float3 Col1;
    Float3 Col2;

    Float3x3() = default;
    explicit Float3x3(Float2x2 const& v);
    explicit Float3x3(Float3x4 const& v);
    explicit Float3x3(Float4x4 const& v);
    constexpr Float3x3(Float3 const& col0, Float3 const& col1, Float3 const& col2) :
        Col0(col0), Col1(col1), Col2(col2) {}
    constexpr Float3x3(float _00, float _01, float _02, float _10, float _11, float _12, float _20, float _21, float _22) :
        Col0(_00, _01, _02), Col1(_10, _11, _12), Col2(_20, _21, _22) {}
    constexpr explicit Float3x3(float diagonal) :
        Col0(diagonal, 0, 0), Col1(0, diagonal, 0), Col2(0, 0, diagonal) {}
    constexpr explicit Float3x3(Float3 const& diagonal) :
        Col0(diagonal.X, 0, 0), Col1(0, diagonal.Y, 0), Col2(0, 0, diagonal.Z) {}

    float* ToPtr()
    {
        return &Col0.X;
    }

    const float* ToPtr() const
    {
        return &Col0.X;
    }

    Float3& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float3 const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float3 GetRow(int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return Float3(Col0[index], Col1[index], Col2[index]);
    }

    bool operator==(Float3x3 const& rhs) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 9; i++)
        {
            if (*pm1++ != *pm2++)
            {
                return false;
            }
        }
        return true;
    }
    bool operator!=(Float3x3 const& rhs) const { return !(operator==(rhs)); }

    bool CompareEps(Float3x3 const& rhs, float epsilon) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 9; i++)
        {
            if (Math::Abs(*pm1++ - *pm2++) >= epsilon)
            {
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
        return Float3x3(Col0.X, Col1.X, Col2.X, Col0.Y, Col1.Y, Col2.Y, Col0.Z, Col1.Z, Col2.Z);
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float3x3 Inversed() const
    {
        Float3x3 const& m                  = *this;
        const float     A                  = m[1][1] * m[2][2] - m[2][1] * m[1][2];
        const float     B                  = m[0][1] * m[2][2] - m[2][1] * m[0][2];
        const float     C                  = m[0][1] * m[1][2] - m[1][1] * m[0][2];
        const float     OneOverDeterminant = 1.0f / (m[0][0] * A - m[1][0] * B + m[2][0] * C);

        Float3x3 Inversed;
        Inversed[0][0] = A * OneOverDeterminant;
        Inversed[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * OneOverDeterminant;
        Inversed[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
        Inversed[0][1] = -B * OneOverDeterminant;
        Inversed[1][1] = (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * OneOverDeterminant;
        Inversed[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;
        Inversed[0][2] = C * OneOverDeterminant;
        Inversed[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * OneOverDeterminant;
        Inversed[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * OneOverDeterminant;
        return Inversed;
    }

    float Determinant() const
    {
        return Col0[0] * (Col1[1] * Col2[2] - Col2[1] * Col1[2]) -
            Col1[0] * (Col0[1] * Col2[2] - Col2[1] * Col0[2]) +
            Col2[0] * (Col0[1] * Col1[2] - Col1[1] * Col0[2]);
    }

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }

    void SetIdentity()
    {
        *this = Identity();
    }

    static Float3x3 Scale(Float3 const& scale)
    {
        return Float3x3(scale);
    }

    Float3x3 Scaled(Float3 const& scale) const
    {
        return Float3x3(Col0 * scale[0], Col1 * scale[1], Col2 * scale[2]);
    }

    // Return rotation around normalized axis
    static Float3x3 RotationAroundNormal(float angleInRadians, Float3 const& normal)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        const Float3 Temp  = (1.0f - c) * normal;
        const Float3 Temp2 = s * normal;
        return Float3x3(c + Temp[0] * normal[0], Temp[0] * normal[1] + Temp2[2], Temp[0] * normal[2] - Temp2[1],
                        Temp[1] * normal[0] - Temp2[2], c + Temp[1] * normal[1], Temp[1] * normal[2] + Temp2[0],
                        Temp[2] * normal[0] + Temp2[1], Temp[2] * normal[1] - Temp2[0], c + Temp[2] * normal[2]);
    }

    // Return rotation around normalized axis
    Float3x3 RotateAroundNormal(float angleInRadians, Float3 const& normal) const
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        Float3 Temp  = (1.0f - c) * normal;
        Float3 Temp2 = s * normal;
        return Float3x3(Col0 * (c + Temp[0] * normal[0]) + Col1 * (Temp[0] * normal[1] + Temp2[2]) + Col2 * (Temp[0] * normal[2] - Temp2[1]),
                        Col0 * (Temp[1] * normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * normal[1]) + Col2 * (Temp[1] * normal[2] + Temp2[0]),
                        Col0 * (Temp[2] * normal[0] + Temp2[1]) + Col1 * (Temp[2] * normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * normal[2]));
    }

    // Return rotation around unnormalized vector
    static Float3x3 RotationAroundVector(float angleInRadians, Float3 const& vector)
    {
        return RotationAroundNormal(angleInRadians, vector.Normalized());
    }

    // Return rotation around unnormalized vector
    Float3x3 RotateAroundVector(float angleInRadians, Float3 const& vector) const
    {
        return RotateAroundNormal(angleInRadians, vector.Normalized());
    }

    // Return rotation around X axis
    static Float3x3 RotationX(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float3x3(1, 0, 0,
                        0, c, s,
                        0, -s, c);
    }

    // Return rotation around Y axis
    static Float3x3 RotationY(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float3x3(c, 0, -s,
                        0, 1, 0,
                        s, 0, c);
    }

    // Return rotation around Z axis
    static Float3x3 RotationZ(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float3x3(c, s, 0,
                        -s, c, 0,
                        0, 0, 1);
    }

    Float3x3 operator*(float v) const
    {
        return Float3x3(Col0 * v,
                        Col1 * v,
                        Col2 * v);
    }

    Float3x3& operator*=(float v)
    {
        Col0 *= v;
        Col1 *= v;
        Col2 *= v;
        return *this;
    }

    Float3x3 operator/(float v) const
    {
        const float OneOverValue = 1.0f / v;
        return Float3x3(Col0 * OneOverValue,
                        Col1 * OneOverValue,
                        Col2 * OneOverValue);
    }

    Float3x3& operator/=(float v)
    {
        const float OneOverValue = 1.0f / v;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
        return *this;
    }

    template <typename T>
    TVector3<T> operator*(TVector3<T> const& vec) const
    {
        return TVector3<T>(Col0[0] * vec.X + Col1[0] * vec.Y + Col2[0] * vec.Z,
                           Col0[1] * vec.X + Col1[1] * vec.Y + Col2[1] * vec.Z,
                           Col0[2] * vec.X + Col1[2] * vec.Y + Col2[2] * vec.Z);
    }

    Float3x3 operator*(Float3x3 const& matrix) const
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
        const float R00 = matrix[0][0];
        const float R01 = matrix[0][1];
        const float R02 = matrix[0][2];
        const float R10 = matrix[1][0];
        const float R11 = matrix[1][1];
        const float R12 = matrix[1][2];
        const float R20 = matrix[2][0];
        const float R21 = matrix[2][1];
        const float R22 = matrix[2][2];
        return Float3x3(L00 * R00 + L10 * R01 + L20 * R02,
                        L01 * R00 + L11 * R01 + L21 * R02,
                        L02 * R00 + L12 * R01 + L22 * R02,
                        L00 * R10 + L10 * R11 + L20 * R12,
                        L01 * R10 + L11 * R11 + L21 * R12,
                        L02 * R10 + L12 * R11 + L22 * R12,
                        L00 * R20 + L10 * R21 + L20 * R22,
                        L01 * R20 + L11 * R21 + L21 * R22,
                        L02 * R20 + L12 * R21 + L22 * R22);
    }

    Float3x3& operator*=(Float3x3 const& matrix)
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
        const float R00 = matrix[0][0];
        const float R01 = matrix[0][1];
        const float R02 = matrix[0][2];
        const float R10 = matrix[1][0];
        const float R11 = matrix[1][1];
        const float R12 = matrix[1][2];
        const float R20 = matrix[2][0];
        const float R21 = matrix[2][1];
        const float R22 = matrix[2][2];
        Col0[0]         = L00 * R00 + L10 * R01 + L20 * R02;
        Col0[1]         = L01 * R00 + L11 * R01 + L21 * R02;
        Col0[2]         = L02 * R00 + L12 * R01 + L22 * R02;
        Col1[0]         = L00 * R10 + L10 * R11 + L20 * R12;
        Col1[1]         = L01 * R10 + L11 * R11 + L21 * R12;
        Col1[2]         = L02 * R10 + L12 * R11 + L22 * R12;
        Col2[0]         = L00 * R20 + L10 * R21 + L20 * R22;
        Col2[1]         = L01 * R20 + L11 * R21 + L21 * R22;
        Col2[2]         = L02 * R20 + L12 * R21 + L22 * R22;
        return *this;
    }

    AN_FORCEINLINE Float3x3 ViewInverseFast() const
    {
        return Transposed();
    }

    // String conversions
    AString ToString(int precision = Math::FloatingPointPrecision<float>()) const
    {
        return AString("( ") + Col0.ToString(precision) + " " + Col1.ToString(precision) + " " + Col2.ToString(precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Col0.ToHexString(bLeadingZeros, bPrefix) + " " + Col1.ToHexString(bLeadingZeros, bPrefix) + " " + Col2.ToHexString(bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        Col0.Write(stream);
        Col1.Write(stream);
        Col2.Write(stream);
    }

    void Read(IBinaryStream& stream)
    {
        Col0.Read(stream);
        Col1.Read(stream);
        Col2.Read(stream);
    }

    static constexpr int   NumComponents() { return 3; }

    static Float3x3 const& Identity()
    {
        static constexpr Float3x3 IdentityMat(1);
        return IdentityMat;
    }
};

// Column-major matrix 4x4
struct Float4x4
{
    using ElementType = Float4;

    Float4 Col0;
    Float4 Col1;
    Float4 Col2;
    Float4 Col3;

    Float4x4() = default;
    explicit Float4x4(Float2x2 const& v);
    explicit Float4x4(Float3x3 const& v);
    explicit Float4x4(Float3x4 const& v);
    constexpr Float4x4(Float4 const& col0, Float4 const& col1, Float4 const& col2, Float4 const& col3) :
        Col0(col0), Col1(col1), Col2(col2), Col3(col3) {}
    constexpr Float4x4(float _00, float _01, float _02, float _03, float _10, float _11, float _12, float _13, float _20, float _21, float _22, float _23, float _30, float _31, float _32, float _33) :
        Col0(_00, _01, _02, _03), Col1(_10, _11, _12, _13), Col2(_20, _21, _22, _23), Col3(_30, _31, _32, _33) {}
    constexpr explicit Float4x4(float diagonal) :
        Col0(diagonal, 0, 0, 0), Col1(0, diagonal, 0, 0), Col2(0, 0, diagonal, 0), Col3(0, 0, 0, diagonal) {}
    constexpr explicit Float4x4(Float4 const& diagonal) :
        Col0(diagonal.X, 0, 0, 0), Col1(0, diagonal.Y, 0, 0), Col2(0, 0, diagonal.Z, 0), Col3(0, 0, 0, diagonal.W) {}

    float* ToPtr()
    {
        return &Col0.X;
    }

    const float* ToPtr() const
    {
        return &Col0.X;
    }

    Float4& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float4 const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float4 GetRow(int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return Float4(Col0[index], Col1[index], Col2[index], Col3[index]);
    }

    bool operator==(Float4x4 const& rhs) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 16; i++)
        {
            if (*pm1++ != *pm2++)
            {
                return false;
            }
        }
        return true;
    }
    bool operator!=(Float4x4 const& rhs) const { return !(operator==(rhs)); }

    bool CompareEps(Float4x4 const& rhs, float epsilon) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 16; i++)
        {
            if (Math::Abs(*pm1++ - *pm2++) >= epsilon)
            {
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
        return Float4x4(Col0.X, Col1.X, Col2.X, Col3.X,
                        Col0.Y, Col1.Y, Col2.Y, Col3.Y,
                        Col0.Z, Col1.Z, Col2.Z, Col3.Z,
                        Col0.W, Col1.W, Col2.W, Col3.W);
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float4x4 Inversed() const
    {
        Float4x4 const& m = *this;

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
        Float4x4     Inversed(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

        const Float4 Row0(Inversed[0][0], Inversed[1][0], Inversed[2][0], Inversed[3][0]);

        const Float4 Dot0(m[0] * Row0);
        const float  Dot1 = (Dot0.X + Dot0.Y) + (Dot0.Z + Dot0.W);

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
            +(Col1[1] * SubFactor00 - Col1[2] * SubFactor01 + Col1[3] * SubFactor02),
            -(Col1[0] * SubFactor00 - Col1[2] * SubFactor03 + Col1[3] * SubFactor04),
            +(Col1[0] * SubFactor01 - Col1[1] * SubFactor03 + Col1[3] * SubFactor05),
            -(Col1[0] * SubFactor02 - Col1[1] * SubFactor04 + Col1[2] * SubFactor05));

        return Col0[0] * DetCof[0] + Col0[1] * DetCof[1] +
            Col0[2] * DetCof[2] + Col0[3] * DetCof[3];
    }

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }

    void SetIdentity()
    {
        *this = Identity();
    }

    static Float4x4 Translation(Float3 const& vec)
    {
        return Float4x4(Float4(1, 0, 0, 0),
                        Float4(0, 1, 0, 0),
                        Float4(0, 0, 1, 0),
                        Float4(vec[0], vec[1], vec[2], 1));
    }

    Float4x4 Translated(Float3 const& vec) const
    {
        return Float4x4(Col0, Col1, Col2, Col0 * vec[0] + Col1 * vec[1] + Col2 * vec[2] + Col3);
    }

    static Float4x4 Scale(Float3 const& scale)
    {
        return Float4x4(Float4(scale[0], 0, 0, 0),
                        Float4(0, scale[1], 0, 0),
                        Float4(0, 0, scale[2], 0),
                        Float4(0, 0, 0, 1));
    }

    Float4x4 Scaled(Float3 const& scale) const
    {
        return Float4x4(Col0 * scale[0], Col1 * scale[1], Col2 * scale[2], Col3);
    }


    // Return rotation around normalized axis
    static Float4x4 RotationAroundNormal(float angleInRadians, Float3 const& normal)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        const Float3 Temp  = (1.0f - c) * normal;
        const Float3 Temp2 = s * normal;
        return Float4x4(c + Temp[0] * normal[0], Temp[0] * normal[1] + Temp2[2], Temp[0] * normal[2] - Temp2[1], 0,
                        Temp[1] * normal[0] - Temp2[2], c + Temp[1] * normal[1], Temp[1] * normal[2] + Temp2[0], 0,
                        Temp[2] * normal[0] + Temp2[1], Temp[2] * normal[1] - Temp2[0], c + Temp[2] * normal[2], 0,
                        0, 0, 0, 1);
    }

    // Return rotation around normalized axis
    Float4x4 RotateAroundNormal(float angleInRadians, Float3 const& normal) const
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        Float3 Temp  = (1.0f - c) * normal;
        Float3 Temp2 = s * normal;
        return Float4x4(Col0 * (c + Temp[0] * normal[0]) + Col1 * (Temp[0] * normal[1] + Temp2[2]) + Col2 * (Temp[0] * normal[2] - Temp2[1]),
                        Col0 * (Temp[1] * normal[0] - Temp2[2]) + Col1 * (c + Temp[1] * normal[1]) + Col2 * (Temp[1] * normal[2] + Temp2[0]),
                        Col0 * (Temp[2] * normal[0] + Temp2[1]) + Col1 * (Temp[2] * normal[1] - Temp2[0]) + Col2 * (c + Temp[2] * normal[2]),
                        Col3);
    }

    // Return rotation around unnormalized vector
    static Float4x4 RotationAroundVector(float angleInRadians, Float3 const& vector)
    {
        return RotationAroundNormal(angleInRadians, vector.Normalized());
    }

    // Return rotation around unnormalized vector
    Float4x4 RotateAroundVector(float angleInRadians, Float3 const& vector) const
    {
        return RotateAroundNormal(angleInRadians, vector.Normalized());
    }

    // Return rotation around X axis
    static Float4x4 RotationX(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float4x4(1, 0, 0, 0,
                        0, c, s, 0,
                        0, -s, c, 0,
                        0, 0, 0, 1);
    }

    // Return rotation around Y axis
    static Float4x4 RotationY(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float4x4(c, 0, -s, 0,
                        0, 1, 0, 0,
                        s, 0, c, 0,
                        0, 0, 0, 1);
    }

    // Return rotation around Z axis
    static Float4x4 RotationZ(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float4x4(c, s, 0, 0,
                        -s, c, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1);
    }

    template <typename T>
    TVector4<T> operator*(TVector4<T> const& vec) const
    {
        return TVector4<T>(Col0[0] * vec.X + Col1[0] * vec.Y + Col2[0] * vec.Z + Col3[0] * vec.W,
                           Col0[1] * vec.X + Col1[1] * vec.Y + Col2[1] * vec.Z + Col3[1] * vec.W,
                           Col0[2] * vec.X + Col1[2] * vec.Y + Col2[2] * vec.Z + Col3[2] * vec.W,
                           Col0[3] * vec.X + Col1[3] * vec.Y + Col2[3] * vec.Z + Col3[3] * vec.W);
    }

    // Assume vec.W = 1
    template <typename T>
    TVector4<T> operator*(TVector3<T> const& vec) const
    {
        return TVector4<T>(Col0[0] * vec.X + Col1[0] * vec.Y + Col2[0] * vec.Z + Col3[0],
                           Col0[1] * vec.X + Col1[1] * vec.Y + Col2[1] * vec.Z + Col3[1],
                           Col0[2] * vec.X + Col1[2] * vec.Y + Col2[2] * vec.Z + Col3[2],
                           Col0[3] * vec.X + Col1[3] * vec.Y + Col2[3] * vec.Z + Col3[3]);
    }

    // Same as Float3x3(*this)*vec
    template <typename T>
    TVector3<T> TransformAsFloat3x3(TVector3<T> const& vec) const
    {
        return TVector3<T>(Col0[0] * vec.X + Col1[0] * vec.Y + Col2[0] * vec.Z,
                           Col0[1] * vec.X + Col1[1] * vec.Y + Col2[1] * vec.Z,
                           Col0[2] * vec.X + Col1[2] * vec.Y + Col2[2] * vec.Z);
    }

    // Same as Float3x3(*this)*matrix
    Float3x3 TransformAsFloat3x3(Float3x3 const& matrix) const
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
        const float R00 = matrix[0][0];
        const float R01 = matrix[0][1];
        const float R02 = matrix[0][2];
        const float R10 = matrix[1][0];
        const float R11 = matrix[1][1];
        const float R12 = matrix[1][2];
        const float R20 = matrix[2][0];
        const float R21 = matrix[2][1];
        const float R22 = matrix[2][2];
        return Float3x3(L00 * R00 + L10 * R01 + L20 * R02,
                        L01 * R00 + L11 * R01 + L21 * R02,
                        L02 * R00 + L12 * R01 + L22 * R02,
                        L00 * R10 + L10 * R11 + L20 * R12,
                        L01 * R10 + L11 * R11 + L21 * R12,
                        L02 * R10 + L12 * R11 + L22 * R12,
                        L00 * R20 + L10 * R21 + L20 * R22,
                        L01 * R20 + L11 * R21 + L21 * R22,
                        L02 * R20 + L12 * R21 + L22 * R22);
    }

    Float4x4 operator*(float v) const
    {
        return Float4x4(Col0 * v,
                        Col1 * v,
                        Col2 * v,
                        Col3 * v);
    }

    Float4x4& operator*=(float v)
    {
        Col0 *= v;
        Col1 *= v;
        Col2 *= v;
        Col3 *= v;
        return *this;
    }

    Float4x4 operator/(float v) const
    {
        const float OneOverValue = 1.0f / v;
        return Float4x4(Col0 * OneOverValue,
                        Col1 * OneOverValue,
                        Col2 * OneOverValue,
                        Col3 * OneOverValue);
    }

    Float4x4& operator/=(float v)
    {
        const float OneOverValue = 1.0f / v;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
        Col3 *= OneOverValue;
        return *this;
    }

    Float4x4 operator*(Float4x4 const& matrix) const
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
        const float R00 = matrix[0][0];
        const float R01 = matrix[0][1];
        const float R02 = matrix[0][2];
        const float R03 = matrix[0][3];
        const float R10 = matrix[1][0];
        const float R11 = matrix[1][1];
        const float R12 = matrix[1][2];
        const float R13 = matrix[1][3];
        const float R20 = matrix[2][0];
        const float R21 = matrix[2][1];
        const float R22 = matrix[2][2];
        const float R23 = matrix[2][3];
        const float R30 = matrix[3][0];
        const float R31 = matrix[3][1];
        const float R32 = matrix[3][2];
        const float R33 = matrix[3][3];
        return Float4x4(L00 * R00 + L10 * R01 + L20 * R02 + L30 * R03,
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
                        L03 * R30 + L13 * R31 + L23 * R32 + L33 * R33);
    }

    Float4x4& operator*=(Float4x4 const& matrix)
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
        const float R00 = matrix[0][0];
        const float R01 = matrix[0][1];
        const float R02 = matrix[0][2];
        const float R03 = matrix[0][3];
        const float R10 = matrix[1][0];
        const float R11 = matrix[1][1];
        const float R12 = matrix[1][2];
        const float R13 = matrix[1][3];
        const float R20 = matrix[2][0];
        const float R21 = matrix[2][1];
        const float R22 = matrix[2][2];
        const float R23 = matrix[2][3];
        const float R30 = matrix[3][0];
        const float R31 = matrix[3][1];
        const float R32 = matrix[3][2];
        const float R33 = matrix[3][3];
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
        return *this;
    }

    Float4x4 operator*(Float3x4 const& matrix) const;

    Float4x4& operator*=(Float3x4 const& matrix);

    Float4x4 ViewInverseFast() const
    {
        Float4x4 inversed;

        float*       DstPtr = inversed.ToPtr();
        const float* SrcPtr = ToPtr();

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

        DstPtr[12] = -(DstPtr[0] * SrcPtr[12] + DstPtr[4] * SrcPtr[13] + DstPtr[8] * SrcPtr[14]);
        DstPtr[13] = -(DstPtr[1] * SrcPtr[12] + DstPtr[5] * SrcPtr[13] + DstPtr[9] * SrcPtr[14]);
        DstPtr[14] = -(DstPtr[2] * SrcPtr[12] + DstPtr[6] * SrcPtr[13] + DstPtr[10] * SrcPtr[14]);
        DstPtr[15] = 1;

        return inversed;
    }

    AN_FORCEINLINE Float4x4 PerspectiveProjectionInverseFast() const
    {
        Float4x4 inversed;

        // TODO: check correctness for all perspective projections

        float*       DstPtr = inversed.ToPtr();
        const float* SrcPtr = ToPtr();

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
        DstPtr[15] = -SrcPtr[10] / (SrcPtr[11] * SrcPtr[14]);

        return inversed;
    }

    AN_FORCEINLINE Float4x4 OrthoProjectionInverseFast() const
    {
        // TODO: ...
        return Inversed();
    }

    // String conversions
    AString ToString(int precision = Math::FloatingPointPrecision<float>()) const
    {
        return AString("( ") + Col0.ToString(precision) + " " + Col1.ToString(precision) + " " + Col2.ToString(precision) + " " + Col3.ToString(precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Col0.ToHexString(bLeadingZeros, bPrefix) + " " + Col1.ToHexString(bLeadingZeros, bPrefix) + " " + Col2.ToHexString(bLeadingZeros, bPrefix) + " " + Col3.ToHexString(bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        Col0.Write(stream);
        Col1.Write(stream);
        Col2.Write(stream);
        Col3.Write(stream);
    }

    void Read(IBinaryStream& stream)
    {
        Col0.Read(stream);
        Col1.Read(stream);
        Col2.Read(stream);
        Col3.Read(stream);
    }

    static constexpr int   NumComponents() { return 4; }

    static Float4x4 const& Identity()
    {
        static constexpr Float4x4 IdentityMat(1);
        return IdentityMat;
    }

    static AN_FORCEINLINE Float4x4 LookAt(Float3 const& eye, Float3 const& center, Float3 const& up)
    {
        Float3 const f((center - eye).Normalized());
        Float3 const s(Math::Cross(up, f).Normalized());
        Float3 const u(Math::Cross(f, s));

        Float4x4 result;
        result[0][0] = s.X;
        result[1][0] = s.Y;
        result[2][0] = s.Z;
        result[3][0] = -Math::Dot(s, eye);

        result[0][1] = u.X;
        result[1][1] = u.Y;
        result[2][1] = u.Z;
        result[3][1] = -Math::Dot(u, eye);

        result[0][2] = f.X;
        result[1][2] = f.Y;
        result[2][2] = f.Z;
        result[3][2] = -Math::Dot(f, eye);

        result[0][3] = 0;
        result[1][3] = 0;
        result[2][3] = 0;
        result[3][3] = 1;

        return result;
    }

    // Conversion from standard projection matrix to clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 const& ClipControl_UpperLeft_ZeroToOne()
    {
        static constexpr float ClipTransform[] = {
            1, 0, 0, 0,
            0, -1, 0, 0,
            0, 0, 0.5f, 0,
            0, 0, 0.5f, 1};

        return *reinterpret_cast<Float4x4 const*>(&ClipTransform[0]);

        // Same
        //return Float4x4::Identity().Scaled(Float3(1.0f,1.0f,0.5f)).Translated(Float3(0,0,0.5)).Scaled(Float3(1,-1,1));
    }

    // Standard OpenGL ortho projection for 2D
    static AN_FORCEINLINE Float4x4 Ortho2D(Float2 const& mins, Float2 const& maxs)
    {
        const float InvX = 1.0f / (maxs.X - mins.X);
        const float InvY = 1.0f / (maxs.Y - mins.Y);
        const float tx   = -(maxs.X + mins.X) * InvX;
        const float ty   = -(maxs.Y + mins.Y) * InvY;
        return Float4x4(2 * InvX, 0, 0, 0,
                        0, 2 * InvY, 0, 0,
                        0, 0, -2, 0,
                        tx, ty, -1, 1);
    }

    // OpenGL ortho projection for 2D with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 Ortho2DCC(Float2 const& mins, Float2 const& maxs)
    {
        return ClipControl_UpperLeft_ZeroToOne() * Ortho2D(mins, maxs);
    }

    // Standard OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 Ortho(Float2 const& mins, Float2 const& maxs, float znear, float zfar)
    {
        const float InvX = 1.0f / (maxs.X - mins.X);
        const float InvY = 1.0f / (maxs.Y - mins.Y);
        const float InvZ = 1.0f / (zfar - znear);
        const float tx   = -(maxs.X + mins.X) * InvX;
        const float ty   = -(maxs.Y + mins.Y) * InvY;
        const float tz   = -(zfar + znear) * InvZ;
        return Float4x4(2 * InvX, 0, 0, 0,
                        0, 2 * InvY, 0, 0,
                        0, 0, -2 * InvZ, 0,
                        tx, ty, tz, 1);
    }

    // OpenGL ortho projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 OrthoCC(Float2 const& mins, Float2 const& maxs, float znear, float zfar)
    {
        const float InvX = 1.0f / (maxs.X - mins.X);
        const float InvY = 1.0f / (maxs.Y - mins.Y);
        const float InvZ = 1.0f / (zfar - znear);
        const float tx   = -(maxs.X + mins.X) * InvX;
        const float ty   = -(maxs.Y + mins.Y) * InvY;
        const float tz   = -(zfar + znear) * InvZ;
        return Float4x4(2 * InvX, 0, 0, 0,
                        0, -2 * InvY, 0, 0,
                        0, 0, -InvZ, 0,
                        tx, -ty, tz * 0.5 + 0.5, 1);
        // Same
        // Transform according to clip control
        //return ClipControl_UpperLeft_ZeroToOne() * Ortho( mins, maxs, znear, zfar );
    }

    // Reversed-depth OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 OrthoRev(Float2 const& mins, Float2 const& maxs, float znear, float zfar)
    {
        const float InvX = 1.0f / (maxs.X - mins.X);
        const float InvY = 1.0f / (maxs.Y - mins.Y);
        const float InvZ = 1.0f / (znear - zfar);
        const float tx   = -(maxs.X + mins.X) * InvX;
        const float ty   = -(maxs.Y + mins.Y) * InvY;
        const float tz   = -(znear + zfar) * InvZ;
        return Float4x4(2 * InvX, 0, 0, 0,
                        0, 2 * InvY, 0, 0,
                        0, 0, -2 * InvZ, 0,
                        tx, ty, tz, 1);
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL ortho projection
    static AN_FORCEINLINE Float4x4 OrthoRevCC(Float2 const& mins, Float2 const& maxs, float znear, float zfar)
    {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * OrthoRev(mins, maxs, znear, zfar);
    }

    // Standard OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 Perspective(float _FovXRad, float _Width, float _Height, float znear, float zfar)
    {
        const float TanHalfFovX = tan(_FovXRad * 0.5f);
        const float HalfFovY    = (float)(atan2(_Height, _Width / TanHalfFovX));
        const float TanHalfFovY = tan(HalfFovY);
        return Float4x4(1 / TanHalfFovX, 0, 0, 0,
                        0, 1 / TanHalfFovY, 0, 0,
                        0, 0, (zfar + znear) / (znear - zfar), -1,
                        0, 0, 2 * zfar * znear / (znear - zfar), 0);
    }

    static AN_FORCEINLINE Float4x4 Perspective(float _FovXRad, float _FovYRad, float znear, float zfar)
    {
        const float TanHalfFovX = tan(_FovXRad * 0.5f);
        const float TanHalfFovY = tan(_FovYRad * 0.5f);
        return Float4x4(1 / TanHalfFovX, 0, 0, 0,
                        0, 1 / TanHalfFovY, 0, 0,
                        0, 0, (zfar + znear) / (znear - zfar), -1,
                        0, 0, 2 * zfar * znear / (znear - zfar), 0);
    }

    // OpenGL perspective projection with clip control "upper-left & zero-to-one"
    static AN_FORCEINLINE Float4x4 PerspectiveCC(float _FovXRad, float _Width, float _Height, float znear, float zfar)
    {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective(_FovXRad, _Width, _Height, znear, zfar);
    }

    static AN_FORCEINLINE Float4x4 PerspectiveCC(float _FovXRad, float _FovYRad, float znear, float zfar)
    {
        // TODO: Optimize multiplication

        // Transform according to clip control
        return ClipControl_UpperLeft_ZeroToOne() * Perspective(_FovXRad, _FovYRad, znear, zfar);
    }

    // Reversed-depth OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRev(float _FovXRad, float _Width, float _Height, float znear, float zfar)
    {
        const float TanHalfFovX = tan(_FovXRad * 0.5f);
        const float HalfFovY    = (float)(atan2(_Height, _Width / TanHalfFovX));
        const float TanHalfFovY = tan(HalfFovY);
        return Float4x4(1 / TanHalfFovX, 0, 0, 0,
                        0, 1 / TanHalfFovY, 0, 0,
                        0, 0, (znear + zfar) / (zfar - znear), -1,
                        0, 0, 2 * znear * zfar / (zfar - znear), 0);
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRev(float _FovXRad, float _FovYRad, float znear, float zfar)
    {
        const float TanHalfFovX = tan(_FovXRad * 0.5f);
        const float TanHalfFovY = tan(_FovYRad * 0.5f);
        return Float4x4(1 / TanHalfFovX, 0, 0, 0,
                        0, 1 / TanHalfFovY, 0, 0,
                        0, 0, (znear + zfar) / (zfar - znear), -1,
                        0, 0, 2 * znear * zfar / (zfar - znear), 0);
    }

    // Reversed-depth with clip control "upper-left & zero-to-one" OpenGL perspective projection
    static AN_FORCEINLINE Float4x4 PerspectiveRevCC(float _FovXRad, float _Width, float _Height, float znear, float zfar)
    {
        const float TanHalfFovX = tan(_FovXRad * 0.5f);
        const float HalfFovY    = (float)(atan2(_Height, _Width / TanHalfFovX));
        const float TanHalfFovY = tan(HalfFovY);
        return Float4x4(1 / TanHalfFovX, 0, 0, 0,
                        0, -1 / TanHalfFovY, 0, 0,
                        0, 0, znear / (zfar - znear), -1,
                        0, 0, znear * zfar / (zfar - znear), 0);
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRevCC_Y(float _FovYRad, float _Width, float _Height, float znear, float zfar)
    {
        const float TanHalfFovY = tan(_FovYRad * 0.5f);
        const float HalfFovX    = atan2(TanHalfFovY * _Width, _Height);
        const float TanHalfFovX = tan(HalfFovX);
        return Float4x4(1 / TanHalfFovX, 0, 0, 0,
                        0, -1 / TanHalfFovY, 0, 0,
                        0, 0, znear / (zfar - znear), -1,
                        0, 0, znear * zfar / (zfar - znear), 0);
    }

    static AN_FORCEINLINE Float4x4 PerspectiveRevCC(float _FovXRad, float _FovYRad, float znear, float zfar)
    {
        const float TanHalfFovX = tan(_FovXRad * 0.5f);
        const float TanHalfFovY = tan(_FovYRad * 0.5f);
        return Float4x4(1 / TanHalfFovX, 0, 0, 0,
                        0, -1 / TanHalfFovY, 0, 0,
                        0, 0, znear / (zfar - znear), -1,
                        0, 0, znear * zfar / (zfar - znear), 0);
    }

    static AN_INLINE void GetCubeFaceMatrices(Float4x4& _PositiveX,
                                              Float4x4& _NegativeX,
                                              Float4x4& _PositiveY,
                                              Float4x4& _NegativeY,
                                              Float4x4& _PositiveZ,
                                              Float4x4& _NegativeZ)
    {
        _PositiveX = Float4x4::RotationZ(Math::_PI).RotateAroundNormal(Math::_HALF_PI, Float3(0, 1, 0));
        _NegativeX = Float4x4::RotationZ(Math::_PI).RotateAroundNormal(-Math::_HALF_PI, Float3(0, 1, 0));
        _PositiveY = Float4x4::RotationX(-Math::_HALF_PI);
        _NegativeY = Float4x4::RotationX(Math::_HALF_PI);
        _PositiveZ = Float4x4::RotationX(Math::_PI);
        _NegativeZ = Float4x4::RotationZ(Math::_PI);
    }

    static AN_INLINE Float4x4 const* GetCubeFaceMatrices()
    {
        // TODO: Precompute this matrices
        static Float4x4 const CubeFaceMatrices[6] = {
            Float4x4::RotationZ(Math::_PI).RotateAroundNormal(Math::_HALF_PI, Float3(0, 1, 0)),
            Float4x4::RotationZ(Math::_PI).RotateAroundNormal(-Math::_HALF_PI, Float3(0, 1, 0)),
            Float4x4::RotationX(-Math::_HALF_PI),
            Float4x4::RotationX(Math::_HALF_PI),
            Float4x4::RotationX(Math::_PI),
            Float4x4::RotationZ(Math::_PI)};
        return CubeFaceMatrices;
    }
};

// Column-major matrix 3x4
// Keep transformations transposed
struct Float3x4
{
    using ElementType = Float4;

    Float4 Col0;
    Float4 Col1;
    Float4 Col2;

    Float3x4() = default;
    explicit Float3x4(Float2x2 const& v);
    explicit Float3x4(Float3x3 const& v);
    explicit Float3x4(Float4x4 const& v);
    constexpr Float3x4(Float4 const& col0, Float4 const& col1, Float4 const& col2) :
        Col0(col0), Col1(col1), Col2(col2) {}
    constexpr Float3x4(float _00, float _01, float _02, float _03, float _10, float _11, float _12, float _13, float _20, float _21, float _22, float _23) :
        Col0(_00, _01, _02, _03), Col1(_10, _11, _12, _13), Col2(_20, _21, _22, _23) {}
    constexpr explicit Float3x4(float diagonal) :
        Col0(diagonal, 0, 0, 0), Col1(0, diagonal, 0, 0), Col2(0, 0, diagonal, 0) {}
    constexpr explicit Float3x4(Float3 const& diagonal) :
        Col0(diagonal.X, 0, 0, 0), Col1(0, diagonal.Y, 0, 0), Col2(0, 0, diagonal.Z, 0) {}

    float* ToPtr()
    {
        return &Col0.X;
    }

    const float* ToPtr() const
    {
        return &Col0.X;
    }

    Float4& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float4 const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Col0)[index];
    }

    Float3 GetRow(int index) const
    {
        AN_ASSERT_(index >= 0 && index < ElementType::NumComponents(), "Index out of range");
        return Float3(Col0[index], Col1[index], Col2[index]);
    }

    bool operator==(Float3x4 const& rhs) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 12; i++)
        {
            if (*pm1++ != *pm2++)
            {
                return false;
            }
        }
        return true;
    }
    bool operator!=(Float3x4 const& rhs) const { return !(operator==(rhs)); }

    bool CompareEps(Float3x4 const& rhs, float epsilon) const
    {
        const float* pm1 = ToPtr();
        const float* pm2 = rhs.ToPtr();
        for (int i = 0; i < 12; i++)
        {
            if (Math::Abs(*pm1++ - *pm2++) >= epsilon)
            {
                return false;
            }
        }
        return true;
    }

    // Math operators

    void Compose(Float3 const& _Translation, Float3x3 const& _Rotation, Float3 const& scale)
    {
        Col0[3] = _Translation.X;
        Col1[3] = _Translation.Y;
        Col2[3] = _Translation.Z;

        Col0[0] = _Rotation[0][0] * scale.X;
        Col0[1] = _Rotation[1][0] * scale.Y;
        Col0[2] = _Rotation[2][0] * scale.Z;

        Col1[0] = _Rotation[0][1] * scale.X;
        Col1[1] = _Rotation[1][1] * scale.Y;
        Col1[2] = _Rotation[2][1] * scale.Z;

        Col2[0] = _Rotation[0][2] * scale.X;
        Col2[1] = _Rotation[1][2] * scale.Y;
        Col2[2] = _Rotation[2][2] * scale.Z;
    }

    void Compose(Float3 const& _Translation, Float3x3 const& _Rotation)
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

    void SetTranslation(Float3 const& _Translation)
    {
        Col0[3] = _Translation.X;
        Col1[3] = _Translation.Y;
        Col2[3] = _Translation.Z;
    }

    void DecomposeAll(Float3& _Translation, Float3x3& _Rotation, Float3& scale) const
    {
        _Translation.X = Col0[3];
        _Translation.Y = Col1[3];
        _Translation.Z = Col2[3];

        scale.X = Float3(Col0[0], Col1[0], Col2[0]).Length();
        scale.Y = Float3(Col0[1], Col1[1], Col2[1]).Length();
        scale.Z = Float3(Col0[2], Col1[2], Col2[2]).Length();

        float sx = 1.0f / scale.X;
        float sy = 1.0f / scale.Y;
        float sz = 1.0f / scale.Z;

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
        return Float3(Col0[3], Col1[3], Col2[3]);
    }

    Float3x3 DecomposeRotation() const
    {
        return Float3x3(Float3(Col0[0], Col1[0], Col2[0]) / Float3(Col0[0], Col1[0], Col2[0]).Length(),
                        Float3(Col0[1], Col1[1], Col2[1]) / Float3(Col0[1], Col1[1], Col2[1]).Length(),
                        Float3(Col0[2], Col1[2], Col2[2]) / Float3(Col0[2], Col1[2], Col2[2]).Length());
    }

    Float3 DecomposeScale() const
    {
        return Float3(Float3(Col0[0], Col1[0], Col2[0]).Length(),
                      Float3(Col0[1], Col1[1], Col2[1]).Length(),
                      Float3(Col0[2], Col1[2], Col2[2]).Length());
    }

    void DecomposeRotationAndScale(Float3x3& _Rotation, Float3& scale) const
    {
        scale.X = Float3(Col0[0], Col1[0], Col2[0]).Length();
        scale.Y = Float3(Col0[1], Col1[1], Col2[1]).Length();
        scale.Z = Float3(Col0[2], Col1[2], Col2[2]).Length();

        float sx = 1.0f / scale.X;
        float sy = 1.0f / scale.Y;
        float sz = 1.0f / scale.Z;

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

    void DecomposeNormalMatrix(Float3x3& _NormalMatrix) const
    {
        Float3x4 const& m = *this;

        const float Determinant = m[0][0] * m[1][1] * m[2][2] +
            m[1][0] * m[2][1] * m[0][2] +
            m[2][0] * m[0][1] * m[1][2] -
            m[2][0] * m[1][1] * m[0][2] -
            m[1][0] * m[0][1] * m[2][2] -
            m[0][0] * m[2][1] * m[1][2];

        const float OneOverDeterminant = 1.0f / Determinant;

        _NormalMatrix[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * OneOverDeterminant;
        _NormalMatrix[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * OneOverDeterminant;
        _NormalMatrix[0][2] = (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * OneOverDeterminant;

        _NormalMatrix[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * OneOverDeterminant;
        _NormalMatrix[1][1] = (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * OneOverDeterminant;
        _NormalMatrix[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * OneOverDeterminant;

        _NormalMatrix[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
        _NormalMatrix[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;
        _NormalMatrix[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * OneOverDeterminant;

        // Or
        //_NormalMatrix = Float3x3(Inversed());
    }

    void InverseSelf()
    {
        *this = Inversed();
    }

    Float3x4 Inversed() const
    {
        Float3x4 const& m = *this;

        const float Determinant = m[0][0] * m[1][1] * m[2][2] +
            m[1][0] * m[2][1] * m[0][2] +
            m[2][0] * m[0][1] * m[1][2] -
            m[2][0] * m[1][1] * m[0][2] -
            m[1][0] * m[0][1] * m[2][2] -
            m[0][0] * m[2][1] * m[1][2];

        const float OneOverDeterminant = 1.0f / Determinant;
        Float3x4    Result;

        Result[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * OneOverDeterminant;
        Result[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * OneOverDeterminant;
        Result[0][2] = (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * OneOverDeterminant;
        Result[0][3] = -(m[0][3] * Result[0][0] + m[1][3] * Result[0][1] + m[2][3] * Result[0][2]);

        Result[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * OneOverDeterminant;
        Result[1][1] = (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * OneOverDeterminant;
        Result[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * OneOverDeterminant;
        Result[1][3] = -(m[0][3] * Result[1][0] + m[1][3] * Result[1][1] + m[2][3] * Result[1][2]);

        Result[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
        Result[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;
        Result[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * OneOverDeterminant;
        Result[2][3] = -(m[0][3] * Result[2][0] + m[1][3] * Result[2][1] + m[2][3] * Result[2][2]);

        return Result;
    }

    float Determinant() const
    {
        return Col0[0] * (Col1[1] * Col2[2] - Col2[1] * Col1[2]) +
            Col1[0] * (Col2[1] * Col0[2] - Col0[1] * Col2[2]) +
            Col2[0] * (Col0[1] * Col1[2] - Col1[1] * Col0[2]);
    }

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }

    void SetIdentity()
    {
        *this = Identity();
    }

    static Float3x4 Translation(Float3 const& vec)
    {
        return Float3x4(Float4(1, 0, 0, vec[0]),
                        Float4(0, 1, 0, vec[1]),
                        Float4(0, 0, 1, vec[2]));
    }

    static Float3x4 Scale(Float3 const& scale)
    {
        return Float3x4(Float4(scale[0], 0, 0, 0),
                        Float4(0, scale[1], 0, 0),
                        Float4(0, 0, scale[2], 0));
    }

    // Return rotation around normalized axis
    static Float3x4 RotationAroundNormal(float angleInRadians, Float3 const& normal)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        const Float3 Temp  = (1.0f - c) * normal;
        const Float3 Temp2 = s * normal;
        return Float3x4(c + Temp[0] * normal[0], Temp[1] * normal[0] - Temp2[2], Temp[2] * normal[0] + Temp2[1], 0,
                        Temp[0] * normal[1] + Temp2[2], c + Temp[1] * normal[1], Temp[2] * normal[1] - Temp2[0], 0,
                        Temp[0] * normal[2] - Temp2[1], Temp[1] * normal[2] + Temp2[0], c + Temp[2] * normal[2], 0);
    }

    // Return rotation around unnormalized vector
    static Float3x4 RotationAroundVector(float angleInRadians, Float3 const& vector)
    {
        return RotationAroundNormal(angleInRadians, vector.Normalized());
    }

    // Return rotation around X axis
    static Float3x4 RotationX(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float3x4(1, 0, 0, 0,
                        0, c, -s, 0,
                        0, s, c, 0);
    }

    // Return rotation around Y axis
    static Float3x4 RotationY(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float3x4(c, 0, s, 0,
                        0, 1, 0, 0,
                        -s, 0, c, 0);
    }

    // Return rotation around Z axis
    static Float3x4 RotationZ(float angleInRadians)
    {
        float s, c;
        Math::SinCos(angleInRadians, s, c);
        return Float3x4(c, -s, 0, 0,
                        s, c, 0, 0,
                        0, 0, 1, 0);
    }

    // Assume vec.W = 1
    template <typename T>
    TVector3<T> operator*(TVector3<T> const& vec) const
    {
        return TVector3<T>(Col0[0] * vec.X + Col0[1] * vec.Y + Col0[2] * vec.Z + Col0[3],
                           Col1[0] * vec.X + Col1[1] * vec.Y + Col1[2] * vec.Z + Col1[3],
                           Col2[0] * vec.X + Col2[1] * vec.Y + Col2[2] * vec.Z + Col2[3]);
    }

    // Assume vec.Z = 0, vec.W = 1
    template <typename T>
    TVector3<T> operator*(TVector2<T> const& vec) const
    {
        return TVector3<T>(Col0[0] * vec.X + Col0[1] * vec.Y + Col0[3],
                           Col1[0] * vec.X + Col1[1] * vec.Y + Col1[3],
                           Col2[0] * vec.X + Col2[1] * vec.Y + Col2[3]);
    }

    template <typename T>
    TVector2<T> Mult_Float2_IgnoreZ(TVector2<T> const& vec) const
    {
        return TVector2<T>(Col0[0] * vec.X + Col0[1] * vec.Y + Col0[3],
                           Col1[0] * vec.X + Col1[1] * vec.Y + Col1[3]);
    }

    Float3x4 operator*(float v) const
    {
        return Float3x4(Col0 * v,
                        Col1 * v,
                        Col2 * v);
    }

    Float3x4& operator*=(float v)
    {
        Col0 *= v;
        Col1 *= v;
        Col2 *= v;
        return *this;
    }

    Float3x4 operator/(float v) const
    {
        const float OneOverValue = 1.0f / v;
        return Float3x4(Col0 * OneOverValue,
                        Col1 * OneOverValue,
                        Col2 * OneOverValue);
    }

    Float3x4& operator/=(float v)
    {
        const float OneOverValue = 1.0f / v;
        Col0 *= OneOverValue;
        Col1 *= OneOverValue;
        Col2 *= OneOverValue;
        return *this;
    }

    Float3x4 operator*(Float3x4 const& matrix) const
    {
        return Float3x4(Col0[0] * matrix[0][0] + Col0[1] * matrix[1][0] + Col0[2] * matrix[2][0],
                        Col0[0] * matrix[0][1] + Col0[1] * matrix[1][1] + Col0[2] * matrix[2][1],
                        Col0[0] * matrix[0][2] + Col0[1] * matrix[1][2] + Col0[2] * matrix[2][2],
                        Col0[0] * matrix[0][3] + Col0[1] * matrix[1][3] + Col0[2] * matrix[2][3] + Col0[3],

                        Col1[0] * matrix[0][0] + Col1[1] * matrix[1][0] + Col1[2] * matrix[2][0],
                        Col1[0] * matrix[0][1] + Col1[1] * matrix[1][1] + Col1[2] * matrix[2][1],
                        Col1[0] * matrix[0][2] + Col1[1] * matrix[1][2] + Col1[2] * matrix[2][2],
                        Col1[0] * matrix[0][3] + Col1[1] * matrix[1][3] + Col1[2] * matrix[2][3] + Col1[3],

                        Col2[0] * matrix[0][0] + Col2[1] * matrix[1][0] + Col2[2] * matrix[2][0],
                        Col2[0] * matrix[0][1] + Col2[1] * matrix[1][1] + Col2[2] * matrix[2][1],
                        Col2[0] * matrix[0][2] + Col2[1] * matrix[1][2] + Col2[2] * matrix[2][2],
                        Col2[0] * matrix[0][3] + Col2[1] * matrix[1][3] + Col2[2] * matrix[2][3] + Col2[3]);
    }

    Float3x4& operator*=(Float3x4 const& matrix)
    {
        *this = Float3x4(Col0[0] * matrix[0][0] + Col0[1] * matrix[1][0] + Col0[2] * matrix[2][0],
                         Col0[0] * matrix[0][1] + Col0[1] * matrix[1][1] + Col0[2] * matrix[2][1],
                         Col0[0] * matrix[0][2] + Col0[1] * matrix[1][2] + Col0[2] * matrix[2][2],
                         Col0[0] * matrix[0][3] + Col0[1] * matrix[1][3] + Col0[2] * matrix[2][3] + Col0[3],

                         Col1[0] * matrix[0][0] + Col1[1] * matrix[1][0] + Col1[2] * matrix[2][0],
                         Col1[0] * matrix[0][1] + Col1[1] * matrix[1][1] + Col1[2] * matrix[2][1],
                         Col1[0] * matrix[0][2] + Col1[1] * matrix[1][2] + Col1[2] * matrix[2][2],
                         Col1[0] * matrix[0][3] + Col1[1] * matrix[1][3] + Col1[2] * matrix[2][3] + Col1[3],

                         Col2[0] * matrix[0][0] + Col2[1] * matrix[1][0] + Col2[2] * matrix[2][0],
                         Col2[0] * matrix[0][1] + Col2[1] * matrix[1][1] + Col2[2] * matrix[2][1],
                         Col2[0] * matrix[0][2] + Col2[1] * matrix[1][2] + Col2[2] * matrix[2][2],
                         Col2[0] * matrix[0][3] + Col2[1] * matrix[1][3] + Col2[2] * matrix[2][3] + Col2[3]);
        return *this;
    }

    // String conversions
    AString ToString(int precision = Math::FloatingPointPrecision<float>()) const
    {
        return AString("( ") + Col0.ToString(precision) + " " + Col1.ToString(precision) + " " + Col2.ToString(precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Col0.ToHexString(bLeadingZeros, bPrefix) + " " + Col1.ToHexString(bLeadingZeros, bPrefix) + " " + Col2.ToHexString(bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        Col0.Write(stream);
        Col1.Write(stream);
        Col2.Write(stream);
    }

    void Read(IBinaryStream& stream)
    {
        Col0.Read(stream);
        Col1.Read(stream);
        Col2.Read(stream);
    }

    static constexpr int   NumComponents() { return 3; }

    static Float3x4 const& Identity()
    {
        static constexpr Float3x4 IdentityMat(1);
        return IdentityMat;
    }
};

// Type conversion

// Float3x3 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2(Float3x3 const& v) :
    Col0(v.Col0), Col1(v.Col1) {}

// Float3x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2(Float3x4 const& v) :
    Col0(v.Col0), Col1(v.Col1) {}

// Float4x4 to Float2x2
AN_FORCEINLINE Float2x2::Float2x2(Float4x4 const& v) :
    Col0(v.Col0), Col1(v.Col1) {}

// Float2x2 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3(Float2x2 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(0, 0, 1) {}

// Float3x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3(Float3x4 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(v.Col2) {}

// Float4x4 to Float3x3
AN_FORCEINLINE Float3x3::Float3x3(Float4x4 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(v.Col2) {}

// Float2x2 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4(Float2x2 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(0, 0, 1, 0), Col3(0, 0, 0, 1) {}

// Float3x3 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4(Float3x3 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(v.Col2), Col3(0, 0, 0, 1) {}

// Float3x4 to Float4x4
AN_FORCEINLINE Float4x4::Float4x4(Float3x4 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(v.Col2), Col3(0, 0, 0, 1) {}

// Float2x2 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4(Float2x2 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(0.0f) {}

// Float3x3 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4(Float3x3 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(v.Col2) {}

// Float4x4 to Float3x4
AN_FORCEINLINE Float3x4::Float3x4(Float4x4 const& v) :
    Col0(v.Col0), Col1(v.Col1), Col2(v.Col2) {}


template <typename T>
AN_FORCEINLINE TVector2<T> operator*(TVector2<T> const& vec, Float2x2 const& matrix)
{
    return TVector2<T>(matrix[0][0] * vec.X + matrix[0][1] * vec.Y,
                       matrix[1][0] * vec.X + matrix[1][1] * vec.Y);
}

template <typename T>
AN_FORCEINLINE TVector3<T> operator*(TVector3<T> const& vec, Float3x3 const& matrix)
{
    return TVector3<T>(matrix[0][0] * vec.X + matrix[0][1] * vec.Y + matrix[0][2] * vec.Z,
                       matrix[1][0] * vec.X + matrix[1][1] * vec.Y + matrix[1][2] * vec.Z,
                       matrix[2][0] * vec.X + matrix[2][1] * vec.Y + matrix[2][2] * vec.Z);
}

template <typename T>
AN_FORCEINLINE TVector4<T> operator*(TVector4<T> const& vec, Float4x4 const& matrix)
{
    return TVector4<T>(matrix[0][0] * vec.X + matrix[0][1] * vec.Y + matrix[0][2] * vec.Z + matrix[0][3] * vec.W,
                       matrix[1][0] * vec.X + matrix[1][1] * vec.Y + matrix[1][2] * vec.Z + matrix[1][3] * vec.W,
                       matrix[2][0] * vec.X + matrix[2][1] * vec.Y + matrix[2][2] * vec.Z + matrix[2][3] * vec.W,
                       matrix[3][0] * vec.X + matrix[3][1] * vec.Y + matrix[3][2] * vec.Z + matrix[3][3] * vec.W);
}

AN_FORCEINLINE Float4x4 Float4x4::operator*(Float3x4 const& matrix) const
{
    Float4 const& SrcB0 = matrix.Col0;
    Float4 const& SrcB1 = matrix.Col1;
    Float4 const& SrcB2 = matrix.Col2;

    return Float4x4(Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                    Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                    Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                    Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3);
}

AN_FORCEINLINE Float4x4& Float4x4::operator*=(Float3x4 const& matrix)
{
    Float4 const& SrcB0 = matrix.Col0;
    Float4 const& SrcB1 = matrix.Col1;
    Float4 const& SrcB2 = matrix.Col2;

    *this = Float4x4(Col0 * SrcB0[0] + Col1 * SrcB1[0] + Col2 * SrcB2[0],
                     Col0 * SrcB0[1] + Col1 * SrcB1[1] + Col2 * SrcB2[1],
                     Col0 * SrcB0[2] + Col1 * SrcB1[2] + Col2 * SrcB2[2],
                     Col0 * SrcB0[3] + Col1 * SrcB1[3] + Col2 * SrcB2[3] + Col3);
    return *this;
}

namespace Math
{

AN_INLINE bool Unproject(Float4x4 const& _ModelViewProjectionInversed, const float _Viewport[4], Float3 const& _Coord, Float3& _Result)
{
    Float4 In(_Coord, 1.0f);

    // Map x and y from window coordinates
    In.X = (In.X - _Viewport[0]) / _Viewport[2];
    In.Y = (In.Y - _Viewport[1]) / _Viewport[3];

    // Map to range -1 to 1
    In.X = In.X * 2.0f - 1.0f;
    In.Y = In.Y * 2.0f - 1.0f;
    In.Z = In.Z * 2.0f - 1.0f;

    _Result.X       = _ModelViewProjectionInversed[0][0] * In[0] + _ModelViewProjectionInversed[1][0] * In[1] + _ModelViewProjectionInversed[2][0] * In[2] + _ModelViewProjectionInversed[3][0] * In[3];
    _Result.Y       = _ModelViewProjectionInversed[0][1] * In[0] + _ModelViewProjectionInversed[1][1] * In[1] + _ModelViewProjectionInversed[2][1] * In[2] + _ModelViewProjectionInversed[3][1] * In[3];
    _Result.Z       = _ModelViewProjectionInversed[0][2] * In[0] + _ModelViewProjectionInversed[1][2] * In[1] + _ModelViewProjectionInversed[2][2] * In[2] + _ModelViewProjectionInversed[3][2] * In[3];
    const float Div = _ModelViewProjectionInversed[0][3] * In[0] + _ModelViewProjectionInversed[1][3] * In[1] + _ModelViewProjectionInversed[2][3] * In[2] + _ModelViewProjectionInversed[3][3] * In[3];

    if (Div == 0.0f)
    {
        return false;
    }

    _Result /= Div;

    return true;
}

AN_INLINE bool UnprojectRay(Float4x4 const& _ModelViewProjectionInversed, const float _Viewport[4], float x, float y, Float3& _RayStart, Float3& _RayEnd)
{
    Float3 Coord;

    Coord.X = x;
    Coord.Y = y;

    // get start point
    Coord.Z = -1.0f;
    if (!Unproject(_ModelViewProjectionInversed, _Viewport, Coord, _RayStart))
        return false;

    // get end point
    Coord.Z = 1.0f;
    if (!Unproject(_ModelViewProjectionInversed, _Viewport, Coord, _RayEnd))
        return false;

    return true;
}

AN_INLINE bool UnprojectRayDir(Float4x4 const& _ModelViewProjectionInversed, const float _Viewport[4], float x, float y, Float3& _RayStart, Float3& _RayDir)
{
    Float3 Coord;

    Coord.X = x;
    Coord.Y = y;

    // get start point
    Coord.Z = -1.0f;
    if (!Unproject(_ModelViewProjectionInversed, _Viewport, Coord, _RayStart))
        return false;

    // get end point
    Coord.Z = 1.0f;
    if (!Unproject(_ModelViewProjectionInversed, _Viewport, Coord, _RayDir))
        return false;

    _RayDir -= _RayStart;
    _RayDir.NormalizeSelf();

    return true;
}

AN_INLINE bool UnprojectPoint(Float4x4 const& _ModelViewProjectionInversed,
                              const float     _Viewport[4],
                              float           x,
                              float           y,
                              float           _Depth,
                              Float3&         _Result)
{
    return Unproject(_ModelViewProjectionInversed, _Viewport, Float3(x, y, _Depth), _Result);
}

} // namespace Math
