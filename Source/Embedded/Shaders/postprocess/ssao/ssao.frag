/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

// Based on DeinterleavedTexturing sample by Louis Bavoil
// https://github.com/NVIDIAGameWorks/D3DSamples/tree/master/samples/DeinterleavedTexturing

#include "base/viewuniforms.glsl"
#include "base/core.glsl"

#pragma optionNV(unroll all)

layout( location = 0, index = 0 ) out float FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

#ifdef AO_DEINTERLEAVED
layout( binding = 0 ) uniform sampler2DArray Smp_LinearDepth;
layout( binding = 1 ) uniform sampler2D Smp_Normals;
#else
layout( binding = 0 ) uniform sampler2D Smp_LinearDepth;
layout( binding = 1 ) uniform sampler2D Smp_Normals;
layout( binding = 2 ) uniform sampler2D Smp_RandomMap;
#endif

layout( binding = 1, std140 ) uniform DrawCall
{
    float AOBias;
    float AOFallofFactor;
    float AORadiusToScreen;
    float AOPowExponent;
    float AOMultiplier;
    vec2 AOInvFullResolution;
    vec2 AOInvQuarterResolution;
};

const vec2 Float2Offsets[16] = vec2[](
    vec2( 0.500000, 0.500000 ),
    vec2( 1.500000, 0.500000 ),
    vec2( 2.500000, 0.500000 ),
    vec2( 3.500000, 0.500000 ),
    vec2( 0.500000, 1.500000 ),
    vec2( 1.500000, 1.500000 ),
    vec2( 2.500000, 1.500000 ),
    vec2( 3.500000, 1.500000 ),
    vec2( 0.500000, 2.500000 ),
    vec2( 1.500000, 2.500000 ),
    vec2( 2.500000, 2.500000 ),
    vec2( 3.500000, 2.500000 ),
    vec2( 0.500000, 3.500000 ),
    vec2( 1.500000, 3.500000 ),
    vec2( 2.500000, 3.500000 ),
    vec2( 3.500000, 3.500000 )
);

const vec4 Jitters[16] = vec4[](
    vec4( 0.711252, 0.702937, 0.536444, 0.0 ),
    vec4( 0.990358, 0.138533, 0.264061, 0.0 ),
    vec4( 0.885653, 0.464348, 0.268157, 0.0 ),
    vec4( 0.718024, 0.696018, 0.792889, 0.0 ),
    vec4( 0.754635, 0.656145, 0.264613, 0.0 ),
    vec4( 0.853547, 0.521016, 0.803435, 0.0 ),
    vec4( 0.999385, 0.035062, 0.337718, 0.0 ),
    vec4( 0.895324, 0.445416, 0.037044, 0.0 ),
    vec4( 0.952182, 0.305531, 0.600023, 0.0 ),
    vec4( 0.968180, 0.250256, 0.564537, 0.0 ),
    vec4( 0.959586, 0.281417, 0.830388, 0.0 ),
    vec4( 0.811686, 0.584094, 0.748312, 0.0 ),
    vec4( 0.790724, 0.612172, 0.105814, 0.0 ),
    vec4( 0.757606, 0.652713, 0.072330, 0.0 ),
    vec4( 0.957969, 0.286872, 0.746835, 0.0 ),
    vec4( 0.887341, 0.461114, 0.864001, 0.0 )
);

#define AO_LAYER gl_Layer//gl_PrimitiveID

const vec2 g_Float2Offset = Float2Offsets[AO_LAYER].xy;
const vec4 g_Jitter = Jitters[AO_LAYER];

const float NUM_STEPS = 4;
const float NUM_DIRECTIONS = 8;

// (cos(Alpha),sin(Alpha),rand1,rand2)
vec4 GetJitter()
{
#ifdef AO_DEINTERLEAVED
    // Get the current jitter vector from the per-pass constant buffer
    return g_Jitter;
#else
    #if 1
        // Divide gl_FragCoord by four (random map dimension)
        return textureLod( Smp_RandomMap, gl_FragCoord.xy * 0.25, 0);
    #else
        int x = int( gl_FragCoord.x ) & 3;
        int y = int( gl_FragCoord.y ) & 3;
        int index = y * 4 + x;
        return Jitters[index];
    #endif    
#endif
}

#ifdef AO_ORTHO_PROJECTION
#define SSAO_UVToView UVToView_ORTHO
#else
#define SSAO_UVToView UVToView_PERSPECTIVE
#endif

#ifdef AO_DEINTERLEAVED

vec3 GetQuarterCoord( vec2 UV ) {
    return vec3( UV, float( AO_LAYER ) );
}
  
vec3 FetchQuarterResViewPos(vec2 UV)
{
    float ViewDepth = textureLod( Smp_LinearDepth, GetQuarterCoord(UV), 0 ).x;
    //float ViewDepth = texelFetch( Smp_LinearDepth, ivec3(GetQuarterCoord(UV/GetAOQuarterSizeInverted())), 0 ).x;
    return SSAO_UVToView( UV, -ViewDepth );
}

#else

vec3 FetchPos( vec2 UV )
{
    //vec2 tc = min( vec2(UV.x,1.0-UV.y), vec2(1.0) - GetViewportSizeInverted() ) * GetDynamicResolutionRatio();    
    //tc.y = 1.0 - tc.y;
    
    vec2 tc = AdjustTexCoord(vec2(UV.x,1.0-UV.y));
    float ViewDepth = textureLod( Smp_LinearDepth, tc, 0 ).x;
    //ivec2 tc = ivec2( vec2(UV.x,UV.y) * (1.0/GetViewportSizeInverted()) );
    //float ViewDepth = texelFetch( Smp_LinearDepth, tc, 0 ).x;
    
    //UV = UV / GetDynamicResolutionRatio();
    
    return SSAO_UVToView( UV, -ViewDepth );
}

#endif

vec2 RotateDirection( vec2 Dir, vec2 CosSin )
{
    return vec2( Dir.x*CosSin.x - Dir.y*CosSin.y,
                  Dir.x*CosSin.y + Dir.y*CosSin.x );
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO( vec3 P, vec3 N, vec3 S )
{
    const vec3 V = S - P;

    const float DistanceSquare = dot( V, V );
    const float NdotV = dot( N, V ) * inversesqrt( DistanceSquare );
  
    // 1 scalar mad instruction + saturate
    const float Falloff = clamp( DistanceSquare * AOFallofFactor + 1.0, 0.0, 1.0 );

    // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
    return clamp( NdotV - AOBias, 0, 1 ) * Falloff;
}

float ComputeCoarseAO( vec2 FullResUV, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal )
{
#ifdef AO_DEINTERLEAVED
    // Divide for deinterleaved AO
    RadiusPixels *= 0.25;
#endif

    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    const float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

    const float Alpha = 2.0 * PI / NUM_DIRECTIONS;
    float AO = 0;

    for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
    {
        const float Angle = Alpha * DirectionIndex;

        // Compute normalized 2D direction
        const vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);
        
        // Jitter starting sample within the first step
        float RayPixels = (Rand.z * StepSizePixels + 1.0);

        for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
        {
#ifdef AO_DEINTERLEAVED
            const vec2 SnappedUV = round(RayPixels * Direction) * AOInvQuarterResolution + FullResUV;
            const vec3 S = FetchQuarterResViewPos(SnappedUV);
#else
            const vec2 SnappedUV = round(RayPixels * Direction) * AOInvFullResolution + FullResUV;
            const vec3 S = FetchPos( SnappedUV );
#endif

            RayPixels += StepSizePixels;

            AO += ComputeAO( ViewPosition, ViewNormal, S );
        }
    }

    AO *= AOMultiplier * ( 1.0 / (NUM_DIRECTIONS * NUM_STEPS) );
    
    return clamp( 1.0 - AO * 2.0, 0, 1 );
}

void main() {
#ifdef AO_DEINTERLEAVED
    const vec2 base = floor( gl_FragCoord.xy ) * 4.0 + g_Float2Offset;
    const vec2 uv = base * ( AOInvQuarterResolution * 0.25 );

    vec3 ViewPosition = FetchQuarterResViewPos( uv );
    vec3 ViewNormal = texelFetch( Smp_Normals, ivec2( base ), 0 ).xyz;
#else
    vec2 uv = VS_TexCoord;//AdjustTexCoord( VS_TexCoord );
    uv.y = 1.0-uv.y;
  
    vec3 ViewPosition = FetchPos( uv );
    vec3 ViewNormal = texelFetch( Smp_Normals, ivec2( gl_FragCoord.xy ), 0 ).xyz;
#endif

    ViewNormal = normalize( ViewNormal * 2.0 - 1.0 );

    // Compute projection of disk of Radius into screen space
    #ifdef AO_ORTHO_PROJECTION
    const float RadiusPixels = AORadiusToScreen;
    #else
    const float LinearDepth = -ViewPosition.z;
    const float RadiusPixels = AORadiusToScreen / LinearDepth;
    #endif

    // Get jitter vector for the current full-res pixel
    const vec4 Rand = GetJitter();

    const float AO = ComputeCoarseAO( uv, RadiusPixels, Rand, ViewPosition, ViewNormal );
  
    FS_FragColor = pow( AO, AOPowExponent );
    
    //FS_FragColor = builtin_luminance(vec3(Rand)*0.5+0.5);
    //FS_FragColor = float(gl_Layer) / 15;
    //FS_FragColor = builtin_luminance( ViewNormal*0.5+0.5 );
    //FS_FragColor = (-textureLod( Smp_LinearDepth, GetQuarterCoord(uv), 0 ).x*0.1);
    //FS_FragColor = builtin_luminance( vec3(GetQuarterCoord(uv)) );
}
