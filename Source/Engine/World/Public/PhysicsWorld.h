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

#include "CollisionEvents.h"
#include "Components/PhysicalBody.h"

class ADebugRenderer;

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btSoftRigidDynamicsWorld;
struct btSoftBodyWorldInfo;

/** Collision trace result */
struct SCollisionTraceResult
{
    /** Colliding body */
    APhysicalBody * Body;
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
struct SCollisionQueryFilter
{
    /** List of actors that will be ignored during collision query */
    AActor ** IgnoreActors;
    int ActorsCount;

    /** List of bodies that will be ignored during collision query */
    APhysicalBody ** IgnoreBodies;
    int BodiesCount;

    /** Physical body collision mask */
    int CollisionMask;

    /** Sort result by the distance */
    bool bSortByDistance;

    SCollisionQueryFilter()
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
struct SConvexSweepTest
{
    /** Convex collision body */
    ACollisionBody * CollisionBody;
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
    SCollisionQueryFilter QueryFilter;
};

/** Collision contact */
struct SCollisionContact {
    class btPersistentManifold * Manifold;

    AActor * ActorA;
    AActor * ActorB;
    APhysicalBody * ComponentA;
    APhysicalBody * ComponentB;

    bool bActorADispatchContactEvents;
    bool bActorBDispatchContactEvents;
    bool bActorADispatchOverlapEvents;
    bool bActorBDispatchOverlapEvents;

    bool bComponentADispatchContactEvents;
    bool bComponentBDispatchContactEvents;
    bool bComponentADispatchOverlapEvents;
    bool bComponentBDispatchOverlapEvents;

    int Hash() const {
        uint32_t hash = Core::PHHash64( ComponentA->Id );
        hash = Core::PHHash64( ComponentB->Id, hash );
        return hash;
    }
};

class IPhysicsWorldInterface {
public:
    virtual void OnPrePhysics( float _TimeStep ) {}
    virtual void OnPostPhysics( float _TimeStep ) {}
};

class APhysicsWorld {
    AN_FORBID_COPY( APhysicsWorld )

public:
    /** Physics refresh rate */
    int PhysicsHertz = 60;

    /** Enable interpolation during physics simulation */
    bool bEnablePhysicsInterpolation = true;

    /** Contact solver split impulse. Disabled by default for performance */
    bool bContactSolverSplitImpulse = false;

    /** Contact solver iterations count */
    int NumContactSolverIterations = 10;

    Float3 GravityVector;

    bool bGravityDirty;

    bool bDuringPhysicsUpdate;

    btSoftBodyWorldInfo * SoftBodyWorldInfo;

    btSoftRigidDynamicsWorld * DynamicsWorld;

    explicit APhysicsWorld( IPhysicsWorldInterface * InOwnerWorld );
    ~APhysicsWorld();

    /** Trace collision bodies */
    bool Trace( TPodArray< SCollisionTraceResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Trace collision bodies */
    bool TraceClosest( SCollisionTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Trace collision bodies */
    bool TraceSphere( SCollisionTraceResult & _Result, float _Radius, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Trace collision bodies */
    bool TraceBox( SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Trace collision bodies */
    bool TraceCylinder( SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Trace collision bodies */
    bool TraceCapsule( SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Trace collision bodies */
    bool TraceConvex( SCollisionTraceResult & _Result, SConvexSweepTest const & _SweepTest ) const;

    /** Query objects in sphere */
    void QueryPhysicalBodies( TPodArray< APhysicalBody * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Query objects in box */
    void QueryPhysicalBodies( TPodArray< APhysicalBody * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Query objects in AABB */
    void QueryPhysicalBodies( TPodArray< APhysicalBody * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Query objects in sphere */
    void QueryActors( TPodArray< AActor * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Query objects in box */
    void QueryActors( TPodArray< AActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Query objects in AABB */
    void QueryActors( TPodArray< AActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter = nullptr ) const;

    /** Simulate the physics */
    void Simulate( float _TimeStep );

    void DrawDebug( ADebugRenderer * InRenderer );

private:
    // Allow physical body to register self in PhysicsWorld
    friend class APhysicalBody;

    /** Add or re-add physical body to the world */
    void AddPhysicalBody( APhysicalBody * InPhysicalBody );

    /** Remove physical body from the world */
    void RemovePhysicalBody( APhysicalBody * InPhysicalBody );

private:
    /** Add physical body to pending list */
    void AddPendingBody( APhysicalBody * InPhysicalBody );

    /** Remove physical body from pending list */
    void RemovePendingBody( APhysicalBody * InPhysicalBody );

    /** Add physical bodies in pending list to physics world */
    void AddPendingBodies();

    void GenerateContactPoints( int _ContactIndex, SCollisionContact & _Contact );

    void DispatchContactAndOverlapEvents();

    void RemoveCollisionContacts();

    static void OnPrePhysics( class btDynamicsWorld * _World, float _TimeStep );
    static void OnPostPhysics( class btDynamicsWorld * _World, float _TimeStep );

    IPhysicsWorldInterface * pOwnerWorld;
    btBroadphaseInterface * PhysicsBroadphase;
    btDefaultCollisionConfiguration * CollisionConfiguration;
    btCollisionDispatcher * CollisionDispatcher;
    btSequentialImpulseConstraintSolver * ConstraintSolver;
    TPodArray< SCollisionContact > CollisionContacts[ 2 ];
    THash<> ContactHash[ 2 ];
    TPodArray< SContactPoint > ContactPoints;
    APhysicalBody * PendingAddToWorldHead;
    APhysicalBody * PendingAddToWorldTail;
    float TimeAccumulation;
    int FixedTickNumber;
};
