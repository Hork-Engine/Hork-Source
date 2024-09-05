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

#include <Engine/Geometry/ConvexHull.h>
#include <Engine/Core/Logger.h>
#include <Engine/Core/Platform.h>

HK_NAMESPACE_BEGIN

//#define CONVEX_HULL_CW

ConvexHull::ConvexHull(PlaneF const& plane, float maxExtents)
{
    FromPlane(plane, maxExtents);
}

void ConvexHull::FromPlane(PlaneF const& plane, float maxExtents)
{
    Float3 rightVec;
    Float3 upVec;

    plane.Normal.ComputeBasis(rightVec, upVec);

    Float3 p = plane.Normal * plane.GetDist(); // point on plane

    m_Points.Resize(4);

#ifdef CONVEX_HULL_CW
    // CW
    m_Points[0] = (upVec - rightVec) * maxExtents;
    m_Points[1] = (upVec + rightVec) * maxExtents;
#else
    // CCW
    m_Points[0] = (upVec - rightVec) * maxExtents;
    m_Points[1] = (-upVec - rightVec) * maxExtents;
#endif

    m_Points[2] = -m_Points[0];
    m_Points[3] = -m_Points[1];
    m_Points[0] += p;
    m_Points[1] += p;
    m_Points[2] += p;
    m_Points[3] += p;
}

void ConvexHull::FromPoints(Float3 const* points, size_t numPoints)
{
    m_Points.Resize(numPoints);
    Core::Memcpy(m_Points.ToPtr(), points, numPoints * sizeof(Float3));
}

ConvexHull ConvexHull::Reversed() const
{
    ConvexHull hull = *this;
    hull.Reverse();
    return hull;
}

void ConvexHull::Reverse()
{
    m_Points.Reverse();
}

PlaneSide ConvexHull::Classify(PlaneF const& plane, float epsilon) const
{
    int front = 0;
    int back = 0;
    int onplane = 0;

    for (Float3 const& point : m_Points)
    {
        const float d = plane.DistanceToPoint(point);
        if (d > epsilon)
        {
            if (back > 0 || onplane > 0)
            {
                return PlaneSide::Cross;
            }
            front++;
        }
        else if (d < -epsilon)
        {
            if (front > 0 || onplane > 0)
            {
                return PlaneSide::Cross;
            }
            back++;
        }
        else
        {
            if (back > 0 || front > 0)
            {
                return PlaneSide::Cross;
            }
            onplane++;
        }
    }

    if (onplane)
    {
        return PlaneSide::On;
    }

    if (front)
    {
        return PlaneSide::Front;
    }

    if (back)
    {
        return PlaneSide::Back;
    }

    return PlaneSide::Cross;
}

bool ConvexHull::IsTiny(float minEdgeLength) const
{
    const float minEdgeLengthSqr = minEdgeLength * minEdgeLength;
    int numEdges = 0;
    for (size_t i = 0, count = m_Points.Size(); i < count; ++i)
    {
        Float3 const& p1 = m_Points[i];
        Float3 const& p2 = m_Points[(i + 1) % count];
        if (p1.DistSqr(p2) >= minEdgeLengthSqr)
        {
            numEdges++;
            if (numEdges == 3)
                return false;
        }
    }
    return true;
}

bool ConvexHull::IsHuge() const
{
    for (Float3 const& p : m_Points)
        if (p.X <= CONVEX_HULL_MIN_BOUNDS || p.X >= CONVEX_HULL_MAX_BOUNDS || p.Y <= CONVEX_HULL_MIN_BOUNDS || p.Y >= CONVEX_HULL_MAX_BOUNDS || p.Z <= CONVEX_HULL_MIN_BOUNDS || p.Z >= CONVEX_HULL_MAX_BOUNDS)
            return true;
    return false;
}

float ConvexHull::CalcArea() const
{
    float area = 0;
    for (size_t i = 2, count = m_Points.Size(); i < count; ++i)
        area += Math::Cross(m_Points[i - 1] - m_Points[0], m_Points[i] - m_Points[0]).Length();
    return area * 0.5f;
}

BvAxisAlignedBox ConvexHull::CalcBounds() const
{
    if (m_Points.IsEmpty())
        return BvAxisAlignedBox::sEmpty();

    BvAxisAlignedBox bounds(m_Points[0], m_Points[0]);
    for (size_t i = 1, count = m_Points.Size(); i < count; ++i)
        bounds.AddPoint(m_Points[i]);
    return bounds;
}

Float3 ConvexHull::CalcNormal() const
{
    if (m_Points.Size() < 3)
    {
        LOG("ConvexHull::CalcNormal: num points < 3\n");
        return Float3(0);
    }

    Float3 center = CalcCenter();

#ifdef CONVEX_HULL_CW
    // CW
    return Math::Cross(m_Points[1] - center, m_Points[0] - center).NormalizeFix();
#else
    // CCW
    return Math::Cross(m_Points[0] - center, m_Points[1] - center).NormalizeFix();
#endif
}

PlaneF ConvexHull::CalcPlane() const
{
    PlaneF plane;

    if (m_Points.Size() < 3)
    {
        LOG("ConvexHull::CalcPlane: num points < 3\n");
        plane.Clear();
        return plane;
    }

    Float3 center = CalcCenter();

#ifdef CONVEX_HULL_CW
    // CW
    plane.Normal = Math::Cross(m_Points[1] - center, m_Points[0] - center).NormalizeFix();
#else
    // CCW
    plane.Normal = Math::Cross(m_Points[0] - center, m_Points[1] - center).NormalizeFix();
#endif
    plane.D = -Math::Dot(m_Points[0], plane.Normal);
    return plane;
}

Float3 ConvexHull::CalcCenter() const
{
    if (m_Points.IsEmpty())
    {
        LOG("ConvexHull::CalcCenter: no points in hull\n");
        return Float3(0);
    }
    Float3 center = m_Points[0];
    for (size_t i = 1, count = m_Points.Size(); i < count; ++i)
        center += m_Points[i];
    return center * (1.0f / m_Points.Size());
}

PlaneSide ConvexHull::Split(PlaneF const& plane, float epsilon, ConvexHull& frontHull, ConvexHull& backHull) const
{
    size_t i;
    size_t count = m_Points.Size();

    int front = 0;
    int back = 0;

    constexpr size_t MaxHullVerts = 128;

    SmallVector<float, MaxHullVerts> distances(count + 1);
    SmallVector<PlaneSide, MaxHullVerts> sides(count + 1);

    frontHull.Clear();
    backHull.Clear();

    // Classify each point of the hull
    for (i = 0; i < count; ++i)
    {
        float dist = Math::Dot(m_Points[i], plane);

        distances[i] = dist;

        if (dist > epsilon)
        {
            sides[i] = PlaneSide::Front;
            front++;
        }
        else if (dist < -epsilon)
        {
            sides[i] = PlaneSide::Back;
            back++;
        }
        else
        {
            sides[i] = PlaneSide::On;
        }
    }

    sides[i] = sides[0];
    distances[i] = distances[0];

    // If all points on the plane
    if (!front && !back)
    {
        Float3 hullNormal = CalcNormal();

        if (Math::Dot(hullNormal, plane.Normal) > 0)
        {
            frontHull = *this;
            return PlaneSide::Front;
        }
        else
        {
            backHull = *this;
            return PlaneSide::Back;
        }
    }

    if (!front)
    {
        // All poins at the back side of the plane
        backHull = *this;
        return PlaneSide::Back;
    }

    if (!back)
    {
        // All poins at the front side of the plane
        frontHull = *this;
        return PlaneSide::Front;
    }

    frontHull.m_Points.Reserve(count + 4);
    backHull.m_Points.Reserve(count + 4);

    for (i = 0; i < count; ++i)
    {
        Float3 const& p = m_Points[i];

        switch (sides[i])
        {
            case PlaneSide::On:
                frontHull.m_Points.Add(p);
                backHull.m_Points.Add(p);
                continue;
            case PlaneSide::Front:
                frontHull.m_Points.Add(p);
                break;
            case PlaneSide::Back:
                backHull.m_Points.Add(p);
                break;
        }

        auto nextSide = sides[i + 1];

        if (nextSide == PlaneSide::On || nextSide == sides[i])
        {
            continue;
        }

        Float3 newVertex = m_Points[(i + 1) % count];

        if (sides[i] == PlaneSide::Front)
        {
            float dist = distances[i] / (distances[i] - distances[i + 1]);
            for (int j = 0; j < 3; j++)
            {
                if (plane.Normal[j] == 1)
                    newVertex[j] = -plane.D;
                else if (plane.Normal[j] == -1)
                    newVertex[j] = plane.D;
                else
                    newVertex[j] = p[j] + dist * (newVertex[j] - p[j]);
            }
        }
        else
        {
            float dist = distances[i + 1] / (distances[i + 1] - distances[i]);
            for (int j = 0; j < 3; j++)
            {
                if (plane.Normal[j] == 1)
                    newVertex[j] = -plane.D;
                else if (plane.Normal[j] == -1)
                    newVertex[j] = plane.D;
                else
                    newVertex[j] = newVertex[j] + dist * (p[j] - newVertex[j]);
            }
        }

        frontHull.m_Points.Add(newVertex);
        backHull.m_Points.Add(newVertex);
    }

    return PlaneSide::Cross;
}

PlaneSide ConvexHull::Clip(PlaneF const& plane, float epsilon, ConvexHull& frontHull) const
{
    size_t i;
    size_t count = m_Points.Size();

    int front = 0;
    int back = 0;

    constexpr size_t MaxHullVerts = 128;

    SmallVector<float, MaxHullVerts> distances(count + 1);
    SmallVector<PlaneSide, MaxHullVerts> sides(count + 1);

    frontHull.Clear();

    // Classify each point of the hull
    for (i = 0; i < count; ++i)
    {
        float dist = Math::Dot(m_Points[i], plane);

        distances[i] = dist;

        if (dist > epsilon)
        {
            sides[i] = PlaneSide::Front;
            front++;
        }
        else if (dist < -epsilon)
        {
            sides[i] = PlaneSide::Back;
            back++;
        }
        else
        {
            sides[i] = PlaneSide::On;
        }
    }

    sides[i] = sides[0];
    distances[i] = distances[0];

    if (!front)
    {
        // All poins at the back side of the plane
        return PlaneSide::Back;
    }

    if (!back)
    {
        // All poins at the front side of the plane
        frontHull = *this;
        return PlaneSide::Front;
    }

    frontHull.m_Points.Reserve(count + 4);

    for (i = 0; i < count; ++i)
    {
        Float3 const& p = m_Points[i];

        switch (sides[i])
        {
            case PlaneSide::On:
                frontHull.m_Points.Add(p);
                continue;
            case PlaneSide::Front:
                frontHull.m_Points.Add(p);
                break;
        }

        auto nextSide = sides[i + 1];

        if (nextSide == PlaneSide::On || nextSide == sides[i])
        {
            continue;
        }

        Float3 newVertex = m_Points[(i + 1) % count];

        float dist = distances[i] / (distances[i] - distances[i + 1]);
        for (int j = 0; j < 3; j++)
        {
            if (plane.Normal[j] == 1)
                newVertex[j] = -plane.D;
            else if (plane.Normal[j] == -1)
                newVertex[j] = plane.D;
            else
                newVertex[j] = p[j] + dist * (newVertex[j] - p[j]);
        }

        frontHull.m_Points.Add(newVertex);
    }

    return PlaneSide::Cross;
}

HK_NAMESPACE_END
