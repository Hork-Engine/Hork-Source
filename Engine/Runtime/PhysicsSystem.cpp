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

#include "PhysicsSystem.h"
#include "World/PhysicalBody.h"
#include "DebugRenderer.h"
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/Core/Platform/Logger.h>

#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>

#include "BulletCompatibility.h"

#ifdef HK_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4456 4828)
#endif
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#ifdef HK_COMPILER_MSVC
#    pragma warning(pop)
#endif

#define SOFT_BODY_WORLD

extern ContactAddedCallback gContactAddedCallback;

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawContactPoints("com_DrawContactPoints"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawConstraints("com_DrawConstraints"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawConstraintLimits("com_DrawConstraintLimits"s, "0"s, CVAR_CHEAT);
ConsoleVar com_NoPhysicsSimulation("com_NoPhysicsSimulation"s, "0"s, CVAR_CHEAT);

static CollisionQueryFilter DefaultCollisionQueryFilter;

struct CollisionFilterCallback : public btOverlapFilterCallback
{
    // Return true when pairs need collision
    bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const override
    {
        if ((proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask))
        {
            // FIXME: can we safe cast m_clientObject to btCollisionObject?
            btCollisionObject const* colObj0 = static_cast<btCollisionObject const*>(proxy0->m_clientObject);
            btCollisionObject const* colObj1 = static_cast<btCollisionObject const*>(proxy1->m_clientObject);

            HitProxy const* hitProxy0 = static_cast<HitProxy const*>(colObj0->getUserPointer());
            HitProxy const* hitProxy1 = static_cast<HitProxy const*>(colObj1->getUserPointer());

            if (!hitProxy0 || !hitProxy1)
            {
                return true;
            }

            Actor* actor0 = hitProxy0->GetOwnerActor();
            Actor* actor1 = hitProxy1->GetOwnerActor();

            if (hitProxy0->GetCollisionIgnoreActors().Contains(actor1))
            {
                return false;
            }

            if (hitProxy1->GetCollisionIgnoreActors().Contains(actor0))
            {
                return false;
            }

            return true;
        }

        return false;
    }
};

static CollisionFilterCallback CollisionFilterCallback;

static bool CustomMaterialCombinerCallback(btManifoldPoint& cp,
                                           const btCollisionObjectWrapper* colObj0Wrap,
                                           int partId0,
                                           int index0,
                                           const btCollisionObjectWrapper* colObj1Wrap,
                                           int partId1,
                                           int index1)
{
    const int normalAdjustFlags = 0
        //| BT_TRIANGLE_CONVEX_BACKFACE_MODE
        //| BT_TRIANGLE_CONCAVE_DOUBLE_SIDED //double sided options are experimental, single sided is recommended
        //| BT_TRIANGLE_CONVEX_DOUBLE_SIDED
        ;

    btAdjustInternalEdgeContacts(cp, colObj1Wrap, colObj0Wrap, partId1, index1, normalAdjustFlags);

    cp.m_combinedFriction = btManifoldResult::calculateCombinedFriction(colObj0Wrap->getCollisionObject(), colObj1Wrap->getCollisionObject());
    cp.m_combinedRestitution = btManifoldResult::calculateCombinedRestitution(colObj0Wrap->getCollisionObject(), colObj1Wrap->getCollisionObject());

    return true;
}

void PhysicsSystem::GenerateContactPoints(int _ContactIndex, CollisionContact& _Contact)
{
    if (m_CacheContactPoints == _ContactIndex)
    {
        // Contact points already generated for this contact
        return;
    }

    m_CacheContactPoints = _ContactIndex;

    m_ContactPoints.ResizeInvalidate(_Contact.Manifold->getNumContacts());

    bool bSwapped = static_cast<HitProxy*>(_Contact.Manifold->getBody0()->getUserPointer()) == _Contact.ComponentB;

    if ((_ContactIndex & 1) == 0)
    {
        // BodyA

        if (bSwapped)
        {
            for (int j = 0; j < _Contact.Manifold->getNumContacts(); ++j)
            {
                btManifoldPoint& point = _Contact.Manifold->getContactPoint(j);
                ContactPoint& contact = m_ContactPoints[j];
                contact.Position = btVectorToFloat3(point.m_positionWorldOnA);
                contact.Normal = -btVectorToFloat3(point.m_normalWorldOnB);
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        }
        else
        {
            for (int j = 0; j < _Contact.Manifold->getNumContacts(); ++j)
            {
                btManifoldPoint& point = _Contact.Manifold->getContactPoint(j);
                ContactPoint& contact = m_ContactPoints[j];
                contact.Position = btVectorToFloat3(point.m_positionWorldOnB);
                contact.Normal = btVectorToFloat3(point.m_normalWorldOnB);
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        }
    }
    else
    {
        // BodyB

        if (bSwapped)
        {
            for (int j = 0; j < _Contact.Manifold->getNumContacts(); ++j)
            {
                btManifoldPoint& point = _Contact.Manifold->getContactPoint(j);
                ContactPoint& contact = m_ContactPoints[j];
                contact.Position = btVectorToFloat3(point.m_positionWorldOnB);
                contact.Normal = btVectorToFloat3(point.m_normalWorldOnB);
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        }
        else
        {
            for (int j = 0; j < _Contact.Manifold->getNumContacts(); ++j)
            {
                btManifoldPoint& point = _Contact.Manifold->getContactPoint(j);
                ContactPoint& contact = m_ContactPoints[j];
                contact.Position = btVectorToFloat3(point.m_positionWorldOnA);
                contact.Normal = -btVectorToFloat3(point.m_normalWorldOnB);
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        }
    }
}

PhysicsSystem::PhysicsSystem()
{
    GravityVector = Float3(0.0f, -9.81f, 0.0f);

    gContactAddedCallback = CustomMaterialCombinerCallback;

#if 1
    m_BroadphaseInterface = MakeUnique<btDbvtBroadphase>();
#else
    m_BroadphaseInterface = MakeUnique<btAxisSweep3>(btVector3(-10000, -10000, -10000), btVector3(10000, 10000, 10000));
#endif

    //m_CollisionConfiguration = MakeUnique< btDefaultCollisionConfiguration >();
    m_CollisionConfiguration = MakeUnique<btSoftBodyRigidBodyCollisionConfiguration>();
    m_CollisionDispatcher = MakeUnique<btCollisionDispatcher>(m_CollisionConfiguration.GetObject());
    // TODO: remove this if we don't use gimpact
    btGImpactCollisionAlgorithm::registerAlgorithm(m_CollisionDispatcher.GetObject());
    m_ConstraintSolver = MakeUnique<btSequentialImpulseConstraintSolver>();

#ifdef SOFT_BODY_WORLD
    m_DynamicsWorld = MakeUnique<btSoftRigidDynamicsWorld>(m_CollisionDispatcher.GetObject(),
                                                           m_BroadphaseInterface.GetObject(),
                                                           m_ConstraintSolver.GetObject(),
                                                           m_CollisionConfiguration.GetObject(),
                                                           /* SoftBodySolver */ nullptr);
#else
    m_DynamicsWorld = MakeUnique<btDiscreteDynamicsWorld>(m_CollisionDispatcher, m_BroadphaseInterface, m_ConstraintSolver, m_CollisionConfiguration);
#endif

    m_DynamicsWorld->setGravity(btVectorToFloat3(GravityVector));
    m_DynamicsWorld->getDispatchInfo().m_useContinuous = true;
    //m_DynamicsWorld->getSolverInfo().m_splitImpulse = pOwnerWorld->bContactSolverSplitImpulse;
    //m_DynamicsWorld->getSolverInfo().m_numIterations = pOwnerWorld->NumContactSolverIterations;
    m_DynamicsWorld->getPairCache()->setOverlapFilterCallback(&CollisionFilterCallback);
    m_DynamicsWorld->setInternalTickCallback(OnPrePhysics, static_cast<void*>(this), true);
    m_DynamicsWorld->setInternalTickCallback(OnPostPhysics, static_cast<void*>(this), false);
    //m_DynamicsWorld->setSynchronizeAllMotionStates( true ); // TODO: check how it works

    m_GhostPairCallback = MakeUnique<btGhostPairCallback>();
    m_BroadphaseInterface->getOverlappingPairCache()->setInternalGhostPairCallback(m_GhostPairCallback.GetObject());

#ifdef SOFT_BODY_WORLD
    m_SoftBodyWorldInfo = &m_DynamicsWorld->getWorldInfo();
    m_SoftBodyWorldInfo->m_dispatcher = m_CollisionDispatcher.GetObject();
    m_SoftBodyWorldInfo->m_broadphase = m_BroadphaseInterface.GetObject();
    m_SoftBodyWorldInfo->m_gravity = btVectorToFloat3(GravityVector);
    m_SoftBodyWorldInfo->air_density = (btScalar)1.2;
    m_SoftBodyWorldInfo->water_density = 0;
    m_SoftBodyWorldInfo->water_offset = 0;
    m_SoftBodyWorldInfo->water_normal = btVector3(0, 0, 0);
    m_SoftBodyWorldInfo->m_sparsesdf.Initialize();
#endif
}

PhysicsSystem::~PhysicsSystem()
{
    RemoveCollisionContacts();

    m_DynamicsWorld.Reset();
    m_ConstraintSolver.Reset();
    m_CollisionDispatcher.Reset();
    m_CollisionConfiguration.Reset();
    m_BroadphaseInterface.Reset();
    m_GhostPairCallback.Reset();
}

void PhysicsSystem::RemoveCollisionContacts()
{
    for (int i = 0; i < 2; i++)
    {
        TVector<CollisionContact>& currentContacts = m_CollisionContacts[i];
        auto& contactHash = m_ContactHash[i];

        currentContacts.Clear();
        contactHash.Clear();
    }
}

void PhysicsSystem::AddPendingBody(HitProxy* InPhysicalBody)
{
    INTRUSIVE_ADD_UNIQUE(InPhysicalBody, NextMarked, PrevMarked, m_PendingAddToWorldHead, m_PendingAddToWorldTail);
}

void PhysicsSystem::RemovePendingBody(HitProxy* InPhysicalBody)
{
    INTRUSIVE_REMOVE(InPhysicalBody, NextMarked, PrevMarked, m_PendingAddToWorldHead, m_PendingAddToWorldTail);
}

void PhysicsSystem::AddHitProxy(HitProxy* HitProxy)
{
    if (!HitProxy)
    {
        // Passed a null pointer
        return;
    }

    if (HitProxy->bInWorld)
    {
        // Physical body is already in world, so remove it from the world
        if (HitProxy->GetCollisionObject())
        {
            m_DynamicsWorld->removeCollisionObject(HitProxy->GetCollisionObject());
        }
        HitProxy->bInWorld = false;
    }

    if (HitProxy->GetCollisionObject())
    {
        // Add physical body to pending list
        AddPendingBody(HitProxy);
    }
}

void PhysicsSystem::RemoveHitProxy(HitProxy* HitProxy)
{
    if (!HitProxy)
    {
        // Passed a null pointer
        return;
    }

    // Remove physical body from pending list
    RemovePendingBody(HitProxy);

    if (!HitProxy->bInWorld)
    {
        // Physical body is not in world
        return;
    }

    m_DynamicsWorld->removeCollisionObject(HitProxy->GetCollisionObject());

    HitProxy->bInWorld = false;
}

void PhysicsSystem::AddPendingBodies()
{
    HitProxy* next;
    for (HitProxy* hitProxy = m_PendingAddToWorldHead; hitProxy; hitProxy = next)
    {
        next = hitProxy->NextMarked;

        hitProxy->NextMarked = hitProxy->PrevMarked = nullptr;

        if (hitProxy->GetCollisionObject())
        {
            HK_ASSERT(!hitProxy->bInWorld);

            btRigidBody* rigidBody = btRigidBody::upcast(hitProxy->GetCollisionObject());
            if (rigidBody)
            {
                m_DynamicsWorld->addRigidBody(rigidBody, hitProxy->GetCollisionGroup(), hitProxy->GetCollisionMask());
            }
            else
            {
                m_DynamicsWorld->addCollisionObject(hitProxy->GetCollisionObject(), hitProxy->GetCollisionGroup(), hitProxy->GetCollisionMask());
            }
            hitProxy->bInWorld = true;
        }
    }
    m_PendingAddToWorldHead = m_PendingAddToWorldTail = nullptr;
}

void PhysicsSystem::DispatchContactAndOverlapEvents()
{
#ifdef HK_COMPILER_MSVC
#    pragma warning(disable : 4456)
#endif

    int curTickNumber = m_FixedTickNumber & 1;
    int prevTickNumber = (m_FixedTickNumber + 1) & 1;

    TVector<CollisionContact>& currentContacts = m_CollisionContacts[curTickNumber];
    TVector<CollisionContact>& prevContacts = m_CollisionContacts[prevTickNumber];

    auto& contactHash = m_ContactHash[curTickNumber];
    auto& prevContactHash = m_ContactHash[prevTickNumber];

    OverlapEvent overlapEvent;
    ContactEvent contactEvent;

    contactHash.Clear();
    currentContacts.Clear();

    int numManifolds = m_CollisionDispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; i++)
    {
        btPersistentManifold* contactManifold = m_CollisionDispatcher->getManifoldByIndexInternal(i);

        if (!contactManifold->getNumContacts())
        {
            continue;
        }

        HitProxy* objectA = static_cast<HitProxy*>(contactManifold->getBody0()->getUserPointer());
        HitProxy* objectB = static_cast<HitProxy*>(contactManifold->getBody1()->getUserPointer());

        if (!objectA || !objectB)
        {
            // ghost object
            continue;
        }

        if (objectA->Id < objectB->Id)
        {
            std::swap(objectA, objectB);
        }

        Actor* actorA = objectA->GetOwnerActor();
        Actor* actorB = objectB->GetOwnerActor();

        SceneComponent* componentA = objectA->GetOwnerComponent();
        SceneComponent* componentB = objectB->GetOwnerComponent();

        if (actorA->IsPendingKill() || actorB->IsPendingKill() || componentA->IsPendingKill() || componentB->IsPendingKill())
        {
            // don't generate contact or overlap events for destroyed objects
            continue;
        }

        // Do not generate contact events if one of components is trigger
        bool bContactWithTrigger = objectA->IsTrigger() || objectB->IsTrigger();

        CollisionContact contact;
        contact.bComponentADispatchContactEvents = !bContactWithTrigger && objectA->bDispatchContactEvents && (objectA->E_OnBeginContact || objectA->E_OnEndContact || objectA->E_OnUpdateContact);
        contact.bComponentBDispatchContactEvents = !bContactWithTrigger && objectB->bDispatchContactEvents && (objectB->E_OnBeginContact || objectB->E_OnEndContact || objectB->E_OnUpdateContact);
        contact.bComponentADispatchOverlapEvents = objectA->IsTrigger() && objectA->bDispatchOverlapEvents && (objectA->E_OnBeginOverlap || objectA->E_OnEndOverlap || objectA->E_OnUpdateOverlap);
        contact.bComponentBDispatchOverlapEvents = objectB->IsTrigger() && objectB->bDispatchOverlapEvents && (objectB->E_OnBeginOverlap || objectB->E_OnEndOverlap || objectB->E_OnUpdateOverlap);
        contact.bActorADispatchContactEvents = !bContactWithTrigger && objectA->bDispatchContactEvents && (actorA->E_OnBeginContact || actorA->E_OnEndContact || actorA->E_OnUpdateContact);
        contact.bActorBDispatchContactEvents = !bContactWithTrigger && objectB->bDispatchContactEvents && (actorB->E_OnBeginContact || actorB->E_OnEndContact || actorB->E_OnUpdateContact);
        contact.bActorADispatchOverlapEvents = objectA->IsTrigger() && objectA->bDispatchOverlapEvents && (actorA->E_OnBeginOverlap || actorA->E_OnEndOverlap || actorA->E_OnUpdateOverlap);
        contact.bActorBDispatchOverlapEvents = objectB->IsTrigger() && objectB->bDispatchOverlapEvents && (actorB->E_OnBeginOverlap || actorB->E_OnEndOverlap || actorB->E_OnUpdateOverlap);

        if (contact.bComponentADispatchContactEvents || contact.bComponentBDispatchContactEvents || contact.bComponentADispatchOverlapEvents || contact.bComponentBDispatchOverlapEvents || contact.bActorADispatchContactEvents || contact.bActorBDispatchContactEvents || contact.bActorADispatchOverlapEvents || contact.bActorBDispatchOverlapEvents)
        {
            contact.ActorA = actorA;
            contact.ActorB = actorB;
            contact.ComponentA = objectA;
            contact.ComponentB = objectB;
            contact.Manifold = contactManifold;

            ContactKey key(contact);

            if (!contactHash.Contains(key))
            {
                currentContacts.Add(std::move(contact));

                contactHash.Insert(key);
            }
            else
            {
                //LOG( "Contact duplicate\n" );
            }
        }
    }

    // Reset cache
    m_CacheContactPoints = -1;

    struct DispatchContactCondition
    {
        ContactEvent const& m_ContactEvent;

        DispatchContactCondition(ContactEvent const& InContactEvent) :
            m_ContactEvent(InContactEvent)
        {}

        bool operator()() const
        {
            if (m_ContactEvent.SelfActor->IsPendingKill())
            {
                return false;
            }

            if (m_ContactEvent.OtherActor->IsPendingKill())
            {
                return false;
            }

            if (!m_ContactEvent.SelfBody->GetOwnerComponent())
            {
                return false;
            }

            if (!m_ContactEvent.OtherBody->GetOwnerComponent())
            {
                return false;
            }

            return true;
        }
    };

    struct DispatchOverlapCondition
    {
        OverlapEvent const& m_OverlapEvent;

        DispatchOverlapCondition(OverlapEvent const& InOverlapEvent) :
            m_OverlapEvent(InOverlapEvent)
        {}

        bool operator()() const
        {
            if (m_OverlapEvent.SelfActor->IsPendingKill())
            {
                return false;
            }

            if (m_OverlapEvent.OtherActor->IsPendingKill())
            {
                return false;
            }

            if (!m_OverlapEvent.SelfBody->GetOwnerComponent())
            {
                return false;
            }

            if (!m_OverlapEvent.OtherBody->GetOwnerComponent())
            {
                return false;
            }

            return true;
        }
    };

    // Dispatch contact and overlap events (OnBeginContact, OnBeginOverlap, OnUpdateContact, OnUpdateOverlap)
    for (int i = 0; i < currentContacts.Size(); i++)
    {
        CollisionContact& contact = currentContacts[i];

        bool bFirstContact = !prevContactHash.Contains(ContactKey(contact));

        //if ( !contact.ActorA->IsPendingKill() )
        {
            if (contact.bActorADispatchContactEvents)
            {
                if (contact.ActorA->E_OnBeginContact || contact.ActorA->E_OnUpdateContact)
                {
                    if (contact.ComponentA->bGenerateContactPoints)
                    {
                        GenerateContactPoints(i << 1, contact);

                        contactEvent.Points = m_ContactPoints.ToPtr();
                        contactEvent.NumPoints = m_ContactPoints.Size();
                    }
                    else
                    {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorA;
                    contactEvent.SelfBody = contact.ComponentA;
                    contactEvent.OtherActor = contact.ActorB;
                    contactEvent.OtherBody = contact.ComponentB;

                    if (bFirstContact)
                    {
                        contact.ActorA->E_OnBeginContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                    else
                    {
                        contact.ActorA->E_OnUpdateContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                }
            }
            else if (contact.bActorADispatchOverlapEvents)
            {
                overlapEvent.SelfActor = contact.ActorA;
                overlapEvent.SelfBody = contact.ComponentA;
                overlapEvent.OtherActor = contact.ActorB;
                overlapEvent.OtherBody = contact.ComponentB;

                if (bFirstContact)
                {
                    contact.ActorA->E_OnBeginOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
                else
                {
                    contact.ActorA->E_OnUpdateOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
            }
        }

        //if ( !contact.ComponentA->IsPendingKill() )
        {
            if (contact.bComponentADispatchContactEvents)
            {
                if (contact.ComponentA->E_OnBeginContact || contact.ComponentA->E_OnUpdateContact)
                {
                    if (contact.ComponentA->bGenerateContactPoints)
                    {
                        GenerateContactPoints(i << 1, contact);

                        contactEvent.Points = m_ContactPoints.ToPtr();
                        contactEvent.NumPoints = m_ContactPoints.Size();
                    }
                    else
                    {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorA;
                    contactEvent.SelfBody = contact.ComponentA;
                    contactEvent.OtherActor = contact.ActorB;
                    contactEvent.OtherBody = contact.ComponentB;

                    if (bFirstContact)
                    {
                        contact.ComponentA->E_OnBeginContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                    else
                    {
                        contact.ComponentA->E_OnUpdateContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                }
            }
            else if (contact.bComponentADispatchOverlapEvents)
            {

                overlapEvent.SelfActor = contact.ActorA;
                overlapEvent.SelfBody = contact.ComponentA;
                overlapEvent.OtherActor = contact.ActorB;
                overlapEvent.OtherBody = contact.ComponentB;

                if (bFirstContact)
                {
                    contact.ComponentA->E_OnBeginOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
                else
                {
                    contact.ComponentA->E_OnUpdateOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
            }
        }

        //if ( !contact.ActorB->IsPendingKill() )
        {
            if (contact.bActorBDispatchContactEvents)
            {

                if (contact.ActorB->E_OnBeginContact || contact.ActorB->E_OnUpdateContact)
                {
                    if (contact.ComponentB->bGenerateContactPoints)
                    {
                        GenerateContactPoints((i << 1) + 1, contact);

                        contactEvent.Points = m_ContactPoints.ToPtr();
                        contactEvent.NumPoints = m_ContactPoints.Size();
                    }
                    else
                    {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorB;
                    contactEvent.SelfBody = contact.ComponentB;
                    contactEvent.OtherActor = contact.ActorA;
                    contactEvent.OtherBody = contact.ComponentA;

                    if (bFirstContact)
                    {
                        contact.ActorB->E_OnBeginContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                    else
                    {
                        contact.ActorB->E_OnUpdateContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                }
            }
            else if (contact.bActorBDispatchOverlapEvents)
            {
                overlapEvent.SelfActor = contact.ActorB;
                overlapEvent.SelfBody = contact.ComponentB;
                overlapEvent.OtherActor = contact.ActorA;
                overlapEvent.OtherBody = contact.ComponentA;

                if (bFirstContact)
                {
                    contact.ActorB->E_OnBeginOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
                else
                {
                    contact.ActorB->E_OnUpdateOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
            }
        }

        //if ( !contact.ComponentB->IsPendingKill() )
        {
            if (contact.bComponentBDispatchContactEvents)
            {

                if (contact.ComponentB->E_OnBeginContact || contact.ComponentB->E_OnUpdateContact)
                {
                    if (contact.ComponentB->bGenerateContactPoints)
                    {
                        GenerateContactPoints((i << 1) + 1, contact);

                        contactEvent.Points = m_ContactPoints.ToPtr();
                        contactEvent.NumPoints = m_ContactPoints.Size();
                    }
                    else
                    {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorB;
                    contactEvent.SelfBody = contact.ComponentB;
                    contactEvent.OtherActor = contact.ActorA;
                    contactEvent.OtherBody = contact.ComponentA;

                    if (bFirstContact)
                    {
                        contact.ComponentB->E_OnBeginContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                    else
                    {
                        contact.ComponentB->E_OnUpdateContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
                    }
                }
            }
            else if (contact.bComponentBDispatchOverlapEvents)
            {

                overlapEvent.SelfActor = contact.ActorB;
                overlapEvent.SelfBody = contact.ComponentB;
                overlapEvent.OtherActor = contact.ActorA;
                overlapEvent.OtherBody = contact.ComponentA;

                if (bFirstContact)
                {
                    contact.ComponentB->E_OnBeginOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
                else
                {
                    contact.ComponentB->E_OnUpdateOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
                }
            }
        }
    }

    // Dispatch contact and overlap events (OnEndContact, OnEndOverlap)
    for (int i = 0; i < prevContacts.Size(); i++)
    {
        CollisionContact& contact = prevContacts[i];
        ContactKey key(contact);
        bool bHaveContact = contactHash.Contains(key);

        if (bHaveContact)
        {
            continue;
        }

        if (contact.bActorADispatchContactEvents)
        {
            if (contact.ActorA->E_OnEndContact)
            {
                contactEvent.SelfActor = contact.ActorA;
                contactEvent.SelfBody = contact.ComponentA;
                contactEvent.OtherActor = contact.ActorB;
                contactEvent.OtherBody = contact.ComponentB;
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ActorA->E_OnEndContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
            }
        }
        else if (contact.bActorADispatchOverlapEvents)
        {
            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            contact.ActorA->E_OnEndOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
        }

        if (contact.bComponentADispatchContactEvents)
        {

            if (contact.ComponentA->E_OnEndContact)
            {
                contactEvent.SelfActor = contact.ActorA;
                contactEvent.SelfBody = contact.ComponentA;
                contactEvent.OtherActor = contact.ActorB;
                contactEvent.OtherBody = contact.ComponentB;
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ComponentA->E_OnEndContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
            }
        }
        else if (contact.bComponentADispatchOverlapEvents)
        {

            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            contact.ComponentA->E_OnEndOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
        }

        if (contact.bActorBDispatchContactEvents)
        {

            if (contact.ActorB->E_OnEndContact)
            {
                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ActorB->E_OnEndContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
            }
        }
        else if (contact.bActorBDispatchOverlapEvents)
        {
            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            contact.ActorB->E_OnEndOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
        }

        if (contact.bComponentBDispatchContactEvents)
        {

            if (contact.ComponentB->E_OnEndContact)
            {
                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ComponentB->E_OnEndContact.DispatchConditional(DispatchContactCondition(contactEvent), contactEvent);
            }
        }
        else if (contact.bComponentBDispatchOverlapEvents)
        {

            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            contact.ComponentB->E_OnEndOverlap.DispatchConditional(DispatchOverlapCondition(overlapEvent), overlapEvent);
        }
    }
}

void PhysicsSystem::OnPrePhysics(btDynamicsWorld* _World, float _TimeStep)
{
    PhysicsSystem* self = static_cast<PhysicsSystem*>(_World->getWorldUserInfo());

    //self->AddPendingBodies();

    self->PrePhysicsCallback(_TimeStep);
}

void PhysicsSystem::OnPostPhysics(btDynamicsWorld* _World, float _TimeStep)
{
    PhysicsSystem* self = static_cast<PhysicsSystem*>(_World->getWorldUserInfo());

    self->DispatchContactAndOverlapEvents();

    self->PostPhysicsCallback(_TimeStep);
    self->m_FixedTickNumber++;
}

void PhysicsSystem::Simulate(float _TimeStep)
{
    AddPendingBodies();

    if (com_NoPhysicsSimulation)
    {
        return;
    }

    const float FixedTimeStep = 1.0f / PhysicsHertz;

    int maxSubSteps = Math::ToIntFast(Math::Floor(_TimeStep * PhysicsHertz)) + 1;
    //maxSubSteps = Math::Min( maxSubSteps, MAX_SIMULATION_SUB_STEPS );

    btContactSolverInfo& contactSolverInfo = m_DynamicsWorld->getSolverInfo();
    contactSolverInfo.m_numIterations = Math::Clamp(NumContactSolverIterations, 1, 256);
    contactSolverInfo.m_splitImpulse = bContactSolverSplitImpulse;

    // Update world gravity
    if (bGravityDirty)
    {
        m_DynamicsWorld->setGravity(btVectorToFloat3(GravityVector));
        bGravityDirty = false;
    }

    // Simulation
    bDuringPhysicsUpdate = true;
    m_DynamicsWorld->stepSimulation(_TimeStep, maxSubSteps, FixedTimeStep);
    bDuringPhysicsUpdate = false;

#ifdef SOFT_BODY_WORLD
    m_SoftBodyWorldInfo->m_sparsesdf.GarbageCollect();
#endif
}

void PhysicsSystem::DrawDebug(DebugRenderer* InRenderer)
{
    int mode = 0;
    if (com_DrawContactPoints)
    {
        mode |= btIDebugDraw::DBG_DrawContactPoints;
    }
    if (com_DrawConstraints)
    {
        mode |= btIDebugDraw::DBG_DrawConstraints;
    }
    if (com_DrawConstraintLimits)
    {
        mode |= btIDebugDraw::DBG_DrawConstraintLimits;
    }
    if (mode)
    {
        class ABulletDebugDraw : public btIDebugDraw
        {
        public:
            DebugRenderer* Renderer;
            int DebugMode;

            void drawLine(btVector3 const& from, btVector3 const& to, btVector3 const& color) override
            {
                Renderer->SetColor(Color4(color.x(), color.y(), color.z(), 1.0f));
                Renderer->DrawLine(btVectorToFloat3(from), btVectorToFloat3(to));
            }

            void drawContactPoint(btVector3 const& pointOnB, btVector3 const& normalOnB, btScalar distance, int lifeTime, btVector3 const& color) override
            {
                Renderer->SetColor(Color4(color.x(), color.y(), color.z(), 1.0f));
                Renderer->DrawPoint(btVectorToFloat3(pointOnB));
                Renderer->DrawPoint(btVectorToFloat3(normalOnB));
            }

            void reportErrorWarning(const char* warningString) override
            {
            }

            void draw3dText(btVector3 const& location, const char* textString) override
            {
            }

            void setDebugMode(int debugMode) override
            {
                DebugMode = debugMode;
            }

            int getDebugMode() const override
            {
                return DebugMode;
            }

            void flushLines() override
            {
            }
        };
        static ABulletDebugDraw debugDrawer;

        InRenderer->SetDepthTest(false);
        debugDrawer.Renderer = InRenderer;
        debugDrawer.setDebugMode(mode);
        m_DynamicsWorld->setDebugDrawer(&debugDrawer);
        m_DynamicsWorld->debugDrawWorld();
    }
}

static bool CompareDistance(CollisionTraceResult const& A, CollisionTraceResult const& B)
{
    return A.Distance < B.Distance;
}

static bool FindCollisionActor(CollisionQueryFilter const& _QueryFilter, Actor* _Actor)
{
    for (int i = 0; i < _QueryFilter.ActorsCount; i++)
    {
        if (_Actor == _QueryFilter.IgnoreActors[i])
        {
            return true;
        }
    }
    return false;
}

static bool FindCollisionBody(CollisionQueryFilter const& _QueryFilter, SceneComponent* _Body)
{
    for (int i = 0; i < _QueryFilter.BodiesCount; i++)
    {
        if (_Body->Id == _QueryFilter.IgnoreBodies[i]->Id)
        {
            return true;
        }
    }
    return false;
}

HK_FORCEINLINE static bool NeedsCollision(CollisionQueryFilter const& _QueryFilter, btBroadphaseProxy* _Proxy)
{
    btCollisionObject* collisionObject = static_cast<btCollisionObject*>(_Proxy->m_clientObject);

    HitProxy* hitProxy = static_cast<HitProxy*>(collisionObject->getUserPointer());

    if (!hitProxy)
    {
        // something bad
        return false;
    }

    if (FindCollisionActor(_QueryFilter, hitProxy->GetOwnerActor()))
    {
        return false;
    }

    if (FindCollisionBody(_QueryFilter, hitProxy->GetOwnerComponent()))
    {
        return false;
    }

    return (_Proxy->m_collisionFilterGroup & _QueryFilter.CollisionMask) && _Proxy->m_collisionFilterMask;
}

#if 0
static bool CullTriangle( btCollisionObject const * Object, btCollisionWorld::LocalShapeInfo const * LocalShapeInfo, btVector3 const & HitNormal, bool bNormalInWorldSpace, int TriangleFaceCull )
{
    if ( TriangleFaceCull == COLLISION_TRIANGLE_CULL_NONE ) {
        return false;
    }

    // TODO: TERRAIN_SHAPE_PROXYTYPE

    if ( Object->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE
         || Object->getCollisionShape()->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE )
    {
        const btBvhTriangleMeshShape * trimesh;

        if ( Object->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE ) {
            trimesh = static_cast< const btScaledBvhTriangleMeshShape * >( Object->getCollisionShape() )->getChildShape();
        } else {
            trimesh = static_cast< const btBvhTriangleMeshShape * >( Object->getCollisionShape() );
        }

        const Float3 * pVertices;
        int VertexCount;
        PHY_ScalarType VertexType;
        int VertexStride;
        const unsigned int *pIndices;
        int IndexStride;
        int FaceCount;
        PHY_ScalarType IndexType;

        trimesh->getMeshInterface()->getLockedReadOnlyVertexIndexBase((const byte **)&pVertices,
                                                                      VertexCount,
                                                                      VertexType,
                                                                      VertexStride,
                                                                      (const byte **)&pIndices,
                                                                      IndexStride,
                                                                      FaceCount,
                                                                      IndexType,
                                                                      LocalShapeInfo->m_shapePart);

        HK_ASSERT(VertexType==PHY_FLOAT);
        HK_ASSERT(IndexType==PHY_INTEGER);
        HK_ASSERT(VertexStride==sizeof(Float3));
        HK_ASSERT(IndexStride==sizeof(unsigned int)*3);

        const unsigned int * triangleIndices = &pIndices[ LocalShapeInfo->m_triangleIndex * 3 ];

        Float3 v0 = pVertices[ triangleIndices[0] ];
        Float3 v1 = pVertices[ triangleIndices[1] ];
        Float3 v2 = pVertices[ triangleIndices[2] ];

        Float3 normal = Math::Cross( v1-v0, v2-v0 ).Normalized();
        btVector3 triangleNormal = Object->getWorldTransform().getBasis() * btVectorToFloat3( normal );

        btVector3 contactNormal = bNormalInWorldSpace
                ? HitNormal
                : Object->getWorldTransform().getBasis() * HitNormal;

        const float dp = btDot( contactNormal, triangleNormal );
        //const float eps = 0.001f;

        if ( TriangleFaceCull == COLLISION_TRIANGLE_CULL_BACKFACE ) {
            //return dp < 1.0f - eps;
            return dp < 0.0f;
        }

        // front face
        //return dp > -1.0f + eps;
        return dp > 0.0f;
    }

    return false;
}
#endif

struct TraceRayResultCallback : btCollisionWorld::RayResultCallback
{
    TraceRayResultCallback(CollisionQueryFilter const* _QueryFilter, Float3 const& _RayStart, Float3 const& _RayDir, TPodVector<CollisionTraceResult>& _Result) :
        RayLength(_RayDir.Length()), RayStart(_RayStart), RayDir(_RayDir), QueryFilter(_QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter), Result(_Result)
    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;

        if (QueryFilter.bCullBackFace)
        {
            m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        }

        m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;

        //    enum EFlags
        //    {
        //       kF_None                 = 0,
        //       kF_FilterBackfaces      = 1 << 0,
        //       kF_KeepUnflippedNormal  = 1 << 1,   // Prevents returned face normal getting flipped when a ray hits a back-facing triangle
        //         ///SubSimplexConvexCastRaytest is the default, even if kF_None is set.
        //       kF_UseSubSimplexConvexCastRaytest = 1 << 2,   // Uses an approximate but faster ray versus convex intersection algorithm
        //       kF_UseGjkConvexCastRaytest = 1 << 3,
        //       kF_Terminator        = 0xFFFFFFFF
        //    };
    }

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        return NeedsCollision(QueryFilter, proxy0);
    }

    btScalar addSingleResult(btCollisionWorld::LocalRayResult& RayResult, bool bNormalInWorldSpace) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( RayResult.m_collisionObject, RayResult.m_localShapeInfo, RayResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        const btCollisionObject* hitCollisionObject = RayResult.m_collisionObject;

        CollisionTraceResult& hit = Result.Add();
        hit.Clear();
        hit.HitProxy = static_cast<HitProxy*>(hitCollisionObject->getUserPointer());
        hit.Position = RayStart + RayResult.m_hitFraction * RayDir;
        hit.Normal = bNormalInWorldSpace ? btVectorToFloat3(RayResult.m_hitNormalLocal) : btVectorToFloat3(hitCollisionObject->getWorldTransform().getBasis() * RayResult.m_hitNormalLocal);
        hit.Distance = RayResult.m_hitFraction * RayLength;
        hit.Fraction = RayResult.m_hitFraction;

        return m_closestHitFraction;
    }

    float RayLength;
    Float3 RayStart;
    Float3 RayDir;
    CollisionQueryFilter const& QueryFilter;
    TPodVector<CollisionTraceResult>& Result;
};

struct TraceClosestRayResultCallback : btCollisionWorld::RayResultCallback
{
    TraceClosestRayResultCallback(CollisionQueryFilter const* _QueryFilter, btVector3 const& rayFromWorld, btVector3 const& rayToWorld) :
        QueryFilter(_QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter), m_rayFromWorld(rayFromWorld), m_rayToWorld(rayToWorld)

    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;

        if (QueryFilter.bCullBackFace)
        {
            m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        }

        m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;
    }

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        return NeedsCollision(QueryFilter, proxy0);
    }

    btScalar addSingleResult(btCollisionWorld::LocalRayResult& RayResult, bool bNormalInWorldSpace) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( RayResult.m_collisionObject, RayResult.m_localShapeInfo, RayResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        // caller already does the filter on the m_closestHitFraction
        btAssert(RayResult.m_hitFraction <= m_closestHitFraction);

        m_closestHitFraction = RayResult.m_hitFraction;
        m_collisionObject = RayResult.m_collisionObject;
        m_hitNormalWorld = bNormalInWorldSpace ? RayResult.m_hitNormalLocal : m_collisionObject->getWorldTransform().getBasis() * RayResult.m_hitNormalLocal;
        m_hitPointWorld.setInterpolate3(m_rayFromWorld, m_rayToWorld, RayResult.m_hitFraction);
        return RayResult.m_hitFraction;
    }

    CollisionQueryFilter const& QueryFilter;
    btVector3 m_rayFromWorld;
    btVector3 m_rayToWorld;
    btVector3 m_hitPointWorld;
    btVector3 m_hitNormalWorld;
};

struct TraceClosestConvexResultCallback : btCollisionWorld::ConvexResultCallback
{
    TraceClosestConvexResultCallback(CollisionQueryFilter const* _QueryFilter) :
        m_hitCollisionObject(0), QueryFilter(_QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter)
    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }


    btVector3 m_hitNormalWorld;
    btVector3 m_hitPointWorld;
    const btCollisionObject* m_hitCollisionObject;

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        return NeedsCollision(QueryFilter, proxy0);
    }

    btScalar addSingleResult(btCollisionWorld::LocalConvexResult& ConvexResult, bool bNormalInWorldSpace) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( ConvexResult.m_hitCollisionObject, ConvexResult.m_localShapeInfo, ConvexResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        // caller already does the filter on the m_closestHitFraction
        btAssert(ConvexResult.m_hitFraction <= m_closestHitFraction);

        m_closestHitFraction = ConvexResult.m_hitFraction;
        m_hitCollisionObject = ConvexResult.m_hitCollisionObject;
        if (bNormalInWorldSpace)
        {
            m_hitNormalWorld = ConvexResult.m_hitNormalLocal;
        }
        else
        {
            // need to transform normal into worldspace
            m_hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis() * ConvexResult.m_hitNormalLocal;
        }
        m_hitPointWorld = ConvexResult.m_hitPointLocal;
        return ConvexResult.m_hitFraction;
    }

    CollisionQueryFilter const& QueryFilter;
};

struct TraceConvexResultCallback : btCollisionWorld::ConvexResultCallback
{
    TraceConvexResultCallback(CollisionQueryFilter const* _QueryFilter, float _RayLength, TPodVector<CollisionTraceResult>& _Result) :
        RayLength(_RayLength), QueryFilter(_QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter), Result(_Result)
    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        return NeedsCollision(QueryFilter, proxy0);
    }

    btScalar addSingleResult(btCollisionWorld::LocalConvexResult& ConvexResult, bool bNormalInWorldSpace) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( ConvexResult.m_hitCollisionObject, ConvexResult.m_localShapeInfo, ConvexResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        const btCollisionObject* hitCollisionObject = ConvexResult.m_hitCollisionObject;

        CollisionTraceResult& hit = Result.Add();
        hit.Clear();
        hit.HitProxy = static_cast<HitProxy*>(hitCollisionObject->getUserPointer());
        hit.Position = btVectorToFloat3(ConvexResult.m_hitPointLocal);
        hit.Normal = bNormalInWorldSpace ? btVectorToFloat3(ConvexResult.m_hitNormalLocal) : btVectorToFloat3(hitCollisionObject->getWorldTransform().getBasis() * ConvexResult.m_hitNormalLocal);
        hit.Distance = ConvexResult.m_hitFraction * RayLength;
        hit.Fraction = ConvexResult.m_hitFraction;

        return m_closestHitFraction;
    }

    float RayLength;
    CollisionQueryFilter const& QueryFilter;
    TPodVector<CollisionTraceResult>& Result;
};

bool PhysicsSystem::Trace(TPodVector<CollisionTraceResult>& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter) const
{
    if (!_QueryFilter)
    {
        _QueryFilter = &DefaultCollisionQueryFilter;
    }

    _Result.Clear();

    Float3 rayDir = _RayEnd - _RayStart;

    TraceRayResultCallback hitResult(_QueryFilter, _RayStart, rayDir, _Result);

    m_DynamicsWorld->rayTest(btVectorToFloat3(_RayStart), btVectorToFloat3(_RayEnd), hitResult);

    if (_QueryFilter->bSortByDistance)
    {
        std::sort(_Result.Begin(), _Result.End(), CompareDistance);
    }

    return !_Result.IsEmpty();
}

bool PhysicsSystem::TraceClosest(CollisionTraceResult& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter) const
{
    TraceClosestRayResultCallback hitResult(_QueryFilter, btVectorToFloat3(_RayStart), btVectorToFloat3(_RayEnd));

    m_DynamicsWorld->rayTest(hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult);

    _Result.Clear();

    if (!hitResult.hasHit())
    {
        return false;
    }

    _Result.HitProxy = static_cast<HitProxy*>(hitResult.m_collisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3(hitResult.m_hitPointWorld);
    _Result.Normal = btVectorToFloat3(hitResult.m_hitNormalWorld);
    _Result.Distance = (_Result.Position - _RayStart).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool PhysicsSystem::TraceSphere(CollisionTraceResult& _Result, float _Radius, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter) const
{
    TraceClosestConvexResultCallback hitResult(_QueryFilter);

    btSphereShape shape(_Radius);
    shape.setMargin(0.0f);

    m_DynamicsWorld->convexSweepTest(&shape,
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(_RayStart)),
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(_RayEnd)), hitResult);

    _Result.Clear();

    if (!hitResult.hasHit())
    {
        return false;
    }

    _Result.HitProxy = static_cast<HitProxy*>(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3(hitResult.m_hitPointWorld);
    _Result.Normal = btVectorToFloat3(hitResult.m_hitNormalWorld);
    _Result.Distance = hitResult.m_closestHitFraction * (_RayEnd - _RayStart).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool PhysicsSystem::TraceBox(CollisionTraceResult& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter) const
{
    Float3 boxPosition = (_Maxs + _Mins) * 0.5f;
    Float3 halfExtents = (_Maxs - _Mins) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    TraceClosestConvexResultCallback hitResult(_QueryFilter);

    btBoxShape shape(btVectorToFloat3(halfExtents));
    shape.setMargin(0.0f);

    m_DynamicsWorld->convexSweepTest(&shape,
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(startPos)),
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(endPos)), hitResult);

    _Result.Clear();

    if (!hitResult.hasHit())
    {
        return false;
    }

    _Result.HitProxy = static_cast<HitProxy*>(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3(hitResult.m_hitPointWorld);
    _Result.Normal = btVectorToFloat3(hitResult.m_hitNormalWorld);
    _Result.Distance = hitResult.m_closestHitFraction * (endPos - startPos).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

// TODO: Check TraceBox2 and add TraceSphere2, TraceCylinder2 etc
bool PhysicsSystem::TraceBox2(TPodVector<CollisionTraceResult>& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter) const
{
    Float3 boxPosition = (_Maxs + _Mins) * 0.5f;
    Float3 halfExtents = (_Maxs - _Mins) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;
    float rayLength = (endPos - startPos).Length();

    _Result.Clear();

    TraceConvexResultCallback hitResult(_QueryFilter, rayLength, _Result);

    btBoxShape shape(btVectorToFloat3(halfExtents));
    shape.setMargin(0.0f);

    m_DynamicsWorld->convexSweepTest(&shape,
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(startPos)),
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(endPos)), hitResult);

    return !_Result.IsEmpty();
}

bool PhysicsSystem::TraceCylinder(CollisionTraceResult& _Result, Float3 const& _Mins, Float3 const& _Maxs, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter) const
{
    Float3 boxPosition = (_Maxs + _Mins) * 0.5f;
    Float3 halfExtents = (_Maxs - _Mins) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    TraceClosestConvexResultCallback hitResult(_QueryFilter);

    btCylinderShape shape(btVectorToFloat3(halfExtents));
    shape.setMargin(0.0f);

    m_DynamicsWorld->convexSweepTest(&shape,
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(startPos)),
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(endPos)), hitResult);

    _Result.Clear();

    if (!hitResult.hasHit())
    {
        return false;
    }

    _Result.HitProxy = static_cast<HitProxy*>(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3(hitResult.m_hitPointWorld);
    _Result.Normal = btVectorToFloat3(hitResult.m_hitNormalWorld);
    _Result.Distance = hitResult.m_closestHitFraction * (endPos - startPos).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool PhysicsSystem::TraceCapsule(CollisionTraceResult& _Result, float _CapsuleHeight, float CapsuleRadius, Float3 const& _RayStart, Float3 const& _RayEnd, CollisionQueryFilter const* _QueryFilter) const
{
    TraceClosestConvexResultCallback hitResult(_QueryFilter);

    btCapsuleShape shape(CapsuleRadius, _CapsuleHeight);
    shape.setMargin(0.0f);

    m_DynamicsWorld->convexSweepTest(&shape,
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(_RayStart)),
                                     btTransform(btQuaternion::getIdentity(), btVectorToFloat3(_RayEnd)), hitResult);

    _Result.Clear();

    if (!hitResult.hasHit())
    {
        return false;
    }

    _Result.HitProxy = static_cast<HitProxy*>(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3(hitResult.m_hitPointWorld);
    _Result.Normal = btVectorToFloat3(hitResult.m_hitNormalWorld);
    _Result.Distance = hitResult.m_closestHitFraction * (_RayEnd - _RayStart).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool PhysicsSystem::TraceConvex(CollisionTraceResult& _Result, ConvexSweepTest const& _SweepTest) const
{
    _Result.Clear();

    Float3x4 startTransform, endTransform;

    startTransform.Compose(_SweepTest.StartPosition, _SweepTest.StartRotation.ToMatrix3x3());
    endTransform.Compose(_SweepTest.EndPosition, _SweepTest.EndRotation.ToMatrix3x3());

    TUniqueRef<btConvexShape> shape;
    Float3 position;
    Quat rotation;
    float margin;

    COLLISION_SHAPE type = _SweepTest.CollisionSphere->Type;
    switch (type)
    {
        case COLLISION_SHAPE_SPHERE: {
            shape.Reset(new btSphereShape(_SweepTest.CollisionSphere->Radius));
            position = _SweepTest.CollisionSphere->Position;
            rotation = Quat::Identity();
            margin = _SweepTest.CollisionSphere->Margin;
            break;
        }
        case COLLISION_SHAPE_SPHERE_RADII: {
            btVector3 pos(0, 0, 0);
            float radius = 1.0f;
            shape.Reset(new btMultiSphereShape(&pos, &radius, 1));
            shape->setLocalScaling(btVectorToFloat3(_SweepTest.CollisionSphereRADII->Radius));
            position = _SweepTest.CollisionSphereRADII->Position;
            rotation = _SweepTest.CollisionSphereRADII->Rotation;
            margin = _SweepTest.CollisionSphereRADII->Margin;
            break;
        }
        case COLLISION_SHAPE_BOX:
            shape.Reset(new btBoxShape(btVectorToFloat3(_SweepTest.CollisionBox->HalfExtents)));
            position = _SweepTest.CollisionBox->Position;
            rotation = _SweepTest.CollisionBox->Rotation;
            margin = _SweepTest.CollisionBox->Margin;
            break;
        case COLLISION_SHAPE_CYLINDER:
            switch (_SweepTest.CollisionCylinder->Axial)
            {
                case COLLISION_SHAPE_AXIAL_X:
                    shape.Reset(new btCylinderShapeX(btVector3(_SweepTest.CollisionCylinder->Height * 0.5f, _SweepTest.CollisionCylinder->Radius, _SweepTest.CollisionCylinder->Radius)));
                    break;
                case COLLISION_SHAPE_AXIAL_Z:
                    shape.Reset(new btCylinderShapeZ(btVector3(_SweepTest.CollisionCylinder->Radius, _SweepTest.CollisionCylinder->Radius, _SweepTest.CollisionCylinder->Height * 0.5f)));
                    break;
                case COLLISION_SHAPE_AXIAL_Y:
                default:
                    shape.Reset(new btCylinderShape(btVector3(_SweepTest.CollisionCylinder->Radius, _SweepTest.CollisionCylinder->Height * 0.5f, _SweepTest.CollisionCylinder->Radius)));
                    break;
            }
            position = _SweepTest.CollisionCylinder->Position;
            rotation = _SweepTest.CollisionCylinder->Rotation;
            margin = _SweepTest.CollisionCylinder->Margin;
            break;
        case COLLISION_SHAPE_CONE:
            switch (_SweepTest.CollisionCone->Axial)
            {
                case COLLISION_SHAPE_AXIAL_X:
                    shape.Reset(new btConeShapeX(_SweepTest.CollisionCone->Radius, _SweepTest.CollisionCone->Height));
                    break;
                case COLLISION_SHAPE_AXIAL_Y:
                    shape.Reset(new btConeShape(_SweepTest.CollisionCone->Radius, _SweepTest.CollisionCone->Height));
                    break;
                case COLLISION_SHAPE_AXIAL_Z:
                    shape.Reset(new btConeShapeZ(_SweepTest.CollisionCone->Radius, _SweepTest.CollisionCone->Height));
                    break;
                default:
                    shape.Reset(new btConeShape(_SweepTest.CollisionCone->Radius, _SweepTest.CollisionCone->Height));
                    break;
            }
            position = _SweepTest.CollisionCone->Position;
            rotation = _SweepTest.CollisionCone->Rotation;
            margin = _SweepTest.CollisionCone->Margin;
            break;
        case COLLISION_SHAPE_CAPSULE:
            switch (_SweepTest.CollisionCapsule->Axial)
            {
                case COLLISION_SHAPE_AXIAL_X:
                    shape.Reset(new btCapsuleShapeX(_SweepTest.CollisionCapsule->Radius, _SweepTest.CollisionCapsule->Height));
                    break;
                case COLLISION_SHAPE_AXIAL_Y:
                    shape.Reset(new btCapsuleShape(_SweepTest.CollisionCapsule->Radius, _SweepTest.CollisionCapsule->Height));
                    break;
                case COLLISION_SHAPE_AXIAL_Z:
                    shape.Reset(new btCapsuleShapeZ(_SweepTest.CollisionCapsule->Radius, _SweepTest.CollisionCapsule->Height));
                    break;
                default:
                    shape.Reset(new btCapsuleShape(_SweepTest.CollisionCapsule->Radius, _SweepTest.CollisionCapsule->Height));
                    break;
            }
            position = _SweepTest.CollisionCapsule->Position;
            rotation = _SweepTest.CollisionCapsule->Rotation;
            margin = _SweepTest.CollisionCapsule->Margin;
            break;
        case COLLISION_SHAPE_CONVEX_HULL:
            shape.Reset(new btConvexHullShape(&_SweepTest.CollisionConvexHull->pVertices[0][0], _SweepTest.CollisionConvexHull->VertexCount, sizeof(Float3)));
            position = _SweepTest.CollisionConvexHull->Position;
            rotation = _SweepTest.CollisionConvexHull->Rotation;
            margin = _SweepTest.CollisionConvexHull->Margin;
            break;
        default:
            LOG("PhysicsSystem::TraceConvex: unsupported collision shape\n");
            return false;
    }

    shape->setMargin(margin);
    Float3 startPos = startTransform * position;
    Float3 endPos = endTransform * position;
    Quat startRot = _SweepTest.StartRotation * rotation;
    Quat endRot = _SweepTest.EndRotation * rotation;

    TraceClosestConvexResultCallback hitResult(&_SweepTest.QueryFilter);

    m_DynamicsWorld->convexSweepTest(shape.GetObject(),
                                     btTransform(btQuaternionToQuat(startRot), btVectorToFloat3(startPos)),
                                     btTransform(btQuaternionToQuat(endRot), btVectorToFloat3(endPos)), hitResult);

    if (!hitResult.hasHit())
    {
        return false;
    }

    _Result.HitProxy = static_cast<HitProxy*>(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3(hitResult.m_hitPointWorld);
    _Result.Normal = btVectorToFloat3(hitResult.m_hitNormalWorld);
    _Result.Distance = hitResult.m_closestHitFraction * (endPos - startPos).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

struct QueryCollisionObjectsCallback : public btCollisionWorld::ContactResultCallback
{
    QueryCollisionObjectsCallback(TPodVector<HitProxy*>& _Result, CollisionQueryFilter const* _QueryFilter) :
        Result(_Result), QueryFilter(_QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter)
    {
        _Result.Clear();

        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        return NeedsCollision(QueryFilter, proxy0);
    }

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
    {
        HitProxy* hitProxy;

        hitProxy = static_cast<HitProxy*>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask))
        {
            AddUnique(hitProxy);
        }

        hitProxy = static_cast<HitProxy*>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask))
        {
            AddUnique(hitProxy);
        }

        return 0.0f;
    }

    void AddUnique(HitProxy* hitProxy)
    {
        for (HitProxy* proxy : Result)
        {
            if (proxy->Id == hitProxy->Id)
            {
                return;
            }
        }
        Result.Add(hitProxy);
    }

    TPodVector<HitProxy*>& Result;
    CollisionQueryFilter const& QueryFilter;
};

struct QueryCollisionCallback : public btCollisionWorld::ContactResultCallback
{
    QueryCollisionCallback(TPodVector<CollisionQueryResult>& _Result, CollisionQueryFilter const* _QueryFilter) :
        Result(_Result), QueryFilter(_QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter)
    {
        _Result.Clear();

        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        return NeedsCollision(QueryFilter, proxy0);
    }

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
    {
        HitProxy* hitProxy;

        hitProxy = static_cast<HitProxy*>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask))
        {
            AddContact(hitProxy, cp);
        }

        hitProxy = static_cast<HitProxy*>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask))
        {
            AddContact(hitProxy, cp);
        }

        return 0.0f;
    }

    void AddContact(HitProxy* HitProxy, btManifoldPoint& cp)
    {
        CollisionQueryResult& contact = Result.Add();

        contact.HitProxy = HitProxy;
        contact.Position = btVectorToFloat3(cp.m_positionWorldOnB);
        contact.Normal = btVectorToFloat3(cp.m_normalWorldOnB);
        contact.Distance = cp.m_distance1; // FIXME?
        //contact.Fraction; // FIXME

        //contact.Fraction = contact.Distance / RayLength;

        //contact.Impulse = cp.m_appliedImpulse;
    }

    TPodVector<CollisionQueryResult>& Result;
    CollisionQueryFilter const& QueryFilter;
};

struct QueryActorsCallback : public btCollisionWorld::ContactResultCallback
{
    QueryActorsCallback(TPodVector<Actor*>& _Result, CollisionQueryFilter const* _QueryFilter) :
        Result(_Result), QueryFilter(_QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter)
    {
        _Result.Clear();

        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision(btBroadphaseProxy* proxy0) const override
    {
        return NeedsCollision(QueryFilter, proxy0);
    }

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
    {
        HitProxy* hitProxy;

        hitProxy = static_cast<HitProxy*>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask))
        {
            AddUnique(hitProxy->GetOwnerActor());
        }

        hitProxy = static_cast<HitProxy*>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask))
        {
            AddUnique(hitProxy->GetOwnerActor());
        }

        return 0.0f;
    }

    void AddUnique(Actor* Actor)
    {
        Result.AddUnique(Actor);
    }

    TPodVector<Actor*>& Result;
    CollisionQueryFilter const& QueryFilter;
};

static void CollisionShapeContactTest(btDiscreteDynamicsWorld const* InWorld, Float3 const& InPosition, btCollisionShape* InShape, btCollisionWorld::ContactResultCallback& InCallback)
{
    btRigidBody tempBody(0.0f, nullptr, InShape);
    tempBody.setWorldTransform(btTransform(btQuaternion::getIdentity(), btVectorToFloat3(InPosition)));

    btDiscreteDynamicsWorld* physWorld = const_cast<btDiscreteDynamicsWorld*>(InWorld);
    physWorld->contactTest(&tempBody, InCallback);
}

void PhysicsSystem::QueryHitProxies_Sphere(TPodVector<HitProxy*>& _Result, Float3 const& _Position, float _Radius, CollisionQueryFilter const* _QueryFilter) const
{
    QueryCollisionObjectsCallback callback(_Result, _QueryFilter);
    btSphereShape shape(_Radius);
    shape.setMargin(0.0f);
    CollisionShapeContactTest(m_DynamicsWorld.GetObject(), _Position, &shape, callback);
}

void PhysicsSystem::QueryHitProxies_Box(TPodVector<HitProxy*>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, CollisionQueryFilter const* _QueryFilter) const
{
    QueryCollisionObjectsCallback callback(_Result, _QueryFilter);
    btBoxShape shape(btVectorToFloat3(_HalfExtents));
    shape.setMargin(0.0f);
    CollisionShapeContactTest(m_DynamicsWorld.GetObject(), _Position, &shape, callback);
}

void PhysicsSystem::QueryHitProxies(TPodVector<HitProxy*>& _Result, BvAxisAlignedBox const& _BoundingBox, CollisionQueryFilter const* _QueryFilter) const
{
    QueryHitProxies_Box(_Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter);
}

void PhysicsSystem::QueryActors_Sphere(TPodVector<Actor*>& _Result, Float3 const& _Position, float _Radius, CollisionQueryFilter const* _QueryFilter) const
{
    QueryActorsCallback callback(_Result, _QueryFilter);
    btSphereShape shape(_Radius);
    shape.setMargin(0.0f);
    CollisionShapeContactTest(m_DynamicsWorld.GetObject(), _Position, &shape, callback);
}

void PhysicsSystem::QueryActors_Box(TPodVector<Actor*>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, CollisionQueryFilter const* _QueryFilter) const
{
    QueryActorsCallback callback(_Result, _QueryFilter);
    btBoxShape shape(btVectorToFloat3(_HalfExtents));
    shape.setMargin(0.0f);
    CollisionShapeContactTest(m_DynamicsWorld.GetObject(), _Position, &shape, callback);
}

void PhysicsSystem::QueryActors(TPodVector<Actor*>& _Result, BvAxisAlignedBox const& _BoundingBox, CollisionQueryFilter const* _QueryFilter) const
{
    QueryActors_Box(_Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter);
}

void PhysicsSystem::QueryCollision_Sphere(TPodVector<CollisionQueryResult>& _Result, Float3 const& _Position, float _Radius, CollisionQueryFilter const* _QueryFilter) const
{
    QueryCollisionCallback callback(_Result, _QueryFilter);
    btSphereShape shape(_Radius);
    shape.setMargin(0.0f);
    CollisionShapeContactTest(m_DynamicsWorld.GetObject(), _Position, &shape, callback);
}

void PhysicsSystem::QueryCollision_Box(TPodVector<CollisionQueryResult>& _Result, Float3 const& _Position, Float3 const& _HalfExtents, CollisionQueryFilter const* _QueryFilter) const
{
    QueryCollisionCallback callback(_Result, _QueryFilter);
    btBoxShape shape(btVectorToFloat3(_HalfExtents));
    shape.setMargin(0.0f);
    CollisionShapeContactTest(m_DynamicsWorld.GetObject(), _Position, &shape, callback);
}

void PhysicsSystem::QueryCollision(TPodVector<CollisionQueryResult>& _Result, BvAxisAlignedBox const& _BoundingBox, CollisionQueryFilter const* _QueryFilter) const
{
    QueryCollision_Box(_Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter);
}

HK_NAMESPACE_END
