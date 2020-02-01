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

#include "BvAxisAlignedBox.h"
#include "BvOrientedBox.h"
#include "BvSphere.h"

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

/** Sphere - Sphere */
AN_INLINE bool BvSphereOverlapSphere( BvSphere const & _S1, BvSphere const & _S2 ) {
    const float R = _S1.Radius + _S2.Radius;
    return _S2.Center.DistSqr( _S1.Center ) <= R * R;
}

/** Sphere - Point */
AN_INLINE bool BvSphereOverlapPoint( BvSphere const & _Sphere, Float3 const & _Point ) {
    return _Point.DistSqr( _Sphere.Center ) <= _Sphere.Radius * _Sphere.Radius;
}

/** Sphere - Triangle */
AN_INLINE bool BvSphereOverlapTriangle( BvSphere const & _Sphere, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {
    // based on OPCODE library

    const float radiusSqr = _Sphere.Radius*_Sphere.Radius;

    // Is vertices inside the sphere
    if ( (_P2 - _Sphere.Center).LengthSqr() <= radiusSqr ) {
        return true;
    }

    if ( (_P1 - _Sphere.Center).LengthSqr() <= radiusSqr ) {
        return true;
    }

    const Float3 vec = _P0 - _Sphere.Center;
    const float vecDistSqr = vec.LengthSqr();
    if ( vecDistSqr <= radiusSqr ) {
        return true;
    }

    // Full distance test
    const Float3 e0 = _P1 - _P0;
    const Float3 e1 = _P2 - _P0;

    const float a00 = e0.LengthSqr();
    const float a01 = Math::Dot( e0, e1 );
    const float a11 = e1.LengthSqr();
    const float b0 = Math::Dot( vec, e0 );
    const float b1 = Math::Dot( vec, e1 );
    const float det = Math::Abs( a00 * a11 - a01 * a01 );
    float u = a01 * b1 - a11 * b0;
    float v = a01 * b0 - a00 * b1;
    float distSqr;

    if ( u + v <= det )
    {
        if ( u < 0.0f )
        {
            if ( v < 0.0f )  // region 4
            {
                if ( b0 < 0.0f )
                {
                    // v = 0.0f;
                    if ( -b0 >= a00 ) {
                        // u = 1.0f;
                        distSqr = a00+2.0f*b0+vecDistSqr;
                    } else {
                        u = -b0/a00;
                        distSqr = b0*u+vecDistSqr;
                    }
                } else {
                    // u = 0.0f;
                    if ( b1 >= 0.0f ) {
                        // v = 0.0f;
                        distSqr = vecDistSqr;
                    } else if ( -b1 >= a11 ) {
                        // v = 1.0f;
                        distSqr = a11+2.0f*b1+vecDistSqr;
                    } else {
                        v = -b1/a11;
                        distSqr = b1*v+vecDistSqr;
                    }
                }
            } else {  // region 3
                // u = 0.0f;
                if ( b1 >= 0.0f ) {
                    // v = 0.0f;
                    distSqr = vecDistSqr;
                } else if ( -b1 >= a11 ) {
                    // v = 1.0f;
                    distSqr = a11+2.0f*b1+vecDistSqr;
                } else {
                    v = -b1 / a11;
                    distSqr = b1*v+vecDistSqr;
                }
            }
        } else if ( v < 0.0f ) {  // region 5
                                  // v = 0.0f;
            if ( b0 >= 0.0f ) {
                // u = 0.0f;
                distSqr = vecDistSqr;
            } else if ( -b0 >= a00 ) {
                // u = 1.0f;
                distSqr = a00+2.0f*b0+vecDistSqr;
            } else {
                u = -b0/a00;
                distSqr = b0*u+vecDistSqr;
            }
        } else {  // region 0
            // minimum at interior point
            if ( det == 0.0f ) {
                // u = 0.0f;
                // v = 0.0f;
                distSqr = Math::MaxValue< float >();
            } else {
                float invDet = 1.0f / det;
                u *= invDet;
                v *= invDet;
                distSqr = u*(a00*u+a01*v+2.0f*b0) + v*(a01*u+a11*v+2.0f*b1)+vecDistSqr;
            }
        }
    } else {
        float tmp0, tmp1, num, denom;

        if ( u < 0.0f ) {  // region 2
            tmp0 = a01 + b0;
            tmp1 = a11 + b1;
            if ( tmp1 > tmp0 ) {
                num = tmp1 - tmp0;
                denom = a00-2.0f*a01+a11;
                if ( num >= denom ) {
                    // u = 1.0f;
                    // v = 0.0f;
                    distSqr = a00+2.0f*b0+vecDistSqr;
                } else {
                    u = num/denom;
                    v = 1.0f - u;
                    distSqr = u*(a00*u+a01*v+2.0f*b0) + v*(a01*u+a11*v+2.0f*b1)+vecDistSqr;
                }
            } else {
                // u = 0.0f;
                if ( tmp1 <= 0.0f ) {
                    // v = 1.0f;
                    distSqr = a11+2.0f*b1+vecDistSqr;
                } else if ( b1 >= 0.0f ) {
                    // v = 0.0f;
                    distSqr = vecDistSqr;
                } else {
                    v = -b1/a11;
                    distSqr = b1*v+vecDistSqr;
                }
            }
        } else if ( v < 0.0f ) {  // region 6
            tmp0 = a01 + b1;
            tmp1 = a00 + b0;
            if ( tmp1 > tmp0 ) {
                num = tmp1 - tmp0;
                denom = a00-2.0f*a01+a11;
                if ( num >= denom ) {
                    // v = 1.0f;
                    // u = 0.0f;
                    distSqr = a11+2.0f*b1+vecDistSqr;
                } else {
                    v = num/denom;
                    u = 1.0f - v;
                    distSqr = u*(a00*u+a01*v+2.0f*b0) + v*(a01*u+a11*v+2.0f*b1)+vecDistSqr;
                }
            } else {
                // v = 0.0f;
                if ( tmp1 <= 0.0f ) {
                    // u = 1.0f;
                    distSqr = a00+2.0f*b0+vecDistSqr;
                } else if ( b0 >= 0.0f ) {
                    // u = 0.0f;
                    distSqr = vecDistSqr;
                } else {
                    u = -b0/a00;
                    distSqr = b0*u+vecDistSqr;
                }
            }
        } else {  // region 1
            num = a11 + b1 - a01 - b0;
            if ( num <= 0.0f ) {
                // u = 0.0f;
                // v = 1.0f;
                distSqr = a11+2.0f*b1+vecDistSqr;
            } else {
                denom = a00-2.0f*a01+a11;
                if ( num >= denom ) {
                    // u = 1.0f;
                    // v = 0.0f;
                    distSqr = a00+2.0f*b0+vecDistSqr;
                } else {
                    u = num/denom;
                    v = 1.0f - u;
                    distSqr = u*(a00*u+a01*v+2.0f*b0) + v*(a01*u+a11*v+2.0f*b1)+vecDistSqr;
                }
            }
        }
    }

    return Math::Abs( distSqr ) < radiusSqr;
}

/** Sphere - Plane */
AN_INLINE bool BvSphereOverlapPlane( BvSphere const & _Sphere, PlaneF const & _Plane ) {
    float dist = _Plane.Dist( _Sphere.Center );
    if ( dist > _Sphere.Radius || dist < -_Sphere.Radius ) {
        return false;
    }
    return true;
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

/** AABB - AABB */
AN_INLINE bool BvBoxOverlapBox( BvAxisAlignedBox const & _AABB1, BvAxisAlignedBox const & _AABB2 ) {
    if ( _AABB1.Maxs[0] < _AABB2.Mins[0] || _AABB1.Mins[0] > _AABB2.Maxs[0] ) return false;
    if ( _AABB1.Maxs[1] < _AABB2.Mins[1] || _AABB1.Mins[1] > _AABB2.Maxs[1] ) return false;
    if ( _AABB1.Maxs[2] < _AABB2.Mins[2] || _AABB1.Mins[2] > _AABB2.Maxs[2] ) return false;
    return true;
}

/** AABB - AABB (2D) */
AN_INLINE bool BvBoxOverlapBox2D( Float2 const & _AABB1Mins, Float2 const & _AABB1Maxs, Float2 const & _AABB2Mins, Float2 const & _AABB2Maxs ) {
    if ( _AABB1Maxs[ 0 ] < _AABB2Mins[ 0 ] || _AABB1Mins[ 0 ] > _AABB2Maxs[ 0 ] ) return false;
    if ( _AABB1Maxs[ 1 ] < _AABB2Mins[ 1 ] || _AABB1Mins[ 1 ] > _AABB2Maxs[ 1 ] ) return false;
    return true;
}

/** AABB - Point */
AN_INLINE bool BvBoxOverlapPoint( BvAxisAlignedBox const & _AABB, Float3 const & _Point ) {
    if (    _Point.X < _AABB.Mins.X
         || _Point.Y < _AABB.Mins.Y
         || _Point.Z < _AABB.Mins.Z
         || _Point.X > _AABB.Maxs.X
         || _Point.Y > _AABB.Maxs.Y
         || _Point.Z > _AABB.Maxs.Z )
    {
        return false;
    }
    return true;
}

/** AABB - Sphere */
AN_INLINE bool BvBoxOverlapSphere( BvAxisAlignedBox const & _AABB, BvSphere const & _Sphere ) {
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

/** AABB - Triangle */
AN_INLINE bool BvBoxOverlapTriangle( BvAxisAlignedBox const & _AABB, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {
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
    n = Math::Cross( edge0, edge1 );

    if ( Math::Abs( Math::Dot( n, distVec ) ) > halfSize.X * Math::Abs( n.X ) + halfSize.Y * Math::Abs( n.Y ) + halfSize.Z * Math::Abs( n.Z ) ) {
        return false;
    }

    for ( int i = 0; i < 3; i++ ) {
        p = distVec[i];
        d0 = edge0[i];
        d1 = edge1[i];
        radius = halfSize[ i ];

        if ( Math::Min( p, Math::Min( p + d0, p + d1 ) ) > radius || Math::Max( p, Math::Max( p + d0, p + d1 ) ) < -radius ) {
            return false;
        }
    }

    // check axisX and edge0
    n = Float3( 0, -edge0.Z, edge0.Y );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge1 );

    radius = halfSize[ 1 ] * Math::Abs( edge0[2] ) + halfSize[ 2 ] * Math::Abs( edge0[1] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge1
    n = Float3( 0, -edge1.Z, edge1.Y );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = halfSize[ 1 ] * Math::Abs( edge1[2] ) + halfSize[ 2 ] * Math::Abs( edge1[1] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge2
    n = Float3( 0, -edge2.Z, edge2.Y );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = halfSize[ 1 ] * Math::Abs( edge2[2] ) + halfSize[ 2 ] * Math::Abs( edge2[1] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge0
    n = Float3( edge0.Z, 0, -edge0.X );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge1 );

    radius = halfSize[ 0 ] * Math::Abs( edge0[2] ) + halfSize[ 2 ] * Math::Abs( edge0[0] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge1
    n = Float3( edge1.Z, 0, -edge1.X );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = halfSize[ 0 ] * Math::Abs( edge1[2] ) + halfSize[ 2 ] * Math::Abs( edge1[0] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge2
    n = Float3( edge2.Z, 0, -edge2.X );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = halfSize[ 0 ] * Math::Abs( edge2[2] ) + halfSize[ 2 ] * Math::Abs( edge2[0] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge0
    n = Float3( -edge0.Y, edge0.X, 0 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge1 );

    radius = halfSize[ 0 ] * Math::Abs( edge0[1] ) + halfSize[ 1 ] * Math::Abs( edge0[0] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge1
    n = Float3( -edge1.Y, edge1.X, 0 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = halfSize[ 0 ] * Math::Abs( edge1[1] ) + halfSize[ 1 ] * Math::Abs( edge1[0] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge2
    n = Float3( -edge2.Y, edge2.X, 0 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = halfSize[ 0 ] * Math::Abs( edge2[1] ) + halfSize[ 1 ] * Math::Abs( edge2[0] );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    return true;
}

/** AABB - Triangle (approximation) */
AN_INLINE bool BvBoxOverlapTriangle_FastApproximation( BvAxisAlignedBox const & _BoundingBox, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {

    // Simple fast triangle - AABB overlap test

    BvAxisAlignedBox triangleBounds;

    triangleBounds.Mins.X = Math::Min( _P0.X, _P1.X, _P2.X );
    triangleBounds.Mins.Y = Math::Min( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Mins.Z = Math::Min( _P0.Z, _P1.Z, _P2.Z );

    triangleBounds.Maxs.X = Math::Max( _P0.X, _P1.X, _P2.X );
    triangleBounds.Maxs.Y = Math::Max( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Maxs.Z = Math::Max( _P0.Z, _P1.Z, _P2.Z );

    return BvBoxOverlapBox( _BoundingBox, triangleBounds );
}

/** AABB intersection box */
AN_INLINE bool BvGetBoxIntersection( BvAxisAlignedBox const & _A, BvAxisAlignedBox const & _B, BvAxisAlignedBox & _Intersection ) {
    const float x_min = Math::Max( _A.Mins[ 0 ], _B.Mins[ 0 ] );
    const float x_max = Math::Min( _A.Maxs[ 0 ], _B.Maxs[ 0 ] );
    if ( x_max <= x_min ) {
        return false;
    }

    const float y_min = Math::Max( _A.Mins[ 1 ], _B.Mins[ 1 ] );
    const float y_max = Math::Min( _A.Maxs[ 1 ], _B.Maxs[ 1 ] );
    if ( y_max <= y_min ) {
        return false;
    }

    const float z_min = Math::Max( _A.Mins[ 2 ], _B.Mins[ 2 ] );
    const float z_max = Math::Min( _A.Maxs[ 2 ], _B.Maxs[ 2 ] );
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

/** AABB overlap convex volume */
AN_INLINE bool BvBoxOverlapConvex( BvAxisAlignedBox const & _AABB, PlaneF const * _Planes, int _PlaneCount ) {
    for ( int i = 0 ; i < _PlaneCount ; i++ ) {
        PlaneF const & plane = _Planes[i];

        if ( plane.Dist( { plane.Normal.X > 0.0f ? _AABB.Mins.X : _AABB.Maxs.X,
                           plane.Normal.Y > 0.0f ? _AABB.Mins.Y : _AABB.Maxs.Y,
                           plane.Normal.Z > 0.0f ? _AABB.Mins.Z : _AABB.Maxs.Z
                         } ) > 0 )
        {
            return false;
        }
    }
    return true;
}

/** AABB inside convex volume */
AN_INLINE bool BvBoxInsideConvex( BvAxisAlignedBox const & _AABB, PlaneF const * _Planes, int _PlaneCount ) {
    for ( int i = 0 ; i < _PlaneCount ; i++ ) {
        PlaneF const & plane = _Planes[i];

        if ( plane.Dist( { plane.Normal.X < 0.0f ? _AABB.Mins.X : _AABB.Maxs.X,
                           plane.Normal.Y < 0.0f ? _AABB.Mins.Y : _AABB.Maxs.Y,
                           plane.Normal.Z < 0.0f ? _AABB.Mins.Z : _AABB.Maxs.Z
                         } ) > 0 )
        {
            return false;
        }
    }
    return true;
}

/** AABB overlap plane */
AN_INLINE bool BvBoxOverlapPlane( Float3 const * _BoxVertices, PlaneF const & _Plane ) {
    bool front = false;
    bool back = false;

    for ( int i = 0 ; i < 8 ; i++ )
    {
        float dist = _Plane.Dist( _BoxVertices[i] );
        if ( dist > 0 ) {
            front = true;
        } else {
            back = true;
        }
    }

    return front && back;
}

/** AABB overlap plane */
AN_INLINE int BvBoxOverlapPlaneSideMask( Float3 const & _Mins, Float3 const & _Maxs, PlaneF const & _Plane ) {
    int sideMask = 0;

    if ( _Plane.Normal[0] * _Mins[0] + _Plane.Normal[1] * _Mins[1] + _Plane.Normal[2] * _Mins[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;
    if ( _Plane.Normal[0] * _Maxs[0] + _Plane.Normal[1] * _Mins[1] + _Plane.Normal[2] * _Mins[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;
    if ( _Plane.Normal[0] * _Mins[0] + _Plane.Normal[1] * _Maxs[1] + _Plane.Normal[2] * _Mins[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;
    if ( _Plane.Normal[0] * _Maxs[0] + _Plane.Normal[1] * _Maxs[1] + _Plane.Normal[2] * _Mins[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;
    if ( _Plane.Normal[0] * _Mins[0] + _Plane.Normal[1] * _Mins[1] + _Plane.Normal[2] * _Maxs[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;
    if ( _Plane.Normal[0] * _Maxs[0] + _Plane.Normal[1] * _Mins[1] + _Plane.Normal[2] * _Maxs[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;
    if ( _Plane.Normal[0] * _Mins[0] + _Plane.Normal[1] * _Maxs[1] + _Plane.Normal[2] * _Maxs[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;
    if ( _Plane.Normal[0] * _Maxs[0] + _Plane.Normal[1] * _Maxs[1] + _Plane.Normal[2] * _Maxs[2] + _Plane.D > 0 ) sideMask |= 1; else sideMask |= 2;

    return sideMask;
}

/** AABB overlap plane */
AN_INLINE bool BvBoxOverlapPlane( Float3 const & _Mins, Float3 const & _Maxs, PlaneF const & _Plane ) {
    return BvBoxOverlapPlaneSideMask( _Mins, _Maxs, _Plane ) == 3;
}

/** AABB overlap plane */
AN_INLINE bool BvBoxOverlapPlane( BvAxisAlignedBox const & _AABB, PlaneF const & _Plane ) {
    return BvBoxOverlapPlane( _AABB.Mins, _AABB.Maxs, _Plane );
}

/** AABB overlap plane based on precomputed plane axial type and plane sign bits */
AN_INLINE bool BvBoxOverlapPlaneFast( BvAxisAlignedBox const & _AABB, PlaneF const & _Plane, int _AxialType, int _SignBits ) {
    const float dist = _Plane.Dist();

    if ( _AxialType < 3 ) {
        if ( dist < _AABB.Mins[_AxialType] ) {
            return false;
        }
        if ( dist > _AABB.Maxs[_AxialType] ) {
            return false;
        }
        return true;
    }

    float d1, d2;
    switch ( _SignBits ) {
    case 0:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 1:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 2:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 3:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 4:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    case 5:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    case 6:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    case 7:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    default:
        d1 = d2 = 0;
        break;
    }

    return ( d1 >= dist ) && ( d2 < dist );
}

/** AABB overlap plane based on precomputed plane axial type and plane sign bits */
AN_INLINE int BvBoxOverlapPlaneSideMask( BvAxisAlignedBox const & _AABB, PlaneF const & _Plane, int _AxialType, int _SignBits ) {
    const float dist = _Plane.Dist();

    if ( _AxialType < 3 ) {
        if ( dist <= _AABB.Mins[_AxialType] ) {
            return 1;
        }
        if ( dist >= _AABB.Maxs[_AxialType] ) {
            return 2;
        }
        return 3;
    }

    float d1, d2;
    switch ( _SignBits ) {
    case 0:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 1:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 2:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 3:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        break;
    case 4:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    case 5:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    case 6:
        d1 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    case 7:
        d1 = _Plane.Normal[0]*_AABB.Mins[0] + _Plane.Normal[1]*_AABB.Mins[1] + _Plane.Normal[2]*_AABB.Mins[2];
        d2 = _Plane.Normal[0]*_AABB.Maxs[0] + _Plane.Normal[1]*_AABB.Maxs[1] + _Plane.Normal[2]*_AABB.Maxs[2];
        break;
    default:
        d1 = d2 = 0;
        break;
    }

    int sideMask = ( d1 >= dist );

    if ( d2 < dist ) {
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

/** OBB - OBB */
AN_INLINE bool BvOrientedBoxOverlapOrientedBox( BvOrientedBox const & _OBB1, BvOrientedBox const & _OBB2 ) {
    // Code based on http://gdlinks.hut.ru/cdfaq/obb.shtml

    const Float3x3 orientInversed = _OBB1.Orient.Transposed();

    // transform OBB2 position to OBB1 space
    const Float3 T = orientInversed * ( _OBB2.Center - _OBB1.Center );

    // transform OBB2 orientation to OBB1 space
    const Float3x3 R = orientInversed * _OBB2.Orient;

    float ra, rb;

    for ( int i = 0; i < 3; i++ ) {
        ra = _OBB1.HalfSize[ i ];
        rb = _OBB2.HalfSize[ 0 ] * Math::Abs( R[ i ][ 0 ] ) + _OBB2.HalfSize[ 1 ] * Math::Abs( R[ i ][ 1 ] ) + _OBB2.HalfSize[ 2 ] * Math::Abs( R[ i ][ 2 ] );
        if ( Math::Abs( T[ i ] ) > ra + rb )
            return false;
    }

    for ( int i = 0; i < 3; i++ ) {
        ra = _OBB1.HalfSize[ 0 ] * Math::Abs( R[ 0 ][ i ] ) + _OBB1.HalfSize[ 1 ] * Math::Abs( R[ 1 ][ i ] ) + _OBB1.HalfSize[ 2 ] * Math::Abs( R[ 2 ][ i ] );
        rb = _OBB2.HalfSize[ i ];
        if ( Math::Abs( ( T[ 0 ] * R[ 0 ][ i ] + T[ 1 ] * R[ 1 ][ i ] + T[ 2 ] * R[ 2 ][ i ] ) ) > ra + rb )
            return false;
    }

    ra = _OBB1.HalfSize[ 1 ] * Math::Abs( R[ 2 ][ 0 ] ) + _OBB1.HalfSize[ 2 ] * Math::Abs( R[ 1 ][ 0 ] );
    rb = _OBB2.HalfSize[ 1 ] * Math::Abs( R[ 0 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * Math::Abs( R[ 0 ][ 1 ] );
    if ( Math::Abs( ( T[ 2 ] * R[ 1 ][ 0 ] - T[ 1 ] * R[ 2 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 1 ] * Math::Abs( R[ 2 ][ 1 ] ) + _OBB1.HalfSize[ 2 ] * Math::Abs( R[ 1 ][ 1 ] );
    rb = _OBB2.HalfSize[ 0 ] * Math::Abs( R[ 0 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * Math::Abs( R[ 0 ][ 0 ] );
    if ( Math::Abs( ( T[ 2 ] * R[ 1 ][ 1 ] - T[ 1 ] * R[ 2 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 1 ] * Math::Abs( R[ 2 ][ 2 ] ) + _OBB1.HalfSize[ 2 ] * Math::Abs( R[ 1 ][ 2 ] );
    rb = _OBB2.HalfSize[ 0 ] * Math::Abs( R[ 0 ][ 1 ] ) + _OBB2.HalfSize[ 1 ] * Math::Abs( R[ 0 ][ 0 ] );
    if ( Math::Abs( ( T[ 2 ] * R[ 1 ][ 2 ] - T[ 1 ] * R[ 2 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * Math::Abs( R[ 2 ][ 0 ] ) + _OBB1.HalfSize[ 2 ] * Math::Abs( R[ 0 ][ 0 ] );
    rb = _OBB2.HalfSize[ 1 ] * Math::Abs( R[ 1 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * Math::Abs( R[ 1 ][ 1 ] );
    if ( Math::Abs( ( T[ 0 ] * R[ 2 ][ 0 ] - T[ 2 ] * R[ 0 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * Math::Abs( R[ 2 ][ 1 ] ) + _OBB1.HalfSize[ 2 ] * Math::Abs( R[ 0 ][ 1 ] );
    rb = _OBB2.HalfSize[ 0 ] * Math::Abs( R[ 1 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * Math::Abs( R[ 1 ][ 0 ] );
    if ( Math::Abs( ( T[ 0 ] * R[ 2 ][ 1 ] - T[ 2 ] * R[ 0 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * Math::Abs( R[ 2 ][ 2 ] ) + _OBB1.HalfSize[ 2 ] * Math::Abs( R[ 0 ][ 2 ] );
    rb = _OBB2.HalfSize[ 0 ] * Math::Abs( R[ 1 ][ 1 ] ) + _OBB2.HalfSize[ 1 ] * Math::Abs( R[ 1 ][ 0 ] );
    if ( Math::Abs( ( T[ 0 ] * R[ 2 ][ 2 ] - T[ 2 ] * R[ 0 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * Math::Abs( R[ 1 ][ 0 ] ) + _OBB1.HalfSize[ 1 ] * Math::Abs( R[ 0 ][ 0 ] );
    rb = _OBB2.HalfSize[ 1 ] * Math::Abs( R[ 2 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * Math::Abs( R[ 2 ][ 1 ] );
    if ( Math::Abs( ( T[ 1 ] * R[ 0 ][ 0 ] - T[ 0 ] * R[ 1 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * Math::Abs( R[ 1 ][ 1 ] ) + _OBB1.HalfSize[ 1 ] * Math::Abs( R[ 0 ][ 1 ] );
    rb = _OBB2.HalfSize[ 0 ] * Math::Abs( R[ 2 ][ 2 ] ) + _OBB2.HalfSize[ 2 ] * Math::Abs( R[ 2 ][ 0 ] );
    if ( Math::Abs( ( T[ 1 ] * R[ 0 ][ 1 ] - T[ 0 ] * R[ 1 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _OBB1.HalfSize[ 0 ] * Math::Abs( R[ 1 ][ 2 ] ) + _OBB1.HalfSize[ 1 ] * Math::Abs( R[ 0 ][ 2 ] );
    rb = _OBB2.HalfSize[ 0 ] * Math::Abs( R[ 2 ][ 1 ] ) + _OBB2.HalfSize[ 1 ] * Math::Abs( R[ 2 ][ 0 ] );
    if ( Math::Abs( ( T[ 1 ] * R[ 0 ][ 2 ] - T[ 0 ] * R[ 1 ][ 2 ] ) ) > ra + rb )
        return false;

    return true;
}

/** OBB - Sphere */
AN_INLINE bool BvOrientedBoxOverlapSphere( BvOrientedBox const & _OBB, BvSphere const & _Sphere ) {

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

/** OBB - AABB */
AN_INLINE bool BvOrientedBoxOverlapBox( BvOrientedBox const & _OBB, Float3 const & _AABBCenter, Float3 const & _AABBHalfSize ) {
    // transform OBB position to AABB space
    const Float3 T = _OBB.Center - _AABBCenter;

    // OBB orientation relative to AABB space
    const Float3x3 & R = _OBB.Orient;

    float ra, rb;

    for ( int i = 0; i < 3; i++ ) {
        ra = _AABBHalfSize[ i ];
        rb = _OBB.HalfSize[ 0 ] * Math::Abs( R[ i ][ 0 ] ) + _OBB.HalfSize[ 1 ] * Math::Abs( R[ i ][ 1 ] ) + _OBB.HalfSize[ 2 ] * Math::Abs( R[ i ][ 2 ] );
        if ( Math::Abs( T[ i ] ) > ra + rb )
            return false;
    }

    for ( int i = 0; i < 3; i++ ) {
        ra = _AABBHalfSize[ 0 ] * Math::Abs( R[ 0 ][ i ] ) + _AABBHalfSize[ 1 ] * Math::Abs( R[ 1 ][ i ] ) + _AABBHalfSize[ 2 ] * Math::Abs( R[ 2 ][ i ] );
        rb = _OBB.HalfSize[ i ];
        if ( Math::Abs( ( T[ 0 ] * R[ 0 ][ i ] + T[ 1 ] * R[ 1 ][ i ] + T[ 2 ] * R[ 2 ][ i ] ) ) > ra + rb )
            return false;
    }

    ra = _AABBHalfSize[ 1 ] * Math::Abs( R[ 2 ][ 0 ] ) + _AABBHalfSize[ 2 ] * Math::Abs( R[ 1 ][ 0 ] );
    rb = _OBB.HalfSize[ 1 ] * Math::Abs( R[ 0 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * Math::Abs( R[ 0 ][ 1 ] );
    if ( Math::Abs( ( T[ 2 ] * R[ 1 ][ 0 ] - T[ 1 ] * R[ 2 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 1 ] * Math::Abs( R[ 2 ][ 1 ] ) + _AABBHalfSize[ 2 ] * Math::Abs( R[ 1 ][ 1 ] );
    rb = _OBB.HalfSize[ 0 ] * Math::Abs( R[ 0 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * Math::Abs( R[ 0 ][ 0 ] );
    if ( Math::Abs( ( T[ 2 ] * R[ 1 ][ 1 ] - T[ 1 ] * R[ 2 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 1 ] * Math::Abs( R[ 2 ][ 2 ] ) + _AABBHalfSize[ 2 ] * Math::Abs( R[ 1 ][ 2 ] );
    rb = _OBB.HalfSize[ 0 ] * Math::Abs( R[ 0 ][ 1 ] ) + _OBB.HalfSize[ 1 ] * Math::Abs( R[ 0 ][ 0 ] );
    if ( Math::Abs( ( T[ 2 ] * R[ 1 ][ 2 ] - T[ 1 ] * R[ 2 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * Math::Abs( R[ 2 ][ 0 ] ) + _AABBHalfSize[ 2 ] * Math::Abs( R[ 0 ][ 0 ] );
    rb = _OBB.HalfSize[ 1 ] * Math::Abs( R[ 1 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * Math::Abs( R[ 1 ][ 1 ] );
    if ( Math::Abs( ( T[ 0 ] * R[ 2 ][ 0 ] - T[ 2 ] * R[ 0 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * Math::Abs( R[ 2 ][ 1 ] ) + _AABBHalfSize[ 2 ] * Math::Abs( R[ 0 ][ 1 ] );
    rb = _OBB.HalfSize[ 0 ] * Math::Abs( R[ 1 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * Math::Abs( R[ 1 ][ 0 ] );
    if ( Math::Abs( ( T[ 0 ] * R[ 2 ][ 1 ] - T[ 2 ] * R[ 0 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * Math::Abs( R[ 2 ][ 2 ] ) + _AABBHalfSize[ 2 ] * Math::Abs( R[ 0 ][ 2 ] );
    rb = _OBB.HalfSize[ 0 ] * Math::Abs( R[ 1 ][ 1 ] ) + _OBB.HalfSize[ 1 ] * Math::Abs( R[ 1 ][ 0 ] );
    if ( Math::Abs( ( T[ 0 ] * R[ 2 ][ 2 ] - T[ 2 ] * R[ 0 ][ 2 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * Math::Abs( R[ 1 ][ 0 ] ) + _AABBHalfSize[ 1 ] * Math::Abs( R[ 0 ][ 0 ] );
    rb = _OBB.HalfSize[ 1 ] * Math::Abs( R[ 2 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * Math::Abs( R[ 2 ][ 1 ] );
    if ( Math::Abs( ( T[ 1 ] * R[ 0 ][ 0 ] - T[ 0 ] * R[ 1 ][ 0 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * Math::Abs( R[ 1 ][ 1 ] ) + _AABBHalfSize[ 1 ] * Math::Abs( R[ 0 ][ 1 ] );
    rb = _OBB.HalfSize[ 0 ] * Math::Abs( R[ 2 ][ 2 ] ) + _OBB.HalfSize[ 2 ] * Math::Abs( R[ 2 ][ 0 ] );
    if ( Math::Abs( ( T[ 1 ] * R[ 0 ][ 1 ] - T[ 0 ] * R[ 1 ][ 1 ] ) ) > ra + rb )
        return false;

    ra = _AABBHalfSize[ 0 ] * Math::Abs( R[ 1 ][ 2 ] ) + _AABBHalfSize[ 1 ] * Math::Abs( R[ 0 ][ 2 ] );
    rb = _OBB.HalfSize[ 0 ] * Math::Abs( R[ 2 ][ 1 ] ) + _OBB.HalfSize[ 1 ] * Math::Abs( R[ 2 ][ 0 ] );
    if ( Math::Abs( ( T[ 1 ] * R[ 0 ][ 2 ] - T[ 0 ] * R[ 1 ][ 2 ] ) ) > ra + rb )
        return false;

    return true;
}

/** OBB - AABB */
AN_INLINE bool BvOrientedBoxOverlapBox( BvOrientedBox const & _OBB, BvAxisAlignedBox const & _AABB ) {
    return BvOrientedBoxOverlapBox( _OBB, _AABB.Center(), _AABB.HalfSize() );
}

/** OBB - Triangle */
AN_INLINE bool BvOrientedBoxOverlapTriangle( BvOrientedBox const & _OBB, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {

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
    n = Math::Cross( edge0, edge1 );

    if ( Math::Abs( Math::Dot( n, distVec ) ) > ( _OBB.HalfSize.X * Math::Abs( Math::Dot( n, _OBB.Orient[ 0 ] ) ) + _OBB.HalfSize.Y * Math::Abs( Math::Dot( n, _OBB.Orient[ 1 ] ) ) + _OBB.HalfSize.Z * Math::Abs( Math::Dot( n, _OBB.Orient[ 2 ] ) ) ) )
        return false;

    for ( int i = 0; i < 3; i++ ) {
        p = Math::Dot( _OBB.Orient[ i ], distVec );
        d0 = Math::Dot( _OBB.Orient[ i ], edge0 );
        d1 = Math::Dot( _OBB.Orient[ i ], edge1 );
        radius = _OBB.HalfSize[ i ];

        if ( Math::Min( p, Math::Min( p + d0, p + d1 ) ) > radius || Math::Max( p, Math::Max( p + d0, p + d1 ) ) < -radius ) {
            return false;
        }
    }

    // check axisX and edge0
    n = Math::Cross( _OBB.Orient[ 0 ], edge0 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge1 );

    radius = _OBB.HalfSize[ 1 ] * Math::Abs( Math::Dot( _OBB.Orient[ 2 ], edge0 ) ) + _OBB.HalfSize[ 2 ] * Math::Abs( Math::Dot( _OBB.Orient[ 1 ], edge0 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge1
    n = Math::Cross( _OBB.Orient[ 0 ], edge1 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = _OBB.HalfSize[ 1 ] * Math::Abs( Math::Dot( _OBB.Orient[ 2 ], edge1 ) ) + _OBB.HalfSize[ 2 ] * Math::Abs( Math::Dot( _OBB.Orient[ 1 ], edge1 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisX and edge2
    n = Math::Cross( _OBB.Orient[ 0 ], edge2 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = _OBB.HalfSize[ 1 ] * Math::Abs( Math::Dot( _OBB.Orient[ 2 ], edge2 ) ) + _OBB.HalfSize[ 2 ] * Math::Abs( Math::Dot( _OBB.Orient[ 1 ], edge2 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge0
    n = Math::Cross( _OBB.Orient[ 1 ], edge0 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge1 );

    radius = _OBB.HalfSize[ 0 ] * Math::Abs( Math::Dot( _OBB.Orient[ 2 ], edge0 ) ) + _OBB.HalfSize[ 2 ] * Math::Abs( Math::Dot( _OBB.Orient[ 0 ], edge0 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge1
    n = Math::Cross( _OBB.Orient[ 1 ], edge1 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = _OBB.HalfSize[ 0 ] * Math::Abs( Math::Dot( _OBB.Orient[ 2 ], edge1 ) ) + _OBB.HalfSize[ 2 ] * Math::Abs( Math::Dot( _OBB.Orient[ 0 ], edge1 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisY and edge2
    n = Math::Cross( _OBB.Orient[ 1 ], edge2 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = _OBB.HalfSize[ 0 ] * Math::Abs( Math::Dot( _OBB.Orient[ 2 ], edge2 ) ) + _OBB.HalfSize[ 2 ] * Math::Abs( Math::Dot( _OBB.Orient[ 0 ], edge2 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge0
    n = Math::Cross( _OBB.Orient[ 2 ], edge0 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge1 );

    radius = _OBB.HalfSize[ 0 ] * Math::Abs( Math::Dot( _OBB.Orient[ 1 ], edge0 ) ) + _OBB.HalfSize[ 1 ] * Math::Abs( Math::Dot( _OBB.Orient[ 0 ], edge0 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge1
    n = Math::Cross( _OBB.Orient[ 2 ], edge1 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = _OBB.HalfSize[ 0 ] * Math::Abs( Math::Dot( _OBB.Orient[ 1 ], edge1 ) ) + _OBB.HalfSize[ 1 ] * Math::Abs( Math::Dot( _OBB.Orient[ 0 ], edge1 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    // check axisZ and edge2
    n = Math::Cross( _OBB.Orient[ 2 ], edge2 );
    p = Math::Dot( n, distVec );
    d0 = Math::Dot( n, edge0 );

    radius = _OBB.HalfSize[ 0 ] * Math::Abs( Math::Dot( _OBB.Orient[ 1 ], edge2 ) ) + _OBB.HalfSize[ 1 ] * Math::Abs( Math::Dot( _OBB.Orient[ 0 ], edge2 ) );

    if ( Math::Min( p, p + d0 ) > radius || Math::Max( p, p + d0 ) < -radius )
        return false;

    return true;
}

/** OBB - Triangle (approximation) */
AN_INLINE bool BvOrientedBoxOverlapTriangle_FastApproximation( BvOrientedBox const & _OBB, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {

    // Simple fast triangle - AABB overlap test

    BvAxisAlignedBox triangleBounds;

    triangleBounds.Mins.X = Math::Min( _P0.X, _P1.X, _P2.X );
    triangleBounds.Mins.Y = Math::Min( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Mins.Z = Math::Min( _P0.Z, _P1.Z, _P2.Z );

    triangleBounds.Maxs.X = Math::Max( _P0.X, _P1.X, _P2.X );
    triangleBounds.Maxs.Y = Math::Max( _P0.Y, _P1.Y, _P2.Y );
    triangleBounds.Maxs.Z = Math::Max( _P0.Z, _P1.Z, _P2.Z );

    return BvOrientedBoxOverlapBox( _OBB, triangleBounds );
}

/** OBB overlap convex */
AN_INLINE bool BvOrientedBoxOverlapConvex( BvOrientedBox const & b, PlaneF const * _Planes, int _PlaneCount ) {
    Float3 point;

    for ( int i = 0 ; i < _PlaneCount ; i++ ) {
        PlaneF const & plane = _Planes[i];

        const float x = Math::Dot( b.Orient[ 0 ], plane.Normal ) > 0.0f ? -b.HalfSize[ 0 ] : b.HalfSize[ 0 ];
        const float y = Math::Dot( b.Orient[ 1 ], plane.Normal ) > 0.0f ? -b.HalfSize[ 1 ] : b.HalfSize[ 1 ];
        const float z = Math::Dot( b.Orient[ 2 ], plane.Normal ) > 0.0f ? -b.HalfSize[ 2 ] : b.HalfSize[ 2 ];

        point = b.Center + ( x*b.Orient[ 0 ] + y*b.Orient[ 1 ] + z*b.Orient[ 2 ] );

        if ( plane.Dist( point ) > 0.0f ) {
            return false;
        }
    }

    return true;
}

/** OBB inside convex */
AN_INLINE bool BvOrientedBoxInsideConvex( BvOrientedBox const & b, PlaneF const * _Planes, int _PlaneCount ) {
    Float3 point;

    for ( int i = 0 ; i < _PlaneCount ; i++ ) {
        PlaneF const & plane = _Planes[i];

        const float x = Math::Dot( b.Orient[ 0 ], plane.Normal ) < 0.0f ? -b.HalfSize[ 0 ] : b.HalfSize[ 0 ];
        const float y = Math::Dot( b.Orient[ 1 ], plane.Normal ) < 0.0f ? -b.HalfSize[ 1 ] : b.HalfSize[ 1 ];
        const float z = Math::Dot( b.Orient[ 2 ], plane.Normal ) < 0.0f ? -b.HalfSize[ 2 ] : b.HalfSize[ 2 ];

        point = b.Center + ( x*b.Orient[ 0 ] + y*b.Orient[ 1 ] + z*b.Orient[ 2 ] );

        if ( plane.Dist( point ) > 0.0f ) {
            return false;
        }
    }

    return true;
}

/** OBB overlap plane */
AN_INLINE bool BvOrientedBoxOverlapPlane( BvOrientedBox const & _OBB, PlaneF const & _Plane ) {
    Float3 vertices[8];

    _OBB.GetVertices( vertices );

    return BvBoxOverlapPlane( vertices, _Plane );
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

/** Ray - Sphere */
AN_INLINE bool BvRayIntersectSphere( Float3 const & _RayStart, Float3 const & _RayDir, BvSphere const & _Sphere, float & _Min, float & _Max ) {
    const Float3 k = _RayStart - _Sphere.Center;
    const float b = Math::Dot( k, _RayDir );
    float distance = b * b - k.LengthSqr() + _Sphere.Radius * _Sphere.Radius;
    if ( distance < 0.0f ) {
        return false;
    }
    distance = Math::Sqrt( distance );
    Math::MinMax( -b + distance, -b - distance, _Min, _Max );
    return _Min > 0.0f || _Max > 0.0f;
}

/** Ray - Sphere */
AN_INLINE bool BvRayIntersectSphere( Float3 const & _RayStart, Float3 const & _RayDir, BvSphere const & _Sphere, float & _Distance ) {
    const Float3 k = _RayStart - _Sphere.Center;
    const float b = Math::Dot( k, _RayDir );
    _Distance = b * b - k.LengthSqr() + _Sphere.Radius * _Sphere.Radius;
    if ( _Distance < 0.0f ) {
        return false;
    }
    float t1, t2;
    _Distance = Math::Sqrt( _Distance );
    Math::MinMax( -b + _Distance, -b - _Distance, t1, t2 );
    _Distance = t1 >= 0.0f ? t1 : t2;
    return _Distance > 0.0f;
}

/** Ray - AABB
If raydir is normalized, min and max will be in rage [0,raylength]
If raydir is non-normalized, min and max will be in rage [0,1] */
AN_INLINE bool BvRayIntersectBox( Float3 const & _RayStart, Float3 const & _InvRayDir, BvAxisAlignedBox const & _AABB, float & _Min, float & _Max ) {
#if 0
    float Lo = _InvRayDir.X*(_AABB.Mins.X - _RayStart.X);
    float Hi = _InvRayDir.X*(_AABB.Maxs.X - _RayStart.X);
    _Min = Math::Min(Lo, Hi);
    _Max = Math::Max(Lo, Hi);

    Lo = _InvRayDir.Y*(_AABB.Mins.Y - _RayStart.Y);
    Hi = _InvRayDir.Y*(_AABB.Maxs.Y - _RayStart.Y);
    _Min = Math::Max(_Min, Math::Min(Lo, Hi));
    _Max = Math::Min(_Max, Math::Max(Lo, Hi));

    Lo = _InvRayDir.Z*(_AABB.Mins.Z - _RayStart.Z);
    Hi = _InvRayDir.Z*(_AABB.Maxs.Z - _RayStart.Z);
    _Min = Math::Max(_Min, Math::Min(Lo, Hi));
    _Max = Math::Min(_Max, Math::Max(Lo, Hi));

    return (_Min <= _Max) && (_Max > 0.0f);
#else
    _Min = -Math::MaxValue< float >();
    _Max = Math::MaxValue< float >();

    for ( int i = 0; i < 3; i++ ) {
        // Check is ray axial
        if ( Math::IsInfinite( _InvRayDir[ i ] ) ) {
            if ( _RayStart[ i ] < _AABB.Mins[ i ] || _RayStart[ i ] > _AABB.Maxs[ i ] ) {
                // ray origin must be within the bounds
                return false;
            }
        } else {
            float lo = _InvRayDir[ i ] * ( _AABB.Mins[ i ] - _RayStart[ i ] );
            float hi = _InvRayDir[ i ] * ( _AABB.Maxs[ i ] - _RayStart[ i ] );
            if ( lo > hi ) {
                float tmp = lo;
                lo = hi;
                hi = tmp;
            }
            if ( lo > _Min ) {
                _Min = lo;
            }
            if ( hi < _Max ) {
                _Max = hi;
            }
            if ( _Min > _Max            // Ray doesn't intersect AABB
                || _Max <= 0.0f ) {     // or AABB is behind ray origin                
                return false;
            }
        }
    }
    return true;
#endif
}

/** Ray - AABB2D
If raydir is normalized, min and max will be in rage [0,raylength]
If raydir is non-normalized, min and max will be in rage [0,1] */
AN_INLINE bool BvRayIntersectBox2D( Float2 const & _RayStart, Float2 const & _InvRayDir, Float2 const & _Mins, Float2 const & _Maxs, float & _Min, float & _Max ) {
    _Min = -Math::MaxValue< float >();
    _Max = Math::MaxValue< float >();

    for ( int i = 0; i < 2; i++ ) {
        // Check is ray axial
        if ( Math::IsInfinite( _InvRayDir[ i ] ) ) {
            if ( _RayStart[ i ] < _Mins[ i ] || _RayStart[ i ] > _Maxs[ i ] ) {
                // ray origin must be within the bounds
                return false;
            }
        } else {
            float lo = _InvRayDir[ i ] * ( _Mins[ i ] - _RayStart[ i ] );
            float hi = _InvRayDir[ i ] * ( _Maxs[ i ] - _RayStart[ i ] );
            if ( lo > hi ) {
                float tmp = lo;
                lo = hi;
                hi = tmp;
            }
            if ( lo > _Min ) {
                _Min = lo;
            }
            if ( hi < _Max ) {
                _Max = hi;
            }
            if ( _Min > _Max            // Ray doesn't intersect AABB
                || _Max <= 0.0f ) {     // or AABB is behind ray origin                
                return false;
            }
        }
    }
    return true;
}

/** Ray - OBB
If raydir is normalized, min and max will be in rage [0,raylength]
If raydir is non-normalized, min and max will be in rage [0,1] */
AN_INLINE bool BvRayIntersectOrientedBox( Float3 const & _RayStart, Float3 const & _RayDir, BvOrientedBox const & _OBB, float & _Min, float & _Max ) {
    const Float3x3 OrientInversed = _OBB.Orient.Transposed();

    // Transform ray to OBB space
    const Float3 RayStart = OrientInversed * ( _RayStart - _OBB.Center );
    const Float3 RayDir = OrientInversed * _RayDir;

    // Mins and maxs in OBB space
    Float3 const Mins = -_OBB.HalfSize;
    Float3 const & Maxs = _OBB.HalfSize;

#if 0
    const float InvRayDirX = 1.0f / RayDir.X;
    const float InvRayDirY = 1.0f / RayDir.Y;
    const float InvRayDirZ = 1.0f / RayDir.Z;

    float Lo = InvRayDirX*(Mins.X - RayStart.X);
    float Hi = InvRayDirX*(Maxs.X - RayStart.X);
    _Min = Math::Min(Lo, Hi);
    _Max = Math::Max(Lo, Hi);

    Lo = InvRayDirY*(Mins.Y - RayStart.Y);
    Hi = InvRayDirY*(Maxs.Y - RayStart.Y);
    _Min = Math::Max(_Min, Math::Min(Lo, Hi));
    _Max = Math::Min(_Max, Math::Max(Lo, Hi));

    Lo = InvRayDirZ*(Mins.Z - RayStart.Z);
    Hi = InvRayDirZ*(Maxs.Z - RayStart.Z);
    _Min = Math::Max(_Min, Math::Min(Lo, Hi));
    _Max = Math::Min(_Max, Math::Max(Lo, Hi));

    return (_Min <= _Max) && (_Max > 0.0f);
#else
    _Min = -Math::MaxValue< float >();
    _Max = Math::MaxValue< float >();

    for ( int i = 0; i < 3; i++ ) {
        // Check is ray axial
        if ( fabsf( RayDir[ i ] ) < 1e-6f ) {
            if ( RayStart[ i ] < Mins[ i ] || RayStart[ i ] > Maxs[ i ] ) {
                // ray origin must be within the bounds
                return false;
            }
        } else {
            float invRayDir = 1.0f / RayDir[ i ];
            float lo = invRayDir * ( Mins[ i ] - RayStart[ i ] );
            float hi = invRayDir * ( Maxs[ i ] - RayStart[ i ] );
            if ( lo > hi ) {
                float tmp = lo;
                lo = hi;
                hi = tmp;
            }
            if ( lo > _Min ) {
                _Min = lo;
            }
            if ( hi < _Max ) {
                _Max = hi;
            }
            if ( _Min > _Max            // Ray doesn't intersect AABB
                || _Max <= 0.0f ) {     // or AABB is behind ray origin                
                return false;
            }
        }
    }
    return true;
#endif
}

/** Ray - triangle */
AN_INLINE bool BvRayIntersectTriangle( Float3 const & _RayStart, Float3 const & _RayDir, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2, float & _Distance, float & _U, float & _V, bool _CullBackFace = true ) {
    const Float3 e1 = _P1 - _P0;
    const Float3 e2 = _P2 - _P0;
    const Float3 h = Math::Cross( _RayDir, e2 );

    // calc determinant
    const float det = Math::Dot( e1, h );

    if ( _CullBackFace ) {
        if ( det < 0.00001 ) {
            return false;
        }
    } else {
        // ray lies in plane of triangle, so there is no intersection
        if ( det > -0.00001 && det < 0.00001 ) {
            return false;
        }
    }

    // calc inverse determinant to minimalize math divisions in next calculations
    const float invDet = 1 / det;

    // calc vector from ray origin to P0
    const Float3 s = _RayStart - _P0;

    // calc U
    _U = invDet * Math::Dot( s, h );
    if ( _U < 0.0f || _U > 1.0f ) {
        return false;
    }

    // calc perpendicular to compute V
    const Float3 q = Math::Cross( s, e1 );

    // calc V
    _V = invDet * Math::Dot( _RayDir, q );
    if ( _V < 0.0f || _U + _V > 1.0f ) {
        return false;
    }

    // compute distance to find out where the intersection point is on the line
    _Distance = invDet * Math::Dot( e2, q );

    // if _Distance < 0 this is a line intersection but not a ray intersection
    return _Distance > 0.0f;
}

/** Ray - Plane */
AN_INLINE bool BvRayIntersectPlane( Float3 const & _RayStart, Float3 const & _RayDir, PlaneF const & _Plane, float & _Distance ) {
    // Calculate distance from ray origin to plane
    const float d1 = Math::Dot( _RayStart, _Plane.Normal ) + _Plane.D;

    // Ray origin is on plane
    if ( d1 == 0.0f ) {
        _Distance = 0.0f;
        return true;
    }

    // Check ray direction
    const float d2 = Math::Dot( _Plane.Normal, _RayDir );
    if ( Math::Abs( d2 ) < 0.0001f ) {
        // ray is parallel
        return false;
    }

    // Calculate distance from ray origin to plane intersection
    _Distance = -( d1 / d2 );

    return _Distance >= 0.0f;
}

/** Ray - Plane */
AN_INLINE bool BvRayIntersectPlaneFront( Float3 const & _RayStart, Float3 const & _RayDir, PlaneF const & _Plane, float & _Distance ) {
    // Calculate distance from ray origin to plane
    const float d1 = Math::Dot( _RayStart, _Plane.Normal ) + _Plane.D;

    // Perform face culling
    if ( d1 < 0.0f ) {
        return false;
    }

    // Check ray direction
    const float d2 = Math::Dot( _Plane.Normal, _RayDir );
    if ( d2 >= 0.0f ) {
        // ray is parallel or has wrong direction
        return false;
    }

    // Calculate distance from ray origin to plane intersection
    _Distance = d1 / -d2;

    return true;
}

/** Ray - Plane */
AN_INLINE bool BvRayIntersectPlaneBack( Float3 const & _RayStart, Float3 const & _RayDir, PlaneF const & _Plane, float & _Distance ) {
    // Calculate distance from ray origin to plane
    const float d1 = Math::Dot( _RayStart, _Plane.Normal ) + _Plane.D;

    // Perform face culling
    if ( d1 > 0.0f ) {
        return false;
    }

    // Ray origin is on plane
    if ( d1 == 0.0f ) {
        _Distance = 0.0f;
        return true;
    }

    // Check ray direction
    const float d2 = Math::Dot( _Plane.Normal, _RayDir );
    if ( d2 <= 0.0f ) {
        // ray is parallel or has wrong direction
        return false;
    }

    // Calculate distance from ray origin to plane intersection
    _Distance = -d1 / d2;

    return true;
}

/** Ray - Elipsoid */
AN_INLINE bool BvRayIntersectElipsoid( Float3 const & _RayStart, Float3 const & _RayDir, float const & _Radius, float const & _MParam, float const & _NParam, float & _Min, float & _Max ) {
    const float a = _RayDir.X*_RayDir.X + _MParam*_RayDir.Y*_RayDir.Y + _NParam*_RayDir.Z*_RayDir.Z;
    const float b = 2.0f*( _RayStart.X*_RayDir.X + _MParam*_RayStart.Y*_RayDir.Y + _NParam*_RayStart.Z*_RayDir.Z );
    const float c = _RayStart.X*_RayStart.X + _MParam*_RayStart.Y*_RayStart.Y + _NParam*_RayStart.Z*_RayStart.Z - _Radius*_Radius;
    const float d = b*b - 4.0f*a*c;
    if ( d < 0.0f ) {
        return false;
    }
    const float distance = Math::Sqrt( d );
    const float denom = 0.5f / a;
    Math::MinMax( ( -b + distance ) * denom, ( -b - distance ) * denom, _Min, _Max );
    return _Min > 0.0f || _Max > 0.0f;
}

/** Ray - Elipsoid */
AN_INLINE bool BvRayIntersectElipsoid( Float3 const & _RayStart, Float3 const & _RayDir, float const & _Radius, float const & _MParam, float const & _NParam, float & _Distance ) {
    const float a = _RayDir.X*_RayDir.X + _MParam*_RayDir.Y*_RayDir.Y + _NParam*_RayDir.Z*_RayDir.Z;
    const float b = 2.0f*( _RayStart.X*_RayDir.X + _MParam*_RayStart.Y*_RayDir.Y + _NParam*_RayStart.Z*_RayDir.Z );
    const float c = _RayStart.X*_RayStart.X + _MParam*_RayStart.Y*_RayStart.Y + _NParam*_RayStart.Z*_RayStart.Z - _Radius*_Radius;
    const float d = b*b - 4.0f*a*c;
    if ( d < 0.0f ) {
        return false;
    }
    _Distance = Math::Sqrt( d );
    const float denom = 0.5f / a;
    float t1, t2;
    Math::MinMax( ( -b + _Distance ) * denom, ( -b - _Distance ) * denom, t1, t2 );
    _Distance = t1 >= 0.0f ? t1 : t2;
    return _Distance > 0.0f;
}


/*


Point tests


*/

AN_INLINE bool BvPointInPoly2D( Float2 const * _Points, int _NumPoints, float const & _PX, float const & _PY ) {
    int i, j, count = 0;
	
    for ( i = 0, j = _NumPoints - 1; i < _NumPoints; j = i++ ) {
        if (    ( ( _Points[i].Y <= _PY && _PY < _Points[j].Y ) || ( _Points[j].Y <= _PY && _PY < _Points[i].Y ) )
            &&  ( _PX < ( _Points[j].X - _Points[i].X ) * ( _PY - _Points[i].Y ) / ( _Points[j].Y - _Points[i].Y ) + _Points[i].X ) ) {
            count++;
        }
    }
	
    return count & 1;
}

AN_INLINE bool BvPointInPoly2D( Float2 const * _Points, int _NumPoints, Float2 const & _Point ) {
    return BvPointInPoly2D( _Points, _NumPoints, _Point.X, _Point.Y );
}

/**
Check is point inside convex hull:
InPoint - testing point (assumed point is on hull plane)
InNormal - hull normal
InPoints - hull vertices (CCW order required)
InNumPoints - hull vertex count (more or equal 3)
*/
AN_INLINE bool BvPointInConvexHullCCW( Float3 const & InPoint, Float3 const & InNormal, Float3 const * InPoints, int InNumPoints )
{
    AN_ASSERT( InNumPoints >= 3 );

    Float3 const * p = &InPoints[InNumPoints-1];

    for ( int i = 0 ; i < InNumPoints ; p = &InPoints[i], i++ ) {
        const Float3 edge = *p - InPoints[i];
        const Float3 edgeNormal = Math::Cross( InNormal, edge );
        float d = -Math::Dot( edgeNormal, InPoints[i] );
        float dist = Math::Dot( edgeNormal, InPoint ) + d;
        if ( dist > 0.0f ) {
            return false;
        }
    }

    return true;
}

/**
Check is point inside convex hull:
InPoint - testing point (assumed point is on hull plane)
InNormal - hull normal
InPoints - hull vertices (CW order required)
InNumPoints - hull vertex count (more or equal 3)
*/
AN_INLINE bool BvPointInConvexHullCW( Float3 const & InPoint, Float3 const & InNormal, Float3 const * InPoints, int InNumPoints )
{
    AN_ASSERT( InNumPoints >= 3 );

    Float3 const * p = &InPoints[InNumPoints-1];

    for ( int i = 0 ; i < InNumPoints ; p = &InPoints[i], i++ ) {
        const Float3 edge = InPoints[i] - *p;
        const Float3 edgeNormal = Math::Cross( InNormal, edge );
        float d = -Math::Dot( edgeNormal, InPoints[i] );
        float dist = Math::Dot( edgeNormal, InPoint ) + d;
        if ( dist > 0.0f ) {
            return false;
        }
    }

    return true;
}

/** Square of shortest distance between Point and Segment */
AN_INLINE float BvShortestDistanceSqr( Float3 const & _Point, Float3 const & _Start, Float3 const & _End ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const float dp1 = Math::Dot( v, dir );
    if ( dp1 <= 0.0f ) {
        return _Point.DistSqr( _Start );
    }

    const float dp2 = Math::Dot( dir, dir );
    if ( dp2 <= dp1 ) {
        return _Point.DistSqr( _End );
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir );
}

/** Square of distance between Point and Segment */
AN_INLINE bool BvDistanceSqr( Float3 const & _Point, Float3 const & _Start, Float3 const & _End, float & _Distance ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const float dp1 = Math::Dot( v, dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = Math::Dot( dir, dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    _Distance = v.DistSqr( ( dp1 / dp2 ) * dir );

    return true;
}

/** Check Point on Segment */
AN_INLINE bool BvIsPointOnSegment( Float3 const & _Point, Float3 const & _Start, Float3 const & _End, float _Epsilon ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const float dp1 = Math::Dot( v, dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = Math::Dot( dir, dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir ) < _Epsilon;
}

/** Square of distance between Point and Segment (2D) */
AN_INLINE float BvShortestDistanceSqr( Float2 const & _Point, Float2 const & _Start, Float2 const & _End ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const float dp1 = Math::Dot( v, dir );
    if ( dp1 <= 0.0f ) {
        return _Point.DistSqr( _Start );
    }

    const float dp2 = Math::Dot( dir, dir );
    if ( dp2 <= dp1 ) {
        return _Point.DistSqr( _End );
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir );
}

/** Square of distance between Point and Segment (2D) */
AN_INLINE bool BvDistanceSqr( Float2 const & _Point, Float2 const & _Start, Float2 const & _End, float & _Distance ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const float dp1 = Math::Dot( v, dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = Math::Dot( dir, dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    _Distance = v.DistSqr( ( dp1 / dp2 ) * dir );

    return true;
}

/** Check Point on Segment (2D) */
AN_INLINE bool BvIsPointOnSegment( Float2 const & _Point, Float2 const & _Start, Float2 const & _End, float _Epsilon ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const float dp1 = Math::Dot( v, dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const float dp2 = Math::Dot( dir, dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir ) < _Epsilon;
}


#if 0
/** Segment - Sphere */
AN_INLINE bool BvSegmentIntersectSphere( SegmentF const & _Segment, BvSphere const & _Sphere ) {
    const Float3 s = _Segment.Start - _Sphere.Center;
    const Float3 e = _Segment.End - _Sphere.Center;
    Float3 r = e - s;
    const float a = Math::Dot( r, -s );

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

/** Ray - OBB */
//AN_INLINE bool BvSegmentIntersectOrientedBox( RaySegment const & _Ray, BvOrientedBox const & _OBB ) {
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
/** Segment - Plane */
// _Distance - расстояние от начала луча до плоскости в направлении луча
AN_INLINE bool BvSegmentIntersectPlane( SegmentF const & _Segment, PlaneF const & _Plane, float & _Distance ) {
    const Float3 Dir = _Segment.End - _Segment.Start;
    const float Length = Dir.Length();
    if ( Length.CompareEps( 0.0f, 0.00001f ) ) {
        return false; // null-length segment
    }
    const float d2 = Math::Dot( _Plane.Normal, Dir / Length );
    if ( d2.CompareEps( 0.0f, 0.00001f ) ) {
        // Луч параллелен плоскости
        return false;
    }
    const float d1 = Math::Dot( _Plane.Normal, _Segment.Start ) + _Plane.D;
    _Distance = -( d1 / d2 );
    return _Distance >= 0.0f && _Distance <= Length;
}
#endif
