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

#include "SkinnedMeshComponent.h"
#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawSkeleton("com_DrawSkeleton"s, "0"s);

void SkinnedMeshComponent::UpdatePoses()
{
    SkeletonPose* pose = Pose;
    if (!pose)
        return;

    float timeStep = GetWorld()->GetTick().FixedTimeStep;

    if (AnimInstance)
        AnimInstance->Update(timeStep, pose);
#if 0
                SkeletonResource* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
                if (skeleton)
                {
                    {
                        SkeletonJoint const* joints = skeleton->GetJoints().ToPtr();
                        uint32_t jointsCount = skeleton->GetJointsCount();

                        for (uint32_t jointIndex = 0; jointIndex < jointsCount; jointIndex++)
                        {
                            pose->m_AbsoluteTransforms[jointIndex + 1] = pose->m_AbsoluteTransforms[joints[jointIndex].Parent + 1] * pose->m_RelativeTransforms[jointIndex];
                        }
                    }

                    IkTransform target;

                    int joint = skeleton->FindJoint("Bone.011");
                    target.Rotation.FromMatrix(pose->m_AbsoluteTransforms[joint+1].DecomposeRotation());
                    target.Position = pose->m_AbsoluteTransforms[joint+1].DecomposeTranslation();

                    target.Position.Y += (sin(frame.VariableTime*2) * 0.5f + 0.5f) * 0.6f;
                    //if (target.Position.Y < 0.6f)
                    //    target.Position.Y = 0.6f;

                    skeleton->Solve(pose, 1, target);

                    SkeletonJoint const* joints = skeleton->GetJoints().ToPtr();
                    uint32_t jointsCount = skeleton->GetJointsCount();

                    for (uint32_t jointIndex = 0; jointIndex < jointsCount; jointIndex++)
                    {
                        pose->m_AbsoluteTransforms[jointIndex + 1] = pose->m_AbsoluteTransforms[joints[jointIndex].Parent + 1] * pose->m_RelativeTransforms[jointIndex];
                    }
                }
#else

    SkeletonResource* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
    if (skeleton)
    {
        SkeletonJoint const* joints = skeleton->GetJoints().ToPtr();
        uint32_t jointsCount = skeleton->GetJointsCount();
        for (uint32_t jointIndex = 0; jointIndex < jointsCount; jointIndex++)
        {
            pose->m_AbsoluteTransforms[jointIndex + 1] = pose->m_AbsoluteTransforms[joints[jointIndex].Parent + 1] * pose->m_RelativeTransforms[jointIndex];
        }
    }

#endif
}

void SkinnedMeshComponent::UpdateSkins()
{
    SkeletonPose* pose = Pose;

    if (!pose)
        return;

    if (!pose->IsValid())
        return;

    SkeletonResource const* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
    if (!skeleton)
        return;

    MeshResource const* meshResource = GameApplication::GetResourceManager().TryGet(Mesh);
    if (!meshResource)
        return;

    MeshSkin const& skin = meshResource->GetSkin();

    pose->m_SkeletonSize = skin.JointIndices.Size() * sizeof(Float3x4);
    if (pose->m_SkeletonSize > 0)
    {
        StreamedMemoryGPU* streamedMemory = GameApplication::GetFrameLoop().GetStreamedMemoryGPU();

        // Write joints from previous frame
        pose->m_SkeletonOffsetMB = streamedMemory->AllocateJoint(pose->m_SkeletonSize, pose->m_SkinningTransforms);

        // Write joints from current frame
        pose->m_SkeletonOffset = streamedMemory->AllocateJoint(pose->m_SkeletonSize, nullptr);
        Float3x4* data = (Float3x4*)streamedMemory->Map(pose->m_SkeletonOffset);
        for (int j = 0; j < skin.JointIndices.Size(); j++)
        {
            int jointIndex = skin.JointIndices[j];
            data[j] = pose->m_SkinningTransforms[j] = pose->m_AbsoluteTransforms[jointIndex + 1] * skin.OffsetMatrices[j];
        }
    }
    else
    {
        pose->m_SkeletonOffset = pose->m_SkeletonOffsetMB = 0;
    }
}

void SkinnedMeshComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawSkeleton)
    {
        renderer.SetColor(Color4(1, 0, 0, 1));
        renderer.SetDepthTest(false);

        Float3x4 tranformMat;

        SkeletonPose* pose = Pose;

        SkeletonResource* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
        if (skeleton)
        {
            auto owner = GetOwner();
            tranformMat.Compose(owner->GetWorldPosition(), owner->GetWorldRotation().ToMatrix3x3(), owner->GetWorldScale());

            Vector<SkeletonJoint> const& joints = skeleton->GetJoints();
            for (int jointIndex = 0, count = joints.Size(); jointIndex < count; ++jointIndex)
            {
                SkeletonJoint const& joint = joints[jointIndex];

                Float3x4 t = tranformMat * pose->GetJointTransform(jointIndex);
                Float3 v1 = t.DecomposeTranslation();

                renderer.DrawOrientedBox(v1, t.DecomposeRotation(), Float3(0.01f));

                if (joint.Parent >= 0)
                {
                    Float3 v0 = (tranformMat * pose->GetJointTransform(joint.Parent)).DecomposeTranslation();
                    renderer.DrawLine(v0, v1);
                }
            }
        }
    }

    // TODO: Draw bounding box
}

HK_NAMESPACE_END
