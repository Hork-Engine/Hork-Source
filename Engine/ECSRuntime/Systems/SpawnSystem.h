#pragma once

#include <Engine/ECS/ECS.h>

#include "../Resources/ResourceManager.h"
#include "../Resources/MaterialManager.h"

#include "../Utils.h"

HK_NAMESPACE_BEGIN

class SpawnSystem
{
public:
    SpawnSystem(ECS::World* world);

    void Update(struct GameFrame const& frame);

private:
    void SpawnFunction(ECS::CommandBuffer& commandBuffer, Float3 const& worldPosition);

    ECS::World* m_World;
    TVector<ECS::EntityHandle> m_Entities;

    TSmallVector<TRef<CollisionModel>, 6> m_Models;
    TSmallVector<StringView, 6> m_Meshes;
};

HK_NAMESPACE_END
