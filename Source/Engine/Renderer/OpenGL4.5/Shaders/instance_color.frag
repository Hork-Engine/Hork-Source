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

#include "base/common.frag"
#include "base/pbr.frag"
#include "base/debug.glsl"

layout( binding = 3, std140 ) uniform ShadowMatrixBuffer {
    mat4 CascadeViewProjection[ MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES ];
    mat4 ShadowMapMatrices[ MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES ];
};

struct SClusterLight
{
	vec4 Pack;        // For point and spot lights: position and radius
    vec4 Pack2;       // Pack2.x - light type, Pack2.y - inner radius, z - cone outer angle, w - cone inner angle
    vec4 Pack3;       // Pack3.xyz - direction of spot light, w - spot exponent
    vec4 Color;       // RGB, alpha - ambient intensity
    uvec4 IPack;       // IPack.x - RenderMask
};

#define MAX_LIGHTS 768

layout( binding = 4, std140 ) uniform UniformBuffer4 {
	SClusterLight LightBuffer[MAX_LIGHTS];
};


layout( origin_upper_left ) in vec4 gl_FragCoord;

layout( binding = 13 ) uniform usamplerBuffer ClusterItemTBO;
layout( binding = 14 ) uniform usampler3D ClusterLookup; // TODO: Use bindless?
layout( binding = 15 ) uniform sampler2DArrayShadow ShadowMapShadow; // TODO: Use bindless?


vec3 InViewspaceToEyeVec;

vec3 ComputeLight( in vec4 LightColor, in vec3 Diffuse, in vec3 RealSpecularColor, in float SqrRoughnessClamped, in vec3 Normal, in vec3 L, in float NdL, in float NdV, in float Shadow ) {
    float AmbientLightIntensity = LightColor.w; // UNUSED!!!

    vec3 H = normalize( L + InViewspaceToEyeVec );
    float NdH = saturate( dot( Normal, H ) );
    float VdH = saturate( dot( InViewspaceToEyeVec, H ) );
    float LdV = saturate( dot( L, InViewspaceToEyeVec ) );

    vec3 Spec = Specular( RealSpecularColor, SqrRoughnessClamped, NdL, NdV, NdH, VdH, LdV );

    return LightColor.xyz * Shadow * /* NdL * */( Diffuse * ( 1.0 - Spec ) + Spec ); // FIXME: Почему 1 - Spec а не (1.0 - Specular_F)
}

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

float PCSS_PenumbraSize( in float zReceiver, in float zBlocker ) { //Parallel plane estimation
    return (zReceiver - zBlocker) / zBlocker;
} 

const float PCSS_bias =0;//0.0001;// 0.1;

void PCSS_FindBlocker( in sampler2DArray _ShadowMap, in float _CascadeIndex, in vec2 uv, in float zReceiver, out float _AvgBlockerDepth, out float _NumBlockers ) {
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

float PCF_Filter( in sampler2DArrayShadow _ShadowMapShadow, in vec4 _TexCoord, in float _FilterRadiusUV ) {
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
#endif

// Сглаживание границы тени (Percentage Closer Filtering)
float PCF_3x3( in sampler2DArrayShadow _ShadowMap, in vec4 _TexCoord ) {
//float sum=0;
//for ( int i=-2;i<=2;i++)
//for ( int j=-2;j<=2;j++)
//sum += textureOffset( _ShadowMap, _TexCoord, ivec2( i, j) );
//return sum / 25.0;
//return 1;

    return ( textureOffset( _ShadowMap, _TexCoord, ivec2(-1,-1) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 0,-1) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 1,-1) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2(-1, 0) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 0, 0) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 1, 0) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2(-1, 1) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 0, 1) )
           + textureOffset( _ShadowMap, _TexCoord, ivec2( 1, 1) ) ) / 9.0;

}


#if 0
float PCSS_Shadow( in sampler2DArray _ShadowMap, in sampler2DArrayShadow _ShadowMapShadow, in vec4 _TexCoord ) {
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
float SampleLightShadow( in vec4 ClipspacePosition, in uint _ShadowPoolPosition, in uint _NumCascades, in float Bias ) {
    float ShadowValue = 1.0;
    uint CascadeIndex;

    for ( uint i = 0; i < _NumCascades ; i++ ) {
        CascadeIndex = _ShadowPoolPosition + i;
        vec4 SMTexCoord = ShadowMapMatrices[ CascadeIndex ] * ClipspacePosition;
		
		//SMTexCoord.z += -Bias*0.001*SMTexCoord.w;

        vec3 ShadowCoord = SMTexCoord.xyz / SMTexCoord.w;
		
ShadowCoord.z += -Bias*0.0002*(i+1);

#ifdef SHADOWMAP_PCF
        bool SamplingOutside = any( bvec2( any( lessThan( ShadowCoord, vec3( 0.0 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0 ) ) ) ) );
        ShadowValue = SamplingOutside ? 1.0 : PCF_3x3( ShadowMapShadow, vec4( ShadowCoord.xy, float(CascadeIndex), ShadowCoord.z ) );

        // test:
        //if ( !SamplingOutside ) {
        //    ShadowValue = float(i) / _NumCascades;
        //}
#endif
#ifdef SHADOWMAP_PCSS
        bool SamplingOutside = any( bvec2( any( lessThan( ShadowCoord, vec3( 0.05 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0-0.05 ) ) ) ) );
        ShadowValue = SamplingOutside ? 1.0 : PCSS_Shadow( ShadowMap, ShadowMapShadow, vec4( ShadowCoord.xy, float(CascadeIndex), ShadowCoord.z ) );
#endif

#ifdef SHADOWMAP_VSM
        bool SamplingOutside = any( bvec2( any( lessThan( ShadowCoord, vec3( 0.01 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0-0.01 ) ) ) ) );
        ShadowValue = SamplingOutside ? 1.0 : VSM_Shadow( ShadowMap, vec4( ShadowCoord.xy, float(CascadeIndex), ShadowCoord.z ) );
        //Shadow = VSM_Shadow_PCF_3x3( ShadowMap, SMTexCoord );
#endif
#ifdef SHADOWMAP_EVSM
        bool SamplingOutside = any( bvec2( any( lessThan( ShadowCoord, vec3( 0.01 ) ) ), any( greaterThan( ShadowCoord, vec3( 1.0-0.01 ) ) ) ) );
        ShadowValue = SamplingOutside ? 1.0 : EVSM_Shadow( ShadowMap, vec4( ShadowCoord.xy, float(CascadeIndex), ShadowCoord.z ) );
#endif

        if ( !SamplingOutside ) {
            break;
        }
    }

    return ShadowValue;
}

vec3 CalcDirectionalLightingPBR( in vec4 ClipspacePosition, in vec3 Diff, in vec3 RealSpecularColor, in float SqrRoughnessClamped, in vec3 Normal, in float NdV, in vec3 FaceNormal ) {
    vec3 light = vec3(0.0);

    for ( int i = 0 ; i < NumDirectionalLights.x ; ++i ) {
        uint renderMask = LightParameters[ i ][ 0 ];

        // TODO:
        //if ( ( renderMask & Material.AffectedLightMask ) == 0 ) {
        //    continue;
        //}

        vec3 lightDir = LightDirs[ i ].xyz;

        float NdL = saturate( dot( Normal, lightDir ) );

        if ( NdL == 0.0 ) {
            //DebugEarlyCutoff = true;
            continue;
        }

        float shadow;
        // TODO:
        //if ( ( renderMask & Material.ShadowReceiveMask ) != 0 ) {

		    float bias = max( 1.0 - max( dot(FaceNormal, lightDir), 0.0 ), 0.1 ) * 0.05;
 	
            //shadow = NdL * SampleLightShadow( ClipspacePosition, LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ] );
            shadow = NdL * SampleLightShadow( ClipspacePosition, LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ], bias );

            if ( shadow == 0.0 ) {
                //DebugEarlyCutoff = true;
                continue;
            }
        //} else {
        //    shadow = NdL;
        //}

        light += ComputeLight( LightColors[ i ], Diff, RealSpecularColor, SqrRoughnessClamped, Normal, lightDir, NdL, NdV, shadow );
    }
    return light;
}
/*
vec3 CalcPointLightLightingPBR( in vec3 VertexPosition, in vec3 Diff, in vec3 RealSpecularColor, in float SqrRoughnessClamped, in vec3 Normal, in float NdV ) {
    vec3 light = vec3(0.0);
	
	
	    
		
		const vec4 LIGHT_COLOR = vec4(10,10,10,1);
		const vec3 LIGHT_POS = vec3(0);
		const float LIGHT_OUTER_RADIUS = 10;
		const float LIGHT_INNER_RADIUS = 2;
		const float SHADOW = 1.0;

		vec3 lightDir = LIGHT_POS - VertexPosition;
		float LightDist = length( lightDir );
		float OuterRadius = LIGHT_OUTER_RADIUS;

		if ( LightDist > OuterRadius || LightDist == 0.0 ) {
			//continue;
		}
		lightDir /= LightDist; // normalize
		float InnerRadius = LIGHT_INNER_RADIUS;
		float d = max( InnerRadius, LightDist );
		float Attenuation = saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
		float NdL = saturate( dot( Normal, lightDir ) );

		Attenuation = Attenuation * NdL * SHADOW;

		light += ComputeLight( LIGHT_COLOR, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, lightDir, NdL, NdV, Attenuation );
	
	

    return light;
}
*/
vec3 CalcPointLightLightingPBR( in vec3 VertexPosition, in vec3 Diff, in vec3 RealSpecularColor, in float SqrRoughnessClamped, in vec3 Normal, in float NdV/*, in vec3 ViewDir*/, in vec2 FragCoord/*, in vec3 Specular*/ ) {
    vec3 light = vec3(0.0);

	float InScreenDepth           = gl_FragCoord.z;
	float InNearZ                 = ViewportParams.z;
    float InFarZ                  = ViewportParams.w;
    float InLinearScreenDepth     = ComputeLinearDepth( InScreenDepth, InNearZ, InFarZ );

    // Расчет Scale и Bias ////////////////////////////////////////////////////////
    // TODO: вынести Scale и Bias в юниформы
    #define NUM_CLUSTERS_Z 24
    const float ZNear = 0.0125f;
    const float ZFar = 512;
    const int NearOffset = 20;
    const float Scale = ( NUM_CLUSTERS_Z + NearOffset ) / log2( ZFar / ZNear );
    const float Bias = -log2( ZNear ) * Scale - NearOffset;
    ///////////////////////////////////////////////////////////////////////////////

    // Получаем кластер ///////////////////////////////////////////////////////////
    const float Slice = max( 0.0, floor( log2( InLinearScreenDepth ) * Scale + Bias ) );
    const ivec3 P = ivec3( FragCoord.x * 16, FragCoord.y * 8, Slice );
	
	const uvec2 Cluster = texelFetch( ClusterLookup, P, 0 ).xy;
	

				///////////////////////////////////////////////////////////////////////////////

				// Распаковываем кластер //////////////////////////////////////////////////////
				const int NumProbes = int( Cluster.y & 0xff );
				const int NumDecals = int( ( Cluster.y >> 8 ) & 0xff );
				const int NumLights = int( ( Cluster.y >> 16 ) & 0xff );
				//const int Unused = int( ( Cluster.y >> 24 ) & 0xff ); // can be used in future
				///////////////////////////////////////////////////////////////////////////////
				
	//			if ( NumLights > 0 || NumDecals > 0 || NumProbes > 0 ) {
	//				FS_FragColor.rgb = vec3(P.x/16.0, P.y/8.0, floor(Slice)/24);
	//			}
	
	vec3 L;
	
	float Shadow = 1;
	float Attenuation;
	
	#define POINT_LIGHT 1
    #define SPOT_LIGHT 2

	
	for ( int i = 0 ; i < NumLights ; i++ ) {
		const uint Indices = texelFetch( ClusterItemTBO, int(Cluster.x + i) ).x;
        const uint LightIndex = Indices & 0x3ff;
		
		
		
		//uint RenderMask = LightBuffer[ LightIndex ].IPack.x;

        //if ( ( RenderMask & Material.AffectedLightMask ) == 0 ) {
        //    continue;
        //}
		
/*
        switch ( int( LightBuffer[ LightIndex ].Pack2.x ) ) { // light type
            case POINT_LIGHT:*/
                float OuterRadius = LightBuffer[ LightIndex ].Pack.w;

                L = LightBuffer[ LightIndex ].Pack.xyz - VertexPosition;
				
                float LightDist = length( L );

                if ( LightDist > OuterRadius || LightDist == 0.0 ) {
                    continue;
                }

                L /= LightDist; // normalize

                float InnerRadius = LightBuffer[ LightIndex ].Pack2.y;

                float d = max( InnerRadius, LightDist );
                Attenuation = saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
                //Attenuation = pow( saturate( 1.0 - pow( LightDist / OuterRadius, 4.0 ) ), 2.0 );// / (LightDist*LightDist + 1.0 );   // from Unreal
                //Attenuation = pow( 1.0 - min( LightDist / OuterRadius, 1.0 ), 2.2 );   // My attenuation

                //if ( ( RenderMask & Material.ShadowReceiveMask ) != 0 ) {
                //    // TODO: // Sample point light shadows
                //    Shadow = 1;
                //} else {
                //    Shadow = 1;
                //}

/*                break;
            case SPOT_LIGHT:
                OuterRadius = LightBuffer[ LightIndex ].Pack.w;

                L = LightBuffer[ LightIndex ].Pack.xyz - VertexPosition;

                LightDist = length( L );

                if ( LightDist > OuterRadius || LightDist == 0.0 ) {
                    continue;
                }

                L /= LightDist; // normalize

                InnerRadius = LightBuffer[ LightIndex ].Pack2.y;
                d = max( InnerRadius, LightDist );
                Attenuation = saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
                //Attenuation = pow( saturate( 1.0 - pow( LightDist / OuterRadius, 4.0 ) ), 2.0 );// / (LightDist*LightDist + 1.0 );   // from Unreal
                //Attenuation = pow( 1.0 - min( LightDist / OuterRadius, 1.0 ), 2.2 );   // My attenuation

                float CosConeRay = dot( L, LightBuffer[ LightIndex ].Pack3.xyz );
                float ConeFalloff = pow( smoothstep( LightBuffer[ LightIndex ].Pack2.z, LightBuffer[ LightIndex ].Pack2.w, CosConeRay ), LightBuffer[ LightIndex ].Pack3.w );

                Attenuation *= ConeFalloff;

                //if ( ( RenderMask & Material.ShadowReceiveMask ) != 0 ) {
                //    // TODO: // Sample spot light shadows
                //    Shadow = 1;
                //} else {
                //    Shadow = 1;
                //}

                break;
        }*/
/*
    const vec3 LIGHT_COLOR = vec3(20,10,4);
    const vec3 LIGHT_POS = vec3(0);
    const float LIGHT_OUTER_RADIUS = 10;
    const float LIGHT_INNER_RADIUS = 2;
    const float SHADOW = 1.0;

    vec3 lightDir = LIGHT_POS - VertexPosition;
    float LightDist = length( lightDir );
    float OuterRadius = LIGHT_OUTER_RADIUS;

    if ( LightDist > OuterRadius || LightDist == 0.0 ) {
        //continue;
    }
    lightDir /= LightDist; // normalize
    float InnerRadius = LIGHT_INNER_RADIUS;
    float d = max( InnerRadius, LightDist );
    float Attenuation = saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
*/

		float NdL = saturate( dot( Normal, L ) );

		Attenuation = Attenuation * NdL * Shadow;

		light += ComputeLight( LightBuffer[ LightIndex ].Color, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, L, NdL, NdV, Attenuation );
		
		//vec3 reflectDir = reflect(-L, Normal);		
		//float specularPow = 32;
        //float specFactor = pow(max(dot(ViewDir, reflectDir), 0.0), specularPow) * 50;
        //light += Specular * specFactor*Attenuation;
	}

	//if ( NumLights > 0 )
	//    light = vec3(P.x/16.0, P.y/8.0, floor(Slice)/24);


    return light;
}

vec3 CalcDirectionalLighting( in vec4 ClipspacePosition, in vec3 Normal, in vec3 ViewDir, in vec3 FaceNormal, in vec3 Specular ) {
    vec3 light = vec3(0.0);

    for ( int i = 0 ; i < NumDirectionalLights.x ; ++i ) {
        uint renderMask = LightParameters[ i ][ 0 ];

        // TODO:
        //if ( ( renderMask & Material.AffectedLightMask ) == 0 ) {
        //    continue;
        //}

        vec3 lightDir = LightDirs[ i ].xyz;

        float NdL = saturate( dot( Normal, lightDir ) );

        if ( NdL == 0.0 ) {
            //DebugEarlyCutoff = true;
            continue;
        }

        float shadow;
        // TODO:
        //if ( ( renderMask & Material.ShadowReceiveMask ) != 0 ) {
		
		    float bias = max( 1.0 - max( dot(FaceNormal, lightDir), 0.0 ), 0.1 ) * 0.05;

            shadow = NdL * SampleLightShadow( ClipspacePosition, LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ], bias );

            if ( shadow == 0.0 ) {
                //DebugEarlyCutoff = true;
                continue;
            }
        //} else {
        //    shadow = NdL;
        //}

        light += LightColors[ i ].xyz * shadow;

        // Directional light specular
        vec3 reflectDir = reflect(-lightDir, Normal);		
	float specularPow = 32;
        float specFactor = pow(max(dot(ViewDir, reflectDir), 0.0), specularPow) * 10;
        light += Specular * specFactor;//*Attenuation;
    }
    return light;
}

#if defined FRAGMENT_SHADER && defined MATERIAL_PASS_COLOR
vec3 CalcPointLightLighting( in vec3 VertexPosition, in vec3 Normal, in vec3 ViewDir, in vec2 FragCoord, in vec3 Specular ) {
    vec3 light = vec3(0.0);

	float InScreenDepth           = gl_FragCoord.z;
	float InNearZ                 = ViewportParams.z;
    float InFarZ                  = ViewportParams.w;
    float InLinearScreenDepth     = ComputeLinearDepth( InScreenDepth, InNearZ, InFarZ );

    // Расчет Scale и Bias ////////////////////////////////////////////////////////
    // TODO: вынести Scale и Bias в юниформы
    #define NUM_CLUSTERS_Z 24
    const float ZNear = 0.0125f;
    const float ZFar = 512;
    const int NearOffset = 20;
    const float Scale = ( NUM_CLUSTERS_Z + NearOffset ) / log2( ZFar / ZNear );
    const float Bias = -log2( ZNear ) * Scale - NearOffset;
    ///////////////////////////////////////////////////////////////////////////////

    // Получаем кластер ///////////////////////////////////////////////////////////
    const float Slice = max( 0.0, floor( log2( InLinearScreenDepth ) * Scale + Bias ) );
    const ivec3 P = ivec3( FragCoord.x * 16, FragCoord.y * 8, Slice );
	
	const uvec2 Cluster = texelFetch( ClusterLookup, P, 0 ).xy;
	

				///////////////////////////////////////////////////////////////////////////////

				// Распаковываем кластер //////////////////////////////////////////////////////
				const int NumProbes = int( Cluster.y & 0xff );
				const int NumDecals = int( ( Cluster.y >> 8 ) & 0xff );
				const int NumLights = int( ( Cluster.y >> 16 ) & 0xff );
				//const int Unused = int( ( Cluster.y >> 24 ) & 0xff ); // can be used in future
				///////////////////////////////////////////////////////////////////////////////
				
	//			if ( NumLights > 0 || NumDecals > 0 || NumProbes > 0 ) {
	//				FS_FragColor.rgb = vec3(P.x/16.0, P.y/8.0, floor(Slice)/24);
	//			}
	
	vec3 L;
	
	float Shadow = 1;
	float Attenuation;
	
	#define POINT_LIGHT 1
    #define SPOT_LIGHT 2

	
	for ( int i = 0 ; i < NumLights ; i++ ) {
		const uint Indices = texelFetch( ClusterItemTBO, int(Cluster.x + i) ).x;
        const uint LightIndex = Indices & 0x3ff;
		
		
		
		//uint RenderMask = LightBuffer[ LightIndex ].IPack.x;

        //if ( ( RenderMask & Material.AffectedLightMask ) == 0 ) {
        //    continue;
        //}
/*
        switch ( int( LightBuffer[ LightIndex ].Pack2.x ) ) { // light type
            case POINT_LIGHT:*/
                float OuterRadius = LightBuffer[ LightIndex ].Pack.w;

                L = LightBuffer[ LightIndex ].Pack.xyz - VertexPosition;
				
                float LightDist = length( L );

                if ( LightDist > OuterRadius || LightDist == 0.0 ) {
                    continue;
                }

                L /= LightDist; // normalize

                float InnerRadius = LightBuffer[ LightIndex ].Pack2.y;

                float d = max( InnerRadius, LightDist );
                //Attenuation = saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
                //Attenuation = pow( saturate( 1.0 - pow( LightDist / OuterRadius, 4.0 ) ), 2.0 );// / (LightDist*LightDist + 1.0 );   // from Unreal
                Attenuation = pow( 1.0 - min( LightDist / OuterRadius, 1.0 ), 2.2 );   // My attenuation

                //if ( ( RenderMask & Material.ShadowReceiveMask ) != 0 ) {
                //    // TODO: // Sample point light shadows
                //    Shadow = 1;
                //} else {
                //    Shadow = 1;
                //}

/*                break;
            case SPOT_LIGHT:
                OuterRadius = LightBuffer[ LightIndex ].Pack.w;

                L = LightBuffer[ LightIndex ].Pack.xyz - VertexPosition;

                LightDist = length( L );

                if ( LightDist > OuterRadius || LightDist == 0.0 ) {
                    continue;
                }

                L /= LightDist; // normalize

                InnerRadius = LightBuffer[ LightIndex ].Pack2.y;
                d = max( InnerRadius, LightDist );
                Attenuation = saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
                //Attenuation = pow( saturate( 1.0 - pow( LightDist / OuterRadius, 4.0 ) ), 2.0 );// / (LightDist*LightDist + 1.0 );   // from Unreal
                //Attenuation = pow( 1.0 - min( LightDist / OuterRadius, 1.0 ), 2.2 );   // My attenuation

                float CosConeRay = dot( L, LightBuffer[ LightIndex ].Pack3.xyz );
                float ConeFalloff = pow( smoothstep( LightBuffer[ LightIndex ].Pack2.z, LightBuffer[ LightIndex ].Pack2.w, CosConeRay ), LightBuffer[ LightIndex ].Pack3.w );

                Attenuation *= ConeFalloff;

                //if ( ( RenderMask & Material.ShadowReceiveMask ) != 0 ) {
                //    // TODO: // Sample spot light shadows
                //    Shadow = 1;
                //} else {
                //    Shadow = 1;
                //}

                break;
        }*/
/*
    const vec3 LIGHT_COLOR = vec3(20,10,4);
    const vec3 LIGHT_POS = vec3(0);
    const float LIGHT_OUTER_RADIUS = 10;
    const float LIGHT_INNER_RADIUS = 2;
    const float SHADOW = 1.0;

    vec3 lightDir = LIGHT_POS - VertexPosition;
    float LightDist = length( lightDir );
    float OuterRadius = LIGHT_OUTER_RADIUS;

    if ( LightDist > OuterRadius || LightDist == 0.0 ) {
        //continue;
    }
    lightDir /= LightDist; // normalize
    float InnerRadius = LIGHT_INNER_RADIUS;
    float d = max( InnerRadius, LightDist );
    float Attenuation = saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
*/
		float NdL = saturate( dot( Normal, L ) );

		Attenuation = Attenuation * Shadow;

		light += LightBuffer[ LightIndex ].Color.rgb * (Attenuation*NdL);
		
		vec3 reflectDir = reflect(-L, Normal);		
		float specularPow = 32;
        float specFactor = pow(max(dot(ViewDir, reflectDir), 0.0), specularPow) * 50;
        light += Specular * specFactor*Attenuation;
	}

	//if ( NumLights > 0 )
	//    light = vec3(P.x/16.0, P.y/8.0, floor(Slice)/24);


    return light;
}
#endif // FRAGMENT_SHADER && MATERIAL_PASS_COLOR

// Built-in samplers
#include "$COLOR_PASS_FRAGMENT_SAMPLERS$"

// Built-in varyings
#include "$COLOR_PASS_FRAGMENT_INPUT_VARYINGS$"

layout( location = 0 ) out vec4 FS_FragColor;

#ifdef USE_LIGHTMAP
layout( binding = LIGHTMAP_SLOT ) uniform sampler2D tslot_lightmap;
layout( location = BAKED_LIGHT_LOCATION ) in vec2 VS_LightmapTexCoord;
#endif

#ifdef USE_VERTEX_LIGHT
layout( location = BAKED_LIGHT_LOCATION ) in vec3 VS_VertexLight;
#endif

#ifdef COMPUTE_TBN
layout( location = TANGENT_LOCATION  ) in vec3 VS_T;
layout( location = BINORMAL_LOCATION ) in vec3 VS_B;
layout( location = NORMAL_LOCATION   ) in vec3 VS_N;
layout( location = POSITION_LOCATION ) in vec3 VS_Position;
#endif  // COMPUTE_TBN

void main() {

#ifdef COMPUTE_TBN
    InViewspaceToEyeVec = normalize( -VS_Position );
#endif

    vec2  InScreenCoord           = gl_FragCoord.xy;
    float InScreenDepth           = gl_FragCoord.z;
    vec2  InNormalizedScreenCoord = InScreenCoord * ViewportParams.xy;

    // Built-in material code
    #include "$COLOR_PASS_FRAGMENT_CODE$"

#ifdef MATERIAL_TYPE_PBR
//MaterialNormal=normalize(MaterialNormal.bgr);
//MaterialNormal=vec3(0,0,1); // just for test

    vec3 Light = vec3(0);

    // Compute macro normal
    const vec3 Normal = normalize( MaterialNormal.x * VS_T + MaterialNormal.y * VS_B + MaterialNormal.z * VS_N );

    // Lerp with metallic value to find the good diffuse and specular.
    const vec3 RealAlbedo = BaseColor.rgb - BaseColor.rgb * MaterialMetallic;

    // Compute diffuse
    const vec3 Diff = Diffuse( RealAlbedo );

    // 0.03 default specular value for dielectric.
    const vec3 RealSpecularColor = mix( vec3( 0.03 ), BaseColor.rgb, vec3( MaterialMetallic ) );

    // Reflected vector
    const vec3 R = normalize( reflect( -InViewspaceToEyeVec, Normal ) );

    const float SqrRoughness = MaterialRoughness * MaterialRoughness;
    const float SqrRoughnessClamped = max( 0.001, SqrRoughness );
    const float NdV = saturate( dot( Normal, InViewspaceToEyeVec ) );

	vec4 ClipspacePosition = vec4( InNormalizedScreenCoord * 2.0 - 1.0, InScreenDepth, 1.0 );
	
    //const vec2 FragCoord = vec2( InNormalizedScreenCoord.x, 1.0 - InNormalizedScreenCoord.y );
	const vec2 FragCoord = vec2( InNormalizedScreenCoord.x, InNormalizedScreenCoord.y );

    #ifdef USE_LIGHTMAP
    // TODO: lightmaps for PBR
    vec4 lm = texture( tslot_lightmap, VS_LightmapTexCoord );
    Light += BaseColor.rgb*lm.rgb;
    #endif

    #ifdef USE_VERTEX_LIGHT
    Light += VS_VertexLight;
    #endif

    Light += CalcDirectionalLightingPBR( ClipspacePosition, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, VS_N );
    Light += CalcPointLightLightingPBR( VS_Position, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, FragCoord );
	//Light += MaterialAmbientLight;

    const vec3 EnvFresnel = Specular_F_Roughness( RealSpecularColor, SqrRoughness, NdV );

    samplerCubeArray EnvProbeArray = samplerCubeArray(EnvProbeSampler);
    int ArrayIndex = 0;

    vec3 EnvColor = vec3(0);
    vec3 Irradiance = vec3(0);

    #define ENV_MAP_MAX_LOD 7

    const mat3 NormalToWorldSpace = (mat3(vec3(WorldNormalToViewSpace0),vec3(WorldNormalToViewSpace1),vec3(WorldNormalToViewSpace2))); // TODO: Optimize this!

    vec3 ReflectionVector = NormalToWorldSpace*R;
    float MipIndex = SqrRoughness * ENV_MAP_MAX_LOD;

    vec3 EnvProbeColor = vec3(1.0);

    EnvColor += textureLod( EnvProbeArray, vec4( ReflectionVector, ArrayIndex ), MipIndex ).xyz * EnvProbeColor;
    Irradiance += textureLod( EnvProbeArray, vec4( NormalToWorldSpace*Normal, ArrayIndex ), (ENV_MAP_MAX_LOD * 0.6) ).xyz * EnvProbeColor;

    // TODO: EnvFresnel записывать в одну текстуру, а EnvColor в другую. MaterialAmbientOcclusion тоже можно записать
    // в отдельную текстуру. Отдельным проходом мешать EnvColor со Screen-Space reflections
    vec3 ReflectedEnvLight = EnvFresnel * EnvColor * MaterialAmbientOcclusion;

    Light = Light/*  * MaterialAmbientOcclusion*/ + MaterialEmissive + RealAlbedo*Irradiance*MaterialAmbientOcclusion + ReflectedEnvLight;

    FS_FragColor = vec4( Light, Opacity );

#endif // MATERIAL_TYPE_PBR

#ifdef MATERIAL_TYPE_BASELIGHT

//MaterialNormal=vec3(0,0,1); // just for test

	vec4 ClipspacePosition = vec4( InNormalizedScreenCoord * 2.0 - 1.0, InScreenDepth, 1.0 );
	
    const vec2 FragCoord = vec2( InNormalizedScreenCoord.x, InNormalizedScreenCoord.y );

    vec3 Light = vec3(0);

    #ifdef USE_LIGHTMAP
    vec4 lm = texture( tslot_lightmap, VS_LightmapTexCoord );
    Light += lm.rgb;
    #endif

    #ifdef USE_VERTEX_LIGHT
    Light += VS_VertexLight;
    #endif

    // Compute macro normal
    const vec3 Normal = normalize( MaterialNormal.x * VS_T + MaterialNormal.y * VS_B + MaterialNormal.z * VS_N );
	
    Light += CalcDirectionalLighting( ClipspacePosition, Normal, InViewspaceToEyeVec, VS_N, MaterialSpecular );
    Light += CalcPointLightLighting( VS_Position, Normal, InViewspaceToEyeVec, FragCoord, MaterialSpecular );
    Light += MaterialAmbientLight;
    Light *= BaseColor.xyz;
    Light += MaterialEmissive;

    FS_FragColor = vec4( Light, Opacity );

#endif // MATERIAL_TYPE_BASELIGHT

#ifdef MATERIAL_TYPE_UNLIT
    FS_FragColor = vec4( BaseColor.xyz, Opacity );
#endif // MATERIAL_TYPE_UNLIT

#ifdef MATERIAL_TYPE_HUD
    FS_FragColor = BaseColor;
#endif // MATERIAL_TYPE_HUD

#ifdef MATERIAL_TYPE_POSTPROCESS
    FS_FragColor = BaseColor;
#endif // MATERIAL_TYPE_POSTPROCESS

#ifdef DEBUG_RENDER_MODE
    uint DebugMode = NumDirectionalLights.w;

    switch( DebugMode ) {
    case DEBUG_FULLBRIGHT:
        FS_FragColor = vec4( BaseColor.xyz, 1.0 );
        break;
    case DEBUG_NORMAL:
#if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( Normal*0.5+0.5, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_METALLIC:
#if defined MATERIAL_TYPE_PBR
        FS_FragColor = vec4( vec3( MaterialMetallic ), 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_ROUGHNESS:
#if defined MATERIAL_TYPE_PBR
        FS_FragColor = vec4( vec3( MaterialRoughness ), 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_AMBIENT:
#if defined MATERIAL_TYPE_PBR
        FS_FragColor = vec4( vec3( MaterialAmbientOcclusion ), 1.0 );
#else
        FS_FragColor = vec4( 1.0, 1.0, 1.0, 1.0 );
#endif
        break;
    case DEBUG_EMISSION:
#if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( MaterialEmissive, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_LIGHTMAP:
#if ( defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT ) && defined USE_LIGHTMAP
        FS_FragColor = vec4( lm.rgb, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
#if ( defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT )
        FS_FragColor.xyz += MaterialAmbientLight;
#endif
        break;
    case DEBUG_VERTEX_LIGHT:
#if ( defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT ) && defined USE_VERTEX_LIGHT
        FS_FragColor = vec4( VS_VertexLight, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_DIRLIGHT:
#if defined MATERIAL_TYPE_PBR
        FS_FragColor = vec4( CalcDirectionalLightingPBR( ClipspacePosition, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, VS_N ), 1.0 );
#elif defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( CalcDirectionalLighting( ClipspacePosition, Normal, InViewspaceToEyeVec, VS_N, MaterialSpecular ), 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_POINTLIGHT:
#if defined MATERIAL_TYPE_PBR
        FS_FragColor = vec4( CalcPointLightLightingPBR( VS_Position, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, FragCoord ), 1.0 );
#elif defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( CalcPointLightLighting( VS_Position, Normal, InViewspaceToEyeVec, FragCoord, MaterialSpecular ), 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    //case DEBUG_TEXCOORDS:
    //    FS_FragColor = vec4( nsv_VS0_TexCoord.xy, 0.0, 1.0 );
    //    break;
    case DEBUG_TEXNORMAL:
#if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( MaterialNormal*0.5+0.5, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_TBN_NORMAL:
#if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( VS_N*0.5+0.5, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_TBN_TANGENT:
#if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( VS_T*0.5+0.5, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_TBN_BINORMAL:
#if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
        FS_FragColor = vec4( VS_B*0.5+0.5, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
	case DEBUG_SPECULAR:
		FS_FragColor = vec4( 0.0 );
#if defined MATERIAL_TYPE_BASELIGHT
		FS_FragColor = vec4( MaterialSpecular, 1.0 );
#endif
		break;
    }
#endif // DEBUG_RENDER_MODE

	//FS_FragColor.rgb = vec3(pow(texture( ShadowMapShadow, vec3(InNormalizedScreenCoord,3.0) ).r,1.0));
	
#if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
#if 0
    float InNearZ                 = ViewportParams.z;
    float InFarZ                  = ViewportParams.w;
    float InLinearScreenDepth     = ComputeLinearDepth( InScreenDepth, InNearZ, InFarZ );

    // Расчет Scale и Bias ////////////////////////////////////////////////////////
    // TODO: вынести Scale и Bias в юниформы
    #define NUM_CLUSTERS_Z 24
    const float ZNear = 0.0125f;
    const float ZFar = 512;
    const int NearOffset = 20;
    const float Scale = ( NUM_CLUSTERS_Z + NearOffset ) / log2( ZFar / ZNear );
    const float Bias = -log2( ZNear ) * Scale - NearOffset;
    ///////////////////////////////////////////////////////////////////////////////

    // Получаем кластер ///////////////////////////////////////////////////////////
    const float Slice = max( 0.0, floor( log2( InLinearScreenDepth ) * Scale + Bias ) );
    const ivec3 P = ivec3( FragCoord.x * 16, FragCoord.y * 8, Slice );
	
	const uvec2 Cluster = texelFetch( ClusterLookup, P, 0 ).xy;
	

				///////////////////////////////////////////////////////////////////////////////

				// Распаковываем кластер //////////////////////////////////////////////////////
				const int NumProbes = int( Cluster.y & 0xff );
				const int NumDecals = int( ( Cluster.y >> 8 ) & 0xff );
				const int NumLights = int( ( Cluster.y >> 16 ) & 0xff );
				//const int Unused = int( ( Cluster.y >> 24 ) & 0xff ); // can be used in future
				///////////////////////////////////////////////////////////////////////////////
				
				if ( NumLights > 0 || NumDecals > 0 || NumProbes > 0 ) {
					FS_FragColor.rgb = vec3(P.x/16.0, P.y/8.0, floor(Slice)/24);
				}
#endif
#endif
}
