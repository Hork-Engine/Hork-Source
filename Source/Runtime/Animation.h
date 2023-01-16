/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "Resource.h"
#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/Transform.h>
#include <Geometry/Skinning.h>

HK_NAMESPACE_BEGIN

/**

SkeletalAnimation

Animation class

*/
class SkeletalAnimation : public Resource
{
    HK_CLASS(SkeletalAnimation, Resource)

public:
    SkeletalAnimation();
    ~SkeletalAnimation();

    static SkeletalAnimation* Create(int _FrameCount, float _FrameDelta, Transform const* _Transforms, int _TransformsCount, AnimationChannel const* _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const* _Bounds)
    {
        SkeletalAnimation* anim = NewObj<SkeletalAnimation>();
        anim->Initialize(_FrameCount, _FrameDelta, _Transforms, _TransformsCount, _AnimatedJoints, _NumAnimatedJoints, _Bounds);
        return anim;
    }

    void Purge();

    TPodVector<AnimationChannel> const& GetChannels() const { return Channels; }
    TPodVector<Transform> const&        GetTransforms() const { return Transforms; }

    unsigned short GetChannelIndex(int _JointIndex) const;

    int                                 GetFrameCount() const { return FrameCount; }
    float                               GetFrameDelta() const { return FrameDelta; }
    float                               GetFrameRate() const { return FrameRate; }
    float                               GetDurationInSeconds() const { return DurationInSeconds; }
    float                               GetDurationNormalizer() const { return DurationNormalizer; }
    TPodVector<BvAxisAlignedBox> const& GetBoundingBoxes() const { return Bounds; }
    bool                                IsValid() const { return bIsAnimationValid; }

protected:
    void Initialize(int _FrameCount, float _FrameDelta, Transform const* _Transforms, int _TransformsCount, AnimationChannel const* _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const* _Bounds);

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView _Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Animation/Default"; }

private:
    TPodVector<AnimationChannel> Channels;
    TPodVector<Transform>        Transforms;
    TPodVector<unsigned short>   ChannelsMap;
    TPodVector<BvAxisAlignedBox> Bounds;
    int                          MinNodeIndex       = 0;
    int                          MaxNodeIndex       = 0;
    int                          FrameCount         = 0;  // frames count
    float                        FrameDelta         = 0;  // fixed time delta between frames
    float                        FrameRate          = 60; // frames per second (animation speed) FrameRate = 1.0 / FrameDelta
    float                        DurationInSeconds  = 0;  // animation duration is FrameDelta * ( FrameCount - 1 )
    float                        DurationNormalizer = 1;  // to normalize track timeline (DurationNormalizer = 1.0 / DurationInSeconds)
    bool                         bIsAnimationValid  = false;
};

HK_FORCEINLINE unsigned short SkeletalAnimation::GetChannelIndex(int _JointIndex) const
{
    return (_JointIndex < MinNodeIndex || _JointIndex > MaxNodeIndex) ? (unsigned short)-1 : ChannelsMap[_JointIndex - MinNodeIndex];
}

HK_NAMESPACE_END
