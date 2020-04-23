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

#include "base/core.glsl"
#include "base/srgb.glsl"
#include "base/viewuniforms.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) in vec3 GS_Color;

/*
TODO:
layout( binding = 1, std140 ) uniform UniformBuffer1
{
    vec3 uTemperatureScale;
    vec3 uTemperatureStrength;
    vec3 uGrain;
    vec3 uGamma;
    vec3 uLift;
    vec3 uPresaturation;
    float uLuminanceNormalization;
};
*/

void main()
{
    // Source color
    vec3 color = pow( GS_Color, vec3(2.2) );

    // Source luminance
    float luminance = builtin_luminance( color );
    
    // Apply temperature
    vec3 temperatureColor = color * uTemperatureScale.xyz;
    
    // Lerp between source and scaled color
    color = mix( color, temperatureColor, uTemperatureStrength.xyz );
    
    // Calc new luminance
    float luminance2 = builtin_luminance( color );
    
    // Calc luminance scale
    float luminanceRatio = ( luminance2 > 1e-6 ) ? ( luminance / luminance2 ) : 1.0;
    
    float brightness = mix( 1.0, luminanceRatio, uLuminanceNormalization.x );    
    color *= brightness;
    
    // Calc grayscaled color
    vec3 grayscaled = vec3( builtin_luminance( color ) );
    
    // Lerp between rgb and grayscaled
    color = mix( grayscaled, color, uPresaturation.xyz );
    
    // Apply gamma
    color = pow( uGrain.xyz * ( color - uLift.xyz * ( color - 1.0 ) ), uGamma.xyz );

    const float eyeAdaptationSpeed = GetColorGradingAdaptationSpeed();
    
    FS_FragColor = vec4( color,
                         clamp( eyeAdaptationSpeed * GameplayFrameDelta(), 0.0001, 1.0 )
                         //1.0 - pow( 0.98, eyeAdaptationSpeed * GameplayFrameDelta() )                         
                         );
}
