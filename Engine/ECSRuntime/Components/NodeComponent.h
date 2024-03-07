#pragma once

#include "../SceneGraph.h"

HK_NAMESPACE_BEGIN

struct NodeComponent
{
    ECS::EntityHandle Parent;

    SCENE_NODE_FLAGS Flags{SCENE_NODE_FLAGS_DEFAULT};

    //[[Hidden]]
    SceneNodeID _ID{};

    NodeComponent() = default;
    NodeComponent(ECS::EntityHandle parent, SCENE_NODE_FLAGS flags) :
        Parent(parent), Flags(flags)
    {}
};

HK_NAMESPACE_END
