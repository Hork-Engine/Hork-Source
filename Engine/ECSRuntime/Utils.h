#pragma once

#include <Engine/ECS/ECS.h>

HK_NAMESPACE_BEGIN

ECS::EntityHandle CreateSkybox(ECS::CommandBuffer& commandBuffer, ECS::EntityHandle parent);

HK_NAMESPACE_END
