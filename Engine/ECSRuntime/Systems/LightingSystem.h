#pragma once

#include "EngineSystem.h"

#include "../Components/WorldTransformComponent.h"
#include "../Components/ExperimentalComponents.h"
#include "../SpatialTree.h"

HK_NAMESPACE_BEGIN

class LightingSystem : public EngineSystemECS
{
public:
    LightingSystem(ECS::World* world);
    ~LightingSystem();

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<DirectionalLightComponent_ECS> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<PunctualLightComponent_ECS> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<PunctualLightComponent_ECS> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<EnvironmentProbeComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<EnvironmentProbeComponent> const& event);

    void Update(struct GameFrame const& frame);

    // Call after transform update
    void UpdateBoundingBoxes(GameFrame const& frame);

    void DrawDebug(DebugRenderer& renderer) override;

private:
    void UpdateLightBounding(PunctualLightComponent_ECS& light, Float3 const& worldPosition, Quat const& worldRotation);

    ECS::World* m_World;

    //SpatialTree m_SpatialTree;
};

HK_NAMESPACE_END
