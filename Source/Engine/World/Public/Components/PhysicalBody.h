/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <World/Public/CollisionEvents.h>
#include <World/Public/Resource/CollisionBody.h>

class btRigidBody;
class btSoftBody;
class SPhysicalBodyMotionState;

enum ECollisionMask {
    CM_NOCOLLISION   = 0,
    CM_WORLD_STATIC  = 1,
    CM_WORLD_DYNAMIC = 2,
    CM_WORLD         = CM_WORLD_STATIC | CM_WORLD_DYNAMIC,
    CM_PAWN          = 4,
    CM_PROJECTILE    = 8,
    CM_TRIGGER       = 16,
    CM_UNUSED5       = 32,
    CM_UNUSED6       = 64,
    CM_UNUSED7       = 128,
    CM_UNUSED8       = 256,
    CM_UNUSED9       = 512,
    CM_UNUSED10      = 1024,
    CM_UNUSED11      = 1024,
    CM_UNUSED12      = 2048,
    CM_UNUSED13      = 4096,
    CM_UNUSED14      = 8192,
    CM_UNUSED15      = 16384,
    CM_ALL           = 0xffff
};

enum EPhysicsBehavior {
    /** No physics simulation, just collisions */
    PB_STATIC,

    /** Physics simulated by engine */
    PB_DYNAMIC,

    /** Physics simulated by game logic */
    PB_KINEMATIC
};

enum EAINavigationBehavior {
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

class APhysicalBody : public ASceneComponent {
    AN_COMPONENT( APhysicalBody, ASceneComponent )

    friend struct SCollisionFilterCallback;
    friend class SPhysicalBodyMotionState;
    friend class APhysicsWorld;
    friend class AAINavigationMesh;

public:
    // Component events
    AContactDelegate E_OnBeginContact;
    AContactDelegate E_OnEndContact;
    AContactDelegate E_OnUpdateContact;
    AOverlapDelegate E_OnBeginOverlap;
    AOverlapDelegate E_OnEndOverlap;
    AOverlapDelegate E_OnUpdateOverlap;

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    bool bDispatchContactEvents;

    /** Dispatch contact events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap) */
    bool bDispatchOverlapEvents;

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    bool bGenerateContactPoints;

    /** Collision body composition. Set it before component initialization or call UpdatePhysicsAttribs() to apply property. */
    ACollisionBodyComposition BodyComposition;

    /** Set to true if you want to use body composition from overrided method DefaultBodyComposition(). Set it before component initialization
    or call UpdatePhysicsAttribs() to apply property. */
    bool bUseDefaultBodyComposition;

    /** Set physics simulation type: static, dynamic, kinematic */
    void SetPhysicsBehavior( EPhysicsBehavior _PhysicsBehavior );

    /** Get physics simulation type: static, dynamic, kinematic */
    EPhysicsBehavior GetPhysicsBehavior() const { return PhysicsBehavior; }

    /** Specifies how the body will be used by navigation mesh generator */
    void SetAINavigationBehavior( EAINavigationBehavior _AINavigationBehavior );

    /** How the body will be used to build AI navigation mesh */
    EAINavigationBehavior GetAINavigationBehavior() const { return AINavigationBehavior; }

    /** Trigger can produce overlap events. */
    void SetTrigger( bool _Trigger );

    /** Trigger can produce overlap events. */
    bool IsTrigger() const { return bTrigger; }

    /** Set to true to disable world gravity. Only for PS_DYNAMIC */
    void SetDisableGravity( bool _DisableGravity );

    /** Return true if gravity is disabled for the object. */
    bool IsGravityDisabled() const { return bDisableGravity; }

    /** Set to true to override world gravity and use self gravity. Only for PS_DYNAMIC */
    void SetOverrideWorldGravity( bool _OverrideWorldGravity );

    /** Return true if gravity is overriden for the object. */
    bool IsWorldGravityOverriden() const { return bOverrideWorldGravity; }

    /** Object self gravity, use with bOverrideWorldGravity. Only for PS_DYNAMIC */
    void SetSelfGravity( Float3 const & _SelfGravity );

    /** Object self gravity, use with bOverrideWorldGravity. Only for PS_DYNAMIC */
    Float3 const & GetSelfGravity() const { return SelfGravity; }

    /** Object mass. Only for PS_DYNAMIC */
    void SetMass( float _Mass );

    /** Object mass. Only for PS_DYNAMIC */
    float GetMass() const { return Mass; }

    /** Set collision group/layer. See ECollisionMask. */
    void SetCollisionGroup( int _CollisionGroup );

    /** Get collision group. See ECollisionMask. */
    int GetCollisionGroup() const { return CollisionGroup; }

    /** Set collision mask. See ECollisionMask. */
    void SetCollisionMask( int _CollisionMask );

    /** Get collision mask. See ECollisionMask. */
    int GetCollisionMask() const { return CollisionMask; }

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

    ACollisionBodyComposition const & GetBodyComposition() const;

    /** Create 3d mesh model from collision body composition. Store coordinates in world space. */
    void CreateCollisionModel( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices );

    void CollisionContactQuery( TPodArray< APhysicalBody * > & _Result ) const;

    void CollisionContactQueryActor( TPodArray< AActor * > & _Result ) const;

    void UpdatePhysicsAttribs();

protected:
    APhysicalBody();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void BeginPlay() override;
    void EndPlay() override;

    void OnTransformDirty() override;

    void DrawDebug( ADebugRenderer * InRenderer ) override;

    virtual ACollisionBodyComposition const & DefaultBodyComposition() const { return BodyComposition; }

    bool bSoftBodySimulation;
    btSoftBody * SoftBody; // managed by ASoftMeshComponent
    //Float3 PrevWorldPosition;
    //Quat PrevWorldRotation;
    //bool bUpdateSoftbodyTransform;

private:
    void CreateRigidBody();
    void DestroyRigidBody();
    void SetCenterOfMassPosition( Float3 const & _Position );
    void SetCenterOfMassRotation( Quat const & _Rotation );
    void AddPhysicalBodyToWorld();
    void RemovePhysicalBodyFromWorld();

    TPodArray< AActor *, 1 > CollisionIgnoreActors;

    float Mass = 1.0f;
    Float3 SelfGravity;
    Float3 LinearFactor = Float3( 1 );
    float LinearDamping;
    Float3 AngularFactor = Float3( 1 );
    float AngularDamping;
    float Friction = 0.5f;
    Float3 AnisotropicFriction = Float3( 1 );
    float RollingFriction;
    //float SpinningFriction;   // Torsional friction around contact normal
    float Restitution;
    float ContactProcessingThreshold = 1e18f;
    float LinearSleepingThreshold = 0.8f;
    float AngularSleepingThreshold = 1.0f;
    float CcdRadius;
    float CcdMotionThreshold;
    int CollisionGroup = CM_WORLD;
    int CollisionMask = CM_ALL;
    EPhysicsBehavior PhysicsBehavior;
    EAINavigationBehavior AINavigationBehavior;
    bool bTrigger;
    bool bDisableGravity;
    bool bOverrideWorldGravity;
    bool bInWorld;

    btRigidBody * RigidBody;
    btCompoundShape * CompoundShape;
    SPhysicalBodyMotionState * MotionState;
    Float3 CachedScale;

    APhysicalBody * NextMarked;
    APhysicalBody * PrevMarked;

    APhysicalBody * NextNavBody;
    APhysicalBody * PrevNavBody;
};
