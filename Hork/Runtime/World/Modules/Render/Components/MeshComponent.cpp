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

#include "MeshComponent.h"

#include <Hork/Core/ConsoleVar.h>

#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawMeshDebug("com_DrawMeshDebug"_s, "0"_s);
ConsoleVar com_DrawMeshBounds("com_DrawMeshBounds"_s, "0"_s);

void MeshComponent::SetMaterial(Material* material)
{
    SetMaterial(0, material);
}

void MeshComponent::SetMaterial(uint32_t index, Material* material)
{
    while (m_Materials.Size() <= index)
        m_Materials.EmplaceBack();
    m_Materials[index] = material;
}

Material* MeshComponent::GetMaterial(uint32_t index)
{
    return (index < m_Materials.Size()) ? m_Materials[index].RawPtr() : nullptr;
}

void MeshComponent::SetMaterialCount(uint32_t count)
{
    m_Materials.Reserve(count);
    while (m_Materials.Size() < count)
        m_Materials.EmplaceBack();
    m_Materials.Resize(count);
}

uint32_t MeshComponent::GetMaterialCount() const
{
    return m_Materials.Size();
}

void MeshComponent::SetLocalBoundingBox(BvAxisAlignedBox const& boundingBox)
{
    m_LocalBoundingBox = boundingBox;
    m_WorldBoundingBox = m_LocalBoundingBox.Transform(GetOwner()->GetWorldTransformMatrix());
}

void MeshComponent::UpdateWorldBoundingBox()
{
    m_WorldBoundingBox = m_LocalBoundingBox.Transform(GetOwner()->GetWorldTransformMatrix());
}

void MeshComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawMeshDebug)
    {
        if (MeshResource* resource = GameApplication::sGetResourceManager().TryGet(m_Resource))
        {
            renderer.PushTransform(GetOwner()->GetWorldTransformMatrix());

            renderer.SetDepthTest(false);
            renderer.SetColor(Color4::sWhite());
            renderer.DrawAABB(resource->GetBoundingBox());

            int surfaceCount = resource->GetSurfaceCount();
            for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex)
            {
                MeshSurface const* surface = resource->GetSurfaces() + surfaceIndex;

                renderer.DrawAABB(surface->BoundingBox);

                auto& bvhNodes = surface->Bvh.GetNodes();
                if (!bvhNodes.IsEmpty())
                {
                    for (BvhNode const& node : bvhNodes)
                    {
                        if (node.IsLeaf())
                            renderer.DrawAABB(node.Bounds);
                    }
                }
            }
            renderer.PopTransform();
        }
    }

    if (com_DrawMeshBounds)
    {
        renderer.SetDepthTest(false);
        if (m_Resource)
            renderer.SetColor(Color4(1, 1, 1, 1));
        else if (m_ProceduralData)
            renderer.SetColor(Color4(0.5f, 1, 0.5f, 1));
        else
            renderer.SetColor(Color4(1, 0, 0, 1));
        renderer.DrawAABB(m_WorldBoundingBox);
    }
}

void StaticMeshComponent::BeginPlay()
{
    m_RenderTransform  = GetOwner()->GetWorldTransformMatrix();
    m_RotationMatrix   = GetOwner()->GetWorldRotation().ToMatrix3x3();
    m_WorldBoundingBox = m_LocalBoundingBox.Transform(GetOwner()->GetWorldTransformMatrix());
}

void DynamicMeshComponent::SkipInterpolation()
{
    m_Transform[0].Position = m_Transform[1].Position = GetOwner()->GetWorldPosition();
    m_Transform[0].Rotation = m_Transform[1].Rotation = GetOwner()->GetWorldRotation();
    m_Transform[0].Scale    = m_Transform[1].Scale    = GetOwner()->GetWorldScale();

    m_LastFrame = 0;
}

void DynamicMeshComponent::BeginPlay()
{
    m_Transform[0].Position = m_Transform[1].Position = GetOwner()->GetWorldPosition();
    m_Transform[0].Rotation = m_Transform[1].Rotation = GetOwner()->GetWorldRotation();
    m_Transform[0].Scale    = m_Transform[1].Scale    = GetOwner()->GetWorldScale();

    m_RenderTransform[0].Compose(m_Transform[0].Position, m_Transform[0].Rotation.ToMatrix3x3(), m_Transform[0].Scale);
    m_RenderTransform[1] = m_RenderTransform[0];

    m_WorldBoundingBox = m_LocalBoundingBox.Transform(GetOwner()->GetWorldTransformMatrix());

    m_PoseComponent = GetOwner()->GetComponentHandle<SkeletonPoseComponent>();
}

void DynamicMeshComponent::PostTransform()
{
    auto index = GetWorld()->GetTick().StateIndex;

    m_Transform[index].Position = GetOwner()->GetWorldPosition();
    m_Transform[index].Rotation = GetOwner()->GetWorldRotation();
    m_Transform[index].Scale    = GetOwner()->GetWorldScale();
}

void DynamicMeshComponent::PreRender(PreRenderContext const& context)
{
    if (m_LastFrame == context.FrameNum)
        return;  // already called for this frame

    Float3 position = Math::Lerp (m_Transform[context.Prev].Position, m_Transform[context.Cur].Position, context.Frac);
    Quat   rotation = Math::Slerp(m_Transform[context.Prev].Rotation, m_Transform[context.Cur].Rotation, context.Frac);
    Float3 scale    = Math::Lerp (m_Transform[context.Prev].Scale,    m_Transform[context.Cur].Scale,    context.Frac);

    m_RotationMatrix = rotation.ToMatrix3x3();

    m_RenderTransform[context.FrameNum & 1].Compose(position, m_RotationMatrix, scale);

    if (m_LastFrame + 1 != context.FrameNum)
        m_RenderTransform[(context.FrameNum + 1) & 1] = m_RenderTransform[context.FrameNum & 1];

    m_LastFrame = context.FrameNum;

    m_WorldBoundingBox = m_LocalBoundingBox.Transform(GetOwner()->GetWorldTransformMatrix());

    UpdateSkinningMatrices();
}

void DynamicMeshComponent::UpdateSkinningMatrices()
{
    SkeletonPoseComponent* poseComponent = GetWorld()->GetComponent(m_PoseComponent);
    if (poseComponent && poseComponent->GetPose())
    {
        SkeletonPose* pose = poseComponent->GetPose();
        m_SkinningData.Pose = pose;
        m_SkinningData.StreamBuffers.Clear();
        if (MeshResource const* meshResource = GameApplication::sGetResourceManager().TryGet(m_Resource))
        {
            auto& allJointRemaps = meshResource->GetJointRemaps();
            auto& allInverseBindPoses = meshResource->GetInverseBindPoses();
            if (m_SkinningData.SkinningMatrices.Size() != allInverseBindPoses.Size())
                m_SkinningData.SkinningMatrices.Resize(allInverseBindPoses.Size());

            alignas(16) Float4x4 jointTransform;

            for (auto& skin : meshResource->GetSkins())
            {
                auto& buffer = m_SkinningData.StreamBuffers.EmplaceBack();
                buffer.Size = skin.MatrixCount * sizeof(Float3x4);
                HK_ASSERT(buffer.Size > 0);

                StreamedMemoryGPU* streamedMemory = GameApplication::sGetFrameLoop().GetStreamedMemoryGPU();

                buffer.Offset = streamedMemory->AllocateJoint(buffer.Size);
                buffer.OffsetP = streamedMemory->AllocateJoint(buffer.Size);

                Float3x4* data = (Float3x4*)streamedMemory->Map(buffer.Offset);
                Float3x4* dataP = (Float3x4*)streamedMemory->Map(buffer.OffsetP);

                auto* jointRemaps = &allJointRemaps[skin.FirstMatrix];
                auto* inverseBindPoses = &allInverseBindPoses[skin.FirstMatrix];

                for (size_t i = 0; i < skin.MatrixCount; ++i)
                {
                    dataP[i] = m_SkinningData.SkinningMatrices[skin.FirstMatrix + i];

                    Simd::StoreFloat4x4((pose->m_ModelMatrices[jointRemaps[i]] * inverseBindPoses[i]).cols, jointTransform);

                    data[i] = m_SkinningData.SkinningMatrices[skin.FirstMatrix + i] = Float3x4(jointTransform.Transposed());
                }
            }
        }
    }
    else
    {
        m_SkinningData.Pose.Reset();
    }
}

void DynamicMeshComponent::DrawDebug(DebugRenderer& renderer)
{
    MeshComponent::DrawDebug(renderer);
}

HK_NAMESPACE_END
