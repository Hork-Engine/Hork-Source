#pragma once

#include "GameplaySystem.h"

#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class World_ECS;
class SpringArmSystem : public GameplaySystemECS
{
public:
    SpringArmSystem(World_ECS* world);

    void PostPhysicsUpdate(GameFrame const& frame) override;

private:
    World_ECS* m_World;
    PhysicsInterface& m_PhysicsInterface;
};

HK_NAMESPACE_END
