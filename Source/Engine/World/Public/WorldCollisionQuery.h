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

#include "Components/PhysicalBody.h"

/** Collision trace result */
struct FCollisionTraceResult
{
    /** Colliding body */
    FPhysicalBody * Body;
    /** Contact position */
    Float3 Position;
    /** Contact normal */
    Float3 Normal;
    /** Contact distance */
    float Distance;
    /** Contact fraction */
    float Fraction;

    /** Clear trace result */
    void Clear()
    {
        memset( this, 0, sizeof( *this ) );
    }
};

/** Collision query filter */
struct FCollisionQueryFilter
{
    /** List of actors that will be ignored during collision query */
    FActor ** IgnoreActors;
    int ActorsCount;

    /** List of bodies that will be ignored during collision query */
    FPhysicalBody ** IgnoreBodies;
    int BodiesCount;

    /** Physical body collision mask */
    int CollisionMask;

    /** Sort result by the distance */
    bool bSortByDistance;

    FCollisionQueryFilter()
    {
        IgnoreActors = nullptr;
        ActorsCount = 0;

        IgnoreBodies = nullptr;
        BodiesCount = 0;

        CollisionMask = CM_ALL;

        bSortByDistance = true;
    }
};

/** Convex sweep tracing */
struct FConvexSweepTest
{
    /** Convex collision body */
    FCollisionBody * CollisionBody;
    /** Scale of collision body */
    Float3 Scale;
    /** Start position for convex sweep trace */
    Float3 StartPosition;
    /** Start rotation for convex sweep trace */
    Quat StartRotation;
    /** End position for convex sweep trace */
    Float3 EndPosition;
    /** End rotation for convex sweep trace */
    Quat EndRotation;
    /** Query filter */
    FCollisionQueryFilter QueryFilter;
};

/** World collision queries */
struct FWorldCollisionQuery
{    
    /** Trace collision bodies */
    static bool Trace( FWorld const * _World, TPodArray< FCollisionTraceResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Trace collision bodies */
    static bool TraceClosest( FWorld const * _World, FCollisionTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Trace collision bodies */
    static bool TraceSphere( FWorld const * _World, FCollisionTraceResult & _Result, float _Radius, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Trace collision bodies */
    static bool TraceBox( FWorld const * _World, FCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Trace collision bodies */
    static bool TraceCylinder( FWorld const * _World, FCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Trace collision bodies */
    static bool TraceCapsule( FWorld const * _World, FCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Trace collision bodies */
    static bool TraceConvex( FWorld const * _World, FCollisionTraceResult & _Result, FConvexSweepTest const & _SweepTest );

    /** Query objects in sphere */
    static void QueryPhysicalBodies( FWorld const * _World, TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Query objects in box */
    static void QueryPhysicalBodies( FWorld const * _World, TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Query objects in AABB */
    static void QueryPhysicalBodies( FWorld const * _World, TPodArray< FPhysicalBody * > & _Result, BvAxisAlignedBox const & _BoundingBox, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Query objects in sphere */
    static void QueryActors( FWorld const * _World, TPodArray< FActor * > & _Result, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Query objects in box */
    static void QueryActors( FWorld const * _World, TPodArray< FActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, FCollisionQueryFilter const * _QueryFilter = nullptr );

    /** Query objects in AABB */
    static void QueryActors( FWorld const * _World, TPodArray< FActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, FCollisionQueryFilter const * _QueryFilter = nullptr );
};
