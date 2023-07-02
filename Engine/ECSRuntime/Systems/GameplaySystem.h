#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Core/Ref.h>
#include "../GameFrame.h"

HK_NAMESPACE_BEGIN

enum GAMEPLAY_SYSTEM_EXECUTION
{
    GAMEPLAY_SYSTEM_VARIABLE_TIMESTEP = 1,
    GAMEPLAY_SYSTEM_FIXED_TIMESTEP = 2,
};
HK_FLAG_ENUM_OPERATORS(GAMEPLAY_SYSTEM_EXECUTION)

class GameplaySystemECS : public RefCounted
{
public:
    virtual ~GameplaySystemECS() = default;
    virtual void VariableTimestepUpdate(float timeStep) {}
    virtual void FixedTimestepUpdate(GameFrame const& frame) {}
};

HK_NAMESPACE_END
