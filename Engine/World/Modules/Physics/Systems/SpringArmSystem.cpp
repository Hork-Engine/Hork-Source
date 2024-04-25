#include "SpringArmSystem.h"
#include <Engine/World/Common/GameFrame.h>
#include <Engine/World/World.h>

#include <Engine/World/Modules/Transform/Components/WorldTransformComponent.h>
#include <Engine/World/Modules/Transform/Components/TransformComponent.h>
#include "../Components/SpringArmComponent.h"

HK_NAMESPACE_BEGIN

SpringArmSystem::SpringArmSystem(World* world) :
    m_World(world),
    m_PhysicsInterface(world->GetPhysicsInterface())
{}

void SpringArmSystem::PostPhysicsUpdate(GameFrame const& frame)
{
    using Query = ECS::Query<>
        ::ReadOnly<WorldTransformComponent>
        ::Required<TransformComponent>
        ::Required<SpringArmComponent>;

    int frameNum = frame.StateIndex;

    ShapeCastResult result;
    ShapeCastFilter castFilter;

    castFilter.bIgonreBackFaces = false;
    castFilter.BroadphaseLayerMask
        .AddLayer(BroadphaseLayer::MOVING)
        .AddLayer(BroadphaseLayer::NON_MOVING);

    for (Query::Iterator q(*m_World); q; q++)
    {
        WorldTransformComponent const* worldTransform = q.Get<WorldTransformComponent>();
        TransformComponent* transform = q.Get<TransformComponent>();
        SpringArmComponent* springArm = q.Get<SpringArmComponent>();

        for (int i = 0; i < q.Count(); i++)
        {
            Float3 dir = worldTransform[i].Rotation[frameNum].ZAxis();
            Float3 worldPos = worldTransform[i].Position[frameNum] - dir * springArm[i].ActualDistance;

            if (m_PhysicsInterface.CastSphereClosest(worldPos, dir * springArm[i].DesiredDistance, SpringArmComponent::SPRING_ARM_SPHERE_CAST_RADIUS, result, castFilter))
            {
                float distance = springArm[i].DesiredDistance * result.Fraction;

                springArm[i].ActualDistance = Math::Lerp(springArm[i].ActualDistance, distance, 0.5f);

                if (springArm[i].ActualDistance < springArm[i].MinDistance)
                    springArm[i].ActualDistance = springArm[i].MinDistance;
            }
            else
            {
                springArm[i].ActualDistance = Math::Lerp(springArm[i].ActualDistance, springArm[i].DesiredDistance, springArm[i].Speed * frame.FixedTimeStep);
            }

            transform[i].Position.Z = springArm[i].ActualDistance;
        }
    }
}

HK_NAMESPACE_END
