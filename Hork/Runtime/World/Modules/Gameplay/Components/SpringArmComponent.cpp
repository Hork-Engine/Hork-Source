/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "SpringArmComponent.h"
#include <Hork/Runtime/World/Modules/Physics/PhysicsInterface.h>
#include <Hork/Runtime/World/World.h>

HK_NAMESPACE_BEGIN

void SpringArmComponent::PhysicsUpdate()
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
