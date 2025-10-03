/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "Resource_Animation.h"

#include "Implementation/OzzIO.h"

#include <Hork/Geometry/RawMesh.h>

#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_animation_utils.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>

HK_NAMESPACE_BEGIN

float AnimationResource::GetDuration() const
{
    return m_OzzAnimation ? m_OzzAnimation->duration() : 0.0f;
}

AnimationResource::~AnimationResource()
{}

UniqueRef<AnimationResource> AnimationResource::sLoad(IBinaryStreamReadInterface& stream)
{
    StringView extension = PathUtils::sGetExt(stream.GetName());

    if (!extension.Icmp(".gltf") || !extension.Icmp(".glb") || !extension.Icmp(".fbx"))
    {
        RawMesh mesh;
        RawMeshLoadFlags flags = RawMeshLoadFlags::Skeleton | RawMeshLoadFlags::SingleAnimation;

        bool result;
        if (!extension.Icmp(".fbx"))
            result = mesh.LoadFBX(stream, flags);
        else
            result = mesh.LoadGLTF(stream, flags);

        if (!result || mesh.Animations.IsEmpty())
            return {};

        return AnimationResourceBuilder().Build(*mesh.Animations[0].RawPtr(), mesh.Skeleton);
    }

    UniqueRef<AnimationResource> resource = MakeUnique<AnimationResource>();
    if (!resource->Read(stream))
        return {};
    return resource;
}

bool AnimationResource::Read(IBinaryStreamReadInterface& stream)
{
    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return false;
    }

    m_OzzAnimation = OzzReadAnimation(stream);
    return m_OzzAnimation.RawPtr() != nullptr;
}

void AnimationResource::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt32(MakeResourceMagic(Type, Version));
    OzzWriteAnimation(stream, m_OzzAnimation.RawPtr());
}

namespace
{

    template <typename ValueType>
    ozz::span<const ValueType> DataView(Vector<float> const& data)
    {
        return ozz::span{reinterpret_cast<ValueType const*>(data.ToPtr()), data.Size() * sizeof(float) / sizeof(ValueType)};
    }

    template <typename KeyFrame>
    bool SampleLinearChannel(Vector<float> const& timestamps, Vector<float> const& data, KeyFrame* keyframes)
    {
        using ValueType = typename KeyFrame::value_type::Value;

        const size_t keyCount = timestamps.Size();
        if (data.Size() * sizeof(float) != keyCount * sizeof(ValueType))
        {
            LOG("Inconsistent number of keys\n");
            return false;
        }

        ozz::span<const ValueType> values = DataView<ValueType>(data);

        keyframes->reserve(keyCount);
        for (size_t i = 0; i < keyCount; ++i)
        {
            keyframes->emplace_back();
            auto& keyframe = keyframes->back();
            keyframe.time = timestamps[i];
            keyframe.value = values[i];
        }

        return true;
    }

    // NOTE: There are twice-1 as many ozz keyframes as gltf keyframes
    template <typename KeyFrame>
    bool SampleStepChannel(Vector<float> const& timestamps, Vector<float> const& data, KeyFrame* keyframes)
    {
        using ValueType = typename KeyFrame::value_type::Value;

        const size_t keyCount = timestamps.Size();
        if (data.Size() * sizeof(float) != keyCount * sizeof(ValueType))
        {
            LOG("Inconsistent number of keys\n");
            return false;
        }

        ozz::span<const ValueType> values = DataView<ValueType>(data);

        // A step is created with 2 consecutive keys. Last step is a single key.
        keyframes->resize(keyCount * 2 - 1);

        for (size_t i = 0; i < keyCount; ++i)
        {
            auto& keyframe = keyframes->at(i * 2);
            keyframe.time = timestamps[i];
            keyframe.value = values[i];

            if (i < keyCount - 1)
            {
                auto& next_key = keyframes->at(i * 2 + 1);
                next_key.time = nexttowardf(timestamps[i + 1], 0.f);
                next_key.value = values[i];
            }
        }

        return true;
    }

    // NOTE: The number of keyframes is determined from the given sample rate
    template <typename KeyFrame>
    bool SampleCubicSplineChannel(Vector<float> const& timestamps, Vector<float> const& data, float sampleRate, KeyFrame* keyframes)
    {
        using ValueType = typename KeyFrame::value_type::Value;

        assert(data.Size() % 3 == 0);
        size_t keyCount = timestamps.Size();

        if (data.Size() * sizeof(float) != keyCount * 3 * sizeof(ValueType))
        {
            LOG("Inconsistent number of keys\n");
            return false;
        }

        const ozz::span<const ValueType> values = DataView<ValueType>(data);

        // Iterate keyframes at sampleRate steps, between first and last time stamps
        ozz::animation::offline::FixedRateSamplingTime fixed_it(timestamps[keyCount - 1] - timestamps[0], sampleRate);
        keyframes->resize(fixed_it.num_keys());
        size_t cubic_key0 = 0;
        for (size_t k = 0; k < fixed_it.num_keys(); ++k)
        {
            const float time = fixed_it.time(k) + timestamps[0];

            auto& keyframe = keyframes->at(k);

            keyframe.time = time;

            // Makes sure time is in between the correct cubic keyframes.
            while (timestamps[cubic_key0 + 1] < time)
                cubic_key0++;

            assert(timestamps[cubic_key0] <= time && time <= timestamps[cubic_key0 + 1]);

            // Interpolate cubic key
            const float t0 = timestamps[cubic_key0];      // keyframe before time
            const float t1 = timestamps[cubic_key0 + 1];  // keyframe after time
            const float alpha = (time - t0) / (t1 - t0);
            const ValueType& p0 = values[cubic_key0 * 3 + 1];
            const ValueType  m0 = values[cubic_key0 * 3 + 2] * (t1 - t0);
            const ValueType& p1 = values[(cubic_key0 + 1) * 3 + 1];
            const ValueType  m1 = values[(cubic_key0 + 1) * 3] * (t1 - t0);
            keyframe.value = Math::HermiteCubicSpline(p0, m0, p1, m1, alpha);
        }

        return true;
    }

    template <typename KeyFrame>
    bool SampleChannel(RawAnimation::Channel::InterpolationType interpolation, Vector<float> const& timestamps, Vector<float> const& data, float sampleRate, KeyFrame* keyframes)
    {
        bool result = false;
        switch (interpolation)
        {
        case RawAnimation::Channel::InterpolationType::Linear:
            result = SampleLinearChannel(timestamps, data, keyframes);
            break;
        case RawAnimation::Channel::InterpolationType::Step:
            result = SampleStepChannel(timestamps, data, keyframes);
            break;
        case RawAnimation::Channel::InterpolationType::CubicSpline:
            result = SampleCubicSplineChannel(timestamps, data, sampleRate, keyframes);
            break;
        default:
            HK_ASSERT(0);
            break;
        }

        if (result)
        {
            result = std::is_sorted(keyframes->begin(), keyframes->end(),        
                [](auto& a, auto& b)
                {
                    return a.time < b.time;
                });
            if (!result)
                LOG("Keyframes are not sorted in increasing order\n");
        }

        // Remove keyframes with strictly equal times, keeping the first one.
        if (result)
        {
            auto new_end = std::unique(keyframes->begin(), keyframes->end(),
                [](auto& a, auto& b)
                {
                    return a.time == b.time;
                });
            if (new_end != keyframes->end())
            {
                keyframes->erase(new_end, keyframes->end());
                LOG("Keyframe times are not unique. Imported data were modified to remove keyframes at consecutive equivalent times\n");
            }
        }
        return result;
    }

    bool SampleAnimationChannel(RawAnimation::Channel const* rawChannel, float sampleRate, ozz::animation::offline::RawAnimation::JointTrack& track, float& duration)
    {
        if (rawChannel->Timestamps.IsEmpty())
            return true;

        float maxTime = rawChannel->Timestamps.Last();

        if (duration < maxTime)
            duration = maxTime;    

        bool result = false;
        switch (rawChannel->Type)
        {
        case RawAnimation::Channel::ChannelType::Translation:
            result = SampleChannel(rawChannel->Interpolation, rawChannel->Timestamps, rawChannel->Data, sampleRate, &track.translations);
            break;
        case RawAnimation::Channel::ChannelType::Rotation:
            result = SampleChannel(rawChannel->Interpolation, rawChannel->Timestamps, rawChannel->Data, sampleRate, &track.rotations);
            if (result)
            {
                for (auto& key : track.rotations)
                    key.value = ozz::math::Normalize(key.value);
            }
            break;
        case RawAnimation::Channel::ChannelType::Scale:
            result = SampleChannel(rawChannel->Interpolation, rawChannel->Timestamps, rawChannel->Data, sampleRate, &track.scales);
            break;
        default:
            HK_ASSERT(0);
            break;
        }
        return result;
    }

    ozz::animation::offline::RawAnimation::TranslationKey CreateTranslationRestPoseKey(Float3 const& translation)
    {
        ozz::animation::offline::RawAnimation::TranslationKey key;
        key.time = 0.0f;
        key.value = ozz::math::Float3(translation[0], translation[1], translation[2]);
        return key;
    }

    ozz::animation::offline::RawAnimation::RotationKey CreateRotationRestPoseKey(Quat const& rotation)
    {
        ozz::animation::offline::RawAnimation::RotationKey key;
        key.time = 0.0f;
        key.value = ozz::math::Quaternion(rotation[0], rotation[1], rotation[2], rotation[3]);
        return key;
    }

    ozz::animation::offline::RawAnimation::ScaleKey CreateScaleRestPoseKey(Float3 const& scale)
    {
        ozz::animation::offline::RawAnimation::ScaleKey key;
        key.time = 0.0f;  
        key.value = ozz::math::Float3(scale[0], scale[1], scale[2]);
        return key;
    }

    UniqueRef<OzzAnimation> ConvertAnimationToOzz(RawAnimation const* rawAnimation, RawSkeleton const* rawSkeleton)
    {
        ozz::animation::offline::RawAnimation ozzAnimation;

        int jointsInSkeleton = rawSkeleton->Joints.Size();

        ozzAnimation.name = rawAnimation->Name.CStr();
        ozzAnimation.duration = 0.0f;

        Vector<Vector<RawAnimation::Channel const*>> channelsPerJoint;
        channelsPerJoint.Resize(jointsInSkeleton);

        ozzAnimation.tracks.resize(jointsInSkeleton);

        for (size_t channelIndex = 0; channelIndex < rawAnimation->Channels.Size(); ++channelIndex)
        {
            auto& rawChannel = rawAnimation->Channels[channelIndex];
            if (rawChannel.Type == RawAnimation::Channel::ChannelType::Translation ||
                rawChannel.Type == RawAnimation::Channel::ChannelType::Rotation ||
                rawChannel.Type == RawAnimation::Channel::ChannelType::Scale)
            {
                if (rawChannel.JointIndex < jointsInSkeleton)
                    channelsPerJoint[rawChannel.JointIndex].Add(&rawChannel);
                else
                    LOG("Joint index {} is out of range (0..{})\n", rawChannel.JointIndex, jointsInSkeleton);
            }
        }

        float sampleRate = rawAnimation->SampleRate;
        if (sampleRate <= 0.0f)
            sampleRate = 30.0f;
        else if (sampleRate > 300)
            sampleRate = 300.0f;

        for (int jointIndex = 0; jointIndex < jointsInSkeleton; ++jointIndex)
        {
            auto& track = ozzAnimation.tracks[jointIndex];
            auto& channels = channelsPerJoint[jointIndex];

            for (RawAnimation::Channel const* channel : channels)
            {
                if (!SampleAnimationChannel(channel, sampleRate, track, ozzAnimation.duration))
                    return {};
            }

            // Pads the rest pose transform for any joints which do not have an
            // associated channel for this animation
            if (track.translations.empty())
                track.translations.push_back(CreateTranslationRestPoseKey(rawSkeleton->Joints[jointIndex].Position));
            if (track.rotations.empty())
                track.rotations.push_back(CreateRotationRestPoseKey(rawSkeleton->Joints[jointIndex].Rotation));
            if (track.scales.empty())
                track.scales.push_back(CreateScaleRestPoseKey(rawSkeleton->Joints[jointIndex].Scale));
        }

        if (!ozzAnimation.Validate())
            return {};

        ozz::animation::offline::AnimationBuilder builder;
        ozz::unique_ptr<ozz::animation::Animation> animation = builder(ozzAnimation);
        if (!animation)
            return {};

        // NOTE: Ozz использует свой Deleter для unique_ptr, поэтому мы не можем просто забрать указатель на анимацию
        // и поместить его в наш UniqueRef. Чтобы все было нормально, мы выделяем память под анимацию своим аллокатором и
        // делаем 'move' полученной анимации.
        return MakeUnique<ozz::animation::Animation>(std::move(*animation.get()));
    }

}

UniqueRef<AnimationResource> AnimationResourceBuilder::Build(RawAnimation const& rawAnimation, RawSkeleton const& rawSkeleton)
{
    auto ozzAnimation = ConvertAnimationToOzz(&rawAnimation, &rawSkeleton);
    if (!ozzAnimation)
        return {};
    auto animation = MakeUnique<AnimationResource>();
    animation->m_OzzAnimation = std::move(ozzAnimation);
    return animation;
}

HK_NAMESPACE_END
