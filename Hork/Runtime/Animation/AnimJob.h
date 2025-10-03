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

#pragma once

#include <Hork/Core/BaseTypes.h>
#include <Hork/Resources/Resource_Animation.h>

#include <Hork/Math/Simd/Simd.h>

HK_NAMESPACE_BEGIN

enum class AnimJobType
{
    Sample,
    Blend,
    Sum,
    Backup,
    Restore
};

class AnimJob
{
public:
    AnimJob(AnimJobType type) : m_Type(type)
    {}

    virtual ~AnimJob() {}

    AnimJobType GetType() const { return m_Type; }

    SoaTransform* Pose = nullptr;

private:
    AnimJobType m_Type;
};

struct AnimationSampleContext;

class AnimJob_Sample : public AnimJob
{
public:
    AnimJob_Sample() : AnimJob(AnimJobType::Sample)
    {}

    AnimationHandle m_Clip;
    float m_Phase = 0;
    std::shared_ptr<AnimationSampleContext> m_SamplingContext;
};

class AnimJob_Blend : public AnimJob
{
public:
    AnimJob_Blend() : AnimJob(AnimJobType::Blend)
    {}

    uint32_t m_ChildJobIDs[2];
    float m_Weight;
};

class AnimJob_Sum : public AnimJob
{
public:
    AnimJob_Sum() : AnimJob(AnimJobType::Sum)
    {}

    uint32_t m_ChildJobIDs[2];
};

class AnimJob_Backup : public AnimJob
{
public:
    AnimJob_Backup() : AnimJob(AnimJobType::Backup)
    {}

    uint32_t m_SavedJobID;
    uint32_t m_SavedPoseIndex;
};

class AnimJob_Restore : public AnimJob
{
public:
    AnimJob_Restore() : AnimJob(AnimJobType::Restore)
    {}

    uint32_t m_SavedPoseIndex;
};

HK_NAMESPACE_END
