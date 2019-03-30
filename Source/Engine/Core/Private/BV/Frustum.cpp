#include <Engine/Core/Public/BV/Frustum.h>

void FFrustum::FromMatrix( const Float4x4 & _Matrix ) {
    float Normalizer;

    // Setup the RIGHT clipping plane
    m_Planes[FPL_RIGHT].Normal.X = _Matrix[0][3] - _Matrix[0][0];
    m_Planes[FPL_RIGHT].Normal.Y = _Matrix[1][3] - _Matrix[1][0];
    m_Planes[FPL_RIGHT].Normal.Z = _Matrix[2][3] - _Matrix[2][0];
    m_Planes[FPL_RIGHT].D =_Matrix[3][3] - _Matrix[3][0];

    // Normalize it
    Normalizer = 1.0f / m_Planes[FPL_RIGHT].Normal.Length();
    m_Planes[FPL_RIGHT].Normal *= Normalizer;
    m_Planes[FPL_RIGHT].D *= Normalizer;

    // Setup the LEFT clipping plane
    m_Planes[FPL_LEFT].Normal.X	= _Matrix[0][3] + _Matrix[0][0];
    m_Planes[FPL_LEFT].Normal.Y	= _Matrix[1][3] + _Matrix[1][0];
    m_Planes[FPL_LEFT].Normal.Z	= _Matrix[2][3] + _Matrix[2][0];
    m_Planes[FPL_LEFT].D = _Matrix[3][3] + _Matrix[3][0];

    // Normalize it
    Normalizer = 1.0f / m_Planes[FPL_LEFT].Normal.Length();
    m_Planes[FPL_LEFT].Normal *= Normalizer;
    m_Planes[FPL_LEFT].D *= Normalizer;

    // Setup the TOP clipping plane
    m_Planes[FPL_TOP].Normal.X = _Matrix[0][3] + _Matrix[0][1];
    m_Planes[FPL_TOP].Normal.Y = _Matrix[1][3] + _Matrix[1][1];
    m_Planes[FPL_TOP].Normal.Z = _Matrix[2][3] + _Matrix[2][1];
    m_Planes[FPL_TOP].D = _Matrix[3][3] + _Matrix[3][1];

    // Normalize it
    Normalizer = 1.0f / m_Planes[FPL_TOP].Normal.Length();
    m_Planes[FPL_TOP].Normal *= Normalizer;
    m_Planes[FPL_TOP].D *= Normalizer;

    // Setup the BOTTOM clipping plane
    m_Planes[FPL_BOTTOM].Normal.X = _Matrix[0][3] - _Matrix[0][1];
    m_Planes[FPL_BOTTOM].Normal.Y = _Matrix[1][3] - _Matrix[1][1];
    m_Planes[FPL_BOTTOM].Normal.Z = _Matrix[2][3] - _Matrix[2][1];
    m_Planes[FPL_BOTTOM].D = _Matrix[3][3] - _Matrix[3][1];

    // Normalize it
    Normalizer = 1.0f / m_Planes[FPL_BOTTOM].Normal.Length();
    m_Planes[FPL_BOTTOM].Normal *= Normalizer;
    m_Planes[FPL_BOTTOM].D *= Normalizer;

    // Setup the FAR clipping plane
    m_Planes[FPL_FAR].Normal.X = _Matrix[0][3] - _Matrix[0][2];
    m_Planes[FPL_FAR].Normal.Y = _Matrix[1][3] - _Matrix[1][2];
    m_Planes[FPL_FAR].Normal.Z = _Matrix[2][3] - _Matrix[2][2];
    m_Planes[FPL_FAR].D = _Matrix[3][3] - _Matrix[3][2];

    // Normalize it
    Normalizer = 1.0f / m_Planes[FPL_FAR].Normal.Length();
    m_Planes[FPL_FAR].Normal *= Normalizer;
    m_Planes[FPL_FAR].D *= Normalizer;

    // Setup the NEAR clipping plane
    m_Planes[FPL_NEAR].Normal.X = _Matrix[0][3] + _Matrix[0][2];
    m_Planes[FPL_NEAR].Normal.Y = _Matrix[1][3] + _Matrix[1][2];
    m_Planes[FPL_NEAR].Normal.Z = _Matrix[2][3] + _Matrix[2][2];
    m_Planes[FPL_NEAR].D = _Matrix[3][3] + _Matrix[3][2];

    // Normalize it
    Normalizer = 1.0f / m_Planes[FPL_NEAR].Normal.Length();
    m_Planes[FPL_NEAR].Normal *= Normalizer;
    m_Planes[FPL_NEAR].D *= Normalizer;

    UpdateSignBits();

#ifdef AN_FRUSTUM_USE_SSE
    frustum_planes_x[0] = _mm_set1_ps(m_Planes[0].Normal.X);
    frustum_planes_y[0] = _mm_set1_ps(m_Planes[0].Normal.Y);
    frustum_planes_z[0] = _mm_set1_ps(m_Planes[0].Normal.Z);
    frustum_planes_d[0] = _mm_set1_ps(m_Planes[0].D);

    frustum_planes_x[1] = _mm_set1_ps(m_Planes[1].Normal.X);
    frustum_planes_y[1] = _mm_set1_ps(m_Planes[1].Normal.Y);
    frustum_planes_z[1] = _mm_set1_ps(m_Planes[1].Normal.Z);
    frustum_planes_d[1] = _mm_set1_ps(m_Planes[1].D);

    frustum_planes_x[2] = _mm_set1_ps(m_Planes[2].Normal.X);
    frustum_planes_y[2] = _mm_set1_ps(m_Planes[2].Normal.Y);
    frustum_planes_z[2] = _mm_set1_ps(m_Planes[2].Normal.Z);
    frustum_planes_d[2] = _mm_set1_ps(m_Planes[2].D);

    frustum_planes_x[3] = _mm_set1_ps(m_Planes[3].Normal.X);
    frustum_planes_y[3] = _mm_set1_ps(m_Planes[3].Normal.Y);
    frustum_planes_z[3] = _mm_set1_ps(m_Planes[3].Normal.Z);
    frustum_planes_d[3] = _mm_set1_ps(m_Planes[3].D);

    frustum_planes_x[4] = _mm_set1_ps(m_Planes[4].Normal.X);
    frustum_planes_y[4] = _mm_set1_ps(m_Planes[4].Normal.Y);
    frustum_planes_z[4] = _mm_set1_ps(m_Planes[4].Normal.Z);
    frustum_planes_d[4] = _mm_set1_ps(m_Planes[4].D);

    frustum_planes_x[5] = _mm_set1_ps(m_Planes[5].Normal.X);
    frustum_planes_y[5] = _mm_set1_ps(m_Planes[5].Normal.Y);
    frustum_planes_z[5] = _mm_set1_ps(m_Planes[5].Normal.Z);
    frustum_planes_d[5] = _mm_set1_ps(m_Planes[5].D);
#endif
}

void FFrustum::CornerVector_TR( Float3 & _Vector ) const {
    _Vector = m_Planes[FPL_TOP].Normal.Cross( m_Planes[FPL_RIGHT].Normal ).Normalized();
}

void FFrustum::CornerVector_TL( Float3 & _Vector ) const {
    _Vector = m_Planes[FPL_LEFT].Normal.Cross( m_Planes[FPL_TOP].Normal ).Normalized();
}

void FFrustum::CornerVector_BR( Float3 & _Vector ) const {
    _Vector = m_Planes[FPL_RIGHT].Normal.Cross( m_Planes[FPL_BOTTOM].Normal ).Normalized();
}

void FFrustum::CornerVector_BL( Float3 & _Vector ) const {
    _Vector = m_Planes[FPL_BOTTOM].Normal.Cross( m_Planes[FPL_LEFT].Normal ).Normalized();
}

void FFrustum::CullSphere_Generic( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const {
    bool cull;
    const PlaneF * p;

    for ( const BvSphereSSE * Last = _Bounds + _NumObjects ; _Bounds < Last ; _Bounds++ ) {
        cull = false;
        for ( p = m_Planes ; p < m_Planes + 6 ; p++ ) {
            if ( FMath::Dot( p->Normal, _Bounds->Center ) + p->D <= -_Bounds->Radius ) {
			    cull = true;
            }
	    }
	    *_Result++ = cull;
    }
}

void FFrustum::CullSphere2_Generic( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const {
    bool cull;
    const PlaneF * p;

    for ( const BvSphereSSE * Last = _Bounds + _NumObjects ; _Bounds < Last ; _Bounds++ ) {
        cull = false;
        for ( p = m_Planes ; p < m_Planes + 4 ; p++ ) {
            if ( FMath::Dot( p->Normal, _Bounds->Center ) + p->D <= -_Bounds->Radius ) {
			    cull = true;
            }
	    }
	    *_Result++ = cull;
    }
}

void FFrustum::CullAABB_Generic( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const {
    for ( const BvAxisAlignedBoxSSE * Last = _Bounds + _NumObjects ; _Bounds < Last ; _Bounds++ ) {
        *_Result++ = !CheckAABB( _Bounds->Mins, _Bounds->Maxs );
    }
}

void FFrustum::CullAABB2_Generic( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const {
    for ( const BvAxisAlignedBoxSSE * Last = _Bounds + _NumObjects ; _Bounds < Last ; _Bounds++ ) {
        *_Result++ = !CheckAABB2( _Bounds->Mins, _Bounds->Maxs );
    }
}

void FFrustum::CullSphere_SSE( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const {
#ifdef AN_FRUSTUM_USE_SSE
    const float *sphere_data_ptr = reinterpret_cast< const float * >(&_Bounds->Center[0]);
    int *culling_res_sse = &_Result[0];

    //to optimize calculations we gather xyzw elements in separate vectors
    __m128 zero_v = _mm_setzero_ps();
    int i, j;

    //we process 4 objects per step
    for (i = 0; i < _NumObjects; i += 4)
    {
        //load bounding sphere data
        __m128 spheres_pos_x = _mm_load_ps(sphere_data_ptr);
        __m128 spheres_pos_y = _mm_load_ps(sphere_data_ptr + 4);
        __m128 spheres_pos_z = _mm_load_ps(sphere_data_ptr + 8);
        __m128 spheres_radius = _mm_load_ps(sphere_data_ptr + 12);
        sphere_data_ptr += 16;

        //but for our calculations we need transpose data, to collect x, y, z and w coordinates in separate vectors
        _MM_TRANSPOSE4_PS(spheres_pos_x, spheres_pos_y, spheres_pos_z, spheres_radius);

        __m128 spheres_neg_radius = _mm_sub_ps(zero_v, spheres_radius);  // negate all elements
        //http://fastcpp.blogspot.ru/2011/03/changing-sign-of-float-values-using-sse.html

        __m128 intersection_res = _mm_setzero_ps();

        for (j = 0; j < 6; j++) //plane index
        {
            //1. calc distance to plane dot(sphere_pos.Xyz, plane.Xyz) + plane.w
            //2. if distance < sphere radius, then sphere outside frustum
            __m128 dot_x = _mm_mul_ps(spheres_pos_x, frustum_planes_x[j]);
            __m128 dot_y = _mm_mul_ps(spheres_pos_y, frustum_planes_y[j]);
            __m128 dot_z = _mm_mul_ps(spheres_pos_z, frustum_planes_z[j]);

            __m128 sum_xy = _mm_add_ps(dot_x, dot_y);
            __m128 sum_zw = _mm_add_ps(dot_z, frustum_planes_d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            __m128 plane_res = _mm_cmple_ps(distance_to_plane, spheres_neg_radius); //dist < -sphere_r ?
            intersection_res = _mm_or_ps(intersection_res, plane_res); //if yes - sphere behind the plane & outside frustum
        }

        //store result
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
        _mm_store_si128((__m128i *)&culling_res_sse[i], intersection_res_i);
    }
#else
    return CullSphere_Generic( _Bounds, _NumObjects, _Result );
#endif
}

void FFrustum::CullSphere2_SSE( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const {
#ifdef AN_FRUSTUM_USE_SSE
    const float *sphere_data_ptr = reinterpret_cast< const float * >(&_Bounds->Center[0]);
    int *culling_res_sse = &_Result[0];

    //to optimize calculations we gather xyzw elements in separate vectors
    __m128 zero_v = _mm_setzero_ps();
    int i, j;

    //we process 4 objects per step
    for (i = 0; i < _NumObjects; i += 4)
    {
        //load bounding sphere data
        __m128 spheres_pos_x = _mm_load_ps(sphere_data_ptr);
        __m128 spheres_pos_y = _mm_load_ps(sphere_data_ptr + 4);
        __m128 spheres_pos_z = _mm_load_ps(sphere_data_ptr + 8);
        __m128 spheres_radius = _mm_load_ps(sphere_data_ptr + 12);
        sphere_data_ptr += 16;

        //but for our calculations we need transpose data, to collect x, y, z and w coordinates in separate vectors
        _MM_TRANSPOSE4_PS(spheres_pos_x, spheres_pos_y, spheres_pos_z, spheres_radius);

        __m128 spheres_neg_radius = _mm_sub_ps(zero_v, spheres_radius);  // negate all elements
        //http://fastcpp.blogspot.ru/2011/03/changing-sign-of-float-values-using-sse.html

        __m128 intersection_res = _mm_setzero_ps();

        for (j = 0; j < 4; j++) //plane index
        {
            //1. calc distance to plane dot(sphere_pos.Xyz, plane.Xyz) + plane.w
            //2. if distance < sphere radius, then sphere outside frustum
            __m128 dot_x = _mm_mul_ps(spheres_pos_x, frustum_planes_x[j]);
            __m128 dot_y = _mm_mul_ps(spheres_pos_y, frustum_planes_y[j]);
            __m128 dot_z = _mm_mul_ps(spheres_pos_z, frustum_planes_z[j]);

            __m128 sum_xy = _mm_add_ps(dot_x, dot_y);
            __m128 sum_zw = _mm_add_ps(dot_z, frustum_planes_d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            __m128 plane_res = _mm_cmple_ps(distance_to_plane, spheres_neg_radius); //dist < -sphere_r ?
            intersection_res = _mm_or_ps(intersection_res, plane_res); //if yes - sphere behind the plane & outside frustum
        }

        //store result
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
        _mm_store_si128((__m128i *)&culling_res_sse[i], intersection_res_i);
    }
#else
    return CullSphere2_Generic( _Bounds, _NumObjects, _Result );
#endif
}

void FFrustum::CullAABB_SSE( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const {
#ifdef AN_FRUSTUM_USE_SSE
    const float *aabb_data_ptr = reinterpret_cast< const float * >( &_Bounds->Mins.X );
    int *culling_res_sse = &_Result[0];

    //to optimize calculations we gather xyzw elements in separate vectors
    //__m128 zero_v = _mm_setzero_ps();
    int i, j;

    __m128 zero = _mm_setzero_ps();
    //we process 4 objects per step
    for (i = 0; i < _NumObjects; i += 4)
    {
        //load objects data
        //load aabb min
        __m128 aabb_min_x = _mm_load_ps(aabb_data_ptr);
        __m128 aabb_min_y = _mm_load_ps(aabb_data_ptr + 8);
        __m128 aabb_min_z = _mm_load_ps(aabb_data_ptr + 16);
        __m128 aabb_min_w = _mm_load_ps(aabb_data_ptr + 24);

        //load aabb max
        __m128 aabb_max_x = _mm_load_ps(aabb_data_ptr + 4);
        __m128 aabb_max_y = _mm_load_ps(aabb_data_ptr + 12);
        __m128 aabb_max_z = _mm_load_ps(aabb_data_ptr + 20);
        __m128 aabb_max_w = _mm_load_ps(aabb_data_ptr + 28);

        aabb_data_ptr += 32;

        //for now we have points in vectors aabb_min_x..w, but for calculations we need to xxxx yyyy zzzz vectors representation - just transpose data
        _MM_TRANSPOSE4_PS(aabb_min_x, aabb_min_y, aabb_min_z, aabb_min_w);
        _MM_TRANSPOSE4_PS(aabb_max_x, aabb_max_y, aabb_max_z, aabb_max_w);

        __m128 intersection_res = _mm_setzero_ps();
        for (j = 0; j < 6; j++) //plane index // TODO: uwrap
        {
            //this code is similar to what we make in simple culling
            //pick closest point to plane and check if it begind the plane. if yes - object outside frustum

            //dot product, separate for each coordinate, for min & max aabb points
            __m128 aabbMin_frustumPlane_x = _mm_mul_ps(aabb_min_x, frustum_planes_x[j]);
            __m128 aabbMin_frustumPlane_y = _mm_mul_ps(aabb_min_y, frustum_planes_y[j]);
            __m128 aabbMin_frustumPlane_z = _mm_mul_ps(aabb_min_z, frustum_planes_z[j]);

            __m128 aabbMax_frustumPlane_x = _mm_mul_ps(aabb_max_x, frustum_planes_x[j]);
            __m128 aabbMax_frustumPlane_y = _mm_mul_ps(aabb_max_y, frustum_planes_y[j]);
            __m128 aabbMax_frustumPlane_z = _mm_mul_ps(aabb_max_z, frustum_planes_z[j]);

            //we have 8 box points, but we need pick closest point to plane. Just take max
            __m128 res_x = _mm_max_ps(aabbMin_frustumPlane_x, aabbMax_frustumPlane_x);
            __m128 res_y = _mm_max_ps(aabbMin_frustumPlane_y, aabbMax_frustumPlane_y);
            __m128 res_z = _mm_max_ps(aabbMin_frustumPlane_z, aabbMax_frustumPlane_z);

            //dist to plane = dot(aabb_point.Xyz, plane.Xyz) + plane.w
            __m128 sum_xy = _mm_add_ps(res_x, res_y);
            __m128 sum_zw = _mm_add_ps(res_z, frustum_planes_d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            __m128 plane_res = _mm_cmple_ps(distance_to_plane, zero); //dist from closest point to plane < 0 ?
            intersection_res = _mm_or_ps(intersection_res, plane_res); //if yes - aabb behind the plane & outside frustum
        }

        //store result
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
        _mm_store_si128((__m128i *)&culling_res_sse[i], intersection_res_i);
    }
#else
    return CullAABB_Generic( _Bounds, _NumObjects, _Result );
#endif
}

void FFrustum::CullAABB2_SSE( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const {
#ifdef AN_FRUSTUM_USE_SSE
    const float *aabb_data_ptr = reinterpret_cast< const float * >( &_Bounds->Mins.X );
    int *culling_res_sse = &_Result[0];

    //to optimize calculations we gather xyzw elements in separate vectors
    //__m128 zero_v = _mm_setzero_ps();
    int i, j;

    __m128 zero = _mm_setzero_ps();
    //we process 4 objects per step
    for (i = 0; i < _NumObjects; i += 4)
    {
        //load objects data
        //load aabb min
        __m128 aabb_min_x = _mm_load_ps(aabb_data_ptr);
        __m128 aabb_min_y = _mm_load_ps(aabb_data_ptr + 8);
        __m128 aabb_min_z = _mm_load_ps(aabb_data_ptr + 16);
        __m128 aabb_min_w = _mm_load_ps(aabb_data_ptr + 24);

        //load aabb max
        __m128 aabb_max_x = _mm_load_ps(aabb_data_ptr + 4);
        __m128 aabb_max_y = _mm_load_ps(aabb_data_ptr + 12);
        __m128 aabb_max_z = _mm_load_ps(aabb_data_ptr + 20);
        __m128 aabb_max_w = _mm_load_ps(aabb_data_ptr + 28);

        aabb_data_ptr += 32;

        //for now we have points in vectors aabb_min_x..w, but for calculations we need to xxxx yyyy zzzz vectors representation - just transpose data
        _MM_TRANSPOSE4_PS(aabb_min_x, aabb_min_y, aabb_min_z, aabb_min_w);
        _MM_TRANSPOSE4_PS(aabb_max_x, aabb_max_y, aabb_max_z, aabb_max_w);

        __m128 intersection_res = _mm_setzero_ps();
        for (j = 0; j < 4; j++) //plane index // TODO: uwrap
        {
            //this code is similar to what we make in simple culling
            //pick closest point to plane and check if it begind the plane. if yes - object outside frustum

            //dot product, separate for each coordinate, for min & max aabb points
            __m128 aabbMin_frustumPlane_x = _mm_mul_ps(aabb_min_x, frustum_planes_x[j]);
            __m128 aabbMin_frustumPlane_y = _mm_mul_ps(aabb_min_y, frustum_planes_y[j]);
            __m128 aabbMin_frustumPlane_z = _mm_mul_ps(aabb_min_z, frustum_planes_z[j]);

            __m128 aabbMax_frustumPlane_x = _mm_mul_ps(aabb_max_x, frustum_planes_x[j]);
            __m128 aabbMax_frustumPlane_y = _mm_mul_ps(aabb_max_y, frustum_planes_y[j]);
            __m128 aabbMax_frustumPlane_z = _mm_mul_ps(aabb_max_z, frustum_planes_z[j]);

            //we have 8 box points, but we need pick closest point to plane. Just take max
            __m128 res_x = _mm_max_ps(aabbMin_frustumPlane_x, aabbMax_frustumPlane_x);
            __m128 res_y = _mm_max_ps(aabbMin_frustumPlane_y, aabbMax_frustumPlane_y);
            __m128 res_z = _mm_max_ps(aabbMin_frustumPlane_z, aabbMax_frustumPlane_z);

            //dist to plane = dot(aabb_point.Xyz, plane.Xyz) + plane.w
            __m128 sum_xy = _mm_add_ps(res_x, res_y);
            __m128 sum_zw = _mm_add_ps(res_z, frustum_planes_d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);

            __m128 plane_res = _mm_cmple_ps(distance_to_plane, zero); //dist from closest point to plane < 0 ?
            intersection_res = _mm_or_ps(intersection_res, plane_res); //if yes - aabb behind the plane & outside frustum
        }

        //store result
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
        _mm_store_si128((__m128i *)&culling_res_sse[i], intersection_res_i);
    }
#else
    return CullAABB2_Generic( _Bounds, _NumObjects, _Result );
#endif
}
