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

layout( location = 0 ) out vec4 FS_FragColor; 

layout( location = 0 ) in vec3 GS_Normal;

// Envmap must be HDRI in linear space
layout( binding = 0 ) uniform samplerCube Envmap;

layout( binding = 0, std140 ) uniform UniformBlock
{
    mat4 uTransform[6];
    vec4 uIndex;
};

vec3 CalcIrradiance( samplerCube environmentMap, vec3 Normal ) {
    vec3 Irradiance = vec3(0.0);  

    vec3 Up = vec3( 0.0, 1.0, 0.0 );
    vec3 Right = cross( Up, Normal );
    Up = cross( Normal, Right );

    float NumSamples = 0.0; 
    
    const float _2PI = 2.0 * PI;
    const float HalfPI = 0.5 * PI;
    const float SampleDelta = 0.025;
    
    for ( float Phi = 0.0 ; Phi < _2PI ; Phi += SampleDelta ) {
        const float sp = sin( Phi );
        const float cp = cos( Phi );
            
        for ( float Theta = 0.0 ; Theta < HalfPI ; Theta += SampleDelta ) {
            const float st = sin( Theta );
            const float ct = cos( Theta );
            
            const vec3 tangentSample = vec3( st * cp,  st * sp, ct );
            const vec3 sampleVec = tangentSample.x * Right + tangentSample.y * Up + tangentSample.z * Normal; 

            Irradiance += texture( environmentMap, sampleVec ).rgb * ( ct * st );
            NumSamples++;
        }
    }
    Irradiance = Irradiance * ( PI / NumSamples );
    return Irradiance;
}

void main() {
    FS_FragColor = vec4( CalcIrradiance( Envmap, normalize( GS_Normal ) ), 1.0 );
}
