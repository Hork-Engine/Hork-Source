#include "TransformHistorySystem.h"

#include "../Components/FinalTransformComponent.h"
#include "../Components/TransformHistoryComponent.h"

HK_NAMESPACE_BEGIN

TransformHistorySystem::TransformHistorySystem(ECS::World* world) :
    m_World(world)
{}

void TransformHistorySystem::Update(GameFrame const& frame)
{
    using Query = ECS::Query<>
        ::Required<TransformHistoryComponent>
        ::ReadOnly<FinalTransformComponent>;

    for (Query::Iterator q(*m_World); q; q++)
    {
        TransformHistoryComponent* history = q.Get<TransformHistoryComponent>();
        FinalTransformComponent const* transform = q.Get<FinalTransformComponent>();

        for (int i = 0; i < q.Count(); i++)
        {
            history[i].TransformHistory = transform[i].ToMatrix();
        }
    }
}

HK_NAMESPACE_END
