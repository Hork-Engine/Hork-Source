/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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
#include <World/Public/Resource/Skeleton.h>

class AAnimationController;

/**

ASkinnedComponent

Mesh component with skinning

*/
class ASkinnedComponent : public AMeshComponent {
    AN_COMPONENT( ASkinnedComponent, AMeshComponent )

    friend class ARenderWorld;
    friend class AAnimationController;

public:
    /** Allow raycasting */
    void SetAllowRaycast( bool _AllowRaycast ) override {}

    /** Get skeleton. Never return null */
    ASkeleton * GetSkeleton() { return Skeleton; }

    /** Add animation controller */
    void AddAnimationController( AAnimationController * _Controller );

    /** Remove animation controller */
    void RemoveAnimationController( AAnimationController * _Controller );

    /** Remove all animation controllers */
    void RemoveAnimationControllers();

    /** Get animation controllers */
    TPodArray< AAnimationController * > const & GetAnimationControllers() const { return AnimControllers; }

    /** Set position on all animation tracks */
    void SetTimeBroadcast( float _Time );

    /** Step time delta on all animation tracks */
    void AddTimeDeltaBroadcast( float _TimeDelta );

    /** Recompute bounding box. */
    void UpdateBounds();

    /** Get transform of the joint */
    Float3x4 const & GetJointTransform( int _JointIndex );

    /** Iterate meshes in parent world */
    ASkinnedComponent * GetNextSkinnedMesh() { return Next; }
    ASkinnedComponent * GetPrevSkinnedMesh() { return Prev; }

    void GetSkeletonHandle( size_t & _SkeletonOffset, size_t & _SkeletonOffsetMB, size_t & _SkeletonSize );

protected:
    ASkinnedComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnMeshChanged() override;

    void DrawDebug( ADebugRenderer * InRenderer ) override;

    void OnPreRenderUpdate( SRenderFrontendDef const * _Def ) override;

    Float3x4 const & _GetJointTransform( int _JointIndex ) override
    {
        return GetJointTransform( _JointIndex );
    }

private:
    void UpdateControllersIfDirty();
    void UpdateControllers();

    void UpdateTransformsIfDirty();
    void UpdateTransforms();

    void UpdateAbsoluteTransformsIfDirty();

    void MergeJointAnimations();

    TRef< ASkeleton > Skeleton;

    TPodArray< AAnimationController * > AnimControllers;

    TPodArray< Float3x4 > AbsoluteTransforms;
    TPodArray< Float3x4 > RelativeTransforms;

    alignas( 16 ) Float3x4 JointsBufferData[ASkeleton::MAX_JOINTS];

    ASkinnedComponent * Next = nullptr;
    ASkinnedComponent * Prev = nullptr;

    // Memory offset/size for the skeleton animation snapshot
    size_t SkeletonOffset = 0;
    size_t SkeletonOffsetMB = 0;
    size_t SkeletonSize = 0;

    bool bUpdateBounds : 1;
    bool bUpdateControllers : 1;
    bool bUpdateRelativeTransforms : 1;
    //bool bWriteTransforms : 1;

protected:
    bool bUpdateAbsoluteTransforms : 1;
    bool bJointsSimulatedByPhysics : 1;
};
