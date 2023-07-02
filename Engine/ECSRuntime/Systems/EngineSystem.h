#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Core/Ref.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

class EngineSystemECS : public RefCounted
{
public:
    virtual ~EngineSystemECS() = default;
    virtual void DrawDebug(DebugRenderer& renderer) {}
};

HK_NAMESPACE_END
