#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

enum SCENE_NODE_FLAGS : uint8_t
{
    SCENE_NODE_FLAGS_DEFAULT = 0,
    SCENE_NODE_ABSOLUTE_POSITION = 1,
    SCENE_NODE_ABSOLUTE_ROTATION = 2,
    SCENE_NODE_ABSOLUTE_SCALE = 4
};
HK_FLAG_ENUM_OPERATORS(SCENE_NODE_FLAGS);

/// Regular scene node
struct SceneNodeDesc
{
    /// Scene node parent
    ECS::EntityHandle Parent;

    /// Position of the node
    Float3            Position;

    /// Rotation of the node
    Quat              Rotation;

    /// Scale of the node
    Float3            Scale = Float3(1);

    SCENE_NODE_FLAGS  NodeFlags = SCENE_NODE_FLAGS_DEFAULT;

    bool              bMovable = false;

    /// Perform node transform interpolation between fixed time steps
    bool              bTransformInterpolation = true;
};

ECS::EntityHandle CreateSceneNode(ECS::CommandBuffer& commandBuffer, SceneNodeDesc const& desc);

HK_NAMESPACE_END
