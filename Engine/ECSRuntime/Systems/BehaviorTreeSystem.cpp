#include "BehaviorTreeSystem.h"
#include "../GameFrame.h"
#include "../Components/BehaviorTreeComponent.h"

HK_NAMESPACE_BEGIN

BehaviorTreeSystem::BehaviorTreeSystem(ECS::World* world) :
    m_World(world)
{}

void BehaviorTreeSystem::Update(GameFrame const& frame)
{
    using Query = ECS::Query<>
        ::Required<BehaviorTreeComponent>;

    BT::BehaviorTreeContext context;
    context.TimeStep = frame.FixedTimeStep;
    context.CommandBuf = &m_World->GetCommandBuffer(0);
    //context.RandomGenerator = ... // TODO

    for (Query::Iterator it(*m_World); it; it++)
    {
        BehaviorTreeComponent const* bt = it.Get<BehaviorTreeComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            BT::BehaviorTree* tree = bt[i].BT_Tree;

            if (tree)
            {
                switch (tree->Status())
                {
                    case BT::RUNNING:
                        tree->Update(context);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

HK_NAMESPACE_END
