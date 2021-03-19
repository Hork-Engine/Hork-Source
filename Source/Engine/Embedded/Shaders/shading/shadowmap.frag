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

#if 0

#define BLOCKER_SEARCH_NUM_SAMPLES 16
#define PCF_NUM_SAMPLES 16
//#define NEAR_PLANE 0.04//9.5
//#define LIGHT_WORLD_SIZE 0.1//.5
//#define LIGHT_FRUSTUM_WIDTH 2.0//3.75
// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
//#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)

#define LIGHT_SIZE_UV 0.02

const vec2 poissonDisk[ 16 ] = vec2[](
        vec2( -0.94201624, -0.39906216 ),
        vec2( 0.94558609, -0.76890725 ),
        vec2( -0.094184101, -0.92938870 ),
        vec2( 0.34495938, 0.29387760 ),
        vec2( -0.91588581, 0.45771432 ),
        vec2( -0.81544232, -0.87912464 ),
        vec2( -0.38277543, 0.27676845 ),
        vec2( 0.97484398, 0.75648379 ),
        vec2( 0.44323325, -0.97511554 ),
        vec2( 0.53742981, -0.47373420 ),
        vec2( -0.26496911, -0.41893023 ),
        vec2( 0.79197514, 0.19090188 ),
        vec2( -0.24188840, 0.99706507 ),
        vec2( -0.81409955, 0.91437590 ),
        vec2( 0.19984126, 0.78641367 ),
        vec2( 0.14383161, -0.14100790 )
);

float PCSS_PenumbraSize( float zReceiver, float zBlocker ) { //Parallel plane estimation
    return (zReceiver - zBlocker) / zBlocker;
} 

const float PCSS_bias =0;//0.0001;// 0.1;

void PCSS_FindBlocker( sampler2DArray _ShadowMap, float _CascadeIndex, vec2 uv, float zReceiver, out float _AvgBlockerDepth, out float _NumBlockers ) {
    //This uses similar triangles to compute what
    //area of the shadow map we should search
    const float SearchWidth = LIGHT_SIZE_UV;// * ( zReceiver - NEAR_PLANE ) / zReceiver;
    float BlockerSum = 0;
    _NumBlockers = 0;

    float biasedZ = zReceiver + PCSS_bias;

    for ( int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i ) {
        float ShadowMapDepth = texture( _ShadowMap /*PointSampler*/, vec3( uv + poissonDisk[ i ] * SearchWidth, _CascadeIndex ) ).x;
        if ( ShadowMapDepth < biasedZ ) {
            BlockerSum += ShadowMapDepth;
            _NumBlockers++;
        }
    }
    _AvgBlockerDepth = BlockerSum / _NumBlockers;
}

float PCF_Filter( sampler2DArrayShadow _ShadowMapShadow, vec4 _TexCoord, float _FilterRadiusUV ) {
    float sum = 0.0f;
    vec4 SampleCoord = _TexCoord;
    for ( int i = 0; i < PCF_NUM_SAMPLES; ++i ) {
        SampleCoord.xy = _TexCoord.xy + poissonDisk[ i ] * _FilterRadiusUV;

        //bool SamplingOutside = any( bvec2( any( lessThan( SampleCoord.xy, vec2( 0.0 ) ) ), any( greaterThan( SampleCoord.xy, vec2( 1.0 ) ) ) ) );
        //if ( SamplingOutside ) {
        //    sum += 0;
        //} else {
            sum += texture( _ShadowMapShadow, SampleCoord );
        //}
    }
    return sum / PCF_NUM_SAMPLES;
}

float PCSS_Shadow( sampler2DArray _ShadowMap, sampler2DArrayShadow _ShadowMapShadow, vec4 _TexCoord ) {
    float zReceiver = _TexCoord.w;

    float AvgBlockerDepth = 0;
    float NumBlockers = 0;
    PCSS_FindBlocker( _ShadowMap, _TexCoord.z, _TexCoord.xy, zReceiver, AvgBlockerDepth, NumBlockers );
    if ( NumBlockers < 1 ) {
        //There are no occluders so early out (this saves filtering)
        //DebugEarlyCutoff = true;
        return 1.0f;
    }

    float PenumbraRatio = PCSS_PenumbraSize( zReceiver, AvgBlockerDepth );
    float FilterRadiusUV = PenumbraRatio * LIGHT_SIZE_UV / zReceiver / exp( _TexCoord.z );// * NEAR_PLANE / zReceiver;

    return PCF_Filter( _ShadowMapShadow, _TexCoord, FilterRadiusUV );
}
#endif

float PCF_3x3( sampler2DArrayShadow _ShadowMap, vec4 _TexCoord ) {
#ifdef ATI
    const float invSize = 1.0 / textureSize(_ShadowMap,0).x; // NOTE: shadow maps has equal width and height

    return ( texture( _ShadowMap, _TexCoord + vec4(-invSize,-invSize, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4( 0,      -invSize, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4( invSize,-invSize, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4(-invSize,       0, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4( 0,             0, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4( invSize,       0, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4(-invSize, invSize, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4( 0,       invSize, 0, 0 ) )
           + texture( _ShadowMap, _TexCoord + vec4( invSize, invSize, 0, 0 ) ) ) / 9.0;
#else
    return ( textureOffset( _ShadowMap, _TexCoord, ivec2(-1,-1 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 0,-1 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 1,-1 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2(-1, 0 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 0, 0 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 1, 0 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2(-1, 1 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 0, 1 ) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 1, 1 ) ) ) / 9.0;
#endif
}

float PCF_5x5( sampler2DArrayShadow _ShadowMap, vec4 _TexCoord ) {
#ifdef ATI
    const float invSize = 1.0 / textureSize(_ShadowMap,0).x; // NOTE: shadow maps has equal width and height

    float Shadow = 0;
    for ( float i = -2 ; i <= 2 ; i++ )
        for ( float j = -2 ; j <= 2 ; j++ )
            Shadow += texture( _ShadowMap, _TexCoord + vec4(i,j,0,0)*invSize );
#else
    float Shadow = 0;
    for ( int i = -2 ; i <= 2 ; i++ )
        for ( int j = -2 ; j <= 2 ; j++ )
            Shadow += textureOffset( _ShadowMap, _TexCoord, ivec2( i, j ) );
#endif
    return Shadow * (1.0f/25.0f);
}

float SampleLightShadow( uint ShadowmapIndex, uint FirstCascade, uint NumCascades, float Bias ) {
    for ( uint i = 0; i < NumCascades ; i++ ) {
        uint CascadeIndex = FirstCascade + i;
        vec4 SMTexCoord = ShadowMapMatrices[ CascadeIndex ] * InClipspacePosition;
        vec3 ShadowCoord = SMTexCoord.xyz / SMTexCoord.w;
        
        // TODO: Move this values to uniforms
        ShadowCoord.z -= clamp( Bias * (1 << i)*1.5, 0.45, 5.0 ) * 0.0001;
        
#ifdef SHADOWMAP_PCF
        if ( !any( bvec2( any( lessThan( ShadowCoord, vec3( 0.0 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0 ) ) ) ) ) ) {
            return PCF_5x5( ShadowMap[ShadowmapIndex], vec4( ShadowCoord.xy, float(i), ShadowCoord.z ) );
        }
#endif

#ifdef SHADOWMAP_PCSS
        if ( !any( bvec2( any( lessThan( ShadowCoord, vec3( 0.05 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0-0.05 ) ) ) ) ) ) {
            return PCSS_Shadow( ShadowMap[ShadowmapIndex], vec4( ShadowCoord.xy, float(i), ShadowCoord.z ) );
        }
#endif

#ifdef SHADOWMAP_VSM
        if ( !any( bvec2( any( lessThan( ShadowCoord, vec3( 0.01 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0-0.01 ) ) ) ) ) ) {
            return VSM_Shadow( ShadowMap[ShadowmapIndex], vec4( ShadowCoord.xy, float(i), ShadowCoord.z ) );
            //Shadow = VSM_Shadow_PCF_3x3( ShadowMap[ShadowmapIndex], SMTexCoord );
        }
#endif

#ifdef SHADOWMAP_EVSM
        if ( !any( bvec2( any( lessThan( ShadowCoord, vec3( 0.01 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0-0.01 ) ) ) ) ) ) {
            return EVSM_Shadow( ShadowMap[ShadowmapIndex], vec4( ShadowCoord.xy, float(i), ShadowCoord.z ) );
        }
#endif
    }

    return 1.0;
}

vec3 DebugShadowCascades( uint FirstCascade, uint NumCascades )
{    
    const vec3 CascadeColor[MAX_SHADOW_CASCADES] = vec3[](
        vec3( 1,0,0 ),
        vec3( 0,1,0 ),
        vec3( 0,0,1 ),
        vec3( 1,1,0 )
    );
    
    for ( uint i = 0; i < NumCascades ; i++ ) {
        const uint CascadeIndex = FirstCascade + i;
        const vec4 SMTexCoord = ShadowMapMatrices[ CascadeIndex ] * InClipspacePosition;
        
        vec3 ShadowCoord = SMTexCoord.xyz / SMTexCoord.w;
        
        if ( !any( bvec2( any( lessThan( ShadowCoord, vec3( 0.0 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0 ) ) ) ) ) ) {
            return CascadeColor[i];
        }
    }

    return vec3( 0.0 );
}

vec3 DebugDirectionalLightCascades()
{
    const uint NumLights = GetNumDirectionalLights();
    vec3 Result = vec3(0.0);
    
    for ( int i = 0 ; i < NumLights ; ++i ) {
        float x = 1.0 / float(NumLights);
        
        if ( i * x <= InNormalizedScreenCoord.x && ( i + 1 ) * x > InNormalizedScreenCoord.x ) {
            Result += DebugShadowCascades( LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ] );
        }
    }
    return Result;
}
