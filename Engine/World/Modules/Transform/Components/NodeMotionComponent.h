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
    NodeMotionComponent(uint32_t nodeId, NodeMotion* animation, ECS::EntityHandle timer) :
        NodeId(nodeId),
        pAnimation(animation),
        Timer(timer)
    {}

    uint32_t NodeId{};
    NodeMotion* pAnimation{};
    ECS::EntityHandle Timer{};
};

HK_NAMESPACE_END
