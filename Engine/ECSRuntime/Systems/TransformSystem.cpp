#include "TransformSystem.h"
#include "../GameFrame.h"

#include "../Components/TransformComponent.h"
#include "../Components/WorldTransformComponent.h"
#include "../Components/FinalTransformComponent.h"
#include "../Components/TransformInterpolationTag.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/NodeComponent.h"
#include "../Components/MovableTag.h"

HK_NAMESPACE_BEGIN

TransformSystem::TransformSystem(ECS::World* world) :
    m_World(world),
    m_SceneGraph(world)
{
    m_World->AddEventHandler<ECS::Event::OnComponentAdded<NodeComponent>>(this);
    m_World->AddEventHandler<ECS::Event::OnComponentRemoved<NodeComponent>>(this);
    m_World->AddEventHandler<ECS::Event::OnComponentAdded<WorldTransformComponent>>(this);
}

TransformSystem::~TransformSystem()
{
    m_World->RemoveHandler(this);
}

void TransformSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<NodeComponent> const& event)
{
    event.Component().m_Node = m_SceneGraph.CreateNode(event.GetEntity(), event.Component().GetParent());
}

void TransformSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<NodeComponent> const& event)
{
    m_SceneGraph.DestroyNode(event.Component().m_Node);
}

void TransformSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<WorldTransformComponent> const& event)
{
    ECS::EntityView view = world->GetEntityView(event.GetEntity());

    if (!view.HasComponent<MovableTag>() && view.HasComponent<FinalTransformComponent>())
    {
        m_StaticObjects.Add(view.GetHandle());
    }
}

void TransformSystem::Update(GameFrame const& frame)
{
    int frameNum = frame.StateIndex;

    m_SceneGraph.UpdateHierarchy();

    // TODO: Catch event DESTROY_SCENE_NODE. Get all children entities and call commandBuffer->Destroy(entity) for each entity


    // Update scene graph
    {
        using Query = ECS::Query<>
            ::Required<NodeComponent>
            ::ReadOnly<TransformComponent>
            
            //::ReadOnly<MovableTag>
            ;

        for (Query::Iterator q(*m_World); q; q++)
        {
            //if (!q.HasComponent<RigidBodyComponent>())
            {
                NodeComponent* node = q.Get<NodeComponent>();
                TransformComponent const* t = q.Get<TransformComponent>();

                for (int i = 0; i < q.Count(); i++)
                {
                    node[i].m_Node->SetTransform(t[i].Position, t[i].Rotation, t[i].Scale, node[i].Flags);
                }
            }
        }
    }

    //{
    //    using Query = ECS::Query<>
    //        ::Required<NodeComponent>
    //        ::ReadOnly<TransformComponent>
    //        ::ReadOnly<RigidBodyComponent>
    //        
    //        //::ReadOnly<MovableTag>
    //        ;

    //    for (Query::Iterator q(*m_World); q; q++)
    //    {
    //        NodeComponent* node = q.Get<NodeComponent>();
    //        TransformComponent const* t = q.Get<TransformComponent>();
    //        RigidBodyComponent const* rigidBodies = q.Get<RigidBodyComponent>();

    //        for (int i = 0; i < q.Count(); i++)
    //        {
    //            switch (rigidBodies[i].GetMotionBehavior())
    //            {
    //                case MB_SIMULATED:
    //                    node[i].m_Node->SetTransform(t[i].Position, t[i].Rotation, t[i].Scale, SCENE_NODE_ABSOLUTE_POSITION | SCENE_NODE_ABSOLUTE_ROTATION | SCENE_NODE_ABSOLUTE_SCALE);
    //                    break;
    //                default: //case MB_KINEMATIC:
    //                    node[i].m_Node->SetTransform(t[i].Position, t[i].Rotation, t[i].Scale, node[i].Flags);
    //                    break;
    //            }                
    //        }
    //    }
    //}

    m_SceneGraph.UpdateWorldTransforms();

    // Update world transform
    {
        using Query = ECS::Query<>
            ::ReadOnly<NodeComponent>
            ::Required<WorldTransformComponent>;

        auto* worldTransforms = m_SceneGraph.WorldTransform;

        for (Query::Iterator q(*m_World); q; q++)
        {
            NodeComponent const* node = q.Get<NodeComponent>();
            WorldTransformComponent* t = q.Get<WorldTransformComponent>();
            for (int i = 0; i < q.Count(); i++)
            {
                t[i].Position[frameNum] = worldTransforms[node[i].m_Node->Index].Position;
                t[i].Rotation[frameNum] = worldTransforms[node[i].m_Node->Index].Rotation;
                t[i].Scale[frameNum]    = worldTransforms[node[i].m_Node->Index].Scale;
            }
        }
    }

    for (ECS::EntityHandle staticObject : m_StaticObjects)
    {
        ECS::EntityView view = m_World->GetEntityView(staticObject);

        WorldTransformComponent* worldTransform = view.GetComponent<WorldTransformComponent>();
        FinalTransformComponent* transform = view.GetComponent<FinalTransformComponent>();

        if (worldTransform && transform)
        {
            transform->Position = worldTransform->Position[frameNum];
            transform->Rotation = worldTransform->Rotation[frameNum];
            transform->Scale    = worldTransform->Scale[frameNum];
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
        ::Required<FinalTransformComponent>;

    // TODO: For non-movable objects set FinalTransformComponent once (when it spawned)

    // TODO: If object was teleported, do not interpolate! HACK: if object was teleported just set worldtransfom[prev] = worldtransfom[next]

    for (Query::Iterator q(*m_World); q; q++)
    {
        WorldTransformComponent const* t = q.Get<WorldTransformComponent>();
        FinalTransformComponent* interpolated = q.Get<FinalTransformComponent>();

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
        ::Required<FinalTransformComponent>;

    for (Query::Iterator q(*m_World); q; q++)
    {
        WorldTransformComponent const* t = q.Get<WorldTransformComponent>();
        FinalTransformComponent* interpolated = q.Get<FinalTransformComponent>();

        for (int i = 0; i < q.Count(); i++)
        {
            interpolated[i].Position = t[i].Position[frame.StateIndex];
            interpolated[i].Rotation = t[i].Rotation[frame.StateIndex];
            interpolated[i].Scale    = t[i].Scale[frame.StateIndex];
        }
    }
}

HK_NAMESPACE_END
