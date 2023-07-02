#pragma once

#include "EngineSystem.h"

HK_NAMESPACE_BEGIN

class CameraSystem : public EngineSystemECS
{
public:
    CameraSystem(ECS::World* world);

    void Update();

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END
