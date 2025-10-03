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
#include "Quat.h"

HK_NAMESPACE_BEGIN

/*

Plane equalation: Normal.X * X + Normal.Y * Y + Normal.Z * Z + D = 0

*/

template <typename T>
struct TPlane
{
    TVector3<T> Normal{0,0,1};
    T           D{0};

    /// Construct
    TPlane() = default;

    /// Construct from plane equalation: A * X + B * Y + C * Z + D = 0
    constexpr explicit TPlane(T a, T b, T c, T d) :
        Normal(a, b, c), D(d) {}

    /// Construct from normal and distance to plane
    constexpr explicit TPlane(TVector3<T> const& normal, T dist) :
        Normal(normal), D(-dist) {}

    /// Construct from normal and point on plane
    constexpr explicit TPlane(TVector3<T> const& normal, TVector3<T> const& point) :
        Normal(normal), D(-Math::Dot(point, normal)) {}

    /// Construct from three points on plane
    constexpr explicit TPlane(TVector3<T> const& p0, TVector3<T> const& p1, TVector3<T> const& p2) :
        Normal(Math::Cross(p0 - p1, p2 - p1).Normalized()),
        D(-Math::Dot(Normal, p1))
    {}

    /// Construct from another plane
    template <typename T2>
    constexpr explicit TPlane(TPlane<T2> const& plane) :
        Normal(plane.Normal), D(plane.D) {}

    T* ToPtr()
    {
        return &Normal.X;
    }

    T const* ToPtr() const
    {
        return &Normal.X;
    }

    constexpr TPlane operator-() const
    {
        // construct from normal and distance. D = -distance
        return TPlane(-Normal, D);
    }

    constexpr bool operator==(TPlane const& rhs) const { return ToFloat4() == rhs.ToFloat4(); }
    constexpr bool operator!=(TPlane const& rhs) const { return !(operator==(rhs)); }

    constexpr bool CompareEps(TPlane const& rhs, T normalEps, T distEps) const
    {
        return Bool4(Math::Dist(Normal.X, rhs.Normal.X) < normalEps,
                     Math::Dist(Normal.Y, rhs.Normal.Y) < normalEps,
                     Math::Dist(Normal.Z, rhs.Normal.Z) < normalEps,
                     Math::Dist(D, rhs.D) < distEps)
            .All();
    }

    void Clear()
    {
        Normal.X = Normal.Y = Normal.Z = D = T(0);
    }

    void SetDist(T dist)
    {
        D = -dist;
    }

    constexpr T GetDist() const
    {
        return -D;
    }

    TVector3<T> GetOrigin() const
    {
        return Normal * (-D);
    }

    Quat GetRotation() const
    {
        return (Normal.Z == T(-1)) ? Quat(0, 1, 0, 0) : Quat(Normal.Z + T(1), -Normal.Y, Normal.X, T(0)).Normalized();
    }

    int AxialType() const
    {
        return Normal.NormalAxialType();
    }

    int PositiveAxialType() const
    {
        return Normal.NormalPositiveAxialType();
    }

    constexpr int SignBits() const
    {
        return Normal.SignBits();
    }

    void FromPoints(TVector3<T> const& p0, TVector3<T> const& p1, TVector3<T> const& p2)
    {
        Normal = Math::Cross(p0 - p1, p2 - p1).Normalized();
        D      = -Math::Dot(Normal, p1);
    }

    void FromPoints(TVector3<T> const points[3])
    {
        FromPoints(points[0], points[1], points[2]);
    }

    T DistanceToPoint(TVector3<T> const& p) const
    {
        return Math::Dot(p, Normal) + D;
    }

    void NormalizeSelf()
    {
        const T len = Normal.Length();
        if (len != T(0))
        {
            const T invLen = T(1) / len;
            Normal *= invLen;
            D *= invLen;
        }
    }

    TPlane Normalized() const
    {
        const T len = Normal.Length();
        if (len != T(0))
        {
            const T invLen = T(1) / len;
            return TPlane(Normal * invLen, D * invLen);
        }
        return *this;
    }

    TPlane Snap(T normalEps, T distEps) const
    {
        TPlane  snapPlane(Normal.SnapNormal(normalEps), D);
        const T snapD = Math::Round(D);
        if (Math::Abs(D - snapD) < distEps)
        {
            snapPlane.D = snapD;
        }
        return snapPlane;
    }

    TVector4<T>&                 ToFloat4() { return *reinterpret_cast<TVector4<T>*>(this); }
    constexpr TVector4<T> const& ToFloat4() const { return *reinterpret_cast<const TVector4<T>*>(this); }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const
    {
        struct Writer
        {
            Writer(IBinaryStreamWriteInterface& stream, float value)
            {
                stream.WriteFloat(value);
            }

            Writer(IBinaryStreamWriteInterface& stream, double value)
            {
                stream.WriteDouble(value);
            }
        };
        Normal.Write(stream);
        Writer(stream, D);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        struct Reader
        {
            Reader(IBinaryStreamReadInterface& stream, float& value)
            {
                value = stream.ReadFloat();
            }

            Reader(IBinaryStreamReadInterface& stream, double& value)
            {
                value = stream.ReadDouble();
            }
        };
        Normal.Read(stream);
        Reader(stream, D);
    }
};

using PlaneF = TPlane<float>;
using PlaneD = TPlane<double>;

HK_NAMESPACE_END

HK_FORMAT_DEF_(Hk::PlaneF, "( {} {} {} {} )", v.Normal.X, v.Normal.Y, v.Normal.Z, v.D);
HK_FORMAT_DEF_(Hk::PlaneD, "( {} {} {} {} )", v.Normal.X, v.Normal.Y, v.Normal.Z, v.D);
