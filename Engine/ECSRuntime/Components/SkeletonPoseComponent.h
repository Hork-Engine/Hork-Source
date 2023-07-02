#pragma once

#include <Engine/ECSRuntime/SkeletalAnimation.h>
#include <Engine/ECSRuntime/Resources/Resource_Mesh.h>

HK_NAMESPACE_BEGIN

struct SkeletonPoseComponent
{
    TRef<SkeletonPose> Pose;
    MeshHandle Mesh;
};

HK_NAMESPACE_END
