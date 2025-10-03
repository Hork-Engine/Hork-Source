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

#include "BV/BvAxisAlignedBox.h"

#include <Hork/Math/Plane.h>
#include <Hork/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

constexpr float CONVEX_HULL_MAX_BOUNDS = 5 * 1024;
constexpr float CONVEX_HULL_MIN_BOUNDS = -5 * 1024;

enum class PlaneSide : int8_t
{
    Back  = -1,
    Front = 1,
    On    = 0,
    Cross = 2
};

class ConvexHull
{
public:
                                ConvexHull() = default;

                                ConvexHull(ConvexHull const&) = default;
                                ConvexHull& operator=(ConvexHull const&) = default;

                                ConvexHull(ConvexHull&&) = default;
                                ConvexHull& operator=(ConvexHull&&) = default;

                                ConvexHull(PlaneF const& plane, float maxExtents = CONVEX_HULL_MAX_BOUNDS);

    void                        FromPlane(PlaneF const& plane, float maxExtents = CONVEX_HULL_MAX_BOUNDS);
    void                        FromPoints(Float3 const* points, size_t numPoints);

    Float3&                     operator[](size_t n) { return m_Points[n]; }
    Float3 const&               operator[](size_t n) const { return m_Points[n]; }

    size_t                      NumPoints() const { return m_Points.Size(); }

    bool                        IsEmpty() const { return m_Points.IsEmpty(); }

    Vector<Float3>&             GetVector() { return m_Points; }
    Vector<Float3> const&       GetVector() const { return m_Points; }

    Float3*                     GetPoints() { return m_Points.ToPtr(); }
    Float3 const*               GetPoints() const { return m_Points.ToPtr(); }

    void                        Clear() { m_Points.Clear(); }

    ConvexHull                  Reversed() const;

    void                        Reverse();

    PlaneSide                   Classify(PlaneF const& plane, float epsilon) const;

    bool                        IsTiny(float minEdgeLength) const;

    bool                        IsHuge() const;

    float                       CalcArea() const;

    BvAxisAlignedBox            CalcBounds() const;

    Float3                      CalcNormal() const;

    PlaneF                      CalcPlane() const;

    Float3                      CalcCenter() const;

    PlaneSide                   Split(PlaneF const& plane, float epsilon, ConvexHull& front, ConvexHull& back) const;

    PlaneSide                   Clip(PlaneF const& plane, float epsilon, ConvexHull& front) const;

private:
    Vector<Float3>              m_Points;
};

HK_NAMESPACE_END
