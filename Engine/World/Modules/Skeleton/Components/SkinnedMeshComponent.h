#pragma once

#include <Engine/World/Modules/Skeleton/SkeletalAnimation.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Component.h>
#include <Engine/World/TickFunction.h>

HK_NAMESPACE_BEGIN

class SkinnedMeshComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    TRef<AnimationInstance> AnimInstance;
    TRef<SkeletonPose> Pose;
    MeshHandle Mesh;

    void FixedUpdate()
    {
        UpdatePoses();
    }

    void LateUpdate()
    {
        UpdateSkins();
    }
    
    void DrawDebug(DebugRenderer& renderer);

private:
    void UpdatePoses();
    void UpdateSkins();
};

namespace TickGroup_FixedUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<SkinnedMeshComponent>(TickFunctionDesc& desc)
    {
        desc.Name.FromString("Update Poses");
    }
}

namespace TickGroup_LateUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<SkinnedMeshComponent>(TickFunctionDesc& desc)
    {
        desc.Name.FromString("Update Skins");
    }
}

HK_NAMESPACE_END
