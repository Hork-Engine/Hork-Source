#pragma once

#include "EngineSystem.h"

HK_NAMESPACE_BEGIN

struct GameFrame;

class TransformHistorySystem : public EngineSystemECS
{
public:
    TransformHistorySystem(ECS::World* world);

    void Update(GameFrame const& frame);

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END
