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

vec3 ComputeLight( in vec4 LightColor, in vec3 Diffuse, in vec3 RealSpecularColor, in float SqrRoughnessClamped, in vec3 Normal, in vec3 L, in float NdL, in float NdV, in float Shadow ) {
    float AmbientLightIntensity = LightColor.w; // UNUSED!!!

    vec3 H = normalize( L + InViewspaceToEyeVec );
    float NdH = saturate( dot( Normal, H ) );
    float VdH = saturate( dot( InViewspaceToEyeVec, H ) );
    float LdV = saturate( dot( L, InViewspaceToEyeVec ) );

    vec3 Spec = Specular( RealSpecularColor, SqrRoughnessClamped, NdL, NdV, NdH, VdH, LdV );

    return LightColor.xyz * Shadow * /* NdL * */( Diffuse * ( 1.0 - Spec ) + Spec ); // FIXME: Почему 1 - Spec а не (1.0 - Specular_F)
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
                
    //          if ( NumLights > 0 || NumDecals > 0 || NumProbes > 0 ) {
    //              FS_FragColor.rgb = vec3(P.x/16.0, P.y/8.0, floor(Slice)/24);
    //          }
    
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

void MaterialPBRShader( vec3 BaseColor, vec3 MaterialNormal, float MaterialMetallic, float MaterialRoughness, vec3 MaterialEmissive, float MaterialAmbientOcclusion, float Opacity )
{
    vec3 Light = vec3(0);

    // Compute macro normal
    const vec3 Normal = normalize( MaterialNormal.x * VS_T + MaterialNormal.y * VS_B + MaterialNormal.z * VS_N );

    // Lerp with metallic value to find the good diffuse and specular.
    const vec3 RealAlbedo = BaseColor - BaseColor * MaterialMetallic;

    // Compute diffuse
    const vec3 Diff = Diffuse( RealAlbedo );

    // 0.03 default specular value for dielectric.
    const vec3 RealSpecularColor = mix( vec3( 0.03 ), BaseColor, vec3( MaterialMetallic ) );

    // Reflected vector
    const vec3 R = normalize( reflect( -InViewspaceToEyeVec, Normal ) );

    const float SqrRoughness = MaterialRoughness * MaterialRoughness;
    const float SqrRoughnessClamped = max( 0.001, SqrRoughness );
    const float NdV = saturate( dot( Normal, InViewspaceToEyeVec ) );

    #ifdef USE_LIGHTMAP
    // TODO: lightmaps for PBR
    vec4 lm = texture( tslot_lightmap, VS_LightmapTexCoord );
    Light += BaseColor*lm.rgb;
    #endif

    #ifdef USE_VERTEX_LIGHT
    Light += VS_VertexLight;
    #endif

    Light += CalcDirectionalLightingPBR( InClipspacePosition, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, VS_N );
    Light += CalcPointLightLightingPBR( VS_Position, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, InNormalizedScreenCoord );
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
    
    
    
#ifdef DEBUG_RENDER_MODE
    uint DebugMode = NumDirectionalLights.w;

    switch( DebugMode ) {
    case DEBUG_FULLBRIGHT:
        FS_FragColor = vec4( BaseColor.xyz, 1.0 );
        break;
    case DEBUG_NORMAL:
        FS_FragColor = vec4( Normal*0.5+0.5, 1.0 );
        break;
    case DEBUG_METALLIC:
        FS_FragColor = vec4( vec3( MaterialMetallic ), 1.0 );
        break;
    case DEBUG_ROUGHNESS:
        FS_FragColor = vec4( vec3( MaterialRoughness ), 1.0 );
        break;
    case DEBUG_AMBIENT:
        FS_FragColor = vec4( vec3( MaterialAmbientOcclusion ), 1.0 );
        break;
    case DEBUG_EMISSION:
        FS_FragColor = vec4( MaterialEmissive, 1.0 );
        break;
    case DEBUG_LIGHTMAP:
#ifdef USE_LIGHTMAP
        FS_FragColor = vec4( lm.rgb, 1.0 );
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
        FS_FragColor = vec4( CalcDirectionalLightingPBR( InClipspacePosition, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, VS_N ), 1.0 );
        break;
    case DEBUG_POINTLIGHT:
        FS_FragColor = vec4( CalcPointLightLightingPBR( VS_Position, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, InNormalizedScreenCoord ), 1.0 );
        break;
    //case DEBUG_TEXCOORDS:
    //    FS_FragColor = vec4( nsv_VS0_TexCoord.xy, 0.0, 1.0 );
    //    break;
    case DEBUG_TEXNORMAL:
        FS_FragColor = vec4( MaterialNormal*0.5+0.5, 1.0 );
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
        FS_FragColor = vec4( 0.0 );
        break;
    }
#endif // DEBUG_RENDER_MODE
}
