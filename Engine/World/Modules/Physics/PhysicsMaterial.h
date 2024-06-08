#pragma once

#include <Engine/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

class PhysicsMaterial
{
public:
    float       Restitution = 0.0f;
    float       Friction = 0.2f;
};

HK_NAMESPACE_END
