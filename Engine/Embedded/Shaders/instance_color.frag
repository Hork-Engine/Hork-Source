/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#include "base/debug.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( origin_upper_left ) in vec4 gl_FragCoord;


// Built-in samplers
#include "$COLOR_PASS_FRAGMENT_SAMPLERS$"

// Built-in varyings
#include "$COLOR_PASS_FRAGMENT_INPUT_VARYINGS$"

layout( binding = 3, std140 ) uniform ShadowMatrixBuffer {
    mat4 ShadowMapMatrices[ MAX_TOTAL_SHADOW_CASCADES_PER_VIEW ];
};

// Keep paddings, don't use vec3, keep in sync with cpp struct
struct SClusterLight // TODO: rename to SLightParameters
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
    uint ShadowmapIndex;
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
#define GetLightShadowmapIndex( i ) LightBuffer[i].ShadowmapIndex
//#define GetLightIESScale( i ) LightBuffer[i].Color_IESLuminousIntensityScale.w

struct SClusterProbe
{
    vec4 PositionAndRadius;
    uvec4 IrradianceAndReflectionMaps;  // xy - Irradiance map index, zw - Reflection map index
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
layout( binding = 19 ) uniform samplerCubeArray OmnidirectionalShadowMapArray;


#ifdef USE_LIGHTMAP
layout( binding  = COLOR_PASS_TEXTURE_LIGHTMAP ) uniform sampler2D tslot_lightmap;
layout( location = COLOR_PASS_VARYING_BAKED_LIGHT ) in vec2 VS_LightmapTexCoord;
#endif

#ifdef USE_VERTEX_LIGHT
layout( location = COLOR_PASS_VARYING_BAKED_LIGHT ) in vec3 VS_VertexLight;
#endif

#ifdef COMPUTE_TBN
layout( location = COLOR_PASS_VARYING_TANGENT  ) in vec3 VS_T;
layout( location = COLOR_PASS_VARYING_BINORMAL ) in vec3 VS_B;
layout( location = COLOR_PASS_VARYING_NORMAL   ) in vec3 VS_N;
layout( location = COLOR_PASS_VARYING_POSITION ) in vec3 VS_Position;
#endif

#ifdef USE_VIRTUAL_TEXTURE
layout( location = COLOR_PASS_VARYING_VT_TEXCOORD ) in vec2 VS_TexCoordVT;
#endif

/*

Public variables

*/


vec2  InScreenCoord;
float InScreenDepth;
vec2  InNormalizedScreenCoord;
vec3  InViewspaceToEyeVec;
vec4  InClipspacePosition;
vec2  InScreenUV;

#ifdef USE_VIRTUAL_TEXTURE
vec2  InPhysicalUV;
#endif

#ifdef USE_VIRTUAL_TEXTURE
#include "virtualtexture.frag"
#endif

#include "shading/parallax_mapping.frag"

#ifdef MATERIAL_TYPE_PBR
#include "shading/pbr.frag"
#endif

#ifdef MATERIAL_TYPE_BASELIGHT
#include "shading/baselight.frag"
#endif

float getSoftStripes(float value, float gridSize, float stripeSize) {
    float mainVal = value * gridSize;
    float filterWidth = fwidth(value);
    float edge = filterWidth * gridSize * 2.0;
    
    // major line shading, currently set to place a major line every 64 units
    float mValue = 1.0 / (64.0 * gridSize);
    float triMajor = abs(2.0 * fract(mainVal * mValue) - 1.0);
    float isMajor = step(1.0 - mValue, triMajor);
    
    float outIntensity = isMajor * 0.7 + 0.85; // tweak intensities here
    float sSize = stripeSize;
    
    float triangle = abs(2.0 * fract(mainVal) - 1.0);
    return smoothstep(sSize - edge, sSize + edge, triangle) * outIntensity;
}

/**
 * Draws two overlaid grids
 * 
 * @param inCoords fragment coordinates (TODO: document units)
 * @param gridRatio the reciprocal of the first grid size, e.g. (1/16) for a 16 unit grid
 * @param gridRatio2 the reciprocal of the second grid size, e.g. (1/16) for a 16 unit grid
 * @param lineWidth the line width (TODO: document units)
 * @param gridBlend the fraction of the two grids to draw, between 0 and 1, 
 *                  where 0.0 means 100% of grid one, and 1.0 means 100% of grid two.
 */
float gridLinesSoft(vec2 inCoords, float gridRatio, float gridRatio2, float lineWidth, float gridBlend) {
    float stripeRatio = lineWidth * gridRatio;
    float stripeRatio2 = lineWidth * gridRatio2;
    float stripeSize = 1.0 - stripeRatio;
    float stripeSize2 = 1.0 - stripeRatio2;
    
    float theGrid, nextGrid;
    
    theGrid = getSoftStripes(inCoords.x, gridRatio, stripeSize);
    theGrid = max(theGrid, getSoftStripes(inCoords.y, gridRatio, stripeSize));
    nextGrid = getSoftStripes(inCoords.x, gridRatio2, stripeSize2);
    nextGrid = max(nextGrid, getSoftStripes(inCoords.y, gridRatio2, stripeSize2));
    
    theGrid = mix(theGrid, nextGrid, gridBlend);
    
    return theGrid * 0.5;
}

/**
 * Given a valid grid size, returns the next larger one.
 */
float gridNextLarger(float size) {
    return 2.0 * size;
}

/**
 * Given any size, finds the next smaller size that is a proper grid size.
 * Returns the input unmodified if it's already a grid size.
 */
float gridFloor(float size) {
    return exp2(floor(log2(size)));
}

/*
 * Computes the grid for the current fragment with the given parameters.
 *
 * @param coords the coordinates of the fragment in world space (same units as gridSize)
 * @param normal the normal vector of the fragment
 * @param gridSize the actual size of the grid (e.g. 16, 32, 64, etc.)
 * @param minGridSize the minimal grid size to render (to fade out smaller grids to prevent moire etc.)
 * @param lineWidthFactor a factor for the line width of the grid
 *
 * @return opacity of the grid line to draw on this fragment, in [0..1]
 */
float grid(vec3 coords, vec3 normal, float gridSize, float minGridSize, float lineWidthFactor) {
    float lineWidth = (gridSize < 1.0 ? (1.0 / 32.0)
                       : (gridSize < 4.0 ? 0.25 : 0.5)) * lineWidthFactor;

    // magic number to make the grid fade sooner, preventing aliasing
    float minGridSizeToRender = minGridSize * 2.0;

    float baseGridSize = gridFloor(minGridSizeToRender);
    if (gridSize > baseGridSize) {
        baseGridSize = gridSize;
    }
    float nextGridSize = gridNextLarger(baseGridSize);
    
    // This is 0 if we want to render just baseGridSize, and 1 if we want to fully render at nextGridSize
    float gridBlend = smoothstep(baseGridSize, nextGridSize, minGridSizeToRender);

    float gridRatio = 1.0 / baseGridSize;
    float gridRatio2 = 1.0 / nextGridSize;
    
    vec2 baseCoords; // coordinates used for overlay creation
    
    if (abs(normal.x) > abs(normal.y)) {
        if (abs(normal.x) > abs(normal.z))
            baseCoords = coords.yz;
        else
            baseCoords = coords.xy;
    } else if (abs(normal.y) > abs(normal.z)) {
        baseCoords = coords.xz;
    } else {
        baseCoords = coords.xy;
    }

    return gridLinesSoft(baseCoords, gridRatio, gridRatio2, lineWidth, gridBlend);
}


	
void main()
{
#ifdef COMPUTE_TBN
    InViewspaceToEyeVec     = normalize( -VS_Position );
#endif

    InScreenCoord           = gl_FragCoord.xy;
    InScreenDepth           = gl_FragCoord.z;
    InNormalizedScreenCoord = InScreenCoord * GetViewportSizeInverted();
    InClipspacePosition     = vec4( InNormalizedScreenCoord * 2.0 - 1.0, InScreenDepth, 1.0 );
    InScreenUV              = vec2( InNormalizedScreenCoord.x, 1.0 - InNormalizedScreenCoord.y );
    
#ifdef USE_VIRTUAL_TEXTURE
    // FIXME: Parallax mapping with VT?
    InPhysicalUV = VT_CalcPhysicalUV( vt_IndirectionTable, VS_TexCoordVT, VTUnit );
#endif

    InitParallaxTechnique();

    // Built-in material code
#   include "$COLOR_PASS_FRAGMENT_CODE$"

    // Calc nomal
#if defined( MATERIAL_TYPE_PBR ) || defined( MATERIAL_TYPE_BASELIGHT )
    #ifdef TWOSIDED
    const vec3 Normal = normalize( MaterialNormal.x * VS_T + MaterialNormal.y * VS_B + MaterialNormal.z * VS_N ) * ( 1.0 - float(gl_FrontFacing) * 2.0 );
    #else
    const vec3 Normal = normalize( MaterialNormal.x * VS_T + MaterialNormal.y * VS_B + MaterialNormal.z * VS_N );
    #endif
#endif

#ifdef MATERIAL_TYPE_PBR
    MaterialPBRShader( BaseColor.xyz,
                       Normal,
                       VS_N,
                       MaterialMetallic,
                       MaterialRoughness,
                       MaterialEmissive,
                       MaterialAmbientOcclusion,
                       Opacity );
#endif

#ifdef MATERIAL_TYPE_BASELIGHT
    const float MaterialSpecularPower = 32; // TODO: Allow to set in the material
    MaterialBaseLightShader( BaseColor.xyz,
                             Normal,
                             VS_N,
                             MaterialSpecular,
                             MaterialSpecularPower,
                             MaterialAmbientLight,
                             MaterialEmissive,
                             Opacity );
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


#if defined(MATERIAL_TYPE_PBR) || defined (MATERIAL_TYPE_BASELIGHT)
// Transform vector from view space to world space
    const mat3 VectorTransformWS = mat3( vec3(WorldNormalToViewSpace0),
                                            vec3(WorldNormalToViewSpace1),
                                            vec3(WorldNormalToViewSpace2)
                                           );
    vec3 norm = normalize(VectorTransformWS * VS_N);
	
	vec2 baseCoords;
	vec3 coords = (InverseViewMatrix * vec4(VS_Position, 1.0)).xyz;
	
	norm = floor(norm*10000)/10000;

	float gridSize = 16;
	float minGridSize = 4;
	float lineWidthFactor = 1;
	
	float line = grid(coords * 32, norm, gridSize, minGridSize, lineWidthFactor);

	//FS_FragColor.rgb = mix(FS_FragColor.rgb, vec3(1.0), line);
#endif
}