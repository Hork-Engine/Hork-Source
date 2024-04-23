#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Core/Ref.h>
#include "../GameFrame.h"

HK_NAMESPACE_BEGIN

enum GAMEPLAY_SYSTEM_EXECUTION
{
    GAMEPLAY_SYSTEM_VARIABLE_UPDATE = 1,
    GAMEPLAY_SYSTEM_FIXED_UPDATE = 2,
    GAMEPLAY_SYSTEM_POST_PHYSICS_UPDATE = 4,
    GAMEPLAY_SYSTEM_LATE_UPDATE = 8
};
HK_FLAG_ENUM_OPERATORS(GAMEPLAY_SYSTEM_EXECUTION)

class DebugRenderer;

class GameplaySystemECS : public RefCounted
{
public:
    bool bTickEvenWhenPaused = false;

    virtual ~GameplaySystemECS() = default;
    virtual void VariableUpdate(float timeStep) {}
    virtual void FixedUpdate(GameFrame const& frame) {}
    virtual void PostPhysicsUpdate(GameFrame const& frame) {}
    virtual void LateUpdate(float timeStep) {}
    virtual void DrawDebug(DebugRenderer& renderer) {}
};

HK_NAMESPACE_END
