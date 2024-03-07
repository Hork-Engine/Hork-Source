#pragma once

#include <Engine/ECS/ECS.h>

HK_NAMESPACE_BEGIN

struct CompoundEntityComponent
{
    TVector<ECS::EntityHandle> Entities;
};

HK_NAMESPACE_END
