#pragma once

#include <Engine/Geometry/VectorMath.h>
#include <Engine/Geometry/Quat.h>
#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

enum INTERPOLATION_TYPE
{
    INTERPOLATION_TYPE_STEP,
    INTERPOLATION_TYPE_LINEAR,
    INTERPOLATION_TYPE_CUBIC_SPLINE
};

enum NODE_ANIMATION_PATH_TYPE
{
    NODE_ANIMATION_PATH_TRANSLATION,
    NODE_ANIMATION_PATH_ROTATION,
    NODE_ANIMATION_PATH_SCALE
};

class NodeMotion // Resource
{
public:
    TVector<float> m_AnimationTimes;

    TVector<Float3> m_VectorData;
    TVector<Quat> m_QuaternionData;

    struct Sampler
    {
        // Time offset
        size_t Offset;
        // Keyframe count
        size_t Count;
        // Vector or quaternion data offset
        size_t DataOffset;
        // Data interpolation
        INTERPOLATION_TYPE Interpolation;
    };

    struct AnimationChannel
    {
        Sampler Smp;
        size_t TargetNode;
        NODE_ANIMATION_PATH_TYPE TargetPath;
    };

    TVector<AnimationChannel> m_Channels;

    Float3 SampleVector(Sampler& sampler, float time);
    Quat SampleQuaternion(Sampler& sampler, float time);
};

HK_NAMESPACE_END
