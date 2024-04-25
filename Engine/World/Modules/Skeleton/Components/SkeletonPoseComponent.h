#pragma once

#include <Engine/World/Modules/Skeleton/SkeletalAnimation.h>
#include <Engine/World/Resources/Resource_Mesh.h>

HK_NAMESPACE_BEGIN

struct SkeletonPoseComponent
{
    TRef<SkeletonPose> Pose;
    MeshHandle Mesh;
};

HK_NAMESPACE_END
