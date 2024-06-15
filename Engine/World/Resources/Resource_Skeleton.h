/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

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

    Vector<AnimationChannel> const& GetChannels() const { return m_Channels; }
    Vector<Transform> const& GetTransforms() const { return m_Transforms; }

    int GetFrameCount() const { return m_FrameCount; }
    float GetFrameDelta() const { return m_FrameDelta; }
    float GetFrameRate() const { return m_FrameRate; }
    float GetDurationInSeconds() const { return m_DurationInSeconds; }
    float GetDurationNormalizer() const { return m_DurationNormalizer; }
    Vector<BvAxisAlignedBox> const& GetBoundingBoxes() const { return m_Bounds; }
    bool IsValid() const { return m_bIsAnimationValid; }

    void Read(IBinaryStreamReadInterface& stream);

    void Write(IBinaryStreamWriteInterface& stream) const;

public:
    void Initialize();

    String m_Name;
    Vector<AnimationChannel> m_Channels;
    Vector<Transform> m_Transforms;
    Vector<BvAxisAlignedBox> m_Bounds;
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

    Vector<SkeletonJoint> const& GetJoints() const { return m_Joints; }

    BvAxisAlignedBox const& GetBindposeBounds() const { return m_BindposeBounds; }

    uint32_t FindAnimation(StringView name) const;

    void SetAnimations(ArrayView<Ref<SkeletalAnimation>> const& animations);
    Vector<Ref<SkeletalAnimation>> const& GetAnimations() const { return m_Animations; }

    int AddSolver(StringView name, int jointIndex, int chainSize);

    FABRIKSolver* GetSolver(int solverHandle)
    {
        return m_Solvers[solverHandle - 1].Solver;
    }

    void Solve(struct SkeletonPose* pose, int solverHandle, IkTransform const& target);

    bool Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager);

    void Write(IBinaryStreamWriteInterface& stream) const;

public:
    Vector<SkeletonJoint> m_Joints;
    BvAxisAlignedBox m_BindposeBounds;
    Vector<Ref<SkeletalAnimation>> m_Animations;

    struct SolverInfo
    {
        String Name;
        FABRIKSolver* Solver;
        int JointIndex;
    };

    Vector<SolverInfo> m_Solvers;
};

using SkeletonHandle = ResourceHandle<SkeletonResource>;

struct SkeletonPose : RefCounted
{
    SkeletonHandle Skeleton;

    float SummaryWeights[MAX_SKELETON_JOINTS];

    Vector<Float3x4> m_RelativeTransforms;
    Vector<Float3x4> m_AbsoluteTransforms;

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
