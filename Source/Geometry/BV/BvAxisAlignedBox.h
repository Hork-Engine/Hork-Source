/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Geometry/VectorMath.h>

struct BvAxisAlignedBox
{
    Float3 Mins;
    Float3 Maxs;

    BvAxisAlignedBox() = default;
    constexpr BvAxisAlignedBox(Float3 const& mins, Float3 const& maxs) :
        Mins(mins), Maxs(maxs) {}
    constexpr BvAxisAlignedBox(Float3 const& Pos, float Radius) :
        Mins(Pos - Radius), Maxs(Pos + Radius)
    {}

    HK_FORCEINLINE float* ToPtr()
    {
        return &Mins.X;
    }

    HK_FORCEINLINE const float* ToPtr() const
    {
        return &Mins.X;
    }

    constexpr HK_FORCEINLINE BvAxisAlignedBox operator+(Float3 const& vec) const
    {
        return BvAxisAlignedBox(Mins + vec, Maxs + vec);
    }

    constexpr HK_FORCEINLINE BvAxisAlignedBox operator-(Float3 const& vec) const
    {
        return BvAxisAlignedBox(Mins - vec, Maxs - vec);
    }

    constexpr HK_FORCEINLINE BvAxisAlignedBox operator*(float scale) const
    {
        return BvAxisAlignedBox(Mins * scale, Maxs * scale);
    }

    constexpr HK_FORCEINLINE BvAxisAlignedBox operator/(float scale) const
    {
        return (*this) * (1.0f / scale);
    }

    HK_FORCEINLINE BvAxisAlignedBox& operator+=(Float3 const& vec)
    {
        Mins += vec;
        Maxs += vec;
        return *this;
    }

    HK_FORCEINLINE BvAxisAlignedBox& operator-=(Float3 const& vec)
    {
        Mins -= vec;
        Maxs -= vec;
        return *this;
    }

    HK_FORCEINLINE BvAxisAlignedBox& operator*=(float scale)
    {
        Mins *= scale;
        Maxs *= scale;
        return *this;
    }

    HK_FORCEINLINE BvAxisAlignedBox& operator/=(float scale)
    {
        const float InvScale = 1.0f / scale;
        Mins *= InvScale;
        Maxs *= InvScale;
        return *this;
    }

    HK_FORCEINLINE bool CompareEps(BvAxisAlignedBox const& rhs, float epsilon) const
    {
        return Mins.CompareEps(rhs.Mins, epsilon) && Maxs.CompareEps(rhs.Maxs, epsilon);
    }

    HK_FORCEINLINE bool operator==(BvAxisAlignedBox const& rhs) const
    {
        return Mins == rhs.Mins && Maxs == rhs.Maxs;
    }

    HK_FORCEINLINE bool operator!=(BvAxisAlignedBox const& rhs) const
    {
        return !(operator==(rhs));
    }

    HK_FORCEINLINE void Clear()
    {
        Mins = Float3(9999999999.0f);
        Maxs = Float3(-9999999999.0f);
    }

    HK_FORCEINLINE void AddPoint(Float3 const& p)
    {
        Mins.X = Math::Min(p.X, Mins.X);
        Maxs.X = Math::Max(p.X, Maxs.X);
        Mins.Y = Math::Min(p.Y, Mins.Y);
        Maxs.Y = Math::Max(p.Y, Maxs.Y);
        Mins.Z = Math::Min(p.Z, Mins.Z);
        Maxs.Z = Math::Max(p.Z, Maxs.Z);
    }

    HK_FORCEINLINE void AddPoint(float x, float y, float z)
    {
        Mins.X = Math::Min(x, Mins.X);
        Maxs.X = Math::Max(x, Maxs.X);
        Mins.Y = Math::Min(y, Mins.Y);
        Maxs.Y = Math::Max(y, Maxs.Y);
        Mins.Z = Math::Min(z, Mins.Z);
        Maxs.Z = Math::Max(z, Maxs.Z);
    }

    HK_FORCEINLINE void AddAABB(BvAxisAlignedBox const& box)
    {
        Mins.X = Math::Min(box.Mins.X, Mins.X);
        Maxs.X = Math::Max(box.Maxs.X, Maxs.X);
        Mins.Y = Math::Min(box.Mins.Y, Mins.Y);
        Maxs.Y = Math::Max(box.Maxs.Y, Maxs.Y);
        Mins.Z = Math::Min(box.Mins.Z, Mins.Z);
        Maxs.Z = Math::Max(box.Maxs.Z, Maxs.Z);
    }

    HK_FORCEINLINE void AddAABB(Float3 const& mins, Float3 const& maxs)
    {
        Mins.X = Math::Min(mins.X, Mins.X);
        Maxs.X = Math::Max(maxs.X, Maxs.X);
        Mins.Y = Math::Min(mins.Y, Mins.Y);
        Maxs.Y = Math::Max(maxs.Y, Maxs.Y);
        Mins.Z = Math::Min(mins.Z, Mins.Z);
        Maxs.Z = Math::Max(maxs.Z, Maxs.Z);
    }

    HK_FORCEINLINE void AddSphere(Float3 const& position, float radius)
    {
        Mins.X = Math::Min(position.X - radius, Mins.X);
        Maxs.X = Math::Max(position.X + radius, Maxs.X);
        Mins.Y = Math::Min(position.Y - radius, Mins.Y);
        Maxs.Y = Math::Max(position.Y + radius, Maxs.Y);
        Mins.Z = Math::Min(position.Z - radius, Mins.Z);
        Maxs.Z = Math::Max(position.Z + radius, Maxs.Z);
    }

    HK_FORCEINLINE void FromSphere(Float3 const& position, float radius)
    {
        Mins.X = position.X - radius;
        Maxs.X = position.X + radius;
        Mins.Y = position.Y - radius;
        Maxs.Y = position.Y + radius;
        Mins.Z = position.Z - radius;
        Maxs.Z = position.Z + radius;
    }

    constexpr HK_FORCEINLINE Float3 Center() const
    {
        return (Maxs + Mins) * 0.5f;
    }

    HK_FORCEINLINE float Radius() const
    {
        return HalfSize().Length();
    }

    HK_FORCEINLINE float InnerRadius() const
    {
        Float3 halfSize = HalfSize();
        return Math::Min3(halfSize.X, halfSize.Y, halfSize.Z);
    }

    constexpr HK_FORCEINLINE Float3 Size() const
    {
        return Maxs - Mins;
    }

    constexpr HK_FORCEINLINE Float3 HalfSize() const
    {
        return (Maxs - Mins) * 0.5f;
    }

    constexpr float Width() const
    {
        return Maxs.X - Mins.X;
    }

    constexpr float Height() const
    {
        return Maxs.Y - Mins.Y;
    }

    constexpr float Depth() const
    {
        return Maxs.Z - Mins.Z;
    }

    constexpr float Volume() const
    {
        return IsEmpty() ? 0.0f : (Maxs.X - Mins.X) * (Maxs.Y - Mins.Y) * (Maxs.Z - Mins.Z);
    }

    float LongestAxisSize() const
    {
        Float3 size    = Maxs - Mins;
        float  maxSize = size.X;
        if (size.Y > maxSize)
        {
            maxSize = size.Y;
        }
        if (size.Z > maxSize)
        {
            maxSize = size.Z;
        }
        return maxSize;
    }

    float ShortestAxisSize() const
    {
        Float3 size    = Maxs - Mins;
        float  minSize = size.X;
        if (size.Y < minSize)
        {
            minSize = size.Y;
        }
        if (size.Z < minSize)
        {
            minSize = size.Z;
        }
        return minSize;
    }

    void GetVertices(Float3 _Vertices[8]) const;

    constexpr bool IsEmpty() const
    {
        return Mins.X >= Maxs.X || Mins.Y >= Maxs.Y || Mins.Z >= Maxs.Z;
    }

    BvAxisAlignedBox Transform(Float3 const& origin, Float3x3 const& orient) const
    {
        const Float3 InCenter = Center();
        const Float3 InEdge   = HalfSize();
        const Float3 OutCenter(orient[0][0] * InCenter[0] + orient[1][0] * InCenter[1] + orient[2][0] * InCenter[2] + origin.X,
                               orient[0][1] * InCenter[0] + orient[1][1] * InCenter[1] + orient[2][1] * InCenter[2] + origin.Y,
                               orient[0][2] * InCenter[0] + orient[1][2] * InCenter[1] + orient[2][2] * InCenter[2] + origin.Z);
        const Float3 OutEdge(Math::Abs(orient[0][0]) * InEdge.X + Math::Abs(orient[1][0]) * InEdge.Y + Math::Abs(orient[2][0]) * InEdge.Z,
                             Math::Abs(orient[0][1]) * InEdge.X + Math::Abs(orient[1][1]) * InEdge.Y + Math::Abs(orient[2][1]) * InEdge.Z,
                             Math::Abs(orient[0][2]) * InEdge.X + Math::Abs(orient[1][2]) * InEdge.Y + Math::Abs(orient[2][2]) * InEdge.Z);
        return BvAxisAlignedBox(OutCenter - OutEdge, OutCenter + OutEdge);
    }

    BvAxisAlignedBox Transform(Float3x4 const& transformMatrix) const
    {
        const Float3 InCenter = Center();
        const Float3 InEdge   = HalfSize();
        const Float3 OutCenter(transformMatrix[0][0] * InCenter[0] + transformMatrix[0][1] * InCenter[1] + transformMatrix[0][2] * InCenter[2] + transformMatrix[0][3],
                               transformMatrix[1][0] * InCenter[0] + transformMatrix[1][1] * InCenter[1] + transformMatrix[1][2] * InCenter[2] + transformMatrix[1][3],
                               transformMatrix[2][0] * InCenter[0] + transformMatrix[2][1] * InCenter[1] + transformMatrix[2][2] * InCenter[2] + transformMatrix[2][3]);
        const Float3 OutEdge(Math::Abs(transformMatrix[0][0]) * InEdge.X + Math::Abs(transformMatrix[0][1]) * InEdge.Y + Math::Abs(transformMatrix[0][2]) * InEdge.Z,
                             Math::Abs(transformMatrix[1][0]) * InEdge.X + Math::Abs(transformMatrix[1][1]) * InEdge.Y + Math::Abs(transformMatrix[1][2]) * InEdge.Z,
                             Math::Abs(transformMatrix[2][0]) * InEdge.X + Math::Abs(transformMatrix[2][1]) * InEdge.Y + Math::Abs(transformMatrix[2][2]) * InEdge.Z);
        return BvAxisAlignedBox(OutCenter - OutEdge, OutCenter + OutEdge);
    }

    BvAxisAlignedBox FromOrientedBox(Float3 const& origin, Float3 const& halfSize, Float3x3 const& orient) const
    {
        const Float3 OutEdge(Math::Abs(orient[0][0]) * halfSize.X + Math::Abs(orient[1][0]) * halfSize.Y + Math::Abs(orient[2][0]) * halfSize.Z,
                             Math::Abs(orient[0][1]) * halfSize.X + Math::Abs(orient[1][1]) * halfSize.Y + Math::Abs(orient[2][1]) * halfSize.Z,
                             Math::Abs(orient[0][2]) * halfSize.X + Math::Abs(orient[1][2]) * halfSize.Y + Math::Abs(orient[2][2]) * halfSize.Z);
        return BvAxisAlignedBox(origin - OutEdge, origin + OutEdge);
    }

    static BvAxisAlignedBox const& Empty()
    {
        static BvAxisAlignedBox EmptyBox(Float3(9999999999.0f), Float3(-9999999999.0f));
        return EmptyBox;
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        Mins.Write(stream);
        Maxs.Write(stream);
    }

    void Read(IBinaryStream& stream)
    {
        Mins.Read(stream);
        Maxs.Read(stream);
    }
};

struct alignas(16) BvAxisAlignedBoxSSE
{
    Float4 Mins;
    Float4 Maxs;

    BvAxisAlignedBoxSSE() = default;

    BvAxisAlignedBoxSSE(BvAxisAlignedBox const& boundingBox)
    {
        *(Float3*)&Mins.X = boundingBox.Mins;
        *(Float3*)&Maxs.X = boundingBox.Maxs;
    }

    void operator=(BvAxisAlignedBox const& boundingBox)
    {
        *(Float3*)&Mins.X = boundingBox.Mins;
        *(Float3*)&Maxs.X = boundingBox.Maxs;
    }
};
