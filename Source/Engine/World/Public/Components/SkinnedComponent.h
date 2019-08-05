/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
#include <Engine/Resource/Public/Skeleton.h>

class FAnimationController;

/*

FSkinnedComponent

Mesh component with skinning

*/
class FSkinnedComponent : public FMeshComponent, public IRenderProxyOwner {
    AN_COMPONENT( FSkinnedComponent, FMeshComponent )

    friend class FWorld;
    friend class FAnimationController;

public:
    // Set skeleton for the component
    void SetSkeleton( FSkeleton * _Skeleton );

    // Get skeleton
    FSkeleton * GetSkeleton() { return Skeleton; }

    // Add animation controller
    void AddAnimationController( FAnimationController * _Controller );

    // Remove animation controller
    void RemoveAnimationController( FAnimationController * _Controller );

    // Remove all animation controllers
    void RemoveAnimationControllers();

    // Get animation controllers
    TPodArray< FAnimationController * > const & GetAnimationControllers() const { return AnimControllers; }

    // Set position on all animation tracks
    void SetTimeBroadcast( float _Time );

    // Step time delta on all animation tracks
    void AddTimeDeltaBroadcast( float _TimeDelta );

    // Recompute bounding box. Don't use directly. Use GetBounds() instead, it will recompute bounding box automatically.
    void UpdateBounds();

    // Get transform of the joint
    Float3x4 const & GetJointTransform( int _JointIndex );

    // Iterate meshes in parent world
    FSkinnedComponent * GetNextSkinnedMesh() { return Next; }
    FSkinnedComponent * GetPrevSkinnedMesh() { return Prev; }

    // Render proxy for skeleton transformation state
    FRenderProxy_Skeleton * GetRenderProxy() { return RenderProxy; }

    void UpdateJointTransforms();

protected:
    FSkinnedComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void DrawDebug( FDebugDraw * _DebugDraw ) override;

    void OnLazyBoundsUpdate() override;

private:
    void UpdateControllersIfDirty();
    void UpdateControllers();

    void UpdateTransformsIfDirty();
    void UpdateTransforms();

    void UpdateAbsoluteTransformsIfDirty();

    void MergeJointAnimations();

    void ReallocateRenderProxy();

    Float3x4 * WriteJointTransforms( int _JointsCount, int _StartJointLocation );

    TRef< FSkeleton > Skeleton;

    TPodArray< FAnimationController * > AnimControllers;

    TPodArray< Float3x4 > AbsoluteTransforms;
    TPodArray< Float3x4 > RelativeTransforms;

    FRenderProxy_Skeleton * RenderProxy;

    FSkinnedComponent * Next;
    FSkinnedComponent * Prev;

    bool bUpdateBounds;
    bool bUpdateControllers;
    bool bUpdateRelativeTransforms;
    bool bWriteTransforms;

protected:
    bool bUpdateAbsoluteTransforms;
    bool bJointsSimulatedByPhysics;
};
