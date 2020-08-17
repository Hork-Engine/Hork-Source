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

#include "shading/shadowmap.frag"
#include "shading/sslr.frag"
#include "shading/unpack_cluster.frag"
#include "shading/photometric.frag"

#define SPECULAR_BRIGHTNESS 50.0

vec3 CalcDirectionalLighting( vec3 Normal, vec3 Specular, float SpecularPower )
{
    vec3 Light = vec3(0.0);

    const uint NumLights = GetNumDirectionalLights();

    for ( int i = 0 ; i < NumLights ; ++i ) {
        //const uint RenderMask = LightParameters[ i ][ 0 ];

        vec3 L = LightDirs[ i ].xyz;

        float NdL = saturate( dot( Normal, L ) );

        if ( NdL > 0.0 ) {
        
#           ifdef ALLOW_SHADOW_RECEIVE
            float Bias = max( 1.0 - saturate( dot( VS_N, L ) ), 0.1 ) * 0.05;
            float Shadow = SampleLightShadow( LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ], Bias );
#           else
            const float Shadow = 1.0;
#           endif

            if ( Shadow > 0.0 ) {
                const float ParallaxSelfShadow = GetParallaxSelfShadow( L );
            
                Light += LightColors[ i ].xyz * ( Shadow * NdL * ParallaxSelfShadow);

                // Directional light specular
                vec3 R = reflect(-L, Normal);       
                float SpecFactor = pow( saturate( dot( InViewspaceToEyeVec, R ) ), SpecularPower ) * SPECULAR_BRIGHTNESS;
                Light += Specular * SpecFactor;
            }
        }
    }

    return Light;
}

vec3 CalcPointLightLighting( vec3 Normal, vec3 Specular, float SpecularPower )
{
    uint NumProbes;
    uint NumDecals;
    uint NumLights;
    uint FirstIndex;
    vec3 Light = vec3(0.0);

    UnpackCluster( NumProbes, NumDecals, NumLights, FirstIndex );

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
//              Attenuation = CalcDistanceAttenuationUnreal( Dist, OuterRadius );
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

            Attenuation *= Shadow;

            Light += GetLightColor( lightIndex ) * ( Attenuation * NdL );
        
            vec3 R = reflect(-L, Normal);
            float SpecFactor = pow( saturate( dot( InViewspaceToEyeVec, R ) ), SpecularPower ) * SPECULAR_BRIGHTNESS;
            Light += Specular * ( SpecFactor * Attenuation );
        }
    }

    return Light;
}

void MaterialBaseLightShader( vec3 BaseColor,
                              vec3 N,
                              vec3 Specular,
                              float SpecularPower,
                              vec3 AmbientLight,
                              vec3 Emissive,
                              float Opacity )
{
    vec3 Light = vec3(0);

#ifndef NO_LIGHTMAP
    #ifdef USE_LIGHTMAP
    vec4 lm = texture( tslot_lightmap, VS_LightmapTexCoord );
    Light += lm.rgb;
    #endif

    #ifdef USE_VERTEX_LIGHT
    Light += VS_VertexLight;
    #endif
#endif

    // Compute macro normal
    #ifdef TWOSIDED
    const vec3 Normal = normalize( N.x * VS_T + N.y * VS_B + N.z * VS_N ) * ( 1.0 - float(gl_FrontFacing) * 2.0 );
    #else
    const vec3 Normal = normalize( N.x * VS_T + N.y * VS_B + N.z * VS_N );
    #endif
    
    Light += CalcDirectionalLighting( Normal, Specular, SpecularPower );
    Light += CalcPointLightLighting( Normal, Specular, SpecularPower );
    Light += AmbientLight;
    
#if defined WITH_SSAO && defined ALLOW_SSAO
    float AO = Opacity < 1.0 ? 1.0 : textureLod( AOLookup, InScreenUV, 0.0 ).x;
#else
    float AO = 1.0;
#endif
    
    Light *= AO;
    
    Light *= BaseColor;
    Light += Emissive;

    // Get roughness from specular
    float Roughness = 1.0 - saturate(builtin_luminance(Specular));
    Roughness = Roughness*Roughness;

#if defined WITH_SSLR && defined ALLOW_SSLR
    Light += FetchLocalReflection( normalize( reflect( -InViewspaceToEyeVec, Normal ) ), Roughness );
#endif
    
    FS_FragColor = vec4( Light, Opacity );
    
    //FS_Normal = Normal*0.5+0.5;
    //FS_Normal = normalize(VS_N)*0.5+0.5;
    
    //vec2 UV = vec2( InNormalizedScreenCoord.x, 1.0-InNormalizedScreenCoord.y );
    //FS_FragColor.rgb = vec3(UV,0);
    //FS_FragColor.rgb = vec3(ViewToUV( VS_Position ),0);
    //FS_FragColor.rgb = vec3(UVToView( UV, VS_Position.z ));
    //FS_FragColor.rgb = VS_Position;
    
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
        FS_FragColor = vec4( AO, AO, AO, 1.0 );
        break;
    case DEBUG_EMISSION:
        FS_FragColor = vec4( Emissive, 1.0 );
        break;
    case DEBUG_LIGHTMAP:
#if defined USE_LIGHTMAP && !defined NO_LIGHTMAP
        FS_FragColor = vec4( lm.rgb, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        FS_FragColor.xyz += AmbientLight;
        break;
    case DEBUG_VERTEX_LIGHT:
#ifdef USE_VERTEX_LIGHT
        FS_FragColor = vec4( VS_VertexLight, 1.0 );
#else
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
        break;
    case DEBUG_DIRLIGHT:
        FS_FragColor = vec4( CalcDirectionalLighting( Normal, Specular, SpecularPower ), 1.0 );
        break;
    case DEBUG_POINTLIGHT:
        FS_FragColor = vec4( CalcPointLightLighting( Normal, Specular, SpecularPower ), 1.0 );
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
        FS_FragColor = vec4( Specular, 1.0 );
        break;
    case DEBUG_AMBIENT_LIGHT:
        FS_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
        break;
    case DEBUG_LIGHT_CASCADES:
        FS_FragColor = vec4( (FS_FragColor.rgb+DebugDirectionalLightCascades()) * 0.5, 1.0 );
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
        #if defined( WITH_MOTION_BLUR ) && !defined( TRANSLUCENT ) && defined( ALLOW_MOTION_BLUR )
        FS_FragColor = vec4( abs(FS_Velocity), 0.0, 1.0 );
        #endif
        break;
    }
#endif // DEBUG_RENDER_MODE
}
