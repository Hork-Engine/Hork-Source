#pragma once

#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

struct LightAnimationComponent_ECS
{
    //TRef<LightAnimation> Anim; // todo: Keyframes to perform light transitions
    float Time{};
};

HK_NAMESPACE_END
