#pragma once

#include "EngineSystem.h"

HK_NAMESPACE_BEGIN

class BehaviorTreeSystem : public EngineSystemECS
{
public:
    BehaviorTreeSystem(ECS::World* world);

    void Update(struct GameFrame const& frame);

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END
