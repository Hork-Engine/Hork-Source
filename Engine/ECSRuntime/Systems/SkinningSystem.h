#pragma once

#include <Engine/ECS/ECS.h>

#include "../SkeletalAnimation.h"

#include <Engine/Runtime/DebugRenderer.h>

HK_NAMESPACE_BEGIN

class SkinningSystem_ECS
{
public:
    SkinningSystem_ECS(ECS::World* world);

    void Update(struct GameFrame const& frame);

    void DrawDebug(DebugRenderer& InRenderer);

private:
    void UpdatePoses(GameFrame const& frame);
    void UpdateSkins();

    ECS::World* m_World;
};

HK_NAMESPACE_END

