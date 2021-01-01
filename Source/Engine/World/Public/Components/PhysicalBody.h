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

#include "SceneComponent.h"

#include <World/Public/Collision.h>
#include <World/Public/Resource/CollisionBody.h>

class APhysicalBodyMotionState;
class ABoneCollisionInstance;

enum EMotionBehavior
{
    /** Static non-movable object */
    MB_STATIC,

    /** Object motion is simulated by physics engine */
    MB_SIMULATED,

    /** Movable object */
    MB_KINEMATIC
};

enum EAINavigationBehavior
{
    /** The body will not be used for navmesh generation */
    AI_NAVIGATION_BEHAVIOR_NONE,

    /** The body will be used for navmesh generation. AI can walk on. */
    AI_NAVIGATION_BEHAVIOR_STATIC,

    /** The body will be used for navmesh generation. AI can't walk on. */
    AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE,

    /** The body is dynamic obstacle. AI can walk on. */
    AI_NAVIGATION_BEHAVIOR_DYNAMIC,                 // TODO

    /** The body is dynamic obstacle. AI can't walk on. */
    AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE     // TODO
};

struct SDebugDrawCache
{
    TPodArrayHeap< Float3 > Vertices;
    TPodArrayHeap< unsigned int > Indices;
    bool bDirty;
};

class APhysicalBody : public ASceneComponent {
    AN_COMPONENT( APhysicalBody, ASceneComponent )

    friend struct SCollisionFilterCallback;
    friend class APhysicalBodyMotionState;
    friend class APhysicsWorld;
    friend class AAINavigationMesh;

public:
    AHitProxy * GetHitProxy() const
    {
        return HitProxy;
    }

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    void SetDispatchContactEvents( bool bDispatch )
    {
        HitProxy->bDispatchContactEvents = bDispatch;
    }

    bool ShouldDispatchContactEvents() const
    {
        return HitProxy->bDispatchContactEvents;
    }

    /** Dispatch overlap events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap) */
    void SetDispatchOverlapEvents( bool bDispatch )
    {
        HitProxy->bDispatchOverlapEvents = bDispatch;
    }

    bool ShouldDispatchOverlapEvents() const
    {
        return HitProxy->bDispatchOverlapEvents;
    }

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    void SetGenerateContactPoints( bool bGenerate )
    {
        HitProxy->bGenerateContactPoints = bGenerate;
    }

    bool ShouldGenerateContactPoints() const
    {
        return HitProxy->bGenerateContactPoints;
    }

    /** Set to false if you want to use own collision model and discard collisions from the mesh */
    void SetUseMeshCollision( bool bUseMeshCollision );

    bool ShouldUseMeshCollision() const
    {
        return bUseMeshCollision;
    }

    /** Collision model. */
    void SetCollisionModel( ACollisionModel * CollisionModel );

    /** Get current collision model */
    ACollisionModel const * GetCollisionModel() const;

    /** Set object motion behavior: static, simulated, kinematic */
    void SetMotionBehavior( EMotionBehavior _MotionBehavior );

    /** Get object motion behavior: static, dynamic, kinematic */
    EMotionBehavior GetMotionBehavior() const { return MotionBehavior; }

    /** Specifies how the body will be used by navigation mesh generator */
    void SetAINavigationBehavior( EAINavigationBehavior _AINavigationBehavior );

    /** How the body will be used to build AI navigation mesh */
    EAINavigationBehavior GetAINavigationBehavior() const { return AINavigationBehavior; }

    /** Trigger can produce overlap events. */
    void SetTrigger( bool _Trigger );

    /** Trigger can produce overlap events. */
    bool IsTrigger() const { return HitProxy->IsTrigger(); }

    /** Set to true to disable world gravity. Only for MB_SIMULATED */
    void SetDisableGravity( bool _DisableGravity );

    /** Return true if gravity is disabled for the object. */
    bool IsGravityDisabled() const { return bDisableGravity; }

    /** Set to true to override world gravity and use self gravity. Only for MB_SIMULATED */
    void SetOverrideWorldGravity( bool _OverrideWorldGravity );

    /** Return true if gravity is overriden for the object. */
    bool IsWorldGravityOverriden() const { return bOverrideWorldGravity; }

    /** Object self gravity, use with bOverrideWorldGravity. Only for MB_SIMULATED */
    void SetSelfGravity( Float3 const & _SelfGravity );

    /** Object self gravity, use with bOverrideWorldGravity. Only for MB_SIMULATED */
    Float3 const & GetSelfGravity() const { return SelfGravity; }

    /** Object mass. Only for MB_SIMULATED */
    void SetMass( float _Mass );

    /** Object mass. Only for MB_SIMULATED */
    float GetMass() const { return Mass; }

    /** Set collision group/layer. See ECollisionMask. */
    void SetCollisionGroup( int _CollisionGroup );

    /** Get collision group. See ECollisionMask. */
    int GetCollisionGroup() const { return HitProxy->GetCollisionGroup(); }

    /** Set collision mask. See ECollisionMask. */
    void SetCollisionMask( int _CollisionMask );

    /** Get collision mask. See ECollisionMask. */
    int GetCollisionMask() const { return HitProxy->GetCollisionMask(); }

    /** Set collision group and mask. See ECollisionMask. */
    void SetCollisionFilter( int _CollisionGroup, int _CollisionMask );

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor( AActor * _Actor );

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor( AActor * _Actor );

    /** Force physics activation */
    void ActivatePhysics();

    /** Is physics active */
    bool IsPhysicsActive() const;

    /** Object linear velocity */
    void SetLinearVelocity( Float3 const & _Velocity );

    /** Add value to current velocity */
    void AddLinearVelocity( Float3 const & _Velocity );

    /** Get object velocity. For soft bodies use GetVertexVelocity in ASoftMeshComponent. */
    Float3 GetLinearVelocity() const;

    /** Get object velocity at local point. */
    Float3 GetVelocityAtPoint( Float3 const & _Position ) const;

    /** Object linear velocity factor */
    void SetLinearFactor( Float3 const & _Factor );

    /** Object linear velocity factor */
    Float3 const & GetLinearFactor() const;

    void SetLinearSleepingThreshold( float _Threshold );

    float GetLinearSleepingThreshold() const;

    void SetLinearDamping( float _Damping );

    float GetLinearDamping() const;

    /** Object angular velocity */
    void SetAngularVelocity( Float3 const & _Velocity );

    /** Add value to current velocity */
    void AddAngularVelocity( Float3 const & _Velocity );

    /** Object angular velocity */
    Float3 GetAngularVelocity() const;

    /** Object angular velocity factor */
    void SetAngularFactor( Float3 const & _Factor );

    /** Object angular velocity factor */
    Float3 const & GetAngularFactor() const;

    void SetAngularSleepingThreshold( float _Threshold );

    float GetAngularSleepingThreshold() const;

    void SetAngularDamping( float _Damping );

    float GetAngularDamping() const;

    void SetFriction( float _Friction );

    float GetFriction() const;

    void SetAnisotropicFriction( Float3 const & _Friction );

    Float3 const & GetAnisotropicFriction() const;

    /** The RollingFriction prevents rounded shapes, such as spheres, cylinders and capsules from rolling forever. */
    void SetRollingFriction( float _Friction );

    /** The RollingFriction prevents rounded shapes, such as spheres, cylinders and capsules from rolling forever. */
    float GetRollingFriction() const;

    /** Best simulation results using zero restitution. */
    void SetRestitution( float _Restitution );

    /** Best simulation results using zero restitution. */
    float GetRestitution() const;

    /** Keep ContactProcessingThreshold*ContactProcessingThreshold < FLT_MAX */
    void SetContactProcessingThreshold( float _Threshold );

    float GetContactProcessingThreshold() const;

    /** Continuous collision detection swept radius */
    void SetCcdRadius( float _Radius );

    /** Continuous collision detection swept radius */
    float GetCcdRadius() const;

    /** Don't do continuous collision detection if the motion (in one step) is less then CcdMotionThreshold */
    void SetCcdMotionThreshold( float _Threshold );

    float GetCcdMotionThreshold() const;

    Float3 const & GetCenterOfMass() const;

    Float3 GetCenterOfMassWorldPosition() const;

    void ClearForces();

    void ApplyCentralForce( Float3 const & _Force );

    void ApplyForce( Float3 const & _Force, Float3 const & _Position );

    void ApplyTorque( Float3 const & _Torque );

    void ApplyCentralImpulse( Float3 const & _Impulse );

    void ApplyImpulse( Float3 const & _Impulse, Float3 const & _Position );

    void ApplyTorqueImpulse( Float3 const & _Torque );

    void GetCollisionBodiesWorldBounds( TPodArray< BvAxisAlignedBox > & _BoundingBoxes ) const;

    void GetCollisionWorldBounds( BvAxisAlignedBox & _BoundingBox ) const;

    void GetCollisionBodyWorldBounds( int _Index, BvAxisAlignedBox & _BoundingBox ) const;

    void GetCollisionBodyLocalBounds( int _Index, BvAxisAlignedBox & _BoundingBox ) const;

    float GetCollisionBodyMargin( int _Index ) const;

    int GetCollisionBodiesCount() const;

    /** Create 3d mesh model from collision body composition. Store coordinates in world space. */
    void GatherCollisionGeometry( TPodArrayHeap< Float3 > & _Vertices, TPodArrayHeap< unsigned int > & _Indices ) const;

    void CollisionContactQuery( TPodArray< AHitProxy * > & _Result ) const;

    void CollisionContactQueryActor( TPodArray< AActor * > & _Result ) const;

protected:
    APhysicalBody();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnTransformDirty() override;

    void ClearBoneCollisions();

    void UpdateBoneCollisions();

    void CreateBoneCollisions();

    void UpdatePhysicsAttribs();

    void DrawDebug( ADebugRenderer * InRenderer ) override;

    virtual ACollisionModel const * GetMeshCollisionModel() const { return nullptr; }

    friend class ABoneCollisionInstance;
    virtual Float3x4 const & _GetJointTransform( int _JointIndex )
    {
        return Float3x4::Identity();
    }

    bool bSoftBodySimulation = false;
    class btSoftBody * SoftBody = nullptr; // managed by ASoftMeshComponent

private:
    void CreateRigidBody();
    void DestroyRigidBody();
    void SetCenterOfMassPosition( Float3 const & _Position );
    void SetCenterOfMassRotation( Quat const & _Rotation );
    bool ShouldHaveCollisionBody() const;
    void SetCollisionFlags();
    void SetRigidBodyGravity();
    void UpdateDebugDrawCache();

    TRef< AHitProxy > HitProxy;
    TRef< ACollisionModel > CollisionModel;
    TRef< ACollisionInstance > CollisionInstance;
    TPodArray< ABoneCollisionInstance * > BoneCollisionInst;
    class btRigidBody * RigidBody = nullptr;
    APhysicalBodyMotionState * MotionState = nullptr;
    TUniqueRef< SDebugDrawCache > DebugDrawCache;

    float Mass = 1.0f;
    Float3 SelfGravity = Float3( 0.0f );
    Float3 LinearFactor = Float3( 1 );
    float LinearDamping = 0.0f;
    Float3 AngularFactor = Float3( 1 );
    float AngularDamping = 0.0f;
    float Friction = 0.5f;
    Float3 AnisotropicFriction = Float3( 1 );
    float RollingFriction = 0.0f;
    //float SpinningFriction = 0.0f;   // Torsional friction around contact normal
    float Restitution = 0.0f;
    float ContactProcessingThreshold = 1e18f;
    float LinearSleepingThreshold = 0.8f;
    float AngularSleepingThreshold = 1.0f;
    float CcdRadius = 0.0f;
    float CcdMotionThreshold = 0.0f;
    EMotionBehavior MotionBehavior = MB_STATIC;
    EAINavigationBehavior AINavigationBehavior = AI_NAVIGATION_BEHAVIOR_NONE;
    bool bDisableGravity = false;
    bool bOverrideWorldGravity = false;
    bool bUseMeshCollision = false;
    Float3 CachedScale = Float3( 1.0f );

    APhysicalBody * NextNavBody = nullptr;
    APhysicalBody * PrevNavBody = nullptr;
};
