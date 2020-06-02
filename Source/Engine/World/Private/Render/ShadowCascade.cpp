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

#include <World/Public/Render/RenderFrontend.h>

#include <Runtime/Public/RuntimeVariable.h>

ARuntimeVariable RVShadowCascadeBits( _CTS( "ShadowCascadeBits" ), _CTS( "24" ) );    // Allowed 16, 24 or 32 bits
ARuntimeVariable RVCascadeSplitLambda( _CTS( "CascadeSplitLambda" ), _CTS( "1.0" ) );
ARuntimeVariable RVMaxShadowDistance( _CTS( "MaxShadowDistance" ), _CTS( "128" ) );
ARuntimeVariable RVShadowCalc( _CTS( "ShadowCalc" ), _CTS( "0" ) );

static constexpr int MAX_CASCADE_SPLITS = MAX_SHADOW_CASCADES + 1;

static constexpr Float4x4 ShadowMapBias = Float4x4(
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f,-0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f
);


//
// Temporary variables
//
struct SCascadeSplit { // TODO: move to DirectionalLightComponent.h
    float Distance;     // Distance from view
};
static SCascadeSplit CascadeSplits[ MAX_CASCADE_SPLITS ];   // TODO: move inside ADirectionalLightComponent?
alignas(16) static Float4 LightSpaceVerts[ MAX_CASCADE_SPLITS ][ 4 ];    // Split corners in light space
alignas(16) static Float4 WorldSpaceVerts[MAX_CASCADE_SPLITS][4];    // Split corners in world space
//static BvAxisAlignedBox CascadeBounds[ MAX_SHADOW_CASCADES ];     // Cascade bounding boxes
static float PerspHalfWidth;
static float PerspHalfHeight;
static Float3 RV, UV;           // scaled right and up vectors in world space

static void CalcCascades( SRenderView * View, SDirectionalLightDef & _LightDef, SCascadeSplit * _CascadeSplits );

void ARenderFrontend::CreateDirectionalLightCascades( SRenderView * View ) {
    View->NumShadowMapCascades = 0;
    View->NumCascadedShadowMaps = 0;

    if ( View->bPerspective ) {
        PerspHalfWidth = std::tan( View->ViewFovX * 0.5f );
        PerspHalfHeight = std::tan( View->ViewFovY * 0.5f );
    } else {
        float orthoWidth = View->ViewOrthoMaxs.X - View->ViewOrthoMins.X;
        float orthoHeight = View->ViewOrthoMaxs.Y - View->ViewOrthoMins.Y;
        RV = View->ViewRightVec * Math::Abs( orthoWidth * 0.5f );
        UV = View->ViewUpVec * Math::Abs( orthoHeight * 0.5f );
    }

    for ( int lightIndex = 0 ; lightIndex < View->NumDirectionalLights ; lightIndex++ ) {
        SDirectionalLightDef & lightDef = *FrameData.DirectionalLights[ View->FirstDirectionalLight + lightIndex];

        if ( !lightDef.bCastShadow ) {
            continue;
        }

        // NOTE: We can use View->ViewZFar/MaxVisibleDistance to improve quality (in case if ViewZFar/MaxVisibleDistance is small) or set it in light properties
        const float MaxShadowDistance = RVMaxShadowDistance.GetInteger();

        // Calc splits

        const float a = MaxShadowDistance / View->ViewZNear;
        const float b = MaxShadowDistance - View->ViewZNear;

        CascadeSplits[0].Distance = View->ViewZNear;
        CascadeSplits[MAX_CASCADE_SPLITS-1].Distance = MaxShadowDistance;

        for ( int splitIndex = 1 ; splitIndex < MAX_CASCADE_SPLITS-1 ; splitIndex++ ) {
            const float factor = (float)splitIndex / (MAX_CASCADE_SPLITS-1);
            const float logarithmic = View->ViewZNear * Math::Pow( a, factor );
            const float linear = View->ViewZNear + b * factor;
            const float dist = Math::Lerp( linear, logarithmic, RVCascadeSplitLambda.GetFloat() );
            CascadeSplits[splitIndex].Distance = dist;
        }

        CalcCascades( View, lightDef, CascadeSplits );

        if ( lightDef.NumCascades > 0 ) {
            View->NumCascadedShadowMaps++;  // Just statistics
        }
    }

    FrameData.ShadowCascadePoolSize = Math::Max( FrameData.ShadowCascadePoolSize, View->NumShadowMapCascades );
}

static void CalcCascades( SRenderView * View, SDirectionalLightDef & _LightDef, SCascadeSplit * _CascadeSplits ) {
    int NumSplits = _LightDef.MaxShadowCascades + 1;
    int NumVisibleSplits;
    int NumVisibleCascades;
    Float3 CenterWorldspace; // center of split in world space
    Float4x4 CascadeProjectionMatrix;
    Float4x4 LightViewMatrix[MAX_SHADOW_CASCADES];

    AN_ASSERT( _LightDef.MaxShadowCascades > 0 );
    AN_ASSERT( _LightDef.MaxShadowCascades <= MAX_SHADOW_CASCADES );

    const float LightDist = 400.0f;

    Float3   LightPos = View->ViewPosition + _LightDef.Matrix[2] * LightDist;
    Float3x3 Basis = _LightDef.Matrix.Transposed();
    Float3   Origin = Basis * (-LightPos);

    LightViewMatrix[0][0] = Float4( Basis[0], 0.0f );
    LightViewMatrix[0][1] = Float4( Basis[1], 0.0f );
    LightViewMatrix[0][2] = Float4( Basis[2], 0.0f );
    LightViewMatrix[0][3] = Float4( Origin, 1.0f );

    // FIXME: Возможно лучше для каждого каскада позиционировать источник освещения индивидуально?

    const float MaxShadowcastDistance = View->MaxVisibleDistance; // TODO: Calc max shadow caster distance to camera

    for ( NumVisibleSplits = 0 ;
          NumVisibleSplits < NumSplits && ( _CascadeSplits[ NumVisibleSplits ].Distance <= MaxShadowcastDistance ) ;
          NumVisibleSplits++ )
    {
        const float & SplitDistance = CascadeSplits[ NumVisibleSplits ].Distance;

        if ( View->bPerspective ) {
            // for perspective camera
            RV = View->ViewRightVec * ( PerspHalfWidth * SplitDistance );
            UV = View->ViewUpVec * ( PerspHalfHeight * SplitDistance );
        }

        CenterWorldspace = View->ViewPosition + View->ViewDir * SplitDistance;

        Float4 * pWorldSpaceVerts = WorldSpaceVerts[NumVisibleSplits];
        Float4 * pLightSpaceVerts = LightSpaceVerts[NumVisibleSplits];

        pWorldSpaceVerts[0] = Float4( CenterWorldspace - RV - UV, 1.0f );
        pWorldSpaceVerts[1] = Float4( CenterWorldspace - RV + UV, 1.0f );
        pWorldSpaceVerts[2] = Float4( CenterWorldspace + RV + UV, 1.0f );
        pWorldSpaceVerts[3] = Float4( CenterWorldspace + RV - UV, 1.0f );

        pLightSpaceVerts[0] = LightViewMatrix[0] * pWorldSpaceVerts[0];
        pLightSpaceVerts[1] = LightViewMatrix[0] * pWorldSpaceVerts[1];
        pLightSpaceVerts[2] = LightViewMatrix[0] * pWorldSpaceVerts[2];
        pLightSpaceVerts[3] = LightViewMatrix[0] * pWorldSpaceVerts[3];
    }

    AN_ASSERT( NumVisibleSplits >= 2 );

    NumVisibleCascades = NumVisibleSplits - 1;

    _LightDef.FirstCascade = View->NumShadowMapCascades;
    _LightDef.NumCascades = NumVisibleCascades;

    const float Extrusion = 0.0f;

    alignas(16) float cascadeMins[4];
    alignas(16) float cascadeMaxs[4];
    alignas(16) Float4 lightSpaceVerts[8];

    if ( RVShadowCalc.GetInteger() == 0 ) {
        for ( int CascadeIndex = 0 ; CascadeIndex < NumVisibleCascades ; CascadeIndex++ ) {
            //Float3 & Mins = CascadeBounds[ CascadeIndex ].Mins;
            //Float3 & Maxs = CascadeBounds[ CascadeIndex ].Maxs;

            const Float4 * pVerts = &LightSpaceVerts[CascadeIndex][0];

#if 1
            __m128 mins, maxs, vert;

            mins = maxs = _mm_load_ps( &pVerts->X );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            _mm_store_ps( cascadeMins, mins );
            _mm_store_ps( cascadeMaxs, maxs );

#else
            Mins = Maxs = Float3( *pVerts );

            ++pVerts;

            vert = _mm_set_ps( pVerts->X, pVerts->Y, pVerts->Z, 0.0f );

            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            Mins.X = Math::Min( Mins.X, pVerts->X );
            Mins.Y = Math::Min( Mins.Y, pVerts->Y );
            Mins.Z = Math::Min( Mins.Z, pVerts->Z );

            Maxs.X = Math::Max( Maxs.X, pVerts->X );
            Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
            Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

            ++pVerts;

            Mins.X = Math::Min( Mins.X, pVerts->X );
            Mins.Y = Math::Min( Mins.Y, pVerts->Y );
            Mins.Z = Math::Min( Mins.Z, pVerts->Z );

            Maxs.X = Math::Max( Maxs.X, pVerts->X );
            Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
            Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

            ++pVerts;

            Mins.X = Math::Min( Mins.X, pVerts->X );
            Mins.Y = Math::Min( Mins.Y, pVerts->Y );
            Mins.Z = Math::Min( Mins.Z, pVerts->Z );

            Maxs.X = Math::Max( Maxs.X, pVerts->X );
            Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
            Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

            ++pVerts;

            Mins.X = Math::Min( Mins.X, pVerts->X );
            Mins.Y = Math::Min( Mins.Y, pVerts->Y );
            Mins.Z = Math::Min( Mins.Z, pVerts->Z );

            Maxs.X = Math::Max( Maxs.X, pVerts->X );
            Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
            Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

            ++pVerts;

            Mins.X = Math::Min( Mins.X, pVerts->X );
            Mins.Y = Math::Min( Mins.Y, pVerts->Y );
            Mins.Z = Math::Min( Mins.Z, pVerts->Z );

            Maxs.X = Math::Max( Maxs.X, pVerts->X );
            Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
            Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

            ++pVerts;

            Mins.X = Math::Min( Mins.X, pVerts->X );
            Mins.Y = Math::Min( Mins.Y, pVerts->Y );
            Mins.Z = Math::Min( Mins.Z, pVerts->Z );

            Maxs.X = Math::Max( Maxs.X, pVerts->X );
            Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
            Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

            ++pVerts;

            Mins.X = Math::Min( Mins.X, pVerts->X );
            Mins.Y = Math::Min( Mins.Y, pVerts->Y );
            Mins.Z = Math::Min( Mins.Z, pVerts->Z );

            Maxs.X = Math::Max( Maxs.X, pVerts->X );
            Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
            Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

#endif

            cascadeMins[0] -= Extrusion;
            cascadeMins[1] -= Extrusion;
            cascadeMins[2] -= Extrusion;
            cascadeMaxs[0] += Extrusion;
            cascadeMaxs[1] += Extrusion;
            cascadeMaxs[2] += Extrusion;

            // Snap to minimize jittering
            //cascadeMins[0] = Math::Floor( cascadeMins[0] * 0.5f ) * 2.0f;
            //cascadeMins[1] = Math::Floor( cascadeMins[1] * 0.5f ) * 2.0f;
            //cascadeMins[2] = Math::Floor( cascadeMins[2] * 0.5f ) * 2.0f;
            //cascadeMaxs[0] = Math::Ceil( cascadeMaxs[0] * 0.5f ) * 2.0f;
            //cascadeMaxs[1] = Math::Ceil( cascadeMaxs[1] * 0.5f ) * 2.0f;
            //cascadeMaxs[2] = Math::Ceil( cascadeMaxs[2] * 0.5f ) * 2.0f;

            //Mins.X = floorf( Mins.X * 0.1f ) * 10.0f;
            //Mins.Y = floorf( Mins.Y * 0.1f ) * 10.0f;
            //Mins.Z = floorf( Mins.Z * 0.1f ) * 10.0f;
            //Maxs.X = ceilf( Maxs.X * 0.1f ) * 10.0f;
            //Maxs.Y = ceilf( Maxs.Y * 0.1f ) * 10.0f;
            //Maxs.Z = ceilf( Maxs.Z * 0.1f ) * 10.0f;

            //float x = Mins.X;
            //float y = Mins.Y;
            //float z = Mins.Z;
            //Mins.X = floorf( Mins.X * 0.5f ) * 2.0f;
            //Mins.Y = floorf( Mins.Y * 0.5f ) * 2.0f;
            //Mins.Z = floorf( Mins.Z * 0.5f ) * 2.0f;
            //Maxs.X -= x - Mins.X;
            //Maxs.Y -= y - Mins.Y;
            //Maxs.Z -= z - Mins.Z;

            //CascadeProjectionMatrix = Math::OrthoCC( Mins, Maxs );

            if ( CascadeIndex == NumVisibleCascades - 1 ) {
                // Last cascade have extended far plane
                CascadeProjectionMatrix = Float4x4::OrthoCC( Float2( cascadeMins[0], cascadeMins[1] ), Float2( cascadeMaxs[0], cascadeMaxs[1] ), 0.1f, 5000.0f );
            } else {

                float Z = cascadeMaxs[2] - cascadeMins[2];

                if ( 1 ) {

                    float ExtendedZ;

                    if ( RVShadowCascadeBits.GetInteger() <= 16 ) {
                        ExtendedZ = Z + LightDist + Z * 2.0f;
                    } else {
                        ExtendedZ = 5000.0f;
                    }

                    CascadeProjectionMatrix = Float4x4::OrthoCC( Float2( cascadeMins[0], cascadeMins[1] ), Float2( cascadeMaxs[0], cascadeMaxs[1] ), 0.1f, ExtendedZ );

                } else {
                    CascadeProjectionMatrix = Float4x4::OrthoCC( Float2( cascadeMins[0], cascadeMins[1] ), Float2( cascadeMaxs[0], cascadeMaxs[1] ), 0.1f, Z + LightDist );
                }
            }

            View->LightViewProjectionMatrices[View->NumShadowMapCascades] = CascadeProjectionMatrix * LightViewMatrix[0];
            View->ShadowMapMatrices[View->NumShadowMapCascades] = ShadowMapBias * View->LightViewProjectionMatrices[View->NumShadowMapCascades] * View->ClipSpaceToWorldSpace;

            View->NumShadowMapCascades++;
        }
    } else {
        for ( int CascadeIndex = 0 ; CascadeIndex < NumVisibleCascades ; CascadeIndex++ ) {
            //Float3 & Mins = CascadeBounds[ CascadeIndex ].Mins;
            //Float3 & Maxs = CascadeBounds[ CascadeIndex ].Maxs;

            const Float4 * pVerts = &WorldSpaceVerts[CascadeIndex][0];

            __m128 mins, maxs, vert;

            mins = maxs = _mm_load_ps( &pVerts->X );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            _mm_store_ps( cascadeMins, mins );
            _mm_store_ps( cascadeMaxs, maxs );

            cascadeMins[0] -= Extrusion;
            cascadeMins[1] -= Extrusion;
            cascadeMins[2] -= Extrusion;
            cascadeMaxs[0] += Extrusion;
            cascadeMaxs[1] += Extrusion;
            cascadeMaxs[2] += Extrusion;

            // Snap to minimize jittering
            //cascadeMins[0] = Math::Floor( cascadeMins[0] * 0.5f ) * 2.0f;
            //cascadeMins[1] = Math::Floor( cascadeMins[1] * 0.5f ) * 2.0f;
            //cascadeMins[2] = Math::Floor( cascadeMins[2] * 0.5f ) * 2.0f;
            //cascadeMaxs[0] = Math::Ceil( cascadeMaxs[0] * 0.5f ) * 2.0f;
            //cascadeMaxs[1] = Math::Ceil( cascadeMaxs[1] * 0.5f ) * 2.0f;
            //cascadeMaxs[2] = Math::Ceil( cascadeMaxs[2] * 0.5f ) * 2.0f;


            //Float3 viewDir = m_camera.getOrientation().z_direction();
            Float3 size( cascadeMaxs[0] - cascadeMins[0],
                         cascadeMaxs[1] - cascadeMins[1],
                         cascadeMaxs[2] - cascadeMins[2] );
            Float3 boxCenter( ( cascadeMins[0] + cascadeMaxs[0] ) * 0.5f,
                              ( cascadeMins[1] + cascadeMaxs[1] ) * 0.5f,
                              ( cascadeMins[2] + cascadeMaxs[2] ) * 0.5f );

            Float3 center = boxCenter;// - viewDir * m_splitShift;
            center.Y = 0;

            LightPos = center + _LightDef.Matrix[2] * LightDist;
            Basis = _LightDef.Matrix.Transposed();
            Origin = Basis * (-LightPos);

            LightViewMatrix[CascadeIndex][0] = Float4( Basis[0], 0.0f );
            LightViewMatrix[CascadeIndex][1] = Float4( Basis[1], 0.0f );
            LightViewMatrix[CascadeIndex][2] = Float4( Basis[2], 0.0f );
            LightViewMatrix[CascadeIndex][3] = Float4( Origin, 1.0f );

            // Transform points to light space
            pVerts = &WorldSpaceVerts[CascadeIndex][0];
            for ( int i = 0 ; i < 8 ; i++ ) {
                lightSpaceVerts[i] = LightViewMatrix[CascadeIndex] * pVerts[i];
            }

            pVerts = lightSpaceVerts;

            // Calc min/max in light space
            mins = maxs = _mm_load_ps( &pVerts->X );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            ++pVerts;

            vert = _mm_load_ps( &pVerts->X );
            mins = _mm_min_ps( mins, vert );
            maxs = _mm_max_ps( maxs, vert );

            _mm_store_ps( cascadeMins, mins );
            _mm_store_ps( cascadeMaxs, maxs );

            //float d = Math::Max( size.X, size.Z );
            //CascadeProjectionMatrix.orthoRh( d, d, 0.1f, 2000.0f );
            //CascadeProjectionMatrix = Float4x4::OrthoCC( Float2( -d, -d ) * 0.5f, Float2( d, d ) * 0.5f, 0.1f, 5000.0f );

            if ( CascadeIndex == NumVisibleCascades - 1 ) {
                // Last cascade have extended far plane
                CascadeProjectionMatrix = Float4x4::OrthoCC( Float2( cascadeMins[0], cascadeMins[1] ), Float2( cascadeMaxs[0], cascadeMaxs[1] ), 0.1f, 5000.0f );
            } else {

                float Z = cascadeMaxs[2] - cascadeMins[2];

                if ( 1 ) {

                    float ExtendedZ;

                    if ( RVShadowCascadeBits.GetInteger() <= 16 ) {
                        ExtendedZ = Z + LightDist + Z * 2.0f;
                    } else {
                        ExtendedZ = 5000.0f;
                    }

                    CascadeProjectionMatrix = Float4x4::OrthoCC( Float2( cascadeMins[0], cascadeMins[1] ), Float2( cascadeMaxs[0], cascadeMaxs[1] ), 0.1f, ExtendedZ );

                } else {
                    CascadeProjectionMatrix = Float4x4::OrthoCC( Float2( cascadeMins[0], cascadeMins[1] ), Float2( cascadeMaxs[0], cascadeMaxs[1] ), 0.1f, Z + LightDist );
                }
            }

            View->LightViewProjectionMatrices[View->NumShadowMapCascades] = CascadeProjectionMatrix * LightViewMatrix[CascadeIndex];
            View->ShadowMapMatrices[View->NumShadowMapCascades] = ShadowMapBias * View->LightViewProjectionMatrices[View->NumShadowMapCascades] * View->ClipSpaceToWorldSpace;

            View->NumShadowMapCascades++;
           
        }
    }
}
