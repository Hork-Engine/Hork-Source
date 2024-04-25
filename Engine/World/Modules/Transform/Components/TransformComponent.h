#pragma once

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

struct TransformComponent
{
    Float3 Position;
    Quat Rotation;
    Float3 Scale = Float3(1);

    TransformComponent() = default;
    TransformComponent(Float3 const& position, Quat const& rotation, Float3 const& scale) :
        Position(position),
        Rotation(rotation),
        Scale(scale)
    {}
};

HK_NAMESPACE_END
