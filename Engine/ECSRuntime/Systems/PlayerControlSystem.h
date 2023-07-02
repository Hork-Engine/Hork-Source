#pragma once

#include <Engine/ECS/ECS.h>

#include "../SkeletalAnimation.h"

HK_NAMESPACE_BEGIN

class PlayerControlSystem
{
public:
    PlayerControlSystem(ECS::World* world);

    void Update(float timeStep);

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END
