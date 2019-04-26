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
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class btCollisionShape;
class btRigidBody;
class btCompoundShape;

class FCollisionBody : public FBaseObject {
    AN_CLASS( FCollisionBody, FBaseObject )

    friend class FPhysicalBody;

public:
    Float3 Position;
    Quat Rotation;
    float Margin;

protected:
    FCollisionBody() {
        Rotation = Quat::Identity();
    }

    // Only FPhysicalBody can call Create()
    virtual btCollisionShape * Create() { return nullptr; }
};

class FCollisionSphere : public FCollisionBody {
    AN_CLASS( FCollisionSphere, FCollisionBody )

public:
    float Radius = 1;

protected:
    FCollisionSphere() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionBox : public FCollisionBody {
    AN_CLASS( FCollisionBox, FCollisionBody )

public:
    Float3 HalfExtents = Float3(1.0f);

protected:
    FCollisionBox() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionCylinder : public FCollisionBody {
    AN_CLASS( FCollisionCylinder, FCollisionBody )

public:
    Float3 HalfExtents = Float3(1.0f);

protected:
    FCollisionCylinder() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionCone : public FCollisionBody {
    AN_CLASS( FCollisionCone, FCollisionBody )

public:
    float Radius = 1;
    float Height = 1;

protected:
    FCollisionCone() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionCapsule : public FCollisionBody {
    AN_CLASS( FCollisionCapsule, FCollisionBody )

public:
    float Radius = 1;
    float Height = 1;

protected:
    FCollisionCapsule() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionPlane : public FCollisionBody {
    AN_CLASS( FCollisionPlane, FCollisionBody )

public:
    PlaneF Plane = PlaneF( Float3(0.0f,1.0f,0.0f), 0.0f );

protected:
    FCollisionPlane() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionConvexHull : public FCollisionBody {
    AN_CLASS( FCollisionConvexHull, FCollisionBody )

public:
    TPodArray< Float3 > Vertices;

protected:
    FCollisionConvexHull() {}

private:
    btCollisionShape * Create() override;
};

// TODO:
//class FCollisionTriangleSoup : public FCollisionBody
//class FCollisionTerrain : public FCollisionBody
//etc

class FCollisionBodyComposition {
public:

    ~FCollisionBodyComposition() {
        Clear();
    }

    void Clear() {
        for ( FCollisionBody * body : CollisionBodies ) {
            body->RemoveRef();
        }
        CollisionBodies.Clear();
    }

    void AddCollisionBody( FCollisionBody * _Body ) {
        AN_Assert( CollisionBodies.Find( _Body ) == CollisionBodies.End() );
        CollisionBodies.Append( _Body );
        _Body->AddRef();
    }

    void RemoveCollisionBody( FCollisionBody * _Body ) {
        auto it = CollisionBodies.Find( _Body );
        if ( it == CollisionBodies.End() ) {
            return;
        }
        _Body->RemoveRef();
        CollisionBodies.Erase( it );
    }

    TPodArray< FCollisionBody *, 2 > CollisionBodies;
};

class FPhysicalBodyMotionState;

class FPhysicalBody : public FSceneComponent {
    AN_COMPONENT( FPhysicalBody, FSceneComponent )

    friend class FPhysicalBodyMotionState;
public:
    float Mass;
    bool bTrigger;
    bool bKinematicBody;
    bool bNoGravity;
    bool bOverrideWorldGravity;
    Float3 SelfGravity;
    //float LinearDamping;
    //float AngularDamping;
    //float Friction = 0.5f;
    //float RollingFriction;      // The RollingFriction prevents rounded shapes, such as spheres, cylinders and capsules from rolling forever.
    float SpinningFriction;     // Torsional friction around contact normal
    //float Restitution;          // Best simulation results using zero restitution
    //float LinearSleepingThreshold = 0.8f;
    //float AngularSleepingThreshold = 1.0f;
    int CollisionLayer = 0x1;
    int CollisionMask = 0xffff;
    bool bNoPhysics;

    bool IsActive() const;

    void SetLinearVelocity( Float3 const & _Velocity );
    void SetLinearFactor( Float3 const & _Factor );
    void SetLinearRestThreshold( float _Threshold );
    void SetLinearDamping( float _Damping );
    void SetAngularVelocity( Float3 const & _Velocity );
    void SetAngularFactor( Float3 const & _Factor );
    void SetAngularRestThreshold( float _Threshold );
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
    float GetLinearRestThreshold() const;
    float GetLinearDamping() const;
    Float3 GetAngularVelocity() const;
    Float3 GetAngularFactor() const;
    float GetAngularRestThreshold() const;
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

    void RebuildComponent();

    FCollisionBodyComposition BodyComposition;

protected:
    FPhysicalBody();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnTransformDirty() override;

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
