/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Runtime/Public/RenderCore.h>
#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Runtime/Public/RuntimeVariable.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

#include <Engine/Renderer/OpenGL4.5/OpenGL45RenderBackend.h>

IRenderBackend * GRenderBackend = &OpenGL45::GOpenGL45RenderBackend;

FResourceGPU * FResourceGPU::GPUResources;
FResourceGPU * FResourceGPU::GPUResourcesTail;

void IRenderBackend::RegisterGPUResource( FResourceGPU * _Resource ) {
    INTRUSIVE_ADD( _Resource, pNext, pPrev, FResourceGPU::GPUResources, FResourceGPU::GPUResourcesTail );
}

void IRenderBackend::UnregisterGPUResource( FResourceGPU * _Resource ) {
    INTRUSIVE_REMOVE( _Resource, pNext, pPrev, FResourceGPU::GPUResources, FResourceGPU::GPUResourcesTail );
}

void IRenderBackend::UploadGPUResources() {
    for ( FResourceGPU * resource = FResourceGPU::GPUResources ; resource ; resource = resource->pNext ) {
        resource->pOwner->UploadResourceGPU( resource );
    }
}


/*

Light/Probe/Decal voxelizer

*/

#include <Engine/Core/Public/Atomic.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SSE Math
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

struct Float4x4SSE {
    __m128 col0;
    __m128 col1;
    __m128 col2;
    __m128 col3;

    Float4x4SSE() {
    }

    Float4x4SSE( __m128 _col0, __m128 _col1, __m128 _col2, __m128 _col3 )
        : col0(_col0), col1(_col1), col2( _col2 ), col3( _col3 )
    {
    }

    Float4x4SSE( Float4x4 const & m ) {
        col0 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[0]) );
        col1 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[1]) );
        col2 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[2]) );
        col3 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[3]) );
    }

    AN_FORCEINLINE void operator=( Float4x4 const & m ) {
        col0 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[0]) );
        col1 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[1]) );
        col2 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[2]) );
        col3 = _mm_loadu_ps( reinterpret_cast< const float * >(&m[3]) );
    }
};

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
//
// Frustum slices
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

FFrustumSlice GFrustumSlice;

FFrustumCluster FFrustumSlice::Clusters[FFrustumSlice::NUM_CLUSTERS_Z][FFrustumSlice::NUM_CLUSTERS_Y][FFrustumSlice::NUM_CLUSTERS_X];

FFrustumSlice::FFrustumSlice() {
    ZClip[0] = 1; // extended near cluster

    for ( int sliceIndex = 1 ; sliceIndex < NUM_CLUSTERS_Z + 1 ; sliceIndex++ ) {
        //float sliceZ = ZNear * pow( ( ZFar / ZNear ), ( float )sliceIndex / NUM_CLUSTERS_Z ); // linear depth
        //ZClip[ sliceIndex ] = ( ZFar * ZNear / sliceZ - ZNear ) / ZRange; // to ndc

        ZClip[sliceIndex] = (ZFar / pow( (double)ZFar / ZNear, (double)(sliceIndex + NearOffset) / (NUM_CLUSTERS_Z + NearOffset) ) - ZNear) / ZRange; // to ndc
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Cluster items
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

#define LIGHT_ITEMS_OFFSET 0
#define DECAL_ITEMS_OFFSET 256
#define PROBE_ITEMS_OFFSET 512

struct FItemInfo {
    int MinSlice;
    int MinClusterX;
    int MinClusterY;
    int MaxSlice;
    int MaxClusterX;
    int MaxClusterY;

    // OBB before transform (повернутый OBB, который получается после умножения OBB на OBBTransformInv)
    //Float3 AabbMins;
    //Float3 AabbMaxs;
    Float4x4 ClipToBoxMat;

    // Тоже самое, только для SSE
    //AN_ALIGN_SSE __m128 AabbMinsSSE;
    //AN_ALIGN_SSE __m128 AabbMaxsSSE;
    AN_ALIGN_SSE Float4x4SSE ClipToBoxMatSSE;

    FLightDef * Light;
    //FDecalDef * Decal;
    //FEnvProbeDef * Probe;
};

#define MAX_ITEMS 32768
static FItemInfo ItemInfos[MAX_ITEMS];
static int ItemsCount;

static void PackLight( FClusterLight * _Parameters, FLightDef * _Light ) {
    _Parameters->Position = Float3(/* WorldspaceToViewspace * */ _Light->Position );
    _Parameters->OuterRadius = _Light->OuterRadius;
    _Parameters->InnerRadius = FMath::Min( _Light->InnerRadius, _Light->OuterRadius ); // TODO: do this check early
    _Parameters->Color = _Light->ColorAndAmbientIntensity;
    _Parameters->RenderMask = _Light->RenderMask;

    if ( _Light->bSpot ) {
        _Parameters->LightType = 1;

        const float ToHalfAngleRadians = 0.5f / 180.0f * FMath::_PI;

        _Parameters->OuterConeAngle = cos( _Light->OuterConeAngle * ToHalfAngleRadians );
        _Parameters->InnerConeAngle = cos( FMath::Min( _Light->InnerConeAngle, _Light->OuterConeAngle ) * ToHalfAngleRadians );

        _Parameters->SpotDirection = /*RenderFrame.NormalToView **/ (-_Light->SpotDirection);
        _Parameters->SpotExponent = _Light->SpotExponent;

    } else {
        _Parameters->LightType = 0;
    }
}

static __m128 Item_AabbMinsSSE;
static __m128 Item_AabbMaxsSSE;
static __m128 MinNDC_SSE;
static __m128 MaxNDC_SSE;
static constexpr Float3 Item_AabbMins = Float3( -1.0f );
static constexpr Float3 Item_AabbMaxs = Float3( 1.0f );

static unsigned short Items[FFrustumSlice::NUM_CLUSTERS_Z][FFrustumSlice::NUM_CLUSTERS_Y][FFrustumSlice::NUM_CLUSTERS_X][FFrameLightData::MAX_CLUSTER_ITEMS * 3]; // TODO: optimize size!!! 4 MB

static FAtomicInt ItemCounter;

static FFrameLightData FrameLightData;

FRuntimeVariable RVClusterSSE( _CTS( "ClusterSSE" ), _CTS( "1" ), VAR_CHEAT );
FRuntimeVariable RVReverseNegativeZ( _CTS( "ReverseNegativeZ" ), _CTS( "0" ), VAR_CHEAT );
FRuntimeVariable RVFixFrustumClusters( _CTS( "FixFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );

static Float4x4 ViewProj;
static Float4x4 ViewProjInv;

struct FVoxelizer {
    static void VoxelizeWork( void * _Data ) {
        int SliceIndex = (size_t)_Data;

        //GLogger.Printf( "Vox %d\n", SliceIndex );

        AN_ALIGN_SSE Float3 ClusterMins;
        AN_ALIGN_SSE Float3 ClusterMaxs;

        ClusterMins.Z = GFrustumSlice.ZClip[SliceIndex + 1];
        ClusterMaxs.Z = GFrustumSlice.ZClip[SliceIndex];

        FFrustumCluster * pCluster;
        unsigned short * pClusterItem;

        if ( RVClusterSSE ) {
            //__m128 Zero = _mm_setzero_ps();
            __m128 OutsidePosPlane;
            __m128 OutsideNegPlane;
            __m128 PointSSE;
            __m128 Outside;
            __m128i OutsideI;
            __m128 v_xxxx_min_mul_col0;
            __m128 v_xxxx_max_mul_col0;
            __m128 v_yyyy_min_mul_col1;
            __m128 v_yyyy_max_mul_col1;
            __m128 v_zzzz_min_mul_col2_add_col3;
            __m128 v_zzzz_max_mul_col2_add_col3;
            AN_ALIGN_SSE int CullingResult[4];

            for ( int ItemIndex = 0 ; ItemIndex < ItemsCount ; ItemIndex++ ) {
                FItemInfo & Info = ItemInfos[ItemIndex];

                if ( SliceIndex < Info.MinSlice || SliceIndex >= Info.MaxSlice ) {
                    continue;
                }

                v_zzzz_min_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( ClusterMins.Z ), Info.ClipToBoxMatSSE.col2 ), Info.ClipToBoxMatSSE.col3 );
                v_zzzz_max_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( ClusterMaxs.Z ), Info.ClipToBoxMatSSE.col2 ), Info.ClipToBoxMatSSE.col3 );

                pCluster = &GFrustumSlice.Clusters[SliceIndex][Info.MinClusterY][0];
                pClusterItem = &Items[SliceIndex][Info.MinClusterY][0][0];

                for ( int ClusterY = Info.MinClusterY ; ClusterY < Info.MaxClusterY ; ClusterY++ ) {

                    ClusterMins.Y = ClusterY * GFrustumSlice.DeltaY - 1.0f;
                    ClusterMaxs.Y = ClusterMins.Y + GFrustumSlice.DeltaY;

                    v_yyyy_min_mul_col1 = _mm_mul_ps( _mm_set_ps1( ClusterMins.Y ), Info.ClipToBoxMatSSE.col1 );
                    v_yyyy_max_mul_col1 = _mm_mul_ps( _mm_set_ps1( ClusterMaxs.Y ), Info.ClipToBoxMatSSE.col1 );

                    for ( int ClusterX = Info.MinClusterX ; ClusterX < Info.MaxClusterX ; ClusterX++ ) {

                        ClusterMins.X = ClusterX * GFrustumSlice.DeltaX - 1.0f;
                        ClusterMaxs.X = ClusterMins.X + GFrustumSlice.DeltaX;

                        // for debug
                        //if ( Info.Probe ) {
                        //    static int x;
                        //    x = 2;
                        //}

                        v_xxxx_min_mul_col0 = _mm_mul_ps( _mm_set_ps1( ClusterMins.X ), Info.ClipToBoxMatSSE.col0 );
                        v_xxxx_max_mul_col0 = _mm_mul_ps( _mm_set_ps1( ClusterMaxs.X ), Info.ClipToBoxMatSSE.col0 );

                        OutsidePosPlane = _mm_set1_ps( static_cast< float >(0xffffffff) );
                        OutsideNegPlane = _mm_set1_ps( static_cast< float >(0xffffffff) );

                        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_min_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_min_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_max_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_max_mul_col1, v_zzzz_max_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_min_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_min_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        PointSSE = sum_ps_3( v_xxxx_min_mul_col0, v_yyyy_max_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        PointSSE = sum_ps_3( v_xxxx_max_mul_col0, v_yyyy_max_mul_col1, v_zzzz_min_mul_col2_add_col3 );
                        PointSSE = _mm_div_ps( PointSSE, _mm_shuffle_ps( PointSSE, PointSSE, _MM_SHUFFLE( 3, 3, 3, 3 ) ) );  // Point /= Point.W
                        OutsidePosPlane = _mm_and_ps( OutsidePosPlane, _mm_cmpgt_ps( PointSSE, Item_AabbMaxsSSE ) );
                        OutsideNegPlane = _mm_and_ps( OutsideNegPlane, _mm_cmplt_ps( PointSSE, Item_AabbMinsSSE ) );

                        // combine outside
                        Outside = _mm_or_ps( OutsidePosPlane, OutsideNegPlane );

                        // store result
                        OutsideI = _mm_cvtps_epi32( Outside );
                        _mm_store_si128( (__m128i *)&CullingResult[0], OutsideI );

                        if ( CullingResult[0] != 0 || CullingResult[1] != 0 || CullingResult[2] != 0 ) {
                            // culled
                            continue;
                        }

                        if ( Info.Light ) {
                            pClusterItem[ClusterX * (FFrameLightData::MAX_CLUSTER_ITEMS * 3) + LIGHT_ITEMS_OFFSET + ((pCluster + ClusterX)->LightsCount++ & (FFrameLightData::MAX_CLUSTER_ITEMS - 1))] = ItemIndex;
                        }/* else if ( Info.Probe ) {
                         pClusterItem[ ClusterX * (MAX_CLUSTER_ITEMS * 3) + PROBE_ITEMS_OFFSET + ( ( pCluster + ClusterX )->ProbesCount++ & ( MAX_CLUSTER_ITEMS - 1 ) ) ] = ItemIndex;
                         }*/
                    }

                    pCluster += FFrustumSlice::NUM_CLUSTERS_X;
                    pClusterItem += FFrustumSlice::NUM_CLUSTERS_X * FFrameLightData::MAX_CLUSTER_ITEMS * 3;
                }
            }
        } else {
            Float4 BoxPoints[8];

            for ( int ItemIndex = 0 ; ItemIndex < ItemsCount ; ItemIndex++ ) {
                FItemInfo & Info = ItemInfos[ItemIndex];

                if ( SliceIndex < Info.MinSlice || SliceIndex >= Info.MaxSlice ) {
                    continue;
                }

                pCluster = &GFrustumSlice.Clusters[SliceIndex][Info.MinClusterY][0];
                pClusterItem = &Items[SliceIndex][Info.MinClusterY][0][0];

                for ( int ClusterY = Info.MinClusterY ; ClusterY < Info.MaxClusterY ; ClusterY++ ) {

                    ClusterMins.Y = ClusterY * GFrustumSlice.DeltaY - 1.0f;
                    ClusterMaxs.Y = ClusterMins.Y + GFrustumSlice.DeltaY;

                    for ( int ClusterX = Info.MinClusterX ; ClusterX < Info.MaxClusterX ; ClusterX++ ) {

                        ClusterMins.X = ClusterX * GFrustumSlice.DeltaX - 1.0f;
                        ClusterMaxs.X = ClusterMins.X + GFrustumSlice.DeltaX;

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
                                BoxPoints[0][i] > Item_AabbMaxs[i] &&
                                BoxPoints[1][i] > Item_AabbMaxs[i] &&
                                BoxPoints[2][i] > Item_AabbMaxs[i] &&
                                BoxPoints[3][i] > Item_AabbMaxs[i] &&
                                BoxPoints[4][i] > Item_AabbMaxs[i] &&
                                BoxPoints[5][i] > Item_AabbMaxs[i] &&
                                BoxPoints[6][i] > Item_AabbMaxs[i] &&
                                BoxPoints[7][i] > Item_AabbMaxs[i];

                            bOutsideNegPlane =
                                BoxPoints[0][i] < Item_AabbMins[i] &&
                                BoxPoints[1][i] < Item_AabbMins[i] &&
                                BoxPoints[2][i] < Item_AabbMins[i] &&
                                BoxPoints[3][i] < Item_AabbMins[i] &&
                                BoxPoints[4][i] < Item_AabbMins[i] &&
                                BoxPoints[5][i] < Item_AabbMins[i] &&
                                BoxPoints[6][i] < Item_AabbMins[i] &&
                                BoxPoints[7][i] < Item_AabbMins[i];

                            bCulled = bCulled || bOutsidePosPlane || bOutsideNegPlane;
                        }
                        if ( bCulled ) {
                            continue;
                        }
#endif
                        if ( Info.Light ) {
                            pClusterItem[ClusterX * (FFrameLightData::MAX_CLUSTER_ITEMS * 3) + LIGHT_ITEMS_OFFSET + ((pCluster + ClusterX)->LightsCount++ & (FFrameLightData::MAX_CLUSTER_ITEMS - 1))] = ItemIndex;
                        }/* else if ( Info.Probe ) {
                         pClusterItem[ ClusterX * (FFrameLightData::MAX_CLUSTER_ITEMS * 3) + PROBE_ITEMS_OFFSET + ( ( pCluster + ClusterX )->ProbesCount++ & ( FFrameLightData::MAX_CLUSTER_ITEMS - 1 ) ) ] = ItemIndex;
                         }*/
                    }

                    pCluster += FFrustumSlice::NUM_CLUSTERS_X;
                    pClusterItem += FFrustumSlice::NUM_CLUSTERS_X * FFrameLightData::MAX_CLUSTER_ITEMS * 3;
                }
            }
        }
#

        //    int i = 0;
        int NumClusterItems;

        FClusterBuffer * pBuffer = &FrameLightData.ClusterOffsetBuffer[SliceIndex][0][0];
        FClusterItemBuffer * pItem;
        FItemInfo * pItemInfo;

        pCluster = &GFrustumSlice.Clusters[SliceIndex][0][0];
        pClusterItem = &Items[SliceIndex][0][0][0];

        for ( int ClusterY = 0 ; ClusterY < FFrustumSlice::NUM_CLUSTERS_Y ; ClusterY++ ) {
            for ( int ClusterX = 0 ; ClusterX < FFrustumSlice::NUM_CLUSTERS_X ; ClusterX++ ) {

                pBuffer->NumLights = FMath::Min< unsigned short >( pCluster->LightsCount, FFrameLightData::MAX_CLUSTER_ITEMS );
                pBuffer->NumDecals = FMath::Min< unsigned short >( pCluster->DecalsCount, FFrameLightData::MAX_CLUSTER_ITEMS );
                pBuffer->NumProbes = FMath::Min< unsigned short >( pCluster->ProbesCount, FFrameLightData::MAX_CLUSTER_ITEMS );

                NumClusterItems = FMath::Max( pBuffer->NumLights, pBuffer->NumDecals, pBuffer->NumProbes );

                int ItemOffset = ItemCounter.FetchAdd( NumClusterItems );

                pBuffer->ItemOffset = ItemOffset & (FFrameLightData::MAX_ITEM_BUFFER - 1);

                pItem = &FrameLightData.ClusterItemBuffer[pBuffer->ItemOffset];

                memset( pItem, 0, NumClusterItems * sizeof( FClusterItemBuffer ) );

                for ( int t = 0 ; t < pBuffer->NumLights ; t++ ) {
                    pItemInfo = ItemInfos + pClusterItem[LIGHT_ITEMS_OFFSET + t];

                    (pItem + t)->Indices |= pItemInfo->Light->ListIndex;
                }

                //                for ( int t = 0 ; t < pBuffer->NumProbes ; t++ ) {
                //                    pItemInfo = ItemInfos + pClusterItem[ PROBE_ITEMS_OFFSET + t ];

                //                    ( pItem + t )->Indices |= pItemInfo->Probe->ListIndex << 24;
                //                }

                //for ( int t = 0 ; t < pBuffer->NumDecals ; t++ ) {
                //    i = DecalCounter.FetchIncrement() & 0x3ff;

                //    ( pItem + t )->Indices |= i << 12;

                //    Decals[ i ].Position = pCluster->Decals[ t ]->Position;
                //}

                //for ( int t = 0 ; t < pBuffer->NumProbes ; t++ ) {
                //    i = ProbeCounter.FetchIncrement() & 0xff;

                //    ( pItem + t )->Indices |= i << 24;

                //    Probes[ i ].Position = pCluster->Probes[ t ]->Position;
                //}

                pBuffer++;
                pCluster++;
                pClusterItem += FFrameLightData::MAX_CLUSTER_ITEMS * 3;
            }
        }
    }

    FVoxelizer() {
        // Uniform box
        Item_AabbMinsSSE = _mm_set_ps( 0.0f, -1.0f, -1.0f, -1.0f );
        Item_AabbMaxsSSE = _mm_set_ps( 0.0f, 1.0f, 1.0f, 1.0f );
        MinNDC_SSE = _mm_set_ps( 0.0f, -1.0f, -1.0f, -1.0f );
        MaxNDC_SSE = _mm_set_ps( 0.0f, 1.0f, 1.0f, 1.0f );
    }

    ~FVoxelizer() {

    }

    void Voxelize( FRenderFrame * Frame, FRenderView * RV ) {

        Float4x4SSE ViewProjSSE;
        Float4x4SSE ViewProjInvSSE;

        ViewProj = RV->ClusterProjectionMatrix * RV->ViewMatrix;
        ViewProjInv = ViewProj.Inversed();

        memset( GFrustumSlice.Clusters, 0, sizeof( GFrustumSlice.Clusters ) );

        int lightsCount = RV->NumLights;
        int envProbesCount = 0;//RV->NumEnvProbes;

        if ( lightsCount > FFrameLightData::MAX_LIGHTS ) {
            GLogger.Printf( "MAX_LIGHTS hit\n" );
            lightsCount = FFrameLightData::MAX_LIGHTS;
        }

        if ( envProbesCount > FFrameLightData::MAX_PROBES ) {
            GLogger.Printf( "MAX_PROBES hit\n" );
            envProbesCount = FFrameLightData::MAX_PROBES;
        }

        int ComponentListIndex[2] = { 0, 0 };

        //int NumPackedProbes = 0;
        if ( RVClusterSSE ) {
            __m128 BoxPointsSSE[8];
            //__m128 Zero = _mm_setzero_ps();
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
            AN_ALIGN_SSE Float4 bb_mins;
            AN_ALIGN_SSE Float4 bb_maxs;
            AN_ALIGN_SSE Float4 Point;
            AN_ALIGN_SSE Float3 Mins, Maxs;

            ViewProjSSE = ViewProj;
            ViewProjInvSSE = ViewProjInv;

            ItemsCount = 0;

            FClusterItem ** ClusteredLists[] = {
                (FClusterItem **)(Frame->Lights.ToPtr() + RV->FirstLight),
                //EnvProbes
            };
            const int ClusteredListsSize[] = {
                lightsCount,
                //envProbesCount
            };

            const int numLists = AN_ARRAY_SIZE( ClusteredLists );

            for ( int list = 0 ; list < numLists ; list++ ) {
                FClusterItem ** items = ClusteredLists[list];
                int itemsCount = ClusteredListsSize[list];
                int & listIndex = ComponentListIndex[list];

                for ( int itemIndex = 0; itemIndex < itemsCount ; itemIndex++ ) {

                    FClusterItem * item = items[itemIndex];

                    BvAxisAlignedBox const & AABB = item->BoundingBox;

                    FItemInfo & ItemInfo = ItemInfos[ItemsCount++];

                    if ( list == 0 ) { // Light
                        ItemInfo.Light = static_cast< FLightDef * >(item);
                        //ItemInfo.Probe = NULL;
                        item->ListIndex = listIndex++;// & ( MAX_LIGHTS - 1 );

                        PackLight( &FrameLightData.Lights[item->ListIndex], ItemInfo.Light );

                    } /*else if ( list == 1 ) { // Env Capture
                      ItemInfo.Probe = static_cast< FEnvCaptureComponent * >( item );
                      ItemInfo.Light = NULL;
                      item->ListIndex = ListIndex++;// & ( MAX_PROBES - 1 );

                      PackProbe( &Probes[ item->ListIndex ], ItemInfo.Probe );
                      //NumPackedProbes++;
                      }*/

                    Mins = AABB.Mins;
                    Maxs = AABB.Maxs;

#if 0
                    ItemInfo.AabbMinsSSE = _mm_set_ps( 0.0f, Mins.Z, Mins.Y, Mins.X );
                    ItemInfo.AabbMaxsSSE = _mm_set_ps( 0.0f, Maxs.Z, Maxs.Y, Maxs.X );

                    ItemInfo.ClipToBoxMatSSE.col0 = ViewProjInvSSE.col0;
                    ItemInfo.ClipToBoxMatSSE.col1 = ViewProjInvSSE.col1;
                    ItemInfo.ClipToBoxMatSSE.col2 = ViewProjInvSSE.col2;
                    ItemInfo.ClipToBoxMatSSE.col3 = ViewProjInvSSE.col3;
#else
                    ItemInfo.ClipToBoxMatSSE = item->OBBTransformInverse * ViewProjInv; // TODO: умножение сразу в SSE?
#endif

                                                                                             // OBB to clipspace

                    v_xxxx_min_mul_col0 = _mm_mul_ps( _mm_set_ps1( Mins.X ), ViewProjSSE.col0 );
                    v_xxxx_max_mul_col0 = _mm_mul_ps( _mm_set_ps1( Maxs.X ), ViewProjSSE.col0 );

                    v_yyyy_min_mul_col1 = _mm_mul_ps( _mm_set_ps1( Mins.Y ), ViewProjSSE.col1 );
                    v_yyyy_max_mul_col1 = _mm_mul_ps( _mm_set_ps1( Maxs.Y ), ViewProjSSE.col1 );

                    v_zzzz_min_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( Mins.Z ), ViewProjSSE.col2 ), ViewProjSSE.col3 );
                    v_zzzz_max_mul_col2_add_col3 = _mm_add_ps( _mm_mul_ps( _mm_set_ps1( Maxs.Z ), ViewProjSSE.col2 ), ViewProjSSE.col3 );

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
                    Test.set( FMath::Transpose( ViewProj ) );
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

                        //assert( !std::isnan( Point.X ) && !std::isinf( Point.X ) );
                        //assert( !std::isnan( Point.Y ) && !std::isinf( Point.Y ) );
                        //assert( !std::isnan( Point.Z ) && !std::isinf( Point.Z ) );

                        if ( Point.Z < 0.0 ) {
                            //Point.Z = 200;

                            if ( RVReverseNegativeZ ) {

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
                    bbMaxs = _mm_min_ps( bbMaxs, MaxNDC_SSE );
                    bbMaxs = _mm_max_ps( bbMaxs, MinNDC_SSE );
                    bbMins = _mm_max_ps( bbMins, MinNDC_SSE );
                    bbMins = _mm_min_ps( bbMins, MaxNDC_SSE );

                    _mm_store_ps( &bb_mins.X, bbMins );
                    _mm_store_ps( &bb_maxs.X, bbMaxs );

                    assert( bb_mins.Z >= 0 );

                    ItemInfo.MaxSlice = ceilf ( std::log2f( bb_mins.Z * GFrustumSlice.ZRange + GFrustumSlice.ZNear ) * GFrustumSlice.Scale + GFrustumSlice.Bias );
                    ItemInfo.MinSlice = floorf( std::log2f( bb_maxs.Z * GFrustumSlice.ZRange + GFrustumSlice.ZNear ) * GFrustumSlice.Scale + GFrustumSlice.Bias );

                    ItemInfo.MinClusterX = floorf( (bb_mins.X + 1.0f) * (0.5f * FFrustumSlice::NUM_CLUSTERS_X) );
                    ItemInfo.MaxClusterX = ceilf ( (bb_maxs.X + 1.0f) * (0.5f * FFrustumSlice::NUM_CLUSTERS_X) );

                    ItemInfo.MinClusterY = floorf( (bb_mins.Y + 1.0f) * (0.5f * FFrustumSlice::NUM_CLUSTERS_Y) );
                    ItemInfo.MaxClusterY = ceilf ( (bb_maxs.Y + 1.0f) * (0.5f * FFrustumSlice::NUM_CLUSTERS_Y) );

                    ItemInfo.MinSlice = FMath::Max( ItemInfo.MinSlice, 0 );
                    ItemInfo.MaxSlice = Int( ItemInfo.MaxSlice ).Clamp( 1, FFrustumSlice::NUM_CLUSTERS_Z );

                    assert( ItemInfo.MinSlice >= 0 && ItemInfo.MinSlice <= FFrustumSlice::NUM_CLUSTERS_Z );
                    assert( ItemInfo.MinClusterX >= 0 && ItemInfo.MinClusterX <= FFrustumSlice::NUM_CLUSTERS_X );
                    assert( ItemInfo.MinClusterY >= 0 && ItemInfo.MinClusterY <= FFrustumSlice::NUM_CLUSTERS_Y );
                    assert( ItemInfo.MaxClusterX >= 0 && ItemInfo.MaxClusterX <= FFrustumSlice::NUM_CLUSTERS_X );
                    assert( ItemInfo.MaxClusterY >= 0 && ItemInfo.MaxClusterY <= FFrustumSlice::NUM_CLUSTERS_Y );
                }
            }
        } else {
#if 0
            BvAxisAlignedBox bb;
            Float4 BoxPoints[8];
            Float3 Mins, Maxs;
            ItemsCount = 0;
            for ( int LightIndex = 0; LightIndex < _NumVisibleLights ; LightIndex++ ) {

                FLightComponent * Light = _VisibleLights[LightIndex];

                FItemInfo & ItemInfo = ItemInfos[ItemsCount++];

                ItemInfo.Light = Light;
                Light->ListIndex = ListIndex++ & (MAX_LIGHTS - 1);

                PackLight( &Lights[Light->ListIndex], Light );

#if 0
                ItemInfo.AabbMins = AABB.Mins;
                ItemInfo.AabbMaxs = AABB.Maxs;
                ItemInfo.ClipToBoxMat = ViewProjInv;
#else
                ItemInfo.ClipToBoxMat = Light->GetOBBTransformInverse() * ViewProjInv;
#endif

#if 0
                Mins = Float3( -1.0f );
                Maxs = Float3( 1.0f );

                Float4x4 OBBTransform = FMath::Inverse( Light->GetOBBTransformInverse() );

                BoxPoints[0] = ViewProj * OBBTransform * Float4( Mins.X, Mins.Y, Maxs.Z, 1.0f );
                BoxPoints[1] = ViewProj * OBBTransform * Float4( Maxs.X, Mins.Y, Maxs.Z, 1.0f );
                BoxPoints[2] = ViewProj * OBBTransform * Float4( Maxs.X, Maxs.Y, Maxs.Z, 1.0f );
                BoxPoints[3] = ViewProj * OBBTransform * Float4( Mins.X, Maxs.Y, Maxs.Z, 1.0f );
                BoxPoints[4] = ViewProj * OBBTransform * Float4( Maxs.X, Mins.Y, Mins.Z, 1.0f );
                BoxPoints[5] = ViewProj * OBBTransform * Float4( Mins.X, Mins.Y, Mins.Z, 1.0f );
                BoxPoints[6] = ViewProj * OBBTransform * Float4( Mins.X, Maxs.Y, Mins.Z, 1.0f );
                BoxPoints[7] = ViewProj * OBBTransform * Float4( Maxs.X, Maxs.Y, Mins.Z, 1.0f );
#else
                // This produces better culling results than code above for spot lights

                const BvAxisAlignedBox & AABB = Light->GetAABBWorldBounds();

                Mins = AABB.Mins;
                Maxs = AABB.Maxs;

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
                        if ( RVReverseNegativeZ ) {
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
                bb.Mins.X = FMath::Clamp( bb.Mins.X, -1.0f, 1.0f );
                bb.Mins.Y = FMath::Clamp( bb.Mins.Y, -1.0f, 1.0f );
                bb.Mins.Z = FMath::Clamp( bb.Mins.Z, -1.0f, 1.0f );
                bb.Maxs.X = FMath::Clamp( bb.Maxs.X, -1.0f, 1.0f );
                bb.Maxs.Y = FMath::Clamp( bb.Maxs.Y, -1.0f, 1.0f );
                bb.Maxs.Z = FMath::Clamp( bb.Maxs.Z, -1.0f, 1.0f );

                assert( bb.Mins.Z >= 0 );

                ItemInfo.MaxSlice = ceilf ( std::log2f( bb.Mins.Z * ZRange + ZNear ) * Scale + Bias );
                ItemInfo.MinSlice = floorf( std::log2f( bb.Maxs.Z * ZRange + ZNear ) * Scale + Bias );

                ItemInfo.MinClusterX = floorf( (bb.Mins.X + 1.0f) * 0.5f * NUM_CLUSTERS_X );
                ItemInfo.MaxClusterX = ceilf ( (bb.Maxs.X + 1.0f) * 0.5f * NUM_CLUSTERS_X );

                ItemInfo.MinClusterY = floorf( (bb.Mins.Y + 1.0f) * 0.5f * NUM_CLUSTERS_Y );
                ItemInfo.MaxClusterY = ceilf ( (bb.Maxs.Y + 1.0f) * 0.5f * NUM_CLUSTERS_Y );

                ItemInfo.MinSlice = FMath::Max( ItemInfo.MinSlice, 0 );
                //ItemInfo.MinClusterX = FMath::Max( ItemInfo.MinClusterX, 0 );
                //ItemInfo.MinClusterY = FMath::Max( ItemInfo.MinClusterY, 0 );

                ItemInfo.MaxSlice = FMath::Clamp( ItemInfo.MaxSlice, 1, NUM_CLUSTERS_Z );
                //ItemInfo.MaxSlice = FMath::Max( ItemInfo.MaxSlice, 1 );
                //ItemInfo.MaxClusterX = FMath::Min( ItemInfo.MaxClusterX, NUM_CLUSTERS_X );
                //ItemInfo.MaxClusterY = FMath::Min( ItemInfo.MaxClusterY, NUM_CLUSTERS_Y );

                assert( ItemInfo.MinSlice >= 0 && ItemInfo.MinSlice <= NUM_CLUSTERS_Z );
                assert( ItemInfo.MinClusterX >= 0 && ItemInfo.MinClusterX <= NUM_CLUSTERS_X );
                assert( ItemInfo.MinClusterY >= 0 && ItemInfo.MinClusterY <= NUM_CLUSTERS_Y );
                assert( ItemInfo.MaxClusterX >= 0 && ItemInfo.MaxClusterX <= NUM_CLUSTERS_X );
                assert( ItemInfo.MaxClusterY >= 0 && ItemInfo.MaxClusterY <= NUM_CLUSTERS_Y );
            }
#endif
        }

        ItemCounter.StoreRelaxed( 0 );

        for ( int i = 0 ; i < FFrustumSlice::NUM_CLUSTERS_Z ; i++ ) {
            GRenderFrontendJobList->AddJob( VoxelizeWork, reinterpret_cast< void * >(static_cast< size_t >(i)) );
        }

        GRenderFrontendJobList->SubmitAndWait();
    }

};

static FVoxelizer Voxelizer;

void Voxelize( FRenderFrame * Frame, FRenderView * RV ) {

    //GLogger.Printf( "FrameLightData %f KB\n", sizeof( FFrameLightData ) / 1024.0f );
    if ( !RVFixFrustumClusters ) {
        Voxelizer.Voxelize( Frame, RV );
    }
}
