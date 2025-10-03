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

#include "SkeletonPoseComponent.h"

#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>
#include <Hork/Runtime/World/DebugRenderer.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawSkeletons("com_DrawSkeletons"_s, "0"_s);

void SkeletonPoseComponent::SetMesh(MeshHandle handle)
{
    m_Mesh = handle;
}

void SkeletonPoseComponent::BeginPlay()
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    MeshResource* mesh = resourceMngr.TryGet(m_Mesh);
    if (!mesh)
        return;

    auto skeleton = mesh->GetSkeleton();
    if (!skeleton)
        return;

    auto jointCount = skeleton->num_joints();
    m_Pose.Attach(new SkeletonPose);
    m_Pose->m_ModelMatrices.Resize(jointCount, SimdFloat4x4::identity());
    m_Pose->m_LocalMatrices.Resize(skeleton->num_soa_joints());
    Core::Memcpy(m_Pose->m_LocalMatrices.ToPtr(), skeleton->joint_rest_poses().data(), sizeof(m_Pose->m_LocalMatrices[0]) * m_Pose->m_LocalMatrices.Size());
}

void SkeletonPoseComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawSkeletons && m_Pose)
    {
        if (MeshResource* resource = GameApplication::sGetResourceManager().TryGet(m_Mesh))
        {
            Float3x4 worldTransform = GetOwner()->GetWorldTransformMatrix();
            alignas(16) Float4x4 jointTransform;

            renderer.SetDepthTest(false);
            for (int jointIndex = 0, count = resource->GetJointCount(); jointIndex < count; ++jointIndex)
            {
                Simd::StoreFloat4x4(m_Pose->m_ModelMatrices[jointIndex].cols, jointTransform);

                auto t = worldTransform * Float3x4(jointTransform.Transposed());

                Float3 v1 = t.DecomposeTranslation();
                Float3x3 r1 = t.DecomposeRotation();

                renderer.SetColor(Color4(1, 0, 0, 1));
                renderer.DrawOrientedBox(v1, r1, Float3(0.01f));

                int parent = resource->GetJointParent(jointIndex);
                if (parent >= 0)
                {
                    Simd::StoreFloat4x4(m_Pose->m_ModelMatrices[parent].cols, jointTransform);

                    auto t0 = worldTransform * Float3x4(jointTransform.Transposed());
                    Float3 v0 = t0.DecomposeTranslation();

                    renderer.SetColor(Color4(1, 1, 0, 1));
                    renderer.DrawLine(v0, v1);
                }
            }
        }
    }
}

HK_NAMESPACE_END
