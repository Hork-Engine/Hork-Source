#pragma once

#include <Engine/Geometry/VectorMath.h>
#include <Engine/Geometry/Quat.h>

HK_NAMESPACE_BEGIN

struct WorldTransformComponent
{
    Float3 Position[2];
    Quat   Rotation[2];
    Float3 Scale[2];

    WorldTransformComponent(Float3 const& position, Quat const& rotation, Float3 const& scale)
    {
        Position[0] = Position[1] = position;
        Rotation[0] = Rotation[1] = rotation;
        Scale   [0] = Scale   [1] = scale;
    }
};

HK_NAMESPACE_END
