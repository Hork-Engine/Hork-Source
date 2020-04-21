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

layout( binding = 0, std140 ) uniform UniformBuffer0
{
    // Ortho projection for canvas rendering
    mat4 OrthoProjection;
    
    // View projection matrix: VewProjection = ProjectionMatrix * ViewMatrix
    mat4 ViewProjection;
    
    // Inversed projection matrix
    mat4 InverseProjectionMatrix;
    
    // Conversion of normal from world to view space
    vec4 WorldNormalToViewSpace0;
    vec4 WorldNormalToViewSpace1;
    vec4 WorldNormalToViewSpace2;
    
    // Viewport parameters:
    // x = 1/width
    // y = 1/height
    // z = znear
    // w = zfar
    vec4 ViewportParams;
    
    // Timers
    // x = running time
    // y = gampley timw
    // zw = dynamic resolution ratio
    vec4 Timers;
    
    // View position and frametime delta
    // xyz = ViewPosition
    // w = frametime delta
    vec4 ViewPosition;
    
    vec4 PostprocessBloomMix;
    
    // Postprocess attributes
    // x - bloom enable / disable
    // y - tone mapping exposure (disabled if zero)
    // z - color grading enable / disable
    // w - FXAA enable /disable
    vec4 PostprocessAttrib;
    
    // Vignette
    // xyz - color
    // w - intensity
    vec4 VignetteColorIntensity;
    
    // Vignette
    // x - outer radius^2
    // y - inner radius^2
    // z - brightness
    // w - colorgrading blend
    vec4 VignetteOuterInnerRadiusSqr;
    
    uvec2 EnvProbeSampler;
    
    uvec4 NumDirectionalLights;  // W - DebugMode, YZ - unused
    
    vec4 LightDirs[MAX_DIRECTIONAL_LIGHTS];            // Direction, W-channel is not used
    
    vec4 LightColors[MAX_DIRECTIONAL_LIGHTS];          // RGB, alpha - ambient intensity
    
    uvec4 LightParameters[MAX_DIRECTIONAL_LIGHTS];    // RenderMask, FirstCascade, NumCascades, W-channel is not used
};


/*


Some helper functions


*/

float GetViewportWidthInverted() {
    return ViewportParams.x;
}

float GetViewportHeightInverted() {
    return ViewportParams.y;
}

vec2 GetViewportSizeInverted() {
    return ViewportParams.xy;
}

float GetViewportZNear() {
    return ViewportParams.z;
}

float GetViewportZFar() {
    return ViewportParams.w;
}

float RunningTime() {
    return Timers.x;
}

float GameplayTime() {
    return Timers.y;
}

float GameplayFrameDelta() {
    return ViewPosition.w;
}

vec3 GetViewPosition() {
     return ViewPosition.xyz;
}

vec2 GetDynamicResolutionRatio() {
    return Timers.zw;
}

float GetPostprocessExposure() {
    return PostprocessAttrib.y;
}

float GetFrameBrightness() {
    return VignetteOuterInnerRadiusSqr.z;
}

float GetColorGradingBlend() {
    return VignetteOuterInnerRadiusSqr.w;
}

#define IsBloomEnabled() ( PostprocessAttrib.x > 0.0 )
#define IsTonemappingEnabled() ( PostprocessAttrib.y > 0.0 )
#define IsColorGradingEnabled() ( PostprocessAttrib.z > 0.0 )
#define IsFXAAEnabled() ( PostprocessAttrib.w > 0.0 )
#define IsVignetteEnabled() ( VignetteColorIntensity.a > 0.0 )

#endif // VIEWUNIFORMS_H
