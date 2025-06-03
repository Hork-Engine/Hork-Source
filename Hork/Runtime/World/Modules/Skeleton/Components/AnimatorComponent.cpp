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

#include "AnimatorComponent.h"

#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>

HK_NAMESPACE_BEGIN

AnimatorComponent::AnimatorComponent()
{}

AnimatorComponent::AnimatorComponent(AnimatorComponent&& rhs) = default;

AnimatorComponent::~AnimatorComponent()
{}

void AnimatorComponent::SetMesh(MeshHandle handle)
{
   m_Mesh = handle;
}

void AnimatorComponent::BeginPlay()
{
    m_PoseComponent = GetOwner()->GetComponentHandle<SkeletonPoseComponent>();

    auto& resourceMngr = GameApplication::sGetResourceManager();
    MeshResource* mesh = resourceMngr.TryGet(m_Mesh);
    if (!mesh)
        return;

    auto skeleton = mesh->GetSkeleton();
    if (!skeleton)
        return;

    m_AnimPlayer = MakeUnique<AnimationPlayer>(m_AnimGraph, skeleton);
}

void AnimatorComponent::EndPlay()
{
    m_AnimPlayer.Reset();
}

void AnimatorComponent::Update()
{
    if (!m_AnimPlayer)
        return;

    auto& resourceMngr = GameApplication::sGetResourceManager();

    MeshResource* mesh = resourceMngr.TryGet(m_Mesh);
    if (!mesh)
        return;

    auto skeleton = mesh->GetSkeleton();
    if (!skeleton)
        return;

    SkeletonPoseComponent* poseComponent = GetWorld()->GetComponent(m_PoseComponent);
    if (!poseComponent)
        return;

    SkeletonPose* pose = poseComponent->GetPose();
    if (!pose)
        return;

    m_AnimPlayer->Tick(GetWorld()->GetTick().FrameTimeStep, &m_ParameterSet, pose);

    ozz::animation::LocalToModelJob localToModel;
    localToModel.skeleton = skeleton;
    localToModel.input = ozz::span{pose->m_LocalMatrices.ToPtr(), pose->m_LocalMatrices.Size()};
    localToModel.output = ozz::span{pose->m_ModelMatrices.ToPtr(), pose->m_ModelMatrices.Size()};
    localToModel.Run();
}

HK_NAMESPACE_END
