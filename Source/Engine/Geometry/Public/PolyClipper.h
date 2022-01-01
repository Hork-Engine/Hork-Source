/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Containers/Public/PodVector.h>
#include <Containers/Public/StdVector.h>
#include <Core/Public/Ref.h>
#include "VectorMath.h"

using AClipperContour = TPodVector<Double2>;

struct SClipperPolygon
{
    AClipperContour             Outer;
    TStdVector<AClipperContour> Holes;
};

enum POLY_CLIP_TYPE
{
    POLY_CLIP_TYPE_INTERSECT,
    POLY_CLIP_TYPE_UNION,
    POLY_CLIP_TYPE_DIFF,
    POLY_CLIP_TYPE_XOR
};

namespace ClipperLib
{
class Clipper;
}

class APolyClipper
{
public:
    APolyClipper();
    virtual ~APolyClipper();

    /** Transform matrix for 2D <-> 3D conversion */
    void SetTransform(Float3x3 const& transform3D);

    /** Transform matrix for 2D <-> 3D conversion */
    Float3x3 const& GetTransform() const { return transform3D_; }

    /** Set transform matrix from polygon normal */
    void SetTransformFromNormal(Float3 const& normal);

    /** Remove all contours */
    void Clear();

    /** Add subject contour */
    void AddSubj2D(Double2 const* points, int pointsCount, bool closed = true);

    /** Add clip contour */
    void AddClip2D(Double2 const* points, int pointsCount, bool closed = true);

    /** Add subject contour */
    void AddSubj3D(Double3 const* points, int pointsCount, bool closed = true);

    /** Add clip contour */
    void AddClip3D(Double3 const* points, int pointsCount, bool closed = true);

    /** Execute and build polygons */
    bool Execute(POLY_CLIP_TYPE clipType, TStdVector<SClipperPolygon>& polygons);

    /** Execute and build contours */
    bool Execute(POLY_CLIP_TYPE clipType, TStdVector<AClipperContour>& contours);

private:
    TUniqueRef<ClipperLib::Clipper> clipper_;
    Float3x3                        transform3D_;
    Float3x3                        invTransform3D_;
};
