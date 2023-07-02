#pragma once

#include <Engine/ECS/ECS.h>

HK_NAMESPACE_BEGIN

class OneFrameRemoveSystem
{
public:
    OneFrameRemoveSystem(ECS::World* world) :
        m_World(world)
    {}

    void Update();

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END
