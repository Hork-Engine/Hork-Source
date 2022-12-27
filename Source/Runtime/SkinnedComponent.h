/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "MeshComponent.h"
#include "Skeleton.h"
#include <Core/IntrusiveLinkedListMacro.h>

class AnimationController;

/**

SkinnedComponent

Mesh component with skinning

*/
class SkinnedComponent : public MeshComponent
{
    HK_COMPONENT(SkinnedComponent, MeshComponent)

    friend class AnimationController;

public:
    TLink<SkinnedComponent> Link;

    /** Allow raycasting */
    void SetAllowRaycast(bool bAllowRaycast) override {}

    /** Get skeleton. Never return null */
    Skeleton* GetSkeleton() { return m_Skeleton; }

    /** Add animation controller */
    void AddAnimationController(AnimationController* controller);

    /** Remove animation controller */
    void RemoveAnimationController(AnimationController* controller);

    /** Remove all animation controllers */
    void RemoveAnimationControllers();

    /** Get animation controllers */
    TPodVector<AnimationController*> const& GetAnimationControllers() const { return m_AnimControllers; }

    /** Set position on all animation tracks */
    void SetTimeBroadcast(float time);

    /** Step time delta on all animation tracks */
    void AddTimeDeltaBroadcast(float timeDelta);

    /** Recompute bounding box. */
    void UpdateBounds();

    /** Get transform of the joint */
    Float3x4 const& GetJointTransform(int jointIndex);

    void GetSkeletonHandle(size_t& skeletonOffset, size_t& skeletonOffsetMB, size_t& skeletonSize);

protected:
    SkinnedComponent();
    ~SkinnedComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void DrawDebug(DebugRenderer* InRenderer) override;

    void OnPreRenderUpdate(RenderFrontendDef const* def) override;

    Float3x4 const& _GetJointTransform(int jointIndex) override
    {
        return GetJointTransform(jointIndex);
    }

    void UpdateMesh() override;

private:
    void UpdateControllersIfDirty();
    void UpdateControllers();

    void UpdateTransformsIfDirty();
    void UpdateTransforms();

    void UpdateAbsoluteTransformsIfDirty();

    void MergeJointAnimations();

    TRef<Skeleton> m_Skeleton;

    TPodVector<AnimationController*> m_AnimControllers;

    TPodVector<Float3x4> m_AbsoluteTransforms;
    TPodVector<Float3x4> m_RelativeTransforms;

    alignas(16) Float3x4 m_JointsBufferData[MAX_SKELETON_JOINTS];

    // Memory offset/size for the skeleton animation snapshot
    size_t m_SkeletonOffset = 0;
    size_t m_SkeletonOffsetMB = 0;
    size_t m_SkeletonSize = 0;

    bool m_bUpdateBounds : 1;
    bool m_bUpdateControllers : 1;
    bool m_bUpdateRelativeTransforms : 1;
    //bool m_bWriteTransforms : 1;

protected:
    bool m_bUpdateAbsoluteTransforms : 1;
    bool m_bJointsSimulatedByPhysics : 1;
};
