#pragma once

#include <Engine/ECS/ECS.h>

#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class TeleportSystem
{
public:
    TeleportSystem(ECS::World* world, PhysicsInterface& physicsInterface) :
        m_World(world),
        m_PhysicsInterface(physicsInterface)
    {}

    void Update(struct GameFrame const& frame);

private:
    ECS::World* m_World;
    PhysicsInterface& m_PhysicsInterface;
};

HK_NAMESPACE_END
