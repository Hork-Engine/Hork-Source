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

#include "AnimationPlayerSimple.h"

#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>

HK_NAMESPACE_BEGIN

struct AnimationPlayerSimple::SamplingContext : ozz::animation::SamplingJob::Context {};

struct AnimationPlayerSimple::UpdateContext
{
    OzzSkeleton const*  Skeleton;
    uint32_t            SoaJointCount;
    float               TimeStep;
};

AnimationPlayerSimple::AnimationPlayerSimple()
{
    m_Pose = MakeRef<SkeletonPose>();
}

AnimationPlayerSimple::AnimationPlayerSimple(AnimationPlayerSimple&& rhs) = default;

AnimationPlayerSimple::~AnimationPlayerSimple()
{}

void AnimationPlayerSimple::PlayAnimation(AnimationHandle handle, float fadeIn, float loopOffset)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();

    AnimationResource* animation = resourceMngr.TryGet(handle);
    if (!animation)
    {
        LOG("AnimationPlayerSimple::PlayAnimation: Animation is not loaded\n");
        return;
    }

    if (m_AnimLayers[m_CurrentLayer].Duration == 0.0f)
        fadeIn = 0.0f;

    int nextLayer = fadeIn > 0.0f ? (m_CurrentLayer + 1) & 1 : m_CurrentLayer;
    auto& layer = m_AnimLayers[nextLayer];

    layer.Handle = handle;
    layer.Duration = animation->GetDuration();
    layer.LoopOffset = Math::Min(loopOffset, layer.Duration);
    layer.Ratio = 0;

    if (!layer.Context)
    {
        layer.Context = MakeUnique<SamplingContext>();
    }

    m_FadeIn = fadeIn;
    m_LayerBlendWeight = 0;
}

void AnimationPlayerSimple::SetMesh(MeshHandle handle)
{
    m_Mesh = handle;
}

void AnimationPlayerSimple::SetPlaybackSpeed(float speed)
{
    m_Speed = speed;
}

float AnimationPlayerSimple::GetPlaybackSpeed() const
{
    return m_Speed;
}

void AnimationPlayerSimple::Seek(float time)
{
    auto& layer = m_AnimLayers[m_CurrentLayer];
    if (layer.Duration > 0.0f)
        layer.Ratio = time / layer.Duration;
    else
        layer.Ratio = 0.0f;
}

float AnimationPlayerSimple::GetPlaybackTime() const
{
    auto& layer = m_AnimLayers[m_CurrentLayer];
    return layer.Ratio * layer.Duration;
}

float AnimationPlayerSimple::GetRatio() const
{
    auto& layer = m_AnimLayers[m_CurrentLayer];
    return layer.Ratio;
}

float AnimationPlayerSimple::GetDuration() const
{
    auto& layer = m_AnimLayers[m_CurrentLayer];
    return layer.Duration;
}

bool AnimationPlayerSimple::IsEnded() const
{
    auto& layer = m_AnimLayers[m_CurrentLayer];
    if (layer.LoopOffset >= 0)
        return false;

    return layer.Ratio == 1.0f;
}

SkeletonPose* AnimationPlayerSimple::GetPose()
{
    return m_Pose;
}

void AnimationPlayerSimple::BeginPlay()
{
    UpdatePose(0.0f);
}

void AnimationPlayerSimple::EndPlay()
{
    m_AnimLayers[0].Context.Reset();
    m_AnimLayers[1].Context.Reset();
}

void AnimationPlayerSimple::Update()
{
    UpdatePose(GetWorld()->GetTick().FrameTimeStep);
}

void AnimationPlayerSimple::UpdatePose(float timeStep)
{
    UpdateContext context;

    auto& resourceMngr = GameApplication::sGetResourceManager();
    MeshResource* mesh = resourceMngr.TryGet(m_Mesh);
    if (!mesh)
        return;

    context.Skeleton = mesh->GetSkeleton();
    if (!context.Skeleton)
        return;

    context.SoaJointCount = context.Skeleton->num_soa_joints();
    context.TimeStep = timeStep;

    if (m_FadeIn > 0.0f)
    {
        m_LayerBlendWeight += m_Speed * timeStep / m_FadeIn;
        if (m_LayerBlendWeight > 1.0f)
        {
            m_LayerBlendWeight = 0.0f;
            m_FadeIn = 0.0f;

            m_CurrentLayer = (m_CurrentLayer + 1) & 1;
        }
    }

    AllocatePoseTransforms(context);

    if (m_FadeIn > 0.0f)
    {
        auto& layer1 = m_AnimLayers[m_CurrentLayer];
        auto& layer2 = m_AnimLayers[(m_CurrentLayer + 1) & 1];

        // TODO: Use temp memory?
        SoaTransform* localMatrices1 = (SoaTransform*)GameApplication::sGetFrameLoop().AllocFrameMem(context.SoaJointCount * sizeof(SoaTransform) * 2, alignof(SoaTransform));
        SoaTransform* localMatrices2 = localMatrices1 + context.SoaJointCount;

        UpdatePlayback(context, layer1, localMatrices1);
        UpdatePlayback(context, layer2, localMatrices2);

        BlendLayers(context, localMatrices1, localMatrices2, m_LayerBlendWeight, m_Pose->m_LocalMatrices.ToPtr());
    }
    else
    {
        UpdatePlayback(context, m_AnimLayers[m_CurrentLayer], m_Pose->m_LocalMatrices.ToPtr());
    }

    UpdateModelMatrices(context);
}

void AnimationPlayerSimple::AllocatePoseTransforms(UpdateContext const& context)
{
    if (m_Pose->m_LocalMatrices.Size() != context.SoaJointCount)
        m_Pose->m_LocalMatrices.Resize(context.SoaJointCount);
}

void AnimationPlayerSimple::UpdatePlayback(UpdateContext const& context, AnimationLayer& layer, SoaTransform* outLocalTransforms)
{
    if (layer.Duration > 0.0f)
    {
        layer.Ratio += m_Speed * context.TimeStep / layer.Duration;

        if (layer.LoopOffset >= 0.0f)
        {
            if (layer.Ratio > 1.0f)
                layer.Ratio = layer.LoopOffset / layer.Duration;

            if (layer.Ratio < 0.0f)
                layer.Ratio = 1.0f;
        }
        else
        {
            layer.Ratio = Math::Clamp(layer.Ratio, 0.0f, 1.0f);
        }
    }
    else
    {
        layer.Ratio = 0.0f;
    }

    SampleLayer(context, layer, outLocalTransforms);
}

void AnimationPlayerSimple::SampleLayer(UpdateContext const& context, AnimationLayer& layer, SoaTransform* outLocalTransforms)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    if (AnimationResource* animation = resourceMngr.TryGet(layer.Handle))
    {
        if (static_cast<uint32_t>(layer.Context->max_soa_tracks()) != context.SoaJointCount)
            layer.Context->Resize(context.Skeleton->num_joints());

        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = animation->GetImpl();
        samplingJob.context = layer.Context.RawPtr();
        samplingJob.ratio = layer.Ratio;
        samplingJob.output = ozz::span{outLocalTransforms, context.SoaJointCount};
        samplingJob.Run();
    }
    else
    {
        Core::Memcpy(outLocalTransforms, context.Skeleton->joint_rest_poses().data(), context.SoaJointCount * sizeof(outLocalTransforms[0]));
    }
}

void AnimationPlayerSimple::BlendLayers(UpdateContext const& context, SoaTransform const* inLocalTransforms1, SoaTransform const* inLocalTransforms2, float blendWeight, SoaTransform* outLocalTransforms)
{
    ozz::animation::BlendingJob blendingJob;
    ozz::animation::BlendingJob::Layer layers[2];

    layers[0].weight = 1.0f - blendWeight;
    layers[0].transform = ozz::span{inLocalTransforms1, context.SoaJointCount};

    layers[1].weight = blendWeight;
    layers[1].transform = ozz::span{inLocalTransforms2, context.SoaJointCount};

    blendingJob.layers = layers;
    blendingJob.output = ozz::span{outLocalTransforms, context.SoaJointCount};
    blendingJob.rest_pose = context.Skeleton->joint_rest_poses();

    blendingJob.Run();
}

void AnimationPlayerSimple::UpdateModelMatrices(UpdateContext const& context)
{
    auto jointCount = context.Skeleton->num_joints();
    if (m_Pose->m_ModelMatrices.Size() != jointCount)
        m_Pose->m_ModelMatrices.Resize(jointCount, SimdFloat4x4::identity());

    ozz::animation::LocalToModelJob localToModel;
    localToModel.skeleton = context.Skeleton;
    localToModel.input = ozz::span{m_Pose->m_LocalMatrices.ToPtr(), m_Pose->m_LocalMatrices.Size()};
    localToModel.output = ozz::span{m_Pose->m_ModelMatrices.ToPtr(), m_Pose->m_ModelMatrices.Size()};
    localToModel.Run();
}

HK_NAMESPACE_END
