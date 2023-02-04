/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "World/HitProxy.h"
#include "CollisionModel.h"

class btDynamicsWorld;
class btSoftRigidDynamicsWorld;
class btPersistentManifold;
class btSoftBodyRigidBodyCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btGhostPairCallback;
struct btSoftBodyWorldInfo;
struct btDbvtBroadphase;

HK_NAMESPACE_BEGIN

class DebugRenderer;
class PhysicalBody;
class TerrainComponent;

/** Collision trace result */
struct CollisionTraceResult
{
    /** Colliding body */
    HitProxy* pObject;
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
struct CollisionQueryFilter
{
    /** List of actors that will be ignored during collision query */
    Actor** IgnoreActors;
    int     ActorsCount;

    /** List of bodies that will be ignored during collision query */
    PhysicalBody** IgnoreBodies;
    int            BodiesCount;

    /** Physical body collision mask */
    int CollisionMask;

    /** Ignore triangle frontface, backface and edges */
    bool bCullBackFace;

    /** Sort result by the distance */
    bool bSortByDistance;

    CollisionQueryFilter()
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
struct ConvexSweepTest
{
    /** Convex collision body */
    union
    {
        CollisionSphereDef*      CollisionSphere;
        CollisionSphereRadiiDef* CollisionSphereRADII;
        CollisionBoxDef*         CollisionBox;
        CollisionCylinderDef*    CollisionCylinder;
        CollisionConeDef*        CollisionCone;
        CollisionCapsuleDef*     CollisionCapsule;
        CollisionConvexHullDef*  CollisionConvexHull;
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
    CollisionQueryFilter QueryFilter;
};

/** Collision contact */
struct CollisionContact
{
    btPersistentManifold* Manifold;

    TRef<Actor>   ActorA;
    TRef<Actor>   ActorB;
    TRef<HitProxy> ComponentA;
    TRef<HitProxy> ComponentB;

    bool bActorADispatchContactEvents;
    bool bActorBDispatchContactEvents;
    bool bActorADispatchOverlapEvents;
    bool bActorBDispatchOverlapEvents;

    bool bComponentADispatchContactEvents;
    bool bComponentBDispatchContactEvents;
    bool bComponentADispatchOverlapEvents;
    bool bComponentBDispatchOverlapEvents;
};

struct ContactKey
{
    uint64_t Id[2];

    //ContactKey() = default;

    explicit ContactKey(CollisionContact const& Contact)
    {
        Id[0] = Contact.ComponentA->Id;
        Id[1] = Contact.ComponentB->Id;
    }

    bool operator==(ContactKey const& Rhs) const
    {
        return Id[0] == Rhs.Id[0] && Id[1] == Rhs.Id[1];
    }

    uint32_t Hash() const
    {
        uint32_t hash = HashTraits::Murmur3Hash64(Id[0]);
        hash          = HashTraits::Murmur3Hash64(Id[1], hash);
        return hash;
    }
};

struct CollisionQueryResult
{
    /** Colliding body */
    HitProxy* pObject;
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

class PhysicsSystem
{
    HK_FORBID_COPY(PhysicsSystem)

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

    PhysicsSystem();
    virtual ~PhysicsSystem();

    /** Trace collision bodies */
    bool Trace(TVector<CollisionTraceResult>& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceClosest(CollisionTraceResult& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceSphere(CollisionTraceResult& _Result, float _Radius, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceBox(CollisionTraceResult& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Experimental trace box with array of collisions */
    bool TraceBox2(TVector<CollisionTraceResult>& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceCylinder(CollisionTraceResult& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceCapsule(CollisionTraceResult& _Result, float _CapsuleHeight, float CapsuleRadius, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Trace collision bodies */
    bool TraceConvex(CollisionTraceResult& _Result, ConvexSweepTest const& _SweepTest) const;

    /** Query objects in sphere */
    void QueryHitProxies_Sphere(TVector<HitProxy*>& _Result, Float3 const& _Position, float _Radius, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in box */
    void QueryHitProxies_Box(TVector<HitProxy*>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in AABB */
    void QueryHitProxies(TVector<HitProxy*>& _Result, BvAxisAlignedBox const& _BoundingBox, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in sphere */
    void QueryActors_Sphere(TVector<Actor*>& _Result, Float3 const& _Position, float _Radius, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in box */
    void QueryActors_Box(TVector<Actor*>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    /** Query objects in AABB */
    void QueryActors(TVector<Actor*>& _Result, BvAxisAlignedBox const& _BoundingBox, CollisionQueryFilter const* _QueryFilter = nullptr) const;

    void QueryCollision_Sphere(TVector<CollisionQueryResult>& _Result, Float3 const& _Position, float _Radius, CollisionQueryFilter const* _QueryFilter) const;

    void QueryCollision_Box(TVector<CollisionQueryResult>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, CollisionQueryFilter const* _QueryFilter) const;

    void QueryCollision(TVector<CollisionQueryResult>& _Result, BvAxisAlignedBox const& _BoundingBox, CollisionQueryFilter const* _QueryFilter) const;

    /** Simulate the physics */
    void Simulate(float _TimeStep);

    void DrawDebug(DebugRenderer* InRenderer);

    // Internal
    btSoftRigidDynamicsWorld* GetInternal() const { return m_DynamicsWorld.GetObject(); }

    btSoftBodyWorldInfo* GetSoftBodyWorldInfo() { return m_SoftBodyWorldInfo; }

private:
    friend class HitProxy;

    /** Add or re-add physical body to the world */
    void AddHitProxy(HitProxy* pObject);

    /** Remove physical body from the world */
    void RemoveHitProxy(HitProxy* pObject);

private:
    /** Add physical body to pending list */
    void AddPendingBody(HitProxy* pObject);

    /** Remove physical body from pending list */
    void RemovePendingBody(HitProxy* pObject);

    /** Add physical bodies in pending list to physics world */
    void AddPendingBodies();

    void GenerateContactPoints(int _ContactIndex, CollisionContact& _Contact);

    void DispatchContactAndOverlapEvents();

    void RemoveCollisionContacts();

    static void OnPrePhysics(btDynamicsWorld* _World, float _TimeStep);
    static void OnPostPhysics(btDynamicsWorld* _World, float _TimeStep);

    TUniqueRef<btSoftRigidDynamicsWorld> m_DynamicsWorld;
    TUniqueRef<btDbvtBroadphase> m_BroadphaseInterface;
    TUniqueRef<btSoftBodyRigidBodyCollisionConfiguration> m_CollisionConfiguration;
    TUniqueRef<btCollisionDispatcher> m_CollisionDispatcher;
    TUniqueRef<btSequentialImpulseConstraintSolver> m_ConstraintSolver;
    TUniqueRef<btGhostPairCallback> m_GhostPairCallback;
    btSoftBodyWorldInfo* m_SoftBodyWorldInfo;
    TVector<CollisionContact> m_CollisionContacts[2];
    THashSet<ContactKey> m_ContactHash[2];
    TVector<ContactPoint> m_ContactPoints;
    HitProxy* m_PendingAddToWorldHead = nullptr;
    HitProxy* m_PendingAddToWorldTail = nullptr;
    int m_FixedTickNumber = 0;
    int m_CacheContactPoints = -1;
};

HK_NAMESPACE_END
