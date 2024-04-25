#pragma once

#include <Engine/World/Common/GameplaySystem.h>

#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class World;
class SpringArmSystem : public GameplaySystemECS
{
public:
    SpringArmSystem(World* world);

    void PostPhysicsUpdate(GameFrame const& frame) override;

private:
    World* m_World;
    PhysicsInterface& m_PhysicsInterface;
};

HK_NAMESPACE_END
