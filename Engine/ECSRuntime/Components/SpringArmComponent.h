#pragma once

#include <Engine/ECS/ECS.h>

HK_NAMESPACE_BEGIN

struct SpringArmComponent
{
    static constexpr float SPRING_ARM_SPHERE_CAST_RADIUS = 0.3f;

    float DesiredDistance{};
    float ActualDistance{};
    float MinDistance{0.2f};
    float Speed{2};
};

HK_NAMESPACE_END
