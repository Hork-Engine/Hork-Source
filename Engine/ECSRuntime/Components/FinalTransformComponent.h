#pragma once

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

struct FinalTransformComponent
{
    Float3 Position;
    Quat Rotation;
    Float3 Scale = Float3(1);

    FinalTransformComponent() = default;

    Float3x4 ToMatrix() const
    {
        Float3x4 matrix;
        matrix.Compose(Position, Rotation.ToMatrix3x3(), Scale);
        return matrix;
    }
};

HK_NAMESPACE_END
