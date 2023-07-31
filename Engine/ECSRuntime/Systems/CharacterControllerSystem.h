#pragma once

#include "EngineSystem.h"

#include "../PhysicsInterface.h"

#include "../Components/CharacterControllerComponent.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;
class World;

class CharacterControllerSystem : public EngineSystemECS, public JPH::CharacterContactListener
{
public:
    CharacterControllerSystem(World* world);
    ~CharacterControllerSystem();

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<CharacterControllerComponent>& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<CharacterControllerComponent>& event);

    void Update(struct GameFrame const& frame);

    void DrawDebug(DebugRenderer& renderer) override;

private:
    void UpdateMovement(GameFrame const& frame);

    // Called whenever the character collides with a body. Returns true if the contact can push the character.
    void OnContactAdded(ECS::EntityHandle entityHandle, const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings);

	// Called whenever the character movement is solved and a constraint is hit. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
    void OnContactSolve(ECS::EntityHandle entityHandle, const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity);

    TVector<ECS::EntityHandle> m_PendingAddCharacters;
    TVector<std::pair<ECS::EntityHandle, JPH::CharacterVirtual*>> m_PendingRemoveCharacters;

    World* m_World;
    PhysicsInterface& m_PhysicsInterface;

    friend class CharacterControllerImpl;
};

HK_NAMESPACE_END
