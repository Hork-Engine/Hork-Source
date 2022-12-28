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

#include "Actor.h"
#include "SceneComponent.h"

#include <Runtime/CollisionEvents.h>
#include <Runtime/CollisionModel.h>

class btCollisionObject;

HK_NAMESPACE_BEGIN

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

    World* GetWorld() const { return OwnerComponent->GetWorld(); }

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

    void SetJointIndex(int _JointIndex) { JointIndex = _JointIndex; }

    int GetJointIndex() const { return JointIndex; }

    TPodVector<Actor*> const& GetCollisionIgnoreActors() const { return CollisionIgnoreActors; }

    void CollisionContactQuery(TPodVector<HitProxy*>& _Result) const;

    void CollisionContactQueryActor(TPodVector<Actor*>& _Result) const;

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

    TPodVector<Actor*> CollisionIgnoreActors;

    HitProxy* NextMarked = nullptr;
    HitProxy* PrevMarked = nullptr;
};

HK_NAMESPACE_END
