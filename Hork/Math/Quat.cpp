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

#include "Quat.h"

HK_NAMESPACE_BEGIN

namespace Math {

Quat GetRotation(Float3 const& from, Float3 const& to)
{
    float dp = Math::Dot(from, to);
    float k = sqrt(from.LengthSqr() * to.LengthSqr());

    if (dp / k == -1)
    {
        Float3 axis = from.Perpendicular();
        return Quat(0, axis.X, axis.Y, axis.Z);
    }

    Float3 axis = Math::Cross(from, to);
    return Quat(dp + k, axis.X, axis.Y, axis.Z).Normalized();
}

Quat Slerp(Quat const& qs, Quat const& qe, float f)
{
    if (f <= 0.0f)
    {
        return qs;
    }

    if (f >= 1.0f || qs == qe)
    {
        return qe;
    }

    Quat  temp;
    float scale0, scale1;
    float cosOmega = qs.X * qe.X + qs.Y * qe.Y + qs.Z * qe.Z + qs.W * qe.W;
    if (cosOmega < 0.0f)
    {
        temp     = -qe;
        cosOmega = -cosOmega;
    }
    else
    {
        temp = qe;
    }

    if (1.0 - cosOmega > 1e-6f)
    {
        float sinOmega    = std::sqrt(1.0f - cosOmega * cosOmega);
        float omega       = std::atan2(sinOmega, cosOmega);
        float invSinOmega = 1.0f / sinOmega;
        scale0            = std::sin((1.0f - f) * omega) * invSinOmega;
        scale1            = std::sin(f * omega) * invSinOmega;
    }
    else
    {
        // too small angle, use linear interpolation
        scale0 = 1.0f - f;
        scale1 = f;
    }

    return scale0 * qs + scale1 * temp;
}

Quat MakeRotationFromDir(Float3 const& direction)
{
    Float3x3 orientation;
    Float3 dir = -direction.Normalized();

    if (dir.X * dir.X + dir.Z * dir.Z == 0.0f)
    {
        orientation[0] = Float3(1, 0, 0);
        orientation[1] = Float3(0, 0, -dir.Y);
    }
    else
    {
        orientation[0] = Math::Cross(Float3(0.0f, 1.0f, 0.0f), dir).Normalized();
        orientation[1] = Math::Cross(dir, orientation[0]);
    }
    orientation[2] = dir;

    Quat rotation;
    rotation.FromMatrix(orientation);
    return rotation;
}

void GetTransformVectors(Quat const& rotation, Float3* right, Float3* up, Float3* back)
{
    float qxx(rotation.X * rotation.X);
    float qyy(rotation.Y * rotation.Y);
    float qzz(rotation.Z * rotation.Z);
    float qxz(rotation.X * rotation.Z);
    float qxy(rotation.X * rotation.Y);
    float qyz(rotation.Y * rotation.Z);
    float qwx(rotation.W * rotation.X);
    float qwy(rotation.W * rotation.Y);
    float qwz(rotation.W * rotation.Z);

    if (right)
    {
        right->X = 1 - 2 * (qyy + qzz);
        right->Y = 2 * (qxy + qwz);
        right->Z = 2 * (qxz - qwy);
    }

    if (up)
    {
        up->X = 2 * (qxy - qwz);
        up->Y = 1 - 2 * (qxx + qzz);
        up->Z = 2 * (qyz + qwx);
    }

    if (back)
    {
        back->X = 2 * (qxz + qwy);
        back->Y = 2 * (qyz - qwx);
        back->Z = 1 - 2 * (qxx + qyy);
    }
}

} // namespace Math

HK_NAMESPACE_END
