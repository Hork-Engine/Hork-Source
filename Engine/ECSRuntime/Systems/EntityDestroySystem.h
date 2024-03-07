#pragma once

#include "EngineSystem.h"

HK_NAMESPACE_BEGIN

struct GameFrame;

class EntityDestroySystem : public EngineSystemECS
{
public:
    EntityDestroySystem(ECS::World* world);
    ~EntityDestroySystem();

    void Update();

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END
