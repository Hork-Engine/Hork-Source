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

#include "SkeletalAnimation.h"

#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

HK_FORCEINLINE float Quantize(float v, float quantizer)
{
    return quantizer > 0.0f ? Math::Floor(v * quantizer) / quantizer : v;
}

SkeletalAnimationTrack::SkeletalAnimationTrack(StringView animation) :
    m_Animation(animation)
{}

String const& SkeletalAnimationTrack::GetAnimation() const
{
    return m_Animation;
}

void SkeletalAnimationTrack::SetPlaybackMode(PLAYBACK_MODE mode)
{
    m_PlaybackMode = mode;
}

SkeletalAnimationTrack::PLAYBACK_MODE SkeletalAnimationTrack::GetPlaybackMode() const
{
    return m_PlaybackMode;
}

void SkeletalAnimationTrack::SetQuantizer(float quantizer)
{
    m_Quantizer = quantizer;
}

float SkeletalAnimationTrack::GetQuantizer() const
{
    return m_Quantizer;
}

float AnimationBlendMachine::GetAnimationDuration(StringView animation)
{
    SkeletonResource const* skeleton = GameApplication::GetResourceManager().TryGet(m_Skeleton);
    if (!skeleton)
        return 0;

    uint32_t animationID = skeleton->FindAnimation(animation);
    if (animationID == -1)
        return 0;

    return skeleton->GetAnimations()[animationID]->GetDurationInSeconds();
}

struct PlaybackFrame
{
    int FrameIndex{};
    int NextFrameIndex{};
    float FrameBlend{};
};

PlaybackFrame LocateFrame(SkeletalAnimation* animation, SkeletalAnimationTrack::PLAYBACK_MODE playbackMode, float quantizer, float position)
{
    float trackTimeLine;
    int keyFrame;
    float lerp;
    int take;

    PlaybackFrame frame;

    if (animation->GetFrameCount() > 1)
    {
        switch (playbackMode)
        {
            case SkeletalAnimationTrack::PLAYBACK_CLAMP:

                // clamp and normalize 0..1
                if (position <= 0.0f)
                {
                    frame.FrameBlend = 0;
                    frame.FrameIndex = 0;
                    frame.NextFrameIndex = 0;
                }
                else if (position >= animation->GetDurationInSeconds())
                {
                    frame.FrameBlend = 0;
                    frame.FrameIndex = frame.NextFrameIndex = animation->GetFrameCount() - 1;
                }
                else
                {
                    // normalize 0..1
                    trackTimeLine = position * animation->GetDurationNormalizer();

                    // adjust 0...framecount-1
                    trackTimeLine = trackTimeLine * (float)(animation->GetFrameCount() - 1);

                    keyFrame = Math::Floor(trackTimeLine);
                    lerp = Math::Fract(trackTimeLine);

                    frame.FrameIndex = keyFrame;
                    frame.NextFrameIndex = keyFrame + 1;
                    frame.FrameBlend = Quantize(lerp, quantizer);
                }
                break;

            case SkeletalAnimationTrack::PLAYBACK_WRAP:

                // normalize 0..1
#if 1
                trackTimeLine = position * animation->GetDurationNormalizer();
                trackTimeLine = Math::Fract(trackTimeLine);
#else
                trackTimeLine = fmod(TimeLine, animation->GetDurationInSeconds()) * animation->GetDurationNormalize();
                if (trackTimeLine < 0.0f)
                {
                    trackTimeLine += 1.0f;
                }
#endif

                // adjust 0...framecount-1
                trackTimeLine = trackTimeLine * (float)(animation->GetFrameCount() - 1);

                keyFrame = Math::Floor(trackTimeLine);
                lerp = Math::Fract(trackTimeLine);

                if (position < 0.0f)
                {
                    frame.FrameIndex = keyFrame + 1;
                    frame.NextFrameIndex = keyFrame;
                    frame.FrameBlend = Quantize(1.0f - lerp, quantizer);
                }
                else
                {
                    frame.FrameIndex = keyFrame;
                    frame.NextFrameIndex = frame.FrameIndex + 1;
                    frame.FrameBlend = Quantize(lerp, quantizer);
                }
                break;

            case SkeletalAnimationTrack::PLAYBACK_MIRROR:

                // normalize 0..1
                trackTimeLine = position * animation->GetDurationNormalizer();
                take = Math::Floor(Math::Abs(trackTimeLine));
                trackTimeLine = Math::Fract(trackTimeLine);

                // adjust 0...framecount-1
                trackTimeLine = trackTimeLine * (float)(animation->GetFrameCount() - 1);

                keyFrame = Math::Floor(trackTimeLine);
                lerp = Math::Fract(trackTimeLine);

                if (position < 0.0f)
                {
                    frame.FrameIndex = keyFrame + 1;
                    frame.NextFrameIndex = keyFrame;
                    frame.FrameBlend = Quantize(1.0f - lerp, quantizer);
                }
                else
                {
                    frame.FrameIndex = keyFrame;
                    frame.NextFrameIndex = frame.FrameIndex + 1;
                    frame.FrameBlend = Quantize(lerp, quantizer);
                }

                if (take & 1)
                {
                    frame.FrameIndex = animation->GetFrameCount() - frame.FrameIndex - 1;
                    frame.NextFrameIndex = animation->GetFrameCount() - frame.NextFrameIndex - 1;
                }
                break;
        }
    }
    else if (animation->GetFrameCount() == 1)
    {
        frame.FrameBlend = 0;
        frame.FrameIndex = 0;
        frame.NextFrameIndex = 0;
    }

    return frame;
}

void SetupPose(SkeletonPose* pose, SkeletonResource const* skeleton)
{
    auto jointsCount = skeleton->GetJointsCount();

    //pose->SummaryWeights.Resize(jointsCount);
    Core::ZeroMem(pose->SummaryWeights, sizeof(float) * jointsCount);

    pose->m_RelativeTransforms.Resize(jointsCount);
    pose->m_RelativeTransforms.ZeroMem();

    pose->m_AbsoluteTransforms.Resize(jointsCount + 1); // + 1 for root's parent
    pose->m_AbsoluteTransforms[0].SetIdentity();

    pose->m_Bounds.Clear();
}

void CalculateJointTransforms(SkeletonPose* pose, SkeletalAnimation const* animation, PlaybackFrame const& frame, float weight)
{
    Float3x4 jointMatrix;
    Transform jointTransform;

    for (AnimationChannel const& channel : animation->GetChannels())
    {
        int jointIndex = channel.JointIndex;

        Vector<Transform> const& transforms = animation->GetTransforms();

        if (frame.FrameIndex == frame.NextFrameIndex || frame.FrameBlend < 0.0001f)
        {
            jointTransform = transforms[channel.TransformOffset + frame.FrameIndex];
        }
        else
        {
            Transform const& frame1 = transforms[channel.TransformOffset + frame.FrameIndex];
            Transform const& frame2 = transforms[channel.TransformOffset + frame.NextFrameIndex];

            jointTransform.Position = Math::Lerp(frame1.Position, frame2.Position, frame.FrameBlend);
            jointTransform.Rotation = Math::Slerp(frame1.Rotation, frame2.Rotation, frame.FrameBlend);
            jointTransform.Scale = Math::Lerp(frame1.Scale, frame2.Scale, frame.FrameBlend);
        }

        jointTransform.ComputeTransformMatrix(jointMatrix);

        pose->m_RelativeTransforms[jointIndex][0] += jointMatrix[0] * weight;
        pose->m_RelativeTransforms[jointIndex][1] += jointMatrix[1] * weight;
        pose->m_RelativeTransforms[jointIndex][2] += jointMatrix[2] * weight;

        pose->SummaryWeights[jointIndex] += weight;
    }
}

void FinalizeJointTransforms(SkeletonPose* pose, SkeletonResource const* skeleton)
{
    SkeletonJoint const* joints = skeleton->GetJoints().ToPtr();
    uint32_t jointsCount = skeleton->GetJointsCount();

    for (uint32_t jointIndex = 0; jointIndex < jointsCount; jointIndex++)
    {
        if (pose->SummaryWeights[jointIndex] > 0.0f)
        {
            const float sumWeightReciprocal = 1.0f / pose->SummaryWeights[jointIndex];

            pose->m_RelativeTransforms[jointIndex][0] *= sumWeightReciprocal;
            pose->m_RelativeTransforms[jointIndex][1] *= sumWeightReciprocal;
            pose->m_RelativeTransforms[jointIndex][2] *= sumWeightReciprocal;
        }
        else
        {
            pose->m_RelativeTransforms[jointIndex] = joints[jointIndex].LocalTransform;
        }

        //pose->m_AbsoluteTransforms[jointIndex + 1] = pose->m_AbsoluteTransforms[joints[jointIndex].Parent + 1] * pose->m_RelativeTransforms[jointIndex];
    }
}

void AnimationBlendMachine::Layer::SampleAnimationTrack(SkeletonResource const* skeleton, SkeletalAnimationTrack const* track, float weight, float position, SkeletonPose* pose) const
{
    if (!skeleton)
        return;    

    uint32_t animationID = skeleton->FindAnimation(track->GetAnimation());
    if (animationID == -1)
        return;

    SkeletalAnimation* animation = skeleton->GetAnimations()[animationID];
    HK_ASSERT(animation);

    if (!animation->IsValid())
        return;

    auto frame = LocateFrame(animation, track->GetPlaybackMode(), track->GetQuantizer(), position);

    CalculateJointTransforms(pose, animation, frame, weight);
    pose->m_Bounds.AddAABB(animation->GetBoundingBoxes()[frame.FrameIndex]);
}

void AnimationInstance::Update(float timeStep, SkeletonPose* pose)
{
    auto skeletonHandle = m_BlendMachine->GetSkeleton();

    HK_ASSERT(pose->Skeleton == skeletonHandle);

    SkeletonResource const* skeleton = GameApplication::GetResourceManager().TryGet(skeletonHandle);

    if (skeleton)
        SetupPose(pose, skeleton);

    for (auto& layer : m_Layers)
    {
        float weight = 1.0f;
        layer.Update(timeStep, weight, skeleton, pose);
    }

    if (skeleton)
    {
        FinalizeJointTransforms(pose, skeleton);

        if (pose->m_Bounds.IsEmpty())
            pose->m_Bounds = skeleton->GetBindposeBounds();
    }
}

//void CreateBlendMachine()
//{
//    AnimationTrack* idleAnim;
//    AnimationTrack* walkAnim;
//    AnimationTrack* aimAnim;
//    AnimationTrack* runAnim;
//
//    AnimationBlendMachine machine;
//
//    AnimationBlendMachine::Layer* layer = machine.CreateLayer();
//
//    AnimationBlendMachine::NodeHandle aim = layer->AddNode(aimAnim);
//    AnimationBlendMachine::NodeHandle walk = layer->AddNode(walkAnim);
//    AnimationBlendMachine::NodeHandle blendAimWalk = layer->AddBlendNode({{aim, 0.75f}, {walk, 0.25f}});
//
//    AnimationBlendMachine::NodeHandle run = layer->AddNode(runAnim);
//
//    AnimationBlendMachine::NodeHandle idle = layer->AddNode(idleAnim);
//
//    AnimationBlendMachine::StateHandle walkState = layer->AddState("Walk", blendAimWalk);
//    AnimationBlendMachine::StateHandle idleState = layer->AddState("Idle", idle);
//    AnimationBlendMachine::StateHandle runState = layer->AddState("Run", run);
//
//    layer->AddTransition("Walk->Idle", walkState, idleState, 1.0f);
//    layer->AddTransition("Idle->Walk", idleState, walkState, 1.0f);
//    layer->AddTransition("Walk->Run", walkState, runState, 1.0f, true);
//    layer->AddTransition("Run->Walk", runState, walkState, 1.0f, true);
//    layer->AddTransition("Run->Idle", runState, idleState, 1.0f);
//
//    layer->SetState("Walk");
//    machine.Update(1.0f / 60);
//    layer->ChangeState("Idle");
//}

HK_NAMESPACE_END
