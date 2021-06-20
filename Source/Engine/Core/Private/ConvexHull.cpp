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

#include <Core/Public/ConvexHull.h>
#include <Core/Public/Logger.h>
#include <Core/Public/Core.h>

//#define CONVEX_HULL_CW

AConvexHull * AConvexHull::CreateEmpty( int _MaxPoints ) {
    AN_ASSERT( _MaxPoints > 0 );
    int size = sizeof( AConvexHull ) - sizeof( Points ) + _MaxPoints * sizeof( Points[0] );
    AConvexHull * hull = ( AConvexHull * )GZoneMemory.Alloc( size );
    hull->MaxPoints = _MaxPoints;
    hull->NumPoints = 0;
    return hull;
}

AConvexHull * AConvexHull::CreateForPlane( PlaneF const & _Plane, float _MaxExtents ) {
    Float3 rightVec;
    Float3 upVec;

    _Plane.Normal.ComputeBasis( rightVec, upVec );

    Float3 p = _Plane.Normal * _Plane.Dist(); // point on plane

    AConvexHull * hull;
    hull = CreateEmpty( 4 );
    hull->NumPoints = 4;
    
#ifdef CONVEX_HULL_CW
    // CW
    hull->Points[ 0 ] = ( upVec - rightVec ) * _MaxExtents;
    hull->Points[ 1 ] = ( upVec + rightVec ) * _MaxExtents;
#else
    // CCW
    hull->Points[ 0 ] = ( upVec - rightVec ) * _MaxExtents;
    hull->Points[ 1 ] = (-upVec - rightVec ) * _MaxExtents;
#endif

    hull->Points[ 2 ] = -hull->Points[ 0 ];
    hull->Points[ 3 ] = -hull->Points[ 1 ];
    hull->Points[ 0 ] += p;
    hull->Points[ 1 ] += p;
    hull->Points[ 2 ] += p;
    hull->Points[ 3 ] += p;

    return hull;
}

AConvexHull * AConvexHull::CreateFromPoints( Float3 const * _Points, int _NumPoints ) {
    AConvexHull * hull;
    hull = CreateEmpty( _NumPoints );
    hull->NumPoints = _NumPoints;
    Core::Memcpy( hull->Points, _Points, sizeof( Float3 ) * _NumPoints );
    return hull;
}

#if 0
AConvexHull * AConvexHull::RecreateFromPoints( AConvexHull * _OldHull, Float3 const * _Points, int _NumPoints ) {
    if ( _OldHull ) {

        // hull capacity is big enough
        if ( _OldHull->MaxPoints >= _NumPoints ) {
            Core::Memcpy( _OldHull->Points, _Points, _NumPoints * sizeof( Float3 ) );
            _OldHull->NumPoints = _NumPoints;
            return _OldHull;
        }

        // resize hull
        int oldSize = sizeof( AConvexHull ) - sizeof( Points ) + _OldHull->MaxPoints * sizeof( Points[0] );
        int newSize = oldSize + ( _NumPoints - _OldHull->MaxPoints ) * sizeof( Points[0] );
        AConvexHull * hull = ( AConvexHull * )GZoneMemory.Realloc( _OldHull, newSize, false );
        hull->MaxPoints = _NumPoints;
        hull->NumPoints = _NumPoints;
        Core::Memcpy( hull->Points, _Points, _NumPoints * sizeof( Float3 ) );
        return hull;
    }

    return CreateFromPoints( _Points, _NumPoints );
}
#endif

void AConvexHull::Destroy() {
    GZoneMemory.Free( this );
}

AConvexHull * AConvexHull::Duplicate() const {
    AConvexHull * hull;
    hull = CreateEmpty( MaxPoints );
    hull->NumPoints = NumPoints;
    Core::Memcpy( hull->Points, Points, sizeof( Float3 ) * NumPoints );
    return hull;
}

AConvexHull * AConvexHull::Reversed() const {
    AConvexHull * hull;
    hull = CreateEmpty( MaxPoints );
    hull->NumPoints = NumPoints;
    const int numPointsMinusOne = NumPoints - 1;
    for ( int i = 0 ; i < NumPoints ; i++ ) {
        hull->Points[ i ] = Points[ numPointsMinusOne - i ];
    }
    return hull;
}

EPlaneSide AConvexHull::Classify( PlaneF const & _Plane, float _Epsilon ) const {
    int front = 0;
    int back = 0;
    int onplane = 0;

    for ( int i = 0 ; i < NumPoints ; i++ ) {
        const Float3 & point = Points[ i ];
        const float d = _Plane.Dist( point );
        if ( d > _Epsilon ) {
            if ( back > 0 || onplane > 0 ) {
                return EPlaneSide::Cross;
            }
            front++;
        } else if ( d < -_Epsilon ) {
            if ( front > 0 || onplane > 0 ) {
                return EPlaneSide::Cross;
            }
            back++;
        } else {
            if ( back > 0 || front > 0 ) {
                return EPlaneSide::Cross;
            }
            onplane++;
        }
    }

    if ( onplane ) {
        return EPlaneSide::On;
    }

    if ( front ) {
        return EPlaneSide::Front;
    }

    if ( back ) {
        return EPlaneSide::Back;
    }

    return EPlaneSide::Cross;
}

bool AConvexHull::IsTiny( float _MinEdgeLength ) const {
    int numEdges = 0;
    for ( int i = 0 ; i < NumPoints ; i++ ) {
        const Float3 & p1 = Points[ i ];
        const Float3 & p2 = Points[ ( i + 1 ) % NumPoints ];
        if ( p1.Dist( p2 ) >= _MinEdgeLength ) {
            numEdges++;
            if ( numEdges == 3 ) {
                return false;
            }
        }
    }
    return true;
}

bool AConvexHull::IsHuge() const {
    for ( int i = 0 ; i < NumPoints ; i++ ) {
        const Float3 & p = Points[ i ];
        if (   p.X <= CONVEX_HULL_MIN_BOUNDS || p.X >= CONVEX_HULL_MAX_BOUNDS
            || p.Y <= CONVEX_HULL_MIN_BOUNDS || p.Y >= CONVEX_HULL_MAX_BOUNDS
            || p.Z <= CONVEX_HULL_MIN_BOUNDS || p.Z >= CONVEX_HULL_MAX_BOUNDS ) {
            return true;
        }
    }
    return false;
}

float AConvexHull::CalcArea() const {
    float Area = 0;
    for ( int i = 2 ; i < NumPoints ; i++ ) {
        Area += Math::Cross( Points[i - 1] - Points[0], Points[i] - Points[0] ).Length();
    }
    return Area * 0.5f;
}

BvAxisAlignedBox AConvexHull::CalcBounds() const {
    BvAxisAlignedBox bounds;

    if ( NumPoints ) {
        bounds.Mins = bounds.Maxs = Points[ 0 ];
    } else {
        bounds.Clear();
    }

    for ( int i = 1 ; i < NumPoints ; i++ ) {
        bounds.AddPoint( Points[ i ] );
    }
    return bounds;
}

Float3 AConvexHull::CalcNormal() const {
    if ( NumPoints < 3 ) {
        GLogger.Print( "AConvexHull::CalcNormal: num points < 3\n" );
        return Float3( 0 );
    }

    Float3 center = CalcCenter();

#ifdef CONVEX_HULL_CW
    // CW
    return Math::Cross( Points[1] - center, Points[0] - center ).NormalizeFix();
#else
    // CCW
    return Math::Cross( Points[0] - center, Points[1] - center ).NormalizeFix();
#endif
}

PlaneF AConvexHull::CalcPlane() const {
    PlaneF plane;

    if ( NumPoints < 3 ) {
        GLogger.Print( "AConvexHull::CalcPlane: num points < 3\n" );
        plane.Clear();
        return plane;
    }

    Float3 Center = CalcCenter();

#ifdef CONVEX_HULL_CW
    // CW
    plane.Normal = Math::Cross( Points[1] - Center, Points[0] - Center ).NormalizeFix();
#else
    // CCW
    plane.Normal = Math::Cross( Points[0] - Center, Points[1] - Center ).NormalizeFix();
#endif
    plane.D = -Math::Dot( Points[0], plane.Normal );
    return plane;
}

Float3 AConvexHull::CalcCenter() const {
    Float3 center(0);
    if ( !NumPoints ) {
        GLogger.Print( "AConvexHull::CalcCenter: no points in hull\n" );
        return center;
    }
    for ( int i = 0 ; i < NumPoints ; i++ ) {
        center += Points[i];
    }
    return center * ( 1.0f / NumPoints );
}

void AConvexHull::Reverse() {
    const int n = NumPoints >> 1;
    const int numPointsMinusOne = NumPoints - 1;
    Float3 tmp;
    for ( int i = 0 ; i < n ; i++ ) {
        tmp = Points[ i ];
        Points[ i ] = Points[ numPointsMinusOne - i ];
        Points[ numPointsMinusOne - i ] = tmp;
    }
}

EPlaneSide AConvexHull::Split( PlaneF const & _Plane, float _Epsilon, AConvexHull ** _Front, AConvexHull ** _Back ) const {
    int i;

    int front = 0;
    int back = 0;

    *_Front = *_Back = NULL;

    float * distances = ( float * )StackAlloc( ( NumPoints + 1 ) * sizeof( float ) );
    EPlaneSide * sides = ( EPlaneSide * )StackAlloc( ( NumPoints + 1 ) * sizeof( EPlaneSide ) );

    if ( !distances || !sides ) {
        CriticalError( "AConvexHull:Split: stack overflow\n" );
    }

    // Определить с какой стороны находится каждая точка исходного полигона
    for ( i = 0 ; i < NumPoints ; i++ ) {
        float dist = Math::Dot( Points[i], _Plane.Normal ) + _Plane.D;

        distances[ i ] = dist;

        if ( dist > _Epsilon ) {
            sides[ i ] = EPlaneSide::Front;
            front++;
        } else if ( dist < -_Epsilon ) {
            sides[ i ] = EPlaneSide::Back;
            back++;
        } else {
            sides[ i ] = EPlaneSide::On;
        }
    }

    sides[i] = sides[0];
    distances[i] = distances[0];

    // все точки полигона лежат на плоскости
    if ( !front && !back ) {
        Float3 hullNormal = CalcNormal();

        // По направлению нормали определить, куда можно отнести новый плейн

        if ( Math::Dot( hullNormal, _Plane.Normal ) > 0 ) {
            // Все точки находятся по фронтальную сторону плоскости
            *_Front = Duplicate();
            return EPlaneSide::Front;
        } else {
            // Все точки находятся по заднюю сторону плоскости
            *_Back = Duplicate();
            return EPlaneSide::Back;
        }
    }

    if ( !front ) {
        // Все точки находятся по заднюю сторону плоскости
        *_Back = Duplicate();
        return EPlaneSide::Back;
    }

    if ( !back ) {
        // Все точки находятся по фронтальную сторону плоскости
        *_Front = Duplicate();
        return EPlaneSide::Front;
    }

    *_Front = CreateEmpty( NumPoints + 4 );
    *_Back = CreateEmpty( NumPoints + 4 );

    AConvexHull * f = *_Front;
    AConvexHull * b = *_Back;

    for ( i = 0 ; i < NumPoints ; i++ ) {
        Float3 const & v = Points[ i ];

        if ( sides[ i ] == EPlaneSide::On ) {
            f->Points[f->NumPoints++] = v;
            b->Points[b->NumPoints++] = v;
            continue;
        }

        if ( sides[ i ] == EPlaneSide::Front ) {
            f->Points[f->NumPoints++] = v;
        }

        if ( sides[ i ] == EPlaneSide::Back ) {
            b->Points[b->NumPoints++] = v;
        }

        EPlaneSide nextSide = sides[ i + 1 ];

        if ( nextSide == EPlaneSide::On || nextSide == sides[ i ] ) {
            continue;
        }

        Float3 newVertex = Points[ ( i + 1 ) % NumPoints ];

        if ( sides[i] == EPlaneSide::Front ) {
            float dist = distances[i] / ( distances[i] - distances[ i + 1 ] );
            for ( int j = 0 ; j < 3 ; j++ ) {
                if ( _Plane.Normal[ j ] == 1 ) {
                    newVertex[ j ] = -_Plane.D;
                } else if ( _Plane.Normal[ j ] == -1 ) {
                    newVertex[ j ] = _Plane.D;
                } else {
                    newVertex[ j ] = v[ j ] + dist * ( newVertex[ j ] - v[ j ] );
                }
            }
            //newVertex.s = v.s + dist * ( newVertex.s - v.s );
            //newVertex.t = v.t + dist * ( newVertex.t - v.t );
        } else {
            float dist = distances[ i + 1 ] / ( distances[ i + 1 ] - distances[ i ] );
            for ( int j = 0 ; j < 3 ; j++ ) {
                if ( _Plane.Normal[ j ] == 1 ) {
                    newVertex[ j ] = -_Plane.D;
                } else if ( _Plane.Normal[ j ] == -1 ) {
                    newVertex[ j ] = _Plane.D;
                } else {
                    newVertex[ j ] = newVertex[ j ] + dist * ( v[ j ] - newVertex[ j ] );
                }
            }
            //newVertex.s = newVertex.s + dist * ( v.s - newVertex.s );
            //newVertex.t = newVertex.t + dist * ( v.t - newVertex.t );
        }

        AN_ASSERT( f->NumPoints < f->MaxPoints );
        f->Points[ f->NumPoints++ ] = newVertex;
        b->Points[ b->NumPoints++ ] = newVertex;
    }

    return EPlaneSide::Cross;
}

EPlaneSide AConvexHull::Clip( PlaneF const & _Plane, float _Epsilon, AConvexHull ** _Front ) const {
    int i;
    int front = 0;
    int back = 0;

    *_Front = NULL;

    float * distances = ( float * )StackAlloc( ( NumPoints + 1 ) * sizeof( float ) );
    EPlaneSide * sides = ( EPlaneSide * )StackAlloc( ( NumPoints + 1 ) * sizeof( EPlaneSide ) );

    if ( !distances || !sides ) {
        CriticalError( "AConvexHull:Clip: stack overflow\n" );
    }

    // Определить с какой стороны находится каждая точка исходного полигона
    for ( i = 0 ; i < NumPoints ; i++ ) {
        float dist = Math::Dot( Points[i], _Plane.Normal ) + _Plane.D;

        distances[ i ] = dist;

        if ( dist > _Epsilon ) {
            sides[ i ] = EPlaneSide::Front;
            front++;
        } else if ( dist < -_Epsilon ) {
            sides[ i ] = EPlaneSide::Back;
            back++;
        } else {
            sides[ i ] = EPlaneSide::On;
        }
    }

    sides[i] = sides[0];
    distances[i] = distances[0];

    if ( !front ) {
        // Все точки находятся по заднюю сторону плоскости
        return EPlaneSide::Back;
    }

    if ( !back ) {
        // Все точки находятся по фронтальную сторону плоскости
        *_Front = Duplicate();
        return EPlaneSide::Front;
    }

    *_Front = CreateEmpty( NumPoints + 4 );

    AConvexHull * f = *_Front;

    for ( i = 0 ; i < NumPoints ; i++ ) {
        Float3 const & v = Points[ i ];

        if ( sides[ i ] == EPlaneSide::On ) {
            f->Points[f->NumPoints++] = v;
            continue;
        }

        if ( sides[ i ] == EPlaneSide::Front ) {
            f->Points[f->NumPoints++] = v;
        }

        EPlaneSide nextSide = sides[ i + 1 ];

        if ( nextSide == EPlaneSide::On || nextSide == sides[ i ] ) {
            continue;
        }

        Float3 newVertex = Points[ ( i + 1 ) % NumPoints ];

        float dist = distances[ i ] / ( distances[ i ] - distances[ i + 1 ] );
        for ( int j = 0 ; j < 3 ; j++ ) {
            if ( _Plane.Normal[ j ] == 1 ) {
                newVertex[ j ] = -_Plane.D;
            } else if ( _Plane.Normal[ j ] == -1 ) {
                newVertex[ j ] = _Plane.D;
            } else {
                newVertex[ j ] = v[ j ] + dist * ( newVertex[j] - v[j] );
            }
        }

        f->Points[f->NumPoints++] = newVertex;
    }

    return EPlaneSide::Cross;
}
