#pragma once

#include <Engine/ECS/ECS.h>

#include "SceneGraph.h"

HK_NAMESPACE_BEGIN

/// Regular scene node
struct SceneNodeDesc
{
    /// Scene node parent
    ECS::EntityHandle Parent;

    /// Position of the node
    Float3 Position;

    /// Rotation of the node
    Quat Rotation;

    /// Scale of the node
    Float3 Scale = Float3(1);

    SCENE_NODE_FLAGS NodeFlags = SCENE_NODE_FLAGS_DEFAULT;

    bool bMovable = false;

    /// Perform node transform interpolation between fixed time steps
    bool bTransformInterpolation = true;
};

ECS::EntityHandle CreateSceneNode(ECS::CommandBuffer& commandBuffer, SceneNodeDesc const& desc);
ECS::EntityHandle CreateSkybox(ECS::CommandBuffer& commandBuffer, ECS::EntityHandle parent);

HK_NAMESPACE_END
