#pragma once

#include <Engine/ECS/ECS.h>

#include "SceneGraph.h"

HK_NAMESPACE_BEGIN

ECS::EntityHandle CreateSceneNode(ECS::CommandBuffer& commandBuffer, SceneNodeDesc const& desc);
ECS::EntityHandle CreateSkybox(ECS::CommandBuffer& commandBuffer, ECS::EntityHandle parent);

HK_NAMESPACE_END
