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

#include "SceneComponent.h"

#include "Collision.h"

struct SCharacterControllerTrace
{
    AHitProxy* HitProxy;
    Float3     Position;
    Float3     Normal;
    float      Fraction;

    SCharacterControllerTrace() :
        HitProxy(nullptr), Position(0, 0, 0), Normal(0, 1, 0), Fraction(1)
    {
    }

    void Clear()
    {
        HitProxy = nullptr;
        Position.Clear();
        Normal   = Float3(0, 1, 0);
        Fraction = 1;
    }

    bool HasHit() const
    {
        return Fraction < 1.0f;
    }
};

struct SCharacterControllerContact
{
    AHitProxy* HitProxy;
    Float3     Position;
    Float3     Normal;
};

class ACharacterControllerBase : public ASceneComponent
{
    AN_COMPONENT(ACharacterControllerBase, ASceneComponent)

public:
    AHitProxy* GetHitProxy() const
    {
        return HitProxy;
    }

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    void SetDispatchContactEvents(bool bDispatch)
    {
        HitProxy->bDispatchContactEvents = bDispatch;
    }

    bool ShouldDispatchContactEvents() const
    {
        return HitProxy->bDispatchContactEvents;
    }

    /** Dispatch overlap events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap) */
    void SetDispatchOverlapEvents(bool bDispatch)
    {
        HitProxy->bDispatchOverlapEvents = bDispatch;
    }

    bool ShouldDispatchOverlapEvents() const
    {
        return HitProxy->bDispatchOverlapEvents;
    }

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    void SetGenerateContactPoints(bool bGenerate)
    {
        HitProxy->bGenerateContactPoints = bGenerate;
    }

    bool ShouldGenerateContactPoints() const
    {
        return HitProxy->bGenerateContactPoints;
    }

    /** Set collision group/layer. See ECollisionMask. */
    void SetCollisionGroup(int _CollisionGroup);

    /** Get collision group. See ECollisionMask. */
    int GetCollisionGroup() const { return HitProxy->GetCollisionGroup(); }

    /** Set collision mask. See ECollisionMask. */
    void SetCollisionMask(int _CollisionMask);

    /** Get collision mask. See ECollisionMask. */
    int GetCollisionMask() const { return HitProxy->GetCollisionMask(); }

    /** Set collision group and mask. See ECollisionMask. */
    void SetCollisionFilter(int _CollisionGroup, int _CollisionMask);

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor(AActor* _Actor);

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor(AActor* _Actor);

    void SetCapsuleHeight(float _CapsuleHeight)
    {
        CapsuleHeight        = _CapsuleHeight;
        bNeedToUpdateCapsule = true;
    }

    float GetCapsuleHeight() const
    {
        return CapsuleHeight;
    }

    void SetCapsuleRadius(float _CapsuleRadius)
    {
        CapsuleRadius        = _CapsuleRadius;
        bNeedToUpdateCapsule = true;
    }

    float GetCapsuleRadius() const
    {
        return CapsuleRadius;
    }

    float GetCharacterHeight() const
    {
        return CapsuleHeight + CapsuleRadius * 2;
    }

    float GetCharacterRadius() const
    {
        return GetCapsuleRadius();
    }

    void SetCharacterYaw(float _Yaw);

    float GetCharacterYaw() const
    {
        return AngleYaw;
    }

    void SetCharacterPitch(float _Pitch);

    float GetCharacterPitch() const
    {
        return AnglePitch;
    }

    Float3 GetCenterWorldPosition() const;

    void TraceSelf(Float3 const& Start, Float3 const& End, Float3 const& Up, float MinSlopeDot, SCharacterControllerTrace& Trace, bool bCylinder = false) const;

    void TraceSelf(Float3 const& Start, Float3 const& End, SCharacterControllerTrace& Trace, bool bCylinder = false) const;

    void RecoverFromPenetration(float MaxPenetrationDepth, int MaxIterations);

    void SlideMove(Float3 const& StartPos, Float3 const& TargetPos, float TimeStep, Float3& FinalPos, bool* bClipped = nullptr, TPodVector<SCharacterControllerContact>* pContacts = nullptr);

    void SlideMove(Float3 const& StartPos, Float3 const& LinearVelocity, float TimeStep, Float3& FinalPos, Float3& FinalVelocity, bool* bClipped = nullptr, TPodVector<SCharacterControllerContact>* pContacts = nullptr);

protected:
    ACharacterControllerBase();

    void InitializeComponent() override;

    void DeinitializeComponent() override;

    void BeginPlay() override;

    void OnTransformDirty() override;

    void DrawDebug(ADebugRenderer* InRenderer) override;

    void UpdateCapsuleShape();

    void SetCapsuleWorldPosition(Float3 const& Position);

    virtual void Update(float _TimeStep) {}

    void ClipVelocity(Float3 const& InVelocity, Float3 const& InNormal, Float3& OutVelocity, float Overbounce = 1.001f);

private:
    friend class ACharacterControllerActionInterface;
    void _Update(float _TimeStep);

    bool _RecoverFromPenetration(float MaxPenetrationDepth);

    void CalcYawAndPitch(float& Yaw, float& Pitch);

    Quat GetAngleQuaternion() const;

    bool ClipVelocityByContactNormals(Float3 const* ContactNormals, int NumContacts, Float3& Velocity);

    // Collision hit proxy
    TRef<AHitProxy> HitProxy;

    class ACharacterControllerActionInterface* ActionInterface;
    class btPairCachingGhostObject*            GhostObject;
    class btConvexShape*                       ConvexShape;
    class btConvexShape*                       CylinderShape;
    class btDiscreteDynamicsWorld*             World;

    float AnglePitch;
    float AngleYaw;

    // Attributes
    float CapsuleRadius = 0.5f;
    float CapsuleHeight = 0.9f;

    bool bNeedToUpdateCapsule = false;
    bool bInsideUpdate        = false;
};

struct SProjectileTrace
{
    AHitProxy* HitProxy;
    Float3     Position;
    Float3     Normal;
    float      Fraction;

    SProjectileTrace() :
        HitProxy(nullptr), Position(0, 0, 0), Normal(0, 1, 0), Fraction(1)
    {
    }

    void Clear()
    {
        HitProxy = nullptr;
        Position.Clear();
        Normal   = Float3(0, 1, 0);
        Fraction = 1;
    }

    bool HasHit() const
    {
        return Fraction < 1.0f;
    }
};

class AProjectileExperimental : public ASceneComponent
{
    AN_COMPONENT(AProjectileExperimental, ASceneComponent)

public:
    TEvent<AHitProxy*, Float3 const&, Float3 const&> OnHit;

    AHitProxy* GetHitProxy() const
    {
        return HitProxy;
    }

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    void SetDispatchContactEvents(bool bDispatch)
    {
        HitProxy->bDispatchContactEvents = bDispatch;
    }

    bool ShouldDispatchContactEvents() const
    {
        return HitProxy->bDispatchContactEvents;
    }

    /** Dispatch overlap events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap) */
    void SetDispatchOverlapEvents(bool bDispatch)
    {
        HitProxy->bDispatchOverlapEvents = bDispatch;
    }

    bool ShouldDispatchOverlapEvents() const
    {
        return HitProxy->bDispatchOverlapEvents;
    }

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    void SetGenerateContactPoints(bool bGenerate)
    {
        HitProxy->bGenerateContactPoints = bGenerate;
    }

    bool ShouldGenerateContactPoints() const
    {
        return HitProxy->bGenerateContactPoints;
    }

    /** Set collision group/layer. See ECollisionMask. */
    void SetCollisionGroup(int _CollisionGroup);

    /** Get collision group. See ECollisionMask. */
    int GetCollisionGroup() const { return HitProxy->GetCollisionGroup(); }

    /** Set collision mask. See ECollisionMask. */
    void SetCollisionMask(int _CollisionMask);

    /** Get collision mask. See ECollisionMask. */
    int GetCollisionMask() const { return HitProxy->GetCollisionMask(); }

    /** Set collision group and mask. See ECollisionMask. */
    void SetCollisionFilter(int _CollisionGroup, int _CollisionMask);

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor(AActor* _Actor);

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor(AActor* _Actor);

    void TraceSelf(Float3 const& Start, Float3 const& End, SProjectileTrace& Trace) const;
    void TraceSelf(Float3 const& Start, Quat const& StartRot, Float3 const& End, Quat const& EndRot, SProjectileTrace& Trace) const;

    Float3 LinearVelocity  = Float3(0.0f);
    Float3 AngularVelocity = Float3(0.0f);

    void ApplyForce(Float3 const& force, Float3 const& rel_pos);

    void ApplyTorque(Float3 const& torque);

    void ApplyCentralForce(Float3 const& force);

    Float3 m_totalTorque = Float3(0.0f);
    Float3 m_totalForce  = Float3(0.0f);

    void ClearForces();

protected:
    AProjectileExperimental();

    void InitializeComponent() override;

    void DeinitializeComponent() override;

    void BeginPlay() override;

    void OnTransformDirty() override;

    void DrawDebug(ADebugRenderer* InRenderer) override;

    //void UpdateCapsuleShape();

    //void SetCapsuleWorldPosition( Float3 const & Position );

    virtual void Update(float _TimeStep); // {}

    //void ClipVelocity( Float3 const & InVelocity, Float3 const & InNormal, Float3 & OutVelocity, float Overbounce );

private:
    friend class AProjectileActionInterface;
    void _Update(float _TimeStep);

    void HandlePostPhysicsUpdate(float TimeStep);

    // Collision hit proxy
    TRef<AHitProxy> HitProxy;

    class AProjectileActionInterface* ActionInterface;
    //class btPairCachingGhostObject * GhostObject;
    class btGhostObject*           GhostObject;
    class btConvexShape*           ConvexShape;
    class btDiscreteDynamicsWorld* World;

    bool bInsideUpdate = false;
};
