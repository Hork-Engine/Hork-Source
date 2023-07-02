#pragma once

#include <Engine/ECS/ECS.h>

#include "../SceneGraph.h"
#include "../Components/NodeComponent.h"
#include "../Components/WorldTransformComponent.h"

HK_NAMESPACE_BEGIN

struct GameFrame;

class TransformSystem
{
public:
    TransformSystem(ECS::World* world);
    ~TransformSystem();

    void Update(GameFrame const& frame);

    void InterpolateTransformState(GameFrame const& frame);
    void CopyTransformState(GameFrame const& frame);

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<NodeComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<NodeComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<WorldTransformComponent> const& event);

private:
    ECS::World* m_World;
    SceneGraph m_SceneGraph;
    TVector<ECS::EntityHandle> m_StaticObjects;
};

HK_NAMESPACE_END
