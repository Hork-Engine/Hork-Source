#pragma once

#include "EngineSystem.h"

#include "../SkeletalAnimation.h"

HK_NAMESPACE_BEGIN

class SkinningSystem_ECS : public EngineSystemECS
{
public:
    SkinningSystem_ECS(ECS::World* world);

    void Update(struct GameFrame const& frame);

    void DrawDebug(DebugRenderer& InRenderer) override;

private:
    void UpdatePoses(GameFrame const& frame);
    void UpdateSkins();

    ECS::World* m_World;
};

HK_NAMESPACE_END

