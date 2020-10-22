/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include "LightVoxelizer.h"
#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/Runtime.h>

#define LIGHT_ITEMS_OFFSET 0
#define DECAL_ITEMS_OFFSET 256
#define PROBE_ITEMS_OFFSET 512

ARuntimeVariable com_ClusterSSE( _CTS( "com_ClusterSSE" ), _CTS( "1" ), VAR_CHEAT );
ARuntimeVariable com_ReverseNegativeZ( _CTS( "com_ReverseNegativeZ" ), _CTS( "1" ), VAR_CHEAT );
ARuntimeVariable com_FreezeFrustumClusters( _CTS( "com_FreezeFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );

ALightVoxelizer & GLightVoxelizer = ALightVoxelizer::Inst();

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SSE Math
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

AN_FORCEINLINE __m128 Float4x4SSE_MultiplyFloat4( Float4x4SSE const & m, __m128 v ) {
    __m128 xxxx = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    __m128 yyyy = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
    __m128 zzzz = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    __m128 wwww = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 3, 3, 3 ) );

    return _mm_add_ps(
        _mm_add_ps( _mm_mul_ps( xxxx, m.col0 ), _mm_mul_ps( yyyy, m.col1 ) ),
        _mm_add_ps( _mm_mul_ps( zzzz, m.col2 ), _mm_mul_ps( wwww, m.col3 ) )
    );
}

AN_FORCEINLINE __m128 Float4x4SSE_MultiplyFloat3( Float4x4SSE const & m, __m128 v ) {
    __m128 xxxx = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    __m128 yyyy = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
    __m128 zzzz = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 2, 2, 2, 2 ) );

    return _mm_add_ps(
        _mm_add_ps( _mm_mul_ps( xxxx, m.col0 ), _mm_mul_ps( yyyy, m.col1 ) ),
        _mm_add_ps( _mm_mul_ps( zzzz, m.col2 ), m.col3 )
    );
}

AN_FORCEINLINE __m128 Float4x4SSE_MultiplyFloat3( Float4x4SSE const & m, __m128 v_xxxx, __m128 v_yyyy, __m128 v_zzzz ) {
    return _mm_add_ps(
        _mm_add_ps( _mm_mul_ps( v_xxxx, m.col0 ), _mm_mul_ps( v_yyyy, m.col1 ) ),
        _mm_add_ps( _mm_mul_ps( v_zzzz, m.col2 ), m.col3 )
    );
}

AN_FORCEINLINE void Float4x4SSE_Multiply( Float4x4SSE & dest, Float4x4SSE const & m1, Float4x4SSE const & m2 ) {
    dest.col0 = Float4x4SSE_MultiplyFloat4( m1, m2.col0 );
    dest.col1 = Float4x4SSE_MultiplyFloat4( m1, m2.col1 );
    dest.col2 = Float4x4SSE_MultiplyFloat4( m1, m2.col2 );
    dest.col3 = Float4x4SSE_MultiplyFloat4( m1, m2.col3 );
}

AN_FORCEINLINE Float4x4SSE operator*( Float4x4SSE const & m1, Float4x4SSE const & m2 ) {
    return Float4x4SSE( Float4x4SSE_MultiplyFloat4( m1, m2.col0 ),
                        Float4x4SSE_MultiplyFloat4( m1, m2.col1 ),
                        Float4x4SSE_MultiplyFloat4( m1, m2.col2 ),
                        Float4x4SSE_MultiplyFloat4( m1, m2.col3 ) );
}


#define sum_ps_3( a, b, c ) _mm_add_ps( _mm_add_ps( a, b ), c )

//////////////////////////////////////////////////////////////////////////////////////////////////////

ALightVoxelizer::ALightVoxelizer() {
}

void ALightVoxelizer::TransformItemsSSE() {
    Float4x4SSE ViewProjSSE;
    Float4x4SSE ViewProjInvSSE;

    __m128 BoxPointsSSE[8];
    //__m128 Zero = _mm_setzero_ps();
    __m128 NDCMinsSSE = _mm_set_ps( 0.0f, -1.0f, -1.0f, -1.0f );
    __m128 NDCMaxsSSE = _mm_set_ps( 0.0f, 1.0f, 1.0f, 1.0f );
    __m128 ExtendNeg = _mm_set_ps( 0.0f, 0.0f, -2.0f, -2.0f );
    __m128 ExtendPos = _mm_set_ps( 0.0f, 0.0f, 4.0f, 4.0f );
    __m128 bbMins, bbMaxs;
    __m128 PointSSE;
    __m128 v_xxxx_min_mul_col0;
    __m128 v_xxxx_max_mul_col0;
    __m128 v_yyyy_min_mul_col1;
    __m128 v_yyyy_max_mul_col1;
    __m128 v_zzzz_min_mul_col2_add_col3;
    __m128 v_zzzz_max_mul_col2_add_col3;
    alignas(16) Float4 bb_mins;
    alignas(16) Float4 bb_maxs;
    alignas(16) Float4 Point;

    ViewProjSSE = ViewProj;
    ViewProjInvSSE = ViewProjInv;

    for ( int itemNum = 0 ; itemNum < ItemsCount ; itemNum++ ) {
        SItemInfo & info = ItemInfos[itemNum];

        // OBB to clipspace

        v_xxxx_min_mul_col0 = _mm_mul_ps( _mm_set_ps1( info.Mins.X ), ViewProjSSE.col0 );
        v_xxxx_max_mul_col0 = _mm_mul_ps( _mm_set_ps1( info.Maxs.X ), ViewProjSSE.col0 );

        v_yyyy_min_mul_col1 = _mm_mul_ps( _mm_set_ps1( info.Mins.Y ), ViewProjSSE.col1 );
        v_yyyy_max_mul_col1 = _mm_mul_ps( _mm_set_ps1( info.Maxs.Y ), ViewProjSSE.col1 );

        v_zzzz_min_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( info.Mins.Z ), ViewProjSSE.col2 ), ViewProjSSE.col3 );
        v_zzzz_max_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( info.Maxs.Z ), ViewProjSSE.col2 ), ViewProjSSE.col3 );

        //__m128 MinVec = _mm_set_ps( 0.0000001f, 0.0000001f, 0.0000001f, 0.0000001f );
        //BoxPointsSSE[ i ] = _mm_div_ps( PointSSE, _mm_max_ps( _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ), MinVec ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_min_mul_col1, v_zzzz_max_mul_col2_add_col3 );
        BoxPointsSSE[0] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_min_mul_col1, v_zzzz_max_mul_col2_add_col3 );
        BoxPointsSSE[1] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_max_mul_col1, v_zzzz_max_mul_col2_add_col3 );
        BoxPointsSSE[2] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_max_mul_col1, v_zzzz_max_mul_col2_add_col3 );
        BoxPointsSSE[3] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_min_mul_col1, v_zzzz_min_mul_col2_add_col3 );
        BoxPointsSSE[4] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_min_mul_col1, v_zzzz_min_mul_col2_add_col3 );
        BoxPointsSSE[5] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_max_mul_col1, v_zzzz_min_mul_col2_add_col3 );
        BoxPointsSSE[6] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_max_mul_col1, v_zzzz_min_mul_col2_add_col3 );
        BoxPointsSSE[7] = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W

#if 0
        // Alternative way to multiple matrix by vector
        Float4x4SSE Test;
        Test.set( Math::Transpose( ViewProj ) );
        for ( int c = 0 ; c < 8 ; c++ ) {
            __m128 Vec = _mm_loadu_ps( (const float*)&BoxPoints[c] );
            __m128 x = _mm_dp_ps( Test.col0, Vec, 0b11110001 );
            __m128 y = _mm_dp_ps( Test.col1, Vec, 0b11110010 );
            __m128 z = _mm_dp_ps( Test.col2, Vec, 0b11110100 );
            __m128 w = _mm_dp_ps( Test.col3, Vec, 0b11111000 );
            __m128 result = _mm_add_ps( _mm_add_ps( x, y ), _mm_add_ps( z, w ) );
            BoxPointsSSE[c] = _mm_div_ps( result, _mm_shuffle_ps( result, result, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // result /= result.W
        }
#endif

        // compute bounds

        bbMins = _mm_set_ps1( 8192.0f );
        bbMaxs = _mm_set_ps1( -8192.0f );

        for ( int i = 0; i < 8; i++ ) {

            PointSSE = BoxPointsSSE[i];

            _mm_store_ps( &Point.X, PointSSE );

            // Take care of nan received by division 0/0
            if ( std::isnan( Point.X ) ) Point.X = 1;
            if ( std::isnan( Point.Y ) ) Point.Y = 1;
            if ( std::isnan( Point.Z ) ) Point.Z = 1;

            //AN_ASSERT( !std::isnan( Point.X ) && !std::isinf( Point.X ) );
            //AN_ASSERT( !std::isnan( Point.Y ) && !std::isinf( Point.Y ) );
            //AN_ASSERT( !std::isnan( Point.Z ) && !std::isinf( Point.Z ) );

            if ( Point.Z < 0.0 ) {
                //Point.Z = 200;

                if ( com_ReverseNegativeZ ) {

                    PointSSE = _mm_set_ps( 0.0f, 200.0f, -Point.Y, -Point.X );

                    // extend bounds
                    PointSSE = _mm_add_ps( PointSSE, ExtendNeg );

                    bbMaxs = _mm_max_ps( bbMaxs, PointSSE );
                    bbMins = _mm_min_ps( bbMins, PointSSE );

                    PointSSE = _mm_add_ps( PointSSE, ExtendPos );

                    bbMaxs = _mm_max_ps( bbMaxs, PointSSE );
                    bbMins = _mm_min_ps( bbMins, PointSSE );
                } else {

                    PointSSE = _mm_set_ps( 0.0f, 200.0f, Point.Y, Point.X );

                    bbMaxs = _mm_max_ps( bbMaxs, PointSSE );
                    bbMins = _mm_min_ps( bbMins, PointSSE );
                }

            } else
            {
                PointSSE = _mm_loadu_ps( &Point.X );
                bbMaxs = _mm_max_ps( bbMaxs, PointSSE );
                bbMins = _mm_min_ps( bbMins, PointSSE );
            }
        }

        // Take care +-inf received by division w=0
        bbMaxs = _mm_min_ps( bbMaxs, NDCMaxsSSE );
        bbMaxs = _mm_max_ps( bbMaxs, NDCMinsSSE );
        bbMins = _mm_max_ps( bbMins, NDCMinsSSE );
        bbMins = _mm_min_ps( bbMins, NDCMaxsSSE );

        _mm_store_ps( &bb_mins.X, bbMins );
        _mm_store_ps( &bb_maxs.X, bbMaxs );

        AN_ASSERT( bb_mins.Z >= 0 );

        info.MaxSlice = ceilf ( std::log2f( bb_mins.Z * FRUSTUM_CLUSTER_ZRANGE + FRUSTUM_CLUSTER_ZNEAR ) * FRUSTUM_SLICE_SCALE + FRUSTUM_SLICE_BIAS );
        info.MinSlice = floorf( std::log2f( bb_maxs.Z * FRUSTUM_CLUSTER_ZRANGE + FRUSTUM_CLUSTER_ZNEAR ) * FRUSTUM_SLICE_SCALE + FRUSTUM_SLICE_BIAS );

        info.MinClusterX = floorf( (bb_mins.X + 1.0f) * (0.5f * MAX_FRUSTUM_CLUSTERS_X) );
        info.MaxClusterX = ceilf ( (bb_maxs.X + 1.0f) * (0.5f * MAX_FRUSTUM_CLUSTERS_X) );

        info.MinClusterY = floorf( (bb_mins.Y + 1.0f) * (0.5f * MAX_FRUSTUM_CLUSTERS_Y) );
        info.MaxClusterY = ceilf ( (bb_maxs.Y + 1.0f) * (0.5f * MAX_FRUSTUM_CLUSTERS_Y) );

        info.MinSlice = Math::Max( info.MinSlice, 0 );
        info.MaxSlice = Math::Clamp( info.MaxSlice, 1, MAX_FRUSTUM_CLUSTERS_Z );

        AN_ASSERT( info.MinSlice >= 0 && info.MinSlice <= MAX_FRUSTUM_CLUSTERS_Z );
        AN_ASSERT( info.MinClusterX >= 0 && info.MinClusterX <= MAX_FRUSTUM_CLUSTERS_X );
        AN_ASSERT( info.MinClusterY >= 0 && info.MinClusterY <= MAX_FRUSTUM_CLUSTERS_Y );
        AN_ASSERT( info.MaxClusterX >= 0 && info.MaxClusterX <= MAX_FRUSTUM_CLUSTERS_X );
        AN_ASSERT( info.MaxClusterY >= 0 && info.MaxClusterY <= MAX_FRUSTUM_CLUSTERS_Y );
    }
}

void ALightVoxelizer::TransformItemsGeneric() {
    BvAxisAlignedBox bb;
    Float4 BoxPoints[8];

    for ( int itemNum = 0 ; itemNum < ItemsCount ; itemNum++ ) {
        SItemInfo & info = ItemInfos[itemNum];

#if 0
        constexpr Float3 Mins( -1.0f );
        constexpr Float3 Maxs( 1.0f );

        Float4x4 OBBTransform = Math::Inverse( Light->GetOBBTransformInverse() );

        BoxPoints[0] = ViewProj * OBBTransform * Float4( Mins.X, Mins.Y, Maxs.Z, 1.0f );
        BoxPoints[1] = ViewProj * OBBTransform * Float4( Maxs.X, Mins.Y, Maxs.Z, 1.0f );
        BoxPoints[2] = ViewProj * OBBTransform * Float4( Maxs.X, Maxs.Y, Maxs.Z, 1.0f );
        BoxPoints[3] = ViewProj * OBBTransform * Float4( Mins.X, Maxs.Y, Maxs.Z, 1.0f );
        BoxPoints[4] = ViewProj * OBBTransform * Float4( Maxs.X, Mins.Y, Mins.Z, 1.0f );
        BoxPoints[5] = ViewProj * OBBTransform * Float4( Mins.X, Mins.Y, Mins.Z, 1.0f );
        BoxPoints[6] = ViewProj * OBBTransform * Float4( Mins.X, Maxs.Y, Mins.Z, 1.0f );
        BoxPoints[7] = ViewProj * OBBTransform * Float4( Maxs.X, Maxs.Y, Mins.Z, 1.0f );
#else
        // This produces a better culling results than code above for spot lights

        Float3 const & Mins = info.Mins;
        Float3 const & Maxs = info.Maxs;

        BoxPoints[0] = ViewProj * Float4( Mins.X, Mins.Y, Maxs.Z, 1.0f );
        BoxPoints[1] = ViewProj * Float4( Maxs.X, Mins.Y, Maxs.Z, 1.0f );
        BoxPoints[2] = ViewProj * Float4( Maxs.X, Maxs.Y, Maxs.Z, 1.0f );
        BoxPoints[3] = ViewProj * Float4( Mins.X, Maxs.Y, Maxs.Z, 1.0f );
        BoxPoints[4] = ViewProj * Float4( Maxs.X, Mins.Y, Mins.Z, 1.0f );
        BoxPoints[5] = ViewProj * Float4( Mins.X, Mins.Y, Mins.Z, 1.0f );
        BoxPoints[6] = ViewProj * Float4( Mins.X, Maxs.Y, Mins.Z, 1.0f );
        BoxPoints[7] = ViewProj * Float4( Maxs.X, Maxs.Y, Mins.Z, 1.0f );
#endif

        bb.Clear();

        // OBB to clipspace
        for ( int i = 0; i < 8; i++ ) {

            const float Denom = 1.0f / BoxPoints[i].W;
            Float3 & Point = *reinterpret_cast< Float3 * >(&BoxPoints[i]);
            Point.X *= Denom;
            Point.Y *= Denom;
            Point.Z *= Denom;

            if ( Point.Z < 0.0 ) {
                Point.Z = 200;
                if ( com_ReverseNegativeZ ) {
                    Point.X = -Point.X;
                    Point.Y = -Point.Y;

                    // extend bounds
                    Point.X -= 2;
                    Point.Y -= 2;
                    bb.AddPoint( Point );
                    Point.X += 4;
                    Point.Y += 4;
                    bb.AddPoint( Point );
                } else {
                    bb.AddPoint( Point );
                }

            } else {
                bb.AddPoint( Point );
            }
        }

        // Take care +-inf received by division w=0
        bb.Mins.X = Math::Clamp( bb.Mins.X, -1.0f, 1.0f );
        bb.Mins.Y = Math::Clamp( bb.Mins.Y, -1.0f, 1.0f );
        bb.Mins.Z = Math::Clamp( bb.Mins.Z, -1.0f, 1.0f );
        bb.Maxs.X = Math::Clamp( bb.Maxs.X, -1.0f, 1.0f );
        bb.Maxs.Y = Math::Clamp( bb.Maxs.Y, -1.0f, 1.0f );
        bb.Maxs.Z = Math::Clamp( bb.Maxs.Z, -1.0f, 1.0f );

        AN_ASSERT( bb.Mins.Z >= 0 );

        info.MaxSlice = ceilf ( std::log2f( bb.Mins.Z * FRUSTUM_CLUSTER_ZRANGE + FRUSTUM_CLUSTER_ZNEAR ) * FRUSTUM_SLICE_SCALE + FRUSTUM_SLICE_BIAS );
        info.MinSlice = floorf( std::log2f( bb.Maxs.Z * FRUSTUM_CLUSTER_ZRANGE + FRUSTUM_CLUSTER_ZNEAR ) * FRUSTUM_SLICE_SCALE + FRUSTUM_SLICE_BIAS );

        info.MinClusterX = floorf( (bb.Mins.X + 1.0f) * 0.5f * MAX_FRUSTUM_CLUSTERS_X );
        info.MaxClusterX = ceilf ( (bb.Maxs.X + 1.0f) * 0.5f * MAX_FRUSTUM_CLUSTERS_X );

        info.MinClusterY = floorf( (bb.Mins.Y + 1.0f) * 0.5f * MAX_FRUSTUM_CLUSTERS_Y );
        info.MaxClusterY = ceilf ( (bb.Maxs.Y + 1.0f) * 0.5f * MAX_FRUSTUM_CLUSTERS_Y );

        info.MinSlice = Math::Max( info.MinSlice, 0 );
        //info.MinClusterX = Math::Max( info.MinClusterX, 0 );
        //info.MinClusterY = Math::Max( info.MinClusterY, 0 );

        info.MaxSlice = Math::Clamp( info.MaxSlice, 1, MAX_FRUSTUM_CLUSTERS_Z );
        //info.MaxSlice = Math::Max( info.MaxSlice, 1 );
        //info.MaxClusterX = Math::Min( info.MaxClusterX, MAX_FRUSTUM_CLUSTERS_X );
        //info.MaxClusterY = Math::Min( info.MaxClusterY, MAX_FRUSTUM_CLUSTERS_Y );

        AN_ASSERT( info.MinSlice >= 0 && info.MinSlice <= MAX_FRUSTUM_CLUSTERS_Z );
        AN_ASSERT( info.MinClusterX >= 0 && info.MinClusterX <= MAX_FRUSTUM_CLUSTERS_X );
        AN_ASSERT( info.MinClusterY >= 0 && info.MinClusterY <= MAX_FRUSTUM_CLUSTERS_Y );
        AN_ASSERT( info.MaxClusterX >= 0 && info.MaxClusterX <= MAX_FRUSTUM_CLUSTERS_X );
        AN_ASSERT( info.MaxClusterY >= 0 && info.MaxClusterY <= MAX_FRUSTUM_CLUSTERS_Y );
    }
}

void ALightVoxelizer::Reset() {
    ItemsCount = 0;
    bUseSSE = com_ClusterSSE;
}

struct SWork
{
    int SliceIndex;
    ALightVoxelizer * Self;
};

void ALightVoxelizer::Voxelize( SRenderView * RV ) {
    ViewProj = RV->ClusterViewProjection;
    ViewProjInv = RV->ClusterViewProjectionInversed;

    AStreamedMemoryGPU * streamedMemory = GRuntime.GetStreamedMemoryGPU();

    // NOTE: we add MAX_CLUSTER_ITEMS*3 to resolve array overflow
    int maxItems = MAX_TOTAL_CLUSTER_ITEMS + MAX_CLUSTER_ITEMS*3;
    RV->ClusterPackedIndicesStreamHandle = streamedMemory->AllocateConstant( maxItems * sizeof( SClusterPackedIndex ), nullptr );
    RV->ClusterPackedIndices = (SClusterPackedIndex *)streamedMemory->Map( RV->ClusterPackedIndicesStreamHandle );

    pClusterHeaderData = RV->ClusterLookup;
    pClusterPackedIndices = RV->ClusterPackedIndices;

    Core::ZeroMemSSE( ClusterData, sizeof( ClusterData ) );

    // Calc min/max slices
    if ( bUseSSE ) {
        TransformItemsSSE();
    } else {
        TransformItemsGeneric();
    }

    ItemCounter.StoreRelaxed( 0 );

    SWork works[MAX_FRUSTUM_CLUSTERS_Z];

    for ( int i = 0 ; i < MAX_FRUSTUM_CLUSTERS_Z ; i++ ) {
        works[i].SliceIndex = i;
        works[i].Self = this;
        GRenderFrontendJobList->AddJob( VoxelizeWork, &works[i] );
    }

    GRenderFrontendJobList->SubmitAndWait();

    RV->ClusterPackedIndexCount = ItemCounter.Load();

    if ( RV->ClusterPackedIndexCount > MAX_TOTAL_CLUSTER_ITEMS ) {
        RV->ClusterPackedIndexCount = MAX_TOTAL_CLUSTER_ITEMS;

        GLogger.Printf( "MAX_TOTAL_CLUSTER_ITEMS hit\n" );
    }

    // Shrink ClusterItems
    streamedMemory->ShrinkLastAllocatedMemoryBlock( RV->ClusterPackedIndexCount * sizeof( SClusterPackedIndex ) );
}

void ALightVoxelizer::VoxelizeWork( void * _Data ) {
    SWork * work = static_cast< SWork * >( _Data );

    work->Self->VoxelizeWork( work->SliceIndex );
}

void ALightVoxelizer::VoxelizeWork( int SliceIndex ) {
    alignas(16) Float3 ClusterMins;
    alignas(16) Float3 ClusterMaxs;

    ClusterMins.Z = FRUSTUM_SLICE_ZCLIP[SliceIndex + 1];
    ClusterMaxs.Z = FRUSTUM_SLICE_ZCLIP[SliceIndex];

    SFrustumCluster * pCluster;
    unsigned short * pClusterItem;

    if ( bUseSSE ) {
        //__m128 Zero = _mm_setzero_ps();
        __m128 OutsidePosPlane;
        __m128 OutsideNegPlane;
        __m128 PointSSE;
        __m128 Outside;
        //__m128i OutsideI;
        __m128 v_xxxx_min_mul_col0;
        __m128 v_xxxx_max_mul_col0;
        __m128 v_yyyy_min_mul_col1;
        __m128 v_yyyy_max_mul_col1;
        __m128 v_zzzz_min_mul_col2_add_col3;
        __m128 v_zzzz_max_mul_col2_add_col3;
        //alignas(16) int CullingResult[4];
        //alignas(16) int CullingResult;
        __m128 UniformBoxMinsSSE = _mm_set_ps( 0.0f, -1.0f, -1.0f, -1.0f );
        __m128 UniformBoxMaxsSSE = _mm_set_ps( 0.0f, 1.0f, 1.0f, 1.0f );

        for ( int ItemIndex = 0 ; ItemIndex < ItemsCount ; ItemIndex++ ) {
            SItemInfo & Info = ItemInfos[ItemIndex];

            if ( SliceIndex < Info.MinSlice || SliceIndex >= Info.MaxSlice ) {
                continue;
            }

            v_zzzz_min_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( ClusterMins.Z ), Info.ClipToBoxMatSSE.col2 ), Info.ClipToBoxMatSSE.col3 );
            v_zzzz_max_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( ClusterMaxs.Z ), Info.ClipToBoxMatSSE.col2 ), Info.ClipToBoxMatSSE.col3 );

            pCluster = &ClusterData[SliceIndex][Info.MinClusterY][0];
            pClusterItem = &Items[SliceIndex][Info.MinClusterY][0][0];

            for ( int ClusterY = Info.MinClusterY ; ClusterY < Info.MaxClusterY ; ClusterY++ ) {

                ClusterMins.Y = ClusterY * FRUSTUM_CLUSTER_HEIGHT - 1.0f;
                ClusterMaxs.Y = ClusterMins.Y + FRUSTUM_CLUSTER_HEIGHT;

                v_yyyy_min_mul_col1 = _mm_mul_ps( _mm_set_ps1( ClusterMins.Y ), Info.ClipToBoxMatSSE.col1 );
                v_yyyy_max_mul_col1 = _mm_mul_ps( _mm_set_ps1( ClusterMaxs.Y ), Info.ClipToBoxMatSSE.col1 );

                for ( int ClusterX = Info.MinClusterX ; ClusterX < Info.MaxClusterX ; ClusterX++ ) {

                    ClusterMins.X = ClusterX * FRUSTUM_CLUSTER_WIDTH - 1.0f;
                    ClusterMaxs.X = ClusterMins.X + FRUSTUM_CLUSTER_WIDTH;

                    v_xxxx_min_mul_col0 = _mm_mul_ps( _mm_set_ps1( ClusterMins.X ), Info.ClipToBoxMatSSE.col0 );
                    v_xxxx_max_mul_col0 = _mm_mul_ps( _mm_set_ps1( ClusterMaxs.X ), Info.ClipToBoxMatSSE.col0 );

                    OutsidePosPlane = _mm_set1_ps( static_cast< float >(0xffffffff) );
                    OutsideNegPlane = _mm_set1_ps( static_cast< float >(0xffffffff) );

                    PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_min_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_min_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_max_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_max_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_min_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_min_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_max_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_max_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                    PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                    OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, UniformBoxMaxsSSE ) );
                    OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, UniformBoxMinsSSE ) );

                    // combine outside
                    Outside = _mm_or_ps( OutsidePosPlane, OutsideNegPlane );

#if 0
                    // store result
                    OutsideI = _mm_cvtps_epi32( Outside );
                    _mm_store_si128( (__m128i *)&CullingResult[0], OutsideI );

                    if ( CullingResult[0] != 0 || CullingResult[1] != 0 || CullingResult[2] != 0 ) {
                        // culled
                        continue;
                    }
#else
                    if ( _mm_movemask_ps( Outside ) & 0x7 ) {
                        // culled
                        continue;
                    }
#endif

                    switch ( Info.Type ) {
                    case ITEM_TYPE_LIGHT:
                        pClusterItem[ClusterX * (MAX_CLUSTER_ITEMS * 3) + LIGHT_ITEMS_OFFSET + ((pCluster + ClusterX)->LightsCount++ & (MAX_CLUSTER_ITEMS - 1))] = ItemIndex;
                        break;
                    case ITEM_TYPE_PROBE:
                        pClusterItem[ClusterX * (MAX_CLUSTER_ITEMS * 3) + PROBE_ITEMS_OFFSET + ((pCluster + ClusterX)->ProbesCount++ & (MAX_CLUSTER_ITEMS - 1))] = ItemIndex;
                        break;
                    }
                }

                pCluster += MAX_FRUSTUM_CLUSTERS_X;
                pClusterItem += MAX_FRUSTUM_CLUSTERS_X * MAX_CLUSTER_ITEMS * 3;
            }
        }
    } else {
        constexpr Float3 UniformBoxMins = Float3( -1.0f );
        constexpr Float3 UniformBoxMaxs = Float3( 1.0f );

        Float4 BoxPoints[8];

        for ( int ItemIndex = 0 ; ItemIndex < ItemsCount ; ItemIndex++ ) {
            SItemInfo & Info = ItemInfos[ItemIndex];

            if ( SliceIndex < Info.MinSlice || SliceIndex >= Info.MaxSlice ) {
                continue;
            }

            pCluster = &ClusterData[SliceIndex][Info.MinClusterY][0];
            pClusterItem = &Items[SliceIndex][Info.MinClusterY][0][0];

            for ( int ClusterY = Info.MinClusterY ; ClusterY < Info.MaxClusterY ; ClusterY++ ) {

                ClusterMins.Y = ClusterY * FRUSTUM_CLUSTER_HEIGHT - 1.0f;
                ClusterMaxs.Y = ClusterMins.Y + FRUSTUM_CLUSTER_HEIGHT;

                for ( int ClusterX = Info.MinClusterX ; ClusterX < Info.MaxClusterX ; ClusterX++ ) {

                    ClusterMins.X = ClusterX * FRUSTUM_CLUSTER_WIDTH - 1.0f;
                    ClusterMaxs.X = ClusterMins.X + FRUSTUM_CLUSTER_WIDTH;

                    BoxPoints[0] = Float4( ClusterMins.X, ClusterMins.Y, ClusterMaxs.Z, 1.0f );
                    BoxPoints[1] = Float4( ClusterMaxs.X, ClusterMins.Y, ClusterMaxs.Z, 1.0f );
                    BoxPoints[2] = Float4( ClusterMaxs.X, ClusterMaxs.Y, ClusterMaxs.Z, 1.0f );
                    BoxPoints[3] = Float4( ClusterMins.X, ClusterMaxs.Y, ClusterMaxs.Z, 1.0f );
                    BoxPoints[4] = Float4( ClusterMaxs.X, ClusterMins.Y, ClusterMins.Z, 1.0f );
                    BoxPoints[5] = Float4( ClusterMins.X, ClusterMins.Y, ClusterMins.Z, 1.0f );
                    BoxPoints[6] = Float4( ClusterMins.X, ClusterMaxs.Y, ClusterMins.Z, 1.0f );
                    BoxPoints[7] = Float4( ClusterMaxs.X, ClusterMaxs.Y, ClusterMins.Z, 1.0f );

                    // переводим вершины в спейс бокса
                    for ( int i = 0; i < 8; i++ ) {
                        BoxPoints[i] = Info.ClipToBoxMat * BoxPoints[i];
                        const float Denom = 1.0f / BoxPoints[i].W;
                        BoxPoints[i].X *= Denom;
                        BoxPoints[i].Y *= Denom;
                        BoxPoints[i].Z *= Denom;
                    }

#if 0
                    int out;
                    out = 0; for ( int i = 0; i < 8; i++ ) out += ((BoxPoints[i].X > AabbMaxs.X) ? 1 : 0); if ( out == 8 ) continue;
                    out = 0; for ( int i = 0; i < 8; i++ ) out += ((BoxPoints[i].X < AabbMins.X) ? 1 : 0); if ( out == 8 ) continue;
                    out = 0; for ( int i = 0; i < 8; i++ ) out += ((BoxPoints[i].Y > AabbMaxs.Y) ? 1 : 0); if ( out == 8 ) continue;
                    out = 0; for ( int i = 0; i < 8; i++ ) out += ((BoxPoints[i].Y < AabbMins.Y) ? 1 : 0); if ( out == 8 ) continue;
                    out = 0; for ( int i = 0; i < 8; i++ ) out += ((BoxPoints[i].Z > AabbMaxs.Z) ? 1 : 0); if ( out == 8 ) continue;
                    out = 0; for ( int i = 0; i < 8; i++ ) out += ((BoxPoints[i].Z < AabbMins.Z) ? 1 : 0); if ( out == 8 ) continue;
#else
                    bool bCulled = false, bOutsidePosPlane, bOutsideNegPlane;

                    // имеем 6 плоскостей отсечения, 3 потому что тестируем 2 плоскости за раз (+1 и -1)
                    // находятся ли все 8 точек за плоскостью
                    for ( int i = 0; i < 3; i++ ) {
                        bOutsidePosPlane =
                                BoxPoints[0][i] > UniformBoxMaxs[i] &&
                                BoxPoints[1][i] > UniformBoxMaxs[i] &&
                                BoxPoints[2][i] > UniformBoxMaxs[i] &&
                                BoxPoints[3][i] > UniformBoxMaxs[i] &&
                                BoxPoints[4][i] > UniformBoxMaxs[i] &&
                                BoxPoints[5][i] > UniformBoxMaxs[i] &&
                                BoxPoints[6][i] > UniformBoxMaxs[i] &&
                                BoxPoints[7][i] > UniformBoxMaxs[i];

                        bOutsideNegPlane =
                                BoxPoints[0][i] < UniformBoxMins[i] &&
                                BoxPoints[1][i] < UniformBoxMins[i] &&
                                BoxPoints[2][i] < UniformBoxMins[i] &&
                                BoxPoints[3][i] < UniformBoxMins[i] &&
                                BoxPoints[4][i] < UniformBoxMins[i] &&
                                BoxPoints[5][i] < UniformBoxMins[i] &&
                                BoxPoints[6][i] < UniformBoxMins[i] &&
                                BoxPoints[7][i] < UniformBoxMins[i];

                        bCulled = bCulled || bOutsidePosPlane || bOutsideNegPlane;
                    }
                    if ( bCulled ) {
                        continue;
                    }
#endif
                    switch ( Info.Type ) {
                    case ITEM_TYPE_LIGHT:
                        pClusterItem[ClusterX * (MAX_CLUSTER_ITEMS * 3) + LIGHT_ITEMS_OFFSET + ((pCluster + ClusterX)->LightsCount++ & (MAX_CLUSTER_ITEMS - 1))] = ItemIndex;
                        break;
                    case ITEM_TYPE_PROBE:
                        pClusterItem[ClusterX * (MAX_CLUSTER_ITEMS * 3) + PROBE_ITEMS_OFFSET + ((pCluster + ClusterX)->ProbesCount++ & (MAX_CLUSTER_ITEMS - 1))] = ItemIndex;
                        break;
                    }
                }

                pCluster += MAX_FRUSTUM_CLUSTERS_X;
                pClusterItem += MAX_FRUSTUM_CLUSTERS_X * MAX_CLUSTER_ITEMS * 3;
            }
        }
    }

    //    int i = 0;
    int NumClusterItems;

    SClusterHeader * pClusterHeader = pClusterHeaderData + SliceIndex * ( MAX_FRUSTUM_CLUSTERS_X * MAX_FRUSTUM_CLUSTERS_Y );
    SClusterPackedIndex * pItem;
    SItemInfo * pItemInfo;

    pCluster = &ClusterData[SliceIndex][0][0];
    pClusterItem = &Items[SliceIndex][0][0][0];

    for ( int ClusterY = 0 ; ClusterY < MAX_FRUSTUM_CLUSTERS_Y ; ClusterY++ ) {
        for ( int ClusterX = 0 ; ClusterX < MAX_FRUSTUM_CLUSTERS_X ; ClusterX++ ) {

            pClusterHeader->NumLights = Math::Min< unsigned short >( pCluster->LightsCount, MAX_CLUSTER_ITEMS );
            pClusterHeader->NumDecals = Math::Min< unsigned short >( pCluster->DecalsCount, MAX_CLUSTER_ITEMS );
            pClusterHeader->NumProbes = Math::Min< unsigned short >( pCluster->ProbesCount, MAX_CLUSTER_ITEMS );

            NumClusterItems = Math::Max3( pClusterHeader->NumLights, pClusterHeader->NumDecals, pClusterHeader->NumProbes );

            int firstPackedIndex = ItemCounter.FetchAdd( NumClusterItems );

            pClusterHeader->FirstPackedIndex = firstPackedIndex & (MAX_TOTAL_CLUSTER_ITEMS - 1);

            pItem = &pClusterPackedIndices[pClusterHeader->FirstPackedIndex];

            Core::ZeroMem( pItem, NumClusterItems * sizeof( SClusterPackedIndex ) );

            for ( int t = 0 ; t < pClusterHeader->NumLights ; t++ ) {
                pItemInfo = ItemInfos + pClusterItem[LIGHT_ITEMS_OFFSET + t];

                (pItem + t)->Indices |= pItemInfo->ListIndex;
            }

            for ( int t = 0 ; t < pClusterHeader->NumProbes ; t++ ) {
                pItemInfo = ItemInfos + pClusterItem[PROBE_ITEMS_OFFSET + t];

                (pItem + t)->Indices |= pItemInfo->ListIndex << 24;
            }

            //for ( int t = 0 ; t < pClusterHeader->NumDecals ; t++ ) {
            //    i = DecalCounter.FetchIncrement() & 0x3ff;

            //    ( pItem + t )->Indices |= i << 12;

            //    Decals[ i ].Position = pCluster->Decals[ t ]->Position;
            //}

            //for ( int t = 0 ; t < pClusterHeader->NumProbes ; t++ ) {
            //    i = ProbeCounter.FetchIncrement() & 0xff;

            //    ( pItem + t )->Indices |= i << 24;

            //    Probes[ i ].Position = pCluster->Probes[ t ]->Position;
            //}

            pClusterHeader++;
            pCluster++;
            pClusterItem += MAX_CLUSTER_ITEMS * 3;
        }
    }
}

void ALightVoxelizer::GatherVoxelGeometry( TStdVectorDefault< Float3 > & LinePoints, Float4x4 const & ViewProjectionInversed )
{
    Float3 clusterMins;
    Float3 clusterMaxs;
    Float4 p[ 8 ];
    Float3 * lineP;

    LinePoints.clear();

    for ( int sliceIndex = 0 ; sliceIndex < MAX_FRUSTUM_CLUSTERS_Z ; sliceIndex++ ) {

        clusterMins.Z = FRUSTUM_SLICE_ZCLIP[ sliceIndex + 1 ];
        clusterMaxs.Z = FRUSTUM_SLICE_ZCLIP[ sliceIndex ];

        for ( int clusterY = 0 ; clusterY < MAX_FRUSTUM_CLUSTERS_Y ; clusterY++ ) {

            clusterMins.Y = clusterY * FRUSTUM_CLUSTER_HEIGHT - 1.0f;
            clusterMaxs.Y = clusterMins.Y + FRUSTUM_CLUSTER_HEIGHT;

            for ( int clusterX = 0 ; clusterX < MAX_FRUSTUM_CLUSTERS_X ; clusterX++ ) {

                clusterMins.X = clusterX * FRUSTUM_CLUSTER_WIDTH - 1.0f;
                clusterMaxs.X = clusterMins.X + FRUSTUM_CLUSTER_WIDTH;

                if (   ClusterData[ sliceIndex ][ clusterY ][ clusterX ].LightsCount > 0
                    || ClusterData[ sliceIndex ][ clusterY ][ clusterX ].DecalsCount > 0
                    || ClusterData[ sliceIndex ][ clusterY ][ clusterX ].ProbesCount > 0 ) {
                    p[ 0 ] = Float4( clusterMins.X, clusterMins.Y, clusterMins.Z, 1.0f );
                    p[ 1 ] = Float4( clusterMaxs.X, clusterMins.Y, clusterMins.Z, 1.0f );
                    p[ 2 ] = Float4( clusterMaxs.X, clusterMaxs.Y, clusterMins.Z, 1.0f );
                    p[ 3 ] = Float4( clusterMins.X, clusterMaxs.Y, clusterMins.Z, 1.0f );
                    p[ 4 ] = Float4( clusterMaxs.X, clusterMins.Y, clusterMaxs.Z, 1.0f );
                    p[ 5 ] = Float4( clusterMins.X, clusterMins.Y, clusterMaxs.Z, 1.0f );
                    p[ 6 ] = Float4( clusterMins.X, clusterMaxs.Y, clusterMaxs.Z, 1.0f );
                    p[ 7 ] = Float4( clusterMaxs.X, clusterMaxs.Y, clusterMaxs.Z, 1.0f );
                    LinePoints.resize( LinePoints.size() + 8 );
                    lineP = LinePoints.data() + LinePoints.size() - 8;
                    for ( int i = 0 ; i < 8 ; i++ ) {
                        p[ i ] = ViewProjectionInversed * p[ i ];
                        const float Denom = 1.0f / p[ i ].W;
                        lineP[ i ].X = p[ i ].X * Denom;
                        lineP[ i ].Y = p[ i ].Y * Denom;
                        lineP[ i ].Z = p[ i ].Z * Denom;
                    }
                }
            }
        }
    }
}

void ALightVoxelizer::DrawVoxels( ADebugRenderer * InRenderer )
{
    static TStdVectorDefault< Float3 > LinePoints;

    if ( !com_FreezeFrustumClusters )
    {
        SRenderView const * view = InRenderer->GetRenderView();

        const Float4x4 viewProjInv = ( view->ClusterProjectionMatrix * view->ViewMatrix ).Inversed();
        // TODO: try to optimize with ViewMatrix.ViewInverseFast() * ProjectionMatrix.ProjectionInverseFast()

        GatherVoxelGeometry( LinePoints, viewProjInv );
    }

    if ( bUseSSE )
        InRenderer->SetColor( AColor4( 0, 0, 1 ) );
    else
        InRenderer->SetColor( AColor4( 1, 0, 0 ) );

    int n = 0;
    for ( Float3 * lineP = LinePoints.data() ; n < LinePoints.size() ; lineP += 8, n += 8 )
    {
        InRenderer->DrawLine( lineP, 4, true );
        InRenderer->DrawLine( lineP + 4, 4, true );
        InRenderer->DrawLine( lineP[ 0 ], lineP[ 5 ] );
        InRenderer->DrawLine( lineP[ 1 ], lineP[ 4 ] );
        InRenderer->DrawLine( lineP[ 2 ], lineP[ 7 ] );
        InRenderer->DrawLine( lineP[ 3 ], lineP[ 6 ] );
    }
}
