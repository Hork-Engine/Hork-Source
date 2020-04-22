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
#include "base/pbr.frag"
#include "base/debug.glsl"

layout( location = 0 ) out vec4 FS_FragColor;
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
    vec4 Pack;        // For point and spot lights: position and radius
    vec4 Pack2;       // Pack2.x - light type, Pack2.y - inner radius, z - cone outer angle, w - cone inner angle
    vec4 Pack3;       // Pack3.xyz - direction of spot light, w - spot exponent
    vec4 Color;       // RGB, alpha - ambient intensity
    uvec4 IPack;       // IPack.x - RenderMask
};

#define MAX_LIGHTS 768

layout( binding = 4, std140 ) uniform UniformBuffer4 {
    SClusterLight LightBuffer[MAX_LIGHTS];
};

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
    MaterialBaseLightShader( BaseColor.xyz, MaterialNormal, MaterialSpecular, MaterialAmbientLight, MaterialEmissive, Opacity );
#endif

#ifdef MATERIAL_TYPE_UNLIT
    FS_FragColor = vec4( BaseColor.xyz, Opacity );
#endif

#ifdef MATERIAL_TYPE_HUD
    FS_FragColor = BaseColor;
#endif

#ifdef MATERIAL_TYPE_POSTPROCESS
    FS_FragColor = BaseColor;
#endif
}
