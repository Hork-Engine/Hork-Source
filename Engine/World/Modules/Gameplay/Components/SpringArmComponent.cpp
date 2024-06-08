#include "SpringArmComponent.h"
#include <Engine/World/Modules/Physics/PhysicsInterface.h>
#include <Engine/World/World.h>

HK_NAMESPACE_BEGIN

void SpringArmComponent::FixedUpdate()
{
    ShapeCastResult result;
    ShapeCastFilter castFilter;

    castFilter.IgonreBackFaces = false;
    castFilter.BroadphaseLayers
        .AddLayer(BroadphaseLayer::Static)
        .AddLayer(BroadphaseLayer::Dynamic);

    auto* owner = GetOwner();
    auto* world = GetWorld();

    Float3 dir = -owner->GetWorldDirection();
    Float3 worldPos = owner->GetWorldPosition() - dir * ActualDistance;

    PhysicsInterface& physics = world->GetInterface<PhysicsInterface>();

    if (physics.CastSphereClosest(worldPos, dir * DesiredDistance, SphereCastRadius, result, castFilter))
    {
        float distance = DesiredDistance * result.Fraction;

        ActualDistance = Math::Lerp(ActualDistance, distance, 0.5f);
        if (ActualDistance < MinDistance)
            ActualDistance = MinDistance;
    }
    else
    {
        ActualDistance = Math::Lerp(ActualDistance, DesiredDistance, Speed * world->GetTick().FixedTimeStep);
    }

    Float3 localPosition = owner->GetPosition();
    localPosition.Z = ActualDistance;

    owner->SetPosition(localPosition);
}

HK_NAMESPACE_END
