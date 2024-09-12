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

#include <Hork/Runtime/World/Component.h>
#include <Hork/Runtime/World/Modules/Skeleton/SkeletonPose.h>
#include <Hork/Runtime/Resources/Resource_Mesh.h>
#include <Hork/Runtime/Resources/Resource_Animation.h>

HK_NAMESPACE_BEGIN

class AnimationPlayerSimple : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

                            AnimationPlayerSimple();
                            AnimationPlayerSimple(AnimationPlayerSimple&& rhs);
                            ~AnimationPlayerSimple();

    /// Play the specified animation. The animation should already be loaded.
    void                    PlayAnimation(AnimationHandle handle, float fadeIn, float loopOffset = -1);

    /// The mesh is only used to provide the skeleton.
    void                    SetMesh(MeshHandle handle);

    void                    SetPlaybackSpeed(float speed);
    float                   GetPlaybackSpeed() const;

    void                    Seek(float time);

    float                   GetPlaybackTime() const;
    float                   GetRatio() const;

    float                   GetDuration() const;

    bool                    IsEnded() const;

    SkeletonPose*           GetPose();

    // Internal

    void                    BeginPlay();
    void                    EndPlay();
    void                    Update();

private:
    struct                  UpdateContext;
    struct                  SamplingContext;

    struct AnimationLayer
    {
        AnimationHandle     Handle;
        UniqueRef<SamplingContext> Context;
        float               Duration = 0;
        float               LoopOffset = -1;
        float               Ratio = 0;
    };

    void                    UpdatePose(float timeStep);
    void                    AllocatePoseTransforms(UpdateContext const& context);
    void                    UpdatePlayback(UpdateContext const& context, AnimationLayer& layer, SoaTransform* outLocalTransforms);
    void                    SampleLayer(UpdateContext const& context, AnimationLayer& layer, SoaTransform* outLocalTransforms);
    void                    BlendLayers(UpdateContext const& context, SoaTransform const* inLocalTransforms1, SoaTransform const* inLocalTransforms2, float blendWeight, SoaTransform* outLocalTransforms);
    void                    UpdateModelMatrices(UpdateContext const& context);

    Ref<SkeletonPose>       m_Pose;
    MeshHandle              m_Mesh;
    Array<AnimationLayer, 2>m_AnimLayers;
    int                     m_CurrentLayer = 0;
    float                   m_Speed = 1;
    float                   m_FadeIn = 0;
    float                   m_LayerBlendWeight = 0;
};

HK_NAMESPACE_END
