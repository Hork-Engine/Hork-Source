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

#include "base/viewuniforms.glsl"

layout( location = 0 ) out vec2 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;
        
layout( binding = 0 ) uniform sampler2D Smp_Current;
layout( binding = 1 ) uniform sampler2D Smp_Desired;

void main() {
    vec2 Current = texelFetch( Smp_Current, ivec2( 0 ), 0 ).rg;
    vec2 Desired;

    vec2 a = texelFetch( Smp_Desired, ivec2( 0, 0 ), 0 ).rg;
    vec2 b = texelFetch( Smp_Desired, ivec2( 1, 0 ), 0 ).rg;
    vec2 c = texelFetch( Smp_Desired, ivec2( 1, 1 ), 0 ).rg;
    vec2 d = texelFetch( Smp_Desired, ivec2( 0, 1 ), 0 ).rg;

    const float PixelsCountDenom = 1.0 / (64*64);
#if 0
    Desired.x = exp( (a.x + b.x + c.x + d.x) * PixelsCountDenom );
    Desired.y = max( Desired.x + 0.001, max( max( a.y, b.y ), max( c.y, d.y ) ) );

    //const float Speed = 0.2; // Eye adaptation speed
    //fragColor = mix( Current, Desired, 1.0 - exp(-TimeStep.x*Speed) );

    const float Speed = 20; // Eye adaptation speed
    FS_FragColor = mix( Current, Desired, 1.0 - pow( 0.98, Speed * GameplayFrameDelta() ) );
#endif
    const float eyeAdaptationSpeed = 20;
    float frameLuminanceAvg = (a.x + b.x + c.x + d.x) * PixelsCountDenom;
    float frameLuminanceMax = max( max( a.y, b.y ), max( c.y, d.y ) );
    FS_FragColor.x = mix( Current.x, frameLuminanceAvg, 1.0 - pow( 0.98, eyeAdaptationSpeed * GameplayFrameDelta() ) );

//  float delta=0;
//  if ( abs( frameLuminanceAvg - Current.x ) > 0.4 ) {
//      if ( frameLuminanceAvg - Current.x > 0 ) {
//          delta = Current.x + 0.4;
//      } else {
//          delta = Current.x - 0.4;
//      }
//  } else {
//      delta = frameLuminanceAvg;
//  }
//  FS_FragColor.x = mix( Current.x, delta, min(eyeAdaptationSpeed * GameplayFrameDelta()*0.01,1.0) );

    FS_FragColor.y = 0;
}
