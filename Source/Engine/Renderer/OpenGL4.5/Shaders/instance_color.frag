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

#include "base/common.frag"
#include "base/debug.glsl"

layout( location = 0 ) out vec4 FS_FragColor;
/*layout( location = 1 ) out */vec3 FS_Normal;
layout( origin_upper_left ) in vec4 gl_FragCoord;


// Built-in samplers
#include "$COLOR_PASS_FRAGMENT_SAMPLERS$"

// Built-in varyings
#include "$COLOR_PASS_FRAGMENT_INPUT_VARYINGS$"

layout( binding = 3, std140 ) uniform ShadowMatrixBuffer {
    mat4 CascadeViewProjection[ MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES ];
    mat4 ShadowMapMatrices[ MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES ];
};

struct SClusterLight
{
    vec4 PositionAndRadius;
    vec4 CosHalfOuterConeAngle_CosHalfInnerConeAngle_InverseSquareRadius_Unused;
    vec4 Direction_SpotExponent;
    vec4 Color_IESLuminousIntensityScale;
    uvec4 LightType_RenderMask_PhotometricProfile_Unused_Unused;
};

// Helpers to access light structure fields
#define GetLightType( i ) LightBuffer[i].LightType_RenderMask_PhotometricProfile_Unused_Unused.x
#define GetLightPosition( i ) LightBuffer[i].PositionAndRadius.xyz
#define GetLightRadius( i ) LightBuffer[i].PositionAndRadius.w
#define GetLightInverseSquareRadius( i ) LightBuffer[i].CosHalfOuterConeAngle_CosHalfInnerConeAngle_InverseSquareRadius_Unused.z
#define GetLightDirection( i ) LightBuffer[i].Direction_SpotExponent.xyz
#define GetLightCosHalfOuterConeAngle( i ) LightBuffer[i].CosHalfOuterConeAngle_CosHalfInnerConeAngle_InverseSquareRadius_Unused.x
#define GetLightCosHalfInnerConeAngle( i ) LightBuffer[i].CosHalfOuterConeAngle_CosHalfInnerConeAngle_InverseSquareRadius_Unused.y
#define GetLightSpotExponent( i ) LightBuffer[i].Direction_SpotExponent.w
#define GetLightColor( i ) LightBuffer[i].Color_IESLuminousIntensityScale.xyz
#define GetLightRenderMask( i ) LightBuffer[i].LightType_RenderMask_PhotometricProfile_Unused_Unused.y
#define GetLightPhotometricProfile( i ) LightBuffer[i].LightType_RenderMask_PhotometricProfile_Unused_Unused.z
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

#ifdef SUPPORT_PHOTOMETRIC_LIGHT
layout( binding = 10 ) uniform sampler1DArray IESMap;
#endif

layout( binding = 11 ) uniform sampler2D LookupBRDF;
layout( binding = 12 ) uniform sampler2D AOLookup;
layout( binding = 13 ) uniform usamplerBuffer ClusterItemTBO;
layout( binding = 14 ) uniform usampler3D ClusterLookup; // TODO: Use bindless?
layout( binding = 15 ) uniform sampler2DArrayShadow ShadowMapShadow; // TODO: Use bindless?


#ifdef USE_LIGHTMAP
layout( binding  = LIGHTMAP_SLOT ) uniform sampler2D tslot_lightmap;
layout( location = BAKED_LIGHT_LOCATION ) in vec2 VS_LightmapTexCoord;
#endif

#ifdef USE_VERTEX_LIGHT
layout( location = BAKED_LIGHT_LOCATION ) in vec3 VS_VertexLight;
#endif

#ifdef COMPUTE_TBN
layout( location = TANGENT_LOCATION  ) in vec3 VS_T;
layout( location = BINORMAL_LOCATION ) in vec3 VS_B;
layout( location = NORMAL_LOCATION   ) in vec3 VS_N;
layout( location = POSITION_LOCATION ) in vec3 VS_Position;
#endif


/*

Public variables

*/


vec2  InScreenCoord;
float InScreenDepth;
vec2  InNormalizedScreenCoord;
vec3  InViewspaceToEyeVec;
vec4  InClipspacePosition;


#ifdef MATERIAL_TYPE_PBR
#include "material_type_pbr.frag"
#endif

#ifdef MATERIAL_TYPE_BASELIGHT
#include "material_type_baselight.frag"
#endif

void main()
{
#ifdef COMPUTE_TBN
    InViewspaceToEyeVec     = normalize( -VS_Position );
#endif
    InScreenCoord           = gl_FragCoord.xy;
    InScreenDepth           = gl_FragCoord.z;
    InNormalizedScreenCoord = InScreenCoord * GetViewportSizeInverted();
    InClipspacePosition     = vec4( InNormalizedScreenCoord * 2.0 - 1.0, InScreenDepth, 1.0 );

    // Built-in material code
#   include "$COLOR_PASS_FRAGMENT_CODE$"

#ifdef MATERIAL_TYPE_PBR
    MaterialPBRShader( BaseColor.xyz, MaterialNormal, MaterialMetallic, MaterialRoughness, MaterialEmissive, MaterialAmbientOcclusion, Opacity );
#endif

#ifdef MATERIAL_TYPE_BASELIGHT
    const float MaterialSpecularPower = 32; // TODO: Allow to set in the material
    MaterialBaseLightShader( BaseColor.xyz, MaterialNormal, MaterialSpecular, MaterialSpecularPower, MaterialAmbientLight, MaterialEmissive, Opacity );
#endif

#ifdef MATERIAL_TYPE_UNLIT
    FS_FragColor = vec4( BaseColor.xyz, Opacity );
    FS_Normal = vec3(0.0); // FIXME
#endif

#ifdef MATERIAL_TYPE_HUD
    FS_FragColor = BaseColor;
    FS_Normal = vec3(0.0); // FIXME
#endif

#ifdef MATERIAL_TYPE_POSTPROCESS
    FS_FragColor = BaseColor;
    FS_Normal = vec3(0.0); // FIXME
#endif
}
