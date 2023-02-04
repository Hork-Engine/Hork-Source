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

#include "Animation.h"
#include "Skeleton.h"
#include "IndexedMesh.h"

#include <Engine/Assets/Asset.h>
#include <Engine/Core/Platform/Logger.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(SkeletalAnimation)

SkeletalAnimation::SkeletalAnimation()
{
}

SkeletalAnimation::~SkeletalAnimation()
{
}

void SkeletalAnimation::Purge()
{
    m_Channels.Clear();
    m_Transforms.Clear();
    m_Bounds.Clear();
    m_MinNodeIndex = 0;
    m_MaxNodeIndex = 0;
    m_ChannelsMap.Clear();
    m_FrameCount = 0;
    m_FrameDelta = 0;
    m_FrameRate = 0;
    m_DurationInSeconds = 0;
    m_DurationNormalizer = 1.0f;
}

void SkeletalAnimation::Initialize(int frameCount, float frameDelta, Transform const* transforms, int transformsCount, AnimationChannel const* animatedJoints, int numAnimatedJoints, BvAxisAlignedBox const* bounds)
{
    HK_ASSERT(transformsCount == frameCount * numAnimatedJoints);

    m_Channels.ResizeInvalidate(numAnimatedJoints);
    Platform::Memcpy(m_Channels.ToPtr(), animatedJoints, sizeof(m_Channels[0]) * numAnimatedJoints);

    m_Transforms.ResizeInvalidate(transformsCount);
    Platform::Memcpy(m_Transforms.ToPtr(), transforms, sizeof(m_Transforms[0]) * transformsCount);

    m_Bounds.ResizeInvalidate(frameCount);
    Platform::Memcpy(m_Bounds.ToPtr(), bounds, sizeof(m_Bounds[0]) * frameCount);

    if (!m_Channels.IsEmpty())
    {
        m_MinNodeIndex = Math::MaxValue<int32_t>();
        m_MaxNodeIndex = 0;

        for (int i = 0; i < m_Channels.Size(); i++)
        {
            m_MinNodeIndex = Math::Min(m_MinNodeIndex, m_Channels[i].JointIndex);
            m_MaxNodeIndex = Math::Max(m_MaxNodeIndex, m_Channels[i].JointIndex);
        }

        int mapSize = m_MaxNodeIndex - m_MinNodeIndex + 1;

        m_ChannelsMap.ResizeInvalidate(mapSize);

        for (int i = 0; i < mapSize; i++)
        {
            m_ChannelsMap[i] = (unsigned short)-1;
        }

        for (int i = 0; i < m_Channels.Size(); i++)
        {
            m_ChannelsMap[m_Channels[i].JointIndex - m_MinNodeIndex] = i;
        }
    }
    else
    {
        m_MinNodeIndex = 0;
        m_MaxNodeIndex = 0;

        m_ChannelsMap.Clear();
    }

    m_FrameCount = frameCount;
    m_FrameDelta = frameDelta;
    m_FrameRate = 1.0f / frameDelta;
    m_DurationInSeconds = (m_FrameCount - 1) * m_FrameDelta;
    m_DurationNormalizer = 1.0f / m_DurationInSeconds;

    m_bIsAnimationValid = frameCount > 0 && !m_Channels.IsEmpty();
}

void SkeletalAnimation::LoadInternalResource(StringView _Path)
{
    Purge();
}

bool SkeletalAnimation::LoadResource(IBinaryStreamReadInterface& stream)
{
    TVector<AnimationChannel> channels;
    TVector<Transform>        transforms;
    TVector<BvAxisAlignedBox> bounds;

    uint32_t fileFormat = stream.ReadUInt32();

    if (fileFormat != ASSET_ANIMATION)
    {
        LOG("Expected file format {}\n", ASSET_ANIMATION);
        return false;
    }

    uint32_t fileVersion = stream.ReadUInt32();

    if (fileVersion != ASSET_VERSION_ANIMATION)
    {
        LOG("Expected file version {}\n", ASSET_VERSION_ANIMATION);
        return false;
    }

    String guid = stream.ReadString();

    float frameDelta = stream.ReadFloat();
    uint32_t frameCount = stream.ReadUInt32();
    stream.ReadArray(channels);
    stream.ReadArray(transforms);
    stream.ReadArray(bounds);

    Initialize(frameCount, frameDelta,
               transforms.ToPtr(), transforms.Size(),
               channels.ToPtr(), channels.Size(), bounds.ToPtr());

    return true;
}

HK_NAMESPACE_END
