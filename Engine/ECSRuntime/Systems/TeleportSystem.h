#pragma once

#include "EngineSystem.h"

#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class World_ECS;
class TeleportSystem : public EngineSystemECS
{
public:
    TeleportSystem(World_ECS* world);

    void Update(struct GameFrame const& frame);

private:
    World_ECS* m_World;
    PhysicsInterface& m_PhysicsInterface;
};

HK_NAMESPACE_END
