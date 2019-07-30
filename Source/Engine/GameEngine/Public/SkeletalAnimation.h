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
#include <Engine/Runtime/Public/RenderBackend.h>
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class FSkeletonAnimation;

/*

FJoint

Joint properties

*/
struct FJoint {
    int      Parent;                 // Parent joint index. For root = -1
    Float3x4 OffsetMatrix;           // Transform vertex to joint-space
    Float3x4 LocalTransform;         // Joint local transform
    char     Name[64];               // Joint name
};

/*

FJointTransform

Joint transformation

*/
struct FJointTransform {
    Quat   Rotation;
    Float3 Position;
    Float3 Scale;

    // Helper to convert joint transform to matrix 3x4
    void ToMatrix( Float3x4 & _Matrix ) const {
        _Matrix.Compose( Position, Rotation.ToMatrix(), Scale );
    }
};

/*

FJointAnimation

Animation for single joint

*/
struct FJointAnimation {
    // Joint index in skeleton
    int JointIndex;

    // Joint frames
    int TransformOffset;
};

/*

FSocketDef

Socket for attaching to skeleton joint

*/
class FSocketDef : public FBaseObject {
    AN_CLASS( FSocketDef, FBaseObject )

public:
    int JointIndex;

protected:
    FSocketDef() {}
};

/*

FSkeleton

Skeleton structure

*/
class FSkeleton : public FBaseObject {
    AN_CLASS( FSkeleton, FBaseObject )

public:
    enum { MAX_JOINTS = 256 };

    void Initialize( FJoint * _Joints, int _JointsCount, BvAxisAlignedBox const & _BindposeBounds );

    // Initialize default object representation
    void InitializeDefaultObject() override;

    // Initialize object from file
    bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    void Purge();

    int FindJoint( const char * _Name ) const;

    TPodArray< FJoint > const & GetJoints() const { return Joints; }

    FSkeletonAnimation * CreateAnimation();

    FSocketDef * CreateSocket( const char * _Name, int _JointIndex );
    FSocketDef * CreateSocket( const char * _Name, const char * _JointName );

    FSocketDef * FindSocket( const char * _Name );

    TPodArray< FSkeletonAnimation * > const & GetAnimations() const { return Animations; }
    TPodArray< FSocketDef * > const & GetSockets() const { return Sockets; }

    BvAxisAlignedBox const & GetBindposeBounds() const { return BindposeBounds; }

protected:
    FSkeleton();
    ~FSkeleton();

private:
    TPodArray< FJoint > Joints;
    TPodArray< FSkeletonAnimation * > Animations;
    TPodArray< FSocketDef * > Sockets;
    BvAxisAlignedBox BindposeBounds;
};

/*

FSkeletonAnimation

Skeleton animation

*/
class FSkeletonAnimation : public FBaseObject {
    AN_CLASS( FSkeletonAnimation, FBaseObject )

    friend class FSkeleton;

public:
    void Initialize( int _FrameCount, float _FrameDelta, FJointTransform const * _Transforms, int _TransformsCount, FJointAnimation const * _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const * _Bounds );

    TPodArray< FJointAnimation > const & GetAnimatedJoints() const { return AnimatedJoints; }
    TPodArray< FJointTransform > const & GetTransforms() const { return Transforms; }

    unsigned short const * GetChannelsMap() const { return ChannelsMap.ToPtr(); }

    int GetFrameCount() const { return FrameCount; }
    float GetFrameDelta() const { return FrameDelta; }
    float GetFrameRate() const { return FrameRate; }
    float GetDurationInSeconds() const { return DurationInSeconds; }
    float GetDurationNormalizer() const { return DurationNormalizer; }
    TPodArray< BvAxisAlignedBox > const & GetBoundingBoxes() const { return Bounds; }

protected:
    FSkeletonAnimation();
    ~FSkeletonAnimation();

private:
    // Owner
    FSkeleton * Skeleton;

    // Animation itself
    TPodArray< FJointAnimation > AnimatedJoints;
    TPodArray< FJointTransform > Transforms;
    TPodArray< unsigned short > ChannelsMap;

    int     FrameCount;         // frames count
    float   FrameDelta;         // fixed time delta between frames
    float   FrameRate;          // frames per second (animation speed) FrameRate = 1.0 / FrameDelta
    float   DurationInSeconds;  // animation duration is FrameDelta * ( FrameCount - 1 )
    float   DurationNormalizer; // to normalize track timeline (DurationNormalizer = 1.0 / DurationInSeconds)

    TPodArray< BvAxisAlignedBox > Bounds;
};

/*

EAnimationPlayMode

Animation play mode

*/
enum EAnimationPlayMode {
    ANIMATION_PLAY_WRAP,
    ANIMATION_PLAY_MIRROR,
    ANIMATION_PLAY_CLAMP
};

/*

FAnimationController

Animation controller (track, state)

*/
struct FAnimationController {
    float TimeLine;
    EAnimationPlayMode PlayMode;
    float Quantizer;
    int Frame;
    int NextFrame;
    float Blend;
    float Weight;
    bool bEnabled;
};

/*

FSkinnedComponent

Mesh component with skinning

*/
class FSkinnedComponent : public FMeshComponent, public IRenderProxyOwner {
    AN_COMPONENT( FSkinnedComponent, FMeshComponent )

    friend class FRenderFrontend;
    friend class FWorld;

public:
    // Set skeleton for the component
    void SetSkeleton( FSkeleton * _Skeleton );

    // Get skeleton
    FSkeleton * GetSkeleton() { return Skeleton; }

    // Set position on animation track
    void SetControllerTimeline( int _Controller, float _Timeline, EAnimationPlayMode _PlayMode = ANIMATION_PLAY_WRAP, float _Quantizer = 0.0f );

    // Set position on all animation tracks
    void SetTimelineBroadcast( float _Timeline, EAnimationPlayMode _PlayMode = ANIMATION_PLAY_WRAP, float _Quantizer = 0.0f );

    // Step time delta on animation track
    void AddTimeDelta( int _Controller, float _TimeDelta );

    // Step time delta on all animation tracks
    void AddTimeDeltaBroadcast( float _TimeDelta );

    // Set weight for animation blending
    void SetControllerWeight( int _Controller, float _Weight );

    // Set controller enabled/disabled
    void SetControllerEnabled( int _Controller, bool _Enabled );

    // Get animation tracks
    //FAnimationController * GetControllers() { return AnimControllers.ToPtr(); }

    // Get total animation tracks
    int GetControllersCount() const { return AnimControllers.Size(); }

    // Recompute bounding box. Don't use directly. Use GetBounds() instead, it will recompute bounding box automatically.
    void UpdateBounds();

    // Get transform of the joint
    Float3x4 const & GetJointTransform( int _JointIndex );

    // Iterate meshes in parent world
    FSkinnedComponent * GetNextSkinnedMesh() { return Next; }
    FSkinnedComponent * GetPrevSkinnedMesh() { return Prev; }

    // Render proxy for skeleton transformation state
    FRenderProxy_Skeleton * GetRenderProxy() { return RenderProxy; }

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

    void UpdateJointTransforms();

    void ReallocateRenderProxy();

    Float3x4 * WriteJointTransforms( int _JointsCount, int _StartJointLocation );

    TRef< FSkeleton > Skeleton;

    TPodArray< FAnimationController > AnimControllers;

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
