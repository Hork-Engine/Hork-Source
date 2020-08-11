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

#include "material_shadowmap.frag"
#include "sslr.frag"

float DistributionGGX( float RoughnessSqr, float NdH ) {
    const float a2 = RoughnessSqr * RoughnessSqr; // TODO: Precompute once per-fragment!
    const float NdH2 = NdH * NdH;

    float denominator = mad( NdH2, a2 - 1.0, 1.0 );
    denominator *= denominator;
    denominator *= PI;  // TODO: мы можем вынести деление на PI и делить только один раз: ( L1 + L2 + .. + Ln ) / PI, вместо Diffuse использовать Albedo, т.к. Diffuse=Albedo/PI

    return a2 / denominator;
}

// float k = Sqr( Roughness ) * 0.5f; //  IBL
// float k = Sqr( Roughness + 1 ) * 0.125; // Direct light
float GeometrySmith( float NdV, float NdL, float k ) {
    const float invK = 1.0 - k;

    // Geometry schlick GGX for V
    const float ggxV = NdV / mad(NdV, invK, k); // TODO: Precompute once per-fragment!

    // Geometry schlick GGX for L
    const float ggxL = NdL / mad(NdL, invK, k);

    return ggxV * ggxL;
}

vec3 FresnelSchlick( vec3 F0, float VdH ) {
    return mad( pow( 1.0 - VdH, 5.0 ), 1.0 - F0, F0 );
}

vec3 FresnelSchlick_Roughness( vec3 F0, float NdV, float Roughness ) {
    vec3 a = max( vec3( 1.0 - Roughness ), F0 );
    
    return mad( pow( 1.0 - NdV, 5.0 ), a - F0, F0 );
}

vec3 LightBRDF( vec3 Diffuse, vec3 F0, float RoughnessSqr, vec3 Normal, vec3 L, float NdL, float NdV, float k ) {
    const vec3 H = normalize( L + InViewspaceToEyeVec );
    const float NdH = saturate( dot( Normal, H ) );
    const float VdH = saturate( dot( InViewspaceToEyeVec, H ) );
    const float LdV = saturate( dot( L, InViewspaceToEyeVec ) );
        
    // Cook-Torrance BRDF
    const float D = DistributionGGX( RoughnessSqr, NdH );
    const vec3 F = FresnelSchlick( F0, VdH );
    const float G = GeometrySmith( NdV, NdL, k );

    const vec3 Specular = F * ( D * G / mad( NdL * NdV, 4.0, 0.0001 ) );// * FetchLocalReflection(R);
    
    const vec3 kD = vec3(1.0) - F;
    
    return mad( kD, Diffuse, Specular );
}

vec3 CalcAmbient( vec3 Albedo, vec3 R, vec3 N, float NdV, vec3 F0, float Roughness, float AO, uint FirstIndex, uint NumProbes ) {
#if defined WITH_SSAO && defined ALLOW_SSAO
    // Sample ambient occlusion
    AO *= textureLod( AOLookup, InScreenUV, 0.0 ).x;
#endif
    
    // Calc fresnel
    const vec3 F = FresnelSchlick_Roughness( F0, NdV, Roughness );
    //const vec3 F = InNormalizedScreenCoord.x > 0.5 ? FresnelSchlick_Roughness( F0, NdV, Roughness )
    //                                         : FresnelSchlick_Roughness( F0, NdV, Roughness * Roughness );
    
    // Calc diffuse coef
    const vec3 kD = vec3( 1.0 ) - F;
    
    // Sample BRDF
    const vec2 EnvBRDF = texture( LookupBRDF, vec2( NdV, Roughness ) ).rg;

    // Transform vector from view space to world space
    const mat3 VectorTransformWS = mat3( vec3(WorldNormalToViewSpace0),
                                            vec3(WorldNormalToViewSpace1),
                                            vec3(WorldNormalToViewSpace2)
                                           ); // TODO: Optimize this!

    // Calc sample vectors
    const vec3 ReflectionVector = VectorTransformWS * R;
    const vec3 Normal = VectorTransformWS * N;
    
    // Calc sample lod
    #define ENV_MAP_MAX_LOD 7
    const float MipIndex = Roughness * ENV_MAP_MAX_LOD;
    //const float MipIndex = InNormalizedScreenCoord.x > 0.5 ? Roughness * ENV_MAP_MAX_LOD
    //                                                       : Roughness * Roughness * ENV_MAP_MAX_LOD;
    
    samplerCubeArray IrradianceMap = samplerCubeArray(IrradianceMapSampler); // TODO: Stop using bindless
    samplerCubeArray PrefilteredMap = samplerCubeArray(PrefilteredMapSampler); // TODO: Stop using bindless
    
    vec3 Irradiance = vec3( 0.0 );
    vec3 PrefilteredColor = vec3( 0.0 );
    
    float MinDist = 9999;
    uint NearestProbe = 9999;
    
    for ( int i = 0 ; i < NumProbes ; i++ ) {
        const uint Indices = texelFetch( ClusterItemTBO, int( FirstIndex + i ) ).x;
        const uint ProbeIndex = Indices >> 24;
        
        const float Radius = Probes[ ProbeIndex ].PositionAndRadius.w;
        const vec3 Vec = Probes[ ProbeIndex ].PositionAndRadius.xyz - VS_Position;
        
        const float DistSqr = dot( Vec, Vec );
        
        if ( DistSqr <= Radius * Radius ) {
        
            if ( MinDist > DistSqr ) {
                MinDist = DistSqr;
                NearestProbe = ProbeIndex;
            }
        }
    }
    
    if ( NearestProbe < 9999 ) {
        // Gather irradiance from cubemaps
        Irradiance += texture( IrradianceMap, vec4( Normal, Probes[NearestProbe].IrradianceAndReflectionMaps.x ) ).rgb;
        
        // Gather prefiltered maps from cubemaps
        PrefilteredColor += textureLod( PrefilteredMap, vec4( ReflectionVector, Probes[NearestProbe].IrradianceAndReflectionMaps.y ), MipIndex ).rgb;   
    }
    
    //Irradiance += vec3(0.01); // just for test
    
#if defined WITH_SSLR && defined ALLOW_SSLR
    PrefilteredColor += FetchLocalReflection( R, Roughness );
#endif

    const vec3 Diffuse = Irradiance * Albedo;
    const vec3 Specular = PrefilteredColor * ( F * EnvBRDF.x + EnvBRDF.y );
    
    return mad( kD, Diffuse, Specular ) * AO; 
}

vec3 CalcDirectionalLightingPBR( vec3 Diffuse, vec3 F0, float k, float RoughnessSqr, vec3 Normal, float NdV ) {
    vec3 Light = vec3(0.0);

    const uint NumLights = GetNumDirectionalLights();

    for ( int i = 0 ; i < NumLights ; ++i ) {
        //const uint RenderMask = LightParameters[ i ][ 0 ];

        vec3 L = LightDirs[ i ].xyz;

        float NdL = saturate( dot( Normal, L ) );

        if ( NdL > 0.0 ) {
            //float Bias = max( 1.0 - saturate( dot( VS_N, L ) ), 0.1 ) * 0.05;
            
            #if 0
            float Bias = tan( acos( saturate( dot( VS_N, L ) ) ) );
            #else
            // tan( acos( x ) ) = sqrt(1-x*x)/x
            float NdL_Vertex = saturate( dot( VS_N, L ) );
            float Bias = sqrt( 1.0 - NdL_Vertex*NdL_Vertex ) / max( NdL_Vertex, 0.001 );
            #endif
            
            Bias = min( Bias * 0.005, 0.01 );

#           ifdef ALLOW_SHADOW_RECEIVE            
            float Shadow = SampleLightShadow( LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ], Bias );
#           else
            const float Shadow = 1.0;
#           endif
            
            if ( Shadow > 0.0 ) {
            
                const float ParallaxSelfShadow = GetParallaxSelfShadow( L );
        
                Light += LightBRDF( Diffuse, F0, RoughnessSqr, Normal, L, NdL, NdV, k ) * LightColors[ i ].xyz * ( Shadow * NdL * ParallaxSelfShadow );
                
                //Light = vec3(ParallaxSelfShadow,0.0,0.0);
            }
        }
    }
    return Light;
}

#ifdef SUPPORT_PHOTOMETRIC_LIGHT
float CalcPhotometricAttenuation( float LdotDir, uint Profile ) {
//if ( InNormalizedScreenCoord.x < 0.5 ) {
//    const float angle = acos( LdotDir ) * (1.0 / PI);
//    return textureLod( IESMap, vec2(angle, Profile), 0.0 ).r;
//} else {
    return pow( textureLod( IESMap, vec2(LdotDir*0.5+0.5, Profile), 0.0 ).r, 2.2 );
    //return textureLod( IESMap, vec2(LdotDir*0.5+0.5, Profile), 0.0 ).r;
//}
}
#endif

vec3 CalcPointLightLightingPBR( vec3 Diffuse, vec3 F0, float k, float RoughnessSqr, vec3 Normal, float NdV, uint FirstIndex, uint NumLights ) {
    vec3 Light = vec3(0.0);

    #define POINT_LIGHT 0
    #define SPOT_LIGHT  1
    
    for ( int i = 0 ; i < NumLights ; i++ ) {
        const uint indices = texelFetch( ClusterItemTBO, int( FirstIndex + i ) ).x;
        const uint lightIndex = indices & 0x3ff;
        const uint lightType = GetLightType( lightIndex );

        const float OuterRadius = GetLightRadius( lightIndex );
        const vec3 Vec = GetLightPosition( lightIndex ) - VS_Position;
        const float Dist = length( Vec ); // NOTE: We can use dot(Vec,Vec) to compute DistSqr instead of Dist
        
        if ( Dist > 0.0 && Dist <= OuterRadius ) {
            const vec3 L = Vec / Dist; // normalize( Vec )
//            const float InnerRadius = GetLightInnerRadius( lightIndex );
            
            float Attenuation;
//            if ( InNormalizedScreenCoord.x > 1.0f-1.0f/4.0f ) {
//                Attenuation = CalcDistanceAttenuation( Dist, OuterRadius );
//            } else if ( InNormalizedScreenCoord.x > 1.0f-2.0f/4.0f ) {
                Attenuation = CalcDistanceAttenuationFrostbite( Dist*Dist, GetLightInverseSquareRadius( lightIndex ) );
//            } else if ( InNormalizedScreenCoord.x > 1.0f-3.0f/4.0f ) {                
//                Attenuation = CalcDistanceAttenuationUnreal( Dist, OuterRadius );
//            } else {
//                Attenuation = CalcDistanceAttenuationSkyforge( Dist, 0.0, OuterRadius );
//            }
            
            float Shadow = 1;
            
            float LdotDir = dot( L, GetLightDirection( lightIndex ) );

            if ( lightType == SPOT_LIGHT ) {
                Attenuation *= CalcSpotAttenuation( LdotDir,
                                                        GetLightCosHalfInnerConeAngle( lightIndex ),
                                                        GetLightCosHalfOuterConeAngle( lightIndex ),
                                                        GetLightSpotExponent( lightIndex )
                                                      );
            }

            #ifdef SUPPORT_PHOTOMETRIC_LIGHT
            
            uint PhotometricProfile = GetLightPhotometricProfile( lightIndex );
            
            if ( PhotometricProfile < 0xffffffff ) {
                Attenuation *= CalcPhotometricAttenuation( LdotDir, PhotometricProfile );// * GetLightIESScale( lightIndex );
            }
            
            #endif

            float NdL = saturate( dot( Normal, L ) );

            Attenuation *= NdL * Shadow;

            Light += LightBRDF( Diffuse, F0, RoughnessSqr, Normal, L, NdL, NdV, k ) * GetLightColor( lightIndex ) * Attenuation;
            
            //if (LdotDir<0.0) {Light=vec3(1,0,0);break;}
        }
    }

    return Light;
}

void GetClusterData( out uint numProbes, out uint numDecals, out uint numLights, out uint firstIndex )
{
    // TODO: Move scale and bias to uniforms
    #define NUM_CLUSTERS_Z 24
    const float znear = 0.0125f;
    const float zfar = 512;
    const int nearOffset = 20;
    const float scale = ( NUM_CLUSTERS_Z + nearOffset ) / log2( zfar / znear );
    const float bias = -log2( znear ) * scale - nearOffset;

    // Calc cluster index
    const float linearDepth = -VS_Position.z;
    const float slice = max( 0.0, floor( log2( linearDepth ) * scale + bias ) );
    const ivec3 clusterIndex = ivec3( InNormalizedScreenCoord.x * 16, InNormalizedScreenCoord.y * 8, slice );

    // Fetch packed data
    const uvec2 cluster = texelFetch( ClusterLookup, clusterIndex, 0 ).xy;

    // Unpack cluster data
    numProbes = cluster.y & 0xff;
    numDecals = ( cluster.y >> 8 ) & 0xff;
    numLights = ( cluster.y >> 16 ) & 0xff;
    //unused = ( cluster.y >> 24 ) & 0xff; // can be used in future

    firstIndex = cluster.x;
}

void MaterialPBRShader( vec3 BaseColor, vec3 N, float Metallic, float Roughness, vec3 Emissive, float AO, float Opacity )
{
    uint NumProbes;
    uint NumDecals;
    uint NumLights;
    uint FirstIndex;

    GetClusterData( NumProbes, NumDecals, NumLights, FirstIndex );
    
    // Calc macro normal
    #ifdef TWOSIDED
    const vec3 Normal = normalize( N.x * VS_T + N.y * VS_B + N.z * VS_N ) * ( gl_FrontFacing ? -1.0 : 1.0 );
    #else
    const vec3 Normal = normalize( N.x * VS_T + N.y * VS_B + N.z * VS_N );
    #endif

    // Lerp with metallic value to find the good diffuse and specular.
    const vec3 Albedo = BaseColor * ( 1.0 - Metallic );

    // Calc diffuse
    const vec3 Diffuse = Albedo / PI;

    // 0.03 default specular value for dielectric.
    // Для всех диэлектриков F0 не выше 0.17
    // Для металлов - от 0.5 до 1.0
    const vec3 F0 = mix( vec3( 0.03 ), BaseColor, vec3( Metallic ) ); // TODO: check 0.04

    // Reflected vector
    const vec3 R = normalize( reflect( -InViewspaceToEyeVec, Normal ) );

    const float RoughnessSqr = Roughness * Roughness;
    const float RoughnessSqrClamped = max( 0.001, RoughnessSqr );
    const float NdV = clamp( dot( Normal, InViewspaceToEyeVec ), 0.001, 1.0 );

    vec3 Light = Emissive;

#ifndef NO_LIGHTMAP
    #ifdef USE_LIGHTMAP
    // TODO: lightmaps for PBR
    const vec4 Lightmap = texture( tslot_lightmap, VS_LightmapTexCoord );
    Light += BaseColor * Lightmap.rgb;
    #endif

    #ifdef USE_VERTEX_LIGHT
    // Assume vertex has emission light
    Light += VS_VertexLight;
    #endif
#endif
    
    //const float k = RoughnessSqr * 0.5f; //  IBL
    const float k = (Roughness + 1) * (Roughness + 1) * 0.125; // Direct light
    
    // Accumulate directional lights
    Light += CalcDirectionalLightingPBR( Diffuse, F0, k, RoughnessSqrClamped, Normal, NdV );
    
    // Accumulate point and spot lights
    Light += CalcPointLightLightingPBR( Diffuse, F0, k, RoughnessSqrClamped, Normal, NdV, FirstIndex, NumLights );

    // Apply ambient
    const vec3 Ambient = CalcAmbient( Albedo, R, Normal, NdV, F0, Roughness, AO, FirstIndex, NumProbes );
    Light += Ambient;

    FS_FragColor = vec4( Light, Opacity );
    
    //FS_Normal = Normal*0.5+0.5;
    //FS_Normal = normalize(VS_N)*0.5+0.5;
    
    //FS_FragColor = vec4(texture( LookupBRDF, InNormalizedScreenCoord ).rg,0.0,1.0);
    
    //FS_FragColor.rgb = BaseColor + FetchLocalReflection( R, 0 );
    
    //FS_FragColor.rgb = vec3(-VS_Position.z);
    
    //FS_FragColor.rgb = BaseColor+FetchLocalReflection( R, 0 );
    
    //FS_FragColor.rgb = vec3( gl_FragDepth );
    
    //FS_FragColor.rgb = vec3(ViewToUV( VS_Position ),0);
    //FS_FragColor.rgb = vec3(InNormalizedScreenCoord,0);

    //FS_FragColor.rgb = vec3(UVToView( InNormalizedScreenCoord, VS_Position.z ));
    //FS_FragColor.rgb = VS_Position;
    
    //FS_FragColor.rgb = vec3(-VS_Position.z);
    
#ifdef DEBUG_RENDER_MODE
    uint DebugMode = GetDebugMode();

    switch( DebugMode ) {
    case DEBUG_FULLBRIGHT:
        FS_FragColor = vec4( BaseColor, 1.0 );
        break;
    case DEBUG_NORMAL:
        FS_FragColor = vec4( Normal*0.5+0.5, 1.0 );
        break;
    case DEBUG_METALLIC:
        FS_FragColor = vec4( vec3( Metallic ), 1.0 );
        break;
    case DEBUG_ROUGHNESS:
        FS_FragColor = vec4( vec3( Roughness ), 1.0 );
        break;
    case DEBUG_AMBIENT:
#if defined WITH_SSAO && defined ALLOW_SSAO
        AO *= textureLod( AOLookup, InScreenUV, 0.0 ).x;
#endif
        FS_FragColor = vec4( vec3( AO ), 1.0 );
        break;
    case DEBUG_EMISSION:
        FS_FragColor = vec4( Emissive, 1.0 );
        break;
    case DEBUG_LIGHTMAP:
#ifdef USE_LIGHTMAP
        FS_FragColor = vec4( Lightmap.rgb, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        //FS_FragColor.xyz += MaterialAmbientLight;
        break;
    case DEBUG_VERTEX_LIGHT:
#ifdef USE_VERTEX_LIGHT
        FS_FragColor = vec4( VS_VertexLight, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_DIRLIGHT:
        FS_FragColor = vec4( CalcDirectionalLightingPBR( vec3(0.5/PI), F0, k, RoughnessSqrClamped, Normal, NdV ), 1.0 );
        break;
    case DEBUG_POINTLIGHT:
        FS_FragColor = vec4( CalcPointLightLightingPBR( vec3(0.5/PI), F0, k, RoughnessSqrClamped, Normal, NdV, FirstIndex, NumLights ), 1.0 );
        break;
    //case DEBUG_TEXCOORDS:
    //    FS_FragColor = vec4( nsv_VS0_TexCoord.xy, 0.0, 1.0 );
    //    break;
    case DEBUG_TEXNORMAL:
        FS_FragColor = vec4( N*0.5+0.5, 1.0 );
        break;
    case DEBUG_TBN_NORMAL:
        FS_FragColor = vec4( VS_N*0.5+0.5, 1.0 );
        break;
    case DEBUG_TBN_TANGENT:
        FS_FragColor = vec4( VS_T*0.5+0.5, 1.0 );
        break;
    case DEBUG_TBN_BINORMAL:
        FS_FragColor = vec4( VS_B*0.5+0.5, 1.0 );
        break;
    case DEBUG_SPECULAR:
        FS_FragColor = vec4( F0, 1.0 );
        break;
    case DEBUG_AMBIENT_LIGHT:
        FS_FragColor = vec4( Ambient, 1.0 );
        break;
    case DEBUG_LIGHT_CASCADES:
        FS_FragColor = vec4( mix(FS_FragColor.rgb,DebugDirectionalLightCascades(),vec3(0.05)), 1.0 );
        break;
    case DEBUG_VT_BORDERS:
        #ifdef USE_VIRTUAL_TEXTURE
        const float PageResolutionB = 128;
        vec2 v = mod( InPhysicalUV * VTPageCacheCapacity.xy * PageResolutionB, PageResolutionB ) / PageResolutionB;
        
        float x = 0;
        vec2 s = abs(pow(smoothstep(vec2(x),vec2(1.0-x),fract(v+0.5)),vec2(1.0))-0.5);
        s = 1.0 - min(vec2(1.0), s*10.0);
        float t = max(s.x,s.y);

        FS_FragColor.rgb = mix( FS_FragColor.rgb, vec3(1,0.8,0), t );
        #endif
        break;
    case DEBUG_VELOCITY:
        FS_FragColor = vec4( abs(FS_Velocity), 0.0, 1.0 );
        break;
    }
#endif // DEBUG_RENDER_MODE
}
