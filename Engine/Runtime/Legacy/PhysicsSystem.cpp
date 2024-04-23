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

#if 0

class HitProxy : public GCObject
{
    friend class PhysicsSystem;

public:
    const uint64_t Id;

    // Component events
    ContactDelegate E_OnBeginContact;
    ContactDelegate E_OnEndContact;
    ContactDelegate E_OnUpdateContact;
    OverlapDelegate E_OnBeginOverlap;
    OverlapDelegate E_OnEndOverlap;
    OverlapDelegate E_OnUpdateOverlap;

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    bool bDispatchContactEvents = false;

    /** Dispatch overlap events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap) */
    bool bDispatchOverlapEvents = false;

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    bool bGenerateContactPoints = false;

    void Initialize(SceneComponent* _OwnerComponent, btCollisionObject* _CollisionObject);
    void Deinitialize();

    SceneComponent* GetOwnerComponent() const { return OwnerComponent; }

    Actor* GetOwnerActor() const { return OwnerComponent->GetOwnerActor(); }

    //World* GetWorld() const { return OwnerComponent->GetWorld(); }

    /** Set collision group/layer. See COLLISION_MASK. */
    void SetCollisionGroup(COLLISION_MASK _CollisionGroup);

    /** Get collision group. See COLLISION_MASK. */
    COLLISION_MASK GetCollisionGroup() const { return CollisionGroup; }

    /** Set collision mask. See COLLISION_MASK. */
    void SetCollisionMask(COLLISION_MASK _CollisionMask);

    /** Get collision mask. See COLLISION_MASK. */
    COLLISION_MASK GetCollisionMask() const { return CollisionMask; }

    /** Set collision group and mask. See COLLISION_MASK. */
    void SetCollisionFilter(COLLISION_MASK _CollisionGroup, COLLISION_MASK _CollisionMask);

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor(Actor* _Actor);

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor(Actor* _Actor);

    void SetTrigger(bool _Trigger) { bTrigger = _Trigger; }

    bool IsTrigger() const { return bTrigger; }

    void SetJointIndex(int jointIndex) { JointIndex = jointIndex; }

    int GetJointIndex() const { return JointIndex; }

    TVector<Actor*> const& GetCollisionIgnoreActors() const { return CollisionIgnoreActors; }

    void CollisionContactQuery(TVector<HitProxy*>& _Result) const;

    void CollisionContactQueryActor(TVector<Actor*>& _Result) const;

    btCollisionObject* GetCollisionObject() const { return CollisionObject; }

    void DrawCollisionShape(DebugRenderer* InRenderer);

    void UpdateBroadphase();

    HitProxy();
    ~HitProxy();

private:
    SceneComponent* OwnerComponent  = nullptr;
    btCollisionObject* CollisionObject = nullptr;

    COLLISION_MASK CollisionGroup = CM_WORLD_STATIC;
    COLLISION_MASK CollisionMask  = CM_ALL;

    int JointIndex = 0;

    bool bTrigger = false;

    bool bInWorld = false;

    TVector<Actor*> CollisionIgnoreActors;

    HitProxy* NextMarked = nullptr;
    HitProxy* PrevMarked = nullptr;
};

#    include "World.h"

#    include <Engine/Runtime/BulletCompatibility.h>

HK_NAMESPACE_BEGIN

static uint64_t GUniqueIdGenerator = 0;

HitProxy::HitProxy() :
    Id(++GUniqueIdGenerator)
{}

HitProxy::~HitProxy()
{
    for (Actor* actor : CollisionIgnoreActors)
    {
        actor->RemoveRef();
    }
}

void HitProxy::Initialize(SceneComponent* _OwnerComponent, btCollisionObject* _CollisionObject)
{
    HK_ASSERT(!OwnerComponent);

    OwnerComponent  = _OwnerComponent;
    CollisionObject = _CollisionObject;

    World* world = OwnerComponent->GetWorld();
    world->PhysicsSystem.AddHitProxy(this);
}

void HitProxy::Deinitialize()
{
    if (OwnerComponent)
    {
        World* world = OwnerComponent->GetWorld();
        world->PhysicsSystem.RemoveHitProxy(this);

        OwnerComponent  = nullptr;
        CollisionObject = nullptr;
    }
}

void HitProxy::UpdateBroadphase()
{
    // Re-add collision object to physics world
    if (bInWorld)
    {
        GetWorld()->PhysicsSystem.AddHitProxy(this);
    }
}

void HitProxy::SetCollisionGroup(COLLISION_MASK _CollisionGroup)
{
    if (CollisionGroup == _CollisionGroup)
    {
        return;
    }

    CollisionGroup = _CollisionGroup;

    UpdateBroadphase();
}

void HitProxy::SetCollisionMask(COLLISION_MASK _CollisionMask)
{
    if (CollisionMask == _CollisionMask)
    {
        return;
    }

    CollisionMask = _CollisionMask;

    UpdateBroadphase();
}

void HitProxy::SetCollisionFilter(COLLISION_MASK _CollisionGroup, COLLISION_MASK _CollisionMask)
{
    if (CollisionGroup == _CollisionGroup && CollisionMask == _CollisionMask)
    {
        return;
    }

    CollisionGroup = _CollisionGroup;
    CollisionMask  = _CollisionMask;

    UpdateBroadphase();
}

void HitProxy::AddCollisionIgnoreActor(Actor* _Actor)
{
    if (!_Actor)
    {
        return;
    }
    if (!CollisionIgnoreActors.Contains(_Actor))
    {
        CollisionIgnoreActors.Add(_Actor);
        _Actor->AddRef();

        UpdateBroadphase();
    }
}

void HitProxy::RemoveCollisionIgnoreActor(Actor* _Actor)
{
    if (!_Actor)
    {
        return;
    }
    auto index = CollisionIgnoreActors.IndexOf(_Actor);
    if (index != Core::NPOS)
    {
        _Actor->RemoveRef();

        CollisionIgnoreActors.RemoveUnsorted(index);

        UpdateBroadphase();
    }
}

struct ContactQueryCallback : public btCollisionWorld::ContactResultCallback
{
    ContactQueryCallback(TVector<HitProxy*>& _Result, int _CollisionGroup, int _CollisionMask, HitProxy const* _Self) :
        Result(_Result), Self(_Self)
    {
        m_collisionFilterGroup = _CollisionGroup;
        m_collisionFilterMask  = _CollisionMask;

        Result.Clear();
    }

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
    {
        HitProxy* hitProxy;

        hitProxy = static_cast<HitProxy*>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && hitProxy != Self /*&& (hitProxy->GetCollisionGroup() & CollisionMask)*/)
        {
            AddUnique(hitProxy);
        }

        hitProxy = static_cast<HitProxy*>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && hitProxy != Self /*&& (hitProxy->GetCollisionGroup() & CollisionMask)*/)
        {
            AddUnique(hitProxy);
        }

        return 0.0f;
    }

    void AddUnique(HitProxy* HitProxy)
    {
        Result.AddUnique(HitProxy);
    }

    TVector<HitProxy*>& Result;
    HitProxy const* Self;
};

struct ContactQueryActorCallback : public btCollisionWorld::ContactResultCallback
{
    ContactQueryActorCallback(TVector<Actor*>& _Result, int _CollisionGroup, int _CollisionMask, Actor const* _Self) :
        Result(_Result), Self(_Self)
    {
        m_collisionFilterGroup = _CollisionGroup;
        m_collisionFilterMask  = _CollisionMask;

        Result.Clear();
    }

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
    {
        HitProxy* hitProxy;

        hitProxy = static_cast<HitProxy*>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && hitProxy->GetOwnerActor() != Self /*&& (hitProxy->GetCollisionGroup() & CollisionMask)*/)
        {
            AddUnique(hitProxy->GetOwnerActor());
        }

        hitProxy = static_cast<HitProxy*>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if (hitProxy && hitProxy->GetOwnerActor() != Self /*&& (hitProxy->GetCollisionGroup() & CollisionMask)*/)
        {
            AddUnique(hitProxy->GetOwnerActor());
        }

        return 0.0f;
    }

    void AddUnique(Actor* Actor)
    {
        Result.AddUnique(Actor);
    }

    TVector<Actor*>& Result;
    Actor const* Self;
};

void HitProxy::CollisionContactQuery(TVector<HitProxy*>& _Result) const
{
    ContactQueryCallback callback(_Result, CollisionGroup, CollisionMask, this);

    if (!CollisionObject)
    {
        LOG("HitProxy::CollisionContactQuery: No collision object\n");
        return;
    }

    if (!bInWorld)
    {
        LOG("HitProxy::CollisionContactQuery: The body is not in world\n");
        return;
    }

    GetWorld()->PhysicsSystem.GetInternal()->contactTest(CollisionObject, callback);
}

void HitProxy::CollisionContactQueryActor(TVector<Actor*>& _Result) const
{
    ContactQueryActorCallback callback(_Result, CollisionGroup, CollisionMask, GetOwnerActor());

    if (!CollisionObject)
    {
        LOG("HitProxy::CollisionContactQueryActor: No collision object\n");
        return;
    }

    if (!bInWorld)
    {
        LOG("HitProxy::CollisionContactQueryActor: The body is not in world\n");
        return;
    }

    GetWorld()->PhysicsSystem.GetInternal()->contactTest(CollisionObject, callback);
}

void HitProxy::DrawCollisionShape(DebugRenderer* InRenderer)
{
    if (CollisionObject)
    {
        btDrawCollisionShape(InRenderer, CollisionObject->getWorldTransform(), CollisionObject->getCollisionShape());
    }
}

struct OverlapEvent
{
    Actor*    SelfActor;
    HitProxy* SelfBody;
    Actor*    OtherActor;
    HitProxy* OtherBody;
};

struct ContactPoint
{
    Float3 Position;
    Float3 Normal;
    float  Distance;
    float  Impulse;
};

struct ContactEvent
{
    Actor*              SelfActor;
    HitProxy*           SelfBody;
    Actor*              OtherActor;
    HitProxy*           OtherBody;
    ContactPoint const* Points;
    int                 NumPoints;
};

using OverlapDelegate = TEvent<OverlapEvent const&>;
using ContactDelegate = TEvent<ContactEvent const&>;


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

void PhysicsSystem::OnPostPhysics(btDynamicsWorld* _World, float _TimeStep)
{
    PhysicsSystem* self = static_cast<PhysicsSystem*>(_World->getWorldUserInfo());

    self->DispatchContactAndOverlapEvents();

    self->PostPhysicsCallback(_TimeStep);
    self->m_FixedTickNumber++;
}
#endif
