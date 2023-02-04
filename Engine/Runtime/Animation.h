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
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>
#include <Engine/Geometry/Transform.h>
#include <Engine/Geometry/Skinning.h>

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

    static SkeletalAnimation* Create(int frameCount, float frameDelta, Transform const* transforms, int transformsCount, AnimationChannel const* animatedJoints, int numAnimatedJoints, BvAxisAlignedBox const* bounds)
    {
        SkeletalAnimation* anim = NewObj<SkeletalAnimation>();
        anim->Initialize(frameCount, frameDelta, transforms, transformsCount, animatedJoints, numAnimatedJoints, bounds);
        return anim;
    }

    void Purge();

    TVector<AnimationChannel> const& GetChannels() const { return m_Channels; }
    TVector<Transform> const& GetTransforms() const { return m_Transforms; }

    unsigned short GetChannelIndex(int jointIndex) const;

    int GetFrameCount() const { return m_FrameCount; }
    float GetFrameDelta() const { return m_FrameDelta; }
    float GetFrameRate() const { return m_FrameRate; }
    float GetDurationInSeconds() const { return m_DurationInSeconds; }
    float GetDurationNormalizer() const { return m_DurationNormalizer; }
    TVector<BvAxisAlignedBox> const& GetBoundingBoxes() const { return m_Bounds; }
    bool IsValid() const { return m_bIsAnimationValid; }

protected:
    void Initialize(int frameCount, float frameDelta, Transform const* transforms, int transformsCount, AnimationChannel const* animatedJoints, int numAnimatedJoints, BvAxisAlignedBox const* bounds);

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Animation/Default"; }

private:
    TVector<AnimationChannel> m_Channels;
    TVector<Transform> m_Transforms;
    TVector<unsigned short> m_ChannelsMap;
    TVector<BvAxisAlignedBox> m_Bounds;
    int m_MinNodeIndex = 0;
    int m_MaxNodeIndex = 0;
    int m_FrameCount = 0;                                 // frames count
    float m_FrameDelta = 0;                               // fixed time delta between frames
    float m_FrameRate = 60;                               // frames per second (animation speed) FrameRate = 1.0 / FrameDelta
    float m_DurationInSeconds = 0;                        // animation duration is FrameDelta * ( FrameCount - 1 )
    float m_DurationNormalizer = 1;                       // to normalize track timeline (DurationNormalizer = 1.0 / DurationInSeconds)
    bool m_bIsAnimationValid = false;
};

HK_FORCEINLINE unsigned short SkeletalAnimation::GetChannelIndex(int jointIndex) const
{
    return (jointIndex < m_MinNodeIndex || jointIndex > m_MaxNodeIndex) ? (unsigned short)-1 : m_ChannelsMap[jointIndex - m_MinNodeIndex];
}

HK_NAMESPACE_END
