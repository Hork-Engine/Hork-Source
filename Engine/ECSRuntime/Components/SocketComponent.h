#pragma once

#include "../SkeletalAnimation.h"

HK_NAMESPACE_BEGIN

struct SocketComponent
{
    TRef<SkeletonPose> Pose;
    int SocketIndex{};
};

HK_NAMESPACE_END
