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

#include "AnimationPlayerSimple.h"

#include <Hork/Runtime/World/TickFunction.h>
#include <Hork/Math/Simd/Simd.h>

HK_NAMESPACE_BEGIN

// TODO: Move IK Chain to Skeleton! Save inside asset
struct IKChain
{
    /// Indices of the joints that are IKed for look-at purpose.
    /// Joints must be from the same hierarchy (all ancestors of the first joint listed) and ordered from child to parent.
    Vector<int>             JointsChain;

    /// Defines Up vectors for each joint. This is skeleton/rig dependant
    Vector<SimdFloat4>      UpVectors;

    int                     Size() const { return JointsChain.Size(); }

    bool                    Init(OzzSkeleton const& skeleton, ArrayView<StringView> jointNames);

    bool                    ValidateJointsOrder(OzzSkeleton const& skeleton, ArrayView<int> joints);
};

class IkLookAtComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

                            IkLookAtComponent() {}

    IKChain                 m_IkChain; // TODO: Move to skeleton!

    void                    SetMesh(MeshHandle mesh) { m_Mesh = mesh; }
    MeshHandle              GetMesh() const { return m_Mesh; }

    void                    SetPose(SkeletonPose* pose) { m_Pose = pose; }
    SkeletonPose*           GetPose() { return m_Pose; }
    SkeletonPose const*     GetPose() const { return m_Pose; }

    // Forward vector in head local-space.
    void                    SetHeadForward(Float3 const& forward) { m_HeadForward = forward; }
    Float3 const&           GetHeadForward() const { return m_HeadForward; }

    void                    SetEyesOffset(Float3 const& offset) { m_EyesOffset = offset; }
    Float3 const&           GetEyesOffset() const { return m_EyesOffset; }

    // Overall weight given to the IK on the full chain. This allows blending in
    // and out of IK.
    void                    SetBlendWeight(float weight) { m_BlendWeight = weight; }
    float                   GetBlendWeight() const { return m_BlendWeight; }

    // Weight given to every joint of the chain. If any joint has a weight of 1,
    // no other following joint will contribute (as the target will be reached).
    void                    SetJointWeight(float weight) { m_JointWeight = weight; }
    float                   GetJointWeight() const { return m_JointWeight; }

    void                    SetTargetPosition(Float3 const& position) { m_TargetPosition = position; }
    Float3 const&           GetTargetPosition() const { return m_TargetPosition; }

    // Internal

    void                    Update();
    void                    DrawDebug(DebugRenderer& renderer);

private:
    bool                    UpdateLookAtIK(Float3 const& target, OzzSkeleton const& skeleton);

    MeshHandle              m_Mesh;
    Ref<SkeletonPose>       m_Pose;
    Float3                  m_HeadForward = Float3::sAxisZ();
    Float3                  m_EyesOffset{0, .07f, .1f};
    Float3                  m_TargetPosition;
    float                   m_BlendWeight = 1;
    float                   m_JointWeight = 0.5f;
};

namespace TickGroup_Update
{
    template <>
    HK_INLINE void InitializeTickFunction<IkLookAtComponent>(TickFunctionDesc& desc)
    {
        desc.AddPrerequisiteComponent<AnimationPlayerSimple>();
    }
}

HK_NAMESPACE_END
