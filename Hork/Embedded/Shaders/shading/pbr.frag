/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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
#include "shading/cook_torrance_brdf.frag"
#include "shading/unpack_cluster.frag"
#include "shading/photometric.frag"

vec3 LightBRDF(vec3 Diffuse, vec3 F0, float RoughnessSqr, vec3 Normal, vec3 L, float NdL, float NdV, float k)
{
    const vec3 H = normalize(L + InViewspaceToEyeVec);
    const float NdH = saturate(dot(Normal, H));
    const float VdH = saturate(dot(InViewspaceToEyeVec, H));
    const float LdV = saturate(dot(L, InViewspaceToEyeVec));

    // Cook-Torrance BRDF
    const float D = DistributionGGX(RoughnessSqr, NdH);
    const vec3 F = FresnelSchlick(F0, VdH);
    const float G = GeometrySmith(NdV, NdL, k);

    const vec3 specular = F * (D * G / mad(NdL * NdV, 4.0, 0.0001)); // * FetchLocalReflection(R);

    const vec3 kD = vec3(1.0) - F;

    return mad(kD, Diffuse, specular);
}

vec3 CalcAmbient(vec3 Albedo, vec3 R, vec3 N, float NdV, vec3 F0, float Roughness, float AO, uint FirstIndex, uint NumProbes)
{
    //#if defined WITH_SSAO && defined ALLOW_SSAO
    //    // Sample ambient occlusion
    //    vec2 aotc = min( vec2(InScreenUV.x,1.0-InScreenUV.y), vec2(1.0) - GetViewportSizeInverted() ) * GetDynamicResolutionRatio();
    //    aotc.y = 1.0 - aotc.y;
    //    AO *= textureLod( AOLookup, aotc, 0.0 ).x;
    //#endif

    // Calc fresnel
    const vec3 F = FresnelSchlick_Roughness(F0, NdV, Roughness);
    //const vec3 F = InNormalizedScreenCoord.x > 0.5 ? FresnelSchlick_Roughness( F0, NdV, Roughness )
    //                                         : FresnelSchlick_Roughness( F0, NdV, Roughness * Roughness );

    // Calc diffuse coef
    const vec3 kD = vec3(1.0) - F;

    // Sample BRDF
    const vec2 envBRDF = texture(LookupBRDF, vec2(NdV, Roughness)).rg;

    // Transform vector from view space to world space
    const mat3 VectorTransformWS = mat3(vec3(WorldNormalToViewSpace0),
                                        vec3(WorldNormalToViewSpace1),
                                        vec3(WorldNormalToViewSpace2)); // TODO: Optimize this!

    // Calc sample vectors
    const vec3 reflectionVector = VectorTransformWS * R;
    const vec3 normal = VectorTransformWS * N;

    // Calc sample lod
#define ENV_MAP_MAX_LOD 7
    const float mipIndex = Roughness * ENV_MAP_MAX_LOD;
    //const float mipIndex = InNormalizedScreenCoord.x > 0.5 ? Roughness * ENV_MAP_MAX_LOD
    //                                                       : Roughness * Roughness * ENV_MAP_MAX_LOD;

    float minDist = 9999;
    uint nearestProbe = 9999;

    for (int i = 0; i < NumProbes; i++)
    {
        const uint indices = texelFetch(ClusterItemTBO, int(FirstIndex + i)).x;
        const uint probeIndex = indices >> 24;

        const float radius = Probes[probeIndex].PositionAndRadius.w;
        const vec3 dir = Probes[probeIndex].PositionAndRadius.xyz - VS_Position;

        const float distSqr = dot(dir, dir);

        if (distSqr <= radius * radius)
        {
            if (minDist > distSqr)
            {
                minDist = distSqr;
                nearestProbe = probeIndex;
            }
        }
    }

    vec3 irradiance = vec3(0);
    vec3 prefilteredColor = vec3(0);

    // NOTE: there is an issue
#if 0
    if (nearestProbe < 9999)
    {
        // Gather irradiance from cubemaps
        irradiance += texture(samplerCube(Probes[nearestProbe].IrradianceAndReflectionMaps.xy), normal).rgb;

        // Gather prefiltered maps from cubemaps
        prefilteredColor += textureLod(samplerCube(Probes[nearestProbe].IrradianceAndReflectionMaps.zw), reflectionVector, mipIndex).rgb;   
    }
    else
    {
        // Gather irradiance from cubemaps
        irradiance += texture(samplerCube(GlobalIrradianceAndReflection.xy), normal).rgb;

        // Gather prefiltered maps from cubemaps
        prefilteredColor += textureLod(samplerCube(GlobalIrradianceAndReflection.zw), reflectionVector, mipIndex).rgb;
    }
#endif

#if defined WITH_SSLR && defined ALLOW_SSLR
    prefilteredColor += FetchLocalReflection(R, Roughness);
#endif

    irradiance = max(irradiance, vec3(WorldAmbient));

    const vec3 diffuse = irradiance * Albedo;
    const vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    return mad(kD, diffuse, specular);// * AO;
}

vec3 CalcDirectionalLightingPBR(vec3 Diffuse, vec3 F0, float k, float RoughnessSqr, vec3 Normal, vec3 GeometryNormal, float NdV)
{
    const uint numLights = GetNumDirectionalLights();

    vec3 light = vec3(0.0);
    for (int i = 0 ; i < numLights ; ++i)
    {
        //const uint RenderMask = LightParameters[i][0];

        vec3 L = LightDirs[i].xyz;

        float NdL = saturate(dot(Normal, L));
        if (NdL > 0.0)
        {
            float NdL_Vertex = saturate(dot(GeometryNormal, L));
            #if 0
            // tan(acos(x)) = sqrt(1-x*x)/x
            float bias = sqrt(1.0 - NdL_Vertex*NdL_Vertex) / max(NdL_Vertex, 0.001);
            #endif
            float bias = (1.0 - NdL_Vertex);
#           ifdef ALLOW_SHADOW_RECEIVE            
            float shadow = SampleLightShadow(i, LightParameters[i][1], LightParameters[i][2], bias);
#           else
            const float shadow = 1.0;
#           endif
            
            if (shadow > 0.0)
            {            
                const float parallaxSelfShadow = GetParallaxSelfShadow(L);        
                light += LightBRDF(Diffuse, F0, RoughnessSqr, Normal, L, NdL, NdV, k) * LightColors[i].xyz * (shadow * NdL * parallaxSelfShadow);
            }
        }
    }
    return light;
}

vec3 CalcOmnidirectionalLightingPBR(vec3 Diffuse, vec3 F0, float k, float RoughnessSqr, vec3 Normal, float NdV, uint FirstIndex, uint NumLights)
{
    #define POINT_LIGHT 0
    #define SPOT_LIGHT  1
    
    // TODO: Get shadow bias and smooth disk radius from light structure
    const float ShadowBias = 0.15;

    vec3 light = vec3(0.0);    

    for (int i = 0 ; i < NumLights ; i++)
    {
        const uint indices = texelFetch(ClusterItemTBO, int(FirstIndex + i)).x;
        const uint lightIndex = indices & 0x3ff;
        const uint lightType = GetLightType(lightIndex);

        const float outerRadius = GetLightRadius(lightIndex);
        const vec3 dirToLight = GetLightPosition(lightIndex) - VS_Position;
        const float distanceToLightSqr = dot(dirToLight, dirToLight);
        
        if (distanceToLightSqr > 0.0 && distanceToLightSqr <= outerRadius*outerRadius)
        {
            const vec3 L = normalize(dirToLight);

            float NdL = saturate(dot(Normal, L));
            if (NdL > 0.0)
            {
                float attenuation = CalcDistanceAttenuationFrostbite(distanceToLightSqr, GetLightInverseSquareRadius(lightIndex));
                //float attenuation = CalcDistanceAttenuation(distanceToLight, outerRadius);
                //float attenuation = CalcDistanceAttenuationUnreal(distanceToLight, outerRadius);
                //float attenuation = CalcDistanceAttenuationSkyforge(distanceToLight, 0.0, outerRadius);
            
                #ifdef ALLOW_SHADOW_RECEIVE
                float shadow;
                if (GetLightShadowmapIndex(lightIndex) != -1)
                    shadow = SampleOmnidirectionalLightShadow(GetLightShadowmapIndex(lightIndex), -dirToLight, ShadowBias);
                else
                    shadow = 1;
                #else
                const float shadow = 1.0;
                #endif
               
                if (shadow > 0.0)
                {
                    float LdotDir = dot(L, GetLightDirection(lightIndex));

                    if (lightType == SPOT_LIGHT)
                        attenuation *= CalcSpotAttenuation(LdotDir, GetLightCosHalfInnerConeAngle(lightIndex), GetLightCosHalfOuterConeAngle(lightIndex), GetLightSpotExponent(lightIndex));

                    #ifdef SUPPORT_PHOTOMETRIC_LIGHT            
                    uint photometricProfile = GetLightPhotometricProfile(lightIndex);            
                    if (photometricProfile < 0xffff)
                        attenuation *= CalcPhotometricAttenuation(LdotDir, photometricProfile);// * GetLightIESScale(lightIndex);
                    #endif

                    attenuation *= NdL * shadow;

                    light += LightBRDF(Diffuse, F0, RoughnessSqr, Normal, L, NdL, NdV, k) * GetLightColor(lightIndex) * attenuation;
                }
            }
        }
    }

    return light;
}

void MaterialPBRShader(vec3 BaseColor,
                       vec3 Normal,
                       vec3 GeometryNormal,
                       float Metallic,
                       float Roughness,
                       vec3 Emissive,
                       float AO,
                       float Opacity)
{
    uint numProbes;
    uint numDecals;
    uint numLights;
    uint firstIndex;

    UnpackCluster(numProbes, numDecals, numLights, firstIndex);

    // Lerp with metallic value to find the good diffuse and specular.
    const vec3 albedo = BaseColor * (1.0 - Metallic);

    // Calc diffuse
    const vec3 diffuse = albedo / PI;

    // 0.03 default specular value for dielectric.
    // Для всех диэлектриков F0 не выше 0.17
    // Для металлов - от 0.5 до 1.0
    const vec3 F0 = mix(vec3(0.03), BaseColor, vec3(Metallic)); // TODO: check 0.04

    // Reflected vector
    const vec3 R = normalize(reflect(-InViewspaceToEyeVec, Normal));

    const float roughnessSqr = Roughness * Roughness;
    const float roughnessSqrClamped = max(0.001, roughnessSqr);
    const float NdV = clamp(dot(Normal, InViewspaceToEyeVec), 0.001, 1.0);

    vec3 light = vec3(0);

#ifndef NO_LIGHTMAP
#   ifdef USE_LIGHTMAP
    // TODO: lightmaps for PBR
    const vec4 lightmap = texture(tslot_lightmap, VS_LightmapTexCoord);
    light += BaseColor * lightmap.rgb;
#   endif

#   ifdef USE_VERTEX_LIGHT
    // Assume vertex has emission light
    light += VS_VertexLight;
#   endif
#endif

    //const float k = roughnessSqr * 0.5f; //  IBL
    const float k = (Roughness + 1) * (Roughness + 1) * 0.125; // Direct light

#if defined WITH_SSAO && defined ALLOW_SSAO
    // Sample ambient occlusion
    vec2 aotc = min(vec2(InScreenUV.x, 1.0 - InScreenUV.y), vec2(1.0) - GetViewportSizeInverted()) * GetDynamicResolutionRatio();
    aotc.y = 1.0 - aotc.y;
    AO *= textureLod(AOLookup, aotc, 0.0).x;
#endif

    // Accumulate directional lights
    light += CalcDirectionalLightingPBR(diffuse, F0, k, roughnessSqrClamped, Normal, GeometryNormal, NdV);

    // Accumulate point and spot lights
    light += CalcOmnidirectionalLightingPBR(diffuse, F0, k, roughnessSqrClamped, Normal, NdV, firstIndex, numLights);

    // Apply ambient
    const vec3 ambient = CalcAmbient(albedo, R, Normal, NdV, F0, Roughness, AO, firstIndex, numProbes);
    light += ambient;

    light *= AO;

    light += Emissive;

    FS_FragColor = vec4(light, Opacity);

#ifdef DEBUG_RENDER_MODE
    uint DebugMode = GetDebugMode();

    switch (DebugMode)
    {
        case DEBUG_FULLBRIGHT:
            FS_FragColor.rgb = BaseColor;
            break;
        case DEBUG_NORMAL:
            FS_FragColor.rgb = Normal * 0.5 + 0.5;
            break;
        case DEBUG_METALLIC:
            FS_FragColor.rgb = vec3(Metallic);
            break;
        case DEBUG_ROUGHNESS:
            FS_FragColor.rgb = vec3(Roughness);
            break;
        case DEBUG_AMBIENT:
#    if defined WITH_SSAO && defined ALLOW_SSAO
            vec2 aotc = min(vec2(InScreenUV.x, 1.0 - InScreenUV.y), vec2(1.0) - GetViewportSizeInverted()) * GetDynamicResolutionRatio();
            aotc.y = 1.0 - aotc.y;

            AO *= textureLod(AOLookup, aotc, 0.0).x;
#    endif
            FS_FragColor.rgb = vec3(AO);
            break;
        case DEBUG_EMISSION:
            FS_FragColor.rgb = Emissive;
            break;
        case DEBUG_LIGHTMAP:
#    ifdef USE_LIGHTMAP
            FS_FragColor.rgb = lightmap.rgb;
#    else
            FS_FragColor.rgb = vec3(0.0);
#    endif
            //FS_FragColor.xyz += MaterialAmbientLight;
            break;
        case DEBUG_VERTEX_LIGHT:
#    ifdef USE_VERTEX_LIGHT
            FS_FragColor.rgb = VS_VertexLight;
#    else
            FS_FragColor.rgb = vec3(0.0);
#    endif
            break;
        case DEBUG_DIRLIGHT:
            FS_FragColor.rgb = CalcDirectionalLightingPBR(vec3(0.5 / PI), F0, k, roughnessSqrClamped, Normal, GeometryNormal, NdV);
            break;
        case DEBUG_POINTLIGHT:
            FS_FragColor.rgb = CalcOmnidirectionalLightingPBR(vec3(0.5 / PI), F0, k, roughnessSqrClamped, Normal, NdV, firstIndex, numLights) * AO;
            break;
            //case DEBUG_TEXCOORDS:
            //    FS_FragColor.rgb = vec3( nsv_VS0_TexCoord.xy, 0.0);
            //    break;
        case DEBUG_TEXNORMAL:
            //        FS_FragColor.rgb = N*0.5+0.5;
            break;
        case DEBUG_TBN_NORMAL:
            FS_FragColor.rgb = GeometryNormal * 0.5 + 0.5;
            break;
            //case DEBUG_TBN_TANGENT:
            //    FS_FragColor.rgb = VS_T*0.5+0.5;
            //    break;
            //case DEBUG_TBN_BINORMAL:
            //    FS_FragColor.rgb = VS_B*0.5+0.5;
            //    break;
        case DEBUG_SPECULAR:
            FS_FragColor.rgb = F0;
            break;
        case DEBUG_AMBIENT_LIGHT:
            FS_FragColor.rgb = ambient;
            break;
        case DEBUG_LIGHT_CASCADES:
            FS_FragColor.rgb = FS_FragColor.rgb * 0.3 + DebugDirectionalLightCascades();
            break;
        case DEBUG_VT_BORDERS:
#    ifdef USE_VIRTUAL_TEXTURE
            const float PageResolutionB = 128;
            vec2 v = mod(InPhysicalUV * VTPageCacheCapacity.xy * PageResolutionB, PageResolutionB) / PageResolutionB;

            float x = 0;
            vec2 s = abs(pow(smoothstep(vec2(x), vec2(1.0 - x), fract(v + 0.5)), vec2(1.0)) - 0.5);
            s = 1.0 - min(vec2(1.0), s * 10.0);
            float t = max(s.x, s.y);

            FS_FragColor.rgb = mix(FS_FragColor.rgb, vec3(1, 0.8, 0), t);
#    endif
            break;
        case DEBUG_VELOCITY:
            break;
    }
#endif // DEBUG_RENDER_MODE
}
