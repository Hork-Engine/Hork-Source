/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "IkLookAtComponent.h"

#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

#include <ozz/animation/runtime/ik_aim_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawIKLookAt("com_DrawIKLookAt"_s, "0"_s);

bool IKChain::Init(OzzSkeleton const& skeleton, ArrayView<StringView> jointNames)
{
    int chainLength = jointNames.Size();

    JointsChain.Resize(chainLength);
    UpVectors.Resize(chainLength);

    // Look for each joint in the chain.
    int found = 0;
    for (int i = 0; i < skeleton.num_joints() && found != chainLength; ++i)
    {
        const char* joint_name = skeleton.joint_names()[i];
        if (jointNames[found].Cmp(joint_name) == 0)
        {
            JointsChain[found] = i;

            // Restart search
            ++found;
            i = 0;
        }
    }

    // Exit if all joints weren't found.
    if (found != chainLength)
    {
        LOG("At least a joint wasn't found in the skeleton hierarchy.\n");

        JointsChain.Clear();
        UpVectors.Clear();
        return false;
    }

    // Validates joints are order from child to parent of the same hierarchy.
    if (!ValidateJointsOrder(skeleton, JointsChain))
    {
        LOG("Joints aren't properly ordered, they must be from the same hierarchy (all ancestors of the first joint listed) and ordered from child to parent.\n");

        JointsChain.Clear();
        UpVectors.Clear();
        return false;
    }

    for (int i = 0; i < chainLength; ++i)
        UpVectors[i] = Simd::AxisY();

    return true;
}

bool IKChain::ValidateJointsOrder(OzzSkeleton const& skeleton, ArrayView<int> joints)
{
    const size_t count = joints.Size();
    if (count == 0)
        return true;

    size_t i = 1;
    for (int joint = joints[0]; i != count && joint != OzzSkeleton::kNoParent; )
    {
        int parent = skeleton.joint_parents()[joint];
        if (parent == joints[i])
            ++i;

        joint = parent;
    }

    return count == i;
}

void MultiplySoATransformQuaternion(int jointIndex, SimdQuaternion const& quat, MutableArrayView<SoaTransform> transforms)
{
    HK_ASSERT_(jointIndex >= 0 && static_cast<size_t>(jointIndex) < transforms.Size() * 4, "Joint index out of bound.");

    SoaTransform& transform = transforms[jointIndex / 4];

    SimdQuaternion aosQuats[4];
    Simd::Transpose4x4(&transform.rotation.x, &aosQuats->xyzw);

    SimdQuaternion& aosQuat = aosQuats[jointIndex & 3];
    aosQuat = aosQuat * quat;

    Simd::Transpose4x4(&aosQuats->xyzw, &transform.rotation.x);
}

void IkLookAtComponent::BeginPlay()
{
    m_PoseComponent = GetOwner()->GetComponentHandle<SkeletonPoseComponent>();
}

void IkLookAtComponent::Update()
{
    if (m_BlendWeight < std::numeric_limits<float>::min())
        return;

    SkeletonPoseComponent* poseComponent = GetWorld()->GetComponent(m_PoseComponent);
    if (!poseComponent)
        return;

    SkeletonPose* pose = poseComponent->GetPose();
    if (!pose)
        return;

    auto& resourceMngr = GameApplication::sGetResourceManager();
    MeshResource* mesh = resourceMngr.TryGet(m_Mesh);
    if (!mesh)
        return;

    Float3x4 worldTransformInverse = GetOwner()->GetWorldTransformMatrix().Inversed();
    Float3 targetLocalPosition = worldTransformInverse * m_TargetPosition;

    UpdateLookAtIK(pose, targetLocalPosition, *mesh->GetSkeleton());
}

bool IkLookAtComponent::UpdateLookAtIK(SkeletonPose* pose, Float3 const& target, OzzSkeleton const& skeleton)
{
    auto& models = pose->m_ModelMatrices;
    auto& locals = pose->m_LocalMatrices;

    ozz::animation::IKAimJob ikJob;

    // Pole vector and target position are constant for the whole algorithm, in model-space.
    ikJob.pole_vector = Simd::AxisY();
    ikJob.target = Simd::Load3PtrU(&target.X);

    // The same quaternion will be used each time the job is run.
    SimdQuaternion correction;
    ikJob.joint_correction = &correction;

    int chainLength = m_IkChain.Size();

    int previousJoint = OzzSkeleton::kNoParent;
    for (int i = 0; i < chainLength; ++i)
    {
        int joint = m_IkChain.JointsChain[i];

        // Setups the model-space matrix of the joint being processed by IK.
        ikJob.joint = &models[joint];

        // Setups joint local-space up vector.
        ikJob.up = m_IkChain.UpVectors[i];

        // Setups weights of IK job. The last joint being processed needs a full weight (1.f) to ensure target is reached.
        const bool last = i == chainLength - 1;
        ikJob.weight = m_BlendWeight * (last ? 1.0f : m_JointWeight);

        // Setup offset and forward vector for the current joint being processed.
        if (i == 0)
        {
            // First joint, uses global forward and offset.
            ikJob.offset = Simd::Load3PtrU(&m_EyesOffset.X);
            ikJob.forward = Simd::Load3PtrU(&m_HeadForward.X);
        }
        else
        {
            // Applies previous correction to "forward" and "offset", before bringing them to model-space (_ms).
            const SimdFloat4 corrected_forward_ms = TransformVector(models[previousJoint], TransformVector(correction, ikJob.forward));
            const SimdFloat4 corrected_offset_ms = TransformPoint(models[previousJoint], TransformVector(correction, ikJob.offset));

            // Brings "forward" and "offset" to joint local-space
            const SimdFloat4x4 invJoint = Invert(models[joint]);
            ikJob.forward = TransformVector(invJoint, corrected_forward_ms);
            ikJob.offset = TransformPoint(invJoint, corrected_offset_ms);
        }

        // Runs IK aim job.
        if (!ikJob.Run())
            return false;

        // Apply IK quaternion to its respective local-space transforms.
        MultiplySoATransformQuaternion(joint, correction, locals);

        previousJoint = joint;
    }

    ozz::animation::LocalToModelJob localToModel;
    localToModel.skeleton = &skeleton;
    localToModel.input = ozz::span{pose->m_LocalMatrices.ToPtr(), pose->m_LocalMatrices.Size()};
    localToModel.output = ozz::span{pose->m_ModelMatrices.ToPtr(), pose->m_ModelMatrices.Size()};
    localToModel.from = previousJoint;
    if (!localToModel.Run())
        return false;

    return true;
}

void IkLookAtComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawIKLookAt)
    {
        if (m_IkChain.Size() != 0)
        {
            SkeletonPoseComponent* poseComponent = GetWorld()->GetComponent(m_PoseComponent);
            if (!poseComponent)
                return;

            SkeletonPose* pose = poseComponent->GetPose();
            if (!pose)
                return;

            const int head = m_IkChain.JointsChain[0];
            const SimdFloat4x4 offset = pose->m_ModelMatrices[head] * SimdFloat4x4::Translation(Simd::Load3PtrU(&m_EyesOffset.X));

            alignas(16) Float4x4 eyesTransform;
            Simd::StoreFloat4x4(offset.cols, eyesTransform);

            Float3 p0(eyesTransform.Col3[0], eyesTransform.Col3[1], eyesTransform.Col3[2]);
            Float3 p1(eyesTransform * m_HeadForward);

            renderer.SetColor(Color4::sWhite());
            renderer.DrawSphere(m_TargetPosition, 0.02f);
            renderer.SetColor(Color4::sOrange());
            renderer.PushTransform(GetOwner()->GetWorldTransformMatrix());
            renderer.DrawSphere(p0, 0.02f);
            renderer.DrawLine(p0, p1);
            renderer.PopTransform();
        }
    }
}

HK_NAMESPACE_END
