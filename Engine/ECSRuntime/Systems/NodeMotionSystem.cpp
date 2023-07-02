#include "NodeMotionSystem.h"
#include "../NodeMotion.h"
#include "../GameFrame.h"
#include "../Components/NodeMotionComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/ActiveComponent.h"

HK_NAMESPACE_BEGIN

NodeMotionSystem::NodeMotionSystem(ECS::World* world) :
    m_World(world)
{}

void NodeMotionSystem::Update(GameFrame const& frame)
{
    using Query = ECS::Query<>
        ::ReadOnly<NodeMotionComponent>
        ::Required<TransformComponent>;

    for (Query::Iterator it(*m_World); it; it++)
    {
        NodeMotionComponent const* nodeMotion = it.Get<NodeMotionComponent>();
        TransformComponent* transform = it.Get<TransformComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            auto nodeId = nodeMotion[i].NodeId;
            NodeMotion* animation = nodeMotion[i].pAnimation;

            ECS::EntityView timerView = m_World->GetEntityView(nodeMotion[i].Timer);

            NodeMotionTimer* timer = timerView.GetComponent<NodeMotionTimer>();

            float time = timer ? timer->Time : 0.0f;

            for (auto& channel : animation->m_Channels)
            {
                if (channel.TargetNode == nodeId)
                {
                    switch (channel.TargetPath)
                    {
                        case NODE_ANIMATION_PATH_TRANSLATION:
                            transform[i].Position = animation->SampleVector(channel.Smp, time);
                            break;
                        case NODE_ANIMATION_PATH_ROTATION:
                            transform[i].Rotation = animation->SampleQuaternion(channel.Smp, time);
                            break;
                        case NODE_ANIMATION_PATH_SCALE:
                            transform[i].Scale = animation->SampleVector(channel.Smp, time);
                            break;
                    }
                }
            }
        }
    }

    using TimersQuery = ECS::Query<>
        ::Required<NodeMotionTimer>
        ::ReadOnly<ActiveComponent>;

    for (TimersQuery::Iterator it(*m_World); it; it++)
    {
        NodeMotionTimer* timer = it.Get<NodeMotionTimer>();
        ActiveComponent const* active = it.Get<ActiveComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            if (active[i].bIsActive)
            {
                timer[i].Time += frame.FixedTimeStep;

                if (timer[i].Time > timer[i].LoopTime)
                    timer[i].Time = Math::FMod(timer[i].Time, timer[i].LoopTime);
            }
        }
    }
}

HK_NAMESPACE_END
