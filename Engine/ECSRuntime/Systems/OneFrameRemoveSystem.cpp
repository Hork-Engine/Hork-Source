#include "OneFrameRemoveSystem.h"
#include "../Utils.h"

#include "../Components/ExperimentalComponents.h"

HK_NAMESPACE_BEGIN

void OneFrameRemoveSystem::Update()
{
    using Query = ECS::Query<>::ReadOnly<OneFrameEntityTag>;

    auto& commandBuffer = m_World->GetCommandBuffer(0);

    for (Query::Iterator it(*m_World); it; it++)
    {
        for (int i = 0; i < it.Count(); i++)
        {
            DestroyEntityWithChildren(m_World, commandBuffer, it.GetEntity(i));
        }
    }
}

HK_NAMESPACE_END
