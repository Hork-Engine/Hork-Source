#pragma once

#include <Engine/World/Common/EngineSystem.h>

#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class World;
class TeleportSystem : public EngineSystemECS
{
public:
    TeleportSystem(World* world);

    void Update(struct GameFrame const& frame);

private:
    World* m_World;
    PhysicsInterface& m_PhysicsInterface;
};

HK_NAMESPACE_END
