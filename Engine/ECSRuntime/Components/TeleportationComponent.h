#pragma once

#include <Engine/Geometry/VectorMath.h>
#include <Engine/Geometry/Quat.h>

HK_NAMESPACE_BEGIN

struct TeleportationComponent
{
    Float3 DestPosition;
    Quat DestRotation;

    TeleportationComponent(Float3 destPosition,
                           Quat destRotation) :
        DestPosition(destPosition),
        DestRotation(destRotation)
    {}
};

HK_NAMESPACE_END
