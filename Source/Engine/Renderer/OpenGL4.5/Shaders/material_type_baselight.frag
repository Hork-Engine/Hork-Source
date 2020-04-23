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

vec3 CalcDirectionalLighting( in vec3 Normal, in vec3 Specular ) {
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

            shadow = NdL * SampleLightShadow( InClipspacePosition, LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ], bias );

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
        float specFactor = pow( max( dot( InViewspaceToEyeVec, reflectDir ), 0.0 ), specularPow ) * 10;
        light += Specular * specFactor;//*attenuation;
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
    numProbes = ( cluster.y & 0xff );
    numDecals = ( ( cluster.y >> 8 ) & 0xff );
    numLights = ( ( cluster.y >> 16 ) & 0xff );
    //unused = ( ( cluster.y >> 24 ) & 0xff ); // can be used in future

    firstIndex = cluster.x;
}

vec3 CalcPointLightLighting( in vec3 Normal, in vec3 Specular ) {
    uint numProbes;
    uint numDecals;
    uint numLights;
    uint firstIndex;
    vec3 light = vec3(0.0);

    float linearDepth = ComputeLinearDepth( InScreenDepth, GetViewportZNear(), GetViewportZFar() );

    GetClusterData( linearDepth, numProbes, numDecals, numLights, firstIndex );

    #define POINT_LIGHT 0
    #define SPOT_LIGHT  1

    for ( uint i = 0 ; i < numLights ; i++ ) {
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

        float attenuation;
        //attenuation = saturate( 1.0 - pow( d / outerRadius, 4.0 ) ) / ( d*d + 1.0 ); // from skyforge
        //attenuation = pow( saturate( 1.0 - pow( lightDist / outerRadius, 4.0 ) ), 2.0 );// / (lightDist*lightDist + 1.0 );   // from Unreal
        attenuation = pow( 1.0 - min( lightDist / outerRadius, 1.0 ), 2.2 );   // My attenuation

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

            //if ( ( renderMask & Material.ShadowReceiveMask ) != 0 ) {
            //    // TODO: // Sample spot light shadows
            //    shadow = 1;
            //} else {
            //    shadow = 1;
            //}
            break;
        }

        float NdL = saturate( dot( Normal, lightVec ) );

        attenuation *= shadow;

        light += LightBuffer[ lightIndex ].Color.rgb * ( attenuation * NdL );
        
        vec3 reflectDir = reflect(-lightVec, Normal);
        float specularPow = 32;
        float specFactor = pow(max(dot(InViewspaceToEyeVec, reflectDir), 0.0), specularPow) * 50;
        light += Specular * specFactor * attenuation;
    }

    return light;
}

void MaterialBaseLightShader( vec3 BaseColor, vec3 MaterialNormal, vec3 MaterialSpecular, vec3 MaterialAmbientLight, vec3 MaterialEmissive, float Opacity )
{
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
    
    Light += CalcDirectionalLighting( Normal, MaterialSpecular );
    Light += CalcPointLightLighting( Normal, MaterialSpecular );
    Light += MaterialAmbientLight;
    Light *= BaseColor;
    Light += MaterialEmissive;

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
    case DEBUG_ROUGHNESS:
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
        break;
    case DEBUG_AMBIENT:
        FS_FragColor = vec4( 1.0, 1.0, 1.0, 1.0 );
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
        FS_FragColor.xyz += MaterialAmbientLight;
        break;
    case DEBUG_VERTEX_LIGHT:
#ifdef USE_VERTEX_LIGHT
        FS_FragColor = vec4( VS_VertexLight, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_DIRLIGHT:
        FS_FragColor = vec4( CalcDirectionalLighting( Normal, MaterialSpecular ), 1.0 );
        break;
    case DEBUG_POINTLIGHT:
        FS_FragColor = vec4( CalcPointLightLighting( Normal, MaterialSpecular ), 1.0 );
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
        FS_FragColor = vec4( MaterialSpecular, 1.0 );
        break;
    }
#endif // DEBUG_RENDER_MODE
}
