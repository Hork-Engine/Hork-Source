#pragma once

#include "BvAxisAlignedBox.h"
#include "BvOrientedBox.h"
#include "BvSphere.h"

#include <Engine/Core/Public/Ray.h>

namespace FMath {

// TODO: Optimize

// Sphere - Sphere
AN_FORCEINLINE bool Intersects( BvSphere const & _S1, BvSphere const & _S2 ) {
    const Float R = _S1.Radius + _S2.Radius;
    return _S2.Center.DistSqr( _S1.Center ) <= R * R;
}

// AABB - AABB
AN_FORCEINLINE bool Intersects( BvAxisAlignedBox const & _AABB1, BvAxisAlignedBox const & _AABB2 ) {
    if ( _AABB1.Maxs[0] < _AABB2.Mins[0] || _AABB1.Mins[0] > _AABB2.Maxs[0] ) return false;
    if ( _AABB1.Maxs[1] < _AABB2.Mins[1] || _AABB1.Mins[1] > _AABB2.Maxs[1] ) return false;
    if ( _AABB1.Maxs[2] < _AABB2.Mins[2] || _AABB1.Mins[2] > _AABB2.Maxs[2] ) return false;
    return true;
}

// AABB - Sphere
AN_FORCEINLINE bool Intersects( BvAxisAlignedBox const & _AABB, BvSphere const & _Sphere ) {
#if 0
    float d = 0, dist;

    // проходим по осям X,Y,Z
    for ( int i = 0 ; i < 3 ; i++ ) {
       // если центр сферы лежит перед _AABB
       dist = _Sphere.Center[i] - _AABB.Mins[i];
       if ( dist < 0 ) {
          d += dist * dist; // суммируем квадрат расстояния по этой оси
       }
       // если центр сферы лежит после _AABB,
       dist = _Sphere.Center[i] - _AABB.Maxs[i];
       if ( dist > 0 ) {
          d += dist * dist; // суммируем квадрат расстояния по этой оси
       }
    }

    return d <= _Sphere.Radius * _Sphere.Radius;
#else
    Float3 DifMins = _Sphere.Center - _AABB.Mins;
    Float3 DifMaxs = _Sphere.Center - _AABB.Maxs;

    return  ( ( DifMins.X < 0.0f ) * DifMins.X*DifMins.X
            + ( DifMins.Y < 0.0f ) * DifMins.Y*DifMins.Y
            + ( DifMins.Z < 0.0f ) * DifMins.Z*DifMins.Z
            + ( DifMaxs.X > 0.0f ) * DifMaxs.X*DifMaxs.X
            + ( DifMaxs.Y > 0.0f ) * DifMaxs.Y*DifMaxs.Y
            + ( DifMaxs.Z > 0.0f ) * DifMaxs.Z*DifMaxs.Z ) <= _Sphere.Radius * _Sphere.Radius;
#endif
}

// AABB - Ray
//AN_FORCEINLINE bool Intersects( BvAxisAlignedBox const & _AABB, RaySegment const & _Ray ) {
//    // Code based on http://gdlinks.hut.ru/cdfaq/aabb.shtml

//    const Float3 rayVec = _Ray.end - _Ray.Start;

//    const float ZeroTolerance = 0.000001f;  // TODO: вынести куда-нибудь

//    float length = Length( rayVec );
//    if ( length < ZeroTolerance ) {
//        return false;
//    }

//    const Float3 dir = rayVec / length;
//    const Float3 aabbSize = ( _AABB.Maxs - _AABB.Mins ) * 0.5f;

//    length *= 0.5f; // half length

//    // T = Позиция AABB - midRayPoint
//    const Float3 T = _AABB.Mins + aabbSize - (_Ray.Start + rayVec * 0.5f);

//    // проверяем, является ли одна из осей X,Y,Z разделяющей
//    if ( (fabs( T.X ) > aabbSize.X + length*fabs( dir.X )) ||
//         (fabs( T.Y ) > aabbSize.Y + length*fabs( dir.Y )) ||
//         (fabs( T.Z ) > aabbSize.Z + length*fabs( dir.Z )) )
//        return false;

//    float r;

//    // проверяем X ^ dir
//    r = aabbSize.Y * fabs( dir.Z ) + aabbSize.Z * fabs( dir.Y );
//    if ( fabs( T.Y * dir.Z - T.Z * dir.Y ) > r )
//        return false;

//    // проверяем Y ^ dir
//    r = aabbSize.X * fabs( dir.Z ) + aabbSize.Z * fabs( dir.X );
//    if ( fabs( T.Z * dir.X - T.X * dir.Z ) > r )
//        return false;

//    // проверяем Z ^ dir
//    r = aabbSize.X * fabs( dir.Y ) + aabbSize.Y * fabs( dir.X );
//    if ( fabs( T.X * dir.Y - T.Y * dir.X ) > r )
//        return false;

//    return true;
//}

// AABB - Ray
AN_FORCEINLINE bool Intersects( BvAxisAlignedBox const & _AABB, Float3 const & _RayStart, Float3 const & _InvRayDir, float & _Min, float & _Max ) {
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

// AABB - Ray
AN_FORCEINLINE bool Intersects( BvAxisAlignedBox const & _AABB, Float3 const & _RayStart, Float3 const & _InvRayDir ) {
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

// Ray - triangle
bool Intersects( Float3 const & _RayStart, Float3 const & _RayDir, Float3 const & _P0, Float3 const & _P1, Float3 const & _P2, float & _Dist, float & _U, float & _V ) {
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
    _Dist = invDet * e2.Dot( q );

    // if _Dist < 0 this is a line intersection but not a ray intersection
    return _Dist > 0.0f;
}

// OBB - OBB
AN_FORCEINLINE bool Intersects( BvOrientedBox const & _OBB1, BvOrientedBox const & _OBB2 ) {
    // Code based on http://gdlinks.hut.ru/cdfaq/obb.shtml

    const Float3x3 A = _OBB1.Orient.Transposed();

    //смещение в мировой системе координат
    const Float3 v = _OBB2.Center - _OBB1.Center;

    //смещение в системе координат _OBB1
    const Float3 T = A * v;

    //создаем матрицу поворота _OBB2.Orient относительно _OBB1.Orient
    const Float3x3 R( A * _OBB2.Orient );

    float ra, rb;

    //система координат А
    for ( int i = 0 ; i < 3 ; i++ ) {
       ra = _OBB1.HalfSize[i];
       rb = _OBB2.HalfSize[0]*R[i][0].Abs() + _OBB2.HalfSize[1]*R[i][1].Abs() + _OBB2.HalfSize[2]*R[i][2].Abs();
       if ( T[i].Abs() > ra + rb )
          return false;
    }

    //система координат B
    for ( int i = 0 ; i < 3 ; i++ ) {
       ra = _OBB1.HalfSize[0]*R[0][i].Abs() + _OBB1.HalfSize[1]*R[1][i].Abs() + _OBB1.HalfSize[2]*R[2][i].Abs();
       rb = _OBB2.HalfSize[i];
       if ( ( T[0]*R[0][i] + T[1]*R[1][i] + T[2]*R[2][i] ).Abs() > ra + rb )
          return false;
    }

    //9 векторных произведений
    //L = A0 x B0
    ra = _OBB1.HalfSize[1]*R[2][0].Abs() + _OBB1.HalfSize[2]*R[1][0].Abs();
    rb = _OBB2.HalfSize[1]*R[0][2].Abs() + _OBB2.HalfSize[2]*R[0][1].Abs();
    if ( ( T[2]*R[1][0] - T[1]*R[2][0] ).Abs() > ra + rb )
       return false;

    //L = A0 x B1
    ra = _OBB1.HalfSize[1]*R[2][1].Abs() + _OBB1.HalfSize[2]*R[1][1].Abs();
    rb = _OBB2.HalfSize[0]*R[0][2].Abs() + _OBB2.HalfSize[2]*R[0][0].Abs();
    if ( ( T[2]*R[1][1] - T[1]*R[2][1] ).Abs() > ra + rb )
       return false;

    //L = A0 x B2
    ra = _OBB1.HalfSize[1]*R[2][2].Abs() + _OBB1.HalfSize[2]*R[1][2].Abs();
    rb = _OBB2.HalfSize[0]*R[0][1].Abs() + _OBB2.HalfSize[1]*R[0][0].Abs();
    if ( ( T[2]*R[1][2] - T[1]*R[2][2] ).Abs() > ra + rb )
       return false;

    //L = A1 x B0
    ra = _OBB1.HalfSize[0]*R[2][0].Abs() + _OBB1.HalfSize[2]*R[0][0].Abs();
    rb = _OBB2.HalfSize[1]*R[1][2].Abs() + _OBB2.HalfSize[2]*R[1][1].Abs();
    if ( ( T[0]*R[2][0] - T[2]*R[0][0] ).Abs() > ra + rb )
       return false;

    //L = A1 x B1
    ra = _OBB1.HalfSize[0]*R[2][1].Abs() + _OBB1.HalfSize[2]*R[0][1].Abs();
    rb = _OBB2.HalfSize[0]*R[1][2].Abs() + _OBB2.HalfSize[2]*R[1][0].Abs();
    if ( ( T[0]*R[2][1] - T[2]*R[0][1] ).Abs() > ra + rb )
       return false;

    //L = A1 x B2
    ra = _OBB1.HalfSize[0]*R[2][2].Abs() + _OBB1.HalfSize[2]*R[0][2].Abs();
    rb = _OBB2.HalfSize[0]*R[1][1].Abs() + _OBB2.HalfSize[1]*R[1][0].Abs();
    if ( ( T[0]*R[2][2] - T[2]*R[0][2] ).Abs() > ra + rb )
       return false;

    //L = A2 x B0
    ra = _OBB1.HalfSize[0]*R[1][0].Abs() + _OBB1.HalfSize[1]*R[0][0].Abs();
    rb = _OBB2.HalfSize[1]*R[2][2].Abs() + _OBB2.HalfSize[2]*R[2][1].Abs();
    if ( ( T[1]*R[0][0] - T[0]*R[1][0] ).Abs() > ra + rb )
       return false;

    //L = A2 x B1
    ra = _OBB1.HalfSize[0]*R[1][1].Abs() + _OBB1.HalfSize[1]*R[0][1].Abs();
    rb = _OBB2.HalfSize[0]*R[2][2].Abs() + _OBB2.HalfSize[2]*R[2][0].Abs();
    if ( ( T[1]*R[0][1] - T[0]*R[1][1] ).Abs() > ra + rb )
       return false;

    //L = A2 x B2
    ra = _OBB1.HalfSize[0]*R[1][2].Abs() + _OBB1.HalfSize[1]*R[0][2].Abs();
    rb = _OBB2.HalfSize[0]*R[2][1].Abs() + _OBB2.HalfSize[1]*R[2][0].Abs();
    if ( ( T[1]*R[0][2] - T[0]*R[1][2] ).Abs() > ra + rb )
       return false;

    return true;
}

// AABB - OBB
AN_FORCEINLINE bool Intersects( Float3 const & _AABBCenter, Float3 const & _AABBHalfSize, BvOrientedBox const & _OBB ) {
    //смещение в мировой системе координат
    const Float3 T = _OBB.Center - _AABBCenter;

    const Float3x3 & R = _OBB.Orient;

    float ra, rb;

    //система координат А
    for ( int i = 0 ; i < 3 ; i++ ) {
       ra = _AABBHalfSize[i];
       rb = _OBB.HalfSize[0]*R[i][0].Abs() + _OBB.HalfSize[1]*R[i][1].Abs() + _OBB.HalfSize[2]*R[i][2].Abs();
       if ( T[i].Abs() > ra + rb )
          return false;
    }

    //система координат B
    for ( int i = 0 ; i < 3 ; i++ ) {
       ra = _AABBHalfSize[0]*R[0][i].Abs() + _AABBHalfSize[1]*R[1][i].Abs() + _AABBHalfSize[2]*R[2][i].Abs();
       rb = _OBB.HalfSize[i];
       if ( ( T[0]*R[0][i] + T[1]*R[1][i] + T[2]*R[2][i] ).Abs() > ra + rb )
          return false;
    }

    //9 векторных произведений
    //L = A0 x B0
    ra = _AABBHalfSize[1]*R[2][0].Abs() + _AABBHalfSize[2]*R[1][0].Abs();
    rb = _OBB.HalfSize[1]*R[0][2].Abs() + _OBB.HalfSize[2]*R[0][1].Abs();
    if ( ( T[2]*R[1][0] - T[1]*R[2][0] ).Abs() > ra + rb )
       return false;

    //L = A0 x B1
    ra = _AABBHalfSize[1]*R[2][1].Abs() + _AABBHalfSize[2]*R[1][1].Abs();
    rb = _OBB.HalfSize[0]*R[0][2].Abs() + _OBB.HalfSize[2]*R[0][0].Abs();
    if ( ( T[2]*R[1][1] - T[1]*R[2][1] ).Abs() > ra + rb )
       return false;

    //L = A0 x B2
    ra = _AABBHalfSize[1]*R[2][2].Abs() + _AABBHalfSize[2]*R[1][2].Abs();
    rb = _OBB.HalfSize[0]*R[0][1].Abs() + _OBB.HalfSize[1]*R[0][0].Abs();
    if ( ( T[2]*R[1][2] - T[1]*R[2][2] ).Abs() > ra + rb )
       return false;

    //L = A1 x B0
    ra = _AABBHalfSize[0]*R[2][0].Abs() + _AABBHalfSize[2]*R[0][0].Abs();
    rb = _OBB.HalfSize[1]*R[1][2].Abs() + _OBB.HalfSize[2]*R[1][1].Abs();
    if ( ( T[0]*R[2][0] - T[2]*R[0][0] ).Abs() > ra + rb )
       return false;

    //L = A1 x B1
    ra = _AABBHalfSize[0]*R[2][1].Abs() + _AABBHalfSize[2]*R[0][1].Abs();
    rb = _OBB.HalfSize[0]*R[1][2].Abs() + _OBB.HalfSize[2]*R[1][0].Abs();
    if ( ( T[0]*R[2][1] - T[2]*R[0][1] ).Abs() > ra + rb )
       return false;

    //L = A1 x B2
    ra = _AABBHalfSize[0]*R[2][2].Abs() + _AABBHalfSize[2]*R[0][2].Abs();
    rb = _OBB.HalfSize[0]*R[1][1].Abs() + _OBB.HalfSize[1]*R[1][0].Abs();
    if ( ( T[0]*R[2][2] - T[2]*R[0][2] ).Abs() > ra + rb )
       return false;

    //L = A2 x B0
    ra = _AABBHalfSize[0]*R[1][0].Abs() + _AABBHalfSize[1]*R[0][0].Abs();
    rb = _OBB.HalfSize[1]*R[2][2].Abs() + _OBB.HalfSize[2]*R[2][1].Abs();
    if ( ( T[1]*R[0][0] - T[0]*R[1][0] ).Abs() > ra + rb )
       return false;

    //L = A2 x B1
    ra = _AABBHalfSize[0]*R[1][1].Abs() + _AABBHalfSize[1]*R[0][1].Abs();
    rb = _OBB.HalfSize[0]*R[2][2].Abs() + _OBB.HalfSize[2]*R[2][0].Abs();
    if ( ( T[1]*R[0][1] - T[0]*R[1][1] ).Abs() > ra + rb )
       return false;

    //L = A2 x B2
    ra = _AABBHalfSize[0]*R[1][2].Abs() + _AABBHalfSize[1]*R[0][2].Abs();
    rb = _OBB.HalfSize[0]*R[2][1].Abs() + _OBB.HalfSize[1]*R[2][0].Abs();
    if ( ( T[1]*R[0][2] - T[0]*R[1][2] ).Abs() > ra + rb )
       return false;

    return true;
}

// AABB - OBB
AN_FORCEINLINE bool Intersects( BvAxisAlignedBox const & _AABB, BvOrientedBox const & _OBB ) {
    return Intersects( _AABB.Center(), _AABB.HalfSize(), _OBB );
}

// OBB - Sphere
AN_FORCEINLINE bool Intersects( BvOrientedBox const & _OBB, BvSphere const & s ) {
    AN_UNUSED( _OBB );AN_UNUSED( s );
    AN_ASSERT( 0, "Not implemented yet" );
    // TODO: ...
    return false;
}

// OBB - Ray
//AN_FORCEINLINE bool Intersects( BvOrientedBox const & _OBB, RaySegment const & _Ray ) {
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

AN_FORCEINLINE bool Intersects( BvOrientedBox const & _OBB, RayF const & _Ray, float & _Min, float & _Max ) {
    const Float3 RayStart = _OBB.Orient * ( _Ray.Start - _OBB.Center ) + _OBB.Center;
    const Float3 RayDir = _OBB.Orient * _Ray.Dir;

    const Float3 Mins = _OBB.Center - _OBB.HalfSize;
    const Float3 Maxs = _OBB.Center + _OBB.HalfSize;

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

// OBB - Triangle
AN_FORCEINLINE bool Intersects( BvOrientedBox const & _OBB, Float3 const & _TrianglePoint1, Float3 const & _TrianglePoint2, Float3 const & _TrianglePoint3 ) {

    // Code based on http://gdlinks.hut.ru/cdfaq/obb.shtml

    Float3  n;    // проверяемая ось
    float   p;      // расстояние от _TrianglePoint1 до центра OBB вдоль оси
    float   d0, d1;	// + к p для _TrianglePoint2, _TrianglePoint3
    float   radius; // "радиус" OBB вдоль оси

    // расстояние от _TrianglePoint1 до центра OBB
    const Float3 distVec = _TrianglePoint1 - _OBB.Center;

    // стороны треугольника
    const Float3 edge0 = _TrianglePoint2 - _TrianglePoint1;
    const Float3 edge1 = _TrianglePoint3 - _TrianglePoint1;
    const Float3 edge2 = edge1 - edge0;

    // нормаль треугольника
    n = edge0.Cross( edge1 );

    // проверяем нормаль треугольника
    if ( n.Dot( distVec ).Abs() > (_OBB.HalfSize.X * n.Dot( _OBB.Orient[0] ).Abs() + _OBB.HalfSize.Y * n.Dot( _OBB.Orient[1] ).Abs() + _OBB.HalfSize.Z * n.Dot( _OBB.Orient[2] ).Abs() ) )
        return false;

    // проверяем оси OBB
    for ( int i = 0 ; i < 3 ; i++ ) {
        p  = _OBB.Orient[i].Dot( distVec );
        d0 = _OBB.Orient[i].Dot( edge0 );
        d1 = _OBB.Orient[i].Dot( edge1 );
        radius = _OBB.HalfSize[i];

        if ( FMath::Min( p, FMath::Min( p + d0, p + d1 ) ) > radius || FMath::Max( p, FMath::Max( p + d0 , p + d1 ) ) < -radius ) {
            return false;
        }
    }

    // проверяем obb.Orient[0] ^ edge0
    n = _OBB.Orient[0].Cross( edge0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = _OBB.HalfSize[1] * _OBB.Orient[2].Dot( edge0 ).Abs() + _OBB.HalfSize[2] * _OBB.Orient[1].Dot( edge0 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем obb.Orient[0] ^ edge1
    n = _OBB.Orient[0].Cross( edge1 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[1] * _OBB.Orient[2].Dot( edge1 ).Abs() + _OBB.HalfSize[2] * _OBB.Orient[1].Dot( edge1 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем _OBB.Orient[0] ^ edge2
    n = _OBB.Orient[0].Cross( edge2 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[1] * _OBB.Orient[2].Dot( edge2 ).Abs() + _OBB.HalfSize[2] * _OBB.Orient[1].Dot( edge2 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем obb.Orient[1] ^ edge0
    n = _OBB.Orient[1].Cross( edge0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = _OBB.HalfSize[0] * _OBB.Orient[2].Dot( edge0 ).Abs() + _OBB.HalfSize[2] * _OBB.Orient[0].Dot( edge0 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем obb.Orient[1] ^ edge1
    n = _OBB.Orient[1].Cross( edge1 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[0] * _OBB.Orient[2].Dot( edge1 ).Abs() + _OBB.HalfSize[2] * _OBB.Orient[0].Dot( edge1 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем obb.Orient[1] ^ edge2
    n = _OBB.Orient[1].Cross( edge2 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[0] * _OBB.Orient[2].Dot( edge2 ).Abs() + _OBB.HalfSize[2] * _OBB.Orient[0].Dot( edge2 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем obb.Orient[2] ^ edge0
    n = _OBB.Orient[2].Cross( edge0 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge1 );

    radius = _OBB.HalfSize[0] * _OBB.Orient[1].Dot( edge0 ).Abs() + _OBB.HalfSize[1] * _OBB.Orient[0].Dot( edge0 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем obb.Orient[2] ^ edge1
    n = _OBB.Orient[2].Cross( edge1 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[0] * _OBB.Orient[1].Dot( edge1 ).Abs() + _OBB.HalfSize[1] * _OBB.Orient[0].Dot( edge1 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    // проверяем obb.Orient[2] ^ edge2
    n = _OBB.Orient[2].Cross( edge2 );
    p = n.Dot( distVec );
    d0 = n.Dot( edge0 );

    radius = _OBB.HalfSize[0] * _OBB.Orient[1].Dot( edge2 ).Abs() + _OBB.HalfSize[1] * _OBB.Orient[0].Dot( edge2 ).Abs();

    if ( FMath::Min( p, p + d0 ) > radius || FMath::Max( p, p + d0 ) < -radius )
        return false;

    return true;
}

AN_FORCEINLINE bool PointInPoly( Float2 const * _Points, int _NumPoints, Float const & _PX, Float const & _PY ) {
    int i, j, count = 0;
	
    for ( i = 0, j = _NumPoints - 1; i < _NumPoints; j = i++ ) {
        if (    ( ( _Points[i].Y <= _PY && _PY < _Points[j].Y ) || ( _Points[j].Y <= _PY && _PY < _Points[i].Y ) )
            &&  ( _PX < ( _Points[j].X - _Points[i].X ) * ( _PY - _Points[i].Y ) / ( _Points[j].Y - _Points[i].Y ) + _Points[i].X ) ) {
            count++;
        }
    }
	
    return count & 1;
}

AN_FORCEINLINE bool PointInPoly( Float2 const * _Points, int _NumPoints, Float2 const & _Point ) {
    return PointInPoly( _Points, _NumPoints, _Point.X, _Point.Y );
}

// Sphere - Segment
AN_FORCEINLINE bool Intersects( BvSphere const & _Sphere, SegmentF const & _Segment ) {
    const Float3 s = _Segment.Start - _Sphere.Center;
    const Float3 e = _Segment.End - _Sphere.Center;
    Float3 r = e - s;
    const Float a = r.Dot( -s );

    if ( a <= 0.0f ) {
        return s.LengthSqr() < _Sphere.Radius * _Sphere.Radius;
    }

    const Float dot_r_r = r.LengthSqr();

    if ( a >= dot_r_r ) {
        return e.LengthSqr() < _Sphere.Radius * _Sphere.Radius;
    }

    r = s + ( a / dot_r_r ) * r;
    return r.LengthSqr() < _Sphere.Radius * _Sphere.Radius;
}

// Sphere - Ray
AN_FORCEINLINE bool Intersects( BvSphere const & _Sphere, RayF const & _Ray, Float & _Distance ) {
    const Float3 k = _Ray.Start - _Sphere.Center;
    const Float b = k.Dot( _Ray.Dir );

    _Distance = b * b - k.LengthSqr() + _Sphere.Radius * _Sphere.Radius;

    if ( _Distance < 0.0f ) {
        return false;
    }

    Float t1, t2;
    _Distance = FMath::Sqrt( _Distance );
    FMath::MinMax( -b + _Distance, -b - _Distance, t1, t2 );
    _Distance = t1 >= 0.0f ? t1 : t2;
    return _Distance > 0.0f;
}

// Sphere - Ray
AN_FORCEINLINE bool Intersects( BvSphere const & _Sphere, RayF const & _Ray, Float & _Distance1, Float & _Distance2 ) {
    const Float3 k = _Ray.Start - _Sphere.Center;
    const Float b = k.Dot( _Ray.Dir );
    Float a = _Ray.Dir.LengthSqr();
    Float distance = b * b - (k.LengthSqr() - _Sphere.Radius * _Sphere.Radius) * a;
    if ( distance < 0.0f ) {
        return false;
    }
    distance = FMath::Sqrt( distance );
    a = 1.0f / a;
    _Distance1 = ( -b + distance ) * a;
    _Distance2 = ( -b - distance ) * a;
    return true;
}

// Ray - Plane
// _Dist - расстояние от начала луча до плоскости в направлении луча
AN_FORCEINLINE bool Intersects( PlaneF const & _Plane, RayF const & _Ray, Float & _Dist ) {
    const Float d2 = _Plane.Normal.Dot( _Ray.Dir );
    if ( d2 == 0.0f ) {
        // ray is parallel to plane
        return false;
    }
    const Float d1 = _Plane.Normal.Dot( _Ray.Start ) + _Plane.D;
    _Dist = -( d1 / d2 );
    return true;
}

// Segment - Plane
// _Dist - расстояние от начала луча до плоскости в направлении луча
AN_FORCEINLINE bool Intersects( PlaneF const & _Plane, SegmentF const & _Segment, Float & _Dist ) {
    const Float3 Dir = _Segment.End - _Segment.Start;
    const Float Length = Dir.Length();
    if ( Length.CompareEps( 0.0f, 0.00001f ) ) {
        return false; // null-length segment
    }
    const Float d2 = _Plane.Normal.Dot( Dir / Length );
    if ( d2.CompareEps( 0.0f, 0.00001f ) ) {
        // Луч параллелен плоскости
        return false;
    }
    const Float d1 = _Plane.Normal.Dot( _Segment.Start ) + _Plane.D;
    _Dist = -( d1 / d2 );
    return _Dist >= 0.0f && _Dist <= Length;
}

// Ray - Elipsoid
AN_FORCEINLINE bool RayNearIntersectElipsoid( RayF const & _Ray, Float const & _Radius, Float const & _MParam, Float const & _NParam, Float & _Dist ) {
    const Float a = _Ray.Dir.X*_Ray.Dir.X + _MParam*_Ray.Dir.Y*_Ray.Dir.Y + _NParam*_Ray.Dir.Z*_Ray.Dir.Z;
    const Float b = 2.0f*(_Ray.Start.X*_Ray.Dir.X + _MParam*_Ray.Start.Y*_Ray.Dir.Y + _NParam*_Ray.Start.Z*_Ray.Dir.Z);
    const Float c = _Ray.Start.X*_Ray.Start.X + _MParam*_Ray.Start.Y*_Ray.Start.Y + _NParam*_Ray.Start.Z*_Ray.Start.Z - _Radius*_Radius;
    const Float d = b*b - 4.0f*a*c;
    if ( d < 0.0f ) {
        return false;
    }
    _Dist = ( -b - StdSqrt(d) ) / ( 2.0f*a );
    return _Dist >= 0.0f;
}

// Ray - Elipsoid
AN_FORCEINLINE bool RayFarIntersectElipsoid( RayF const & _Ray, Float const & _Radius, Float const & _MParam, Float const & _NParam, Float & _Dist ) {
    const Float a = _Ray.Dir.X*_Ray.Dir.X + _MParam*_Ray.Dir.Y*_Ray.Dir.Y + _NParam*_Ray.Dir.Z*_Ray.Dir.Z;
    const Float b = 2.0f*(_Ray.Start.X*_Ray.Dir.X + _MParam*_Ray.Start.Y*_Ray.Dir.Y + _NParam*_Ray.Start.Z*_Ray.Dir.Z);
    const Float c = _Ray.Start.X*_Ray.Start.X + _MParam*_Ray.Start.Y*_Ray.Start.Y + _NParam*_Ray.Start.Z*_Ray.Start.Z - _Radius*_Radius;
    const Float d = b*b - 4.0f*a*c;
    if ( d < 0.0f ) {
        return false;
    }
    _Dist = ( -b + StdSqrt(d) ) / ( 2.0f*a );
    return _Dist >= 0.0f;
}

// Square of shortest distance between Point and Segment
AN_FORCEINLINE Float ShortestDistanceSqr( Float3 const & _Point, Float3 const & _Start, Float3 const & _End ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const Float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return _Point.DistSqr( _Start );
    }

    const Float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return _Point.DistSqr( _End );
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir );
}

// Square of distance between Point and Segment
AN_FORCEINLINE bool DistanceSqr( Float3 const & _Point, Float3 const & _Start, Float3 const & _End, Float & _Dist ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const Float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const Float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    _Dist = v.DistSqr( ( dp1 / dp2 ) * dir );

    return true;
}

// Check Point on Segment
AN_FORCEINLINE bool IsPointOnSegment( Float3 const & _Point, Float3 const & _Start, Float3 const & _End, Float _Epsilon ) {
    const Float3 dir = _End - _Start;
    const Float3 v = _Point - _Start;

    const Float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const Float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir ) < _Epsilon;
}

// Square of distance between Point and Segment (2D)
AN_FORCEINLINE Float ShortestDistanceSqr( Float2 const & _Point, Float2 const & _Start, Float2 const & _End ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const Float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return _Point.DistSqr( _Start );
    }

    const Float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return _Point.DistSqr( _End );
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir );
}

// Square of distance between Point and Segment (2D)
AN_FORCEINLINE bool DistanceSqr( Float2 const & _Point, Float2 const & _Start, Float2 const & _End, Float & _Dist ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const Float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const Float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    _Dist = v.DistSqr( ( dp1 / dp2 ) * dir );

    return true;
}

// Check Point on Segment (2D)
AN_FORCEINLINE bool IsPointOnSegment( Float2 const & _Point, Float2 const & _Start, Float2 const & _End, Float _Epsilon ) {
    const Float2 dir = _End - _Start;
    const Float2 v = _Point - _Start;

    const Float dp1 = v.Dot( dir );
    if ( dp1 <= 0.0f ) {
        return false;
    }

    const Float dp2 = dir.Dot( dir );
    if ( dp2 <= dp1 ) {
        return false;
    }

    return v.DistSqr( ( dp1 / dp2 ) * dir ) < _Epsilon;
}

}
