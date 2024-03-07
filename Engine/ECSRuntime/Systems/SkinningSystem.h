#pragma once

#include "EngineSystem.h"

#include "../SkeletalAnimation.h"

HK_NAMESPACE_BEGIN

class SkinningSystem_ECS : public EngineSystemECS
{
public:
    SkinningSystem_ECS(ECS::World* world);

    void UpdatePoses(struct GameFrame const& frame);
    void UpdateSockets();
    void UpdateSkins();

    void DrawDebug(DebugRenderer& InRenderer) override;

private:
    ECS::World* m_World;
    int m_FrameIndex{};
};

HK_NAMESPACE_END

