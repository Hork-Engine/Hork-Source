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

#include <Geometry/ConvexHull.h>
#include <Platform/Logger.h>
#include <Platform/Platform.h>

//#define CONVEX_HULL_CW

AConvexHull* AConvexHull::CreateEmpty(int maxPoints)
{
    HK_ASSERT(maxPoints > 0);
    int          size = sizeof(AConvexHull) - sizeof(Points) + maxPoints * sizeof(Points[0]);
    AConvexHull* hull = (AConvexHull*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(size);
    hull->MaxPoints   = maxPoints;
    hull->NumPoints   = 0;
    return hull;
}

AConvexHull* AConvexHull::CreateForPlane(PlaneF const& plane, float maxExtents)
{
    Float3 rightVec;
    Float3 upVec;

    plane.Normal.ComputeBasis(rightVec, upVec);

    Float3 p = plane.Normal * plane.GetDist(); // point on plane

    AConvexHull* hull;
    hull            = CreateEmpty(4);
    hull->NumPoints = 4;

#ifdef CONVEX_HULL_CW
    // CW
    hull->Points[0] = (upVec - rightVec) * maxExtents;
    hull->Points[1] = (upVec + rightVec) * maxExtents;
#else
    // CCW
    hull->Points[0] = (upVec - rightVec) * maxExtents;
    hull->Points[1] = (-upVec - rightVec) * maxExtents;
#endif

    hull->Points[2] = -hull->Points[0];
    hull->Points[3] = -hull->Points[1];
    hull->Points[0] += p;
    hull->Points[1] += p;
    hull->Points[2] += p;
    hull->Points[3] += p;

    return hull;
}

AConvexHull* AConvexHull::CreateFromPoints(Float3 const* points, int numPoints)
{
    AConvexHull* hull;
    hull            = CreateEmpty(numPoints);
    hull->NumPoints = numPoints;
    Platform::Memcpy(hull->Points, points, sizeof(Float3) * numPoints);
    return hull;
}

#if 0
AConvexHull * AConvexHull::RecreateFromPoints( AConvexHull * oldHull, Float3 const * points, int numPoints ) {
    if ( oldHull ) {

        // hull capacity is big enough
        if ( oldHull->MaxPoints >= numPoints ) {
            Platform::Memcpy( oldHull->Points, points, numPoints * sizeof( Float3 ) );
            oldHull->NumPoints = numPoints;
            return oldHull;
        }

        // resize hull
        int oldSize = sizeof( AConvexHull ) - sizeof( Points ) + oldHull->MaxPoints * sizeof( Points[0] );
        int newSize = oldSize + ( numPoints - oldHull->MaxPoints ) * sizeof( Points[0] );
        AConvexHull * hull = ( AConvexHull * )Platform::MemoryReallocSafe( oldHull, newSize, false );
        hull->MaxPoints = numPoints;
        hull->NumPoints = numPoints;
        Platform::Memcpy( hull->Points, points, numPoints * sizeof( Float3 ) );
        return hull;
    }

    return CreateFromPoints( points, numPoints );
}
#endif

void AConvexHull::Destroy()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(this);
}

AConvexHull* AConvexHull::Duplicate() const
{
    AConvexHull* hull;
    hull            = CreateEmpty(MaxPoints);
    hull->NumPoints = NumPoints;
    Platform::Memcpy(hull->Points, Points, sizeof(Float3) * NumPoints);
    return hull;
}

AConvexHull* AConvexHull::Reversed() const
{
    AConvexHull* hull;
    hull                        = CreateEmpty(MaxPoints);
    hull->NumPoints             = NumPoints;
    const int numPointsMinusOne = NumPoints - 1;
    for (int i = 0; i < NumPoints; i++)
    {
        hull->Points[i] = Points[numPointsMinusOne - i];
    }
    return hull;
}

PLANE_SIDE AConvexHull::Classify(PlaneF const& plane, float epsilon) const
{
    int front   = 0;
    int back    = 0;
    int onplane = 0;

    for (int i = 0; i < NumPoints; i++)
    {
        const Float3& point = Points[i];
        const float   d     = plane.DistanceToPoint(point);
        if (d > epsilon)
        {
            if (back > 0 || onplane > 0)
            {
                return PLANE_SIDE_CROSS;
            }
            front++;
        }
        else if (d < -epsilon)
        {
            if (front > 0 || onplane > 0)
            {
                return PLANE_SIDE_CROSS;
            }
            back++;
        }
        else
        {
            if (back > 0 || front > 0)
            {
                return PLANE_SIDE_CROSS;
            }
            onplane++;
        }
    }

    if (onplane)
    {
        return PLANE_SIDE_ON;
    }

    if (front)
    {
        return PLANE_SIDE_FRONT;
    }

    if (back)
    {
        return PLANE_SIDE_BACK;
    }

    return PLANE_SIDE_CROSS;
}

bool AConvexHull::IsTiny(float minEdgeLength) const
{
    int numEdges = 0;
    for (int i = 0; i < NumPoints; i++)
    {
        const Float3& p1 = Points[i];
        const Float3& p2 = Points[(i + 1) % NumPoints];
        if (p1.Dist(p2) >= minEdgeLength)
        {
            numEdges++;
            if (numEdges == 3)
            {
                return false;
            }
        }
    }
    return true;
}

bool AConvexHull::IsHuge() const
{
    for (int i = 0; i < NumPoints; i++)
    {
        const Float3& p = Points[i];
        if (p.X <= CONVEX_HULL_MIN_BOUNDS || p.X >= CONVEX_HULL_MAX_BOUNDS || p.Y <= CONVEX_HULL_MIN_BOUNDS || p.Y >= CONVEX_HULL_MAX_BOUNDS || p.Z <= CONVEX_HULL_MIN_BOUNDS || p.Z >= CONVEX_HULL_MAX_BOUNDS)
        {
            return true;
        }
    }
    return false;
}

float AConvexHull::CalcArea() const
{
    float Area = 0;
    for (int i = 2; i < NumPoints; i++)
    {
        Area += Math::Cross(Points[i - 1] - Points[0], Points[i] - Points[0]).Length();
    }
    return Area * 0.5f;
}

BvAxisAlignedBox AConvexHull::CalcBounds() const
{
    BvAxisAlignedBox bounds;

    if (NumPoints)
    {
        bounds.Mins = bounds.Maxs = Points[0];
    }
    else
    {
        bounds.Clear();
    }

    for (int i = 1; i < NumPoints; i++)
    {
        bounds.AddPoint(Points[i]);
    }
    return bounds;
}

Float3 AConvexHull::CalcNormal() const
{
    if (NumPoints < 3)
    {
        LOG("AConvexHull::CalcNormal: num points < 3\n");
        return Float3(0);
    }

    Float3 center = CalcCenter();

#ifdef CONVEX_HULL_CW
    // CW
    return Math::Cross(Points[1] - center, Points[0] - center).NormalizeFix();
#else
    // CCW
    return Math::Cross(Points[0] - center, Points[1] - center).NormalizeFix();
#endif
}

PlaneF AConvexHull::CalcPlane() const
{
    PlaneF plane;

    if (NumPoints < 3)
    {
        LOG("AConvexHull::CalcPlane: num points < 3\n");
        plane.Clear();
        return plane;
    }

    Float3 Center = CalcCenter();

#ifdef CONVEX_HULL_CW
    // CW
    plane.Normal = Math::Cross(Points[1] - Center, Points[0] - Center).NormalizeFix();
#else
    // CCW
    plane.Normal = Math::Cross(Points[0] - Center, Points[1] - Center).NormalizeFix();
#endif
    plane.D = -Math::Dot(Points[0], plane.Normal);
    return plane;
}

Float3 AConvexHull::CalcCenter() const
{
    Float3 center(0);
    if (!NumPoints)
    {
        LOG("AConvexHull::CalcCenter: no points in hull\n");
        return center;
    }
    for (int i = 0; i < NumPoints; i++)
    {
        center += Points[i];
    }
    return center * (1.0f / NumPoints);
}

void AConvexHull::Reverse()
{
    const int n                 = NumPoints >> 1;
    const int numPointsMinusOne = NumPoints - 1;
    Float3    tmp;
    for (int i = 0; i < n; i++)
    {
        tmp                           = Points[i];
        Points[i]                     = Points[numPointsMinusOne - i];
        Points[numPointsMinusOne - i] = tmp;
    }
}

PLANE_SIDE AConvexHull::Split(PlaneF const& plane, float epsilon, AConvexHull** ppFront, AConvexHull** ppBack) const
{
    int i;

    int front = 0;
    int back  = 0;

    *ppFront = *ppBack = nullptr;

    float*      distances = (float*)StackAlloc((NumPoints + 1) * sizeof(float));
    PLANE_SIDE* sides     = (PLANE_SIDE*)StackAlloc((NumPoints + 1) * sizeof(PLANE_SIDE));

    if (!distances || !sides)
    {
        CriticalError("AConvexHull:Split: stack overflow\n");
    }

    // Определить с какой стороны находится каждая точка исходного полигона
    for (i = 0; i < NumPoints; i++)
    {
        float dist = Math::Dot(Points[i], plane.Normal) + plane.D;

        distances[i] = dist;

        if (dist > epsilon)
        {
            sides[i] = PLANE_SIDE_FRONT;
            front++;
        }
        else if (dist < -epsilon)
        {
            sides[i] = PLANE_SIDE_BACK;
            back++;
        }
        else
        {
            sides[i] = PLANE_SIDE_ON;
        }
    }

    sides[i]     = sides[0];
    distances[i] = distances[0];

    // все точки полигона лежат на плоскости
    if (!front && !back)
    {
        Float3 hullNormal = CalcNormal();

        // По направлению нормали определить, куда можно отнести новый плейн

        if (Math::Dot(hullNormal, plane.Normal) > 0)
        {
            // Все точки находятся по фронтальную сторону плоскости
            *ppFront = Duplicate();
            return PLANE_SIDE_FRONT;
        }
        else
        {
            // Все точки находятся по заднюю сторону плоскости
            *ppBack = Duplicate();
            return PLANE_SIDE_BACK;
        }
    }

    if (!front)
    {
        // Все точки находятся по заднюю сторону плоскости
        *ppBack = Duplicate();
        return PLANE_SIDE_BACK;
    }

    if (!back)
    {
        // Все точки находятся по фронтальную сторону плоскости
        *ppFront = Duplicate();
        return PLANE_SIDE_FRONT;
    }

    *ppFront = CreateEmpty(NumPoints + 4);
    *ppBack  = CreateEmpty(NumPoints + 4);

    AConvexHull* f = *ppFront;
    AConvexHull* b = *ppBack;

    for (i = 0; i < NumPoints; i++)
    {
        Float3 const& v = Points[i];

        if (sides[i] == PLANE_SIDE_ON)
        {
            f->Points[f->NumPoints++] = v;
            b->Points[b->NumPoints++] = v;
            continue;
        }

        if (sides[i] == PLANE_SIDE_FRONT)
        {
            f->Points[f->NumPoints++] = v;
        }

        if (sides[i] == PLANE_SIDE_BACK)
        {
            b->Points[b->NumPoints++] = v;
        }

        PLANE_SIDE nextSide = sides[i + 1];

        if (nextSide == PLANE_SIDE_ON || nextSide == sides[i])
        {
            continue;
        }

        Float3 newVertex = Points[(i + 1) % NumPoints];

        if (sides[i] == PLANE_SIDE_FRONT)
        {
            float dist = distances[i] / (distances[i] - distances[i + 1]);
            for (int j = 0; j < 3; j++)
            {
                if (plane.Normal[j] == 1)
                {
                    newVertex[j] = -plane.D;
                }
                else if (plane.Normal[j] == -1)
                {
                    newVertex[j] = plane.D;
                }
                else
                {
                    newVertex[j] = v[j] + dist * (newVertex[j] - v[j]);
                }
            }
            //newVertex.s = v.s + dist * ( newVertex.s - v.s );
            //newVertex.t = v.t + dist * ( newVertex.t - v.t );
        }
        else
        {
            float dist = distances[i + 1] / (distances[i + 1] - distances[i]);
            for (int j = 0; j < 3; j++)
            {
                if (plane.Normal[j] == 1)
                {
                    newVertex[j] = -plane.D;
                }
                else if (plane.Normal[j] == -1)
                {
                    newVertex[j] = plane.D;
                }
                else
                {
                    newVertex[j] = newVertex[j] + dist * (v[j] - newVertex[j]);
                }
            }
            //newVertex.s = newVertex.s + dist * ( v.s - newVertex.s );
            //newVertex.t = newVertex.t + dist * ( v.t - newVertex.t );
        }

        HK_ASSERT(f->NumPoints < f->MaxPoints);
        f->Points[f->NumPoints++] = newVertex;
        b->Points[b->NumPoints++] = newVertex;
    }

    return PLANE_SIDE_CROSS;
}

PLANE_SIDE AConvexHull::Clip(PlaneF const& plane, float epsilon, AConvexHull** ppFront) const
{
    int i;
    int front = 0;
    int back  = 0;

    *ppFront = nullptr;

    float*      distances = (float*)StackAlloc((NumPoints + 1) * sizeof(float));
    PLANE_SIDE* sides     = (PLANE_SIDE*)StackAlloc((NumPoints + 1) * sizeof(PLANE_SIDE));

    if (!distances || !sides)
    {
        CriticalError("AConvexHull:Clip: stack overflow\n");
    }

    // Определить с какой стороны находится каждая точка исходного полигона
    for (i = 0; i < NumPoints; i++)
    {
        float dist = Math::Dot(Points[i], plane.Normal) + plane.D;

        distances[i] = dist;

        if (dist > epsilon)
        {
            sides[i] = PLANE_SIDE_FRONT;
            front++;
        }
        else if (dist < -epsilon)
        {
            sides[i] = PLANE_SIDE_BACK;
            back++;
        }
        else
        {
            sides[i] = PLANE_SIDE_ON;
        }
    }

    sides[i]     = sides[0];
    distances[i] = distances[0];

    if (!front)
    {
        // Все точки находятся по заднюю сторону плоскости
        return PLANE_SIDE_BACK;
    }

    if (!back)
    {
        // Все точки находятся по фронтальную сторону плоскости
        *ppFront = Duplicate();
        return PLANE_SIDE_FRONT;
    }

    *ppFront = CreateEmpty(NumPoints + 4);

    AConvexHull* f = *ppFront;

    for (i = 0; i < NumPoints; i++)
    {
        Float3 const& v = Points[i];

        if (sides[i] == PLANE_SIDE_ON)
        {
            f->Points[f->NumPoints++] = v;
            continue;
        }

        if (sides[i] == PLANE_SIDE_FRONT)
        {
            f->Points[f->NumPoints++] = v;
        }

        PLANE_SIDE nextSide = sides[i + 1];

        if (nextSide == PLANE_SIDE_ON || nextSide == sides[i])
        {
            continue;
        }

        Float3 newVertex = Points[(i + 1) % NumPoints];

        float dist = distances[i] / (distances[i] - distances[i + 1]);
        for (int j = 0; j < 3; j++)
        {
            if (plane.Normal[j] == 1)
            {
                newVertex[j] = -plane.D;
            }
            else if (plane.Normal[j] == -1)
            {
                newVertex[j] = plane.D;
            }
            else
            {
                newVertex[j] = v[j] + dist * (newVertex[j] - v[j]);
            }
        }

        f->Points[f->NumPoints++] = newVertex;
    }

    return PLANE_SIDE_CROSS;
}
