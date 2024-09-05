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

#include "BvAxisAlignedBox.h"
#include "BvOrientedBox.h"
#include "BvSphere.h"

HK_NAMESPACE_BEGIN

/*

Overlap tests:

Sphere - Sphere
Sphere - Point
Sphere - Triangle (not tested)
Sphere - Plane
Box - Box
Box - Sphere
Box - Triangle (not tested, not optimized)
Box - Triangle (approximation)
Box - Convex volume (overlap)
Box - Convex volume (box inside)
Box - Plane
Oriented Box - Oriented Box (not tested)
Oriented Box - Sphere (not tested)
Oriented Box - Box (not tested)
Oriented Box - Triangle (not tested, not optimized)
Oriented Box - Triangle (approximation)
Oriented Box - Convex volume (overlap)
Oriented Box - Convex volume (box inside)
Oriented Box - Plane

Intersection tests:

Ray - Sphere
Ray - Box
Ray - Oriented Box
Ray - Triangle
Ray - Plane
Ray - Ellipsoid

...And other

Future work:
More shapes? Cylinder, Cone, Capsule, Convex Hull, etc
Optimization tricks

*/

/*

Sphere overlap test

Sphere - Sphere
Sphere - Point
Sphere - Triangle (not tested)
Sphere - Plane

*/

/// Sphere - Sphere
HK_INLINE bool BvSphereOverlapSphere(BvSphere const& a, BvSphere const& b)
{
    const float R = a.Radius + b.Radius;
    return b.Center.DistSqr(a.Center) <= R * R;
}

/// Sphere - Point
HK_INLINE bool BvSphereOverlapPoint(BvSphere const& sphere, Float3 const& p)
{
    return p.DistSqr(sphere.Center) <= sphere.Radius * sphere.Radius;
}

/// Sphere - Triangle
HK_INLINE bool BvSphereOverlapTriangle(BvSphere const& sphere, Float3 const& p0, Float3 const& p1, Float3 const& p2)
{
    // based on OPCODE library

    const float radiusSqr = sphere.Radius * sphere.Radius;

    // Is vertices inside the sphere
    if ((p2 - sphere.Center).LengthSqr() <= radiusSqr)
    {
        return true;
    }

    if ((p1 - sphere.Center).LengthSqr() <= radiusSqr)
    {
        return true;
    }

    const Float3 vec = p0 - sphere.Center;
    const float vecDistSqr = vec.LengthSqr();
    if (vecDistSqr <= radiusSqr)
    {
        return true;
    }

    // Full distance test
    const Float3 e0 = p1 - p0;
    const Float3 e1 = p2 - p0;

    const float a00 = e0.LengthSqr();
    const float a01 = Math::Dot(e0, e1);
    const float a11 = e1.LengthSqr();
    const float b0 = Math::Dot(vec, e0);
    const float b1 = Math::Dot(vec, e1);
    const float det = Math::Abs(a00 * a11 - a01 * a01);
    float u = a01 * b1 - a11 * b0;
    float v = a01 * b0 - a00 * b1;
    float distSqr;

    if (u + v <= det)
    {
        if (u < 0.0f)
        {
            if (v < 0.0f) // region 4
            {
                if (b0 < 0.0f)
                {
                    // v = 0.0f;
                    if (-b0 >= a00)
                    {
                        // u = 1.0f;
                        distSqr = a00 + 2.0f * b0 + vecDistSqr;
                    }
                    else
                    {
                        u = -b0 / a00;
                        distSqr = b0 * u + vecDistSqr;
                    }
                }
                else
                {
                    // u = 0.0f;
                    if (b1 >= 0.0f)
                    {
                        // v = 0.0f;
                        distSqr = vecDistSqr;
                    }
                    else if (-b1 >= a11)
                    {
                        // v = 1.0f;
                        distSqr = a11 + 2.0f * b1 + vecDistSqr;
                    }
                    else
                    {
                        v = -b1 / a11;
                        distSqr = b1 * v + vecDistSqr;
                    }
                }
            }
            else
            { // region 3
                // u = 0.0f;
                if (b1 >= 0.0f)
                {
                    // v = 0.0f;
                    distSqr = vecDistSqr;
                }
                else if (-b1 >= a11)
                {
                    // v = 1.0f;
                    distSqr = a11 + 2.0f * b1 + vecDistSqr;
                }
                else
                {
                    v = -b1 / a11;
                    distSqr = b1 * v + vecDistSqr;
                }
            }
        }
        else if (v < 0.0f)
        { // region 5
            // v = 0.0f;
            if (b0 >= 0.0f)
            {
                // u = 0.0f;
                distSqr = vecDistSqr;
            }
            else if (-b0 >= a00)
            {
                // u = 1.0f;
                distSqr = a00 + 2.0f * b0 + vecDistSqr;
            }
            else
            {
                u = -b0 / a00;
                distSqr = b0 * u + vecDistSqr;
            }
        }
        else
        { // region 0
            // minimum at interior point
            if (det == 0.0f)
            {
                // u = 0.0f;
                // v = 0.0f;
                distSqr = Math::MaxValue<float>();
            }
            else
            {
                float invDet = 1.0f / det;
                u *= invDet;
                v *= invDet;
                distSqr = u * (a00 * u + a01 * v + 2.0f * b0) + v * (a01 * u + a11 * v + 2.0f * b1) + vecDistSqr;
            }
        }
    }
    else
    {
        float tmp0, tmp1, num, denom;

        if (u < 0.0f)
        { // region 2
            tmp0 = a01 + b0;
            tmp1 = a11 + b1;
            if (tmp1 > tmp0)
            {
                num = tmp1 - tmp0;
                denom = a00 - 2.0f * a01 + a11;
                if (num >= denom)
                {
                    // u = 1.0f;
                    // v = 0.0f;
                    distSqr = a00 + 2.0f * b0 + vecDistSqr;
                }
                else
                {
                    u = num / denom;
                    v = 1.0f - u;
                    distSqr = u * (a00 * u + a01 * v + 2.0f * b0) + v * (a01 * u + a11 * v + 2.0f * b1) + vecDistSqr;
                }
            }
            else
            {
                // u = 0.0f;
                if (tmp1 <= 0.0f)
                {
                    // v = 1.0f;
                    distSqr = a11 + 2.0f * b1 + vecDistSqr;
                }
                else if (b1 >= 0.0f)
                {
                    // v = 0.0f;
                    distSqr = vecDistSqr;
                }
                else
                {
                    v = -b1 / a11;
                    distSqr = b1 * v + vecDistSqr;
                }
            }
        }
        else if (v < 0.0f)
        { // region 6
            tmp0 = a01 + b1;
            tmp1 = a00 + b0;
            if (tmp1 > tmp0)
            {
                num = tmp1 - tmp0;
                denom = a00 - 2.0f * a01 + a11;
                if (num >= denom)
                {
                    // v = 1.0f;
                    // u = 0.0f;
                    distSqr = a11 + 2.0f * b1 + vecDistSqr;
                }
                else
                {
                    v = num / denom;
                    u = 1.0f - v;
                    distSqr = u * (a00 * u + a01 * v + 2.0f * b0) + v * (a01 * u + a11 * v + 2.0f * b1) + vecDistSqr;
                }
            }
            else
            {
                // v = 0.0f;
                if (tmp1 <= 0.0f)
                {
                    // u = 1.0f;
                    distSqr = a00 + 2.0f * b0 + vecDistSqr;
                }
                else if (b0 >= 0.0f)
                {
                    // u = 0.0f;
                    distSqr = vecDistSqr;
                }
                else
                {
                    u = -b0 / a00;
                    distSqr = b0 * u + vecDistSqr;
                }
            }
        }
        else
        { // region 1
            num = a11 + b1 - a01 - b0;
            if (num <= 0.0f)
            {
                // u = 0.0f;
                // v = 1.0f;
                distSqr = a11 + 2.0f * b1 + vecDistSqr;
            }
            else
            {
                denom = a00 - 2.0f * a01 + a11;
                if (num >= denom)
                {
                    // u = 1.0f;
                    // v = 0.0f;
                    distSqr = a00 + 2.0f * b0 + vecDistSqr;
                }
                else
                {
                    u = num / denom;
                    v = 1.0f - u;
                    distSqr = u * (a00 * u + a01 * v + 2.0f * b0) + v * (a01 * u + a11 * v + 2.0f * b1) + vecDistSqr;
                }
            }
        }
    }

    return Math::Abs(distSqr) < radiusSqr;
}

/// Sphere - Plane
HK_INLINE bool BvSphereOverlapPlane(BvSphere const& sphere, PlaneF const& plane)
{
    float dist = plane.DistanceToPoint(sphere.Center);
    if (dist > sphere.Radius || dist < -sphere.Radius)
    {
        return false;
    }
    return true;
}

/// Sphere - Plane
HK_INLINE int BvSphereOverlapPlaneSideMask(BvSphere const& sphere, PlaneF const& plane)
{
    float dist = plane.DistanceToPoint(sphere.Center);
    if (dist > sphere.Radius)
    {
        return 1; // front
    }
    else if (dist < -sphere.Radius)
    {
        return 2; // back
    }
    return 3; // overlap
}


/*

Box overlap test

Box - Box
Box - Point
Box - Sphere
Box - Triangle (not tested, not optimized)
Box - Triangle (approximation)
Box - Convex volume (overlap)
Box - Convex volume (box inside)
Box - Plane

Box/Box intersection

*/

/// AABB - AABB
HK_INLINE bool BvBoxOverlapBox(BvAxisAlignedBox const& a, BvAxisAlignedBox const& b)
{
    if (a.Maxs[0] < b.Mins[0] || a.Mins[0] > b.Maxs[0]) return false;
    if (a.Maxs[1] < b.Mins[1] || a.Mins[1] > b.Maxs[1]) return false;
    if (a.Maxs[2] < b.Mins[2] || a.Mins[2] > b.Maxs[2]) return false;
    return true;
}

/// AABB - AABB (2D)
HK_INLINE bool BvBoxOverlapBox2D(Float2 const& aMins, Float2 const& aMaxs, Float2 const& bMins, Float2 const& bMaxs)
{
    if (aMaxs[0] < bMins[0] || aMins[0] > bMaxs[0]) return false;
    if (aMaxs[1] < bMins[1] || aMins[1] > bMaxs[1]) return false;
    return true;
}

/// AABB - Point
HK_INLINE bool BvBoxOverlapPoint(BvAxisAlignedBox const& box, Float3 const& p)
{
    if (p.X < box.Mins.X || p.Y < box.Mins.Y || p.Z < box.Mins.Z || p.X > box.Maxs.X || p.Y > box.Maxs.Y || p.Z > box.Maxs.Z)
    {
        return false;
    }
    return true;
}

/// AABB - Sphere
HK_INLINE bool BvBoxOverlapSphere(BvAxisAlignedBox const& box, BvSphere const& sphere)
{
#if 0
    float d = 0, dist;
    for ( int i = 0 ; i < 3 ; i++ ) {
       // if sphere center lies behind of box
       dist = sphere.Center[i] - box.Mins[i];
       if ( dist < 0 ) {
          d += dist * dist; // summarize distance squire on this axis
       }
       // if sphere center lies ahead of box,
       dist = sphere.Center[i] - box.Maxs[i];
       if ( dist > 0 ) {
          d += dist * dist; // summarize distance squire on this axis
       }
    }
    return d <= sphere.Radius * sphere.Radius;
#else
    const Float3 difMins = sphere.Center - box.Mins;
    const Float3 difMaxs = sphere.Center - box.Maxs;

    return ((difMins.X < 0.0f) * difMins.X * difMins.X + (difMins.Y < 0.0f) * difMins.Y * difMins.Y + (difMins.Z < 0.0f) * difMins.Z * difMins.Z + (difMaxs.X > 0.0f) * difMaxs.X * difMaxs.X + (difMaxs.Y > 0.0f) * difMaxs.Y * difMaxs.Y + (difMaxs.Z > 0.0f) * difMaxs.Z * difMaxs.Z) <= sphere.Radius * sphere.Radius;
#endif
}

/// AABB - Triangle
HK_INLINE bool BvBoxOverlapTriangle(BvAxisAlignedBox const& box, Float3 const& p0, Float3 const& p1, Float3 const& p2)
{
    Float3 n;
    float p;
    float d0, d1;
    float radius;

    const Float3 boxCenter = box.Center();
    const Float3 halfSize = box.HalfSize();

    // vector from box center to p0
    const Float3 distVec = p0 - boxCenter;

    // triangle edges
    const Float3 edge0 = p1 - p0;
    const Float3 edge1 = p2 - p0;
    const Float3 edge2 = edge1 - edge0;

    // triangle normal (not normalized)
    n = Math::Cross(edge0, edge1);

    if (Math::Abs(Math::Dot(n, distVec)) > halfSize.X * Math::Abs(n.X) + halfSize.Y * Math::Abs(n.Y) + halfSize.Z * Math::Abs(n.Z))
    {
        return false;
    }

    for (int i = 0; i < 3; i++)
    {
        p = distVec[i];
        d0 = edge0[i];
        d1 = edge1[i];
        radius = halfSize[i];

        if (Math::Min(p, Math::Min(p + d0, p + d1)) > radius || Math::Max(p, Math::Max(p + d0, p + d1)) < -radius)
        {
            return false;
        }
    }

    // check axisX and edge0
    n = Float3(0, -edge0.Z, edge0.Y);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge1);

    radius = halfSize[1] * Math::Abs(edge0[2]) + halfSize[2] * Math::Abs(edge0[1]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisX and edge1
    n = Float3(0, -edge1.Z, edge1.Y);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = halfSize[1] * Math::Abs(edge1[2]) + halfSize[2] * Math::Abs(edge1[1]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisX and edge2
    n = Float3(0, -edge2.Z, edge2.Y);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = halfSize[1] * Math::Abs(edge2[2]) + halfSize[2] * Math::Abs(edge2[1]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisY and edge0
    n = Float3(edge0.Z, 0, -edge0.X);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge1);

    radius = halfSize[0] * Math::Abs(edge0[2]) + halfSize[2] * Math::Abs(edge0[0]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisY and edge1
    n = Float3(edge1.Z, 0, -edge1.X);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = halfSize[0] * Math::Abs(edge1[2]) + halfSize[2] * Math::Abs(edge1[0]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisY and edge2
    n = Float3(edge2.Z, 0, -edge2.X);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = halfSize[0] * Math::Abs(edge2[2]) + halfSize[2] * Math::Abs(edge2[0]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisZ and edge0
    n = Float3(-edge0.Y, edge0.X, 0);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge1);

    radius = halfSize[0] * Math::Abs(edge0[1]) + halfSize[1] * Math::Abs(edge0[0]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisZ and edge1
    n = Float3(-edge1.Y, edge1.X, 0);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = halfSize[0] * Math::Abs(edge1[1]) + halfSize[1] * Math::Abs(edge1[0]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisZ and edge2
    n = Float3(-edge2.Y, edge2.X, 0);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = halfSize[0] * Math::Abs(edge2[1]) + halfSize[1] * Math::Abs(edge2[0]);

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    return true;
}

/// AABB - Triangle (approximation)
HK_INLINE bool BvBoxOverlapTriangle_FastApproximation(BvAxisAlignedBox const& box, Float3 const& p0, Float3 const& p1, Float3 const& p2)
{

    // Simple fast triangle - AABB overlap test

    BvAxisAlignedBox triangleBounds;

    triangleBounds.Mins.X = Math::Min3(p0.X, p1.X, p2.X);
    triangleBounds.Mins.Y = Math::Min3(p0.Y, p1.Y, p2.Y);
    triangleBounds.Mins.Z = Math::Min3(p0.Z, p1.Z, p2.Z);

    triangleBounds.Maxs.X = Math::Max3(p0.X, p1.X, p2.X);
    triangleBounds.Maxs.Y = Math::Max3(p0.Y, p1.Y, p2.Y);
    triangleBounds.Maxs.Z = Math::Max3(p0.Z, p1.Z, p2.Z);

    return BvBoxOverlapBox(box, triangleBounds);
}

/// AABB intersection box
HK_INLINE bool BvGetBoxIntersection(BvAxisAlignedBox const& a, BvAxisAlignedBox const& b, BvAxisAlignedBox& intersection)
{
    const float x_min = Math::Max(a.Mins[0], b.Mins[0]);
    const float x_max = Math::Min(a.Maxs[0], b.Maxs[0]);
    if (x_max <= x_min)
    {
        return false;
    }

    const float y_min = Math::Max(a.Mins[1], b.Mins[1]);
    const float y_max = Math::Min(a.Maxs[1], b.Maxs[1]);
    if (y_max <= y_min)
    {
        return false;
    }

    const float z_min = Math::Max(a.Mins[2], b.Mins[2]);
    const float z_max = Math::Min(a.Maxs[2], b.Maxs[2]);
    if (z_max <= z_min)
    {
        return false;
    }

    intersection.Mins[0] = x_min;
    intersection.Mins[1] = y_min;
    intersection.Mins[2] = z_min;

    intersection.Maxs[0] = x_max;
    intersection.Maxs[1] = y_max;
    intersection.Maxs[2] = z_max;

    return true;
}

/// AABB overlap convex volume
HK_INLINE bool BvBoxOverlapConvex(BvAxisAlignedBox const& box, PlaneF const* planes, int numPlanes)
{
    for (int i = 0; i < numPlanes; i++)
    {
        PlaneF const& plane = planes[i];

        if (plane.DistanceToPoint({plane.Normal.X > 0.0f ? box.Mins.X : box.Maxs.X,
                                   plane.Normal.Y > 0.0f ? box.Mins.Y : box.Maxs.Y,
                                   plane.Normal.Z > 0.0f ? box.Mins.Z : box.Maxs.Z}) > 0)
        {
            return false;
        }
    }
    return true;
}

/// AABB inside convex volume
HK_INLINE bool BvBoxInsideConvex(BvAxisAlignedBox const& box, PlaneF const* planes, int numPlanes)
{
    for (int i = 0; i < numPlanes; i++)
    {
        PlaneF const& plane = planes[i];

        if (plane.DistanceToPoint({plane.Normal.X < 0.0f ? box.Mins.X : box.Maxs.X,
                                   plane.Normal.Y < 0.0f ? box.Mins.Y : box.Maxs.Y,
                                   plane.Normal.Z < 0.0f ? box.Mins.Z : box.Maxs.Z}) > 0)
        {
            return false;
        }
    }
    return true;
}

/// AABB overlap plane
HK_INLINE bool BvBoxOverlapPlane(Float3 const* boxVertices, PlaneF const& plane)
{
    bool front = false;
    bool back = false;

    for (int i = 0; i < 8; i++)
    {
        float dist = plane.DistanceToPoint(boxVertices[i]);
        if (dist > 0)
        {
            front = true;
        }
        else
        {
            back = true;
        }
    }

    return front && back;
}

/// AABB overlap plane
HK_INLINE int BvBoxOverlapPlaneSideMask(Float3 const& mins, Float3 const& maxs, PlaneF const& plane)
{
    int sideMask = 0;

    if (plane.Normal[0] * mins[0] + plane.Normal[1] * mins[1] + plane.Normal[2] * mins[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;
    if (plane.Normal[0] * maxs[0] + plane.Normal[1] * mins[1] + plane.Normal[2] * mins[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;
    if (plane.Normal[0] * mins[0] + plane.Normal[1] * maxs[1] + plane.Normal[2] * mins[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;
    if (plane.Normal[0] * maxs[0] + plane.Normal[1] * maxs[1] + plane.Normal[2] * mins[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;
    if (plane.Normal[0] * mins[0] + plane.Normal[1] * mins[1] + plane.Normal[2] * maxs[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;
    if (plane.Normal[0] * maxs[0] + plane.Normal[1] * mins[1] + plane.Normal[2] * maxs[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;
    if (plane.Normal[0] * mins[0] + plane.Normal[1] * maxs[1] + plane.Normal[2] * maxs[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;
    if (plane.Normal[0] * maxs[0] + plane.Normal[1] * maxs[1] + plane.Normal[2] * maxs[2] + plane.D > 0) sideMask |= 1;
    else
        sideMask |= 2;

    return sideMask;
}

/// AABB overlap plane
HK_INLINE bool BvBoxOverlapPlane(Float3 const& mins, Float3 const& maxs, PlaneF const& plane)
{
    return BvBoxOverlapPlaneSideMask(mins, maxs, plane) == 3;
}

/// AABB overlap plane
HK_INLINE bool BvBoxOverlapPlane(BvAxisAlignedBox const& box, PlaneF const& plane)
{
    return BvBoxOverlapPlane(box.Mins, box.Maxs, plane);
}

/// AABB overlap plane based on precomputed plane axial type and plane sign bits
HK_INLINE bool BvBoxOverlapPlaneFast(BvAxisAlignedBox const& box, PlaneF const& plane, int axialType, int signBits)
{
    const float dist = plane.GetDist();

    if (axialType < 3)
    {
        if (dist < box.Mins[axialType])
        {
            return false;
        }
        if (dist > box.Maxs[axialType])
        {
            return false;
        }
        return true;
    }

    float d1, d2;
    switch (signBits)
    {
        case 0:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 1:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 2:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 3:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 4:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            break;
        case 5:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            break;
        case 6:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            break;
        case 7:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            break;
        default:
            d1 = d2 = 0;
            break;
    }

    return (d1 >= dist) && (d2 < dist);
}

/// AABB overlap plane based on precomputed plane axial type and plane sign bits
HK_INLINE int BvBoxOverlapPlaneSideMask(BvAxisAlignedBox const& box, PlaneF const& plane, int axialType, int signBits)
{
    const float dist = plane.GetDist();

    if (axialType < 3)
    {
        if (dist <= box.Mins[axialType])
        {
            return 1;
        }
        if (dist >= box.Maxs[axialType])
        {
            return 2;
        }
        return 3;
    }

    float d1, d2;
    switch (signBits)
    {
        case 0:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 1:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 2:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 3:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            break;
        case 4:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            break;
        case 5:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Maxs[2];
            break;
        case 6:
            d1 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            break;
        case 7:
            d1 = plane.Normal[0] * box.Mins[0] + plane.Normal[1] * box.Mins[1] + plane.Normal[2] * box.Mins[2];
            d2 = plane.Normal[0] * box.Maxs[0] + plane.Normal[1] * box.Maxs[1] + plane.Normal[2] * box.Maxs[2];
            break;
        default:
            d1 = d2 = 0;
            break;
    }

    int sideMask = (d1 >= dist);

    if (d2 < dist)
    {
        sideMask |= 2;
    }

    return sideMask;
}


/*

Oriented box overlap test

Oriented Box - Oriented Box (not tested)
Oriented Box - Sphere (not tested)
Oriented Box - Box (not tested)
Oriented Box - Triangle (not tested, not optimized)
Oriented Box - Triangle (approximation)
Oriented Box - Convex volume (overlap)
Oriented Box - Convex volume (box inside)
Oriented Box - Plane

*/

/// OBB - OBB
HK_INLINE bool BvOrientedBoxOverlapOrientedBox(BvOrientedBox const& box1, BvOrientedBox const& box2)
{
    // Code based on http://gdlinks.hut.ru/cdfaq/obb.shtml

    const Float3x3 orientInversed = box1.Orient.Transposed();

    // transform OBB2 position to OBB1 space
    const Float3 T = orientInversed * (box2.Center - box1.Center);

    // transform OBB2 orientation to OBB1 space
    const Float3x3 R = orientInversed * box2.Orient;

    float ra, rb;

    for (int i = 0; i < 3; i++)
    {
        ra = box1.HalfSize[i];
        rb = box2.HalfSize[0] * Math::Abs(R[i][0]) + box2.HalfSize[1] * Math::Abs(R[i][1]) + box2.HalfSize[2] * Math::Abs(R[i][2]);
        if (Math::Abs(T[i]) > ra + rb)
            return false;
    }

    for (int i = 0; i < 3; i++)
    {
        ra = box1.HalfSize[0] * Math::Abs(R[0][i]) + box1.HalfSize[1] * Math::Abs(R[1][i]) + box1.HalfSize[2] * Math::Abs(R[2][i]);
        rb = box2.HalfSize[i];
        if (Math::Abs((T[0] * R[0][i] + T[1] * R[1][i] + T[2] * R[2][i])) > ra + rb)
            return false;
    }

    ra = box1.HalfSize[1] * Math::Abs(R[2][0]) + box1.HalfSize[2] * Math::Abs(R[1][0]);
    rb = box2.HalfSize[1] * Math::Abs(R[0][2]) + box2.HalfSize[2] * Math::Abs(R[0][1]);
    if (Math::Abs((T[2] * R[1][0] - T[1] * R[2][0])) > ra + rb)
        return false;

    ra = box1.HalfSize[1] * Math::Abs(R[2][1]) + box1.HalfSize[2] * Math::Abs(R[1][1]);
    rb = box2.HalfSize[0] * Math::Abs(R[0][2]) + box2.HalfSize[2] * Math::Abs(R[0][0]);
    if (Math::Abs((T[2] * R[1][1] - T[1] * R[2][1])) > ra + rb)
        return false;

    ra = box1.HalfSize[1] * Math::Abs(R[2][2]) + box1.HalfSize[2] * Math::Abs(R[1][2]);
    rb = box2.HalfSize[0] * Math::Abs(R[0][1]) + box2.HalfSize[1] * Math::Abs(R[0][0]);
    if (Math::Abs((T[2] * R[1][2] - T[1] * R[2][2])) > ra + rb)
        return false;

    ra = box1.HalfSize[0] * Math::Abs(R[2][0]) + box1.HalfSize[2] * Math::Abs(R[0][0]);
    rb = box2.HalfSize[1] * Math::Abs(R[1][2]) + box2.HalfSize[2] * Math::Abs(R[1][1]);
    if (Math::Abs((T[0] * R[2][0] - T[2] * R[0][0])) > ra + rb)
        return false;

    ra = box1.HalfSize[0] * Math::Abs(R[2][1]) + box1.HalfSize[2] * Math::Abs(R[0][1]);
    rb = box2.HalfSize[0] * Math::Abs(R[1][2]) + box2.HalfSize[2] * Math::Abs(R[1][0]);
    if (Math::Abs((T[0] * R[2][1] - T[2] * R[0][1])) > ra + rb)
        return false;

    ra = box1.HalfSize[0] * Math::Abs(R[2][2]) + box1.HalfSize[2] * Math::Abs(R[0][2]);
    rb = box2.HalfSize[0] * Math::Abs(R[1][1]) + box2.HalfSize[1] * Math::Abs(R[1][0]);
    if (Math::Abs((T[0] * R[2][2] - T[2] * R[0][2])) > ra + rb)
        return false;

    ra = box1.HalfSize[0] * Math::Abs(R[1][0]) + box1.HalfSize[1] * Math::Abs(R[0][0]);
    rb = box2.HalfSize[1] * Math::Abs(R[2][2]) + box2.HalfSize[2] * Math::Abs(R[2][1]);
    if (Math::Abs((T[1] * R[0][0] - T[0] * R[1][0])) > ra + rb)
        return false;

    ra = box1.HalfSize[0] * Math::Abs(R[1][1]) + box1.HalfSize[1] * Math::Abs(R[0][1]);
    rb = box2.HalfSize[0] * Math::Abs(R[2][2]) + box2.HalfSize[2] * Math::Abs(R[2][0]);
    if (Math::Abs((T[1] * R[0][1] - T[0] * R[1][1])) > ra + rb)
        return false;

    ra = box1.HalfSize[0] * Math::Abs(R[1][2]) + box1.HalfSize[1] * Math::Abs(R[0][2]);
    rb = box2.HalfSize[0] * Math::Abs(R[2][1]) + box2.HalfSize[1] * Math::Abs(R[2][0]);
    if (Math::Abs((T[1] * R[0][2] - T[0] * R[1][2])) > ra + rb)
        return false;

    return true;
}

/// OBB - Sphere
HK_INLINE bool BvOrientedBoxOverlapSphere(BvOrientedBox const& orientedBox, BvSphere const& sphere)
{
    // Transform sphere center to OBB space
    const Float3 sphereCenter = orientedBox.Orient.Transposed() * (sphere.Center - orientedBox.Center);

    const Float3 difMins = sphereCenter + orientedBox.HalfSize;
    const Float3 difMaxs = sphereCenter - orientedBox.HalfSize;

    return ((difMins.X < 0.0f) * difMins.X * difMins.X + (difMins.Y < 0.0f) * difMins.Y * difMins.Y + (difMins.Z < 0.0f) * difMins.Z * difMins.Z + (difMaxs.X > 0.0f) * difMaxs.X * difMaxs.X + (difMaxs.Y > 0.0f) * difMaxs.Y * difMaxs.Y + (difMaxs.Z > 0.0f) * difMaxs.Z * difMaxs.Z) <= sphere.Radius * sphere.Radius;
}

/// OBB - AABB
HK_INLINE bool BvOrientedBoxOverlapBox(BvOrientedBox const& orientedBox, Float3 const& boxCenter, Float3 const& boxHalfSize)
{
    // transform OBB position to AABB space
    const Float3 T = orientedBox.Center - boxCenter;

    // OBB orientation relative to AABB space
    const Float3x3& R = orientedBox.Orient;

    float ra, rb;

    for (int i = 0; i < 3; i++)
    {
        ra = boxHalfSize[i];
        rb = orientedBox.HalfSize[0] * Math::Abs(R[i][0]) + orientedBox.HalfSize[1] * Math::Abs(R[i][1]) + orientedBox.HalfSize[2] * Math::Abs(R[i][2]);
        if (Math::Abs(T[i]) > ra + rb)
            return false;
    }

    for (int i = 0; i < 3; i++)
    {
        ra = boxHalfSize[0] * Math::Abs(R[0][i]) + boxHalfSize[1] * Math::Abs(R[1][i]) + boxHalfSize[2] * Math::Abs(R[2][i]);
        rb = orientedBox.HalfSize[i];
        if (Math::Abs((T[0] * R[0][i] + T[1] * R[1][i] + T[2] * R[2][i])) > ra + rb)
            return false;
    }

    ra = boxHalfSize[1] * Math::Abs(R[2][0]) + boxHalfSize[2] * Math::Abs(R[1][0]);
    rb = orientedBox.HalfSize[1] * Math::Abs(R[0][2]) + orientedBox.HalfSize[2] * Math::Abs(R[0][1]);
    if (Math::Abs((T[2] * R[1][0] - T[1] * R[2][0])) > ra + rb)
        return false;

    ra = boxHalfSize[1] * Math::Abs(R[2][1]) + boxHalfSize[2] * Math::Abs(R[1][1]);
    rb = orientedBox.HalfSize[0] * Math::Abs(R[0][2]) + orientedBox.HalfSize[2] * Math::Abs(R[0][0]);
    if (Math::Abs((T[2] * R[1][1] - T[1] * R[2][1])) > ra + rb)
        return false;

    ra = boxHalfSize[1] * Math::Abs(R[2][2]) + boxHalfSize[2] * Math::Abs(R[1][2]);
    rb = orientedBox.HalfSize[0] * Math::Abs(R[0][1]) + orientedBox.HalfSize[1] * Math::Abs(R[0][0]);
    if (Math::Abs((T[2] * R[1][2] - T[1] * R[2][2])) > ra + rb)
        return false;

    ra = boxHalfSize[0] * Math::Abs(R[2][0]) + boxHalfSize[2] * Math::Abs(R[0][0]);
    rb = orientedBox.HalfSize[1] * Math::Abs(R[1][2]) + orientedBox.HalfSize[2] * Math::Abs(R[1][1]);
    if (Math::Abs((T[0] * R[2][0] - T[2] * R[0][0])) > ra + rb)
        return false;

    ra = boxHalfSize[0] * Math::Abs(R[2][1]) + boxHalfSize[2] * Math::Abs(R[0][1]);
    rb = orientedBox.HalfSize[0] * Math::Abs(R[1][2]) + orientedBox.HalfSize[2] * Math::Abs(R[1][0]);
    if (Math::Abs((T[0] * R[2][1] - T[2] * R[0][1])) > ra + rb)
        return false;

    ra = boxHalfSize[0] * Math::Abs(R[2][2]) + boxHalfSize[2] * Math::Abs(R[0][2]);
    rb = orientedBox.HalfSize[0] * Math::Abs(R[1][1]) + orientedBox.HalfSize[1] * Math::Abs(R[1][0]);
    if (Math::Abs((T[0] * R[2][2] - T[2] * R[0][2])) > ra + rb)
        return false;

    ra = boxHalfSize[0] * Math::Abs(R[1][0]) + boxHalfSize[1] * Math::Abs(R[0][0]);
    rb = orientedBox.HalfSize[1] * Math::Abs(R[2][2]) + orientedBox.HalfSize[2] * Math::Abs(R[2][1]);
    if (Math::Abs((T[1] * R[0][0] - T[0] * R[1][0])) > ra + rb)
        return false;

    ra = boxHalfSize[0] * Math::Abs(R[1][1]) + boxHalfSize[1] * Math::Abs(R[0][1]);
    rb = orientedBox.HalfSize[0] * Math::Abs(R[2][2]) + orientedBox.HalfSize[2] * Math::Abs(R[2][0]);
    if (Math::Abs((T[1] * R[0][1] - T[0] * R[1][1])) > ra + rb)
        return false;

    ra = boxHalfSize[0] * Math::Abs(R[1][2]) + boxHalfSize[1] * Math::Abs(R[0][2]);
    rb = orientedBox.HalfSize[0] * Math::Abs(R[2][1]) + orientedBox.HalfSize[1] * Math::Abs(R[2][0]);
    if (Math::Abs((T[1] * R[0][2] - T[0] * R[1][2])) > ra + rb)
        return false;

    return true;
}

/// OBB - AABB
HK_INLINE bool BvOrientedBoxOverlapBox(BvOrientedBox const& orientedBox, BvAxisAlignedBox const& box)
{
    return BvOrientedBoxOverlapBox(orientedBox, box.Center(), box.HalfSize());
}

/// OBB - Triangle
HK_INLINE bool BvOrientedBoxOverlapTriangle(BvOrientedBox const& orientedBox, Float3 const& p0, Float3 const& p1, Float3 const& p2)
{

    // Code based on http://gdlinks.hut.ru/cdfaq/obb.shtml

    Float3 n;
    float p;
    float d0, d1;
    float radius;

    // vector from box center to p0
    const Float3 distVec = p0 - orientedBox.Center;

    // triangle edges
    const Float3 edge0 = p1 - p0;
    const Float3 edge1 = p2 - p0;
    const Float3 edge2 = edge1 - edge0;

    // triangle normal (not normalized)
    n = Math::Cross(edge0, edge1);

    if (Math::Abs(Math::Dot(n, distVec)) > (orientedBox.HalfSize.X * Math::Abs(Math::Dot(n, orientedBox.Orient[0])) + orientedBox.HalfSize.Y * Math::Abs(Math::Dot(n, orientedBox.Orient[1])) + orientedBox.HalfSize.Z * Math::Abs(Math::Dot(n, orientedBox.Orient[2]))))
        return false;

    for (int i = 0; i < 3; i++)
    {
        p = Math::Dot(orientedBox.Orient[i], distVec);
        d0 = Math::Dot(orientedBox.Orient[i], edge0);
        d1 = Math::Dot(orientedBox.Orient[i], edge1);
        radius = orientedBox.HalfSize[i];

        if (Math::Min(p, Math::Min(p + d0, p + d1)) > radius || Math::Max(p, Math::Max(p + d0, p + d1)) < -radius)
        {
            return false;
        }
    }

    // check axisX and edge0
    n = Math::Cross(orientedBox.Orient[0], edge0);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge1);

    radius = orientedBox.HalfSize[1] * Math::Abs(Math::Dot(orientedBox.Orient[2], edge0)) + orientedBox.HalfSize[2] * Math::Abs(Math::Dot(orientedBox.Orient[1], edge0));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisX and edge1
    n = Math::Cross(orientedBox.Orient[0], edge1);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = orientedBox.HalfSize[1] * Math::Abs(Math::Dot(orientedBox.Orient[2], edge1)) + orientedBox.HalfSize[2] * Math::Abs(Math::Dot(orientedBox.Orient[1], edge1));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisX and edge2
    n = Math::Cross(orientedBox.Orient[0], edge2);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = orientedBox.HalfSize[1] * Math::Abs(Math::Dot(orientedBox.Orient[2], edge2)) + orientedBox.HalfSize[2] * Math::Abs(Math::Dot(orientedBox.Orient[1], edge2));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisY and edge0
    n = Math::Cross(orientedBox.Orient[1], edge0);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge1);

    radius = orientedBox.HalfSize[0] * Math::Abs(Math::Dot(orientedBox.Orient[2], edge0)) + orientedBox.HalfSize[2] * Math::Abs(Math::Dot(orientedBox.Orient[0], edge0));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisY and edge1
    n = Math::Cross(orientedBox.Orient[1], edge1);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = orientedBox.HalfSize[0] * Math::Abs(Math::Dot(orientedBox.Orient[2], edge1)) + orientedBox.HalfSize[2] * Math::Abs(Math::Dot(orientedBox.Orient[0], edge1));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisY and edge2
    n = Math::Cross(orientedBox.Orient[1], edge2);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = orientedBox.HalfSize[0] * Math::Abs(Math::Dot(orientedBox.Orient[2], edge2)) + orientedBox.HalfSize[2] * Math::Abs(Math::Dot(orientedBox.Orient[0], edge2));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisZ and edge0
    n = Math::Cross(orientedBox.Orient[2], edge0);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge1);

    radius = orientedBox.HalfSize[0] * Math::Abs(Math::Dot(orientedBox.Orient[1], edge0)) + orientedBox.HalfSize[1] * Math::Abs(Math::Dot(orientedBox.Orient[0], edge0));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisZ and edge1
    n = Math::Cross(orientedBox.Orient[2], edge1);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = orientedBox.HalfSize[0] * Math::Abs(Math::Dot(orientedBox.Orient[1], edge1)) + orientedBox.HalfSize[1] * Math::Abs(Math::Dot(orientedBox.Orient[0], edge1));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    // check axisZ and edge2
    n = Math::Cross(orientedBox.Orient[2], edge2);
    p = Math::Dot(n, distVec);
    d0 = Math::Dot(n, edge0);

    radius = orientedBox.HalfSize[0] * Math::Abs(Math::Dot(orientedBox.Orient[1], edge2)) + orientedBox.HalfSize[1] * Math::Abs(Math::Dot(orientedBox.Orient[0], edge2));

    if (Math::Min(p, p + d0) > radius || Math::Max(p, p + d0) < -radius)
        return false;

    return true;
}

/// OBB - Triangle (approximation)
HK_INLINE bool BvOrientedBoxOverlapTriangle_FastApproximation(BvOrientedBox const& orientedBox, Float3 const& p0, Float3 const& p1, Float3 const& p2)
{

    // Simple fast triangle - AABB overlap test

    BvAxisAlignedBox triangleBounds;

    triangleBounds.Mins.X = Math::Min3(p0.X, p1.X, p2.X);
    triangleBounds.Mins.Y = Math::Min3(p0.Y, p1.Y, p2.Y);
    triangleBounds.Mins.Z = Math::Min3(p0.Z, p1.Z, p2.Z);

    triangleBounds.Maxs.X = Math::Max3(p0.X, p1.X, p2.X);
    triangleBounds.Maxs.Y = Math::Max3(p0.Y, p1.Y, p2.Y);
    triangleBounds.Maxs.Z = Math::Max3(p0.Z, p1.Z, p2.Z);

    return BvOrientedBoxOverlapBox(orientedBox, triangleBounds);
}

/// OBB overlap convex
HK_INLINE bool BvOrientedBoxOverlapConvex(BvOrientedBox const& b, PlaneF const* planes, int numPlanes)
{
    Float3 point;

    for (int i = 0; i < numPlanes; i++)
    {
        PlaneF const& plane = planes[i];

        const float x = Math::Dot(b.Orient[0], plane.Normal) > 0.0f ? -b.HalfSize[0] : b.HalfSize[0];
        const float y = Math::Dot(b.Orient[1], plane.Normal) > 0.0f ? -b.HalfSize[1] : b.HalfSize[1];
        const float z = Math::Dot(b.Orient[2], plane.Normal) > 0.0f ? -b.HalfSize[2] : b.HalfSize[2];

        point = b.Center + (x * b.Orient[0] + y * b.Orient[1] + z * b.Orient[2]);

        if (plane.DistanceToPoint(point) > 0.0f)
        {
            return false;
        }
    }

    return true;
}

/// OBB inside convex
HK_INLINE bool BvOrientedBoxInsideConvex(BvOrientedBox const& b, PlaneF const* planes, int numPlanes)
{
    Float3 point;

    for (int i = 0; i < numPlanes; i++)
    {
        PlaneF const& plane = planes[i];

        const float x = Math::Dot(b.Orient[0], plane.Normal) < 0.0f ? -b.HalfSize[0] : b.HalfSize[0];
        const float y = Math::Dot(b.Orient[1], plane.Normal) < 0.0f ? -b.HalfSize[1] : b.HalfSize[1];
        const float z = Math::Dot(b.Orient[2], plane.Normal) < 0.0f ? -b.HalfSize[2] : b.HalfSize[2];

        point = b.Center + (x * b.Orient[0] + y * b.Orient[1] + z * b.Orient[2]);

        if (plane.DistanceToPoint(point) > 0.0f)
        {
            return false;
        }
    }

    return true;
}

/// OBB overlap plane
HK_INLINE bool BvOrientedBoxOverlapPlane(BvOrientedBox const& orientedBox, PlaneF const& plane)
{
    Float3 vertices[8];

    orientedBox.GetVertices(vertices);

    return BvBoxOverlapPlane(vertices, plane);
}

/*

Ray intersection test

Ray - Sphere
Ray - Box
Ray - Oriented Box
Ray - Triangle
Ray - Plane
Ray - Ellipsoid

*/

/// Ray - Sphere
HK_INLINE bool BvRayIntersectSphere(Float3 const& rayStart, Float3 const& rayDir, BvSphere const& sphere, float& outMin, float& outMax)
{
    const Float3 k = rayStart - sphere.Center;
    const float b = Math::Dot(k, rayDir);
    float distance = b * b - k.LengthSqr() + sphere.Radius * sphere.Radius;
    if (distance < 0.0f)
    {
        return false;
    }
    distance = Math::Sqrt(distance);
    Math::MinMax(-b + distance, -b - distance, outMin, outMax);
    return outMin > 0.0f || outMax > 0.0f;
}

/// Ray - Sphere
HK_INLINE bool BvRayIntersectSphere(Float3 const& rayStart, Float3 const& rayDir, BvSphere const& sphere, float& distance)
{
    const Float3 k = rayStart - sphere.Center;
    const float b = Math::Dot(k, rayDir);
    distance = b * b - k.LengthSqr() + sphere.Radius * sphere.Radius;
    if (distance < 0.0f)
    {
        return false;
    }
    float t1, t2;
    distance = Math::Sqrt(distance);
    Math::MinMax(-b + distance, -b - distance, t1, t2);
    distance = t1 >= 0.0f ? t1 : t2;
    return distance > 0.0f;
}

/// Ray - AABB
/// If raydir is normalized, min and max will be in rage [0,raylength]
/// If raydir is non-normalized, min and max will be in rage [0,1]
HK_INLINE bool BvRayIntersectBox(Float3 const& rayStart, Float3 const& invRayDir, BvAxisAlignedBox const& box, float& outMin, float& outMax)
{
#if 0
    float lo = invRayDir.X*(box.Mins.X - rayStart.X);
    float hi = invRayDir.X*(box.Maxs.X - rayStart.X);
    outMin = Math::Min(lo, hi);
    outMax = Math::Max(lo, hi);

    lo = invRayDir.Y*(box.Mins.Y - rayStart.Y);
    hi = invRayDir.Y*(box.Maxs.Y - rayStart.Y);
    outMin = Math::Max(outMin, Math::Min(lo, hi));
    outMax = Math::Min(outMax, Math::Max(lo, hi));

    lo = invRayDir.Z*(box.Mins.Z - rayStart.Z);
    hi = invRayDir.Z*(box.Maxs.Z - rayStart.Z);
    outMin = Math::Max(outMin, Math::Min(lo, hi));
    outMax = Math::Min(outMax, Math::Max(lo, hi));

    return (outMin <= outMax) && (outMax > 0.0f);
#else
    outMin = -Math::MaxValue<float>();
    outMax = Math::MaxValue<float>();

    for (int i = 0; i < 3; i++)
    {
        // Check is ray axial
        if (Math::IsInfinite(invRayDir[i]))
        {
            if (rayStart[i] < box.Mins[i] || rayStart[i] > box.Maxs[i])
            {
                // ray origin must be within the bounds
                return false;
            }
        }
        else
        {
            float lo = invRayDir[i] * (box.Mins[i] - rayStart[i]);
            float hi = invRayDir[i] * (box.Maxs[i] - rayStart[i]);
            if (lo > hi)
            {
                float tmp = lo;
                lo = hi;
                hi = tmp;
            }
            if (lo > outMin)
            {
                outMin = lo;
            }
            if (hi < outMax)
            {
                outMax = hi;
            }
            if (outMin > outMax // Ray doesn't intersect AABB
                || outMax <= 0.0f)
            { // or AABB is behind ray origin
                return false;
            }
        }
    }
    return true;
#endif
}

/// Ray - AABB2D
/// If raydir is normalized, min and max will be in rage [0,raylength]
/// If raydir is non-normalized, min and max will be in rage [0,1]
HK_INLINE bool BvRayIntersectBox2D(Float2 const& rayStart, Float2 const& invRayDir, Float2 const& mins, Float2 const& maxs, float& outMin, float& outMax)
{
    outMin = -Math::MaxValue<float>();
    outMax = Math::MaxValue<float>();

    for (int i = 0; i < 2; i++)
    {
        // Check is ray axial
        if (Math::IsInfinite(invRayDir[i]))
        {
            if (rayStart[i] < mins[i] || rayStart[i] > maxs[i])
            {
                // ray origin must be within the bounds
                return false;
            }
        }
        else
        {
            float lo = invRayDir[i] * (mins[i] - rayStart[i]);
            float hi = invRayDir[i] * (maxs[i] - rayStart[i]);
            if (lo > hi)
            {
                float tmp = lo;
                lo = hi;
                hi = tmp;
            }
            if (lo > outMin)
            {
                outMin = lo;
            }
            if (hi < outMax)
            {
                outMax = hi;
            }
            if (outMin > outMax // Ray doesn't intersect AABB
                || outMax <= 0.0f)
            { // or AABB is behind ray origin
                return false;
            }
        }
    }
    return true;
}

/// Ray - OBB
/// If raydir is normalized, min and max will be in rage [0,raylength]
/// If raydir is non-normalized, min and max will be in rage [0,1]
HK_INLINE bool BvRayIntersectOrientedBox(Float3 const& rayStart, Float3 const& rayDir, BvOrientedBox const& orientedBox, float& outMin, float& outMax)
{
    const Float3x3 orientInversed = orientedBox.Orient.Transposed();

    // Transform ray to OBB space
    const Float3 ro = orientInversed * (rayStart - orientedBox.Center);
    const Float3 rd = orientInversed * rayDir;

    // Mins and maxs in OBB space
    Float3 const mins = -orientedBox.HalfSize;
    Float3 const& maxs = orientedBox.HalfSize;

#if 0
    const float InvRayDirX = 1.0f / rd.X;
    const float InvRayDirY = 1.0f / rd.Y;
    const float InvRayDirZ = 1.0f / rd.Z;

    float Lo = InvRayDirX*(mins.X - ro.X);
    float Hi = InvRayDirX*(maxs.X - ro.X);
    outMin = Math::Min(Lo, Hi);
    outMax = Math::Max(Lo, Hi);

    Lo = InvRayDirY*(mins.Y - ro.Y);
    Hi = InvRayDirY*(maxs.Y - ro.Y);
    outMin = Math::Max(outMin, Math::Min(Lo, Hi));
    outMax = Math::Min(outMax, Math::Max(Lo, Hi));

    Lo = InvRayDirZ*(mins.Z - ro.Z);
    Hi = InvRayDirZ*(maxs.Z - ro.Z);
    outMin = Math::Max(outMin, Math::Min(Lo, Hi));
    outMax = Math::Min(outMax, Math::Max(Lo, Hi));

    return (outMin <= outMax) && (outMax > 0.0f);
#else
    outMin = -Math::MaxValue<float>();
    outMax = Math::MaxValue<float>();

    for (int i = 0; i < 3; i++)
    {
        // Check is ray axial
        if (fabsf(rd[i]) < 1e-6f)
        {
            if (ro[i] < mins[i] || ro[i] > maxs[i])
            {
                // ray origin must be within the bounds
                return false;
            }
        }
        else
        {
            float invRayDir = 1.0f / rd[i];
            float lo = invRayDir * (mins[i] - ro[i]);
            float hi = invRayDir * (maxs[i] - ro[i]);
            if (lo > hi)
            {
                float tmp = lo;
                lo = hi;
                hi = tmp;
            }
            if (lo > outMin)
            {
                outMin = lo;
            }
            if (hi < outMax)
            {
                outMax = hi;
            }
            if (outMin > outMax // Ray doesn't intersect AABB
                || outMax <= 0.0f)
            { // or AABB is behind ray origin
                return false;
            }
        }
    }
    return true;
#endif
}

/// Ray - triangle
HK_INLINE bool BvRayIntersectTriangle(Float3 const& rayStart, Float3 const& rayDir, Float3 const& p0, Float3 const& p1, Float3 const& p2, float& distance, float& u, float& v, bool cullBackFace = true)
{
    const Float3 e1 = p1 - p0;
    const Float3 e2 = p2 - p0;
    const Float3 h = Math::Cross(rayDir, e2);

    // calc determinant
    const float det = Math::Dot(e1, h);

    if (cullBackFace)
    {
        if (det < 0.00001)
        {
            return false;
        }
    }
    else
    {
        // ray lies in plane of triangle, so there is no intersection
        if (det > -0.00001 && det < 0.00001)
        {
            return false;
        }
    }

    // calc inverse determinant to minimalize math divisions in next calculations
    const float invDet = 1 / det;

    // calc vector from ray origin to P0
    const Float3 s = rayStart - p0;

    // calc U
    u = invDet * Math::Dot(s, h);
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }

    // calc perpendicular to compute V
    const Float3 q = Math::Cross(s, e1);

    // calc V
    v = invDet * Math::Dot(rayDir, q);
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    // compute distance to find out where the intersection point is on the line
    distance = invDet * Math::Dot(e2, q);

    // if distance < 0 this is a line intersection but not a ray intersection
    return distance > 0.0f;
}

/// Ray - Plane
HK_INLINE bool BvRayIntersectPlane(Float3 const& rayStart, Float3 const& rayDir, PlaneF const& plane, float& distance)
{
    // Calculate distance from ray origin to plane
    const float d1 = Math::Dot(rayStart, plane.Normal) + plane.D;

    // Ray origin is on plane
    if (d1 == 0.0f)
    {
        distance = 0.0f;
        return true;
    }

    // Check ray direction
    const float d2 = Math::Dot(plane.Normal, rayDir);
    if (Math::Abs(d2) < 0.0001f)
    {
        // ray is parallel
        return false;
    }

    // Calculate distance from ray origin to plane intersection
    distance = -(d1 / d2);

    return distance >= 0.0f;
}

/// Ray - Plane
HK_INLINE bool BvRayIntersectPlaneFront(Float3 const& rayStart, Float3 const& rayDir, PlaneF const& plane, float& distance)
{
    // Calculate distance from ray origin to plane
    const float d1 = Math::Dot(rayStart, plane.Normal) + plane.D;

    // Perform face culling
    if (d1 < 0.0f)
    {
        return false;
    }

    // Check ray direction
    const float d2 = Math::Dot(plane.Normal, rayDir);
    if (d2 >= 0.0f)
    {
        // ray is parallel or has wrong direction
        return false;
    }

    // Calculate distance from ray origin to plane intersection
    distance = d1 / -d2;

    return true;
}

/// Ray - Plane
HK_INLINE bool BvRayIntersectPlaneBack(Float3 const& rayStart, Float3 const& rayDir, PlaneF const& plane, float& distance)
{
    // Calculate distance from ray origin to plane
    const float d1 = Math::Dot(rayStart, plane.Normal) + plane.D;

    // Perform face culling
    if (d1 > 0.0f)
    {
        return false;
    }

    // Ray origin is on plane
    if (d1 == 0.0f)
    {
        distance = 0.0f;
        return true;
    }

    // Check ray direction
    const float d2 = Math::Dot(plane.Normal, rayDir);
    if (d2 <= 0.0f)
    {
        // ray is parallel or has wrong direction
        return false;
    }

    // Calculate distance from ray origin to plane intersection
    distance = -d1 / d2;

    return true;
}

/// Ray - Elipsoid
HK_INLINE bool BvRayIntersectElipsoid(Float3 const& rayStart, Float3 const& rayDir, float radius, float MParam, float NParam, float& outMin, float& outMax)
{
    const float a = rayDir.X * rayDir.X + MParam * rayDir.Y * rayDir.Y + NParam * rayDir.Z * rayDir.Z;
    const float b = 2.0f * (rayStart.X * rayDir.X + MParam * rayStart.Y * rayDir.Y + NParam * rayStart.Z * rayDir.Z);
    const float c = rayStart.X * rayStart.X + MParam * rayStart.Y * rayStart.Y + NParam * rayStart.Z * rayStart.Z - radius * radius;
    const float d = b * b - 4.0f * a * c;
    if (d < 0.0f)
    {
        return false;
    }
    const float distance = Math::Sqrt(d);
    const float denom = 0.5f / a;
    Math::MinMax((-b + distance) * denom, (-b - distance) * denom, outMin, outMax);
    return outMin > 0.0f || outMax > 0.0f;
}

/// Ray - Elipsoid
HK_INLINE bool BvRayIntersectElipsoid(Float3 const& rayStart, Float3 const& rayDir, float radius, float MParam, float NParam, float& distance)
{
    const float a = rayDir.X * rayDir.X + MParam * rayDir.Y * rayDir.Y + NParam * rayDir.Z * rayDir.Z;
    const float b = 2.0f * (rayStart.X * rayDir.X + MParam * rayStart.Y * rayDir.Y + NParam * rayStart.Z * rayDir.Z);
    const float c = rayStart.X * rayStart.X + MParam * rayStart.Y * rayStart.Y + NParam * rayStart.Z * rayStart.Z - radius * radius;
    const float d = b * b - 4.0f * a * c;
    if (d < 0.0f)
    {
        return false;
    }
    distance = Math::Sqrt(d);
    const float denom = 0.5f / a;
    float t1, t2;
    Math::MinMax((-b + distance) * denom, (-b - distance) * denom, t1, t2);
    distance = t1 >= 0.0f ? t1 : t2;
    return distance > 0.0f;
}


/*


Point tests


*/

HK_INLINE bool BvPointInPoly2D(Float2 const* points, int numPoints, float x, float y)
{
    int i, j, count = 0;

    for (i = 0, j = numPoints - 1; i < numPoints; j = i++)
    {
        if (((points[i].Y <= y && y < points[j].Y) || (points[j].Y <= y && y < points[i].Y)) && (x < (points[j].X - points[i].X) * (y - points[i].Y) / (points[j].Y - points[i].Y) + points[i].X))
        {
            count++;
        }
    }

    return count & 1;
}

HK_INLINE bool BvPointInPoly2D(Float2 const* points, int numPoints, Float2 const& p)
{
    return BvPointInPoly2D(points, numPoints, p.X, p.Y);
}

HK_INLINE bool BvPointInRect(Float2 const& mins, Float2 const& maxs, float x, float y)
{
    if (x < mins.X || y < mins.Y || x > maxs.X || y > maxs.Y)
        return false;
    return true;
}

HK_INLINE bool BvPointInRect(Float2 const& mins, Float2 const& maxs, Float2 const& p)
{
    if (p.X < mins.X || p.Y < mins.Y || p.X > maxs.X || p.Y > maxs.Y)
        return false;
    return true;
}

/// Check is point inside convex hull:
/// p - testing point (assumed point is on hull plane)
/// normal - hull normal
/// hullPoints - hull vertices (CCW order required)
/// numPoints - hull vertex count (more or equal 3)
HK_INLINE bool BvPointInConvexHullCCW(Float3 const& p, Float3 const& normal, Float3 const* hullPoints, int numPoints)
{
    HK_ASSERT(numPoints >= 3);

    Float3 const* hullP = &hullPoints[numPoints - 1];

    for (int i = 0; i < numPoints; hullP = &hullPoints[i], i++)
    {
        const Float3 edge = *hullP - hullPoints[i];
        const Float3 edgeNormal = Math::Cross(normal, edge);
        float d = -Math::Dot(edgeNormal, hullPoints[i]);
        float dist = Math::Dot(edgeNormal, p) + d;
        if (dist > 0.0f)
        {
            return false;
        }
    }

    return true;
}

/// Check is point inside convex hull:
/// p - testing point (assumed point is on hull plane)
/// normal - hull normal
/// hullPoints - hull vertices (CW order required)
/// numPoints - hull vertex count (more or equal 3)
HK_INLINE bool BvPointInConvexHullCW(Float3 const& p, Float3 const& normal, Float3 const* hullPoints, int numPoints)
{
    HK_ASSERT(numPoints >= 3);

    Float3 const* hullP = &hullPoints[numPoints - 1];

    for (int i = 0; i < numPoints; hullP = &hullPoints[i], i++)
    {
        const Float3 edge = hullPoints[i] - *hullP;
        const Float3 edgeNormal = Math::Cross(normal, edge);
        float d = -Math::Dot(edgeNormal, hullPoints[i]);
        float dist = Math::Dot(edgeNormal, p) + d;
        if (dist > 0.0f)
        {
            return false;
        }
    }

    return true;
}

/// Square of shortest distance between Point and Segment
HK_INLINE float BvShortestDistanceSqr(Float3 const& p, Float3 const& start, Float3 const& end)
{
    const Float3 dir = end - start;
    const Float3 v = p - start;

    const float dp1 = Math::Dot(v, dir);
    if (dp1 <= 0.0f)
    {
        return p.DistSqr(start);
    }

    const float dp2 = Math::Dot(dir, dir);
    if (dp2 <= dp1)
    {
        return p.DistSqr(end);
    }

    return v.DistSqr((dp1 / dp2) * dir);
}

/// Square of distance between Point and Segment
HK_INLINE bool BvDistanceSqr(Float3 const& p, Float3 const& start, Float3 const& end, float& distance)
{
    const Float3 dir = end - start;
    const Float3 v = p - start;

    const float dp1 = Math::Dot(v, dir);
    if (dp1 <= 0.0f)
    {
        return false;
    }

    const float dp2 = Math::Dot(dir, dir);
    if (dp2 <= dp1)
    {
        return false;
    }

    distance = v.DistSqr((dp1 / dp2) * dir);

    return true;
}

/// Check Point on Segment
HK_INLINE bool BvIsPointOnSegment(Float3 const& p, Float3 const& start, Float3 const& end, float epsilon)
{
    const Float3 dir = end - start;
    const Float3 v = p - start;

    const float dp1 = Math::Dot(v, dir);
    if (dp1 <= 0.0f)
    {
        return false;
    }

    const float dp2 = Math::Dot(dir, dir);
    if (dp2 <= dp1)
    {
        return false;
    }

    return v.DistSqr((dp1 / dp2) * dir) < epsilon;
}

/// Project Point on Segment
HK_INLINE Float3 BvProjectPointOnLine(Float3 const& p, Float3 const& start, Float3 const& end)
{
    const Float3 dir = end - start;
    const Float3 v = p - start;

    const float dp1 = Math::Dot(v, dir);
    const float dp2 = Math::Dot(dir, dir);

    return start + (dp1 / dp2) * dir;
}

/// Square of distance between Point and Segment (2D)
HK_INLINE float BvShortestDistanceSqr(Float2 const& p, Float2 const& start, Float2 const& end)
{
    const Float2 dir = end - start;
    const Float2 v = p - start;

    const float dp1 = Math::Dot(v, dir);
    if (dp1 <= 0.0f)
    {
        return p.DistSqr(start);
    }

    const float dp2 = Math::Dot(dir, dir);
    if (dp2 <= dp1)
    {
        return p.DistSqr(end);
    }

    return v.DistSqr((dp1 / dp2) * dir);
}

/// Square of distance between Point and Segment (2D)
HK_INLINE bool BvDistanceSqr(Float2 const& p, Float2 const& start, Float2 const& end, float& distance)
{
    const Float2 dir = end - start;
    const Float2 v = p - start;

    const float dp1 = Math::Dot(v, dir);
    if (dp1 <= 0.0f)
    {
        return false;
    }

    const float dp2 = Math::Dot(dir, dir);
    if (dp2 <= dp1)
    {
        return false;
    }

    distance = v.DistSqr((dp1 / dp2) * dir);

    return true;
}

/// Check Point on Segment (2D)
HK_INLINE bool BvIsPointOnSegment(Float2 const& p, Float2 const& start, Float2 const& end, float epsilon)
{
    const Float2 dir = end - start;
    const Float2 v = p - start;

    const float dp1 = Math::Dot(v, dir);
    if (dp1 <= 0.0f)
    {
        return false;
    }

    const float dp2 = Math::Dot(dir, dir);
    if (dp2 <= dp1)
    {
        return false;
    }

    return v.DistSqr((dp1 / dp2) * dir) < epsilon;
}


#if 0
/// Segment - Sphere
HK_INLINE bool BvSegmentIntersectSphere(SegmentF const& segment, BvSphere const& sphere)
{
    const Float3 s = segment.Start - sphere.Center;
    const Float3 e = segment.End - sphere.Center;
    Float3 r = e - s;
    const float a = Math::Dot(r, -s);

    if (a <= 0.0f)
    {
        return s.LengthSqr() < sphere.Radius * sphere.Radius;
    }

    const float dot_r_r = r.LengthSqr();

    if (a >= dot_r_r)
    {
        return e.LengthSqr() < sphere.Radius * sphere.Radius;
    }

    r = s + (a / dot_r_r) * r;
    return r.LengthSqr() < sphere.Radius * sphere.Radius;
}

/// Ray - OBB
//HK_INLINE bool BvSegmentIntersectOrientedBox(RaySegment const& ray, BvOrientedBox const& orientedBox)
//{
//    // Трансформируем отрезок в систему координат OBB
//
//    //смещение в мировой системе координат
//    Float3 RayStart = ray.Start - orientedBox.Center;
//    //смещение в системе координат box1
//    RayStart = orientedBox.Orient * RayStart + orientedBox.Center;
//
//    //смещение в мировой системе координат
//    Float3 rayEnd = ray.end - orientedBox.Center;
//    //смещение в системе координат box1
//    rayEnd = orientedBox.Orient * rayEnd + orientedBox.Center;
//
//    const Float3 rayVec = rayEnd - RayStart;
//
//    const float ZeroTolerance = 0.000001f; // TODO: вынести куда-нибудь
//
//    float length = Length(rayVec);
//    if (length < ZeroTolerance)
//    {
//        return false;
//    }
//
//    const Float3 dir = rayVec / length;
//
//    length *= 0.5f; // half length
//
//    // T = Позиция OBB - midRayPoint
//    const Float3 T = orientedBox.Center - (RayStart + rayVec * 0.5f);
//
//    // проверяем, является ли одна из осей X,Y,Z разделяющей
//    if ((fabs(T.X) > orientedBox.HalfSize.X + length * fabs(dir.X)) ||
//        (fabs(T.Y) > orientedBox.HalfSize.Y + length * fabs(dir.Y)) ||
//        (fabs(T.Z) > orientedBox.HalfSize.Z + length * fabs(dir.Z)))
//        return false;
//
//    float r;
//
//    // проверяем X ^ dir
//    r = orientedBox.HalfSize.Y * fabs(dir.Z) + orientedBox.HalfSize.Z * fabs(dir.Y);
//    if (fabs(T.Y * dir.Z - T.Z * dir.Y) > r)
//        return false;
//
//    // проверяем Y ^ dir
//    r = orientedBox.HalfSize.X * fabs(dir.Z) + orientedBox.HalfSize.Z * fabs(dir.X);
//    if (fabs(T.Z * dir.X - T.X * dir.Z) > r)
//        return false;
//
//    // проверяем Z ^ dir
//    r = orientedBox.HalfSize.X * fabs(dir.Y) + orientedBox.HalfSize.Y * fabs(dir.X);
//    if (fabs(T.X * dir.Y - T.Y * dir.X) > r)
//        return false;
//
//    return true;
//}
/// Segment - Plane
// distance - расстояние от начала луча до плоскости в направлении луча
HK_INLINE bool BvSegmentIntersectPlane(SegmentF const& segment, PlaneF const& plane, float& distance)
{
    const Float3 dir = segment.End - segment.Start;
    const float length = dir.Length();
    if (length.CompareEps(0.0f, 0.00001f))
    {
        return false; // null-length segment
    }
    const float d2 = Math::Dot(plane.Normal, dir / length);
    if (d2.CompareEps(0.0f, 0.00001f))
    {
        // Луч параллелен плоскости
        return false;
    }
    const float d1 = Math::Dot(plane.Normal, segment.Start) + plane.D;
    distance = -(d1 / d2);
    return distance >= 0.0f && distance <= length;
}
#endif

HK_NAMESPACE_END
