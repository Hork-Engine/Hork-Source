/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "Plane.h"
#include "BV/BvAxisAlignedBox.h"

#define CONVEX_HULL_MAX_BOUNDS        ( 5*1024 )//99999999999.0f
#define CONVEX_HULL_MIN_BOUNDS        ( -5*1024 )//99999999999.0f

class FConvexHull final {
    AN_FORBID_COPY( FConvexHull )

private:
    FConvexHull() {}
    ~FConvexHull() {}

public:
    static FConvexHull * Create( int _MaxPoints );

    static FConvexHull * CreateForPlane( PlaneF const & _Plane, float _MaxExtents = CONVEX_HULL_MAX_BOUNDS );

    static FConvexHull * CreateFromPoints( Float3 const * _Points, int _NumPoints );
    static FConvexHull * RecreateFromPoints( FConvexHull * _OldHull, Float3 const * _Points, int _NumPoints );

    static void Destroy( FConvexHull * _Hull );

    FConvexHull * Duplicate() const;

    FConvexHull * Reversed() const;

    EPlaneSide Classify( PlaneF const & _Plane, float _Epsilon ) const;

    bool IsTiny( float _MinEdgeLength ) const;

    bool IsHuge() const;

    float CalcArea() const;

    BvAxisAlignedBox CalcBounds() const;

    Float3 CalcNormal() const;

    PlaneF CalcPlane() const;

    Float3 CalcCenter() const;

    void Reverse();

    EPlaneSide Split( PlaneF const & _Plane, float _Epsilon, FConvexHull ** _Front, FConvexHull ** _Back ) const;

    EPlaneSide Clip( PlaneF const & _Plane, float _Epsilon, FConvexHull ** _Front ) const;

    int NumPoints;
    int MaxPoints;
    Float3 Points[4];
};
