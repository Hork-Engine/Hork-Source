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

#include "base/viewuniforms.glsl"
#include "base/common.frag"
#include "base/debug.glsl"
#include "terrain_common.glsl"

#define ALLOW_SHADOW_RECEIVE
#define ALLOW_SSAO

layout( location = 0 ) out vec4 FS_FragColor;

layout( origin_upper_left ) in vec4 gl_FragCoord;

layout( location = 0 ) in vec3 VS_N;
layout( location = 1 ) in vec3 VS_Position;
layout( location = 2 ) in vec3 VS_PositionWS;
layout( location = 3 ) in vec4 VS_Color;
layout( location = 4 ) in vec4 VS_UVAlpha;

layout( binding = 0 ) uniform sampler2DArray ClipmapArray;
layout( binding = 1 ) uniform sampler2DArray NormalMapArray;

layout( binding = 3, std140 ) uniform ShadowMatrixBuffer {
    mat4 ShadowMapMatrices[ MAX_TOTAL_SHADOW_CASCADES_PER_VIEW ];
};

// Keep paddings, don't use vec3, keep in sync with cpp struct
struct SClusterLight
{
    vec4 PositionAndRadius;
    float CosHalfOuterConeAngle;
    float CosHalfInnerConeAngle;
    float InverseSquareRadius;
    float SClusterLight_Pad0;
    vec4 Direction_SpotExponent;
    vec4 Color_IESLuminousIntensityScale;
    uint LightType;
    uint RenderMask;
    uint PhotometricProfile;
    uint SClusterLight_Pad1;
};

// Helpers to access light structure fields
#define GetLightType( i ) LightBuffer[i].LightType
#define GetLightPosition( i ) LightBuffer[i].PositionAndRadius.xyz
#define GetLightRadius( i ) LightBuffer[i].PositionAndRadius.w
#define GetLightInverseSquareRadius( i ) LightBuffer[i].InverseSquareRadius
#define GetLightDirection( i ) LightBuffer[i].Direction_SpotExponent.xyz
#define GetLightCosHalfOuterConeAngle( i ) LightBuffer[i].CosHalfOuterConeAngle
#define GetLightCosHalfInnerConeAngle( i ) LightBuffer[i].CosHalfInnerConeAngle
#define GetLightSpotExponent( i ) LightBuffer[i].Direction_SpotExponent.w
#define GetLightColor( i ) LightBuffer[i].Color_IESLuminousIntensityScale.xyz
#define GetLightRenderMask( i ) LightBuffer[i].RenderMask
#define GetLightPhotometricProfile( i ) LightBuffer[i].PhotometricProfile
//#define GetLightIESScale( i ) LightBuffer[i].Color_IESLuminousIntensityScale.w

struct SClusterProbe
{
    vec4 PositionAndRadius;
    uvec4 IrradianceAndReflectionMaps;  // x - Irradiance map index, y - Reflection map index, zw - unused
};

#define MAX_LIGHTS 768
#define MAX_PROBES 256

layout( binding = 4, std140 ) uniform UniformBuffer4 {
    SClusterLight LightBuffer[MAX_LIGHTS];
};

layout( binding = 5, std140 ) uniform UniformBuffer5 {
    SClusterProbe Probes[MAX_PROBES];
};

#define SUPPORT_PHOTOMETRIC_LIGHT

#ifdef USE_VIRTUAL_TEXTURE
#if VT_LAYERS == 1
layout( binding = 6 ) uniform sampler2D vt_PhysCache0;
#elif VT_LAYERS == 2
layout( binding = 5 ) uniform sampler2D vt_PhysCache0;
layout( binding = 6 ) uniform sampler2D vt_PhysCache1;
#elif VT_LAYERS == 3
layout( binding = 4 ) uniform sampler2D vt_PhysCache0;
layout( binding = 5 ) uniform sampler2D vt_PhysCache1;
layout( binding = 6 ) uniform sampler2D vt_PhysCache2;
#elif VT_LAYERS == 4
layout( binding = 3 ) uniform sampler2D vt_PhysCache0;
layout( binding = 4 ) uniform sampler2D vt_PhysCache1;
layout( binding = 5 ) uniform sampler2D vt_PhysCache2;
layout( binding = 6 ) uniform sampler2D vt_PhysCache3;
// TODO: Add up to 8 phys cache layers
#endif
layout( binding = 7 ) uniform sampler2D vt_IndirectionTable;
#endif

#if defined WITH_SSLR && defined ALLOW_SSLR
layout( binding = 8 ) uniform sampler2D ReflectionDepth;
layout( binding = 9 ) uniform sampler2D ReflectionColor;
#endif

#ifdef SUPPORT_PHOTOMETRIC_LIGHT
layout( binding = 10 ) uniform sampler1DArray IESMap;
#endif

layout( binding = 11 ) uniform sampler2D LookupBRDF;

#if defined WITH_SSAO && defined ALLOW_SSAO
layout( binding = 12 ) uniform sampler2D AOLookup;
#endif

layout( binding = 13 ) uniform usamplerBuffer ClusterItemTBO;
layout( binding = 14 ) uniform usampler3D ClusterLookup; // TODO: Use bindless?
layout( binding = 15 ) uniform sampler2DArrayShadow ShadowMap[MAX_DIRECTIONAL_LIGHTS]; // TODO: Use bindless?

vec2  InScreenCoord;
float InScreenDepth;
vec2  InNormalizedScreenCoord;
vec3  InViewspaceToEyeVec;
vec4  InClipspacePosition;
vec2  InScreenUV;

#include "shading/parallax_mapping.frag"

#if TERRAIN_LIGHT_MODEL == TERRAIN_LIGHT_MODEL_BASELIGHT
#include "shading/baselight.frag"
#elif TERRAIN_LIGHT_MODEL == TERRAIN_LIGHT_MODEL_PBR
#include "shading/pbr.frag"
#endif

float gridTextureGradBox( in vec2 p, in vec2 ddx, in vec2 ddy )
{
    const float N = 10.0; // grid ratio

	// filter kernel
    vec2 w = max(abs(ddx), abs(ddy));// + 0.01;

	// analytic (box) filtering
    vec2 a = p + 0.5*w;                        
    vec2 b = p - 0.5*w;           
    vec2 i = (floor(a)+min(fract(a)*N,1.0)-
              floor(b)-min(fract(b)*N,1.0))/(N*w);

    //pattern
    return (1.0-i.x)*(1.0-i.y);
}

void main() {
    InViewspaceToEyeVec     = normalize( -VS_Position );
    InScreenCoord           = gl_FragCoord.xy;
    InScreenDepth           = gl_FragCoord.z;
    InNormalizedScreenCoord = InScreenCoord * GetViewportSizeInverted();
    InClipspacePosition     = vec4( InNormalizedScreenCoord * 2.0 - 1.0, InScreenDepth, 1.0 );
    InScreenUV              = vec2( InNormalizedScreenCoord.x, 1.0 - InNormalizedScreenCoord.y );
    
    InitParallaxTechnique();
    
    const float MaterialMetallic = 0;
    const float MaterialRoughness = 1;//0.5;
    const vec3 MaterialEmissive = vec3(0.0);
    const float MaterialAmbientOcclusion = 1;
    const float Opacity = 1;
    const vec3 MaterialSpecular = vec3(0.1);
    const vec3 MaterialAmbientLight = vec3(0);
    
    vec3 v = VS_PositionWS;// / 8.0;
    //vec3 v = VS_PositionWS / VertexScale.x;
    float grid = gridTextureGradBox( v.xz, dFdx(v.xz), dFdy(v.xz) );
    
    vec3 color = mix( VS_Color.xyz, vec3(0.9), vec3(1.0-grid) );
    
    //color = pow(vec3(123,130,78)/255.0,vec3(2.2));
    vec3 grass = pow(vec3(123,130,78)/255.0,vec3(2.2)); // grass
    vec3 sand = pow(vec3(177,167,132)/255.0,vec3(2.2)); // sand
    color = mix( grass, sand, clamp(v.y * 0.01,0.0,1.0) );
    //color = mix( color, vec3(1.0,0.0,0.0), clamp(VS_N.y,0.0,1.0) );
    
    //color = vec3(VS_UVAlpha.w);
    
    vec3 Normal = normalize(VS_N);

#if 0
    //MaterialNormal = normalize(cross(dFdx(VS_Position.xyz), dFdy(VS_Position.xyz)));
        
    if ( InNormalizedScreenCoord.x > 0.5 )
    {
        vec4 packedNormal = texture( NormalMapArray, VS_UVAlpha.xyz ).bgra;    
        vec4 temp = packedNormal * 2.0 - 1.0;
        vec2 normal_xz = mix( temp.xy, temp.zw, VS_UVAlpha.w );
        vec4 n = vec4( vec3( normal_xz, sqrt( 1.0 - dot( normal_xz, normal_xz ) ) ).xzy, 0.0 );
        Normal.x = dot( WorldNormalToViewSpace0, n );
        Normal.y = dot( WorldNormalToViewSpace1, n );
        Normal.z = dot( WorldNormalToViewSpace2, n );
        Normal = normalize( Normal );
        
        //Normal = normalize(cross(dFdx(VS_Position.xyz), dFdy(VS_Position.xyz)));
    }
#endif

#if TERRAIN_LIGHT_MODEL == TERRAIN_LIGHT_MODEL_BASELIGHT
    const float MaterialSpecularPower = 32; // TODO: Allow to set in the material
    MaterialBaseLightShader( color,
                             Normal,
                             Normal,
                             MaterialSpecular,
                             MaterialSpecularPower,
                             MaterialAmbientLight,
                             MaterialEmissive,
                             Opacity );
#elif TERRAIN_LIGHT_MODEL == TERRAIN_LIGHT_MODEL_PBR
    MaterialPBRShader( color,
                       Normal,
                       Normal,
                       MaterialMetallic,
                       MaterialRoughness,
                       MaterialEmissive,
                       MaterialAmbientOcclusion,
                       Opacity );
#else // TERRAIN_LIGHT_MODEL_UNLIT
    FS_FragColor = vec4( color, 1.0 );
#endif

    //if ( InNormalizedScreenCoord.x > 0.5 ) {
    //    FS_FragColor = vec4(VS_UV,0.0,1.0);
    //}
}
