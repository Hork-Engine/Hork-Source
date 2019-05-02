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
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class btRigidBody;
class FPhysicalBodyMotionState;

class FPhysicalBody : public FSceneComponent {
    AN_COMPONENT( FPhysicalBody, FSceneComponent )

    friend class FPhysicalBodyMotionState;
public:
    // Component events
    TEvent<> E_OnBeginContact;
    TEvent<> E_OnEndContact;
    TEvent<> E_OnUpdateContact;
    TEvent<> E_OnBeginOverlap;
    TEvent<> E_OnEndOverlap;
    TEvent<> E_OnUpdateOverlap;

    float Mass;
    bool bTrigger;
    bool bKinematicBody;
    bool bNoGravity;
    bool bOverrideWorldGravity;
    bool bUseDefaultBodyComposition;
    Float3 SelfGravity;
    //float LinearDamping;
    //float AngularDamping;
    //float Friction = 0.5f;
    //float RollingFriction;      // The RollingFriction prevents rounded shapes, such as spheres, cylinders and capsules from rolling forever.
    //float SpinningFriction;     // Torsional friction around contact normal
    //float Restitution;          // Best simulation results using zero restitution
    //float LinearSleepingThreshold = 0.8f;
    //float AngularSleepingThreshold = 1.0f;
    int CollisionLayer = 0x1;
    int CollisionMask = 0xffff;
    bool bNoPhysics;
    bool bDispatchContactEvents;
    bool bDispatchOverlapEvents;

    void Activate();
    bool IsActive() const;

    void SetLinearVelocity( Float3 const & _Velocity );
    void SetLinearFactor( Float3 const & _Factor );
    void SetLinearSleepingThreshold( float _Threshold );
    void SetLinearDamping( float _Damping );
    void SetAngularVelocity( Float3 const & _Velocity );
    void SetAngularFactor( Float3 const & _Factor );
    void SetAngularSleepingThreshold( float _Threshold );
    void SetAngularDamping( float _Damping );
    void SetFriction( float _Friction );
    void SetAnisotropicFriction( Float3 const & _Friction );
    void SetRollingFriction( float _Friction );
    void SetRestitution( float _Restitution );
    void SetContactProcessingThreshold( float _Threshold );
    void SetCcdRadius( float _Radius );
    void SetCcdMotionThreshold( float _Threshold );

    Float3 GetLinearVelocity() const;
    Float3 GetLinearFactor() const;
    Float3 GetVelocityAtPoint( Float3 const & _Position ) const;
    float GetLinearSleepingThreshold() const;
    float GetLinearDamping() const;
    Float3 GetAngularVelocity() const;
    Float3 GetAngularFactor() const;
    float GetAngularSleepingThreshold() const;
    float GetAngularDamping() const;
    float GetFriction() const;
    Float3 GetAnisotropicFriction() const;
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

    void RebuildRigidBody();

    FCollisionBodyComposition BodyComposition;

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

    btRigidBody * RigidBody;
    btCompoundShape * ShiftedCompoundShape;
    FPhysicalBodyMotionState * MotionState;
    bool bTransformWasChangedByPhysicsEngine;
    Float3 CachedScale;
};
