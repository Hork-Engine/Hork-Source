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

#include <Engine/World/Modules/Physics/PhysicsInterface.h>

#include "BodyComponent.h"

HK_NAMESPACE_BEGIN

enum class CharacterShapeType
{
    Box,
    Cylinder,
    Capsule
};

enum class CharacterStance
{
    Standing,
    Crouching
};

class CharacterControllerComponent : public BodyComponent
{
    friend class            PhysicsInterface;

public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    //
    // Initial properties
    //

    CharacterShapeType      ShapeType = CharacterShapeType::Cylinder;
    float                   HeightStanding = 1.2f;
    float                   RadiusStanding = 0.3f;
    float                   HeightCrouching = 0.8f;
    float                   RadiusCrouching = 0.3f;   
    float                   CharacterPadding = 0.02f;
    float                   PredictiveContactDistance = 0.1f;

    //
    // Dynamic properties
    //

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t                 CollisionLayer = 0;

    bool                    EnableWalkStairs = true;
    bool                    EnableStickToFloor = true;
    float                   StairsStepUp = 0.5f;
    float                   StickToFloorStepDown = -0.5f;

    /// Teleport character to specified position / rotation
    void                    SetWorldPosition(Float3 const& position);
    void                    SetWorldRotation(Quat const& rotation);
    void                    SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation);

    Float3                  GetWorldPosition() const;
    Quat                    GetWorldRotation() const;

    /// Character mass (kg)
    void                    SetMass(float mass);
    float                   GetMass() const;

    /// Maximum force with which the character can push other bodies (N)
    void                    SetMaxStrength(float maxStrength);
    float                   GetMaxStrength() const;

    /// Set the maximum angle of slope that character can still walk on (degrees)
    void                    SetMaxSlopeAngle(float maxSlopeAngle);
    float                   GetMaxSlopeAngle() const;

    /// This value governs how fast a penetration will be resolved, 0 = nothing is resolved, 1 = everything in one update
    void                    SetPenetrationRecoverySpeed(float speed);
    float                   GetPenetrationRecoverySpeed() const;

    /// Set the linear velocity of the character (m / s)
    void                    SetLinearVelocity(Float3 const& velocity);
    /// Get the linear velocity of the character (m / s)
    Float3                  GetLinearVelocity() const;

    /// Check if the normal of the ground surface is too steep to walk on
    bool                    IsSlopeTooSteep(Float3 const& normal) const;

    /// Get the contact point with the ground
    Float3                  GetGroundPosition() const;

    /// Get the contact normal with the ground
    Float3                  GetGroundNormal() const;

    /// Velocity in world space of ground
    Float3                  GetGroundVelocity() const;

    //PhysicsMaterial const*  GetGroundMaterial() const; // TODO Material that the character is standing on

    /// Body of the object the character is standing on.
    BodyComponent*          TryGetGroundBody();

    /// Character is on the ground and can move freely.
    bool                    IsOnGround() const;

    /// Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
    bool                    IsOnSteepGround() const;

    /// Character is touching an object, but is not supported by it and should fall. The GetGroundXXX functions will return information about the touched object.
    bool                    IsShouldFall() const;

    /// Character is in the air and is not touching anything.
    bool                    IsInAir() const;

    /// Use the ground body ID to get an updated estimate of the ground velocity. This function can be used if the ground body has moved / changed velocity and you want a new estimate of the ground velocity.
    void                    UpdateGroundVelocity();

    bool                    UpdateStance(CharacterStance stance, float maxPenetrationDepth = 0.0f);

    //
    // Internal
    //

    void                    BeginPlay();
    void                    EndPlay();

private:
    class CharacterControllerImpl* m_pImpl;
    float                   m_Mass = 70;
    float                   m_MaxStrength = 100.0f;
    float                   m_MaxSlopeAngle = 45.0f;
    float                   m_PenetrationRecoverySpeed = 1.0f;
    Float3                  m_LinearVelocity;
};

namespace ComponentMeta
{
    template <>
    constexpr ObjectStorageType ComponentStorageType<CharacterControllerComponent>()
    {
        return ObjectStorageType::Compact;
    }
}

HK_NAMESPACE_END
