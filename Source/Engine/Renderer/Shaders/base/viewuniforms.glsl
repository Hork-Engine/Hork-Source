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

#ifndef VIEWUNIFORMS_H
#define VIEWUNIFORMS_H

// Keep paddings, don't use vec3, keep in sync with cpp struct
layout( binding = 0, std140 ) uniform UniformBuffer0
{
    // Ortho projection for canvas rendering
    mat4 OrthoProjection;
    
    // View projection matrix: ViewProjection = ProjectionMatrix * ViewMatrix
    mat4 ViewProjection;

    // Projection matrix
    mat4 ProjectionMatrix;
    
    // Inversed projection matrix
    mat4 InverseProjectionMatrix;
    
    // TODO: Add ViewMatrix
    
    mat4 InverseViewMatrix;

    // Reprojection from viewspace to previous frame projected coordinates:
    // ReprojectionMatrix = ProjectionMatrixPrevFrame * WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    mat4 ReprojectionMatrix;

    // Reprojection from viewspace to previous frame viewspace coordinates:
    // ViewspaceReprojection = WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    mat4 ViewspaceReprojection;
    
    // Conversion of normal from world to view space
    vec4 WorldNormalToViewSpace0;
    vec4 WorldNormalToViewSpace1;
    vec4 WorldNormalToViewSpace2;
    
    // Viewport parameters
    vec2 InvViewportSize;
    float ViewportZNear;
    float ViewportZFar;
    
    // Projection offset and scale
    vec4 ProjectionInfo;
    
    // Timers
    float GameRunningTimeSeconds;
    float GameplayTimeSeconds;
    
    vec2 DynamicResolutionRatio;
    
    vec2 FeedbackBufferResolutionRatio;
    vec2 VTPageCacheCapacity;
    vec4 VTPageTranslationOffsetAndScale;
    
    // View position and frametime delta
    // xyz = ViewPosition
    // w = frametime delta
    vec4 ViewPosition;
    
    vec4 PostprocessBloomMix;
    
    // Postprocess attributes
    //  bloom enable / disable
    float BloomEnabled;
    // tone mapping exposure (disabled if zero)
    float ToneMappingExposure;
    // color grading enable / disable
    float ColorGrading;
    // FXAA enable /disable
    float FXAA;
    
    // Vignette
    // xyz - color
    // w - intensity
    vec4 VignetteColorIntensity;
    
    // Vignette outer radius^2
    float VignetteOuterRadiusSqr;
    // Vignette inner radius^2
    float VignetteInnerRadiusSqr;
    
    // Brightness
    float ViewBrightness;
    
    // Colorgrading adaptation speed
    float ColorGradingAdaptationSpeed;
    
    float SSLRSampleOffset;
    float SSLRMaxDist;
    float IsPerspective;
    float TessellationLevel;
    
    uvec2 PrefilteredMapSampler;
    uvec2 IrradianceMapSampler;
    
    uvec4 NumDirectionalLights;  // W - DebugMode, YZ - unused
    
    vec4 LightDirs[MAX_DIRECTIONAL_LIGHTS];            // Direction, W-channel is not used
    
    vec4 LightColors[MAX_DIRECTIONAL_LIGHTS];          // RGB, alpha - ambient intensity
    
    uvec4 LightParameters[MAX_DIRECTIONAL_LIGHTS];    // RenderMask, FirstCascade, NumCascades, W-channel is not used
};


/*


Some helper functions


*/

float GetViewportWidthInverted() {
    return InvViewportSize.x;
}

float GetViewportHeightInverted() {
    return InvViewportSize.y;
}

vec2 GetViewportSizeInverted() {
    return InvViewportSize;
}

float GetViewportZNear() {
    return ViewportZNear;
}

float GetViewportZFar() {
    return ViewportZFar;
}

float RunningTime() {
    return GameRunningTimeSeconds;
}

float GameplayTime() {
    return GameplayTimeSeconds;
}

float GameplayFrameDelta() {
    return ViewPosition.w;
}

vec3 GetViewPosition() {
     return ViewPosition.xyz;
}

vec2 GetDynamicResolutionRatio() {
    return DynamicResolutionRatio;
}

float GetPostprocessExposure() {
    return ToneMappingExposure;
}

float GetFrameBrightness() {
    return ViewBrightness;
}

float GetColorGradingAdaptationSpeed() {
    return ColorGradingAdaptationSpeed;
}

#define IsBloomEnabled() ( BloomEnabled > 0.0 )
#define IsTonemappingEnabled() ( ToneMappingExposure > 0.0 )
#define IsColorGradingEnabled() ( ColorGrading > 0.0 )
#define IsFXAAEnabled() ( FXAA > 0.0 )
#define IsVignetteEnabled() ( VignetteColorIntensity.a > 0.0 )

uint GetDebugMode() {
    return NumDirectionalLights.w;
}

uint GetNumDirectionalLights() {
    return NumDirectionalLights.x;
}

// Adjust texture coordinates for dynamic resolution
vec2 AdjustTexCoord( in vec2 TexCoord ) {
    vec2 tc = min( TexCoord, vec2(1.0) - GetViewportSizeInverted() ) * GetDynamicResolutionRatio();
    tc.y = 1.0 - tc.y;
    return tc;
}

vec3 UVToView( vec2 TexCoord, float ViewZ )
{
#if 0
    const float znear = GetViewportZNear();
    const float zfar = GetViewportZFar();
    float d = (znear * zfar/-ViewZ - znear)/( zfar - znear ); // delinearize perspective depth
    
    vec4 position;
    position.x = TexCoord.x * 2.0f - 1.0f; 
    position.y = -(TexCoord.y * 2.0f - 1.0f); 
    position.z = d;
    position.w = 1;
    
    position = InverseProjectionMatrix * position; 
 
    position /= position.w;

    return position.xyz;
#else
    return vec3( ( TexCoord * ProjectionInfo.xy + ProjectionInfo.zw ) * mix( 1.0, ViewZ, IsPerspective ), ViewZ );
#endif
}

vec3 UVToView_ORTHO( vec2 TexCoord, float ViewZ )
{
    return vec3( ( TexCoord * ProjectionInfo.xy + ProjectionInfo.zw ), ViewZ );
}

vec3 UVToView_PERSPECTIVE( vec2 TexCoord, float ViewZ )
{
    return vec3( ( TexCoord * ProjectionInfo.xy + ProjectionInfo.zw ) * ViewZ, ViewZ );
}

vec2 ViewToUV( vec3 p )
{
#if 0
    vec4 v = ProjectionMatrix * vec4(p, 1.0f);
    v.xy = vec2(0.5f, 0.5f) + vec2(0.5f, -0.5f) * v.xy / v.w;
    return v.xy;
#else
    return ( p.xy / mix( 1.0, p.z, IsPerspective ) - ProjectionInfo.zw ) / ProjectionInfo.xy;
#endif
}

vec2 ViewToUVReproj( vec3 p )
{
    vec4 v = ReprojectionMatrix * vec4(p, 1.0);
    v.xy = (v.xy / v.w) * vec2(0.5, -0.5) + 0.5;
    return v.xy;
}

#endif // VIEWUNIFORMS_H

