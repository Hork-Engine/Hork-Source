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

#include "base/common.frag"

#if 0

#define BLOCKER_SEARCH_NUM_SAMPLES 16
#define PCF_NUM_SAMPLES 16
//#define NEAR_PLANE 0.04//9.5
//#define LIGHT_WORLD_SIZE 0.1//.5
//#define LIGHT_FRUSTUM_WIDTH 2.0//3.75
// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
//#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)

#define LIGHT_SIZE_UV 0.02

const vec2 poissonDisk[16] = vec2[]
(
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

float PCSS_PenumbraSize(float zReceiver, float zBlocker) //Parallel plane estimation
{
    return (zReceiver - zBlocker) / zBlocker;
} 

const float PCSS_bias = 0;//0.0001;// 0.1;

void PCSS_FindBlocker(sampler2DArray ShadowMap, float CascadeIndex, vec2 uv, float zReceiver, out float AvgBlockerDepth, out float NumBlockers)
{
    //This uses similar triangles to compute what
    //area of the shadow map we should search
    const float searchWidth = LIGHT_SIZE_UV; // * ( zReceiver - NEAR_PLANE ) / zReceiver;
    float blockerSum = 0;

    NumBlockers = 0;

    float biasedZ = zReceiver + PCSS_bias;

    for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i)
    {
        float shadowMapDepth = texture(ShadowMap /*PointSampler*/, vec3(uv + poissonDisk[i] * searchWidth, CascadeIndex)).x;
        if (shadowMapDepth < biasedZ)
        {
            blockerSum += shadowMapDepth;
            NumBlockers++;
        }
    }
    AvgBlockerDepth = blockerSum / NumBlockers;
}

float PCF_Filter(sampler2DArrayShadow ShadowMapShadow, vec4 TexCoord, float _FilterRadiusUV)
{
    float sum = 0.0f;
    vec4 sampleCoord = TexCoord;
    for (int i = 0; i < PCF_NUM_SAMPLES; ++i)
    {
        sampleCoord.xy = TexCoord.xy + poissonDisk[i] * _FilterRadiusUV;

        //bool SamplingOutside = any(bvec2(any(lessThan(sampleCoord.xy, vec2(0.0))), any(greaterThan(sampleCoord.xy, vec2(1.0)))));
        //if (SamplingOutside) {
        //    sum += 0;
        //} else {
        sum += texture(ShadowMapShadow, sampleCoord);
        //}
    }
    return sum / PCF_NUM_SAMPLES;
}

float PCSS_Shadow(sampler2DArray ShadowMap, sampler2DArrayShadow ShadowMapShadow, vec4 TexCoord)
{
    float zReceiver = TexCoord.w;

    float AvgBlockerDepth = 0;
    float NumBlockers = 0;
    PCSS_FindBlocker(ShadowMap, TexCoord.z, TexCoord.xy, zReceiver, AvgBlockerDepth, NumBlockers);
    if (NumBlockers < 1)
    {
        //There are no occluders so early out (this saves filtering)
        //DebugEarlyCutoff = true;
        return 1.0f;
    }

    float PenumbraRatio = PCSS_PenumbraSize(zReceiver, AvgBlockerDepth);
    float FilterRadiusUV = PenumbraRatio * LIGHT_SIZE_UV / zReceiver / exp(TexCoord.z); // * NEAR_PLANE / zReceiver;

    return PCF_Filter(ShadowMapShadow, TexCoord, FilterRadiusUV);
}
#endif

float PCF_3x3(sampler2DArrayShadow ShadowMap, vec4 TexCoord)
{
//#ifdef ATI
    const float invSize = 1.0 / textureSize(ShadowMap,0).x; // NOTE: shadow maps has equal width and height

    return ( texture(ShadowMap, TexCoord + vec4(-invSize,-invSize, 0, 0))
           + texture(ShadowMap, TexCoord + vec4( 0,      -invSize, 0, 0))
           + texture(ShadowMap, TexCoord + vec4( invSize,-invSize, 0, 0))
           + texture(ShadowMap, TexCoord + vec4(-invSize,       0, 0, 0))
           + texture(ShadowMap, TexCoord + vec4( 0,             0, 0, 0))
           + texture(ShadowMap, TexCoord + vec4( invSize,       0, 0, 0))
           + texture(ShadowMap, TexCoord + vec4(-invSize, invSize, 0, 0))
           + texture(ShadowMap, TexCoord + vec4( 0,       invSize, 0, 0))
           + texture(ShadowMap, TexCoord + vec4( invSize, invSize, 0, 0))) / 9.0;
//#else
//    return ( textureOffset(ShadowMap, TexCoord, ivec2(-1,-1))
//           + textureOffset(ShadowMap, TexCoord, ivec2( 0,-1))
//           + textureOffset(ShadowMap, TexCoord, ivec2( 1,-1))
//           + textureOffset(ShadowMap, TexCoord, ivec2(-1, 0))
//           + textureOffset(ShadowMap, TexCoord, ivec2( 0, 0))
//           + textureOffset(ShadowMap, TexCoord, ivec2( 1, 0))
//           + textureOffset(ShadowMap, TexCoord, ivec2(-1, 1))
//           + textureOffset(ShadowMap, TexCoord, ivec2( 0, 1))
//           + textureOffset(ShadowMap, TexCoord, ivec2( 1, 1))) / 9.0;
//#endif
}

float PCF_5x5(sampler2DArrayShadow ShadowMap, vec4 TexCoord)
{
#if 0
    const float invSize = 1.0 / textureSize(ShadowMap,0).x; // NOTE: shadow maps has equal width and height

    float shadow = 0;
    for (float i = -2; i <= 2; i++)
        for (float j = -2; j <= 2; j++)
            shadow += texture(ShadowMap, TexCoord + vec4(i, j, 0, 0) * invSize);
#else
    float shadow = 0;
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-2,-2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-2,-1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-2, 0));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-2, 1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-2, 2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-1,-2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-1,-1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-1, 0));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-1, 1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2(-1, 2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 0,-2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 0,-1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 0, 0));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 0, 1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 0, 2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 1,-2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 1,-1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 1, 0));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 1, 1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 1, 2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 2,-2));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 2,-1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 2, 0));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 2, 1));
    shadow += textureOffset(ShadowMap, TexCoord, ivec2( 2, 2));
#endif
    return shadow * (1.0f/25.0f);
}

float SampleLightShadow(uint ShadowmapIndex, uint FirstCascade, uint NumCascades, float Bias)
{
    for (uint i = 0; i < NumCascades; i++)
    {
        uint cascadeIndex = FirstCascade + i;
        vec4 SMTexCoord = ShadowMapMatrices[cascadeIndex] * InClipspacePosition;
        vec3 shadowCoord = SMTexCoord.xyz / SMTexCoord.w;

        // TODO: Move this values to uniforms
        shadowCoord.z -= clamp(Bias * (1 << i) * 1.5, 0.45, 5.0) * 0.0001;

#ifdef SHADOWMAP_PCF
        if (!any(bvec2(any(lessThan(shadowCoord, vec3(0.0))), any(greaterThan(shadowCoord, vec3(1.0))))))
            return PCF_5x5(ShadowMap[ShadowmapIndex], vec4(shadowCoord.xy, float(i), shadowCoord.z));
#endif

#ifdef SHADOWMAP_PCSS
        if (!any(bvec2(any(lessThan(shadowCoord, vec3(0.05))), any(greaterThan(shadowCoord, vec3(1.0 - 0.05))))))
            return PCSS_Shadow(ShadowMap[ShadowmapIndex], vec4(shadowCoord.xy, float(i), shadowCoord.z));
#endif

#ifdef SHADOWMAP_VSM
        if (!any(bvec2(any(lessThan(shadowCoord, vec3(0.01))), any(greaterThan(shadowCoord, vec3(1.0 - 0.01))))))
            return VSM_Shadow(ShadowMap[ShadowmapIndex], vec4(shadowCoord.xy, float(i), shadowCoord.z));
            //Shadow = VSM_Shadow_PCF_3x3( ShadowMap[ShadowmapIndex], SMTexCoord );
#endif

#ifdef SHADOWMAP_EVSM
        if (!any(bvec2(any(lessThan(shadowCoord, vec3(0.01))), any(greaterThan(shadowCoord, vec3(1.0 - 0.01))))))
            return EVSM_Shadow(ShadowMap[ShadowmapIndex], vec4(shadowCoord.xy, float(i), shadowCoord.z));
#endif
    }
    return 1.0;
}

vec3 DebugShadowCascades(uint FirstCascade, uint NumCascades)
{
    const vec3 CascadeColor[MAX_SHADOW_CASCADES] = vec3[](
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(1, 1, 0));

    for (uint i = 0; i < NumCascades; i++)
    {
        const uint cascadeIndex = FirstCascade + i;
        const vec4 SMTexCoord = ShadowMapMatrices[cascadeIndex] * InClipspacePosition;

        vec3 shadowCoord = SMTexCoord.xyz / SMTexCoord.w;

        if (!any(bvec2(any(lessThan(shadowCoord, vec3(0.0))), any(greaterThan(shadowCoord, vec3(1.0))))))
            return CascadeColor[i];
    }

    return vec3(0.0);
}

vec3 DebugDirectionalLightCascades()
{
    const uint numLights = GetNumDirectionalLights();
    vec3 result = vec3(0.0);
    
    for ( int i = 0 ; i < numLights ; ++i ) {
        float x = 1.0 / float(numLights);
        
        if ( i * x <= InNormalizedScreenCoord.x && ( i + 1 ) * x > InNormalizedScreenCoord.x ) {
            result += DebugShadowCascades( LightParameters[ i ][ 1 ], LightParameters[ i ][ 2 ] );
        }
    }
    return result;
}

float ChebyshevUpperBound(vec2 moments, float t)
{
    const float MinVariance = 0;

    // One-tailed inequality valid if t > moments.x
    float p = float(t <= moments.x);
  
    // Compute variance
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, MinVariance);
  
    // Compute probabilistic upper bound.
    float d = t - moments.x;
    float p_max = variance / (variance + d * d);
  
    return max(p, p_max);
}

float linstep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0, 1);
}

float ReduceLightBleeding(float p_max, float amount)
{
    // Remove the [0, Amount] tail and linearly rescale (Amount, 1].
    return linstep(amount, 1, p_max);
}

float SampleOmnidirectionalLightShadow_VSM(uint shadowmapIndex, vec3 lightDir)
{
    const float ShadowZNear = 0.1; // NOTE: Should match shadowmap projection matrix
    const float ShadowZFar = 1000;

    // Transform vector from view space to world space
    const mat3 VectorTransformWS = mat3( vec3(WorldNormalToViewSpace0),
                                         vec3(WorldNormalToViewSpace1),
                                         vec3(WorldNormalToViewSpace2)
                                       ); // TODO: Optimize this!

    lightDir = VectorTransformWS * lightDir;

    float distanceToLight = length(lightDir);

    vec2 Moments = texture(OmnidirectionalShadowMapArray, vec4(lightDir, shadowmapIndex)).rg;

    float cheb = ChebyshevUpperBound(Moments, distanceToLight);

    cheb = ReduceLightBleeding(cheb, 0.5);

    return cheb;
}

float SampleOmnidirectionalLightShadow(uint shadowmapIndex, vec3 lightDir, float shadowBias)
{
    const float ShadowZNear = 0.1; // NOTE: Should match shadowmap projection matrix
    const float ShadowZFar = 1000;

    // Transform vector from view space to world space
    const mat3 VectorTransformWS = mat3( vec3(WorldNormalToViewSpace0),
                                         vec3(WorldNormalToViewSpace1),
                                         vec3(WorldNormalToViewSpace2)
                                       ); // TODO: Optimize this!

    lightDir = VectorTransformWS * lightDir;

    float fragmentDist = length(lightDir);
    float biasedDist = fragmentDist - shadowBias;

    const int NUM_SAMPLES = 20;

    const vec3 directions[NUM_SAMPLES] =
    {
        vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
        vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
        vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
        vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
        vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
    };

    float viewDistance = length(VS_Position);
    float diskRadius = (0.01 + (fragmentDist / 25 + viewDistance / 25)) / 25.0;  // TODO: move coefficients to light parameters

    float shadow = 0.0;
    for(int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec3 dir = lightDir + directions[i] * diskRadius;
        float occluderDist = texture(OmnidirectionalShadowMapArray, vec4(dir, shadowmapIndex)).r * ShadowZFar;
        shadow += float(occluderDist < biasedDist);
    }
    shadow /= float(NUM_SAMPLES); 

    return 1.0 - shadow;
}
