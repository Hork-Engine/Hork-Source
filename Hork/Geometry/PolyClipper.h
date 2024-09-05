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

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Core/Ref.h>
#include <Hork/Math/VectorMath.h>

namespace ClipperLib
{
class Clipper;
}

HK_NAMESPACE_BEGIN

using ClipperContour = Vector<Double2>;

struct ClipperPolygon
{
    ClipperContour          Outer;
    Vector<ClipperContour>  Holes;
};

enum class PolyClip
{
    Intersect,
    Union,
    Diff,
    XOR
};

class PolyClipper final : public Noncopyable
{
public:
                                    PolyClipper();
                                    ~PolyClipper();

    /// Transform matrix for 2D <-> 3D conversion
    void                            SetTransform(Float3x3 const& transform3D);

    /// Transform matrix for 2D <-> 3D conversion
    Float3x3 const&                 GetTransform() const { return m_Transform3D; }

    /// Set transform matrix from polygon normal
    void                            SetTransformFromNormal(Float3 const& normal);

    /// Remove all contours
    void                            Clear();

    /// Add subject contour
    void                            AddSubj2D(Double2 const* points, int pointsCount, bool closed = true);

    /// Add clip contour
    void                            AddClip2D(Double2 const* points, int pointsCount, bool closed = true);

    /// Add subject contour
    void                            AddSubj3D(Double3 const* points, int pointsCount, bool closed = true);

    /// Add clip contour
    void                            AddClip3D(Double3 const* points, int pointsCount, bool closed = true);

    /// Execute and build polygons
    bool                            Execute(PolyClip clipType, Vector<ClipperPolygon>& polygons);

    /// Execute and build contours
    bool                            Execute(PolyClip clipType, Vector<ClipperContour>& contours);

private:
    UniqueRef<ClipperLib::Clipper>  m_pImpl;
    Float3x3                        m_Transform3D;
    Float3x3                        m_InvTransform3D;
};

HK_NAMESPACE_END
