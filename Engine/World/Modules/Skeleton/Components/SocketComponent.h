#pragma once

#include <Engine/World/Modules/Skeleton/SkeletalAnimation.h>
#include "SkinnedMeshComponent.h"

HK_NAMESPACE_BEGIN

class DebugRenderer;

class SocketComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    TRef<SkeletonPose>  Pose;
    int                 SocketIndex{};
    
    void FixedUpdate();

    void DrawDebug(DebugRenderer& renderer);
};

namespace TickGroup_FixedUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<SocketComponent>(TickFunctionDesc& desc)
    {
        desc.Name.FromString("Update Sockets");
        desc.AddPrerequisiteComponent<SkinnedMeshComponent>();
    }
}

HK_NAMESPACE_END
