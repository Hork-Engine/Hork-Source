#pragma once

#include <Engine/ECS/ECS.h>

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

struct NodeMotionTimer
{
    float Time{};
    float LoopTime{};
};

class NodeMotion;

struct NodeMotionComponent
{
    NodeMotionComponent(size_t nodeId, NodeMotion* animation, ECS::EntityHandle timer) :
        NodeId(nodeId),
        pAnimation(animation),
        Timer(timer)
    {}

    size_t NodeId{};
    NodeMotion* pAnimation{};
    ECS::EntityHandle Timer{};
};

HK_NAMESPACE_END
