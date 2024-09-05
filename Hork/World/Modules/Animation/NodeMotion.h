/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Math/VectorMath.h>
#include <Hork/Math/Quat.h>
#include <Hork/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

enum INTERPOLATION_TYPE
{
    INTERPOLATION_TYPE_STEP,
    INTERPOLATION_TYPE_LINEAR,
    INTERPOLATION_TYPE_CUBIC_SPLINE
};

enum NODE_ANIMATION_PATH_TYPE
{
    NODE_ANIMATION_PATH_TRANSLATION,
    NODE_ANIMATION_PATH_ROTATION,
    NODE_ANIMATION_PATH_SCALE
};

class NodeMotion : public RefCounted // TODO: Resource
{
public:
    Vector<float> m_AnimationTimes;

    Vector<Float3> m_VectorData;
    Vector<Quat> m_QuaternionData;

    struct Sampler
    {
        // Time offset
        size_t Offset;
        // Keyframe count
        size_t Count;
        // Vector or quaternion data offset
        size_t DataOffset;
        // Data interpolation
        INTERPOLATION_TYPE Interpolation;
    };

    struct AnimationChannel
    {
        Sampler Smp;
        uint32_t TargetNode;
        NODE_ANIMATION_PATH_TYPE TargetPath;
    };

    Vector<AnimationChannel> m_Channels;

    Float3 SampleVector(Sampler& sampler, float time);
    Quat SampleQuaternion(Sampler& sampler, float time);
};

HK_NAMESPACE_END
