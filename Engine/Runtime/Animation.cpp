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
    Channels.Clear();
    Transforms.Clear();
    Bounds.Clear();
    MinNodeIndex = 0;
    MaxNodeIndex = 0;
    ChannelsMap.Clear();
    FrameCount         = 0;
    FrameDelta         = 0;
    FrameRate          = 0;
    DurationInSeconds  = 0;
    DurationNormalizer = 1.0f;
}

void SkeletalAnimation::Initialize(int _FrameCount, float _FrameDelta, Transform const* _Transforms, int _TransformsCount, AnimationChannel const* _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const* _Bounds)
{
    HK_ASSERT(_TransformsCount == _FrameCount * _NumAnimatedJoints);

    Channels.ResizeInvalidate(_NumAnimatedJoints);
    Platform::Memcpy(Channels.ToPtr(), _AnimatedJoints, sizeof(Channels[0]) * _NumAnimatedJoints);

    Transforms.ResizeInvalidate(_TransformsCount);
    Platform::Memcpy(Transforms.ToPtr(), _Transforms, sizeof(Transforms[0]) * _TransformsCount);

    Bounds.ResizeInvalidate(_FrameCount);
    Platform::Memcpy(Bounds.ToPtr(), _Bounds, sizeof(Bounds[0]) * _FrameCount);

    if (!Channels.IsEmpty())
    {
        MinNodeIndex = Math::MaxValue<int32_t>();
        MaxNodeIndex = 0;

        for (int i = 0; i < Channels.Size(); i++)
        {
            MinNodeIndex = Math::Min(MinNodeIndex, Channels[i].JointIndex);
            MaxNodeIndex = Math::Max(MaxNodeIndex, Channels[i].JointIndex);
        }

        int mapSize = MaxNodeIndex - MinNodeIndex + 1;

        ChannelsMap.ResizeInvalidate(mapSize);

        for (int i = 0; i < mapSize; i++)
        {
            ChannelsMap[i] = (unsigned short)-1;
        }

        for (int i = 0; i < Channels.Size(); i++)
        {
            ChannelsMap[Channels[i].JointIndex - MinNodeIndex] = i;
        }
    }
    else
    {
        MinNodeIndex = 0;
        MaxNodeIndex = 0;

        ChannelsMap.Clear();
    }

    FrameCount         = _FrameCount;
    FrameDelta         = _FrameDelta;
    FrameRate          = 1.0f / _FrameDelta;
    DurationInSeconds  = (FrameCount - 1) * FrameDelta;
    DurationNormalizer = 1.0f / DurationInSeconds;

    bIsAnimationValid = _FrameCount > 0 && !Channels.IsEmpty();
}

void SkeletalAnimation::LoadInternalResource(StringView _Path)
{
    Purge();
}

bool SkeletalAnimation::LoadResource(IBinaryStreamReadInterface& Stream)
{
    TPodVector<AnimationChannel> channels;
    TPodVector<Transform>        transforms;
    TPodVector<BvAxisAlignedBox> bounds;

    uint32_t fileFormat = Stream.ReadUInt32();

    if (fileFormat != ASSET_ANIMATION)
    {
        LOG("Expected file format {}\n", ASSET_ANIMATION);
        return false;
    }

    uint32_t fileVersion = Stream.ReadUInt32();

    if (fileVersion != ASSET_VERSION_ANIMATION)
    {
        LOG("Expected file version {}\n", ASSET_VERSION_ANIMATION);
        return false;
    }

    String guid = Stream.ReadString();

    float    frameDelta = Stream.ReadFloat();
    uint32_t frameCount = Stream.ReadUInt32();
    Stream.ReadArray(channels);
    Stream.ReadArray(transforms);
    Stream.ReadArray(bounds);

    Initialize(frameCount, frameDelta,
               transforms.ToPtr(), transforms.Size(),
               channels.ToPtr(), channels.Size(), bounds.ToPtr());

    return true;
}

HK_NAMESPACE_END
