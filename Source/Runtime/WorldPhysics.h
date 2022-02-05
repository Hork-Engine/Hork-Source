/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "Collision.h"
#include "CollisionModel.h"

class ADebugRenderer;
class APhysicalBody;
class ATerrainComponent;

/** Collision trace result */
struct SCollisionTraceResult
{
    /** Colliding body */
    AHitProxy* HitProxy;
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
        Platform::ZeroMem(this, sizeof(*this));
    }
};

/** Collision query filter */
struct SCollisionQueryFilter
{
    /** List of actors that will be ignored during collision query */
    AActor** IgnoreActors;
    int      ActorsCount;

    /** List of bodies that will be ignored during collision query */
    APhysicalBody** IgnoreBodies;
    int             BodiesCount;

    /** Physical body collision mask */
    int CollisionMask;

    /** Ignore triangle frontface, backface and edges */
    bool bCullBackFace;

    /** Sort result by the distance */
    bool bSortByDistance;

    SCollisionQueryFilter()
    {
        IgnoreActors = nullptr;
        ActorsCount  = 0;

        IgnoreBodies = nullptr;
        BodiesCount  = 0;

        CollisionMask = CM_ALL;

        bCullBackFace = true;

        bSortByDistance = true;
    }
};

/** Convex sweep tracing */
struct SConvexSweepTest
{
    /** Convex collision body */
    union
    {
        SCollisionSphereDef*      CollisionSphere;
        SCollisionSphereRadiiDef* CollisionSphereRADII;
        SCollisionBoxDef*         CollisionBox;
        SCollisionCylinderDef*    CollisionCylinder;
        SCollisionConeDef*        CollisionCone;
        SCollisionCapsuleDef*     CollisionCapsule;
        SCollisionConvexHullDef*  CollisionConvexHull;
    };
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
struct SCollisionContact
{
    class btPersistentManifold* Manifold;

    AActor*    ActorA;
    AActor*    ActorB;
    AHitProxy* ComponentA;
    AHitProxy* ComponentB;

    bool bActorADispatchContactEvents;
    bool bActorBDispatchContactEvents;
    bool bActorADispatchOverlapEvents;
    bool bActorBDispatchOverlapEvents;

    bool bComponentADispatchContactEvents;
    bool bComponentBDispatchContactEvents;
    bool bComponentADispatchOverlapEvents;
    bool bComponentBDispatchOverlapEvents;

    int Hash() const
    {
        uint32_t hash = Core::MurMur3Hash64(ComponentA->Id);
        hash          = Core::MurMur3Hash64(ComponentB->Id, hash);
        return hash;
    }
};

struct SCollisionQueryResult
{
    /** Colliding body */
    AHitProxy* HitProxy;
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
        Platform::ZeroMem(this, sizeof(*this));
    }
};

class AWorldPhysics
{
    AN_FORBID_COPY(AWorldPhysics)

public:
    /** Physics refresh rate */
    int PhysicsHertz = 60;

    TCallback<void(float)> PrePhysicsCallback;
    TCallback<void(float)> PostPhysicsCallback;

    /** Contact solver split impulse. Disabled by default for performance */
    bool bContactSolverSplitImpulse = false;

    /** Contact solver iterations count */
    int NumContactSolverIterations = 10;

    Float3 GravityVector;

    bool bGravityDirty = false;

    bool bDuringPhysicsUpdate = false;

    AWorldPhysics();
    virtual ~AWorldPhysics();

    /** Trace collision bodies */
    bool Trace(TPodVector<SCollisionTraceResult>& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceClosest(SCollisionTraceResult& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceSphere(SCollisionTraceResult& _Result, float _Radius, Float3 const& _RayStart, Float3 const& _RayEnd, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceBox(SCollisionTraceResult& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Experimental trace box with array of collisions */
    bool TraceBox2(TPodVector<SCollisionTraceResult>& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceCylinder(SCollisionTraceResult& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceCapsule(SCollisionTraceResult& _Result, float _CapsuleHeight, float CapsuleRadius, Float3 const& _RayStart, Float3 const& _RayEnd, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceConvex(SCollisionTraceResult& _Result, SConvexSweepTest const& _SweepTest) const;

    /** Query objects in sphere */
    void QueryHitProxies_Sphere(TPodVector<AHitProxy*>& _Result, Float3 const& _Position, float _Radius, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in box */
    void QueryHitProxies_Box(TPodVector<AHitProxy*>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in AABB */
    void QueryHitProxies(TPodVector<AHitProxy*>& _Result, BvAxisAlignedBox const& _BoundingBox, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in sphere */
    void QueryActors_Sphere(TPodVector<AActor*>& _Result, Float3 const& _Position, float _Radius, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in box */
    void QueryActors_Box(TPodVector<AActor*>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in AABB */
    void QueryActors(TPodVector<AActor*>& _Result, BvAxisAlignedBox const& _BoundingBox, SCollisionQueryFilter const* _QueryFilter = nullptr) const;

    void QueryCollision_Sphere(TPodVector<SCollisionQueryResult>& _Result, Float3 const& _Position, float _Radius, SCollisionQueryFilter const* _QueryFilter) const;

    void QueryCollision_Box(TPodVector<SCollisionQueryResult>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, SCollisionQueryFilter const* _QueryFilter) const;

    void QueryCollision(TPodVector<SCollisionQueryResult>& _Result, BvAxisAlignedBox const& _BoundingBox, SCollisionQueryFilter const* _QueryFilter) const;

    /** Simulate the physics */
    void Simulate(float _TimeStep);

    void DrawDebug(ADebugRenderer* InRenderer);

    // Internal
    class btSoftRigidDynamicsWorld* GetInternal() const { return DynamicsWorld.GetObject(); }

    struct btSoftBodyWorldInfo* GetSoftBodyWorldInfo() { return SoftBodyWorldInfo; }

private:
    friend class AHitProxy;

    /** Add or re-add physical body to the world */
    void AddHitProxy(AHitProxy* HitProxy);

    /** Remove physical body from the world */
    void RemoveHitProxy(AHitProxy* HitProxy);

private:
    /** Add physical body to pending list */
    void AddPendingBody(AHitProxy* InPhysicalBody);

    /** Remove physical body from pending list */
    void RemovePendingBody(AHitProxy* InPhysicalBody);

    /** Add physical bodies in pending list to physics world */
    void AddPendingBodies();

    void GenerateContactPoints(int _ContactIndex, SCollisionContact& _Contact);

    void DispatchContactAndOverlapEvents();

    void RemoveCollisionContacts();

    static void OnPrePhysics(class btDynamicsWorld* _World, float _TimeStep);
    static void OnPostPhysics(class btDynamicsWorld* _World, float _TimeStep);

    TUniqueRef<class btSoftRigidDynamicsWorld>                  DynamicsWorld;
    TUniqueRef<struct btDbvtBroadphase>                         BroadphaseInterface;
    TUniqueRef<class btSoftBodyRigidBodyCollisionConfiguration> CollisionConfiguration;
    TUniqueRef<class btCollisionDispatcher>                     CollisionDispatcher;
    TUniqueRef<class btSequentialImpulseConstraintSolver>       ConstraintSolver;
    TUniqueRef<class btGhostPairCallback>                       GhostPairCallback;
    struct btSoftBodyWorldInfo*                                 SoftBodyWorldInfo;
    TPodVector<SCollisionContact>                               CollisionContacts[2];
    THash<>                                                     ContactHash[2];
    TPodVector<SContactPoint>                                   ContactPoints;
    AHitProxy*                                                  PendingAddToWorldHead = nullptr;
    AHitProxy*                                                  PendingAddToWorldTail = nullptr;
    int                                                         FixedTickNumber       = 0;
    int                                                         CacheContactPoints    = -1;
};
