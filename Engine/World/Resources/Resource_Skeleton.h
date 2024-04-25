#pragma once

#include "ResourceHandle.h"
#include "ResourceBase.h"

#include <Engine/Geometry/Skinning.h>
#include <Engine/Geometry/IK/FABRIKSolver.h>

#include <Engine/Core/Containers/ArrayView.h>

HK_NAMESPACE_BEGIN

class ResourceManager;
struct SkeletonPose;

class SkeletalAnimation : public RefCounted
{
public:
    SkeletalAnimation() = default;
    SkeletalAnimation(IBinaryStreamReadInterface& stream);
    SkeletalAnimation(int frameCount, float frameDelta, Transform const* transforms, int transformsCount, AnimationChannel const* animatedJoints, int numAnimatedJoints, BvAxisAlignedBox const* bounds);

    StringView GetName() const { return m_Name; }

    TVector<AnimationChannel> const& GetChannels() const { return m_Channels; }
    TVector<Transform> const& GetTransforms() const { return m_Transforms; }

    int GetFrameCount() const { return m_FrameCount; }
    float GetFrameDelta() const { return m_FrameDelta; }
    float GetFrameRate() const { return m_FrameRate; }
    float GetDurationInSeconds() const { return m_DurationInSeconds; }
    float GetDurationNormalizer() const { return m_DurationNormalizer; }
    TVector<BvAxisAlignedBox> const& GetBoundingBoxes() const { return m_Bounds; }
    bool IsValid() const { return m_bIsAnimationValid; }

    void Read(IBinaryStreamReadInterface& stream);

    void Write(IBinaryStreamWriteInterface& stream) const;

public:
    void Initialize();

    String m_Name;
    TVector<AnimationChannel> m_Channels;
    TVector<Transform> m_Transforms;
    TVector<BvAxisAlignedBox> m_Bounds;
    int m_FrameCount = 0;           // frames count
    float m_FrameDelta = 0;         // fixed time delta between frames
    float m_FrameRate = 60;         // frames per second (animation speed) FrameRate = 1.0 / FrameDelta
    float m_DurationInSeconds = 0;  // animation duration is FrameDelta * ( FrameCount - 1 )
    float m_DurationNormalizer = 1; // to normalize track timeline (DurationNormalizer = 1.0 / DurationInSeconds)
    bool m_bIsAnimationValid = false;
};

class SkeletonResource : public ResourceBase
{
public:
    static const uint8_t Type = RESOURCE_SKELETON;
    static const uint8_t Version = 1;

    SkeletonResource() = default;
    SkeletonResource(IBinaryStreamReadInterface& stream, ResourceManager* resManager);
    SkeletonResource(SkeletonJoint* joints, int jointsCount, BvAxisAlignedBox const& bindposeBounds);
    ~SkeletonResource();

    int FindJoint(StringView name) const;

    uint32_t GetJointsCount() const { return m_Joints.Size(); }

    TVector<SkeletonJoint> const& GetJoints() const { return m_Joints; }

    BvAxisAlignedBox const& GetBindposeBounds() const { return m_BindposeBounds; }

    uint32_t FindAnimation(StringView name) const;

    void SetAnimations(TArrayView<TRef<SkeletalAnimation>> const& animations);
    TVector<TRef<SkeletalAnimation>> const& GetAnimations() const { return m_Animations; }

    int AddSolver(StringView name, int jointIndex, int chainSize);

    FABRIKSolver* GetSolver(int solverHandle)
    {
        return m_Solvers[solverHandle - 1].Solver;
    }

    void Solve(struct SkeletonPose* pose, int solverHandle, IkTransform const& target);

    bool Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager);

    void Write(IBinaryStreamWriteInterface& stream) const;

public:
    TVector<SkeletonJoint> m_Joints;
    BvAxisAlignedBox m_BindposeBounds;
    TVector<TRef<SkeletalAnimation>> m_Animations;

    struct SolverInfo
    {
        String Name;
        FABRIKSolver* Solver;
        int JointIndex;
    };

    TVector<SolverInfo> m_Solvers;
};

using SkeletonHandle = ResourceHandle<SkeletonResource>;

struct SkeletonPose : RefCounted
{
    SkeletonHandle Skeleton;

    float SummaryWeights[MAX_SKELETON_JOINTS];

    TVector<Float3x4> m_RelativeTransforms;
    TVector<Float3x4> m_AbsoluteTransforms;

    alignas(16) Float3x4 m_SkinningTransforms[MAX_SKELETON_JOINTS]; // TODO: Dynamicaly sized vector?

    // GPU memory offset/size for the mesh skin
    size_t m_SkeletonOffset = 0;
    size_t m_SkeletonOffsetMB = 0;
    size_t m_SkeletonSize = 0;

    BvAxisAlignedBox m_Bounds;

    Float3x4 const& GetJointTransform(uint32_t jointIndex) const
    {
        //HK_ASSERT(jointIndex + 1 < m_AbsoluteTransforms.Size());
        if (jointIndex + 1 >= m_AbsoluteTransforms.Size())
        {
            return Float3x4::Identity();
        }

        return m_AbsoluteTransforms[jointIndex + 1];
    }

    bool IsValid() const
    {
        return !m_AbsoluteTransforms.IsEmpty();
    }
};

HK_NAMESPACE_END
