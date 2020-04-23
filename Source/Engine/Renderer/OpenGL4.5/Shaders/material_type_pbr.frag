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

vec3 CalcDirectionalLightingPBR( in vec3 Diff, in vec3 RealSpecularColor, in float SqrRoughnessClamped, in vec3 Normal, in float NdV ) {
    vec3 light = vec3(0.0);

    uint numLights = GetNumDirectionalLights();

    for ( int i = 0 ; i < numLights ; ++i ) {
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

            float bias = max( 1.0 - max( dot( VS_N, lightDir ), 0.0 ), 0.1 ) * 0.05;
    
            //shadow = NdL * SampleLightShadow( InClipspacePosition, LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ] );
            shadow = NdL * SampleLightShadow( InClipspacePosition, LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ], bias );

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

void GetClusterData( in float linearDepth, out uint numProbes, out uint numDecals, out uint numLights, out uint firstIndex )
{
    // TODO: Move scale and bias to uniforms
    #define NUM_CLUSTERS_Z 24
    const float znear = 0.0125f;
    const float zfar = 512;
    const int nearOffset = 20;
    const float scale = ( NUM_CLUSTERS_Z + nearOffset ) / log2( zfar / znear );
    const float bias = -log2( znear ) * scale - nearOffset;

    // Calc cluster index
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

vec3 CalcPointLightLightingPBR( in vec3 Diff, in vec3 RealSpecularColor, in float SqrRoughnessClamped, in vec3 Normal, in float NdV/*, in vec3 ViewDir*/, in vec2 FragCoord/*, in vec3 Specular*/ ) {
    uint numProbes;
    uint numDecals;
    uint numLights;
    uint firstIndex;
    vec3 light = vec3(0.0);

    float linearDepth = ComputeLinearDepth( InScreenDepth, GetViewportZNear(), GetViewportZFar() );

    GetClusterData( linearDepth, numProbes, numDecals, numLights, firstIndex );

    #define POINT_LIGHT 0
    #define SPOT_LIGHT  1
    
    for ( int i = 0 ; i < numLights ; i++ ) {
        const uint indices = texelFetch( ClusterItemTBO, int( firstIndex + i ) ).x;
        const uint lightIndex = indices & 0x3ff;
        const uint lightType = uint( LightBuffer[ lightIndex ].Pack2.x );
        
        //uint renderMask = LightBuffer[ lightIndex ].IPack.x;

        //if ( ( renderMask & Material.AffectedLightMask ) == 0 ) {
        //    continue;
        //}

        float outerRadius = LightBuffer[ lightIndex ].Pack.w;
        vec3 lightVec = LightBuffer[ lightIndex ].Pack.xyz - VS_Position;
        float lightDist = length( lightVec );
        
        if ( lightDist > outerRadius || lightDist == 0.0 ) {
            continue;
        }
        
        lightVec /= lightDist; // normalize
                
        float innerRadius = LightBuffer[ lightIndex ].Pack2.y;

        float d = max( innerRadius, lightDist );
        
        float attenuation = saturate( 1.0 - pow( d / outerRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
        //attenuation = pow( saturate( 1.0 - pow( lightDist / outerRadius, 4.0 ) ), 2.0 );// / (lightDist*lightDist + 1.0 );   // from Unreal
        //attenuation = pow( 1.0 - min( lightDist / outerRadius, 1.0 ), 2.2 );   // My attenuation
        
        float shadow = 1;
                
        switch ( lightType ) {
        case POINT_LIGHT:
            //if ( ( renderMask & Material.ShadowReceiveMask ) != 0 ) {
            //    // TODO: // Sample point light shadows
            //    shadow = 1;
            //} else {
            //    shadow = 1;
            //}
            break;
        case SPOT_LIGHT:
            float CosConeRay = dot( lightVec, LightBuffer[ lightIndex ].Pack3.xyz );
            float ConeFalloff = pow( smoothstep( LightBuffer[ lightIndex ].Pack2.z, LightBuffer[ lightIndex ].Pack2.w, CosConeRay ), LightBuffer[ lightIndex ].Pack3.w );

            attenuation *= ConeFalloff;
            attenuation=0;

            //if ( ( renderMask & Material.ShadowReceiveMask ) != 0 ) {
            //    // TODO: // Sample spot light shadows
            //    shadow = 1;
            //} else {
            //    shadow = 1;
            //}
            break;
        }

        float NdL = saturate( dot( Normal, lightVec ) );

        attenuation *= NdL * shadow;

        light += ComputeLight( LightBuffer[ lightIndex ].Color, Diff, RealSpecularColor, SqrRoughnessClamped, Normal, lightVec, NdL, NdV, attenuation );
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

    Light += CalcDirectionalLightingPBR( Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV );
    Light += CalcPointLightLightingPBR( Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, InNormalizedScreenCoord );
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
    uint DebugMode = GetDebugMode();

    switch( DebugMode ) {
    case DEBUG_FULLBRIGHT:
        FS_FragColor = vec4( BaseColor, 1.0 );
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
        FS_FragColor = vec4( CalcDirectionalLightingPBR( Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV ), 1.0 );
        break;
    case DEBUG_POINTLIGHT:
        FS_FragColor = vec4( CalcPointLightLightingPBR( Diff, RealSpecularColor, SqrRoughnessClamped, Normal, NdV, InNormalizedScreenCoord ), 1.0 );
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
