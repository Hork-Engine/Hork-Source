#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

#include <Engine/Math/VectorMath.h>

#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

struct CharacterControllerComponent
{
    CharacterControllerComponent() = default;

    PhysBodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

    Float3 MovementDirection;
    Float3 DesiredVelocity;
    bool Jump = false;

    float AttackTime{};

    uint8_t m_CollisionGroup = CollisionGroup::CHARACTER;

    float cCharacterHeightStanding = 1.35f;
    float cCharacterRadiusStanding = 0.3f;
    float cCharacterHeightCrouching = 0.8f;
    float cCharacterRadiusCrouching = 0.3f;
    float cCharacterSpeed = 2; // walk
    //float cCharacterSpeed = 4; // run
    float cJumpSpeed = 4.0f;

    bool m_bAllowSliding = false;

    JPH::CharacterVirtual* m_pCharacter{};
    //JPH::Character* m_pCharacter2{};
    JPH::RefConst<JPH::Shape> m_StandingShape;
    JPH::RefConst<JPH::Shape> m_CrouchingShape;

    PhysBodyID m_BodyId;
};

HK_NAMESPACE_END
