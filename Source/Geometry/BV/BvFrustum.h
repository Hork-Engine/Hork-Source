/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "BvAxisAlignedBox.h"
#include "BvOrientedBox.h"
#include "BvSphere.h"

#include "../Plane.h"

#include <xmmintrin.h>
#include <emmintrin.h>

#define HK_FRUSTUM_USE_SSE

HK_NAMESPACE_BEGIN

enum FRUSTUM_PLANE
{
    FRUSTUM_PLANE_RIGHT,
    FRUSTUM_PLANE_LEFT,
    FRUSTUM_PLANE_TOP,
    FRUSTUM_PLANE_BOTTOM,
    FRUSTUM_PLANE_FAR,
    FRUSTUM_PLANE_NEAR
};

class
#ifdef HK_FRUSTUM_USE_SSE
alignas(16)
#endif
BvFrustum
{
public:
    BvFrustum();
    ~BvFrustum();

    void FromMatrix(Float4x4 const& matrix, bool bReversedDepth = false);

    PlaneF const& operator[](const int index) const;

    bool IsPointVisible(Float3 const& point) const;
    bool IsPointVisible_IgnoreZ(Float3 const& point) const;

    bool IsSphereVisible(BvSphere const& sphere) const;
    bool IsSphereVisible(Float3 const& point, float radius) const;
    bool IsSphereVisible_IgnoreZ(BvSphere const& sphere) const;
    bool IsSphereVisible_IgnoreZ(Float3 const& point, float radius) const;

    bool IsBoxVisible(Float3 const& mins, Float3 const& maxs) const;
    bool IsBoxVisible(Float4 const& mins, Float4 const& maxs) const;
    bool IsBoxVisible(BvAxisAlignedBox const& b) const;
    bool IsBoxVisible_IgnoreZ(Float3 const& mins, Float3 const& maxs) const;
    bool IsBoxVisible_IgnoreZ(Float4 const& mins, Float4 const& maxs) const;
    bool IsBoxVisible_IgnoreZ(BvAxisAlignedBox const& b) const;

    bool IsOrientedBoxVisible(BvOrientedBox const& b) const;
    bool IsOrientedBoxVisible_IgnoreZ(BvOrientedBox const& b) const;

    void CullSphere_Generic(BvSphereSSE const* bounds, int count, int* result) const;
    void CullSphere_IgnoreZ_Generic(BvSphereSSE const* bounds, int count, int* result) const;
    void CullBox_Generic(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const;
    void CullBox_IgnoreZ_Generic(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const;
    void CullSphere_SSE(BvSphereSSE const* bounds, int count, int* result) const;
    void CullSphere_IgnoreZ_SSE(BvSphereSSE const* bounds, int count, int* result) const;
    void CullBox_SSE(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const;
    void CullBox_IgnoreZ_SSE(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const;

    // Top-right
    void CornerVector_TR(Float3& vector) const;

    // Top-left
    void CornerVector_TL(Float3& vector) const;

    // Bottom-right
    void CornerVector_BR(Float3& vector) const;

    // Bottom-left
    void CornerVector_BL(Float3& vector) const;

private:
#ifdef HK_FRUSTUM_USE_SSE
    struct PlanesData
    {
        __m128 x[6];
        __m128 y[6];
        __m128 z[6];
        __m128 d[6];
    };
    PlanesData m_SSEData;
#endif
    PlaneF m_Planes[6];
};

HK_FORCEINLINE PlaneF const& BvFrustum::operator[](const int index) const
{
    HK_ASSERT_((unsigned)index < 6, "BvFrustum[]");
    return m_Planes[index];
}

HK_FORCEINLINE bool BvFrustum::IsPointVisible(Float3 const& point) const
{
    bool inside = true;
    for (PlaneF const* p = m_Planes; p < m_Planes + 6; p++)
    {
        inside &= (Math::Dot(*p, point) > 0.0f);
    }
    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsPointVisible_IgnoreZ(Float3 const& point) const
{
    bool inside = true;
    for (PlaneF const* p = m_Planes; p < m_Planes + 4; p++)
    {
        inside &= (Math::Dot(*p, point) > 0.0f);
    }
    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsSphereVisible(BvSphere const& sphere) const
{
    return IsSphereVisible(sphere.Center, sphere.Radius);
}

HK_FORCEINLINE bool BvFrustum::IsSphereVisible(Float3 const& point, float radius) const
{
    bool inside = true;
    for (PlaneF const* p = m_Planes; p < m_Planes + 6; p++)
    {
        inside &= (Math::Dot(*p, point) > -radius);
    }
    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsSphereVisible_IgnoreZ(BvSphere const& sphere) const
{
    return IsSphereVisible_IgnoreZ(sphere.Center, sphere.Radius);
}

HK_FORCEINLINE bool BvFrustum::IsSphereVisible_IgnoreZ(Float3 const& point, float radius) const
{
    bool inside = true;
    for (PlaneF const* p = m_Planes; p < m_Planes + 4; p++)
    {
        inside &= (Math::Dot(*p, point) > -radius);
    }
    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsBoxVisible(Float3 const& mins, Float3 const& maxs) const
{
    bool inside = true;

    PlaneF const* p = m_Planes;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsBoxVisible(Float4 const& mins, Float4 const& maxs) const
{
    bool inside = true;

    PlaneF const* p = m_Planes;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsBoxVisible(BvAxisAlignedBox const& b) const
{
    return IsBoxVisible(b.Mins, b.Maxs);
}

HK_FORCEINLINE bool BvFrustum::IsBoxVisible_IgnoreZ(Float3 const& mins, Float3 const& maxs) const
{
    bool inside = true;

    PlaneF const* p = m_Planes;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsBoxVisible_IgnoreZ(Float4 const& mins, Float4 const& maxs) const
{
    bool inside = true;

    PlaneF const* p = m_Planes;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    ++p;
    inside &= (Math::Max(mins.X * p->Normal.X, maxs.X * p->Normal.X) + Math::Max(mins.Y * p->Normal.Y, maxs.Y * p->Normal.Y) + Math::Max(mins.Z * p->Normal.Z, maxs.Z * p->Normal.Z) + p->D) > 0.0f;

    return inside;
}

HK_FORCEINLINE bool BvFrustum::IsBoxVisible_IgnoreZ(BvAxisAlignedBox const& b) const
{
    return IsBoxVisible_IgnoreZ(b.Mins, b.Maxs);
}

HK_FORCEINLINE bool BvFrustum::IsOrientedBoxVisible(BvOrientedBox const& b) const
{
    Float3 point;

    for (PlaneF const* p = m_Planes; p < m_Planes + 6; p++)
    {
        const float x = Math::Dot(b.Orient[0], p->Normal) >= 0.0f ? -b.HalfSize[0] : b.HalfSize[0];
        const float y = Math::Dot(b.Orient[1], p->Normal) >= 0.0f ? -b.HalfSize[1] : b.HalfSize[1];
        const float z = Math::Dot(b.Orient[2], p->Normal) >= 0.0f ? -b.HalfSize[2] : b.HalfSize[2];

        point = b.Center + (x * b.Orient[0] + y * b.Orient[1] + z * b.Orient[2]);

        if (Math::Dot(point, *p) >= 0.0f)
        {
            return false;
        }
    }

    return true;
}

HK_FORCEINLINE bool BvFrustum::IsOrientedBoxVisible_IgnoreZ(BvOrientedBox const& b) const
{
    Float3 point;

    for (PlaneF const* p = m_Planes; p < m_Planes + 4; p++)
    {
        const float x = Math::Dot(b.Orient[0], p->Normal) >= 0.0f ? -b.HalfSize[0] : b.HalfSize[0];
        const float y = Math::Dot(b.Orient[1], p->Normal) >= 0.0f ? -b.HalfSize[1] : b.HalfSize[1];
        const float z = Math::Dot(b.Orient[2], p->Normal) >= 0.0f ? -b.HalfSize[2] : b.HalfSize[2];

        point = b.Center + (x * b.Orient[0] + y * b.Orient[1] + z * b.Orient[2]);

        if (Math::Dot(point, *p) >= 0.0f)
        {
            return false;
        }
    }

    return true;
}

HK_NAMESPACE_END
