/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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
#include "CollisionBody.h"
#include "CollisionEvents.h"
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class btRigidBody;
class FPhysicalBodyMotionState;

class FPhysicalBody : public FSceneComponent {
    AN_COMPONENT( FPhysicalBody, FSceneComponent )

    friend class FPhysicalBodyMotionState;
public:
    // Component events
    FContactDelegate E_OnBeginContact;
    FContactDelegate E_OnEndContact;
    FContactDelegate E_OnUpdateContact;
    FOverlapDelegate E_OnBeginOverlap;
    FOverlapDelegate E_OnEndOverlap;
    FOverlapDelegate E_OnUpdateOverlap;

    // Enable physics simulation. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    bool bSimulatePhysics;

    // Collision layer. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    int CollisionLayer = 0x1;

    // Collision mask. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    int CollisionMask = 0xffff;

    // Trigger can produce overlap events. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    bool bTrigger;

    // Kinematic body. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    bool bKinematicBody;

    // Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact)
    bool bDispatchContactEvents;

    // Dispatch contact events (OnBeginOverlap, OnUpdateOverlap, OnEndOverlap)
    bool bDispatchOverlapEvents;

    // Generate contact points for contact events. Use with bDispatchContactEvents.
    bool bGenerateContactPoints;

    // Collision body composition. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    FCollisionBodyComposition BodyComposition;

    // Set to true if you want to use body composition from overrided method DefaultBodyComposition(). Set it before component initialization
    // or call UpdatePhysicsAttribs() to apply property.
    bool bUseDefaultBodyComposition;

    // Set to ture to disable world gravity. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    bool bDisableGravity;

    // Set to ture to override world gravity and use self gravity. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    bool bOverrideWorldGravity;

    // Object self gravity, use with bOverrideWorldGravity. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    Float3 SelfGravity;

    // Object mass. Static objects have Mass == 0 and dynamic object must have Mass > 0. Set it before component initialization or call UpdatePhysicsAttribs() to apply property.
    float Mass;

    // Force physics activation
    void ActivatePhysics();

    // Is physics active
    bool IsPhysicsActive() const;

    // Object linear velocity
    void SetLinearVelocity( Float3 const & _Velocity );

    // Object linear velocity factor
    void SetLinearFactor( Float3 const & _Factor );

    void SetLinearSleepingThreshold( float _Threshold );

    void SetLinearDamping( float _Damping );

    // Object angular velocity
    void SetAngularVelocity( Float3 const & _Velocity );

    // Object angular velocity factor
    void SetAngularFactor( Float3 const & _Factor );

    void SetAngularSleepingThreshold( float _Threshold );

    void SetAngularDamping( float _Damping );

    void SetFriction( float _Friction );

    void SetAnisotropicFriction( Float3 const & _Friction );

    // The RollingFriction prevents rounded shapes, such as spheres, cylinders and capsules from rolling forever.
    void SetRollingFriction( float _Friction );

    // Best simulation results using zero restitution.
    void SetRestitution( float _Restitution );

    // Keep ContactProcessingThreshold*ContactProcessingThreshold < FLT_MAX
    void SetContactProcessingThreshold( float _Threshold );

    void SetCcdRadius( float _Radius );

    void SetCcdMotionThreshold( float _Threshold );

    Float3 GetLinearVelocity() const;

    Float3 const & GetLinearFactor() const;

    Float3 GetVelocityAtPoint( Float3 const & _Position ) const;

    float GetLinearSleepingThreshold() const;

    float GetLinearDamping() const;

    Float3 GetAngularVelocity() const;

    Float3 const & GetAngularFactor() const;

    float GetAngularSleepingThreshold() const;

    float GetAngularDamping() const;

    float GetFriction() const;

    Float3 const & GetAnisotropicFriction() const;

    float GetRollingFriction() const;

    float GetRestitution() const;

    float GetContactProcessingThreshold() const;

    float GetCcdRadius() const;

    float GetCcdMotionThreshold() const;

    void ClearForces();

    void ApplyCentralForce( Float3 const & _Force );

    void ApplyForce( Float3 const & _Force, Float3 const & _Position );

    void ApplyTorque( Float3 const & _Torque );

    void ApplyCentralImpulse( Float3 const & _Impulse );

    void ApplyImpulse( Float3 const & _Impulse, Float3 const & _Position );

    void ApplyTorqueImpulse( Float3 const & _Torque );

    void GetCollisionBodiesWorldBounds( TPodArray< BvAxisAlignedBox > & _BoundingBoxes ) const;

    void UpdatePhysicsAttribs();

protected:
    FPhysicalBody();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void EndPlay() override;

    void OnTransformDirty() override;

    virtual FCollisionBodyComposition const & DefaultBodyComposition() const { return BodyComposition; }

private:
    void CreateRigidBody();
    void DestroyRigidBody();
    void UpdatePhysicalBodyPosition( Float3 const & _Position );
    void UpdatePhysicalBodyRotation( Quat const & _Rotation );

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

    btRigidBody * RigidBody;
    btCompoundShape * CompoundShape;
    FPhysicalBodyMotionState * MotionState;
    bool bTransformWasChangedByPhysicsEngine;
    Float3 CachedScale;
};
