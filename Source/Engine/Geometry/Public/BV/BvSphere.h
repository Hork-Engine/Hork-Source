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

#include <Geometry/Public/Plane.h>
#include "BvAxisAlignedBox.h"

struct BvSphere
{
    Float3 Center;
    float  Radius;

    BvSphere() = default;

    explicit AN_FORCEINLINE BvSphere(float radius)
    {
        Center.X = 0;
        Center.Y = 0;
        Center.Z = 0;
        Radius   = radius;
    }

    AN_FORCEINLINE BvSphere(Float3 const& center, float radius)
    {
        Center = center;
        Radius = radius;
    }

    AN_FORCEINLINE void Clear()
    {
        Center.X = Center.Y = Center.Z = Radius = 0;
    }

    AN_FORCEINLINE BvSphere operator+(Float3 const& vec) const
    {
        return BvSphere(Center + vec, Radius);
    }

    AN_FORCEINLINE BvSphere operator-(Float3 const& vec) const
    {
        return BvSphere(Center - vec, Radius);
    }

    AN_FORCEINLINE BvSphere operator*(float scale) const
    {
        return BvSphere(Center, Radius * scale);
    }

    AN_FORCEINLINE BvSphere operator/(float scale) const
    {
        return BvSphere(Center, Radius / scale);
    }

    AN_FORCEINLINE BvSphere& operator+=(Float3 const& vec)
    {
        Center += vec;
        return *this;
    }

    AN_FORCEINLINE BvSphere& operator-=(Float3 const& vec)
    {
        Center -= vec;
        return *this;
    }

    AN_FORCEINLINE BvSphere& operator*=(float scale)
    {
        Radius *= scale;
        return *this;
    }

    AN_FORCEINLINE BvSphere& operator/=(float scale)
    {
        Radius /= scale;
        return *this;
    }

    AN_FORCEINLINE bool CompareEps(BvSphere const& rhs, float epsilon) const
    {
        return Center.CompareEps(rhs.Center, epsilon) && Math::CompareEps(Radius, rhs.Radius, epsilon);
    }

    AN_FORCEINLINE bool operator==(BvSphere const& rhs) const
    {
        return Center == rhs.Center && Radius == rhs.Radius;
    }

    AN_FORCEINLINE bool operator!=(BvSphere const& rhs) const
    {
        return !(operator==(rhs));
    }

    void FromPointsAverage(Float3 const* points, int numPoints)
    {
        if (numPoints <= 0)
        {
            return;
        }

        Center = points[0];
        for (int i = 1; i < numPoints; ++i)
        {
            Center += points[i];
        }
        Center /= numPoints;

        Radius = 0;
        for (int i = 0; i < numPoints; ++i)
        {
            const float distSqr = Center.DistSqr(points[i]);
            if (distSqr > Radius)
            {
                Radius = distSqr;
            }
        }

        Radius = Math::Sqrt(Radius);
    }

    void FromPoints(Float3 const* points, int numPoints)
    {
        if (numPoints <= 0)
        {
            return;
        }

        // build aabb

        Float3 mins(points[0]), maxs(points[0]);

        for (int i = 1; i < numPoints; i++)
        {
            Float3 const& v = points[i];
            if (v.X < mins.X) { mins.X = v.X; }
            if (v.X > maxs.X) { maxs.X = v.X; }
            if (v.Y < mins.Y) { mins.Y = v.Y; }
            if (v.Y > maxs.Y) { maxs.Y = v.Y; }
            if (v.Z < mins.Z) { mins.Z = v.Z; }
            if (v.Z > maxs.Z) { maxs.Z = v.Z; }
        }

        // get center from aabb
        Center = (mins + maxs) * 0.5;

        // calc Radius
        Radius = 0;
        for (int i = 0; i < numPoints; ++i)
        {
            const float distSqr = Center.DistSqr(points[i]);
            if (distSqr > Radius)
            {
                Radius = distSqr;
            }
        }
        Radius = Math::Sqrt(Radius);
    }

    void FromPointsAroundCenter(Float3 const& center, Float3 const* points, int numPoints)
    {
        if (numPoints <= 0)
        {
            return;
        }

        Center = center;

        // calc radius
        Radius = 0;
        for (int i = 0; i < numPoints; ++i)
        {
            const float distSqr = Center.DistSqr(points[i]);
            if (distSqr > Radius)
            {
                Radius = distSqr;
            }
        }
        Radius = Math::Sqrt(Radius);
    }

    AN_FORCEINLINE void FromAxisAlignedBox(BvAxisAlignedBox const& axisAlignedBox)
    {
        Center = (axisAlignedBox.Maxs + axisAlignedBox.Mins) * 0.5f;
        Radius = Center.Dist(axisAlignedBox.Maxs);
    }

    void AddPoint(Float3 const& point)
    {
        if (Center == Float3::Zero() && Radius == 0.0f)
        {
            Center = point;
        }
        else
        {
            const Float3 CenterDiff = point - Center;
            const float  LenSqr     = CenterDiff.LengthSqr();
            if (LenSqr > Radius * Radius)
            {
                const float Len = Math::Sqrt(LenSqr);
                Center += CenterDiff * 0.5f * (1.0f - Radius / Len);
                Radius += 0.5f * (Len - Radius);
            }
        }
    }

    void AddPoint(float x, float y, float z)
    {
        AddPoint({x, y, z});
    }

    void AddSphere(BvSphere const& sphere)
    {
        if (Radius == 0.0f)
        {
            *this = sphere;
        }
        else
        {
            const Float3 CenterDiff = Center - sphere.Center;
            const float  LenSqr     = CenterDiff.LengthSqr();
            const float  RadiusDiff = Radius - sphere.Radius;

            if (RadiusDiff * RadiusDiff >= LenSqr)
            {
                if (RadiusDiff < 0.0f)
                {
                    *this = sphere;
                }
            }
            else
            {
                constexpr float ZeroTolerance = 0.000001f;
                const float     Len           = Math::Sqrt(LenSqr);
                Center                        = Len > ZeroTolerance ? sphere.Center + CenterDiff * 0.5f * (Len + RadiusDiff) / Len : sphere.Center;
                Radius                        = (Len + sphere.Radius + Radius) * 0.5f;
            }
        }
    }

    float Dist(PlaneF const& plane) const
    {
        float d = plane.DistanceToPoint(Center);
        if (d > Radius)
        {
            return d - Radius;
        }
        if (d < -Radius)
        {
            return d + Radius;
        }
        return 0;
    }
};


struct alignas(16) BvSphereSSE : BvSphere
{};
