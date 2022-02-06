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

#include <Geometry/BV/BvFrustum.h>
#include <Platform/Logger.h>

BvFrustum::BvFrustum()
{
    PlanesSSE = nullptr;
}

BvFrustum::~BvFrustum()
{
#ifdef HK_FRUSTUM_USE_SSE
    GZoneMemory.Free(PlanesSSE);
#endif
}

void BvFrustum::FromMatrix(Float4x4 const& matrix, bool bReversedDepth)
{
    Float4x4 flipZ;

    flipZ.SetIdentity();
    flipZ[2][2] = -1.0f;
    flipZ[3][2] = 1.0f;

    Float4x4 m = bReversedDepth ? flipZ * matrix : matrix;

    planes_[FRUSTUM_PLANE_RIGHT].Normal.X = m[0][3] - m[0][0];
    planes_[FRUSTUM_PLANE_RIGHT].Normal.Y = m[1][3] - m[1][0];
    planes_[FRUSTUM_PLANE_RIGHT].Normal.Z = m[2][3] - m[2][0];
    planes_[FRUSTUM_PLANE_RIGHT].D        = m[3][3] - m[3][0];
    planes_[FRUSTUM_PLANE_RIGHT].NormalizeSelf();

    planes_[FRUSTUM_PLANE_LEFT].Normal.X = m[0][3] + m[0][0];
    planes_[FRUSTUM_PLANE_LEFT].Normal.Y = m[1][3] + m[1][0];
    planes_[FRUSTUM_PLANE_LEFT].Normal.Z = m[2][3] + m[2][0];
    planes_[FRUSTUM_PLANE_LEFT].D        = m[3][3] + m[3][0];
    planes_[FRUSTUM_PLANE_LEFT].NormalizeSelf();

    planes_[FRUSTUM_PLANE_TOP].Normal.X = m[0][3] + m[0][1];
    planes_[FRUSTUM_PLANE_TOP].Normal.Y = m[1][3] + m[1][1];
    planes_[FRUSTUM_PLANE_TOP].Normal.Z = m[2][3] + m[2][1];
    planes_[FRUSTUM_PLANE_TOP].D        = m[3][3] + m[3][1];
    planes_[FRUSTUM_PLANE_TOP].NormalizeSelf();

    planes_[FRUSTUM_PLANE_BOTTOM].Normal.X = m[0][3] - m[0][1];
    planes_[FRUSTUM_PLANE_BOTTOM].Normal.Y = m[1][3] - m[1][1];
    planes_[FRUSTUM_PLANE_BOTTOM].Normal.Z = m[2][3] - m[2][1];
    planes_[FRUSTUM_PLANE_BOTTOM].D        = m[3][3] - m[3][1];
    planes_[FRUSTUM_PLANE_BOTTOM].NormalizeSelf();

    planes_[FRUSTUM_PLANE_FAR].Normal.X = m[0][3] - m[0][2];
    planes_[FRUSTUM_PLANE_FAR].Normal.Y = m[1][3] - m[1][2];
    planes_[FRUSTUM_PLANE_FAR].Normal.Z = m[2][3] - m[2][2];
    planes_[FRUSTUM_PLANE_FAR].D        = m[3][3] - m[3][2];
    planes_[FRUSTUM_PLANE_FAR].NormalizeSelf();

    planes_[FRUSTUM_PLANE_NEAR].Normal.X = m[0][3] + m[0][2];
    planes_[FRUSTUM_PLANE_NEAR].Normal.Y = m[1][3] + m[1][2];
    planes_[FRUSTUM_PLANE_NEAR].Normal.Z = m[2][3] + m[2][2];
    planes_[FRUSTUM_PLANE_NEAR].D        = m[3][3] + m[3][2];
    planes_[FRUSTUM_PLANE_NEAR].NormalizeSelf();

#ifdef HK_FRUSTUM_USE_SSE
    if (!PlanesSSE)
    {
        PlanesSSE = (sse_t*)GZoneMemory.Alloc(sizeof(sse_t));
    }

    PlanesSSE->x[0] = _mm_set1_ps(planes_[0].Normal.X);
    PlanesSSE->y[0] = _mm_set1_ps(planes_[0].Normal.Y);
    PlanesSSE->z[0] = _mm_set1_ps(planes_[0].Normal.Z);
    PlanesSSE->d[0] = _mm_set1_ps(planes_[0].D);

    PlanesSSE->x[1] = _mm_set1_ps(planes_[1].Normal.X);
    PlanesSSE->y[1] = _mm_set1_ps(planes_[1].Normal.Y);
    PlanesSSE->z[1] = _mm_set1_ps(planes_[1].Normal.Z);
    PlanesSSE->d[1] = _mm_set1_ps(planes_[1].D);

    PlanesSSE->x[2] = _mm_set1_ps(planes_[2].Normal.X);
    PlanesSSE->y[2] = _mm_set1_ps(planes_[2].Normal.Y);
    PlanesSSE->z[2] = _mm_set1_ps(planes_[2].Normal.Z);
    PlanesSSE->d[2] = _mm_set1_ps(planes_[2].D);

    PlanesSSE->x[3] = _mm_set1_ps(planes_[3].Normal.X);
    PlanesSSE->y[3] = _mm_set1_ps(planes_[3].Normal.Y);
    PlanesSSE->z[3] = _mm_set1_ps(planes_[3].Normal.Z);
    PlanesSSE->d[3] = _mm_set1_ps(planes_[3].D);

    PlanesSSE->x[4] = _mm_set1_ps(planes_[4].Normal.X);
    PlanesSSE->y[4] = _mm_set1_ps(planes_[4].Normal.Y);
    PlanesSSE->z[4] = _mm_set1_ps(planes_[4].Normal.Z);
    PlanesSSE->d[4] = _mm_set1_ps(planes_[4].D);

    PlanesSSE->x[5] = _mm_set1_ps(planes_[5].Normal.X);
    PlanesSSE->y[5] = _mm_set1_ps(planes_[5].Normal.Y);
    PlanesSSE->z[5] = _mm_set1_ps(planes_[5].Normal.Z);
    PlanesSSE->d[5] = _mm_set1_ps(planes_[5].D);
#endif
}

void BvFrustum::CornerVector_TR(Float3& vector) const
{
    vector = Math::Cross(planes_[FRUSTUM_PLANE_TOP].Normal, planes_[FRUSTUM_PLANE_RIGHT].Normal).Normalized();
}

void BvFrustum::CornerVector_TL(Float3& vector) const
{
    vector = Math::Cross(planes_[FRUSTUM_PLANE_LEFT].Normal, planes_[FRUSTUM_PLANE_TOP].Normal).Normalized();
}

void BvFrustum::CornerVector_BR(Float3& vector) const
{
    vector = Math::Cross(planes_[FRUSTUM_PLANE_RIGHT].Normal, planes_[FRUSTUM_PLANE_BOTTOM].Normal).Normalized();
}

void BvFrustum::CornerVector_BL(Float3& vector) const
{
    vector = Math::Cross(planes_[FRUSTUM_PLANE_BOTTOM].Normal, planes_[FRUSTUM_PLANE_LEFT].Normal).Normalized();
}

void BvFrustum::CullSphere_Generic(BvSphereSSE const* bounds, int count, int* result) const
{
    bool          inside;
    PlaneF const* p;

    for (BvSphereSSE const* Last = bounds + count; bounds < Last; bounds++)
    {
        inside = true;
        for (p = planes_; p < planes_ + 6; p++)
        {
            inside &= (Math::Dot(p->Normal, bounds->Center) + p->D > -bounds->Radius);
        }
        *result++ = !inside;
    }
}

void BvFrustum::CullSphere_IgnoreZ_Generic(BvSphereSSE const* bounds, int count, int* result) const
{
    bool          inside;
    PlaneF const* p;

    for (BvSphereSSE const* Last = bounds + count; bounds < Last; bounds++)
    {
        inside = true;
        for (p = planes_; p < planes_ + 4; p++)
        {
            inside &= (Math::Dot(p->Normal, bounds->Center) + p->D > -bounds->Radius);
        }
        *result++ = !inside;
    }
}

void BvFrustum::CullBox_Generic(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const
{
    for (BvAxisAlignedBoxSSE const* Last = bounds + count; bounds < Last; bounds++)
    {
        *result++ = !IsBoxVisible(bounds->Mins, bounds->Maxs);
    }
}

void BvFrustum::CullBox_IgnoreZ_Generic(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const
{
    for (BvAxisAlignedBoxSSE const* Last = bounds + count; bounds < Last; bounds++)
    {
        *result++ = !IsBoxVisible_IgnoreZ(bounds->Mins, bounds->Maxs);
    }
}

void BvFrustum::CullSphere_SSE(BvSphereSSE const* bounds, int count, int* result) const
{
#ifdef HK_FRUSTUM_USE_SSE
    const float* sphere_data_ptr = reinterpret_cast<const float*>(&bounds->Center[0]);
    int*         culling_res_sse = &result[0];

    //to optimize calculations we gather xyzw elements in separate vectors
    __m128 zero_v = _mm_setzero_ps();
    int    i, j;

    //we process 4 objects per step
    for (i = 0; i < count; i += 4)
    {
        //load bounding sphere data
        __m128 spheres_pos_x  = _mm_load_ps(sphere_data_ptr);
        __m128 spheres_pos_y  = _mm_load_ps(sphere_data_ptr + 4);
        __m128 spheres_pos_z  = _mm_load_ps(sphere_data_ptr + 8);
        __m128 spheres_radius = _mm_load_ps(sphere_data_ptr + 12);
        sphere_data_ptr += 16;

        //but for our calculations we need transpose data, to collect x, y, z and w coordinates in separate vectors
        _MM_TRANSPOSE4_PS(spheres_pos_x, spheres_pos_y, spheres_pos_z, spheres_radius);

        __m128 spheres_neg_radius = _mm_sub_ps(zero_v, spheres_radius); // negate all elements
        //http://fastcpp.blogspot.ru/2011/03/changing-sign-of-float-values-using-sse.html

        __m128 intersection_res = _mm_setzero_ps();

        for (j = 0; j < 6; j++) //plane index
        {
            //1. calc distance to plane dot(sphere_pos.Xyz, plane.Xyz) + plane.w
            //2. if distance < sphere radius, then sphere outside frustum
            __m128 dot_x = _mm_mul_ps(spheres_pos_x, PlanesSSE->x[j]);
            __m128 dot_y = _mm_mul_ps(spheres_pos_y, PlanesSSE->y[j]);
            __m128 dot_z = _mm_mul_ps(spheres_pos_z, PlanesSSE->z[j]);

            __m128 sum_xy            = _mm_add_ps(dot_x, dot_y);
            __m128 sum_zw            = _mm_add_ps(dot_z, PlanesSSE->d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            __m128 plane_res = _mm_cmple_ps(distance_to_plane, spheres_neg_radius); //dist < -sphere_r ?
            intersection_res = _mm_or_ps(intersection_res, plane_res);              //if yes - sphere behind the plane & outside frustum
        }

        //store result
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
        _mm_store_si128((__m128i*)&culling_res_sse[i], intersection_res_i);
    }
#else
    return CullSphere_Generic(bounds, count, result);
#endif
}

void BvFrustum::CullSphere_IgnoreZ_SSE(BvSphereSSE const* bounds, int count, int* result) const
{
#ifdef HK_FRUSTUM_USE_SSE
    const float* sphere_data_ptr = reinterpret_cast<const float*>(&bounds->Center[0]);
    int*         culling_res_sse = &result[0];

    //to optimize calculations we gather xyzw elements in separate vectors
    __m128 zero_v = _mm_setzero_ps();
    int    i, j;

    //we process 4 objects per step
    for (i = 0; i < count; i += 4)
    {
        //load bounding sphere data
        __m128 spheres_pos_x  = _mm_load_ps(sphere_data_ptr);
        __m128 spheres_pos_y  = _mm_load_ps(sphere_data_ptr + 4);
        __m128 spheres_pos_z  = _mm_load_ps(sphere_data_ptr + 8);
        __m128 spheres_radius = _mm_load_ps(sphere_data_ptr + 12);
        sphere_data_ptr += 16;

        //but for our calculations we need transpose data, to collect x, y, z and w coordinates in separate vectors
        _MM_TRANSPOSE4_PS(spheres_pos_x, spheres_pos_y, spheres_pos_z, spheres_radius);

        __m128 spheres_neg_radius = _mm_sub_ps(zero_v, spheres_radius); // negate all elements
        //http://fastcpp.blogspot.ru/2011/03/changing-sign-of-float-values-using-sse.html

        __m128 intersection_res = _mm_setzero_ps();

        for (j = 0; j < 4; j++) //plane index
        {
            //1. calc distance to plane dot(sphere_pos.Xyz, plane.Xyz) + plane.w
            //2. if distance < sphere radius, then sphere outside frustum
            __m128 dot_x = _mm_mul_ps(spheres_pos_x, PlanesSSE->x[j]);
            __m128 dot_y = _mm_mul_ps(spheres_pos_y, PlanesSSE->y[j]);
            __m128 dot_z = _mm_mul_ps(spheres_pos_z, PlanesSSE->z[j]);

            __m128 sum_xy            = _mm_add_ps(dot_x, dot_y);
            __m128 sum_zw            = _mm_add_ps(dot_z, PlanesSSE->d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            __m128 plane_res = _mm_cmple_ps(distance_to_plane, spheres_neg_radius); //dist < -sphere_r ?
            intersection_res = _mm_or_ps(intersection_res, plane_res);              //if yes - sphere behind the plane & outside frustum
        }

        //store result
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
        _mm_store_si128((__m128i*)&culling_res_sse[i], intersection_res_i);
    }
#else
    return CullSphere_IgnoreZ_Generic(bounds, count, result);
#endif
}

void BvFrustum::CullBox_SSE(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const
{
#ifdef HK_FRUSTUM_USE_SSE
    const float* pBoundingBoxData = reinterpret_cast<const float*>(&bounds->Mins.X);
    int*         pCullingResult   = &result[0];
    int          i, j;

    __m128 zero = _mm_setzero_ps();

    // Process 4 objects per step
    for (i = 0; i < count; i += 4)
    {
        // Load bounding mins
        __m128 aabb_min_x = _mm_load_ps(pBoundingBoxData);
        __m128 aabb_min_y = _mm_load_ps(pBoundingBoxData + 8);
        __m128 aabb_min_z = _mm_load_ps(pBoundingBoxData + 16);
        __m128 aabb_min_w = _mm_load_ps(pBoundingBoxData + 24);

        // Load bounding maxs
        __m128 aabb_max_x = _mm_load_ps(pBoundingBoxData + 4);
        __m128 aabb_max_y = _mm_load_ps(pBoundingBoxData + 12);
        __m128 aabb_max_z = _mm_load_ps(pBoundingBoxData + 20);
        __m128 aabb_max_w = _mm_load_ps(pBoundingBoxData + 28);

        pBoundingBoxData += 32;

        // For now we have points in vectors aabb_min_x..w, but for calculations we need to xxxx yyyy zzzz vectors representation.
        // Just transpose data.
        _MM_TRANSPOSE4_PS(aabb_min_x, aabb_min_y, aabb_min_z, aabb_min_w);
        _MM_TRANSPOSE4_PS(aabb_max_x, aabb_max_y, aabb_max_z, aabb_max_w);

        // Set bitmask to zero
        __m128 intersection_res = _mm_setzero_ps();

        for (j = 0; j < 6; j++)
        {
            // Pick closest point to plane and check if it begind the plane. if yes - object outside frustum

            // Dot product, separate for each coordinate, for min & max aabb points
            __m128 mins_mul_plane_x = _mm_mul_ps(aabb_min_x, PlanesSSE->x[j]);
            __m128 mins_mul_plane_y = _mm_mul_ps(aabb_min_y, PlanesSSE->y[j]);
            __m128 mins_mul_plane_z = _mm_mul_ps(aabb_min_z, PlanesSSE->z[j]);

            __m128 maxs_mul_plane_x = _mm_mul_ps(aabb_max_x, PlanesSSE->x[j]);
            __m128 maxs_mul_plane_y = _mm_mul_ps(aabb_max_y, PlanesSSE->y[j]);
            __m128 maxs_mul_plane_z = _mm_mul_ps(aabb_max_z, PlanesSSE->z[j]);

            // We have 8 box points, but we need pick closest point to plane.
            __m128 res_x = _mm_max_ps(mins_mul_plane_x, maxs_mul_plane_x);
            __m128 res_y = _mm_max_ps(mins_mul_plane_y, maxs_mul_plane_y);
            __m128 res_z = _mm_max_ps(mins_mul_plane_z, maxs_mul_plane_z);

            // Distance to plane = dot( aabb_point.xyz, plane.xyz ) + plane.d
            __m128 sum_xy            = _mm_add_ps(res_x, res_y);
            __m128 sum_zw            = _mm_add_ps(res_z, PlanesSSE->d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            // Dist from closest point to plane < 0 ?
            __m128 plane_res = _mm_cmple_ps(distance_to_plane, zero);

            // If yes - aabb behind the plane & outside frustum
            intersection_res = _mm_or_ps(intersection_res, plane_res);
        }

        // Convert packed single-precision (32-bit) floating point elements to
        // packed 32-bit integers
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);

        // Store result
        _mm_store_si128((__m128i*)&pCullingResult[i], intersection_res_i);
    }
#else
    return CullBox_Generic(bounds, count, result);
#endif
}

void BvFrustum::CullBox_IgnoreZ_SSE(BvAxisAlignedBoxSSE const* bounds, int count, int* result) const
{
#ifdef HK_FRUSTUM_USE_SSE
    const float* pBoundingBoxData = reinterpret_cast<const float*>(&bounds->Mins.X);
    int*         pCullingResult   = &result[0];
    int          i, j;

    __m128 zero = _mm_setzero_ps();

    // Process 4 objects per step
    for (i = 0; i < count; i += 4)
    {
        // Load bounding mins
        __m128 aabb_min_x = _mm_load_ps(pBoundingBoxData);
        __m128 aabb_min_y = _mm_load_ps(pBoundingBoxData + 8);
        __m128 aabb_min_z = _mm_load_ps(pBoundingBoxData + 16);
        __m128 aabb_min_w = _mm_load_ps(pBoundingBoxData + 24);

        // Load bounding maxs
        __m128 aabb_max_x = _mm_load_ps(pBoundingBoxData + 4);
        __m128 aabb_max_y = _mm_load_ps(pBoundingBoxData + 12);
        __m128 aabb_max_z = _mm_load_ps(pBoundingBoxData + 20);
        __m128 aabb_max_w = _mm_load_ps(pBoundingBoxData + 28);

        pBoundingBoxData += 32;

        // For now we have points in vectors aabb_min_x..w, but for calculations we need to xxxx yyyy zzzz vectors representation.
        // Just transpose data.
        _MM_TRANSPOSE4_PS(aabb_min_x, aabb_min_y, aabb_min_z, aabb_min_w);
        _MM_TRANSPOSE4_PS(aabb_max_x, aabb_max_y, aabb_max_z, aabb_max_w);

        // Set bitmask to zero
        __m128 intersection_res = _mm_setzero_ps();

        for (j = 0; j < 4; j++)
        {
            // Pick closest point to plane and check if it begind the plane. if yes - object outside frustum

            // Dot product, separate for each coordinate, for min & max aabb points
            __m128 mins_mul_plane_x = _mm_mul_ps(aabb_min_x, PlanesSSE->x[j]);
            __m128 mins_mul_plane_y = _mm_mul_ps(aabb_min_y, PlanesSSE->y[j]);
            __m128 mins_mul_plane_z = _mm_mul_ps(aabb_min_z, PlanesSSE->z[j]);

            __m128 maxs_mul_plane_x = _mm_mul_ps(aabb_max_x, PlanesSSE->x[j]);
            __m128 maxs_mul_plane_y = _mm_mul_ps(aabb_max_y, PlanesSSE->y[j]);
            __m128 maxs_mul_plane_z = _mm_mul_ps(aabb_max_z, PlanesSSE->z[j]);

            // We have 8 box points, but we need pick closest point to plane.
            __m128 res_x = _mm_max_ps(mins_mul_plane_x, maxs_mul_plane_x);
            __m128 res_y = _mm_max_ps(mins_mul_plane_y, maxs_mul_plane_y);
            __m128 res_z = _mm_max_ps(mins_mul_plane_z, maxs_mul_plane_z);

            // Distance to plane = dot( aabb_point.xyz, plane.xyz ) + plane.d
            __m128 sum_xy            = _mm_add_ps(res_x, res_y);
            __m128 sum_zw            = _mm_add_ps(res_z, PlanesSSE->d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            // Dist from closest point to plane < 0 ?
            __m128 plane_res = _mm_cmple_ps(distance_to_plane, zero);

            // If yes - aabb behind the plane & outside frustum
            intersection_res = _mm_or_ps(intersection_res, plane_res);
        }

        // Convert packed single-precision (32-bit) floating point elements to
        // packed 32-bit integers
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);

        // Store result
        _mm_store_si128((__m128i*)&pCullingResult[i], intersection_res_i);
    }
#else
    return CullBox_IgnoreZ_Generic(bounds, count, result);
#endif
}
