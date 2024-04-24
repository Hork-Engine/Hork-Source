#pragma once

#include "EngineSystem.h"

#include "../PhysicsInterface.h"

#include "../Components/CharacterControllerComponent.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;
class World;

class CharacterControllerSystem : public EngineSystemECS
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

    World* m_World;
    PhysicsInterface& m_PhysicsInterface;

    int m_FrameIndex{};
};

HK_NAMESPACE_END
