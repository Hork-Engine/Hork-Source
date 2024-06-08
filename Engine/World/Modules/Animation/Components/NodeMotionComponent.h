#pragma once

#include <Engine/World/Component.h>
#include <Engine/World/TickFunction.h>
#include <Engine/Core/Ref.h>

HK_NAMESPACE_BEGIN

class NodeMotion;

struct NodeMotionTimer
{
    float Time{};
    float LoopTime{};
    bool  IsPaused{};
};

class NodeMotionComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    uint32_t            NodeID;
    TRef<NodeMotion>    Animation;
    NodeMotionTimer     Timer;

    void FixedUpdate();
};

namespace TickGroup_FixedUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<NodeMotionComponent>(TickFunctionDesc& desc)
    {
        desc.Name.FromString("Update Node Motion");
    }
}

HK_NAMESPACE_END
