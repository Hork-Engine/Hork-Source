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

vec2 control_float2Offsets[16] = vec2[](
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

vec4 control_jitters[16] = vec4[](
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

vec2 g_Float2Offset = control_float2Offsets[AO_LAYER].xy;
vec4 g_Jitter       = control_jitters[AO_LAYER];

#else
layout( binding = 0 ) uniform sampler2D Smp_LinearDepth;
layout( binding = 1 ) uniform sampler2D Smp_Normals;
layout( binding = 2 ) uniform sampler2D Smp_RandomMap;
#endif


#define SAMPLES_COUNT 16
#define RANDOM_MAP_SIZE 4

const float NUM_STEPS = 4;
const float NUM_DIRECTIONS = 8; // texRandom/g_Jitter initialization depends on this

layout( binding = 1, std140 ) uniform DrawCall
{
    float AOBias;
    float AOFallofFactor;
    float AORadiusToScreen;
    float AOPowExponent;
    float AOMultiplier;
    vec2 AOInvQuarterResolution;
    vec2 AOInvFullResolution;
};

vec3 UVToView( vec2 uv, float eye_z )
{
#ifdef AO_ORTHO_PROJECTION
    return vec3 ( ( uv * ProjectionInfo.xy + ProjectionInfo.zw ), eye_z );
#else
    return vec3 ( ( uv * ProjectionInfo.xy + ProjectionInfo.zw ) * eye_z, eye_z );
#endif
}

#ifdef AO_DEINTERLEAVED

vec3 getQuarterCoord(vec2 UV){
    return vec3(UV,float(AO_LAYER));
}
  
vec3 FetchQuarterResViewPos(vec2 UV)
{
    float ViewDepth = textureLod( Smp_LinearDepth, getQuarterCoord(UV), 0 ).x;
    //float ViewDepth = texelFetch( Smp_LinearDepth, ivec3(getQuarterCoord(UV/GetAOQuarterSizeInverted())), 0 ).x;
    return UVToView( UV, ViewDepth );
}

#else

vec3 FetchPos( vec2 UV )
{
    float ViewDepth = textureLod(Smp_LinearDepth,UV,0).x;
    return UVToView(UV, ViewDepth);
}

vec3 MinDiff( vec3 P, vec3 Pr, vec3 Pl )
{
    vec3 V1 = Pr - P;
    vec3 V2 = P - Pl;
    return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 ReconstructNormal( vec2 UV, vec3 P )
{
    vec3 Pr = FetchPos(UV + vec2(AOInvFullResolution.x, 0));
    vec3 Pl = FetchPos(UV + vec2(-AOInvFullResolution.x, 0));
    vec3 Pt = FetchPos(UV + vec2(0, AOInvFullResolution.y));
    vec3 Pb = FetchPos(UV + vec2(0, -AOInvFullResolution.y));
    return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

vec3 FetchNormal( vec2 tc ) {
#if 1
    return -ReconstructNormal(tc, FetchPos(tc));
#else
    vec3 n = normalize( texture( Smp_Normals, tc ).xyz * 2.0 - 1.0 );
    n.x=-n.x; // FIX THIS!
    return n;
#endif
}

#endif
    
vec4 GetJitter()
{
#ifdef AO_DEINTERLEAVED
    // Get the current jitter vector from the per-pass constant buffer
    return g_Jitter;
#else
    // (cos(Alpha),sin(Alpha),rand1,rand2)
    // Divide gl_FragCoord by four (random map dimension)
    return textureLod( Smp_RandomMap, gl_FragCoord.xy * 0.25, 0);
#endif
}

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
    vec3 V = S - P;

    float DistanceSquare = dot( V, V );
    float NdotV = dot( N, V ) * inversesqrt( DistanceSquare );
  
    // 1 scalar mad instruction + saturate
    float Falloff = clamp( DistanceSquare * AOFallofFactor + 1.0, 0.0, 1.0 );

    // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
    return clamp( NdotV - AOBias, 0, 1 ) * Falloff;
}

float ComputeCoarseAO(vec2 FullResUV, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal)
{
#ifdef AO_DEINTERLEAVED
    RadiusPixels /= 4.0;
#endif

    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

    const float Alpha = 2.0 * PI / NUM_DIRECTIONS;
    float AO = 0;

    for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
    {
        float Angle = Alpha * DirectionIndex;

        // Compute normalized 2D direction
        vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);
        
        // Jitter starting sample within the first step
        float RayPixels = (Rand.z * StepSizePixels + 1.0);

        for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
        {
#ifdef AO_DEINTERLEAVED
            vec2 SnappedUV = round(RayPixels * Direction) * AOInvQuarterResolution + FullResUV;
            vec3 S = FetchQuarterResViewPos(SnappedUV);
#else
            vec2 SnappedUV = round(RayPixels * Direction) * AOInvFullResolution + FullResUV;
            vec3 S = FetchPos( SnappedUV );
#endif
            RayPixels += StepSizePixels;

            AO += ComputeAO( ViewPosition, ViewNormal, S );
        }
    }

    AO *= AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
    
    return clamp( 1.0 - AO * 2.0, 0, 1 );
}


/*


HBAO Shader


*/

void HBAOShader() {
#ifdef AO_DEINTERLEAVED
    vec2 base = floor(gl_FragCoord.xy) * 4.0 + g_Float2Offset;
    vec2 uv = base * (AOInvQuarterResolution / 4.0);

    vec3 ViewPosition = FetchQuarterResViewPos(uv);
    vec3 ViewNormal = texelFetch( Smp_Normals, ivec2(base), 0 ).xyz;
    
    ViewNormal = normalize( ViewNormal * 2.0 - 1.0 );
#else
    vec2 uv = AdjustTexCoord( VS_TexCoord );
  
    vec3 ViewPosition = FetchPos( uv );
    
    vec3 ViewNormal = texelFetch( Smp_Normals, ivec2(gl_FragCoord.xy), 0 ).xyz;
    
    ViewNormal =  normalize( ViewNormal * 2.0 - 1.0 );
#endif

    // Compute projection of disk of radius control_R into screen space
    #ifdef AO_ORTHO_PROJECTION
    float RadiusPixels = AORadiusToScreen;
    #else
    float RadiusPixels = AORadiusToScreen / ViewPosition.z;
    #endif

    // Get jitter vector for the current full-res pixel
    vec4 Rand = GetJitter();

    float AO = ComputeCoarseAO(uv, RadiusPixels, Rand, ViewPosition, ViewNormal);
  
    FS_FragColor = pow( AO, AOPowExponent );
    
    //FS_FragColor = builtin_luminance(vec3(Rand)*0.5+0.5);
    //FS_FragColor = float(gl_Layer) / 15;
    //FS_FragColor = builtin_luminance( ViewNormal*0.5+0.5 );
    //FS_FragColor = (-textureLod( Smp_LinearDepth, getQuarterCoord(uv), 0 ).x*0.1);
    //FS_FragColor = builtin_luminance( vec3(getQuarterCoord(uv)) );
}

#if 1



const vec2 RotationMap[16] = vec2[](
    normalize( vec2( -0.578837, -0.815443 ) ),
    normalize( vec2( 0.372530, -0.928020 ) ),
    normalize( vec2( -0.999478, 0.032305 ) ),
    normalize( vec2( -0.013471, -0.999909 ) ),
    normalize( vec2( -0.998865, -0.047632 ) ),
    normalize( vec2( -0.487773, -0.872970 ) ),
    normalize( vec2( -0.459573, -0.888140 ) ),
    normalize( vec2( 0.930130, 0.367229 ) ),
    normalize( vec2( 0.782280, -0.622927 ) ),
    normalize( vec2( 0.643223, 0.765679 ) ),
    normalize( vec2( 0.392021, -0.919956 ) ),
    normalize( vec2( 0.826203, -0.563372 ) ),
    normalize( vec2( 0.993278, 0.115752 ) ),
    normalize( vec2( -0.752351, -0.658762 ) ),
    normalize( vec2( -0.979239, -0.202707 ) ),
    normalize( vec2( -0.837535, 0.546384 ) )
);

vec2 RandomVec() {
    int x = int( gl_FragCoord.x ) & 3;
    int y = int( gl_FragCoord.y ) & 3;
    int index = y * 4 + x;
    return RotationMap[ index ];
}

/*


SSAO Shader


*/


float rnd( float x ) {
    return rand( vec2(453,342) * x/* * GameplayTime()*/ );
}

// Calc pseudo-random vector. Can be precomputed.
vec3 RandomSample( const int i ) {
    float n = float(i)+1;
    vec3 v = vec3(rnd(n+0.001) * 2.0f - 1.0f, rnd(n+0.002) * 2.0f - 1.0f, rnd(n+0.003));
    
    v = normalize( v );
    v = v * rnd(n+0.004);
    float scale = float(i) / SAMPLES_COUNT;

    // scale samples s.t. they're more aligned to center of kernel
    scale = mix(0.1f, 1.0f, scale * scale);
    v = v * scale;
    return v;
}



#ifndef AO_DEINTERLEAVED

void SSAOShader() {
    const float radius = 0.9;
    const float bias = 0.025;

    vec2 uv = AdjustTexCoord( VS_TexCoord );
  
    vec3 ViewPosition = FetchPos( uv );
ViewPosition.z=-ViewPosition.z;
    //vec3 ViewNormal = FetchNormal( uv );
    vec3 ViewNormal =  texelFetch( Smp_Normals, ivec2(gl_FragCoord.xy), 0 ).xyz;
    ViewNormal =  normalize( ViewNormal * 2.0 - 1.0 );
ViewNormal.z=-ViewNormal.z;
    // Gram–Schmidt process to construct an orthogonal basis
    vec3 randomVec = vec3( RandomVec(), 0.0 );
    vec3 tangent = normalize(randomVec - ViewNormal * dot(randomVec, ViewNormal));
    vec3 bitangent = cross(ViewNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, ViewNormal);
    
    float occlusion = 0.0;
    
    mat4 p = inverse(InverseProjectionMatrix);//Perspective( radians(100.0), 1.0/GetAOSizeInverted().x, 1.0/GetAOSizeInverted().y, GetViewportZNear(), GetViewportZFar() );

    for ( int i = 0; i < SAMPLES_COUNT; ++i )
    {
        vec3 v = TBN * RandomSample(i);
        v = ViewPosition + v * radius;
        
        vec4 offset = vec4(v, 1.0);
        offset = p * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = -texture( Smp_LinearDepth, /*AdjustTexCoord*/(offset.xy) ).x;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(ViewPosition.z - sampleDepth));
        occlusion += (sampleDepth >= v.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    
    occlusion = clamp( 1.0 - occlusion / SAMPLES_COUNT, 0, 1 );
    
    occlusion = pow( occlusion, AOPowExponent );
    
    FS_FragColor = occlusion;
    
    //FS_FragColor = ViewPosition.z*0.1;
}


/*


HBAO Old Shader


*/

const vec2 SampleMap[SAMPLES_COUNT] = vec2[](
    vec2( 0.018341, 0.018341 ),
    vec2( -0.036681, 0.036681 ),
    vec2( -0.055022, -0.055022 ),
    vec2( 0.073362, -0.073362 ),
    vec2( -0.000000, 0.129688 ),
    vec2( -0.155625, -0.000000 ),
    vec2( -0.000000, -0.181562 ),
    vec2( 0.207500, -0.000000 ),
    vec2( -0.165065, 0.165065 ),
    vec2( -0.183406, -0.183406 ),
    vec2( 0.201746, -0.201746 ),
    vec2( 0.220087, 0.220087 ),
    vec2( -0.337187, -0.000000 ),
    vec2( 0.000001, -0.363125 ),
    vec2( 0.389062, 0.000001 ),
    vec2( -0.000001, 0.415000 )
);

void HBAOShaderOld() {
    vec2 tc = AdjustTexCoord( VS_TexCoord );
    
    vec3 P = FetchPos( tc );
    vec3 N = FetchNormal( tc );
    float dist = length( P );
    vec2 sr = RandomVec() / dist;
    mat2x2 M = mat2x2( sr.x, sr.y, -sr.y, sr.x );
    
    const float radius = 0.5;//0.1

    float occlusion = 0.0;

    for ( int i = 0 ; i < SAMPLES_COUNT ; i++ )
    {
        vec2 sampletc = M * SampleMap[ i ] * radius + tc;
        
        //if ( S.x > 0.0 && S.y > 0.0 && S.x < 1.0 && S.y < 1.0 )
        {
            vec3 S = FetchPos( sampletc );
            vec3 V = S - P;
            float VdotV = dot( V, V );
#if 0           
            occlusion += max( dot( N, V ), 0.0 ) / ( sqrt( VdotV ) * ( 1.0 + VdotV ) );
#else
            float NdotV = dot(N, V) * 1.0/sqrt(VdotV);
            occlusion += clamp(NdotV - AOBias,0,1) * clamp(VdotV * AOFallofFactor + 1.0,0,1);
#endif
        }
    
    }
    occlusion *= AOMultiplier / SAMPLES_COUNT;
    occlusion = clamp( 1.0 - occlusion * 2.0, 0.0, 1.0 );
    
    occlusion = pow( occlusion, AOPowExponent );
    
    FS_FragColor = occlusion;
}

#endif

#endif



void main() {
#ifdef AO_DEINTERLEAVED
    HBAOShader();
#else
    if ( VS_TexCoord.x > 0.5 ) {
       SSAOShader();
    } else {
       HBAOShader();
    }
#endif
    
    //} else {
    //   HBAOShaderOld();
    //}    
}
