#pragma once

#include "EngineSystem.h"

#include "../SkeletalAnimation.h"

HK_NAMESPACE_BEGIN

class SkinningSystem_ECS : public EngineSystemECS
{
public:
    SkinningSystem_ECS(ECS::World* world);

    void UpdatePoses(struct GameFrame const& frame);
    void UpdateSkins();

    void DrawDebug(DebugRenderer& InRenderer) override;

private:
    ECS::World* m_World;
};

HK_NAMESPACE_END

