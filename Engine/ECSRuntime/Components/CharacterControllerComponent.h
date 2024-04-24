#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

#include <Engine/Math/VectorMath.h>

#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

struct CharacterControllerComponent
{
    CharacterControllerComponent(PhysBodyID inBodyID, uint8_t inCollisionGroup) :
        m_BodyId(inBodyID),
        m_CollisionGroup(inCollisionGroup)
    {}

    PhysBodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

    uint8_t GetCollisionGroup() const
    {
        return m_CollisionGroup;
    }

    Float3 MovementDirection;
    Float3 DesiredVelocity;
    bool Jump = false;

    float MoveSpeed = 2;
    float JumpSpeed = 4.0f;

    bool EnableWalkStairs = true;
    bool EnableStickToFloor = true;

    // Internal
    JPH::CharacterVirtual* m_pCharacter{};
    JPH::RefConst<JPH::Shape> m_StandingShape;
    JPH::RefConst<JPH::Shape> m_CrouchingShape;
    bool m_bAllowSliding = false;

private:
    PhysBodyID m_BodyId;
    uint8_t m_CollisionGroup = CollisionGroup::CHARACTER;
};

HK_NAMESPACE_END
