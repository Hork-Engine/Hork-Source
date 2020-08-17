/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "CoreMath.h"
#include "PodArray.h"
#include "Std.h"

typedef TPodArray< Double2 > AClipperContour;

struct SClipperPolygon
{
    AClipperContour Outer;
    TStdVector< AClipperContour > Holes;
};

enum EClipType
{
    CLIP_INTERSECT,
    CLIP_UNION,
    CLIP_DIFF,
    CLIP_XOR
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
    void SetTransform( Float3x3 const & _Transform3D );

    /** Transform matrix for 2D <-> 3D conversion */
    Float3x3 const & GetTransform() const { return Transform3D; }

    /** Set transform matrix from polygon normal */
    void SetTransformFromNormal( Float3 const & _Normal );

    /** Remove all contours */
    void Clear();

    /** Add subject contour */
    void AddSubj2D( Double2 const * _Points, int _NumPoints, bool _Closed = true );

    /** Add clip contour */
    void AddClip2D( Double2 const * _Points, int _NumPoints, bool _Closed = true );

    /** Add subject contour */
    void AddSubj3D( Double3 const * _Points, int _NumPoints, bool _Closed = true );

    /** Add clip contour */
    void AddClip3D( Double3 const * _Points, int _NumPoints, bool _Closed = true );

    /** Execute and build polygons */
    bool Execute( EClipType _ClipType, TStdVector< SClipperPolygon > & _Polygons );

    /** Execute and build contours */
    bool Execute( EClipType _ClipType, TStdVector< AClipperContour > & _Contours );

private:
    std::unique_ptr< ClipperLib::Clipper > pClipper;
    Float3x3 Transform3D;
    Float3x3 InvTransform3D;
};
