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

#include "BvAxisAlignedBox.h"
#include "BvOrientedBox.h"
#include "BvSphere.h"

/*

Overlap tests:

Sphere - Sphere
Sphere - Triangle (not implemented)
Box - Box
Box - Sphere
Box - Triangle (not tested, not optimized)
Box - Triangle (approximation)
Oriented Box - Oriented Box (not tested)
Oriented Box - Sphere (not tested)
Oriented Box - Box (not tested)
Oriented Box - Triangle (not tested, not optimized)
Oriented Box - Triangle (approximation)

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
Sphere - Triangle (not implemented yet)

*/

// Sphere - Sphere
AN_FORCEINLINE bool BvSphereOverlapSphere( BvSphere const & _S1, BvSphere const & _S2 ) {
    const float R = _S1.Radius + _S2.Radius;
    return _S2.Center.DistSqr( _S1.Center ) <= R * R;
}

// Sphere - Triangle
//AN_FORCEINLINE bool BvSphereOverlapTriangle( BvSphere const & _Sphere, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {
// TODO
//}


/*

Box overlap test

Box - Box
Box - Sphere
Box - Triangle (not tested, not optimized)
Box - Triangle (approximation)

Box/Box intersection

*/

// AABB - AABB
AN_FORCEINLINE bool BvBoxOverlapBox( BvAxisAlignedBox const & _AABB1, BvAxisAlignedBox const & _AABB2 ) {
    if ( _AABB1.Maxs[0] < _AABB2.Mins[0] || _AABB1.Mins[0] > _AABB2.Maxs[0] ) return false;
    if ( _AABB1.Maxs[1] < _AABB2.Mins[1] || _AABB1.Mins[1] > _AABB2.Maxs[1] ) return false;
    if ( _AABB1.Maxs[2] < _AABB2.Mins[2] || _AABB1.Mins[2] > _AABB2.Maxs[2] ) return false;
    return true;
}

// AABB - Sphere
AN_FORCEINLINE bool BvBoxOverlapSphere( BvAxisAlignedBox const & _AABB, BvSphere const & _Sphere ) {
#if 0
    float d = 0, dist;
    for ( int i = 0 ; i < 3 ; i++ ) {
       // if sphere center lies behind of _AABB
       dist = _Sphere.Center[i] - _AABB.Mins[i];
       if ( dist < 0 ) {
          d += dist * dist; // summarize distance squire on this axis
       }
       // if sphere center lies ahead of _AABB,
       dist = _Sphere.Center[i] - _AABB.Maxs[i];
       if ( dist > 0 ) {
          d += dist * dist; // summarize distance squire on this axis
       }
    }
    return d <= _Sphere.Radius * _Sphere.Radius;
#else
    const Float3 DifMins = _Sphere.Center - _AABB.Mins;
    const Float3 DifMaxs = _Sphere.Center - _AABB.Maxs;

    return  ( ( DifMins.X < 0.0f ) * DifMins.X*DifMins.X
            + ( DifMins.Y < 0.0f ) * DifMins.Y*DifMins.Y
            + ( DifMins.Z < 0.0f ) * DifMins.Z*DifMins.Z
            + ( DifMaxs.X > 0.0f ) * DifMaxs.X*DifMaxs.X
            + ( DifMaxs.Y > 0.0f ) * DifMaxs.Y*DifMaxs.Y
            + ( DifMaxs.Z > 0.0f ) * DifMaxs.Z*DifMaxs.Z ) <= _Sphere.Radius * _Sphere.Radius;
#endif
}

// AABB - Triangle
AN_FORCEINLINE bool BvBoxOverlapTriangle( BvAxisAlignedBox const & _AABB, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {
    Float3  n;
    float   p;
    float   d0, d1;
    float   radius;

    const Float3 boxCenter = _AABB.Center();
    const Float3 halfSize = _AABB.HalfSize();

    // vector from box center to _P0
    const Float3 distVec = _P0 - boxCenter;

    // triangle edges
    const Float3 edge0 = _P1 - _P0;
    const Float3 edge1 = _P2 - _P0;
    const Float3 edge2 = edge1 - edge0;

    // triangle normal (not normalized)
    n = edge0.Cross( edge1 );

    if ( FMath::Abs( n.Dot( distVec ) ) > halfSize.X * FMath::Abs( n.X ) + halfSize.Y * FMath::Abs( n.Y ) + halfSize.Z * FMath::Abs( n.Z ) ) {
        return false;
    }

    for ( int i = 0; i < 3; i++ ) {
        p = distVec[i];
        d0 = edge0[i];
        d1 = edge1[i];
        radius = halfSize[ i ];

        if ( FMath::Min( p, FMath::Min( p + d0, p + d1 ) ) > radius || FMath::Max( p, FMath::Max( p + d0, p + d1 ) ) < -radius ) {
            return false;
        }
    }

    // check axisX and edge0
    n = Float3( 0, -edge0.Z, edge0.Y );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = halfSize[ 1 ] * FMath::Abs( edge0[2] ) + halfSize[ 2 ] * FMath::Abs( edge0[1] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge1
    n = Float3( 0, -edge1.Z, edge1.Y );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = halfSize[ 1 ] * FMath::Abs( edge1[2] ) + halfSize[ 2 ] * FMath::Abs( edge1[1] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge2
    n = Float3( 0, -edge2.Z, edge2.Y );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = halfSize[ 1 ] * FMath::Abs( edge2[2] ) + halfSize[ 2 ] * FMath::Abs( edge2[1] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge0
    n = Float3( edge0.Z, 0, -edge0.X );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = halfSize[ 0 ] * FMath::Abs( edge0[2] ) + halfSize[ 2 ] * FMath::Abs( edge0[0] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge1
    n = Float3( edge1.Z, 0, -edge1.X );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = halfSize[ 0 ] * FMath::Abs( edge1[2] ) + halfSize[ 2 ] * FMath::Abs( edge1[0] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge2
    n = Float3( edge2.Z, 0, -edge2.X );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = halfSize[ 0 ] * FMath::Abs( edge2[2] ) + halfSize[ 2 ] * FMath::Abs( edge2[0] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge0
    n = Float3( -edge0.Y, edge0.X, 0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = halfSize[ 0 ] * FMath::Abs( edge0[1] ) + halfSize[ 1 ] * FMath::Abs( edge0[0] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge1
    n = Float3( -edge1.Y, edge1.X, 0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = halfSize[ 0 ] * FMath::Abs( edge1[1] ) + halfSize[ 1 ] * FMath::Abs( edge1[0] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge2
    n = Float3( -edge2.Y, edge2.X, 0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = halfSize[ 0 ] * FMath::Abs( edge2[1] ) + halfSize[ 1 ] * FMath::Abs( edge2[0] );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    return true;
}

// AABB - Triangle (approximation)
AN_FORCEINLINE bool BvBoxOverlapTriangle_FastApproximation( BvAxisAlignedBox const & _BoundingBox, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {

    // Simple fast triangle - AABB overlap test

    BvAxisAlignedBox triangleBounds;

    triangleBounds.Mins.X = FMath::Min( _P0.X, _P1.X, _P2.X );
    triangleBounds.Mins.Y = FMath::Min( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Mins.Z = FMath::Min( _P0.Z, _P1.Z, _P2.Z );

    triangleBounds.Maxs.X = FMath::Max( _P0.X, _P1.X, _P2.X );
    triangleBounds.Maxs.Y = FMath::Max( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Maxs.Z = FMath::Max( _P0.Z, _P1.Z, _P2.Z );

    return BvBoxOverlapBox( _BoundingBox, triangleBounds );
}

AN_FORCEINLINE bool BvGetBoxIntersection( BvAxisAlignedBox const & _A, BvAxisAlignedBox const & _B, BvAxisAlignedBox & _Intersection ) {
    const float x_min = FMath::Max( _A.Mins[ 0 ], _B.Mins[ 0 ] );
    const float x_max = FMath::Min( _A.Maxs[ 0 ], _B.Maxs[ 0 ] );
    if ( x_max <= x_min ) {
        return false;
    }

    const float y_min = FMath::Max( _A.Mins[ 1 ], _B.Mins[ 1 ] );
    const float y_max = FMath::Min( _A.Maxs[ 1 ], _B.Maxs[ 1 ] );
    if ( y_max <= y_min ) {
        return false;
    }

    const float z_min = FMath::Max( _A.Mins[ 2 ], _B.Mins[ 2 ] );
    const float z_max = FMath::Min( _A.Maxs[ 2 ], _B.Maxs[ 2 ] );
    if ( z_max <= z_min ) {
        return false;
    }

    _Intersection.Mins[ 0 ] = x_min;
    _Intersection.Mins[ 1 ] = y_min;
    _Intersection.Mins[ 2 ] = z_min;

    _Intersection.Maxs[ 0 ] = x_max;
    _Intersection.Maxs[ 1 ] = y_max;
    _Intersection.Maxs[ 2 ] = z_max;

    return true;
}


/*

Oriented box overlap test

Oriented Box - Oriented Box (not tested)
Oriented Box - Sphere (not tested)
Oriented Box - Box (not tested)
Oriented Box - Triangle (not tested, not optimized)
Oriented Box - Triangle (approximation)

*/

// OBB - OBB
AN_FORCEINLINE bool BvOrientedBoxOverlapOrientedBox( BvOrientedBox const & _OBB1, BvOrientedBox const & _OBB2 ) {
    // Code based on http://gdlinks.hut.ru/cdfaq/obb.shtml

    const Float3x3 orientInversed = _OBB1.Orient.Transposed();

    // transform OBB2 position to OBB1 space
    const Float3 T = orientInversed * ( _OBB2.Center - _OBB1.Center );

    // transform OBB2 orientation to OBB1 space
    const Float3x3 R = orientInversed * _OBB2.Orient;

    float ra, rb;

    for ( int i = 0; i < 3; i++ ) {
        ra = _OBB1.HalfSize[ i ];
        rb = _OBB2.HalfSize[ 0 ] * FMath::Abs( R[ i ][ 0 ] ) + _OBB2.HalfSize[ 1 ] * FMath::Abs( R[ i ][ 1 ] ) + _OBB2.HalfSize[ 2 ] * FMath::Abs( R[ i ][ 2 ] );
        if ( FMath::Abs( T[ i ] ) > ra + rb )
            return false;
    }

    for ( int i = 0; i < 3; i++ ) {
        ra = _OBB1.HalfSize[ 0 ] * FMath::Abs( R[ 0 ][ i ] ) + _OBB1.HalfSize[ 1 ] * FMath::Abs( R[ 1 ][ i ] ) + _OBB1.HalfSize[ 2 ] * FMath::Abs( R[ 2 ][ i ] );
        rb = _OBB2.HalfSize[ i ];
        if ( FMath::Abs( ( T[ 0 ] * R[ 0 ][ i ] + T[ 1 ] * R[ 1 ][ i ] + T[ 2 ] * R[ 2 ][ i ] ) ) > ra + rb )
            return false;
    }

    ra = _OBB1.HalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 0 ] ) + _OBB1.HalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 0 ] );
    rb = _OBB2.HalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 1 ] );
    if ( FMath::Abs( ( T[ 2 ] * R[ 1 ][ 0 ] - T[ 1 ] * R[ 2 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 1 ] ) + _OBB1.HalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 1 ] );
    rb = _OBB2.HalfSize[ 0 ] * FMath::Abs( R[ 0 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 0 ] );
    if ( FMath::Abs( ( T[ 2 ] * R[ 1 ][ 1 ] - T[ 1 ] * R[ 2 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _OBB1.HalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 2 ] );
    rb = _OBB2.HalfSize[ 0 ] * FMath::Abs( R[ 0 ][ 1 ] ) + _OBB2.HalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 0 ] );
    if ( FMath::Abs( ( T[ 2 ] * R[ 1 ][ 2 ] - T[ 1 ] * R[ 2 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 0 ] ) + _OBB1.HalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 0 ] );
    rb = _OBB2.HalfSize[ 1 ] * FMath::Abs( R[ 1 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 1 ] );
    if ( FMath::Abs( ( T[ 0 ] * R[ 2 ][ 0 ] - T[ 2 ] * R[ 0 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 1 ] ) + _OBB1.HalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 1 ] );
    rb = _OBB2.HalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 0 ] );
    if ( FMath::Abs( ( T[ 0 ] * R[ 2 ][ 1 ] - T[ 2 ] * R[ 0 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _OBB1.HalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 2 ] );
    rb = _OBB2.HalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 1 ] ) + _OBB2.HalfSize[ 1 ] * FMath::Abs( R[ 1 ][ 0 ] );
    if ( FMath::Abs( ( T[ 0 ] * R[ 2 ][ 2 ] - T[ 2 ] * R[ 0 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 0 ] ) + _OBB1.HalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 0 ] );
    rb = _OBB2.HalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * FMath::Abs( R[ 2 ][ 1 ] );
    if ( FMath::Abs( ( T[ 1 ] * R[ 0 ][ 0 ] - T[ 0 ] * R[ 1 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 1 ] ) + _OBB1.HalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 1 ] );
    rb = _OBB2.HalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * FMath::Abs( R[ 2 ][ 0 ] );
    if ( FMath::Abs( ( T[ 1 ] * R[ 0 ][ 1 ] - T[ 0 ] * R[ 1 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 2 ] ) + _OBB1.HalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 2 ] );
    rb = _OBB2.HalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 1 ] ) + _OBB2.HalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 0 ] );
    if ( FMath::Abs( ( T[ 1 ] * R[ 0 ][ 2 ] - T[ 0 ] * R[ 1 ][ 2 ] ) ) > ra + rb )
        return false;

    return true;
}

// OBB - Sphere
AN_FORCEINLINE bool BvOrientedBoxOverlapSphere( BvOrientedBox const & _OBB, BvSphere const & _Sphere ) {

    // Transform sphere center to OBB space
    const Float3 sphereCenter = _OBB.Orient.Transposed() * ( _Sphere.Center - _OBB.Center );

    const Float3 DifMins = sphereCenter + _OBB.HalfSize;
    const Float3 DifMaxs = sphereCenter - _OBB.HalfSize;

    return  ( ( DifMins.X < 0.0f ) * DifMins.X*DifMins.X
        + ( DifMins.Y < 0.0f ) * DifMins.Y*DifMins.Y
        + ( DifMins.Z < 0.0f ) * DifMins.Z*DifMins.Z
        + ( DifMaxs.X > 0.0f ) * DifMaxs.X*DifMaxs.X
        + ( DifMaxs.Y > 0.0f ) * DifMaxs.Y*DifMaxs.Y
        + ( DifMaxs.Z > 0.0f ) * DifMaxs.Z*DifMaxs.Z ) <= _Sphere.Radius * _Sphere.Radius;
}

// OBB - AABB
AN_FORCEINLINE bool BvOrientedBoxOverlapBox( BvOrientedBox const & _OBB, Float3 const & _AABBCenter, Float3 const & _AABBHalfSize ) {
    // transform OBB position to AABB space
    const Float3 T = _OBB.Center - _AABBCenter;

    // OBB orientation relative to AABB space
    const Float3x3 & R = _OBB.Orient;

    float ra, rb;

    for ( int i = 0; i < 3; i++ ) {
        ra = _AABBHalfSize[ i ];
        rb = _OBB.HalfSize[ 0 ] * FMath::Abs( R[ i ][ 0 ] ) + _OBB.HalfSize[ 1 ] * FMath::Abs( R[ i ][ 1 ] ) + _OBB.HalfSize[ 2 ] * FMath::Abs( R[ i ][ 2 ] );
        if ( FMath::Abs( T[ i ] ) > ra + rb )
            return false;
    }

    for ( int i = 0; i < 3; i++ ) {
        ra = _AABBHalfSize[ 0 ] * FMath::Abs( R[ 0 ][ i ] ) + _AABBHalfSize[ 1 ] * FMath::Abs( R[ 1 ][ i ] ) + _AABBHalfSize[ 2 ] * FMath::Abs( R[ 2 ][ i ] );
        rb = _OBB.HalfSize[ i ];
        if ( FMath::Abs( ( T[ 0 ] * R[ 0 ][ i ] + T[ 1 ] * R[ 1 ][ i ] + T[ 2 ] * R[ 2 ][ i ] ) ) > ra + rb )
            return false;
    }

    ra = _AABBHalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 0 ] ) + _AABBHalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 0 ] );
    rb = _OBB.HalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 1 ] );
    if ( FMath::Abs( ( T[ 2 ] * R[ 1 ][ 0 ] - T[ 1 ] * R[ 2 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 1 ] ) + _AABBHalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 1 ] );
    rb = _OBB.HalfSize[ 0 ] * FMath::Abs( R[ 0 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 0 ] );
    if ( FMath::Abs( ( T[ 2 ] * R[ 1 ][ 1 ] - T[ 1 ] * R[ 2 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _AABBHalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 2 ] );
    rb = _OBB.HalfSize[ 0 ] * FMath::Abs( R[ 0 ][ 1 ] ) + _OBB.HalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 0 ] );
    if ( FMath::Abs( ( T[ 2 ] * R[ 1 ][ 2 ] - T[ 1 ] * R[ 2 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 0 ] ) + _AABBHalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 0 ] );
    rb = _OBB.HalfSize[ 1 ] * FMath::Abs( R[ 1 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 1 ] );
    if ( FMath::Abs( ( T[ 0 ] * R[ 2 ][ 0 ] - T[ 2 ] * R[ 0 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 1 ] ) + _AABBHalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 1 ] );
    rb = _OBB.HalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * FMath::Abs( R[ 1 ][ 0 ] );
    if ( FMath::Abs( ( T[ 0 ] * R[ 2 ][ 1 ] - T[ 2 ] * R[ 0 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _AABBHalfSize[ 2 ] * FMath::Abs( R[ 0 ][ 2 ] );
    rb = _OBB.HalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 1 ] ) + _OBB.HalfSize[ 1 ] * FMath::Abs( R[ 1 ][ 0 ] );
    if ( FMath::Abs( ( T[ 0 ] * R[ 2 ][ 2 ] - T[ 2 ] * R[ 0 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 0 ] ) + _AABBHalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 0 ] );
    rb = _OBB.HalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * FMath::Abs( R[ 2 ][ 1 ] );
    if ( FMath::Abs( ( T[ 1 ] * R[ 0 ][ 0 ] - T[ 0 ] * R[ 1 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 1 ] ) + _AABBHalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 1 ] );
    rb = _OBB.HalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * FMath::Abs( R[ 2 ][ 0 ] );
    if ( FMath::Abs( ( T[ 1 ] * R[ 0 ][ 1 ] - T[ 0 ] * R[ 1 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * FMath::Abs( R[ 1 ][ 2 ] ) + _AABBHalfSize[ 1 ] * FMath::Abs( R[ 0 ][ 2 ] );
    rb = _OBB.HalfSize[ 0 ] * FMath::Abs( R[ 2 ][ 1 ] ) + _OBB.HalfSize[ 1 ] * FMath::Abs( R[ 2 ][ 0 ] );
    if ( FMath::Abs( ( T[ 1 ] * R[ 0 ][ 2 ] - T[ 0 ] * R[ 1 ][ 2 ] ) ) > ra + rb )
        return false;

    return true;
}

// OBB - AABB
AN_FORCEINLINE bool BvOrientedBoxOverlapBox( BvOrientedBox const & _OBB, BvAxisAlignedBox const & _AABB ) {
    return BvOrientedBoxOverlapBox( _OBB, _AABB.Center(), _AABB.HalfSize() );
}

// OBB - Triangle
AN_FORCEINLINE bool BvOrientedBoxOverlapTriangle( BvOrientedBox const & _OBB, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {

    // Code based on http://gdlinks.hut.ru/cdfaq/obb.shtml

    Float3  n;
    float   p;
    float   d0, d1;
    float   radius;

    // vector from box center to _P0
    const Float3 distVec = _P0 - _OBB.Center;

    // triangle edges
    const Float3 edge0 = _P1 - _P0;
    const Float3 edge1 = _P2 - _P0;
    const Float3 edge2 = edge1 - edge0;

    // triangle normal (not normalized)
    n = edge0.Cross( edge1 );

    if ( FMath::Abs( n.Dot( distVec ) ) > ( _OBB.HalfSize.X * FMath::Abs( n.Dot( _OBB.Orient[ 0 ] ) ) + _OBB.HalfSize.Y * FMath::Abs( n.Dot( _OBB.Orient[ 1 ] ) ) + _OBB.HalfSize.Z * FMath::Abs( n.Dot( _OBB.Orient[ 2 ] ) ) ) )
        return false;

    for ( int i = 0; i < 3; i++ ) {
        p = _OBB.Orient[ i ].Dot( distVec );
        d0 = _OBB.Orient[ i ].Dot( edge0 );
        d1 = _OBB.Orient[ i ].Dot( edge1 );
        radius = _OBB.HalfSize[ i ];

        if ( FMath::Min( p, FMath::Min( p + d0, p + d1 ) ) > radius || FMath::Max( p, FMath::Max( p + d0, p + d1 ) ) < -radius ) {
            return false;
        }
    }

    // check axisX and edge0
    n = _OBB.Orient[ 0 ].Cross( edge0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = _OBB.HalfSize[ 1 ] * FMath::Abs( _OBB.Orient[ 2 ].Dot( edge0 ) ) + _OBB.HalfSize[ 2 ] * FMath::Abs( _OBB.Orient[ 1 ].Dot( edge0 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge1
    n = _OBB.Orient[ 0 ].Cross( edge1 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[ 1 ] * FMath::Abs( _OBB.Orient[ 2 ].Dot( edge1 ) ) + _OBB.HalfSize[ 2 ] * FMath::Abs( _OBB.Orient[ 1 ].Dot( edge1 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge2
    n = _OBB.Orient[ 0 ].Cross( edge2 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[ 1 ] * FMath::Abs( _OBB.Orient[ 2 ].Dot( edge2 ) ) + _OBB.HalfSize[ 2 ] * FMath::Abs( _OBB.Orient[ 1 ].Dot( edge2 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge0
    n = _OBB.Orient[ 1 ].Cross( edge0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = _OBB.HalfSize[ 0 ] * FMath::Abs( _OBB.Orient[ 2 ].Dot( edge0 ) ) + _OBB.HalfSize[ 2 ] * FMath::Abs( _OBB.Orient[ 0 ].Dot( edge0 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge1
    n = _OBB.Orient[ 1 ].Cross( edge1 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[ 0 ] * FMath::Abs( _OBB.Orient[ 2 ].Dot( edge1 ) ) + _OBB.HalfSize[ 2 ] * FMath::Abs( _OBB.Orient[ 0 ].Dot( edge1 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge2
    n = _OBB.Orient[ 1 ].Cross( edge2 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[ 0 ] * FMath::Abs( _OBB.Orient[ 2 ].Dot( edge2 ) ) + _OBB.HalfSize[ 2 ] * FMath::Abs( _OBB.Orient[ 0 ].Dot( edge2 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge0
    n = _OBB.Orient[ 2 ].Cross( edge0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = _OBB.HalfSize[ 0 ] * FMath::Abs( _OBB.Orient[ 1 ].Dot( edge0 ) ) + _OBB.HalfSize[ 1 ] * FMath::Abs( _OBB.Orient[ 0 ].Dot( edge0 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge1
    n = _OBB.Orient[ 2 ].Cross( edge1 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[ 0 ] * FMath::Abs( _OBB.Orient[ 1 ].Dot( edge1 ) ) + _OBB.HalfSize[ 1 ] * FMath::Abs( _OBB.Orient[ 0 ].Dot( edge1 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge2
    n = _OBB.Orient[ 2 ].Cross( edge2 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[ 0 ] * FMath::Abs( _OBB.Orient[ 1 ].Dot( edge2 ) ) + _OBB.HalfSize[ 1 ] * FMath::Abs( _OBB.Orient[ 0 ].Dot( edge2 ) );

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    return true;
}

// OBB - Triangle (approximation)
AN_FORCEINLINE bool BvOrientedBoxOverlapTriangle_FastApproximation( BvOrientedBox const & _OBB, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {

    // Simple fast triangle - AABB overlap test

    BvAxisAlignedBox triangleBounds;

    triangleBounds.Mins.X = FMath::Min( _P0.X, _P1.X, _P2.X );
    triangleBounds.Mins.Y = FMath::Min( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Mins.Z = FMath::Min( _P0.Z, _P1.Z, _P2.Z );

    triangleBounds.Maxs.X = FMath::Max( _P0.X, _P1.X, _P2.X );
    triangleBounds.Maxs.Y = FMath::Max( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Maxs.Z = FMath::Max( _P0.Z, _P1.Z, _P2.Z );

    return BvOrientedBoxOverlapBox( _OBB, triangleBounds );
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

// Ray - Sphere
AN_FORCEINLINE bool BvRayIntersectSphere( Float3 const & _RayStart, Float3 const & _RayDir, BvSphere const & _Sphere, float & _Min, float & _Max ) {
    const Float3 k = _RayStart - _Sphere.Center;
    const float b = k.Dot( _RayDir );
    float distance = b * b - k.LengthSqr() + _Sphere.Radius * _Sphere.Radius;
    if ( distance < 0.0f ) {
        return false;
    }
    distance = FMath::Sqrt( distance );
    FMath::MinMax( -b + distance, -b - distance, _Min, _Max );
    return _Min > 0.0f || _Max > 0.0f;
}

// Ray - Sphere
AN_FORCEINLINE bool BvRayIntersectSphere( Float3 const & _RayStart, Float3 const & _RayDir, BvSphere const & _Sphere, float & _Distance ) {
    const Float3 k = _RayStart - _Sphere.Center;
    const float b = k.Dot( _RayDir );
    _Distance = b * b - k.LengthSqr() + _Sphere.Radius * _Sphere.Radius;
    if ( _Distance < 0.0f ) {
        return false;
    }
    float t1, t2;
    _Distance = FMath::Sqrt( _Distance );
    FMath::MinMax( -b + _Distance, -b - _Distance, t1, t2 );
    _Distance = t1 >= 0.0f ? t1 : t2;
    return _Distance > 0.0f;
}

// Ray - AABB
AN_FORCEINLINE bool BvRayIntersectBox( Float3 const & _RayStart, Float3 const & _InvRayDir, BvAxisAlignedBox const & _AABB, float & _Min, float & _Max ) {
    float Lo = _InvRayDir.X*(_AABB.Mins.X - _RayStart.X);
    float Hi = _InvRayDir.X*(_AABB.Maxs.X - _RayStart.X);
    _Min = FMath::Min(Lo, Hi);
    _Max = FMath::Max(Lo, Hi);

    Lo = _InvRayDir.Y*(_AABB.Mins.Y - _RayStart.Y);
    Hi = _InvRayDir.Y*(_AABB.Maxs.Y - _RayStart.Y);
    _Min = FMath::Max(_Min, FMath::Min(Lo, Hi));
    _Max = FMath::Min(_Max, FMath::Max(Lo, Hi));

    Lo = _InvRayDir.Z*(_AABB.Mins.Z - _RayStart.Z);
    Hi = _InvRayDir.Z*(_AABB.Maxs.Z - _RayStart.Z);
    _Min = FMath::Max(_Min, FMath::Min(Lo, Hi));
    _Max = FMath::Min(_Max, FMath::Max(Lo, Hi));

    return (_Min <= _Max) && (_Max > 0.0f);
}

// Ray - AABB
AN_FORCEINLINE bool BvRayIntersectBox( Float3 const & _RayStart, Float3 const & _InvRayDir, BvAxisAlignedBox const & _AABB ) {
    float Min, Max, Lo, Hi;

    Lo = _InvRayDir.X*(_AABB.Mins.X - _RayStart.X);
    Hi = _InvRayDir.X*(_AABB.Maxs.X - _RayStart.X);
    Min = FMath::Min(Lo, Hi);
    Max = FMath::Max(Lo, Hi);

    Lo = _InvRayDir.Y*(_AABB.Mins.Y - _RayStart.Y);
    Hi = _InvRayDir.Y*(_AABB.Maxs.Y - _RayStart.Y);
    Min = FMath::Max(Min, FMath::Min(Lo, Hi));
    Max = FMath::Min(Max, FMath::Max(Lo, Hi));

    Lo = _InvRayDir.Z*(_AABB.Mins.Z - _RayStart.Z);
    Hi = _InvRayDir.Z*(_AABB.Maxs.Z - _RayStart.Z);
    Min = FMath::Max(Min, FMath::Min(Lo, Hi));
    Max = FMath::Min(Max, FMath::Max(Lo, Hi));

    return (Min <= Max) && (Max > 0.0f);
}

// Ray - OBB
AN_FORCEINLINE bool BvRayIntersectOrientedBox( Float3 const & _RayStart, Float3 const & _RayDir, BvOrientedBox const & _OBB, float & _Min, float & _Max ) {
    const Float3x3 OrientInversed = _OBB.Orient.Transposed();

    // Transform ray to OBB space
    const Float3 RayStart = OrientInversed * ( _RayStart - _OBB.Center );
    const Float3 RayDir = OrientInversed * _RayDir;

    // Mins and maxs in OBB space
    Float3 const Mins = -_OBB.HalfSize;
    Float3 const & Maxs = _OBB.HalfSize;

    const float InvRayDirX = 1.0f / RayDir.X;
    const float InvRayDirY = 1.0f / RayDir.Y;
    const float InvRayDirZ = 1.0f / RayDir.Z;

    float Lo = InvRayDirX*(Mins.X - RayStart.X);
    float Hi = InvRayDirX*(Maxs.X - RayStart.X);
    _Min = FMath::Min(Lo, Hi);
    _Max = FMath::Max(Lo, Hi);

    Lo = InvRayDirY*(Mins.Y - RayStart.Y);
    Hi = InvRayDirY*(Maxs.Y - RayStart.Y);
    _Min = FMath::Max(_Min, FMath::Min(Lo, Hi));
    _Max = FMath::Min(_Max, FMath::Max(Lo, Hi));

    Lo = InvRayDirZ*(Mins.Z - RayStart.Z);
    Hi = InvRayDirZ*(Maxs.Z - RayStart.Z);
    _Min = FMath::Max(_Min, FMath::Min(Lo, Hi));
    _Max = FMath::Min(_Max, FMath::Max(Lo, Hi));

    return (_Min <= _Max) && (_Max > 0.0f);
}

// Ray - triangle
AN_FORCEINLINE bool BvRayIntersectTriangle( Float3 const & _RayStart, Float3 const & _RayDir, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2, float & _Distance, float & _U, float & _V ) {
    Float3 e1 = _P1 - _P0;
    Float3 e2 = _P2 - _P0;
    Float3 h = _RayDir.Cross( e2 );

    // calc determinant
    float det = e1.Dot( h );

    // ray lies in plane of triangle, so there is no intersection
    if ( det > -0.00001 && det < 0.00001 ) {
        return false;
    }

    // calc inverse determinant to minimalize math divisions in next calculations
    float invDet = 1 / det;

    // calc vector from ray origin to P0
    Float3 s = _RayStart - _P0;

    // calc U
    _U = invDet * s.Dot( h );
    if ( _U < 0.0f || _U > 1.0f ) {
        return false;
    }

    // calc perpendicular to compute V
    Float3 q = s.Cross( e1 );

    // calc V
    _V = invDet * _RayDir.Dot( q );
    if ( _V < 0.0f || _U + _V > 1.0f ) {
        return false;
    }

    // compute distance to find out where the intersection point is on the line
    _Distance = invDet * e2.Dot( q );

    // if _Distance < 0 this is a line intersection but not a ray intersection
    return _Distance > 0.0f;
}

// Ray - Plane
AN_FORCEINLINE bool BvRayIntersectPlane( Float3 const & _RayStart, Float3 const & _RayDir, PlaneF const & _Plane, float & _Distance ) {
    const float d2 = _Plane.Normal.Dot( _RayDir );
    if ( d2 == 0.0f ) {
        // ray is parallel to plane
        return false;
    }
    const float d1 = _Plane.Normal.Dot( _RayStart ) + _Plane.D;
    _Distance = -( d1 / d2 );
    return true;
}

// Ray - Elipsoid
AN_FORCEINLINE bool BvRayIntersectElipsoid( Float3 const & _RayStart, Float3 const & _RayDir, float const & _Radius, float const & _MParam, float const & _NParam, float & _Min, float & _Max ) {
    const float a = _RayDir.X*_RayDir.X + _MParam*_RayDir.Y*_RayDir.Y + _NParam*_RayDir.Z*_RayDir.Z;
    const float b = 2.0f*( _RayStart.X*_RayDir.X + _MParam*_RayStart.Y*_RayDir.Y + _NParam*_RayStart.Z*_RayDir.Z );
    const float c = _RayStart.X*_RayStart.X + _MParam*_RayStart.Y*_RayStart.Y + _NParam*_RayStart.Z*_RayStart.Z - _Radius*_Radius;
    const float d = b*b - 4.0f*a*c;
    if ( d < 0.0f ) {
        return false;
    }
    const float distance = FMath::Sqrt( d );
    const float denom = 0.5f / a;
    FMath::MinMax( ( -b + distance ) * denom, ( -b - distance ) * denom, _Min, _Max );
    return _Min > 0.0f || _Max > 0.0f;
}

// Ray - Elipsoid
AN_FORCEINLINE bool BvRayIntersectElipsoid( Float3 const & _RayStart, Float3 const & _RayDir, float const & _Radius, float const & _MParam, float const & _NParam, float & _Distance ) {
    const float a = _RayDir.X*_RayDir.X + _MParam*_RayDir.Y*_RayDir.Y + _NParam*_RayDir.Z*_RayDir.Z;
    const float b = 2.0f*( _RayStart.X*_RayDir.X + _MParam*_RayStart.Y*_RayDir.Y + _NParam*_RayStart.Z*_RayDir.Z );
    const float c = _RayStart.X*_RayStart.X + _MParam*_RayStart.Y*_RayStart.Y + _NParam*_RayStart.Z*_RayStart.Z - _Radius*_Radius;
    const float d = b*b - 4.0f*a*c;
    if ( d < 0.0f ) {
        return false;
    }
    _Distance = FMath::Sqrt( d );
    const float denom = 0.5f / a;
    float t1, t2;
    FMath::MinMax( ( -b + _Distance ) * denom, ( -b - _Distance ) * denom, t1, t2 );
    _Distance = t1 >= 0.0f ? t1 : t2;
    return _Distance > 0.0f;
}


/*


Point tests


*/

AN_FORCEINLINE bool BvPointInPoly2D( Float2 const * _Points, int _NumPoints, float const & _PX, float const & _PY ) {
    int i, j, count = 0;
	
    for ( i = 0, j = _NumPoints - 1; i < _NumPoints; j = i++ ) {
        if (    ( ( _Points[i].Y <= _PY && _PY < _Points[j].Y ) || ( _Points[j].Y <= _PY && _PY < _Points[i].Y ) )
            &&  ( _PX < ( _Points[j].X - _Points[i].X ) * ( _PY - _Points[i].Y ) / ( _Points[j].Y - _Points[i].Y ) + _Points[i].X ) ) {
            count++;
        }
    }
	
    return count & 1;
}

AN_FORCEINLINE bool BvPointInPoly2D( Float2 const * _Points, int _NumPoints, Float2 const & _Point ) {
    return BvPointInPoly2D( _Points, _NumPoints, _Point.X, _Point.Y );
}


// Square of shortest distance between Point and Segment
AN_FORCEINLINE float BvShortestDistanceSqr( Float3 const & _Point, Float3 const & _Start, Float3 const & _End ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return _Point.DistSqr( _Start );
    }

    const float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return _Point.DistSqr( _End );
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir );
}

// Square of distance between Point and Segment
AN_FORCEINLINE bool BvDistanceSqr( Float3 const & _Point, Float3 const & _Start, Float3 const & _End, float & _Distance ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    _Distance = v.DistSqr( ( dp1 / dp2 ) * dir );

    return true;
}

// Check Point on Segment
AN_FORCEINLINE bool BvIsPointOnSegment( Float3 const & _Point, Float3 const & _Start, Float3 const & _End, float _Epsilon ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir ) < _Epsilon;
}

// Square of distance between Point and Segment (2D)
AN_FORCEINLINE float BvShortestDistanceSqr( Float2 const & _Point, Float2 const & _Start, Float2 const & _End ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return _Point.DistSqr( _Start );
    }

    const float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return _Point.DistSqr( _End );
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir );
}

// Square of distance between Point and Segment (2D)
AN_FORCEINLINE bool BvDistanceSqr( Float2 const & _Point, Float2 const & _Start, Float2 const & _End, float & _Distance ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    _Distance = v.DistSqr( ( dp1 / dp2 ) * dir );

    return true;
}

// Check Point on Segment (2D)
AN_FORCEINLINE bool BvIsPointOnSegment( Float2 const & _Point, Float2 const & _Start, Float2 const & _End, float _Epsilon ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir ) < _Epsilon;
}


#if 0
// Segment - Sphere
AN_FORCEINLINE bool BvSegmentIntersectSphere( SegmentF const & _Segment, BvSphere const & _Sphere ) {
    const Float3 s = _Segment.Start - _Sphere.Center;
    const Float3 e = _Segment.End - _Sphere.Center;
    Float3 r = e - s;
    const float a = r.Dot( -s );

    if ( a <= 0.0f ) {
        return s.LengthSqr() < _Sphere.Radius * _Sphere.Radius;
    }

    const float dot_r_r = r.LengthSqr();

    if ( a >= dot_r_r ) {
        return e.LengthSqr() < _Sphere.Radius * _Sphere.Radius;
    }

    r = s + ( a / dot_r_r ) * r;
    return r.LengthSqr() < _Sphere.Radius * _Sphere.Radius;
}

// Ray - OBB
//AN_FORCEINLINE bool BvSegmentIntersectOrientedBox( RaySegment const & _Ray, BvOrientedBox const & _OBB ) {
//    // Трансформируем отрезок в систему координат OBB

//    //смещение в мировой системе координат
//    Float3 RayStart = _Ray.Start - _OBB.Center;
//    //смещение в системе координат _OBB1
//    RayStart = _OBB.Orient * RayStart + _OBB.Center;

//    //смещение в мировой системе координат
//    Float3 rayEnd = _Ray.end - _OBB.Center;
//    //смещение в системе координат _OBB1
//    rayEnd = _OBB.Orient * rayEnd + _OBB.Center;

//    const Float3 rayVec = rayEnd - RayStart;

//    const float ZeroTolerance = 0.000001f;  // TODO: вынести куда-нибудь

//    float length = Length( rayVec );
//    if ( length < ZeroTolerance ) {
//        return false;
//    }

//    const Float3 dir = rayVec / length;

//    length *= 0.5f; // half length

//    // T = Позиция OBB - midRayPoint
//    const Float3 T = _OBB.Center - (RayStart + rayVec * 0.5f);

//    // проверяем, является ли одна из осей X,Y,Z разделяющей
//    if ( (fabs( T.X ) > _OBB.HalfSize.X + length*fabs( dir.X )) ||
//         (fabs( T.Y ) > _OBB.HalfSize.Y + length*fabs( dir.Y )) ||
//         (fabs( T.Z ) > _OBB.HalfSize.Z + length*fabs( dir.Z )) )
//        return false;

//    float r;

//    // проверяем X ^ dir
//    r = _OBB.HalfSize.Y * fabs( dir.Z ) + _OBB.HalfSize.Z * fabs( dir.Y );
//    if ( fabs( T.Y * dir.Z - T.Z * dir.Y ) > r )
//        return false;

//    // проверяем Y ^ dir
//    r = _OBB.HalfSize.X * fabs( dir.Z ) + _OBB.HalfSize.Z * fabs( dir.X );
//    if ( fabs( T.Z * dir.X - T.X * dir.Z ) > r )
//        return false;

//    // проверяем Z ^ dir
//    r = _OBB.HalfSize.X * fabs( dir.Y ) + _OBB.HalfSize.Y * fabs( dir.X );
//    if ( fabs( T.X * dir.Y - T.Y * dir.X ) > r )
//        return false;

//    return true;
//}
// Segment - Plane
// _Distance - расстояние от начала луча до плоскости в направлении луча
AN_FORCEINLINE bool BvSegmentIntersectPlane( SegmentF const & _Segment, PlaneF const & _Plane, float & _Distance ) {
    const Float3 Dir = _Segment.End - _Segment.Start;
    const float Length = Dir.Length();
    if ( Length.CompareEps( 0.0f, 0.00001f ) ) {
        return false; // null-length segment
    }
    const float d2 = _Plane.Normal.Dot( Dir / Length );
    if ( d2.CompareEps( 0.0f, 0.00001f ) ) {
        // Луч параллелен плоскости
        return false;
    }
    const float d1 = _Plane.Normal.Dot( _Segment.Start ) + _Plane.D;
    _Distance = -( d1 / d2 );
    return _Distance >= 0.0f && _Distance <= Length;
}
#endif
