/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Runtime/World/Modules/Physics/PhysicsInterface.h>
#include <Hork/Runtime/World/Modules/Physics/PhysicsMaterial.h>

#include "BodyComponent.h"

namespace JPH
{
    class Shape;
}

HK_NAMESPACE_BEGIN

class DynamicBodyComponent final : public BodyComponent
{
    friend class PhysicsInterface;

public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    //
    // Initial properties
    //

    /// The collision layer this body belongs to (determines if two objects can collide)
    uint8_t                 CollisionLayer = 0;

    /// Set a custom center of mass if you want to override the default, otherwise set it to Nan.
    Float3                  CenterOfMassOverride{std::numeric_limits<float>::quiet_NaN()};

    /// World space linear velocity of the center of mass (m/s)
    Float3                  LinearVelocity;

    /// World space angular velocity (rad/s)
    Float3                  AngularVelocity;

    /// Linear damping: dv/dt = -c * v. c must be between 0 and 1 but is usually close to 0.
    Half                    LinearDamping = 0.05f;

    /// Angular damping: dw/dt = -c * w. c must be between 0 and 1 but is usually close to 0.
    Half                    AngularDamping = 0.05f;

    /// Maximum linear velocity that this body can reach (m/s)
    Half                    MaxLinearVelocity = 500;

    /// Maximum angular velocity that this body can reach (rad/s)
    Half                    MaxAngularVelocity = 0.25f * Math::_PI * 60;

    /// Mass of the body (kg).
    Half                    Mass = 0;

    /// The calculated inertia will be multiplied by this value
    Half                    InertiaMultiplier = 1.0f;

    /// If this body can go to sleep or not
    bool                    AllowSleeping = true;

    bool                    StartAsSleeping = false;

    /// Motion quality, or how well it detects collisions when it has a high velocity
    bool                    UseCCD = false;

    PhysicsMaterial         Material;

    //
    // Dynamic properties
    //

    bool                    DispatchContactEvents = false;
    bool                    CanPushCharacter = true;

    /// Set motion behavior kinematic or dynamic
    void                    SetKinematic(bool isKinematic);
    bool                    IsKinematic() const { return m_IsKinematic; }

    /// Enable to allow rigid body scaling
    void                    SetDynamicScaling(bool isDynamicScaling);
    bool                    IsDynamicScaling() const { return m_IsDynamicScaling; }

    /// Value to multiply gravity with for this body
    void                    SetGravityFactor(float factor);
    float                   GetGravityFactor() const { return m_GravityFactor; }

    //
    // Interaction with the body
    //

    /// Teleport body to specified position / rotation
    void                    SetWorldPosition(Float3 const& position);
    void                    SetWorldRotation(Quat const& rotation);
    void                    SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation);

    Float3                  GetWorldPosition() const;
    Quat                    GetWorldRotation() const;

    enum class CoordinateSpace
    {
        Local,
        World
    };

    /// Kinematic movement (for kinematic body only)
    void                    MoveKinematic(Float3 const& destPosition, CoordinateSpace coordSpace = CoordinateSpace::World);
    void                    RotateKinematic(Quat const& destRotation, CoordinateSpace coordSpace = CoordinateSpace::World);
    void                    MoveAndRotateKinematic(Float3 const& destPosition, Quat const& destRotation, CoordinateSpace coordSpace = CoordinateSpace::World);

    /// Adds a force to the Rigidbody.
    void                    AddForce(Float3 const& force);
    /// Applies force at position. As a result this will apply a torque and force on the object.
    void                    AddForceAtPosition(Float3 const& force, Float3 const& position);
    /// Adds a torque to the rigidbody.
    void                    AddTorque(Float3 const& torque);
    /// A combination of AddForce and AddTorque
    void                    AddForceAndTorque(Float3 const& force, Float3 const& torque);

    /// Applied at center of mass
    void                    AddImpulse(Float3 const& impulse);
    /// Applied at position
    void                    AddImpulseAtPosition(Float3 const& impulse, Float3 const& position);

    void                    AddAngularImpulse(Float3 const& angularImpulse);

    float                   GetMass() const;

    Float3                  GetCenterOfMassPosition() const;

    Float3                  GetLinearVelocity() const;
    Float3                  GetAngularVelocity() const;
    Float3                  GetVelocityAtPosition(Float3 const& position) const;

    bool                    IsSleeping() const;

    // Utilites
    void                    GatherGeometry(Vector<Float3>& vertices, Vector<uint32_t>& indices);

    // Internal

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;
    Float3                  m_CachedScale;
    float                   m_GravityFactor = 1.0f;
    ScalingMode             m_ScalingMode = ScalingMode::NonUniform;
    bool                    m_IsKinematic = false;
    bool                    m_IsDynamicScaling = false;
    class BodyUserData*     m_UserData = nullptr;
    JPH::Shape*             m_Shape = nullptr;
};

namespace ComponentMeta
{
    template <>
    constexpr ObjectStorageType ComponentStorageType<DynamicBodyComponent>()
    {
        return ObjectStorageType::Sparse;
    }
}

HK_NAMESPACE_END
