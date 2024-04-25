#pragma once

#include <Engine/World/Common/EngineSystem.h>

HK_NAMESPACE_BEGIN

class NodeMotionSystem : public EngineSystemECS
{
public:
    NodeMotionSystem(ECS::World* world);

    void Update(struct GameFrame const& frame);

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END
