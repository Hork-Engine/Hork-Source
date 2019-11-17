#include "ShadowCascade.h"

#include <Engine/Runtime/Public/RuntimeVariable.h>

ARuntimeVariable RVShadowCascadeBits( _CTS( "ShadowCascadeBits" ), _CTS( "24" ) );    // Allowed 16, 24 or 32 bits

constexpr int MAX_CASCADE_SPLITS = MAX_SHADOW_CASCADES + 1;

static constexpr Float4x4 ShadowMapBias = Float4x4(
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f,-0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f
);


//
// Temporary variables
//
struct SCascadeSplit { // TODO: move inside LightComponent.h
    float Distance;     // Distance from view
};
static SCascadeSplit CascadeSplits[ MAX_CASCADE_SPLITS ];   // TODO: move inside ADirectionalLightComponent?
static Float4 LightSpaceVerts[ MAX_CASCADE_SPLITS ][ 4 ];    // Split corners in light space
static Float3 CascadeBounds[ MAX_SHADOW_CASCADES ][ 2 ];     // Cascade bounding boxes
static float PerspHalfWidth;
static float PerspHalfHeight;
static Float3 RV, UV;           // scaled right and up vectors in world space

static void CalcCascades( SRenderView * View, SDirectionalLightDef & _LightDef, SCascadeSplit * _CascadeSplits );

void CreateDirectionalLightCascades( SRenderFrame * Frame, SRenderView * View ) {
    View->NumShadowMapCascades = 0;
    View->NumCascadedShadowMaps = 0;

    if ( View->bPerspective ) {
        // for perspective camera

        PerspHalfWidth = std::tan( View->ViewFovX * 0.5f );
        PerspHalfHeight = std::tan( View->ViewFovY * 0.5f );
    } else {
        // for ortho camera

        float orthoWidth = View->ViewOrthoMaxs.X - View->ViewOrthoMins.X;
        float orthoHeight = View->ViewOrthoMaxs.Y - View->ViewOrthoMins.Y;

        RV = View->ViewRightVec * Math::Abs( orthoWidth * 0.5f );
        UV = View->ViewUpVec * Math::Abs( orthoHeight * 0.5f );
    }

    for ( int i = 0 ; i < View->NumDirectionalLights ; i++ ) {
        SDirectionalLightDef & lightDef = *Frame->DirectionalLights[ View->FirstDirectionalLight + i ];

        if ( !lightDef.bCastShadow ) {
            continue;
        }

        // Calc splits
        CascadeSplits[0].Distance = View->ViewZNear;
        CascadeSplits[lightDef.MaxShadowCascades].Distance = View->ViewZFar;

    #if 0
        // TODO: logarithmic subdivision!
        float Range = _CascadeSplits[lightDef.MaxShadowCascades].Distance - _CascadeSplits[0].Distance;
        int NumInnerSplits = lightDef.MaxShadowCascades - 1;
        if ( NumInnerSplits > 0 ) {
            float LinearDist = Range / lightDef.MaxShadowCascades;
            for ( int i = 1 ; i < lightDef.MaxShadowCascades ; i++ ) {
                CascadeSplits[i].Distance = CascadeSplits[i-1].Distance + LinearDist;
            }
        }
    #else
        // Fixed range
        const float Range = 64;
        //_CascadeSplits[1].Distance = 2;
        //_CascadeSplits[2].Distance = 16;
        //_CascadeSplits[3].Distance = 32;
        //_CascadeSplits[4].Distance = 64;
        CascadeSplits[1].Distance = Range * 0.05f;
        CascadeSplits[2].Distance = Range * 0.2f;
        CascadeSplits[3].Distance = Range * 0.5f;
        CascadeSplits[4].Distance = 128;//Range * 1.0f;
    #endif

        CalcCascades( View, lightDef, CascadeSplits );

        if ( lightDef.NumCascades > 0 ) {
            View->NumCascadedShadowMaps++;  // Just statistics
        }
    }

    Frame->ShadowCascadePoolSize = Math::Max( Frame->ShadowCascadePoolSize, View->NumShadowMapCascades );
}

static void CalcCascades( SRenderView * View, SDirectionalLightDef & _LightDef, SCascadeSplit * _CascadeSplits ) {
    int NumSplits = _LightDef.MaxShadowCascades + 1;
    int NumVisibleSplits;
    int NumVisibleCascades;
    Float3 CenterWorldspace; // center of split in world space
    Float4x4 CascadeProjectionMatrix;
    Float4x4 LightViewMatrix;

    assert( _LightDef.MaxShadowCascades > 0 );
    assert( _LightDef.MaxShadowCascades <= MAX_SHADOW_CASCADES );    

    const float LightDist = 400.0f;

    Float3   LightPos = View->ViewPosition + _LightDef.Matrix[2] * LightDist;
    Float3x3 Basis = _LightDef.Matrix.Transposed();
    Float3   Origin = Basis * (-LightPos);

    LightViewMatrix[0] = Float4( Basis[0], 0.0f );
    LightViewMatrix[1] = Float4( Basis[1], 0.0f );
    LightViewMatrix[2] = Float4( Basis[2], 0.0f );
    LightViewMatrix[3] = Float4( Origin, 1.0f );

    // FIXME: Возможно лучше для каждого каскада позиционировать источник освещения индивидуально?

    const float MaxVisibleDistance = Math::Max( View->MaxVisibleDistance, _CascadeSplits[0].Distance );

    for ( NumVisibleSplits = 0 ;
          NumVisibleSplits < NumSplits && ( _CascadeSplits[ NumVisibleSplits - 1 ].Distance <= MaxVisibleDistance ) ;
          NumVisibleSplits++ )
    {
        const float & SplitDistance = CascadeSplits[ NumVisibleSplits ].Distance;

        if ( View->bPerspective ) {
            // for perspective camera
            RV = View->ViewRightVec * ( PerspHalfWidth * SplitDistance );
            UV = View->ViewUpVec * ( PerspHalfHeight * SplitDistance );
        }

        CenterWorldspace = View->ViewPosition + View->ViewDir * SplitDistance;

        LightSpaceVerts[ NumVisibleSplits ][ 0 ] = LightViewMatrix * Float4( CenterWorldspace - RV - UV, 1.0f );
        LightSpaceVerts[ NumVisibleSplits ][ 1 ] = LightViewMatrix * Float4( CenterWorldspace - RV + UV, 1.0f );
        LightSpaceVerts[ NumVisibleSplits ][ 2 ] = LightViewMatrix * Float4( CenterWorldspace + RV + UV, 1.0f );
        LightSpaceVerts[ NumVisibleSplits ][ 3 ] = LightViewMatrix * Float4( CenterWorldspace + RV - UV, 1.0f );
    }

    assert( NumVisibleSplits >= 2 );

    NumVisibleCascades = NumVisibleSplits - 1;

    _LightDef.FirstCascade = View->NumShadowMapCascades;
    _LightDef.NumCascades = NumVisibleCascades;

    //GLogger.Printf( "First cascade %d num %d\n", _LightDef.FirstCascade,_LightDef.NumCascades);

    const float Extrusion = 0.0f;

    for ( int CascadeIndex = 0 ; CascadeIndex < NumVisibleCascades ; CascadeIndex++ ) {
        Float3 & Mins = CascadeBounds[ CascadeIndex ][ 0 ];
        Float3 & Maxs = CascadeBounds[ CascadeIndex ][ 1 ];

        const Float4 * pVerts = &LightSpaceVerts[ CascadeIndex ][ 0 ];

        // TODO: Use SSE

        Mins = Maxs = Float3( *pVerts );

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

        ++pVerts;

        Mins.X = Math::Min( Mins.X, pVerts->X );
        Mins.Y = Math::Min( Mins.Y, pVerts->Y );
        Mins.Z = Math::Min( Mins.Z, pVerts->Z );

        Maxs.X = Math::Max( Maxs.X, pVerts->X );
        Maxs.Y = Math::Max( Maxs.Y, pVerts->Y );
        Maxs.Z = Math::Max( Maxs.Z, pVerts->Z );

        Mins -= Extrusion;
        Maxs += Extrusion;

        // Snap to minimize jittering
        Mins.X = floorf( Mins.X * 0.5f ) * 2.0f;
        Mins.Y = floorf( Mins.Y * 0.5f ) * 2.0f;
        Mins.Z = floorf( Mins.Z * 0.5f ) * 2.0f;
        Maxs.X = ceilf( Maxs.X * 0.5f ) * 2.0f;
        Maxs.Y = ceilf( Maxs.Y * 0.5f ) * 2.0f;
        Maxs.Z = ceilf( Maxs.Z * 0.5f ) * 2.0f;

        //CascadeProjectionMatrix = Math::OrthoCC( Mins, Maxs );

        if ( CascadeIndex == NumVisibleCascades - 1 ) {
            // Last cascade have extended far plane
            CascadeProjectionMatrix = Float4x4::OrthoCC( Mins.Shuffle2<sXY>(), Maxs.Shuffle2<sXY>(), 0.1f, 5000.0f );
        } else {

            float Z = Maxs.Z - Mins.Z;

            if ( 1 ) {

                float ExtendedZ;

                if ( RVShadowCascadeBits.GetInteger() <= 16 ) {
                    ExtendedZ = Z + LightDist + Z * 2.0f;
                } else {
                    ExtendedZ = 5000.0f;
                }

                CascadeProjectionMatrix = Float4x4::OrthoCC( Mins.Shuffle2<sXY>(), Maxs.Shuffle2<sXY>(), 0.1f, ExtendedZ );
                
            } else {
                CascadeProjectionMatrix = Float4x4::OrthoCC( Mins.Shuffle2<sXY>(), Maxs.Shuffle2<sXY>(), 0.1f, Z + LightDist );
            }
        }        

        View->LightViewProjectionMatrices[ View->NumShadowMapCascades ] = CascadeProjectionMatrix * LightViewMatrix;
        View->ShadowMapMatrices[ View->NumShadowMapCascades ] = ShadowMapBias * View->LightViewProjectionMatrices[ View->NumShadowMapCascades ] * View->ClipSpaceToWorldSpace;

        View->NumShadowMapCascades++;
    }
}
