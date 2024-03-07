#include "EntityDestroySystem.h"

#include "../Components/DestroyTag.h"
#include "../Components/CompoundEntityComponent.h"

HK_NAMESPACE_BEGIN

EntityDestroySystem::EntityDestroySystem(ECS::World* world) :
    m_World(world)
{
}

EntityDestroySystem::~EntityDestroySystem()
{
}

void EntityDestroySystem::Update()
{
    auto& command_buffer = m_World->GetCommandBuffer(0);
    
    {
        using Query = ECS::Query<>
            ::ReadOnly<DestroyTag>;
        for (Query::Iterator q(*m_World); q; q++)
        {
            CompoundEntityComponent* compound = q.TryGet<CompoundEntityComponent>();

            if (compound)
            {
                for (int i = 0; i < q.Count(); i++)
                {
                    for (auto child : compound[i].Entities)
                        command_buffer.DestroyEntity(child);
                    command_buffer.DestroyEntity(q.GetEntity(i));
                }
            }
            else
            {
                for (int i = 0; i < q.Count(); i++)
                    command_buffer.DestroyEntity(q.GetEntity(i));
            }
        }
    }
}

HK_NAMESPACE_END
