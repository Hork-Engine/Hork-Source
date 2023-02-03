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

#include "HitProxy.h"

#include <Engine/Runtime/AINavigationMesh.h>

class btRigidBody;
class btSoftBody;

HK_NAMESPACE_BEGIN

class PhysicalBodyMotionState;
class BoneCollisionInstance;

enum MOTION_BEHAVIOR
{
    /** Static non-movable object */
    MB_STATIC,

    /** Object motion is simulated by physics engine */
    MB_SIMULATED,

    /** Movable object */
    MB_KINEMATIC
};

enum AI_NAVIGATION_BEHAVIOR
{
    /** The body will not be used for navmesh generation */
    AI_NAVIGATION_BEHAVIOR_NONE,

    /** The body will be used for navmesh generation. AI can walk on. */
    AI_NAVIGATION_BEHAVIOR_STATIC,

    /** The body will be used for navmesh generation. AI can't walk on. */
    AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE,

    /** The body is dynamic obstacle. AI can walk on. */
    AI_NAVIGATION_BEHAVIOR_DYNAMIC, // TODO

    /** The body is dynamic obstacle. AI can't walk on. */
    AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE // TODO
};

struct DebugDrawCache
{
    TVector<Float3>       Vertices;
    TVector<unsigned int> Indices;
    bool                  bDirty;
};

class PhysicalBody : public SceneComponent, private NavigationPrimitive
{
    HK_COMPONENT(PhysicalBody, SceneComponent)

    friend struct CollisionFilterCallback;
    friend class PhysicalBodyMotionState;
    friend class PhysicsSystem;

public:
    HitProxy* GetHitProxy() const
    {
        return m_HitProxy;
    }

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    void SetDispatchContactEvents(bool bDispatch)
    {
        m_HitProxy->bDispatchContactEvents = bDispatch;
    }

    bool ShouldDispatchContactEvents() const
    {
        return m_HitProxy->bDispatchContactEvents;
    }

    /** Dispatch overlap events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap) */
    void SetDispatchOverlapEvents(bool bDispatch)
    {
        m_HitProxy->bDispatchOverlapEvents = bDispatch;
    }

    bool ShouldDispatchOverlapEvents() const
    {
        return m_HitProxy->bDispatchOverlapEvents;
    }

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    void SetGenerateContactPoints(bool bGenerate)
    {
        m_HitProxy->bGenerateContactPoints = bGenerate;
    }

    bool ShouldGenerateContactPoints() const
    {
        return m_HitProxy->bGenerateContactPoints;
    }

    /** Set to false if you want to use own collision model and discard collisions from the mesh. */
    void SetUseMeshCollision(bool bUseMeshCollision);

    bool ShouldUseMeshCollision() const
    {
        return m_bUseMeshCollision;
    }

    /** Collision model. */
    void SetCollisionModel(CollisionModel* collisionModel);

    /** Get current collision model */
    CollisionModel* GetCollisionModel() const;

    /** Set object motion behavior: static, simulated, kinematic */
    void SetMotionBehavior(MOTION_BEHAVIOR _MotionBehavior);

    /** Get object motion behavior: static, dynamic, kinematic */
    MOTION_BEHAVIOR GetMotionBehavior() const { return m_MotionBehavior; }

    /** Specifies how the body will be used by navigation mesh generator */
    void SetAINavigationBehavior(AI_NAVIGATION_BEHAVIOR _AINavigationBehavior);

    /** How the body will be used to build AI navigation mesh */
    AI_NAVIGATION_BEHAVIOR GetAINavigationBehavior() const { return m_AINavigationBehavior; }

    /** Trigger can produce overlap events. */
    void SetTrigger(bool _Trigger);

    /** Trigger can produce overlap events. */
    bool IsTrigger() const { return m_HitProxy->IsTrigger(); }

    /** Set to true to disable world gravity. Only for MB_SIMULATED */
    void SetDisableGravity(bool _DisableGravity);

    /** Return true if gravity is disabled for the object. */
    bool IsGravityDisabled() const { return m_bDisableGravity; }

    /** Set to true to override world gravity and use self gravity. Only for MB_SIMULATED */
    void SetOverrideWorldGravity(bool _OverrideWorldGravity);

    /** Return true if gravity is overriden for the object. */
    bool IsWorldGravityOverriden() const { return m_bOverrideWorldGravity; }

    /** Object self gravity, use with bOverrideWorldGravity. Only for MB_SIMULATED */
    void SetSelfGravity(Float3 const& _SelfGravity);

    /** Object self gravity, use with bOverrideWorldGravity. Only for MB_SIMULATED */
    Float3 const& GetSelfGravity() const { return m_SelfGravity; }

    /** Object mass. Only for MB_SIMULATED */
    void SetMass(float _Mass);

    /** Object mass. Only for MB_SIMULATED */
    float GetMass() const { return m_Mass; }

    /** Set collision group/layer. See COLLISION_MASK. */
    void SetCollisionGroup(COLLISION_MASK collisionGroup);

    /** Get collision group. See COLLISION_MASK. */
    COLLISION_MASK GetCollisionGroup() const { return m_HitProxy->GetCollisionGroup(); }

    /** Set collision mask. See COLLISION_MASK. */
    void SetCollisionMask(COLLISION_MASK collisionMask);

    /** Get collision mask. See COLLISION_MASK. */
    COLLISION_MASK GetCollisionMask() const { return m_HitProxy->GetCollisionMask(); }

    /** Set collision group and mask. See COLLISION_MASK. */
    void SetCollisionFilter(COLLISION_MASK collisionGroup, COLLISION_MASK collisionMask);

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor(Actor* actor);

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor(Actor* actor);

    /** Force physics activation */
    void ActivatePhysics();

    /** Is physics active */
    bool IsPhysicsActive() const;

    /** Overwrite linear velocity */
    void SetLinearVelocity(Float3 const& velocity);

    /** Increment linear velocity */
    void AddLinearVelocity(Float3 const& velocity);

    /** Get object velocity. For soft bodies use GetVertexVelocity in SoftMeshComponent. */
    Float3 GetLinearVelocity() const;

    /** Get object velocity at local point. */
    Float3 GetVelocityAtPoint(Float3 const& position) const;

    /** Object linear velocity factor */
    void SetLinearFactor(Float3 const& factor);

    /** Object linear velocity factor */
    Float3 const& GetLinearFactor() const;

    void SetLinearSleepingThreshold(float threshold);

    float GetLinearSleepingThreshold() const;

    void SetLinearDamping(float damping);

    float GetLinearDamping() const;

    /** Overwrite angular velocity */
    void SetAngularVelocity(Float3 const& velocity);

    /** Increment angular velocity */
    void AddAngularVelocity(Float3 const& velocity);

    /** Object angular velocity */
    Float3 GetAngularVelocity() const;

    /** Object angular velocity factor */
    void SetAngularFactor(Float3 const& factor);

    /** Object angular velocity factor */
    Float3 const& GetAngularFactor() const;

    void SetAngularSleepingThreshold(float threshold);

    float GetAngularSleepingThreshold() const;

    void SetAngularDamping(float damping);

    float GetAngularDamping() const;

    void SetFriction(float friction);

    float GetFriction() const;

    void SetAnisotropicFriction(Float3 const& friction);

    Float3 const& GetAnisotropicFriction() const;

    /** The RollingFriction prevents rounded shapes, such as spheres, cylinders and capsules from rolling forever. */
    void SetRollingFriction(float friction);

    /** The RollingFriction prevents rounded shapes, such as spheres, cylinders and capsules from rolling forever. */
    float GetRollingFriction() const;

    /** Best simulation results using zero restitution. */
    void SetRestitution(float restitution);

    /** Best simulation results using zero restitution. */
    float GetRestitution() const;

    /** Keep ContactProcessingThreshold*ContactProcessingThreshold < FLT_MAX */
    void SetContactProcessingThreshold(float threshold);

    float GetContactProcessingThreshold() const;

    /** Continuous collision detection swept radius */
    void SetCcdRadius(float radius);

    /** Continuous collision detection swept radius */
    float GetCcdRadius() const;

    /** Don't do continuous collision detection if the motion (in one step) is less then CcdMotionThreshold */
    void SetCcdMotionThreshold(float threshold);

    float GetCcdMotionThreshold() const;

    Float3 const& GetCenterOfMass() const;

    Float3 GetCenterOfMassWorldPosition() const;

    /** Clear total force and torque. */
    void ClearForces();

    /** Change force by formula:
        TotalForce += Force * LinearFactor

        The force is then applied to the linear velocity during the integration step:
        Velocity += TotalForce / Mass * Step; */
    void ApplyCentralForce(Float3 const& force);

    /** Apply force at specified point. */
    void ApplyForce(Float3 const& force, Float3 const& position);

    void ApplyTorque(Float3 const& torque);

    /** Change linear velocity by formula:
        Velocity += Impulse * LinearFactor / Mass */
    void ApplyCentralImpulse(Float3 const& impulse);

    void ApplyImpulse(Float3 const& impulse, Float3 const& position);

    void ApplyTorqueImpulse(Float3 const& torque);

    void GetCollisionBodiesWorldBounds(TPodVector<BvAxisAlignedBox>& boundingBoxes) const;

    void GetCollisionWorldBounds(BvAxisAlignedBox& boundingBox) const;

    void GetCollisionBodyWorldBounds(int index, BvAxisAlignedBox& boundingBox) const;

    void GetCollisionBodyLocalBounds(int index, BvAxisAlignedBox& boundingBox) const;

    float GetCollisionBodyMargin(int index) const;

    int GetCollisionBodiesCount() const;

    /** Create 3d mesh model from collision body composition. Store coordinates in world space. */
    void GatherCollisionGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices) const;

    void CollisionContactQuery(TPodVector<HitProxy*>& _Result) const;

    void CollisionContactQueryActor(TPodVector<Actor*>& _Result) const;

    void GatherNavigationGeometry(NavigationGeometry& Geometry) const override;

protected:
    PhysicalBody();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnTransformDirty() override;

    void ClearBoneCollisions();

    void UpdateBoneCollisions();

    void CreateBoneCollisions();

    void UpdatePhysicsAttribs();

    void DrawDebug(DebugRenderer* InRenderer) override;

    virtual CollisionModel* GetMeshCollisionModel() const { return nullptr; }

    friend class BoneCollisionInstance;
    virtual Float3x4 const& _GetJointTransform(int jointIndex)
    {
        return Float3x4::Identity();
    }

    bool        m_bSoftBodySimulation = false;
    btSoftBody* m_SoftBody = nullptr; // managed by SoftMeshComponent

private:
    void CreateRigidBody();
    void DestroyRigidBody();
    void SetCenterOfMassPosition(Float3 const& _Position);
    void SetCenterOfMassRotation(Quat const& _Rotation);
    bool ShouldHaveCollisionBody() const;
    void SetCollisionFlags();
    void SetRigidBodyGravity();
    void UpdateDebugDrawCache();

    TRef<HitProxy>              m_HitProxy;
    TRef<CollisionModel>        m_CollisionModel;
    TRef<CollisionInstance>     m_CollisionInstance;
    TPodVector<BoneCollisionInstance*> m_BoneCollisionInst;
    btRigidBody*               m_RigidBody = nullptr;
    PhysicalBodyMotionState*   m_MotionState = nullptr;
    TUniqueRef<DebugDrawCache> m_DebugDrawCache;

    float                 m_Mass                       = 1.0f;
    Float3                m_SelfGravity                = Float3(0.0f);
    Float3                m_LinearFactor               = Float3(1);
    float                 m_LinearDamping              = 0.0f;
    Float3                m_AngularFactor              = Float3(1);
    float                 m_AngularDamping             = 0.0f;
    float                 m_Friction                   = 0.5f;
    Float3                m_AnisotropicFriction        = Float3(1);
    float                 m_RollingFriction            = 0.0f;
    float                 m_Restitution                = 0.0f;
    float                 m_ContactProcessingThreshold = 1e18f;
    float                 m_LinearSleepingThreshold    = 0.8f;
    float                 m_AngularSleepingThreshold   = 1.0f;
    float                 m_CcdRadius                  = 0.0f;
    float                 m_CcdMotionThreshold         = 0.0f;
    MOTION_BEHAVIOR       m_MotionBehavior             = MB_STATIC;
    AI_NAVIGATION_BEHAVIOR m_AINavigationBehavior       = AI_NAVIGATION_BEHAVIOR_NONE;
    bool                  m_bDisableGravity            = false;
    bool                  m_bOverrideWorldGravity      = false;
    bool                  m_bUseMeshCollision          = false;
    Float3                m_CachedScale                = Float3(1.0f);
    //float               m_SpinningFriction           = 0.0f;   // Torsional friction around contact normal

    PhysicalBody* pNextNav{};
    PhysicalBody* pPrevNav{};
};

HK_NAMESPACE_END
