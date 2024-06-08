#pragma once

#include <Engine/World/Modules/Physics/PhysicsInterface.h>

#include "BodyComponent.h"

HK_NAMESPACE_BEGIN

class CharacterControllerComponent : public BodyComponent
{
    friend class PhysicsInterface;

public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t     m_CollisionLayer = CollisionLayer::Character;

    float      HeightStanding = 1.35f;
    float      RadiusStanding = 0.3f;
    float      HeightCrouching = 0.8f;
    float      RadiusCrouching = 0.3f;
    float      MaxSlopeAngle = Math::Radians(45.0f);
    float      MaxStrength = 100.0f;
    float      CharacterPadding = 0.02f;
    float      PenetrationRecoverySpeed = 1.0f;
    float      PredictiveContactDistance = 0.1f;




    Float3 MovementDirection;
    Float3 DesiredVelocity;
    bool Jump = false;

    float MoveSpeed = 2;
    float JumpSpeed = 4.0f;

    bool EnableWalkStairs = true;
    bool EnableStickToFloor = true;

    /// Teleport character to specified position / rotation
    void                    SetWorldPosition(Float3 const& position);
    void                    SetWorldRotation(Quat const& rotation);
    void                    SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation);

    Float3                  GetWorldPosition() const;
    Quat                    GetWorldRotation() const;

    /// Set the linear velocity of the character (m / s)
    void                    SetLinearVelocity(Float3 const& velocity);
    /// Get the linear velocity of the character (m / s)
    Float3                  GetLinearVelocity();

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;
    class CharacterControllerImpl* m_pImpl;

    class BodyUserData*     m_UserData = nullptr;
};

HK_NAMESPACE_END
