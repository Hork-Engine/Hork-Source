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

#include "NodeMotion.h"

HK_NAMESPACE_BEGIN

Float3 NodeMotion::SampleVector(Sampler& sampler, float time)
{
    using namespace Hk;

    float const* animtimes = m_AnimationTimes.ToPtr() + sampler.Offset;
    size_t count = sampler.Count;

    Float3 const* data = m_VectorData.ToPtr() + sampler.DataOffset;

    float ft0, ftN, ct, nt;

    HK_ASSERT(count > 0);

    ft0 = animtimes[0];

    if (count == 1 || time <= ft0)
    {
        if (sampler.Interpolation == INTERPOLATION_TYPE_CUBIC_SPLINE)
        {
            return data[0 * 3 + 1];
        }
        else
        {
            return data[0];
        }
    }

    ftN = animtimes[count - 1];

    if (time >= ftN)
    {
        if (sampler.Interpolation == INTERPOLATION_TYPE_CUBIC_SPLINE)
        {
            return data[(count - 1) * 3 + 1];
        }
        else
        {
            return data[count - 1];
        }
    }

    ct = ft0;

    // TODO: binary search
    for (int t = 0; t < (int)count - 1; t++, ct = nt)
    {
        nt = animtimes[t + 1];

        if (ct <= time && nt > time)
        {
            if (sampler.Interpolation == INTERPOLATION_TYPE_LINEAR)
            {
                if (time == ct)
                {
                    return data[t];
                }
                else
                {
                    Float3 p0, p1;
                    p0 = data[t];
                    p1 = data[t + 1];
                    float dur = nt - ct;
                    float fract = (time - ct) / dur;
                    HK_ASSERT(fract >= 0.0f && fract <= 1.0f);
                    return Math::Lerp(p0, p1, fract);
                }
            }
            else if (sampler.Interpolation == INTERPOLATION_TYPE_STEP)
            {
                return data[t];
            }
            else if (sampler.Interpolation == INTERPOLATION_TYPE_CUBIC_SPLINE)
            {
                float dur = nt - ct;
                float fract = (dur == 0.0f) ? 0.0f : (time - ct) / dur;
                HK_ASSERT(fract >= 0.0f && fract <= 1.0f);

                Float3 p0, m0, m1, p1;

                p0 = data[t * 3 + 1];
                m0 = data[t * 3 + 2];
                m1 = data[(t + 1) * 3];
                p1 = data[(t + 1) * 3 + 1];

                m0 *= dur;
                m1 *= dur;

                return Math::HermiteCubicSpline(p0, m0, p1, m1, fract);
            }
            break;
        }
    }

    return Float3(0.0f);
}

Quat NodeMotion::SampleQuaternion(Sampler& sampler, float time)
{
    using namespace Hk;

    float const* animtimes = m_AnimationTimes.ToPtr() + sampler.Offset;
    size_t count = sampler.Count;

    Quat const* data = m_QuaternionData.ToPtr() + sampler.DataOffset;

    float ft0, ftN, ct, nt;

    HK_ASSERT(count > 0);

    ft0 = animtimes[0];

    if (count == 1 || time <= ft0)
    {
        if (sampler.Interpolation == INTERPOLATION_TYPE_CUBIC_SPLINE)
        {
            return data[0 * 3 + 1];
        }
        else
        {
            return data[0];
        }
    }

    ftN = animtimes[count - 1];

    if (time >= ftN)
    {
        if (sampler.Interpolation == INTERPOLATION_TYPE_CUBIC_SPLINE)
        {
            return data[(count - 1) * 3 + 1];
        }
        else
        {
            return data[count - 1];
        }
    }

    ct = ft0;

    // TODO: binary search
    for (int t = 0; t < (int)count - 1; t++, ct = nt)
    {
        nt = animtimes[t + 1];

        if (ct <= time && nt > time)
        {
            if (sampler.Interpolation == INTERPOLATION_TYPE_LINEAR)
            {
                if (time == ct)
                {
                    return data[t];
                }
                else
                {
                    Quat p0, p1;
                    p0 = data[t];
                    p1 = data[t + 1];
                    float dur = nt - ct;
                    float fract = (time - ct) / dur;
                    HK_ASSERT(fract >= 0.0f && fract <= 1.0f);
                    return Math::Slerp(p0, p1, fract).Normalized();
                }
            }
            else if (sampler.Interpolation == INTERPOLATION_TYPE_STEP)
            {
                return data[t];
            }
            else if (sampler.Interpolation == INTERPOLATION_TYPE_CUBIC_SPLINE)
            {
                float dur = nt - ct;
                float fract = (dur == 0.0f) ? 0.0f : (time - ct) / dur;
                HK_ASSERT(fract >= 0.0f && fract <= 1.0f);

                Quat p0, m0, m1, p1;

                p0 = data[t * 3 + 1];
                m0 = data[t * 3 + 2];
                m1 = data[(t + 1) * 3];
                p1 = data[(t + 1) * 3 + 1];

                m0 *= dur;
                m1 *= dur;

                p0.NormalizeSelf();
                m0.NormalizeSelf();
                m1.NormalizeSelf();
                p1.NormalizeSelf();

                return Math::HermiteCubicSpline(p0, m0, p1, m1, fract).Normalized();
            }
            break;
        }
    }

    return Quat::Identity();
}

HK_NAMESPACE_END
