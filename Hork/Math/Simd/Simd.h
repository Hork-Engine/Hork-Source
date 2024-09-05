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

#include <ozz/base/maths/soa_transform.h>

HK_NAMESPACE_BEGIN

using SoaTransform  = ozz::math::SoaTransform;
using SimdFloat4x4  = ozz::math::Float4x4;
using SimdFloat4    = ozz::math::SimdFloat4;

namespace Simd
{

HK_FORCEINLINE SimdFloat4 One()
{
    return ozz::math::simd_float4::one();
}

HK_FORCEINLINE SimdFloat4 Zero()
{
    return ozz::math::simd_float4::zero();
}

HK_FORCEINLINE SimdFloat4 AxisX()
{
    return ozz::math::simd_float4::x_axis();
}

HK_FORCEINLINE SimdFloat4 AxisY()
{
    return ozz::math::simd_float4::y_axis();
}

HK_FORCEINLINE SimdFloat4 AxisZ()
{
    return ozz::math::simd_float4::z_axis();
}

HK_FORCEINLINE SimdFloat4 AxisW()
{
    return ozz::math::simd_float4::w_axis();
}

HK_FORCEINLINE SimdFloat4 LoadFloat4(float x, float y, float z, float w)
{
    return ozz::math::simd_float4::Load(x, y, z, w);
}

HK_FORCEINLINE SimdFloat4 LoadPtr(const float* f)
{
    return ozz::math::simd_float4::LoadPtr(f);
}

HK_FORCEINLINE SimdFloat4 Load3PtrU(const float* f)
{
    return ozz::math::simd_float4::Load3PtrU(f);
}

HK_FORCEINLINE SimdFloat4 LoadPtrU(const float* f)
{
    return ozz::math::simd_float4::LoadPtrU(f);
}

HK_FORCEINLINE void StorePtr(SimdFloat4 v, float* f)
{
    ozz::math::StorePtr(v, f);
}

HK_FORCEINLINE SimdFloat4 NormalizeSafe4(SimdFloat4 v, SimdFloat4 safe)
{
    return ozz::math::NormalizeSafe4(v, safe);
}

HK_FORCEINLINE void Transpose4x3(const SimdFloat4 in[4], SimdFloat4 out[3])
{
    return ozz::math::Transpose4x3(in, out);
}

HK_FORCEINLINE void Transpose4x4(const SimdFloat4 in[4], SimdFloat4 out[4])
{
    return ozz::math::Transpose4x4(in, out);
}

HK_FORCEINLINE void StoreFloat4x4(const SimdFloat4 in[4], Float4x4& out)
{
    StorePtr(in[0], &out[0][0]);
    StorePtr(in[1], &out[1][0]);
    StorePtr(in[2], &out[2][0]);
    StorePtr(in[3], &out[3][0]);
}

HK_FORCEINLINE void LoadFloat4x4(Float4x4 const& in, SimdFloat4 out[4])
{
    out[0] = LoadPtr(&in.Col0[0]);
    out[1] = LoadPtr(&in.Col1[0]);
    out[2] = LoadPtr(&in.Col2[0]);
    out[3] = LoadPtr(&in.Col3[0]);
}

HK_FORCEINLINE bool Decompose(const SimdFloat4x4& m, SimdFloat4* position, SimdFloat4* rotation, SimdFloat4* scale)
{
    return ozz::math::ToAffine(m, position, rotation, scale);
}

}

HK_NAMESPACE_END
