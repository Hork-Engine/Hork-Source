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
#include "CollisionEvents.h"

//CM_SOLID
//CM_PLAYER_CLIP
//CM_MONSTER_CLIP

enum ECollisionMask
{
    CM_NOCOLLISION          = 0,
    CM_WORLD_STATIC         = 1,
    CM_WORLD_DYNAMIC        = 2,
    CM_WORLD                = CM_WORLD_STATIC | CM_WORLD_DYNAMIC,
    CM_PAWN                 = 4,
    CM_PROJECTILE           = 8,
    CM_TRIGGER              = 16,
    CM_CHARACTER_CONTROLLER = 32,
    CM_WATER                = 64,
    CM_SOLID                = CM_WORLD_STATIC | CM_WORLD_DYNAMIC | CM_PAWN | CM_PROJECTILE | CM_CHARACTER_CONTROLLER,
    CM_UNUSED7              = 128,
    CM_UNUSED8              = 256,
    CM_UNUSED9              = 512,
    CM_UNUSED10             = 1024,
    CM_UNUSED11             = 1024,
    CM_UNUSED12             = 2048,
    CM_UNUSED13             = 4096,
    CM_UNUSED14             = 8192,
    CM_UNUSED15             = 16384,
    CM_ALL                  = 0xffffffff
};

class AHitProxy : public ABaseObject
{
    HK_CLASS(AHitProxy, ABaseObject)

    friend class AWorldPhysics;

public:
    // Component events
    AContactDelegate E_OnBeginContact;
    AContactDelegate E_OnEndContact;
    AContactDelegate E_OnUpdateContact;
    AOverlapDelegate E_OnBeginOverlap;
    AOverlapDelegate E_OnEndOverlap;
    AOverlapDelegate E_OnUpdateOverlap;

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    bool bDispatchContactEvents = false;

    /** Dispatch overlap events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap) */
    bool bDispatchOverlapEvents = false;

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    bool bGenerateContactPoints = false;

    void Initialize(ASceneComponent* _OwnerComponent, class btCollisionObject* _CollisionObject);
    void Deinitialize();

    ASceneComponent* GetOwnerComponent() const { return OwnerComponent; }

    AActor* GetOwnerActor() const { return OwnerComponent->GetOwnerActor(); }

    AWorld* GetWorld() const { return OwnerComponent->GetWorld(); }

    /** Set collision group/layer. See ECollisionMask. */
    void SetCollisionGroup(int _CollisionGroup);

    /** Get collision group. See ECollisionMask. */
    int GetCollisionGroup() const { return CollisionGroup; }

    /** Set collision mask. See ECollisionMask. */
    void SetCollisionMask(int _CollisionMask);

    /** Get collision mask. See ECollisionMask. */
    int GetCollisionMask() const { return CollisionMask; }

    /** Set collision group and mask. See ECollisionMask. */
    void SetCollisionFilter(int _CollisionGroup, int _CollisionMask);

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor(AActor* _Actor);

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor(AActor* _Actor);

    void SetTrigger(bool _Trigger) { bTrigger = _Trigger; }

    bool IsTrigger() const { return bTrigger; }

    void SetJointIndex(int _JointIndex) { JointIndex = _JointIndex; }

    int GetJointIndex() const { return JointIndex; }

    TPodVector<AActor*, 1> const& GetCollisionIgnoreActors() const { return CollisionIgnoreActors; }

    void CollisionContactQuery(TPodVector<AHitProxy*>& _Result) const;

    void CollisionContactQueryActor(TPodVector<AActor*>& _Result) const;

    class btCollisionObject* GetCollisionObject() const { return CollisionObject; }

    void DrawCollisionShape(ADebugRenderer* InRenderer);

    void UpdateBroadphase();

    AHitProxy();
    ~AHitProxy();

private:
    ASceneComponent*   OwnerComponent  = nullptr;
    btCollisionObject* CollisionObject = nullptr;

    int CollisionGroup = CM_WORLD_STATIC;
    int CollisionMask  = CM_ALL;

    int JointIndex = 0;

    bool bTrigger = false;

    bool bInWorld = false;

    TPodVector<AActor*, 1> CollisionIgnoreActors;

    AHitProxy* NextMarked = nullptr;
    AHitProxy* PrevMarked = nullptr;
};
