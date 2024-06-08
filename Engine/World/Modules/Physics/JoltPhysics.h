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

#include <Jolt/Jolt.h>

#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

HK_INLINE JPH::Vec3 ConvertVector(Float3 const& v)
{
    return JPH::Vec3(v.X, v.Y, v.Z);
}

HK_INLINE JPH::Vec4 ConvertVector(Float4 const& v)
{
    return JPH::Vec4(v.X, v.Y, v.Z, v.W);
}

HK_INLINE JPH::Quat ConvertQuaternion(Quat const& q)
{
    return JPH::Quat(q.X, q.Y, q.Z, q.W);
}

HK_INLINE Float3 ConvertVector(JPH::Vec3 const& v)
{
    return Float3(v.GetX(), v.GetY(), v.GetZ());
}

HK_INLINE Float4 ConvertVector(JPH::Vec4 const& v)
{
    return Float4(v.GetX(), v.GetY(), v.GetZ(), v.GetW());
}

HK_INLINE Quat ConvertQuaternion(JPH::Quat const& q)
{
    return Quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

HK_INLINE Float4x4 ConvertMatrix(JPH::Mat44 const& m)
{
    return Float4x4(ConvertVector(m.GetColumn4(0)),
                    ConvertVector(m.GetColumn4(1)),
                    ConvertVector(m.GetColumn4(2)),
                    ConvertVector(m.GetColumn4(3)));
}

HK_INLINE JPH::Mat44 ConvertMatrix(Float4x4 const& m)
{
    return JPH::Mat44(ConvertVector(m.Col0),
                      ConvertVector(m.Col1),
                      ConvertVector(m.Col2),
                      ConvertVector(m.Col3));
}

HK_NAMESPACE_END
