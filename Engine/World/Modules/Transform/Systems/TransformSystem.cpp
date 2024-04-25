#include "TransformSystem.h"
#include <Engine/World/Common/GameFrame.h>

#include "../Components/TransformComponent.h"
#include "../Components/WorldTransformComponent.h"
#include "../Components/RenderTransformComponent.h"
#include "../Components/TransformInterpolationTag.h"
#include "../Components/NodeComponent.h"
#include "../Components/MovableTag.h"

HK_NAMESPACE_BEGIN

TransformSystem::TransformSystem(ECS::World* world) :
    m_World(world)
{
    m_World->AddEventHandler<ECS::Event::OnComponentAdded<WorldTransformComponent>>(this);
}

TransformSystem::~TransformSystem()
{
    m_World->RemoveHandler(this);
}

void TransformSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<WorldTransformComponent> const& event)
{
    ECS::EntityView view = world->GetEntityView(event.GetEntity());

    if (!view.HasComponent<MovableTag>() && view.HasComponent<RenderTransformComponent>())
    {
        m_StaticObjects.Add(view.GetHandle());
    }
}

void TransformSystem::Update(GameFrame const& frame)
{
    int frameNum = frame.StateIndex;

    auto& scene_graph_interface = m_SceneGraphInterface;

    scene_graph_interface.Clear();

    // Build scene graph
    {
        using Query = ECS::Query<>
            ::Required<NodeComponent>;

        for (Query::Iterator q(*m_World); q; q++)
        {
            NodeComponent* nodes = q.Get<NodeComponent>();
            for (int i = 0; i < q.Count(); i++)
            {
                nodes[i]._ID = scene_graph_interface.Attach(q.GetEntity(i), nodes[i].Parent);
            }
        }
    }

    // Reorder nodes from parent to children
    scene_graph_interface.FinalizeGraph();

    // Set local transform for all nodes
    {
        using Query = ECS::Query<>
            ::ReadOnly<NodeComponent>
            ::Required<TransformComponent>;

        for (Query::Iterator q(*m_World); q; q++)
        {
            NodeComponent const* nodes = q.Get<NodeComponent>();
            TransformComponent* transforms = q.Get<TransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                scene_graph_interface.SetLocalTransform(nodes[i]._ID, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale, nodes[i].Flags);
            }
        }
    }

    // Calculate world transform for all nodes
    scene_graph_interface.CalcWorldTransform();

    // Get world transform from scene graph
    {
        using Query = ECS::Query<>
            ::ReadOnly<NodeComponent>
            ::Required<WorldTransformComponent>;
        for (Query::Iterator q(*m_World); q; q++)
        {
            NodeComponent const* nodes = q.Get<NodeComponent>();
            WorldTransformComponent* transforms = q.Get<WorldTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                scene_graph_interface.GetWorldTransform(nodes[i]._ID, transforms[i].Position[frameNum], transforms[i].Rotation[frameNum], transforms[i].Scale[frameNum]);
            }
        }
    }

    // Copy world transform to final transform for all static objects
    for (ECS::EntityHandle static_object : m_StaticObjects)
    {
        ECS::EntityView view = m_World->GetEntityView(static_object);

        WorldTransformComponent* world_transforms = view.GetComponent<WorldTransformComponent>();
        RenderTransformComponent* final_transforms = view.GetComponent<RenderTransformComponent>();

        if (world_transforms && final_transforms)
        {
            final_transforms->Position = world_transforms->Position[frameNum];
            final_transforms->Rotation = world_transforms->Rotation[frameNum];
            final_transforms->Scale    = world_transforms->Scale[frameNum];
        }
    }
    m_StaticObjects.Clear();
}

void TransformSystem::InterpolateTransformState(GameFrame const& frame)
{
    int prev = frame.PrevStateIndex;
    int next = frame.StateIndex;
    float interpolate = frame.Interpolate;

    using Query = ECS::Query<>
        ::ReadOnly<WorldTransformComponent>
        ::ReadOnly<MovableTag>
        ::Required<RenderTransformComponent>;

    // TODO: For non-movable objects set RenderTransformComponent once (when it spawned)

    // TODO: If object was teleported, do not interpolate! HACK: if object was teleported just set worldtransfom[prev] = worldtransfom[next]

    for (Query::Iterator q(*m_World); q; q++)
    {
        WorldTransformComponent const* t = q.Get<WorldTransformComponent>();
        RenderTransformComponent* interpolated = q.Get<RenderTransformComponent>();

        if (q.HasComponent<TransformInterpolationTag>())
        {
            for (int i = 0; i < q.Count(); i++)
            {
                interpolated[i].Position = Math::Lerp(t[i].Position[prev], t[i].Position[next], interpolate);
                interpolated[i].Rotation = Math::Slerp(t[i].Rotation[prev], t[i].Rotation[next], interpolate);
                interpolated[i].Scale    = Math::Lerp(t[i].Scale[prev], t[i].Scale[next], interpolate);
            }
        }
        else
        {
            for (int i = 0; i < q.Count(); i++)
            {
                interpolated[i].Position = t[i].Position[next];
                interpolated[i].Rotation = t[i].Rotation[next];
                interpolated[i].Scale    = t[i].Scale[next];
            }
        }
    }
}

void TransformSystem::CopyTransformState(GameFrame const& frame)
{
    using Query = ECS::Query<>
        ::ReadOnly<WorldTransformComponent>
        ::ReadOnly<MovableTag>
        ::Required<RenderTransformComponent>;

    for (Query::Iterator q(*m_World); q; q++)
    {
        WorldTransformComponent const* t = q.Get<WorldTransformComponent>();
        RenderTransformComponent* interpolated = q.Get<RenderTransformComponent>();

        for (int i = 0; i < q.Count(); i++)
        {
            interpolated[i].Position = t[i].Position[frame.StateIndex];
            interpolated[i].Rotation = t[i].Rotation[frame.StateIndex];
            interpolated[i].Scale    = t[i].Scale[frame.StateIndex];
        }
    }
}

HK_NAMESPACE_END
