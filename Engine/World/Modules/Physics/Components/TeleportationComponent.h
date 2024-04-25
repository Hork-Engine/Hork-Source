#pragma once

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

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
