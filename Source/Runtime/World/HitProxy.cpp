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

#include "HitProxy.h"
#include "World.h"

#include <Runtime/BulletCompatibility.h>

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
    ContactQueryCallback(TPodVector<HitProxy*>& _Result, int _CollisionGroup, int _CollisionMask, HitProxy const* _Self) :
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

    TPodVector<HitProxy*>& Result;
    HitProxy const*        Self;
};

struct ContactQueryActorCallback : public btCollisionWorld::ContactResultCallback
{
    ContactQueryActorCallback(TPodVector<Actor*>& _Result, int _CollisionGroup, int _CollisionMask, Actor const* _Self) :
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

    TPodVector<Actor*>& Result;
    Actor const*        Self;
};

void HitProxy::CollisionContactQuery(TPodVector<HitProxy*>& _Result) const
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

void HitProxy::CollisionContactQueryActor(TPodVector<Actor*>& _Result) const
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

HK_NAMESPACE_END
