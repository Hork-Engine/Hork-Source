#include "TeleportSystem.h"
#include "../GameFrame.h"
#include "../World.h"

#include "../Components/CharacterControllerComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/WorldTransformComponent.h"
#include "../Components/TeleportationComponent.h"

HK_NAMESPACE_BEGIN

TeleportSystem::TeleportSystem(World* world) :
    m_World(world),
    m_PhysicsInterface(world->GetPhysicsInterface())
{}

void TeleportSystem::Update(GameFrame const& frame)
{
    auto& commandBuffer = m_World->GetCommandBuffer(0);
    auto& bodyInterface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    // Teleport characters
    {
        using Query = ECS::Query<>
            ::Required<CharacterControllerComponent>
            ::Required<WorldTransformComponent>
            ::ReadOnly<TeleportationComponent>;

        for (Query::Iterator it(*m_World); it; it++)
        {
            CharacterControllerComponent* characterController = it.Get<CharacterControllerComponent>();
            WorldTransformComponent* worldTransform = it.Get<WorldTransformComponent>();
            TeleportationComponent const* teleport = it.Get<TeleportationComponent>();
            for (int i = 0; i < it.Count(); i++)
            {
                auto position = ConvertVector(teleport[i].DestPosition);
                auto rotation = ConvertQuaternion(teleport[i].DestRotation);

                characterController[i].m_pCharacter->SetPosition(position);
                characterController[i].m_pCharacter->SetRotation(rotation);

                bodyInterface.SetPositionAndRotation(characterController[i].m_BodyId, position, rotation, JPH::EActivation::Activate);

                // This will disable interpolation between frames while teleporting.
                worldTransform[i].Position[frame.PrevStateIndex] = teleport[i].DestPosition;
                worldTransform[i].Rotation[frame.PrevStateIndex] = teleport[i].DestRotation;

                commandBuffer.RemoveComponent<TeleportationComponent>(it.GetEntity(i));
            }
        }
    }

    // Teleport dynamic bodies
    {
        using Query = ECS::Query<>
            ::ReadOnly<DynamicBodyComponent>
            ::Required<WorldTransformComponent>
            ::ReadOnly<TeleportationComponent>
            ::Required<RigidBodyComponent>;

        for (Query::Iterator it(*m_World); it; it++)
        {
            RigidBodyComponent* dynamicBody = it.Get<RigidBodyComponent>();
            WorldTransformComponent* worldTransform = it.Get<WorldTransformComponent>();
            TeleportationComponent const* teleport = it.Get<TeleportationComponent>();
            for (int i = 0; i < it.Count(); i++)
            {
                bodyInterface.SetPositionAndRotation(dynamicBody[i].GetBodyId(), ConvertVector(teleport[i].DestPosition), ConvertQuaternion(teleport[i].DestRotation), JPH::EActivation::Activate);

                // This will disable interpolation between frames while teleporting.
                worldTransform[i].Position[frame.PrevStateIndex] = teleport[i].DestPosition;
                worldTransform[i].Rotation[frame.PrevStateIndex] = teleport[i].DestRotation;

                commandBuffer.RemoveComponent<TeleportationComponent>(it.GetEntity(i));
            }
        }
    }
}

HK_NAMESPACE_END
